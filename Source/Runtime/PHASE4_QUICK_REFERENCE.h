/**
 * @file PHASE4_QUICK_REFERENCE.h
 * @brief Quick reference guide for Phase 4 API
 * @author Olympe Engine
 */

#pragma once

/*
═══════════════════════════════════════════════════════════════════════════════
PHASE 4: RUNTIME CONDITION EVALUATION - QUICK REFERENCE
═══════════════════════════════════════════════════════════════════════════════

This file provides a quick reference for using the Phase 4 condition evaluation
API. For detailed documentation, see PHASE4_CONDITION_EVALUATION.md

═══════════════════════════════════════════════════════════════════════════════
1. BASIC IMPORTS
═══════════════════════════════════════════════════════════════════════════════

#include "Runtime/ConditionPresetEvaluator.h"
#include "Runtime/RuntimeEnvironment.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/NodeConditionRef.h"
#include "Editor/ConditionPreset/Operand.h"

using namespace Olympe;

═══════════════════════════════════════════════════════════════════════════════
2. EVALUATE SINGLE CONDITION
═══════════════════════════════════════════════════════════════════════════════

// Create environment
RuntimeEnvironment env;
env.SetBlackboardVariable("mHealth", 45.0f);

// Create condition: mHealth <= 50
ConditionPreset preset("my_preset",
    Operand::CreateVariable("mHealth"),
    ComparisonOp::LessEqual,
    Operand::CreateConst(50.0f));

// Evaluate
std::string error;
bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);
// result = true, error = ""

═══════════════════════════════════════════════════════════════════════════════
3. EVALUATE CONDITION CHAIN (AND/OR)
═══════════════════════════════════════════════════════════════════════════════

// Setup
std::vector<NodeConditionRef> conditions;
conditions.push_back(NodeConditionRef("preset_1", LogicalOp::Start));  // First
conditions.push_back(NodeConditionRef("preset_2", LogicalOp::And));     // AND
conditions.push_back(NodeConditionRef("preset_3", LogicalOp::Or));      // OR

RuntimeEnvironment env;
env.SetBlackboardVariable("var1", 10.0f);
env.SetBlackboardVariable("var2", 20.0f);
env.SetBlackboardVariable("var3", 30.0f);

// Evaluate
std::string error;
bool result = ConditionPresetEvaluator::EvaluateConditionChain(
    conditions,  // Vector of NodeConditionRef
    registry,    // ConditionPresetRegistry
    env,
    error);

// Evaluation: (preset_1) AND (preset_2) OR (preset_3)

═══════════════════════════════════════════════════════════════════════════════
4. OPERAND MODES
═══════════════════════════════════════════════════════════════════════════════

// Variable mode - lookup from blackboard
Operand var = Operand::CreateVariable("mHealth");
env.SetBlackboardVariable("mHealth", 45.0f);

// Const mode - literal value
Operand const_val = Operand::CreateConst(50.0f);

// Pin mode - dynamic pin value
Operand pin = Operand::CreatePin("pin_uuid");
env.SetDynamicPinValue("pin_uuid", 0.75f);

═══════════════════════════════════════════════════════════════════════════════
5. COMPARISON OPERATORS
═══════════════════════════════════════════════════════════════════════════════

ComparisonOp::Equal;        // ==
ComparisonOp::NotEqual;     // !=
ComparisonOp::Less;         // <
ComparisonOp::LessEqual;    // <=
ComparisonOp::Greater;      // >
ComparisonOp::GreaterEqual; // >=

═══════════════════════════════════════════════════════════════════════════════
6. LOGICAL OPERATORS
═══════════════════════════════════════════════════════════════════════════════

LogicalOp::Start;  // First condition (no combinator)
LogicalOp::And;    // AND with previous
LogicalOp::Or;     // OR with previous

// Example: (A) AND (B) AND (C)
conditions.push_back(NodeConditionRef("a_id", LogicalOp::Start));
conditions.push_back(NodeConditionRef("b_id", LogicalOp::And));
conditions.push_back(NodeConditionRef("c_id", LogicalOp::And));

// Example: (X) OR (Y) OR (Z)
conditions.push_back(NodeConditionRef("x_id", LogicalOp::Start));
conditions.push_back(NodeConditionRef("y_id", LogicalOp::Or));
conditions.push_back(NodeConditionRef("z_id", LogicalOp::Or));

═══════════════════════════════════════════════════════════════════════════════
7. ERROR HANDLING
═══════════════════════════════════════════════════════════════════════════════

std::string error;
bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);

if (!error.empty()) {
    std::cerr << "Condition evaluation failed: " << error << std::endl;
    // Example errors:
    // "Blackboard variable not found: 'mHealth'"
    // "Preset not found in registry: 'preset_999'"
    // "Dynamic pin value not found for pin: 'pin_xyz'"
    return false;
}

═══════════════════════════════════════════════════════════════════════════════
8. BRANCH NODE PATTERN
═══════════════════════════════════════════════════════════════════════════════

void ExecuteBranchNode(const BranchNodeData& node)
{
    // 1. Create and populate environment
    RuntimeEnvironment env;
    env.SetBlackboardVariable("mHealth", blackboard.GetHealth());
    env.SetBlackboardVariable("mFatigue", blackboard.GetFatigue());

    // 2. Set dynamic pin values if applicable
    for (const auto& pin : node.dynamicPins) {
        env.SetDynamicPinValue(pin.id, pin.value);
    }

    // 3. Evaluate conditions
    std::string error;
    bool conditionMet = ConditionPresetEvaluator::EvaluateConditionChain(
        node.conditions, registry, env, error);

    // 4. Check for errors
    if (!error.empty()) {
        LOG_ERROR("Condition failed: " << error);
        ExecuteElseBranch(node);  // Safe fallback
        return;
    }

    // 5. Route to branch
    if (conditionMet) {
        ExecuteThenBranch(node);
    } else {
        ExecuteElseBranch(node);
    }
}

═══════════════════════════════════════════════════════════════════════════════
9. SHORT-CIRCUIT BEHAVIOR
═══════════════════════════════════════════════════════════════════════════════

// AND chains: stops at first FALSE
Condition A = false;  // Evaluated
Condition B = true;   // NOT evaluated (short-circuit)
Condition C = true;   // NOT evaluated
Result: false (stops after A)

// OR chains: stops at first TRUE
Condition X = false;  // Evaluated
Condition Y = true;   // Evaluated (not first TRUE)
Condition Z = true;   // NOT evaluated (stops after Y)
Result: true (stops after Y)

// Performance: 1-100% faster depending on condition structure

═══════════════════════════════════════════════════════════════════════════════
10. COMMON PATTERNS
═══════════════════════════════════════════════════════════════════════════════

// Pattern 1: Simple health check
if (EvaluateCondition(healthPreset, env, error)) {
    TriggerLowHealthEvent();
}

// Pattern 2: Complex decision
bool shouldAttack = EvaluateConditionChain(
    {lowHealthCond, highFatigueCond},
    registry, env, error);

// Pattern 3: Multi-branch
if (EvaluateConditionChain(path1Conditions, ...)) {
    ExecutePath1();
} else if (EvaluateConditionChain(path2Conditions, ...)) {
    ExecutePath2();
} else {
    ExecuteDefaultPath();
}

═══════════════════════════════════════════════════════════════════════════════
11. PERFORMANCE TIPS
═══════════════════════════════════════════════════════════════════════════════

1. Place fast conditions first in AND chains
   - If first condition is usually false, no other evaluations needed

2. Place likely-true conditions first in OR chains
   - If first condition is usually true, no other evaluations needed

3. Avoid complex conditions as first in chain
   - Short-circuit optimization most effective with cheap first tests

4. Reuse RuntimeEnvironment when possible
   - Avoid creating new environment for each frame
   - Update values instead of creating new instance

═══════════════════════════════════════════════════════════════════════════════
12. DEBUGGING
═══════════════════════════════════════════════════════════════════════════════

// Debug: Print condition evaluation
std::string error;
bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);
std::cout << "Condition result: " << (result ? "TRUE" : "FALSE") << std::endl;
if (!error.empty()) {
    std::cout << "Error: " << error << std::endl;
}

// Debug: Check environment state
float val;
if (env.GetBlackboardVariable("mHealth", val)) {
    std::cout << "mHealth = " << val << std::endl;
} else {
    std::cout << "mHealth not in environment" << std::endl;
}

// Debug: Chain evaluation step-by-step
for (size_t i = 0; i < conditions.size(); ++i) {
    const auto& cond = conditions[i];
    std::cout << "Condition " << i << ": " << cond.presetID << std::endl;
}

═══════════════════════════════════════════════════════════════════════════════
13. API SUMMARY
═══════════════════════════════════════════════════════════════════════════════

ConditionPresetEvaluator::Evaluate(
    const ConditionPreset& preset,
    RuntimeEnvironment& env,
    std::string& outErrorMsg)
    -> bool result

ConditionPresetEvaluator::EvaluateConditionChain(
    const std::vector<NodeConditionRef>& conditions,
    const ConditionPresetRegistry& registry,
    RuntimeEnvironment& env,
    std::string& outErrorMsg)
    -> bool result

RuntimeEnvironment::SetBlackboardVariable(key, value)
RuntimeEnvironment::GetBlackboardVariable(key, outValue) -> bool
RuntimeEnvironment::SetDynamicPinValue(pinID, value)
RuntimeEnvironment::GetDynamicPinValue(pinID, outValue) -> bool
RuntimeEnvironment::Clear()

═══════════════════════════════════════════════════════════════════════════════
14. BEST PRACTICES
═══════════════════════════════════════════════════════════════════════════════

✓ DO:
  - Check error messages on evaluation failure
  - Populate RuntimeEnvironment completely before evaluation
  - Structure AND chains with likely-false conditions first
  - Structure OR chains with likely-true conditions first
  - Reuse RuntimeEnvironment across multiple evaluations
  - Comment complex condition chains

✗ DON'T:
  - Ignore error messages
  - Create new RuntimeEnvironment for each frame
  - Put expensive computations in operands
  - Leave uninitialized blackboard variables
  - Mix too many conditions without grouping (if possible)

═══════════════════════════════════════════════════════════════════════════════
15. FURTHER READING
═══════════════════════════════════════════════════════════════════════════════

- PHASE4_CONDITION_EVALUATION.md - Full documentation
- PHASE4_COMPLETION_REPORT.md - Implementation details
- Phase4_ConditionEvaluation_Examples.cpp - 7 working examples

═══════════════════════════════════════════════════════════════════════════════
STATUS: ✅ PHASE 4 COMPLETE - PRODUCTION READY
BUILD: ✅ Génération réussie (0 errors)
═══════════════════════════════════════════════════════════════════════════════
*/
