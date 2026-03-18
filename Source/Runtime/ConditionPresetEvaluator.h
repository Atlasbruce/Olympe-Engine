/**
 * @file ConditionPresetEvaluator.h
 * @brief Stateless runtime evaluator for ConditionPreset expressions.
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * ConditionPresetEvaluator is a fully stateless utility class.  All methods are
 * static.  It evaluates a ConditionPreset by:
 *
 *   1. Resolving the left Operand to a float (via the RuntimeEnvironment).
 *   2. Resolving the right Operand to a float (via the RuntimeEnvironment).
 *   3. Applying the ComparisonOp to produce a bool result.
 *
 * Operand resolution strategy:
 *   - Variable  → query RuntimeEnvironment::GetBlackboardVariable()
 *   - Const     → use Operand::constValue directly (no environment access)
 *   - Pin       → query RuntimeEnvironment::GetDynamicPinValue()
 *
 * All errors are reported through the outErrorMsg parameter; the return value
 * is always false when an error occurs.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include "../Editor/ConditionPreset/ConditionPreset.h"
#include "../Editor/ConditionPreset/Operand.h"
#include "RuntimeEnvironment.h"

namespace Olympe {

/**
 * @class ConditionPresetEvaluator
 * @brief Evaluates a ConditionPreset against a RuntimeEnvironment.
 *
 * @details
 * Typical usage:
 * @code
 *   RuntimeEnvironment env;
 *   env.SetBlackboardVariable("mHealth", 1.0f);
 *
 *   ConditionPreset preset("p1",
 *       Operand::CreateVariable("mHealth"),
 *       ComparisonOp::LessEqual,
 *       Operand::CreateConst(2.0));
 *
 *   std::string error;
 *   bool result = ConditionPresetEvaluator::Evaluate(preset, env, error);
 *   // result == true, error == ""
 * @endcode
 */
class ConditionPresetEvaluator {
public:

    /**
     * @brief Evaluates a ConditionPreset and returns the boolean result.
     *
     * @param preset      The condition preset to evaluate.
     * @param env         Runtime environment providing variable and pin values.
     * @param outErrorMsg Receives a human-readable error description on failure.
     *                    Empty on success.
     * @return true when the condition holds; false on failure or when the
     *         comparison evaluates to false.
     */
    static bool Evaluate(
        const ConditionPreset& preset,
        RuntimeEnvironment&    env,
        std::string&           outErrorMsg);

private:

    /**
     * @brief Resolves an Operand to its runtime float value.
     *
     * @param operand     Operand to resolve.
     * @param env         RuntimeEnvironment used for Variable / Pin lookup.
     * @param outErrorMsg Receives an error description when resolution fails.
     * @param outValue    Receives the resolved float value on success.
     * @return true on success; false when the variable / pin is not found.
     */
    static bool ResolveOperand(
        const Operand&      operand,
        RuntimeEnvironment& env,
        std::string&        outErrorMsg,
        float&              outValue);

    /**
     * @brief Applies a ComparisonOp to two float values.
     *
     * @param left        Left-hand side value.
     * @param op          Operator to apply.
     * @param right       Right-hand side value.
     * @param outErrorMsg Receives an error description for unknown operators.
     * @return Result of the comparison, or false on unknown operator.
     */
    static bool EvaluateOperator(
        float        left,
        ComparisonOp op,
        float        right,
        std::string& outErrorMsg);
};

/// @brief Backward-compatible alias — use ConditionPresetEvaluator for new code.
using ConditionEvaluator = ConditionPresetEvaluator;

} // namespace Olympe

