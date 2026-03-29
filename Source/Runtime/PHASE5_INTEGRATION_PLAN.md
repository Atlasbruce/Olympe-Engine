# Phase 5: Integration of Condition Preset Evaluation into Task Graph Executor

## Overview

Phase 5 integrates Phase 4's **runtime condition evaluation** into the actual task graph execution pipeline. Specifically:
- Hook `ConditionPresetEvaluator::EvaluateConditionChain()` into `VSGraphExecutor::HandleBranch()`
- Leverage existing Phase 24 infrastructure: `node->conditionRefs` 
- Maintain backward compatibility with Phase 23-B.4 conditions

## Current System State

### Existing Infrastructure (Phase 24)
```cpp
struct TaskNodeDefinition {
    // Phase 23-B.4 (implicit AND of all conditions)
    std::vector<Condition> conditions;

    // Phase 24 (explicit condition presets with AND/OR operators)
    std::vector<NodeConditionRef> conditionRefs;
};
```

### Current HandleBranch() Logic (VSGraphExecutor.cpp, lines ~750-800)
```cpp
int32_t VSGraphExecutor::HandleBranch(int32_t nodeID,
                                      TaskRunnerComponent& runner,
                                      const TaskGraphTemplate& tmpl,
                                      LocalBlackboard& localBB)
{
    const TaskNodeDefinition* node = tmpl.GetNode(nodeID);

    // Phase 23-B.4: Evaluate structured conditions (implicit AND)
    if (node != nullptr && !node->conditions.empty())
    {
        const bool condition = ConditionEvaluator::EvaluateAll(
            node->conditions, localBB, runner.DataPinCache, nodeID);

        return condition
            ? FindExecTarget(nodeID, "Then", tmpl)
            : FindExecTarget(nodeID, "Else", tmpl);
    }

    // Fallback: Read "Condition" data pin from DataPinCache
    // ... (pin-based evaluation logic)
}
```

### Phase 4 API (ConditionPresetEvaluator)
```cpp
// Single condition evaluation
static bool Evaluate(
    const ConditionPreset& preset,
    RuntimeEnvironment&    env,
    std::string&           outErrorMsg);

// Multi-condition with AND/OR
static bool EvaluateConditionChain(
    const std::vector<NodeConditionRef>& conditions,
    const ConditionPresetRegistry&       registry,
    RuntimeEnvironment&                  env,
    std::string&                         outErrorMsg);
```

## Integration Strategy

### Priority Order (Evaluation Precedence)
1. **Phase 24 Presets** (if `conditionRefs` not empty)
   - Call `ConditionPresetEvaluator::EvaluateConditionChain()`
   - Uses Preset Bank from `ConditionPresetRegistry`
   - Supports AND/OR operators with short-circuit evaluation
   - **NEW** in Phase 5

2. **Phase 23-B.4 Conditions** (if `conditions` not empty)
   - Call existing `ConditionEvaluator::EvaluateAll()`
   - Implicit AND, supports Variable/Const/Pin operands
   - **EXISTING** - unchanged

3. **Data Pin Fallback** (if both above are empty)
   - Read "Condition" from DataPinCache
   - **EXISTING** - unchanged

### RuntimeEnvironment Population Strategy

The `ConditionPresetEvaluator` requires a `RuntimeEnvironment` with three data sources:

```cpp
// 1. Blackboard Variables (Variable-mode operands)
//    → Copy LocalBlackboard values to RuntimeEnvironment
//    → Scope "local:" for local variables
//    → Scope "global:" for global variables

// 2. Dynamic Pin Values (Pin-mode operands)
//    → Copy DataPinCache values to RuntimeEnvironment
//    → Keyed by pin ID (stored in NodeConditionRef)

// 3. Const Values
//    → Embedded in ConditionPreset, no environment access needed
```

## Detailed Integration Changes

### File: Source/TaskSystem/VSGraphExecutor.h

**Change 1**: Add includes for Phase 4 components
```cpp
#include "../Runtime/ConditionPresetEvaluator.h"
#include "../Runtime/RuntimeEnvironment.h"
#include "../Editor/ConditionPreset/ConditionPresetRegistry.h"
```

**Change 2**: Update HandleBranch signature comment to document Phase 24 handling

### File: Source/TaskSystem/VSGraphExecutor.cpp

**Change 1**: Modify HandleBranch() to add Phase 24 logic

**Priority 1**: Check `conditionRefs` (Phase 24 Presets)
```cpp
// Phase 24: Evaluate condition presets with AND/OR operators
if (node != nullptr && !node->conditionRefs.empty())
{
    // 1. Build RuntimeEnvironment from current state
    RuntimeEnvironment env;

    // Populate with blackboard variables (Variable-mode operands)
    PopulateRuntimeEnvironmentFromBlackboard(env, localBB);

    // Populate with dynamic pin values (Pin-mode operands)
    PopulateRuntimeEnvironmentFromDataPins(env, runner.DataPinCache);

    // 2. Get registry (need to determine source - see below)
    const ConditionPresetRegistry& registry = GetConditionPresetRegistry();

    // 3. Evaluate condition chain
    std::string errorMsg;
    const bool condition = ConditionPresetEvaluator::EvaluateConditionChain(
        node->conditionRefs, registry, env, errorMsg);

    if (!errorMsg.empty())
    {
        SYSTEM_LOG << "[VSGraphExecutor] Branch node " << nodeID 
                   << ": " << errorMsg << "\n";
        // On error: execute Else branch (fail-safe)
        return FindExecTarget(nodeID, "Else", tmpl);
    }

    return condition
        ? FindExecTarget(nodeID, "Then", tmpl)
        : FindExecTarget(nodeID, "Else", tmpl);
}
```

**Priority 2**: Check `conditions` (Phase 23-B.4)
```cpp
// Phase 23-B.4: Structured conditions (implicit AND)
if (node != nullptr && !node->conditions.empty())
{
    const bool condition = ConditionEvaluator::EvaluateAll(
        node->conditions, localBB, runner.DataPinCache, nodeID);

    return condition
        ? FindExecTarget(nodeID, "Then", tmpl)
        : FindExecTarget(nodeID, "Else", tmpl);
}
```

**Priority 3**: Fallback to data pin
```cpp
// Fallback: Data pin evaluation (existing logic)
// ...
```

### Helper Functions to Add (VSGraphExecutor.cpp)

```cpp
/**
 * @brief Populates RuntimeEnvironment with blackboard variables.
 *
 * Copies all local blackboard values into the environment, keyed by
 * variable name for Variable-mode operand resolution.
 */
static void PopulateRuntimeEnvironmentFromBlackboard(
    RuntimeEnvironment& env,
    const LocalBlackboard& localBB)
{
    // Implementation: iterate localBB, call env.SetBlackboardVariable()
}

/**
 * @brief Populates RuntimeEnvironment with DataPinCache values.
 *
 * Copies resolved data pin values into the environment, keyed by pin ID
 * for Pin-mode operand resolution.
 */
static void PopulateRuntimeEnvironmentFromDataPins(
    RuntimeEnvironment& env,
    const std::unordered_map<std::string, TaskValue>& dataPinCache)
{
    // Implementation: iterate dataPinCache, call env.SetDynamicPinValue()
}

/**
 * @brief Gets the global ConditionPresetRegistry instance.
 *
 * Queries the application's asset/resource manager or singleton registry
 * to get the current set of condition presets.
 */
static const ConditionPresetRegistry& GetConditionPresetRegistry()
{
    // Implementation: return singleton or manager-owned registry
}
```

## Error Handling Strategy

### Phase 24 Error Cases

| Error | Scenario | Action |
|-------|----------|--------|
| Preset not found | NodeConditionRef references non-existent preset | Log warning, execute Else (fail-safe) |
| Variable not found | Variable-mode operand references missing variable | Log warning, evaluate as 0.0, continue |
| Pin not resolved | Pin-mode operand references unconnected pin | Log warning, evaluate as 0.0, continue |
| Unknown operator | ComparisonOp or LogicalOp invalid | Log error, execute Else (fail-safe) |

### Error Message Format
```
[VSGraphExecutor] Branch node <nodeID>: <error description>
```

## Testing Strategy (Phase 5)

### Unit Tests
- Test each comparison operator (==, !=, <, <=, >, >=)
- Test AND/OR chains with various combinations
- Test short-circuit evaluation
- Test error handling (missing preset, variable, pin)

### Integration Tests
- Test real task graph execution with condition presets
- Verify Then/Else branch routing
- Verify backward compatibility with Phase 23-B.4 conditions
- Verify priority ordering (Phase 24 > Phase 23-B.4 > Pin fallback)

### Regression Tests
- Verify all existing Branch nodes still work
- Verify no changes to other node types
- Verify performance impact is minimal (short-circuit evaluation)

## Backward Compatibility

**Fully backward compatible**:
- Phase 23-B.4 conditions unchanged and still work
- Data pin fallback still works
- If both `conditionRefs` and `conditions` are empty, falls back to pin
- Existing task graphs continue to execute unchanged

## ConditionPresetRegistry Access

**Key Question**: How to access `ConditionPresetRegistry` in VSGraphExecutor?

**Options**:
1. **Singleton Pattern** (if registry is application-global)
   ```cpp
   const ConditionPresetRegistry& registry = 
       ConditionPresetRegistry::GetInstance();
   ```

2. **Dependency Injection** (pass registry as parameter)
   ```cpp
   // Modify ExecuteFrame signature to include registry
   static void ExecuteFrame(..., const ConditionPresetRegistry& registry);
   ```

3. **World/Manager Query** (if registry owned by World or asset manager)
   ```cpp
   const ConditionPresetRegistry& registry = 
       worldPtr->GetAssetManager().GetConditionPresets();
   ```

**Recommendation**: Option 1 (Singleton) for minimal signature changes, but check existing code patterns.

## Performance Considerations

### Short-Circuit Evaluation
- Phase 4's `EvaluateConditionChain()` already implements short-circuit logic
- AND chains: stop at first `false` (saves 40-99% of evaluations in best case)
- OR chains: stop at first `true`
- **Expected impact**: Negligible or positive (fewer evaluations)

### Data Copy Overhead
- Blackboard variables copied to RuntimeEnvironment once per Branch node
- Data pin values copied once per Branch node
- **Expected impact**: Very small (typically 5-50 variables per graph)
- **Optimization**: Cache RuntimeEnvironment in TaskRunnerComponent if needed

## Documentation Updates

**Files to Create/Update**:
1. `Source/Runtime/PHASE5_INTEGRATION_REPORT.md` - What was done
2. `Source/Runtime/PHASE5_QUICK_START.md` - How to use Phase 24 presets
3. Update inline comments in VSGraphExecutor.cpp

## Migration Path for Existing Graphs

**For developers using Phase 23-B.4 conditions**:
- No changes required - continue using `Condition` objects
- Graphs will work unchanged
- Optional migration to Phase 24: move conditions to Condition Presets

**For new graphs**:
- Recommended: Use Phase 24 Condition Presets
- Reason: Better reusability, AND/OR operators, cleaner UI

## Build Status

- Phase 4 code: ✅ Compiles successfully
- Phase 5 pending: Will compile after changes
- No new external dependencies
- C++14 compliant (all code)

## Timeline Estimate

- Implementation: 1-2 hours
- Testing: 1-2 hours
- Documentation: 30 minutes
- Total: 2.5-4.5 hours

## Files Modified in Phase 5

1. `Source/TaskSystem/VSGraphExecutor.h`
   - Added includes for ConditionPresetEvaluator, RuntimeEnvironment
   - Updated HandleBranch documentation

2. `Source/TaskSystem/VSGraphExecutor.cpp`
   - Modified HandleBranch() to support Phase 24 presets
   - Added helper functions for environment population
   - Updated error handling

3. (Optional) `Source/Runtime/RuntimeEnvironment.h`
   - No changes (already has SetBlackboardVariable, SetDynamicPinValue)

## Integration Checklist

- [ ] Add includes to VSGraphExecutor.h
- [ ] Implement helper functions (PopulateRuntimeEnvironmentFromBlackboard, etc.)
- [ ] Modify HandleBranch() to check conditionRefs first
- [ ] Maintain priority order (Phase 24 > 23-B.4 > Pin)
- [ ] Add error handling and logging
- [ ] Verify backward compatibility
- [ ] Build and compile successfully
- [ ] Write unit tests
- [ ] Write integration tests
- [ ] Update documentation
- [ ] Code review and verification

---

**Phase 5 Status**: 🎯 **IMPLEMENTATION READY**

**Next Action**: Implement changes to VSGraphExecutor.cpp and VSGraphExecutor.h

