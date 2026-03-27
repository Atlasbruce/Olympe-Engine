/**
 * @file ConditionPresetEvaluator.cpp
 * @brief Implementation of ConditionPresetEvaluator.
 * @author Olympe Engine
 * @date 2026-03-17
 */

#include "ConditionPresetEvaluator.h"

#include <cmath>
#include <string>
#include <sstream>
#include "../Editor/ConditionPreset/ConditionPresetRegistry.h"

namespace Olympe {

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

/*static*/
bool ConditionPresetEvaluator::Evaluate(
    const ConditionPreset& preset,
    RuntimeEnvironment&    env,
    std::string&           outErrorMsg)
{
    outErrorMsg.clear();

    float leftVal  = 0.0f;
    float rightVal = 0.0f;

    if (!ResolveOperand(preset.left, env, outErrorMsg, leftVal))
        return false;

    if (!ResolveOperand(preset.right, env, outErrorMsg, rightVal))
        return false;

    return EvaluateOperator(leftVal, preset.op, rightVal, outErrorMsg);
}

/*static*/
bool ConditionPresetEvaluator::EvaluateConditionChain(
    const std::vector<NodeConditionRef>& conditions,
    const ConditionPresetRegistry&       registry,
    RuntimeEnvironment&                  env,
    std::string&                         outErrorMsg)
{
    outErrorMsg.clear();

    // ─────────────────────────────────────────────────────────────────────
    // Validation
    // ─────────────────────────────────────────────────────────────────────

    if (conditions.empty())
    {
        outErrorMsg = "ConditionPresetEvaluator::EvaluateConditionChain: Empty condition chain.";
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Evaluate each condition and combine with logical operators
    // ─────────────────────────────────────────────────────────────────────

    // Start with the first condition (LogicalOp::Start is ignored)
    bool currentResult = true;
    bool firstCondition = true;

    for (size_t i = 0; i < conditions.size(); ++i)
    {
        const NodeConditionRef& condRef = conditions[i];

        // ─────────────────────────────────────────────────────────────────
        // Resolve the preset from the registry
        // ─────────────────────────────────────────────────────────────────

        const ConditionPreset* preset = registry.GetPreset(condRef.presetID);

        if (preset == nullptr)
        {
            outErrorMsg = "ConditionPresetEvaluator::EvaluateConditionChain: "
                         "Preset not found in registry: '" + condRef.presetID + "'";
            return false;
        }

        // ─────────────────────────────────────────────────────────────────
        // Evaluate this condition
        // ─────────────────────────────────────────────────────────────────

        bool conditionResult = Evaluate(*preset, env, outErrorMsg);
        if (!outErrorMsg.empty())
        {
            // Evaluation failed
            return false;
        }

        // ─────────────────────────────────────────────────────────────────
        // Apply logical operator (short-circuit evaluation)
        // ─────────────────────────────────────────────────────────────────

        if (firstCondition)
        {
            // First condition: just use its result directly
            currentResult = conditionResult;
            firstCondition = false;
        }
        else
        {
            // Subsequent conditions: combine with logical operator
            switch (condRef.logicalOp)
            {
            case LogicalOp::And:
                // AND: short-circuit if current is false
                currentResult = currentResult && conditionResult;
                if (!currentResult)
                {
                    // Short-circuit: remaining conditions don't matter
                    return false;
                }
                break;

            case LogicalOp::Or:
                // OR: short-circuit if current is true
                currentResult = currentResult || conditionResult;
                if (currentResult)
                {
                    // Short-circuit: remaining conditions don't matter
                    return true;
                }
                break;

            case LogicalOp::Start:
                // Shouldn't occur for non-first conditions, but treat as AND
                currentResult = currentResult && conditionResult;
                if (!currentResult)
                {
                    return false;
                }
                break;

            default:
                outErrorMsg = "ConditionPresetEvaluator::EvaluateConditionChain: "
                             "Unknown LogicalOp.";
                return false;
            }
        }
    }

    return currentResult;
}


// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/*static*/
bool ConditionPresetEvaluator::ResolveOperand(
    const Operand&      operand,
    RuntimeEnvironment& env,
    std::string&        outErrorMsg,
    float&              outValue)
{
    switch (operand.mode)
    {
    case OperandMode::Const:
        outValue = static_cast<float>(operand.constValue);
        return true;

    case OperandMode::Variable:
    {
        float v = 0.0f;
        if (!env.GetBlackboardVariable(operand.stringValue, v))
        {
            outErrorMsg = "ConditionPresetEvaluator: Blackboard variable not found: '"
                        + operand.stringValue + "'";
            return false;
        }
        outValue = v;
        return true;
    }

    case OperandMode::Pin:
    {
        float v = 0.0f;
        if (!env.GetDynamicPinValue(operand.stringValue, v))
        {
            outErrorMsg = "ConditionPresetEvaluator: Dynamic pin value not found for pin: '"
                        + operand.stringValue + "'";
            return false;
        }
        outValue = v;
        return true;
    }

    default:
        outErrorMsg = "ConditionPresetEvaluator: Unknown operand mode.";
        return false;
    }
}

/*static*/
bool ConditionPresetEvaluator::EvaluateOperator(
    float        left,
    ComparisonOp op,
    float        right,
    std::string& outErrorMsg)
{
    switch (op)
    {
    case ComparisonOp::Equal:        return left == right;
    case ComparisonOp::NotEqual:     return left != right;
    case ComparisonOp::Less:         return left <  right;
    case ComparisonOp::LessEqual:    return left <= right;
    case ComparisonOp::Greater:      return left >  right;
    case ComparisonOp::GreaterEqual: return left >= right;
    default:
        outErrorMsg = "ConditionPresetEvaluator: Unknown ComparisonOp.";
        return false;
    }
}

} // namespace Olympe

