# Phase 39: Quick Reference Guide

## One-Minute Overview

**Goal**: Add SubGraph Action nodes to BehaviorTree for recursive graph composition

**Key Idea**:
```
BehaviorTree (Parent)
  ├─ Selector
  │  ├─ Condition: Check Target
  │  └─ SubGraph Action ← References patrol.bt.json (Child)
  │
Child BehaviorTree (patrol.bt.json)
  └─ Sequence
     ├─ Pick Waypoint
     └─ Move To Waypoint
```

**Why**?
- Reuse behavior logic across multiple trees
- Decompose complex behaviors into modules
- Support both BT↔BT and BT↔ATS execution

---

## Current State vs. Proposed State

### VisualScript SubGraph (Reference Implementation)
```cpp
✅ TaskNodeType::SubGraph
✅ TaskNodeDefinition::SubGraphPath
✅ VSGraphExecutor::HandleSubGraph()
✅ VSGraphVerifier rules E003, E004, W003
✅ Editor: Double-click navigation, property panel
✅ Testing: ExecutionTestPanel + GraphExecutionTracer
```

### BehaviorTree SubGraph (Phase 39 - NEW)
```cpp
🆕 BTNodeType::SubGraph
🆕 BTNode::subgraphPath + subgraphInputs/Outputs
🆕 BehaviorTreeExecutor::ExecuteSubGraph()
🆕 Validator rules E501-E505
🆕 Editor: File picker, parameter binding UI
🆕 Testing: Same ExecutionTestPanel (reused)
```

---

## Architecture at a Glance

```
┌─────────────────────────────────────────────────────┐
│          BehaviorTree Editor (UI Layer)             │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Canvas                     Properties Panel       │
│  ┌──────────────────┐      ┌──────────────────┐   │
│  │ SubGraph Node    │◄─────│ File Path: ...   │   │
│  │ (blue + icon)    │      │ Inputs:          │   │
│  │ Double-click ───────────│ patrolSpeed→...  │   │
│  └──────────────────┘      │ Outputs:         │   │
│                            │ foundTarget→...  │   │
│                            └──────────────────┘   │
├─────────────────────────────────────────────────────┤
│      BehaviorTreeExecutor (Execution Layer)        │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ExecuteNode()                                      │
│    ├─ Selector → ExecuteSelector()                │
│    ├─ Sequence → ExecuteSequence()                │
│    ├─ Action → ExecuteAction()                    │
│    └─ SubGraph → ExecuteSubGraph() [NEW]          │
│         ├─ Resolve path (DataManager)             │
│         ├─ Check depth/cycles (CallStack)         │
│         ├─ Load external file                     │
│         ├─ Execute child tree                     │
│         └─ Return Success/Failure                 │
│                    ↓                               │
│       GraphExecutionTracer (records events)        │
│                                                     │
├─────────────────────────────────────────────────────┤
│      BehaviorTreeValidator (Verification)          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  CheckSubGraphNodes()                              │
│    ├─ E501: Empty path                            │
│    ├─ E502: File not found                        │
│    ├─ E503: Circular reference                    │
│    ├─ E504: Depth exceeds 32                      │
│    └─ E505: Type mismatch                         │
│                    ↓                               │
│    ExecutionTestPanel::DisplayBTTrace()            │
│         (show execution flow with nesting)         │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Critical Data Structures

### BTNode Extension
```cpp
struct BTNode {
    BTNodeType type;           // Now includes SubGraph = 8
    uint32_t id;
    std::string name;
    
    // Phase 39 fields for SubGraph:
    std::string subgraphPath;  // "Blueprints/AI/patrol.bt.json"
    std::map<std::string, std::string> subgraphInputs;   // "childVar" → "parentBBKey"
    std::map<std::string, std::string> subgraphOutputs;  // "childVar" → "parentBBKey"
};
```

### JSON Format
```json
{
  "id": 10,
  "type": "SubGraph",
  "name": "Call Patrol",
  "subgraphPath": "Blueprints/AI/patrol.bt.json",
  "subgraphInputs": {
    "patrolSpeed": "moveSpeed",
    "searchRadius": "detectionRange"
  },
  "subgraphOutputs": {
    "foundTarget": "targetSpotted"
  }
}
```

---

## Execution Flow (Sequence Diagram)

```
┌─────────────┐         ┌──────────────┐       ┌──────────────┐
│   BT Edit   │         │   BT Exec    │       │   BT Valid   │
└──────┬──────┘         └──────┬───────┘       └──────┬───────┘
       │                       │                      │
       │ Double-click          │                      │
       │ SubGraph              │                      │
       ├──────────────────────►│                      │
       │                       │                      │
       │                       │ DataManager          │
       │                       │ ResolveFilePath      │
       │                       ├─────┐               │
       │                       │     │               │
       │                       │◄────┘               │
       │                       │                      │
       │                       │ CheckCallStack      │
       │                       │ (depth, cycles)     │
       │                       │                      │
       │                       │ LoadExternal        │
       │                       │ .bt.json file       │
       │                       │                      │
       │                       │ ExecuteNode()       │
       │                       │ (child tree)        │
       │                       │                      │
       │                       │ GraphExecutionTracer│
       │                       │ .RecordNodeEntered()
       │                       ├──────────────────────►
       │                       │                      │
       │ OpenFileInTab()       │                      │
       │ (navigator)◄──────────┤                      │
       │                       │ Return Status       │
       │                       │ (Success/Failure)   │
       │ DisplayTrace()        │                      │
       │◄──────────────────────┤                      │
       │                       │                      │
```

---

## Execution Example: With Trace Output

```
BT Tree: Guardian.bt.json
├─ Root [Enter]
│  ├─ Selector [Enter]
│  │  ├─ Sequence [Enter]
│  │  │  ├─ Condition: TargetVisible [Enter]
│  │  │  │  └─ [Exit] Status: FAILURE
│  │  │  └─ [Exit] Status: FAILURE (short-circuit)
│  │  │
│  │  ├─ SubGraph: Patrol [Enter]
│  │  │  ├─ Loading: Blueprints/AI/patrol.bt.json
│  │  │  ├─ Depth: 2/32
│  │  │  ├─ CallStack: [patrol.bt.json]
│  │  │  │
│  │  │  └─ Executing Child Tree...
│  │  │     ├─ Root [Enter]
│  │  │     │  ├─ Sequence [Enter]
│  │  │     │  │  ├─ Action: PickWaypoint [Enter]
│  │  │     │  │  │  └─ [Exit] Status: SUCCESS
│  │  │     │  │  ├─ Action: MoveToWaypoint [Enter]
│  │  │     │  │  │  └─ [Exit] Status: SUCCESS
│  │  │     │  │  └─ [Exit] Status: SUCCESS
│  │  │     │  └─ [Exit] Status: SUCCESS
│  │  │
│  │  │  ├─ CallStack.Pop()
│  │  │  └─ [Exit] Status: SUCCESS
│  │  │
│  │  └─ [Exit] Status: SUCCESS (parent satisfied)
│  │
│  └─ [Exit] Status: SUCCESS

Final Trace Events:
- NodeEntered(1, "Root", "Root")
- NodeEntered(2, "Selector", "Selector")
- NodeEntered(3, "Sequence", "Sequence")
- NodeEntered(4, "TargetVisible", "Condition")
- NodeExited(4, FAILURE)
- NodeExited(3, FAILURE)
- NodeEntered(10, "Patrol", "SubGraph")
  ... [child events] ...
- NodeExited(10, SUCCESS)
- NodeExited(2, SUCCESS)
- NodeExited(1, SUCCESS)
```

---

## Phase Dependencies Chain

```
Phase 38 (COMPLETE) ✅
  └─ DataManager::ResolveFilePath()
     └─ Used by: SubGraph file loading

Phase 35 (COMPLETE) ✅
  ├─ ExecutionTestPanel
  ├─ GraphExecutionTracer
  └─ Used by: Trace display + testing

Phase 37 (COMPLETE) ✅
  └─ Canvas infrastructure
     └─ Inherited by: SubGraph nodes

Phase 2 (COMPLETE) ✅
  ├─ BTNodePalette
  ├─ BTNodePropertyPanel
  └─ Extended by: SubGraph properties

Phase 39 (NEW) ← Implementation starts here
  ├─ 39a: Core Execution (3-5 days)
  ├─ 39b: Editor UI (4-6 days)
  ├─ 39c: Verification (2-4 days)
  └─ 39d: Polish & Docs (1-2 days)
```

---

## Validation Rules Quick Reference

| Rule | Severity | Check | Example |
|------|----------|-------|---------|
| E501 | Error | Empty SubGraph path | subgraphPath = "" |
| E502 | Error | File not found | Path: "BadPath/file.bt.json" |
| E503 | Error | Circular reference | A→B→A cycle |
| E504 | Error | Depth exceeds 32 | A→B→C→...→Z (33 levels) |
| E505 | Error | Type mismatch | ATS file but expects BT? |

---

## Implementation Checklist (Quick Version)

```
PHASE 39a: CORE EXECUTION (3-5 days)
┌─────────────────────────────────┐
├─ [ ] Add SubGraph to BTNodeType  │
├─ [ ] Extend BTNode structure     │
├─ [ ] JSON serialization          │
├─ [ ] ExecuteSubGraph() method    │
├─ [ ] Call stack + cycles         │
├─ [ ] Unit tests                  │
└─────────────────────────────────┘

PHASE 39b: EDITOR (4-6 days)
┌─────────────────────────────────┐
├─ [ ] Property panel section      │
├─ [ ] File path input + browser   │
├─ [ ] Parameter grids (in/out)    │
├─ [ ] Canvas rendering (blue)     │
├─ [ ] Double-click handler        │
└─────────────────────────────────┘

PHASE 39c: VERIFICATION (2-4 days)
┌─────────────────────────────────┐
├─ [ ] Validator rules E501-E505   │
├─ [ ] ExecutionTestPanel trace    │
├─ [ ] Integration tests           │
└─────────────────────────────────┘

PHASE 39d: POLISH (1-2 days)
┌─────────────────────────────────┐
├─ [ ] Documentation              │
├─ [ ] Example BTs                │
├─ [ ] Profiling                  │
└─────────────────────────────────┘
```

---

## Key Files to Modify

```
Source/AI/BehaviorTree.h
  ├─ BTNodeType::SubGraph = 8
  ├─ BTNode::subgraphPath
  ├─ BTNode::subgraphInputs/Outputs
  └─ SubGraphCallStack struct

Source/BlueprintEditor/BehaviorTreeExecutor.h/cpp
  └─ ExecuteSubGraph() method

Source/BlueprintEditor/BTNodePropertyPanel.h/cpp
  └─ RenderSubGraphProperties() section

Source/BlueprintEditor/BTNodeRenderer.h/cpp
  └─ RenderSubGraphNode() function

Source/BlueprintEditor/BehaviorTreeRenderer.cpp
  └─ Double-click navigation handler

Source/BlueprintEditor/BehaviorTreeValidator.h/cpp
  └─ CheckSubGraphNodes() + 5 rules
```

---

## Success Indicators

✅ When you can:
- [ ] Create SubGraph node in BT editor
- [ ] Set file path and see file picker work
- [ ] Set input/output parameter bindings
- [ ] Double-click SubGraph → opens file in new tab
- [ ] Execute BT with SubGraph → sees child tree execution in trace
- [ ] Validator detects empty paths and circular refs
- [ ] Nested SubGraphs work (A→B→C)
- [ ] BT references ATS and executes it
- [ ] All unit + integration tests pass

---

## Common Questions

**Q: What's the max nesting depth?**
A: 32 levels (same as VisualScript)

**Q: Can I reference an ATS file from BT?**
A: Yes! Execute as VisualScript, return Success/Failure

**Q: What if file doesn't exist?**
A: Executor records error, returns Failure, continues

**Q: How are parameters passed?**
A: String-based input/output mapping in JSON (like VisualScript)

**Q: What about blackboard scope?**
A: Each tree has own blackboard, only mapped params cross boundary

**Q: Can SubGraphs be in the canvas palette?**
A: Yes, as "SubGraph" action type in BTNodePalette

---

## Related Documentation

📄 **Detailed Specs**:
- `PHASE_39_TECHNICAL_DESIGN.md` (full implementation specs)
- `PHASE_39_EXECUTIVE_SUMMARY.md` (high-level overview)
- `PHASE_39_BT_SUBGRAPH_ANALYSIS.md` (feasibility analysis)

📄 **Reference (VisualScript SubGraph)**:
- `ATS_VS_Phase8_Subgraphs.md` (reference architecture)

---

## Notes for Developers

1. **Reuse First**: Don't reinvent - leverage ExecutionTestPanel and GraphExecutionTracer
2. **Test Early**: Write unit tests for ExecuteSubGraph before integrating with editor
3. **Error Handling**: Every SubGraph operation needs trace logging
4. **Path Resolution**: Always use DataManager::ResolveFilePath() (Phase 38)
5. **Depth Tracking**: Use SubGraphCallStack for cycle detection and depth limiting
6. **Parameter Mapping**: Keep it simple - string-based in/out only

---

## Timeline Summary

```
Week 1: Phase 39a (Core Execution)
  Mon-Wed: Data model + JSON (3 days)
  Thu-Fri: ExecuteSubGraph implementation + tests (2 days)

Week 2: Phase 39b (Editor UI)
  Mon-Wed: Property panel + file picker (3 days)
  Thu-Fri: Canvas rendering + navigation (2 days)

Week 3: Phase 39c (Verification)
  Mon: Validator rules (1 day)
  Tue-Wed: Integration tests (2 days)
  Thu: System testing (1 day)

Week 4: Phase 39d (Polish)
  Mon-Tue: Documentation + examples (2 days)
  Wed-Fri: Buffer for fixes/optimization (3 days)
```

---

## Go/No-Go Decision

✅ **RECOMMENDED**: Start Phase 39 now
- All prerequisites complete (Phase 38, 35, 37, 2)
- High feasibility (95% confidence)
- Low risk
- Clear implementation path
- Will unlock major feature (behavior composition)

🚀 **Ready to begin!**
