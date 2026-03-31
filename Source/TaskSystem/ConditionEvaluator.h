/**
 * @file ConditionEvaluator.h
 * @brief Evaluates structured Condition expressions for Branch/While nodes.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * Supports all 6 source combinations for each side of a condition:
 *   - Variable vs Variable  (health < max_health)
 *   - Variable vs Const     (health > 50)
 *   - Variable vs Pin       (health == Node#42.Out)
 *   - Pin vs Pin            (Node#42.Out > Node#43.Out)
 *   - Pin vs Const          (Node#42.Out >= threshold)
 *   - Const vs Const        (50 < 100)
 *
 * Graceful handling:
 *   - Missing Variable: logs WARNING, returns false
 *   - Missing Pin:      logs WARNING, returns false
 *   - Const:            always available, no error
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "TaskGraphTypes.h"
#include "LocalBlackboard.h"

namespace Olympe {

/**
 * @class ConditionEvaluator
 * @brief Stateless evaluator for structured Condition expressions.
 *
 * @details
 * All methods are static.  Call EvaluateAll() with the full conditions list
 * from a TaskNodeDefinition to compute the implicit-AND result.
 *
 * Pin values are looked up in the DataPinCache map (keyed as "nodeID:pinName").
 * The pin field format is "Node#<id>.<pinName>", e.g. "Node#42.Out".
 */
class ConditionEvaluator {
public:

    /**
     * @brief Evaluates all conditions (implicit AND) on a Branch/While node.
     *
     * @param conditions   The conditions list from the node definition.
     * @param localBB      The entity's local blackboard (for Variable mode).
     * @param dataPinCache Frame-local data pin cache (for Pin mode).
     * @param nodeID       Node ID used in log messages.
     * @return true if all conditions evaluate to true (or list is empty).
     */
    static bool EvaluateAll(
        const std::vector<Condition>& conditions,
        const LocalBlackboard& localBB,
        const std::unordered_map<std::string, TaskValue>& dataPinCache,
        int32_t nodeID);

    /**
     * @brief Evaluates a single Condition.
     *
     * @param cond         The condition to evaluate.
     * @param localBB      The entity's local blackboard.
     * @param dataPinCache Frame-local data pin cache.
     * @param nodeID       Node ID for log context.
     * @return true if the condition holds; false on any error or when false.
     */
    static bool EvaluateCondition(
        const Condition& cond,
        const LocalBlackboard& localBB,
        const std::unordered_map<std::string, TaskValue>& dataPinCache,
        int32_t nodeID);

    /**
     * @brief Resolves the TaskValue for one side of a condition.
     *
     * @param mode         "Pin", "Variable", or "Const".
     * @param pin          Pin reference string (mode == "Pin").
     * @param variable     Variable name (mode == "Variable").
     * @param constValue   Literal value (mode == "Const").
     * @param localBB      Local blackboard.
     * @param dataPinCache Data pin cache.
     * @param nodeID       Node ID for logging.
     * @param outValue     Output value (only valid when return is true).
     * @return true if the value was successfully resolved.
     */
    static bool GetConditionValue(
        const std::string& mode,
        const std::string& pin,
        const std::string& variable,
        const TaskValue& constValue,
        const LocalBlackboard& localBB,
        const std::unordered_map<std::string, TaskValue>& dataPinCache,
        int32_t nodeID,
        TaskValue& outValue);

    /**
     * @brief Compares two TaskValues using the given operator.
     *
     * Supported operators: "==", "!=", "<", ">", "<=", ">=".
     * If types differ, attempts Int->Float promotion.
     * On unresolvable mismatch logs WARNING and returns false.
     *
     * @param left     Left-hand value.
     * @param right    Right-hand value.
     * @param op       Operator string.
     * @param nodeID   Node ID for logging.
     * @return Result of the comparison, or false on error.
     */
    static bool CompareValues(const TaskValue& left,
                              const TaskValue& right,
                              const std::string& op,
                              int32_t nodeID);

private:
    ConditionEvaluator() = delete;

    /// Parses "Node#42.Out" -> nodeID=42, pinName="Out".
    /// Returns false if the format is not recognised.
    static bool ParsePinRef(const std::string& pinRef,
                            int32_t& outNodeID,
                            std::string& outPinName);
};

} // namespace Olympe
