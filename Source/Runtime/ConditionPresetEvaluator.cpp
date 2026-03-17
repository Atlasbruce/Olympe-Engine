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

