# Data Pin Stack-Based Evaluation - Usage Guide

## Overview

The Data Pin Stack-Based Evaluation system enables complex data-driven computation graphs in the Blueprint Editor's Visual Script graph system.

**New Capabilities:**
- **Data Pure Nodes**: Variable and MathOp nodes with no execution flow
- **Recursive Dependency Resolution**: Automatic evaluation of data pin networks
- **Smart Caching**: Results cached for performance
- **Cycle Detection**: Prevents infinite loops in data pin connections
- **Type Safety**: Automatic type conversion (Bool, Int, Float)

---

## Basic Concepts

### Data Pure Nodes

Unlike execution flow nodes (Branch, Sequence, etc.), data pure nodes:
- ❌ Do NOT have execution input/output pins
- ✅ ONLY have data input/output pins
- ✅ Always compute their output synchronously
- ✅ Can be connected directly to each other

### Data Pin Connections

Data pins are connected via **DataPinConnection** in the template:

```
Source Node Output Pin → Target Node Input Pin
```

Example:
```
Variable.Out("Value") → MathOp.In("A")
MathOp.Out("Result") → SetBBValue.In("Value")
```

### Execution Model

When executing a node with data pin inputs:

1. **Before execution**: Call `VSGraphExecutor::ResolveDataPins(nodeID)`
2. **Inside ResolveDataPins**: 
   - Detect all incoming `DataPinConnection`s to this node
   - Call `DataPinEvaluator::EvaluateNodeInputPins()`
3. **Inside DataPinEvaluator**:
   - For each incoming connection, recursively evaluate the source pin
   - Store results in `runner.DataPinCache`
4. **Node execution**: Use values from cache
5. **Profit**: Complex data computations without additional code!

---

## Supported Node Types

### 1. Variable Node (GetBBValue)

**Purpose**: Read a value from the blackboard

**Configuration**:
- Set `BBKey` to the blackboard variable to read (e.g., "mHealth")
- Supports scope prefix: "local:mHealth" or "global:myVar"

**Data Pins**:
- **Output**: `Value` (type: whatever is stored in the variable)

**Example**:
```
GetBBValue(BBKey="mHealth")
  → Output: Value = 10 (Int)
```

**Evaluation**:
```cpp
DataPinEvaluationResult result = HandleGetBBValueNode(nodeID, tmpl, localBB);
// result.Value = localBB.GetValue("mHealth")  // = 10
```

---

### 2. MathOp Node

**Purpose**: Compute arithmetic operation between two operands

**Configuration**:
- Set `MathOperator` to one of: `+`, `-`, `*`, `/`, `%`, `^`
- Configure `mathOpRef.leftOperand` (A)
- Configure `mathOpRef.rightOperand` (B)

**Data Pins**:
- **Input**: `A` (left operand)
- **Input**: `B` (right operand)
- **Output**: `Result` (computed value)

**Operand Modes** (for each A and B):

#### Mode: Const (Literal)
```
MathOpOperand::Mode::Const
  → constValue = "5.0"  // stored as string, parsed to float
```

#### Mode: Variable (Blackboard Reference)
```
MathOpOperand::Mode::Variable
  → variableName = "mHealth"  // read from blackboard
```

#### Mode: Pin (Data Input Connection)
```
MathOpOperand::Mode::Pin
  → dynamicPinID = "pin_inst_abc123"  // connected from another node
```

**Example Configurations**:

```
Config 1: Literal + Literal
  MathOp("5.0" + "3.0")
  → Result: 8.0

Config 2: Literal + Variable
  MathOp("5.0" + <read from "mBonusHealth">)
  → Result: 5.0 + [BB value]

Config 3: Variable + Variable
  MathOp(<read from "mHealth"> + <read from "mFoodPortion">)
  → Result: 10 + 5 = 15

Config 4: Pin + Pin (Data Connections)
  Variable("mHealth") → MathOp.A
  Variable("mFoodPortion") → MathOp.B
  MathOp.Result → SetBBValue
  → Result: MathOp evaluates Variable recursively
```

**Supported Operators**:

| Operator | Symbol | Example | Result |
|----------|--------|---------|--------|
| Addition | `+` | 10 + 5 | 15 |
| Subtraction | `-` | 10 - 5 | 5 |
| Multiplication | `*` | 10 * 5 | 50 |
| Division | `/` | 10 / 5 | 2 |
| Modulo | `%` | 10 % 3 | 1 |
| Power | `^` | 2 ^ 3 | 8 |

**Type Conversion**:
- All operands converted to `float` internally
- Bool: `true` = 1.0, `false` = 0.0
- Int: Direct cast to float
- Float: Direct use
- Other types: Error

**Division by Zero**:
- Checked explicitly
- Throws `std::runtime_error`
- Error status logged
- Node execution fails safely

---

### 3. SetBBValue Node

**Purpose**: Write a data value to the blackboard

**Configuration**:
- Set `BBKey` to target variable (e.g., "mResult")
- Supports scope: "local:mResult" or "global:globalVar"

**Data Pins**:
- **Input**: `Value` (data to write - resolved by DataPinEvaluator)
- **Output Exec**: `Completed` (connects to next execution node)

**Example**:
```
SetBBValue(BBKey="mResult")
  ← Input Value = 15 (from MathOp)
  → Execute: Write 15 to localBB["mResult"]
  → Output: "Completed" exec pin
```

---

## Practical Examples

### Example 1: Simple Damage Calculation

**Graph**:
```
┌──────────────────────┐
│ GetBBValue(mATK)=10  │
└──────────┬───────────┘
           │ Value
           ▼
┌──────────────────────┐
│ MathOp (Const:2 * A) │
│ Mode: Const * Pin    │
└──────────┬───────────┘
           │ Result
           ▼
┌──────────────────────────┐
│ SetBBValue(mDamage)      │
└──────────────────────────┘
```

**Execution Trace**:
```
ExecuteFrame():
  CurrentNodeID = SetBBValue

  ResolveDataPins(SetBBValue):
    DataPinEvaluator::EvaluateNodeInputPins(SetBBValue)

    For connection: (MathOp, "Result") → (SetBBValue, "Value")

    EvaluateDataPin(MathOp, "Result"):
      visited = {}, depth = 0

      Type = MathOp
      For connection: (GetBBValue, "Value") → (MathOp, "A")

      EvaluateDataPin(GetBBValue, "Value"):
        visited = {MathOp}, depth = 1
        Type = GetBBValue

        return GetBBValue(BBKey="mATK") = 10
        cache[GetBBValue:Value] = 10

      leftOperand = 10
      rightOperand = 2 (Const)
      result = 2 * 10 = 20
      cache[MathOp:Result] = 20
      return 20

    cache[SetBBValue:Value] = 20

  HandleSetBBValue(SetBBValue):
    value = cache[SetBBValue:Value] = 20
    localBB.SetValue("mDamage", 20)
    return FindExecTarget("Completed")
```

**Result**: mDamage = 20 ✓

---

### Example 2: Complex Health Calculation

**Graph**:
```
                         ┌─────────────────┐
                         │ GetBBValue(mHP) │
                         │     = 50        │
                         └────────┬────────┘
                                  │
         ┌────────────┐           │
         │GetBBValue  │           │
         │(mRegen)=5  │           │
         └─────┬──────┘           │
               │                  │
          MathOp(A*B)  (Value from above)
           │     │
           │     └────────┐
      (Const:2) │         │
               │          │
         MathOp(+)        │
          │        │      │
          │        └──────┘
          │
          ▼
      SetBBValue(mNewHP)
```

**Logic**: `newHP = HP + (Regen * 2)`

**Execution**:
```
EvaluateNodeInputPins(SetBBValue):

  EvaluateDataPin(MathOp1:Result)  // The outer +

    EvaluateDataPin(GetBBValue:Value)  // mHP
      → return 50
      cache[GetBBValue:Value] = 50

    EvaluateDataPin(MathOp2:Result)  // The Regen*2

      EvaluateDataPin(GetBBValue:Value)  // mRegen
        → return 5
        cache[GetBBValue:Value] = 5

      leftOperand = 5
      rightOperand = 2 (Const)
      result = 5 * 2 = 10
      cache[MathOp2:Result] = 10

      return 10

    leftOperand = 50
    rightOperand = 10
    result = 50 + 10 = 60
    cache[MathOp1:Result] = 60

    return 60

cache[SetBBValue:Value] = 60

→ SetBBValue writes 60 to mNewHP ✓
```

---

## Error Handling

### Cycle Detection

**Scenario**: Variable A → Variable B → Variable A

```
Visited set: {}
Evaluate(A):
  visited.add(A)
  Evaluate(B):
    visited.add(B)
    Evaluate(A):
      A in visited? → YES
      return CycleDetected ✗
```

**Result**: Error logged, node execution fails gracefully

---

### Type Mismatch

**Scenario**: Trying to add String + Bool

```cpp
leftOperand = TaskValue("hello")  // String
rightOperand = TaskValue(true)    // Bool

leftVal = TaskValueToFloat(leftOperand)
// String type → throw std::runtime_error
```

**Result**: Status = EvaluationError, logged

---

### Missing Connections

**Scenario**: MathOp with Pin mode, but no incoming connection

```cpp
if (mathOpRef.leftOperand.mode == MathOpOperand::Mode::Pin)
{
    // Search for incoming connection to "A"
    // Not found!
    return "No data connection found for MathOp left operand (Pin mode)"
}
```

**Result**: Status = EvaluationError, graceful failure

---

## Performance Considerations

### Caching Strategy

```
First evaluation of a pin:
  - Recursive traversal of dependencies
  - O(N) where N = nodes in dependency tree
  - Result cached with key "nodeID:pinName"

Second evaluation of same pin (same frame):
  - Cache lookup: O(1)
  - Return cached value immediately
  - No recursion needed

Different frame:
  - Cache cleared
  - Re-evaluation performed
```

### Optimization Tips

1. **Avoid deep chains**
   - Keep dependency depth < 10 for best performance
   - Limit to 32 (MAX_RECURSION_DEPTH)

2. **Reuse computed values**
   - Caching automatically handles this
   - Same pin evaluated twice? Second is O(1)

3. **Use Const mode when possible**
   - Const values don't need resolution
   - Fastest evaluation path

4. **Minimize cycle complexity**
   - Cycle detection is O(visited_nodes) per evaluation
   - Complex graphs = more cycle checks

---

## Best Practices

### ✅ DO

- Connect data pins to create computation graphs
- Use MathOp to combine values
- Use GetBBValue to read input values
- Use SetBBValue to store computed results
- Keep graphs reasonably shallow (< 10 levels deep)
- Use meaningful BBKey names

### ❌ DON'T

- Create circular data pin connections (cycles detected and fail)
- Mix data pins with execution pins on same node
- Try to evaluate nodes that don't support data pins
- Rely on evaluation order for non-dependent nodes (use Sequence!)

---

## Troubleshooting

### Issue: "CycleDetected" error

**Cause**: Circular data pin connection

**Solution**: 
- Review graph visually
- Check for loops in data connections
- Refactor to break cycle

### Issue: "Failed to evaluate pin" warning

**Cause**: One of several issues

**Steps**:
1. Check if node exists in template
2. Check for type mismatches
3. Check if blackboard variable exists
4. Look for division by zero in MathOp

### Issue: Wrong computation result

**Cause**: Operator precedence or operand confusion

**Solution**:
- Break complex calculations into multiple MathOp nodes
- Use intermediate SetBBValue nodes to verify intermediate results
- Debug output: Add logging to blackboard values

### Issue: Performance degradation

**Cause**: Deep dependency chains or large graphs

**Solution**:
- Profile the graph: add timers to critical sections
- Reduce chain depth
- Use Const operands instead of Pin mode where possible

---

## API Reference

### DataPinEvaluator::EvaluateNodeInputPins

```cpp
static bool EvaluateNodeInputPins(
    int32_t nodeID,
    const TaskGraphTemplate& tmpl,
    TaskRunnerComponent& runner,
    LocalBlackboard& localBB);
```

**Purpose**: Evaluate all input data pins for a node

**Parameters**:
- `nodeID`: Node to evaluate inputs for
- `tmpl`: Task graph template (immutable)
- `runner`: Task runner component (contains cache)
- `localBB`: Local blackboard for variable access

**Returns**: 
- `true` if all evaluations succeeded
- `false` if any failed (check logs for details)

**Called by**: `VSGraphExecutor::ResolveDataPins()`

**Usage**:
```cpp
if (!DataPinEvaluator::EvaluateNodeInputPins(
    nodeID, tmpl, runner, localBB))
{
    SYSTEM_LOG << "Warning: Some data pins failed to evaluate\n";
    // Execution continues; failed pins use defaults
}
```

---

## Summary

The Data Pin Stack-Based Evaluation system provides:

✅ **Automatic dependency resolution** - no manual ordering needed
✅ **Type safety** - automatic conversions, error handling
✅ **Performance** - smart caching, early exit optimization
✅ **Safety** - cycle detection, depth limiting, exception handling
✅ **Flexibility** - support for multiple node types and operand modes

Use it to create complex, readable data-driven computation graphs in your Visual Scripts!

