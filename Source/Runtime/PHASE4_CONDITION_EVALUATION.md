# Phase 4: Runtime Condition Evaluation

## Overview

Phase 4 implements **runtime evaluation of conditions** that were created and configured in the UI during Phases 1-3. This phase enables:

1. **Single Condition Evaluation** - Evaluate a single `ConditionPreset`
2. **Condition Chains** - Evaluate multiple conditions combined with AND/OR operators
3. **Variable Resolution** - Support for Variable-mode, Const-mode, and Pin-mode operands
4. **Error Handling** - Comprehensive error reporting for debugging

## Architecture

### Core Components

#### 1. **ConditionPresetEvaluator** (`ConditionPresetEvaluator.h / .cpp`)
Stateless utility class providing two main evaluation methods:

```cpp
// Evaluate a single condition
static bool Evaluate(
    const ConditionPreset& preset,
    RuntimeEnvironment&    env,
    std::string&           outErrorMsg);

// Evaluate multiple conditions with logical operators
static bool EvaluateConditionChain(
    const std::vector<NodeConditionRef>& conditions,
    const ConditionPresetRegistry&       registry,
    RuntimeEnvironment&                  env,
    std::string&                         outErrorMsg);
```

#### 2. **RuntimeEnvironment** (`RuntimeEnvironment.h / .cpp`)
Provides two sources of runtime data:

```cpp
// Blackboard variables (Variable-mode operands)
void SetBlackboardVariable(const std::string& key, float value);
bool GetBlackboardVariable(const std::string& key, float& outValue) const;

// Dynamic pin values (Pin-mode operands)
void SetDynamicPinValue(const std::string& pinID, float value);
bool GetDynamicPinValue(const std::string& pinID, float& outValue) const;
```

#### 3. **ConditionPreset** (`ConditionPreset.h`)
Represents a single boolean comparison:
```
<left Operand> <ComparisonOp> <right Operand>
```

#### 4. **NodeConditionRef** (`NodeConditionRef.h`)
References a `ConditionPreset` from a Branch node, with support for:
- Logical operators (AND/OR) combining multiple conditions
- Dynamic pin IDs for Pin-mode operands

### Operand Modes

| Mode | Source | Example |
|------|--------|---------|
| **Variable** | Blackboard | `mHealth` (value looked up at runtime) |
| **Const** | Literal value | `2.0` (embedded in the preset) |
| **Pin** | Dynamic data pin | `Pin-in #1` (value from connected node) |

### Comparison Operators

- `Equal` (==)
- `NotEqual` (!=)
- `Less` (<)
- `LessEqual` (<=)
- `Greater` (>)
- `GreaterEqual` (>=)

## Usage Examples

### Example 1: Simple Condition Evaluation

```cpp
#include "Runtime/ConditionPresetEvaluator.h"
#include "Runtime/RuntimeEnvironment.h"

// Create environment
RuntimeEnvironment env;
env.SetBlackboardVariable("mHealth", 50.0f);

// Create a condition: mHealth <= 100
ConditionPreset preset(
    "preset_001",
    Operand::CreateVariable("mHealth"),
    ComparisonOp::LessEqual,
    Operand::CreateConst(100.0f)
);

// Evaluate
std::string error;
bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);
// result == true (50 <= 100)
```

### Example 2: Condition Chain with AND/OR

```cpp
// Conditions: (mHealth <= 50) AND (mFatigue > 75)
std::vector<NodeConditionRef> conditions;
conditions.push_back(NodeConditionRef("preset_001", LogicalOp::Start));  // First condition
conditions.push_back(NodeConditionRef("preset_002", LogicalOp::And));    // AND second

// Populate environment
RuntimeEnvironment env;
env.SetBlackboardVariable("mHealth", 30.0f);   // true: 30 <= 50
env.SetBlackboardVariable("mFatigue", 80.0f);  // true: 80 > 75

// Evaluate chain
std::string error;
bool result = ConditionPresetEvaluator::EvaluateConditionChain(
    conditions, registry, env, error);
// result == true (true AND true)
```

### Example 3: Pin-Mode Operand

```cpp
// Condition: Input Pin Value >= 0.5
ConditionPreset preset(
    "preset_003",
    Operand::CreatePin("pin_uuid_123"),
    ComparisonOp::GreaterEqual,
    Operand::CreateConst(0.5f)
);

RuntimeEnvironment env;
env.SetDynamicPinValue("pin_uuid_123", 0.75f);  // Connected node output

bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);
// result == true (0.75 >= 0.5)
```

## Integration Points

### Branch Node Execution

When a Branch node executes, it should:

1. **Populate RuntimeEnvironment** with current blackboard values and pin values
2. **Call EvaluateConditionChain** to determine which branch path to execute
3. **Route execution** to "Then" branch if true, "Else" branch if false

```cpp
// Pseudocode for branch node execution
void ExecuteBranchNode(const NodeData& branchNode, RuntimeEnvironment& env)
{
    const auto& conditionRefs = branchNode.conditions;  // Vector<NodeConditionRef>

    std::string error;
    bool conditionResult = ConditionPresetEvaluator::EvaluateConditionChain(
        conditionRefs, registry, env, error);

    if (!error.empty()) {
        LOG_ERROR("Condition evaluation failed: " << error);
        // Handle error: execute Else branch or skip node
        return;
    }

    if (conditionResult) {
        // Execute "Then" branch
        ExecuteConnectedNodes(branchNode.GetOutput("Then"), env);
    } else {
        // Execute "Else" branch
        ExecuteConnectedNodes(branchNode.GetOutput("Else"), env);
    }
}
```

## Error Handling

All evaluation methods return false on error and populate `outErrorMsg`. Common error scenarios:

| Error | Cause | Recovery |
|-------|-------|----------|
| "Blackboard variable not found: 'mHealth'" | Variable doesn't exist in environment | Populate env with missing variable |
| "Preset not found in registry: 'preset_999'" | Referenced preset doesn't exist | Verify preset ID is correct |
| "Dynamic pin value not found for pin: 'pin_xyz'" | Pin not connected or value not set | Connect node output to pin |
| "Unknown ComparisonOp" | Invalid operator in preset | Rebuild/reload preset |

## Short-Circuit Evaluation

The `EvaluateConditionChain` method implements efficient short-circuit evaluation:

- **AND chains**: Stops at first `false` condition (doesn't evaluate remaining)
- **OR chains**: Stops at first `true` condition (doesn't evaluate remaining)

```cpp
// Example: (A AND B AND C)
// If A=false, B and C are never evaluated
// Performance: O(k) where k is position of first failing condition

// Example: (X OR Y OR Z)
// If X=true, Y and Z are never evaluated
// Performance: O(k) where k is position of first passing condition
```

## Testing Phase 4

### Unit Test Template

```cpp
#include <gtest/gtest.h>
#include "Runtime/ConditionPresetEvaluator.h"

TEST(ConditionEvaluation, SimpleComparison)
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("mHealth", 50.0f);

    ConditionPreset preset("test",
        Operand::CreateVariable("mHealth"),
        ComparisonOp::LessEqual,
        Operand::CreateConst(100.0f));

    std::string error;
    bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);

    EXPECT_TRUE(result);
    EXPECT_EQ(error, "");
}

TEST(ConditionEvaluation, ChainWithAND)
{
    RuntimeEnvironment env;
    env.SetBlackboardVariable("health", 30.0f);
    env.SetBlackboardVariable("fatigue", 80.0f);

    std::vector<NodeConditionRef> conditions;
    conditions.push_back(NodeConditionRef("preset_health", LogicalOp::Start));
    conditions.push_back(NodeConditionRef("preset_fatigue", LogicalOp::And));

    std::string error;
    bool result = ConditionPresetEvaluator::EvaluateConditionChain(
        conditions, registry, env, error);

    EXPECT_TRUE(result);
}
```

## Migration from Phase 3

Phase 3 created the UI for condition editing. Phase 4 adds runtime evaluation. No breaking changes:

- **Phase 3 artifacts**: ConditionPreset objects in JSON, NodeConditionRef in templates
- **Phase 4 addition**: ConditionPresetEvaluator methods for runtime use
- **Backward compatible**: All Phase 3 structures remain unchanged

## Future Enhancements (Phase 5+)

- Nested conditions (parenthesized sub-expressions)
- Custom comparison operators
- Weighted condition chains
- Condition caching for performance optimization
- Telemetry/debugging output for condition chains

## Files Modified in Phase 4

- `Source/Runtime/ConditionPresetEvaluator.h` - Added `EvaluateConditionChain()`
- `Source/Runtime/ConditionPresetEvaluator.cpp` - Implemented chain evaluation with short-circuit logic
- `Source/Runtime/RuntimeEnvironment.h` - No changes (already existed)
- `Source/Runtime/RuntimeEnvironment.cpp` - No changes (already existed)

## Build Status

✅ **Phase 4 Code Compiles Successfully**
- 0 compilation errors
- 0 linker errors
- Ready for integration into task graph executor

---

**Phase 4 Status**: ✅ **COMPLETE - Ready for Integration**

**Next Steps**:
1. Integrate `EvaluateConditionChain()` into Branch node execution logic
2. Add unit tests for all comparison operators
3. Add integration tests for AND/OR chains
4. Verify short-circuit evaluation with profiler
