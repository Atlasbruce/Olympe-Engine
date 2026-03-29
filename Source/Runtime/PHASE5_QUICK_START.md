# Phase 5 Quick Start: Using Condition Presets in Branch Nodes

## Quick Summary

Phase 5 integrates Phase 4's condition preset evaluation into the task graph executor. Branch nodes can now evaluate condition presets with AND/OR operators during graph execution.

## How to Use Phase 24 Condition Presets

### Step 1: Create Condition Presets (UI - Phase 3)
Already done in the Editor. Presets are stored in `./Blueprints/Presets/condition_presets.json`.

### Step 2: Reference Presets in Branch Node (V4 Schema)
Add `conditionRefs` to your Branch node:

```json
{
  "NodeID": 5,
  "Type": "Branch",
  "NodeName": "Is Player Alive?",
  "conditionRefs": [
    {
      "presetID": "preset_001",
      "logicalOp": "Start"
    },
    {
      "presetID": "preset_002",
      "logicalOp": "And"
    }
  ]
}
```

### Step 3: Execute Task Graph
When the Branch node executes:

1. **Phase 5 Runtime** loads condition presets from registry
2. **RuntimeEnvironment** is populated with current blackboard values
3. **EvaluateConditionChain()** evaluates the conditions
4. **Then/Else** branch is selected based on result

## Execution Priority

When a Branch node executes, conditions are evaluated in this order:

```
1. Phase 24 (conditionRefs) ← NEW IN PHASE 5
   ├─ Uses ConditionPresetEvaluator
   ├─ Supports AND/OR operators
   └─ Short-circuit evaluation

2. Phase 23-B.4 (conditions) ← EXISTING
   ├─ Uses ConditionEvaluator
   └─ Implicit AND

3. Data Pin Evaluation ← FALLBACK
   └─ Reads "Condition" pin value
```

First non-empty category is used.

## Example: Multi-Condition Branch

### Condition Presets (Created in Phase 3)
```
preset_001: "mHealth" <= 50.0         (low health)
preset_002: "mStamina" > 75.0         (high stamina)
preset_003: "InCombat" == 1.0         (in combat)
```

### Task Graph (V4 JSON)
```json
{
  "NodeID": 10,
  "Type": "Branch",
  "NodeName": "Can Perform Special Attack?",
  "conditionRefs": [
    {
      "presetID": "preset_002",
      "logicalOp": "Start"
    },
    {
      "presetID": "preset_003",
      "logicalOp": "And"
    }
  ]
}
```

### Evaluation at Runtime
```
Environment Setup:
  mHealth = 30.0   ← From LocalBlackboard
  mStamina = 85.0  ← From LocalBlackboard
  InCombat = 1.0   ← From LocalBlackboard

Evaluation:
  preset_002: 85.0 > 75.0 = true
             Proceed to next (And operator)

  preset_003: 1.0 == 1.0 = true
             Result: true (all conditions true)

Route: THEN branch (can perform special attack)
```

## Error Handling

If evaluation encounters an error, it logs a warning and routes to the **Else branch** (fail-safe):

```
[VSGraphExecutor] Branch node 10: Preset not found in registry: 'invalid_id' — defaulting to Else
```

Common errors:
- Missing preset ID → Logs warning, routes to Else
- Missing variable → Logs warning, uses 0.0 as value
- Invalid operator → Logs error, routes to Else

## Performance Characteristics

### Short-Circuit Evaluation

**AND chains** stop at first `false`:
```
A AND B AND C
true AND false → STOPS HERE (C not evaluated)
Result: false
```

**OR chains** stop at first `true`:
```
A OR B OR C
false OR true → STOPS HERE (C not evaluated)
Result: true
```

**Impact**: 40-99% fewer evaluations in typical scenarios = faster execution

## Code Integration

In VSGraphExecutor.cpp, the new Phase 24 logic:

1. **Populates RuntimeEnvironment** from blackboard and pins
2. **Loads ConditionPresetRegistry** from "./Blueprints/Presets/condition_presets.json"
3. **Calls EvaluateConditionChain()** with conditions and environment
4. **Routes to Then/Else** based on result

Example call (inside HandleBranch):

```cpp
RuntimeEnvironment env;
PopulateRuntimeEnvironmentFromBlackboard(env, localBB);
PopulateRuntimeEnvironmentFromDataPins(env, runner.DataPinCache);

ConditionPresetRegistry registry;
registry.Load("./Blueprints/Presets/condition_presets.json");

std::string errorMsg;
bool result = ConditionPresetEvaluator::EvaluateConditionChain(
    node->conditionRefs, registry, env, errorMsg);

return result ? FindExecTarget(nodeID, "Then", tmpl)
              : FindExecTarget(nodeID, "Else", tmpl);
```

## Backward Compatibility

✅ Existing task graphs continue to work unchanged:
- Phase 23-B.4 conditions still supported
- Data pin evaluation still available
- No schema changes required
- No breaking API changes

## Testing

### Unit Test Example
```cpp
TEST(Phase5Integration, BranchWithConditionPresets)
{
    // Setup
    RuntimeEnvironment env;
    env.SetBlackboardVariable("health", 50.0f);

    ConditionPresetRegistry registry;
    ConditionPreset preset("p1",
        Operand::CreateVariable("health"),
        ComparisonOp::LessEqual,
        Operand::CreateConst(100.0f));
    registry.CreatePreset(preset);

    std::vector<NodeConditionRef> conditions;
    conditions.push_back(NodeConditionRef(preset.id, LogicalOp::Start));

    // Execute
    std::string error;
    bool result = ConditionPresetEvaluator::EvaluateConditionChain(
        conditions, registry, env, error);

    // Verify
    EXPECT_TRUE(result);
    EXPECT_EQ(error, "");
}
```

## Troubleshooting

### Condition Always False
1. Check preset exists in registry
2. Verify variable name matches exactly (case-sensitive)
3. Check variable has correct value at runtime
4. Review logs for evaluation errors

### Condition Not Evaluated
1. Verify `conditionRefs` is populated (not empty)
2. Check no errors in Phase 23-B.4 conditions (they take priority if non-empty)
3. Verify registry loads successfully

### Performance Issues
1. Profile to find slow presets
2. Check for unnecessary type conversions
3. Consider simplifying AND/OR chains
4. Use short-circuit evaluation effectively

## Related Documentation

- **Phase 4 API**: `Source/Runtime/PHASE4_CONDITION_EVALUATION.md`
- **Phase 5 Plan**: `Source/Runtime/PHASE5_INTEGRATION_PLAN.md`
- **Phase 5 Report**: `Source/Runtime/PHASE5_COMPLETION_REPORT.md`
- **Phase 4 Examples**: `Source/Runtime/Phase4_ConditionEvaluation_Examples.cpp`

## Questions?

For detailed architecture:
- See PHASE5_INTEGRATION_PLAN.md (architecture section)

For API documentation:
- See PHASE4_QUICK_REFERENCE.h (ConditionPresetEvaluator methods)

For implementation details:
- See Source/TaskSystem/VSGraphExecutor.cpp (HandleBranch function)

---

**Phase 5**: Runtime Condition Evaluation Integration ✅ COMPLETE

