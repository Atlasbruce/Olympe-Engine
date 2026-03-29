# Visual Architecture Diagrams - Data Pin Stack Evaluation

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                      VSGraphExecutor                             │
│                   (Main Execution Engine)                        │
└────────────────┬────────────────────────────────────────────────┘
                 │
                 ├─ ExecuteFrame(entity, runner, template, ...)
                 │   │
                 │   ├─ for each step in frame:
                 │   │   ├─ [NEW] ResolveDataPins(nodeID)
                 │   │   │   │
                 │   │   │   └─→ ┌────────────────────────────────┐
                 │   │   │       │  DataPinEvaluator              │
                 │   │   │       │  (New Phase 24.1)              │
                 │   │   │       │                                │
                 │   │   │       │  EvaluateNodeInputPins()       │
                 │   │   │       │    └─ Recursive DFS            │
                 │   │   │       │       evaluation of            │
                 │   │   │       │       data pin network         │
                 │   │   │       └────────────────────────────────┘
                 │   │   │
                 │   │   ├─ Execute Node (uses cached data pins)
                 │   │   └─ Advance to next node
                 │   │
                 │   └─ Return (frame complete)
                 │
                 └─ Used by TaskSystem::Process()
```

---

## Data Flow Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    Execution Frame Start                      │
│              CurrentNodeID = SetBBValue                       │
└─────────────────────────┬──────────────────────────────────┘
                          │
                          ▼
          ┌───────────────────────────────┐
          │   ResolveDataPins(nodeID)     │
          │   (VSGraphExecutor.cpp)       │
          └───────────────┬───────────────┘
                          │
                          ▼
    ┌─────────────────────────────────────────────┐
    │ DataPinEvaluator::EvaluateNodeInputPins()   │
    │                                             │
    │ Iterate incoming DataPinConnections:        │
    │ for (MathOp, "Result") → (SetBBValue, "Value")
    └────────────────┬────────────────────────────┘
                     │
                     ▼
    ┌──────────────────────────────────────────────────┐
    │ EvaluateDataPin(MathOp, "Result")                │
    │                                                  │
    │  → Recursive DFS traversal                       │
    │  → Check cache (miss first time)                 │
    │  → Detect cycles (none)                          │
    │  → Resolve operands recursively                  │
    │     ├─ Evaluate (Variable, "mHealth")            │
    │     │  └─ Return localBB.GetValue("mHealth")=10 │
    │     ├─ Evaluate (Variable, "mFood")              │
    │     │  └─ Return localBB.GetValue("mFood")=5    │
    │     └─ Compute 10 + 5 = 15                       │
    │  → Cache result: MathOp:Result = 15              │
    │  → Return 15                                     │
    └────────────┬─────────────────────────────────────┘
                 │
                 ▼
    ┌──────────────────────────────────────────────────┐
    │ Store in Cache:                                  │
    │ runner.DataPinCache["SetBBValue:Value"] = 15    │
    └────────────┬─────────────────────────────────────┘
                 │
                 ▼
    ┌──────────────────────────────────────────────────┐
    │ HandleSetBBValue(nodeID)                         │
    │                                                  │
    │ value = cache["SetBBValue:Value"] = 15          │
    │ localBB.SetValue("mResult", 15)                 │
    │ return FindExecTarget("Completed")               │
    └────────────┬─────────────────────────────────────┘
                 │
                 ▼
         ┌───────────────────────┐
         │ Advance to next node   │
         │ (Execution continues)  │
         └───────────────────────┘
```

---

## Recursive Evaluation - Call Stack

```
EvaluateDataPin(SetBBValue, "Value")
│
└─→ EvaluateRecursive(SetBBValue:Value, depth=0)  ──┐ Not data pure
    │                                                │ Skip
    │
    └─ for connection (MathOp:Result) → (SetBBValue:Value):
       │
       └─→ EvaluateDataPin(MathOp, "Result")  ──┐ IS data pure
           │                                     │ Evaluate!
           │
           └─→ EvaluateRecursive(MathOp:Result, depth=0)
               │
               ├─ Visited: {}
               ├─ Add MathOp to visited: {MathOp}
               ├─ Type: MathOp
               │
               └─ HandleMathOpNode(MathOp)
                  │
                  ├─ Operand A: Pin mode
                  │  │
                  │  └─ for connection (Variable:Value) → (MathOp:A):
                  │     │
                  │     └─→ EvaluateDataPin(Variable, "Value")
                  │         │
                  │         └─→ EvaluateRecursive(Variable:Value, depth=1)
                  │             │
                  │             ├─ Visited: {MathOp}
                  │             ├─ Add Variable to visited: {MathOp, Variable}
                  │             ├─ Type: GetBBValue
                  │             │
                  │             └─ HandleGetBBValueNode(Variable)
                  │                 └─ Return localBB.GetValue("mHealth") = 10
                  │                    (Cache: Variable:Value = 10)
                  │
                  ├─ leftOperand = 10
                  │
                  ├─ Operand B: Pin mode
                  │  │
                  │  └─ for connection (Variable2:Value) → (MathOp:B):
                  │     │
                  │     └─→ EvaluateDataPin(Variable2, "Value")
                  │         │
                  │         └─→ EvaluateRecursive(Variable2:Value, depth=1)
                  │             │
                  │             ├─ Visited: {MathOp}
                  │             ├─ Add Variable2 to visited: {MathOp, Variable2}
                  │             ├─ Type: GetBBValue
                  │             │
                  │             └─ HandleGetBBValueNode(Variable2)
                  │                 └─ Return localBB.GetValue("mFood") = 5
                  │                    (Cache: Variable2:Value = 5)
                  │
                  ├─ rightOperand = 5
                  │
                  └─ Compute: 10 + 5 = 15
                     (Cache: MathOp:Result = 15)

                  Result: 15 ✓
```

---

## Node Type Decision Tree

```
                    EvaluateNode(nodeID)
                           │
                           ▼
                    Get node from template
                           │
                ┌──────────┴──────────┬──────────┐
                │                     │          │
                ▼                     ▼          ▼
           GetBBValue              MathOp      Other
               │                      │          │
               │                      │      Status:
               │                      │      Error
               ▼                      ▼      (not data
         Read from            Resolve Operands  pure)
         blackboard                  │
               │              ┌──────┴──────┐
               │              │             │
               │              ▼             ▼
               │          Mode:          Mode:
               │          Const          Variable
               │           │               │
               │      Parse value      Read BB
               │      from string          │
               │           │               │
        ┌──────┴───────────┴───────────────┴───────┐
        │                                          │
        ▼                                          ▼
    Recursive?                                No Recursion
    (Pin mode)                                (Direct value)
        │                                          │
        ├─ Yes: Recurse                           │
        │  EvaluateDataPin(source)                │
        │        │                                 │
        │        └─ Cycle check                   │
        │        └─ Depth check                   │
        │        └─ Get result                    │
        │             │                           │
        │             └─────────┬─────────────────┘
        │                       │
        └─ No: Use value    ┌───┴────┐
                            │        │
                            ▼        ▼
                        Compute  Return
                      (for MathOp) Value
                            │        │
                            └───┬────┘
                                │
                                ▼
                        Cache result
                        (optimization)
```

---

## Error Handling State Machine

```
Start: Evaluate(nodeID)
  │
  ▼
┌─────────────────────────┐
│ Validate Inputs         │     Error?
│ - Node exists           ├──→ Yes ──→ InvalidNode ──→ Fail
│ - Pin name valid        │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Check Cache             │     Hit?
│ nodeID:pinName          ├──→ Yes ──→ Return cached ──→ Success
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Cycle Detection         │     Cycle?
│ Check visited set       ├──→ Yes ──→ CycleDetected ──→ Fail
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Depth Check             │     Too deep?
│ recursionDepth > 32     ├──→ Yes ──→ EvaluationError ──→ Fail
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Dispatch Handler        │     Type?
│ GetBBValue/MathOp/etc   ├──→ Unknown ──→ EvaluationError ──→ Fail
└──────────┬──────────────┘
           │
      ┌────┴────┐
      │          │
      ▼          ▼
  GetBBValue   MathOp
      │          │
      ├─ Try     ├─ Resolve operands
      │  Read    │   ├─ Recursive (Pin)
      │  BB      │   ├─ Const (Parse)
      │  │       │   ├─ Variable (Read BB)
      └──┼───┐   │
         ▼   │   ├─ Try compute
    Success  │   │  ├─ Type convert
         │   │   │  ├─ Math operation
         │   │   │  └─ Result
         │   │   │
         │  Error?
         │   │   │
         │  ├─┬──┘
         │  │
         ▼  ▼
    Exception  EvaluationError
       │            │
       └────┬───────┘
            │
            ▼
      EvaluationError ──→ Fail

         ║
         ║  (All paths)
         ║
         ▼
    ┌──────────────┐
    │ Cache Result │
    │ (if Success) │
    └──────┬───────┘
           │
           ▼
    ┌──────────────┐
    │ Return       │
    │ Result       │
    └──────────────┘
```

---

## Memory Layout - Data Pin Cache

```
TaskRunnerComponent
│
├─ DataPinCache (std::unordered_map)
│  │
│  ├─ Key: "1:Value"           ──→ Value: TaskValue(10, Int)  ← Variable node #1
│  ├─ Key: "2:Value"           ──→ Value: TaskValue(5, Int)   ← Variable node #2
│  ├─ Key: "3:Result"          ──→ Value: TaskValue(15, Float)← MathOp node #3
│  └─ Key: "4:Value"           ──→ Value: TaskValue(15, Float)← SetBBValue node #4
│
├─ VisitedNodes (std::unordered_set, stack frame local)
│  │
│  ├─ 1  ← Variable #1 (during evaluation)
│  ├─ 3  ← MathOp #3 (during evaluation)
│  └─ 4  ← SetBBValue #4 (during evaluation, but skipped as not data pure)
│
└─ Other fields (StateTimer, etc.)
```

---

## Operand Resolution Flow

```
MathOp Node has two operands: Left (A) and Right (B)

Each operand can be:
┌─────────────────────────────────────────────┐
│ Mode: Const (Literal)                       │
│                                             │
│  constValue: "5.0" (stored as string)      │
│                                             │
│  Resolution:                                │
│    → Parse string to float                  │
│    → TaskValue(5.0f)                        │
│    → Use directly                           │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│ Mode: Variable (Blackboard Reference)       │
│                                             │
│  variableName: "mHealth" (BB key)           │
│                                             │
│  Resolution:                                │
│    → localBB.GetValue("mHealth")            │
│    → TaskValue(10, Int)                     │
│    → Use directly                           │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│ Mode: Pin (Data Input Connection)           │
│                                             │
│  dynamicPinID: "pin_inst_abc123"            │
│                                             │
│  Resolution:                                │
│    → Find incoming DataPinConnection        │
│    → Get source node and pin name           │
│    → Recursively evaluate source:           │
│      EvaluateDataPin(sourceNodeID,          │
│                      sourcePinName)         │
│    → Get result value                       │
│    → Use returned value                     │
│    → (May recurse multiple levels deep)     │
└─────────────────────────────────────────────┘

                    │
                    ▼
            ┌───────────────┐
            │ Combine values│
            │ (A op B)      │
            └───────┬───────┘
                    │
                    ▼
            ┌───────────────┐
            │ Result cached │
            │ For reuse     │
            └───────────────┘
```

---

## Performance Timeline - Complex Graph

```
SetBBValue (BBKey="mResult")
  ├─ MathOp (op="+")
  │   ├─ Variable (mHealth)       = 10  [cache]
  │   └─ MathOp (op="*")          = 10  [cache]
  │       ├─ Variable (mFood)     = 5   [cache]
  │       └─ Const(2)             = 2
  │           Result: 5 * 2 = 10
  │
  └─ Result: 10 + 10 = 20  [cache]
     Final: SetBBValue(mResult, 20)

Time breakdown:
┌─ Frame Start
│
├─ 1ms  Evaluate mHealth (cache lookup after first)
├─ 1ms  Evaluate mFood
├─ 1ms  Evaluate MathOp (*): 5 * 2 = 10
├─ 1ms  Evaluate MathOp (+): 10 + 10 = 20
├─ 1ms  Execute SetBBValue(20)
│
└─ Total: ~5-6ms for first evaluation
           ~2-3ms for second evaluation (mostly cached)

Memory:
├─ Cache entries: 5
├─ Cache size: ~200 bytes
└─ Visited set: Temporary (freed after eval)
```

---

## Compilation Architecture

```
Source files created:
┌─────────────────────────────────────────┐
│ DataPinEvaluator.h                      │
│ - Public interface                      │
│ - Forward declarations                  │
│ - #include DataPinEvaluator_inline.h    │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│ DataPinEvaluator_inline.h               │
│ - Inline implementation                 │
│ - All functions inline                  │
│ - No separate .cpp needed               │
│ - Included in VSGraphExecutor.cpp       │
└─────────────────────────────────────────┘

VSGraphExecutor.cpp includes:
┌─────────────────────────────────────────┐
│ #include "DataPinEvaluator.h"           │
│                                         │
│ VSGraphExecutor::ResolveDataPins()      │
│ {                                       │
│   DataPinEvaluator::EvaluateNodeInputPins
│   (...)                                 │
│ }                                       │
└─────────────────────────────────────────┘

Build result:
┌─────────────────────────────────────────┐
│ OlympeBlueprintEditor.lib/dll           │
│                                         │
│ Contains:                               │
│ ├─ VSGraphExecutor::*                   │
│ ├─ DataPinEvaluator inline functions    │
│ ├─ TaskGraphTemplate::*                 │
│ ├─ LocalBlackboard::*                   │
│ └─ (All other components)               │
│                                         │
│ ✓ No linker errors                      │
│ ✓ All symbols resolved                  │
│ ✓ Build successful                      │
└─────────────────────────────────────────┘
```

---

## Integration Point - VSGraphExecutor

```
ExecuteFrame()
  │
  ├─ OLD: ResolveDataPins (simple cache)
  │  ├─ For each connection:
  │  │  ├─ Check if source in cache
  │  │  ├─ If yes: copy value
  │  │  ├─ If no: use default
  │  │  └─ Done
  │  │
  │  └─ Limitation: Only works if source
  │     already evaluated (wrong order = fail)
  │
  ├─ NEW: ResolveDataPins (smart recursive)
  │  ├─ DataPinEvaluator::EvaluateNodeInputPins()
  │  │  ├─ For each connection:
  │  │  │  ├─ Recursively evaluate source
  │  │  │  ├─ Handle dependencies automatically
  │  │  │  ├─ Cache result
  │  │  │  └─ Store in target pin
  │  │  │
  │  │  └─ Guarantee: All dependencies resolved
  │  │     in correct order (automatic)
  │  │
  │  └─ Result: Complex graphs just work!
  │
  └─ Execute node (same as before)
```

---

## Summary Diagram

```
┌──────────────────────────────────────────────────────────┐
│                   Blueprint Editor                       │
│              (Visual Script Graph System)                │
└──────────────┬───────────────────────────────────────────┘
               │
        ┌──────┴──────────────┐
        │                     │
        ▼                     ▼
   Visual Editor         VSGraphExecutor
   (Display)            (Runtime Execution)
                            │
                    ┌───────┼───────┐
                    │       │       │
              OLD   │  NEW  │ (Phase 24.1)
              ↓     │  ↓    │
        Simple  ────┤  ResolveDataPins()
        Cache       │       │
        Lookup      │  DataPinEvaluator
                    │       │
                    │  ┌────┴────┬────────┐
                    │  │         │        │
                    │  ▼         ▼        ▼
                    │ Cycle   Depth   Error
                    │ Detect  Check   Handle
                    │         │
                    │  ┌──────┴──────┐
                    │  │             │
                    │  ▼             ▼
                    │ Cache      Execute
                    │ Result      Node
                    │
                    ▼
              Works Correctly!
              ✓ Complex graphs
              ✓ Recursive deps
              ✓ Proper ordering
              ✓ Performance
              ✓ Safety
```

---

End of Visual Architecture Diagrams

