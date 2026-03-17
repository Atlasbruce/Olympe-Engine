/**
 * @file RuntimeEnvironment.h
 * @brief Runtime context supplying Blackboard variables and dynamic pin values.
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * RuntimeEnvironment encapsulates the two sources of float data that
 * ConditionEvaluator needs when it resolves Operands at runtime:
 *
 *   - Blackboard variables  (Variable-mode operands)
 *   - Dynamic pin values    (Pin-mode operands)
 *
 * Const-mode operands carry their value directly in the Operand struct and
 * therefore do not consult the RuntimeEnvironment at all.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <map>
#include <string>

namespace Olympe {

/**
 * @class RuntimeEnvironment
 * @brief Provides Blackboard variable values and dynamic pin values at runtime.
 *
 * @details
 * Acts as the single source of truth for all float data during condition
 * evaluation.  Populated before evaluation begins (or updated dynamically
 * as connected nodes provide new data).
 */
class RuntimeEnvironment {
public:

    RuntimeEnvironment() = default;

    // -----------------------------------------------------------------------
    // Blackboard variables
    // -----------------------------------------------------------------------

    /**
     * @brief Stores or overwrites a Blackboard variable.
     * @param key   Variable identifier (e.g. "mHealth").
     * @param value Float value to associate with the key.
     */
    void SetBlackboardVariable(const std::string& key, float value);

    /**
     * @brief Looks up a Blackboard variable.
     * @param key      Variable identifier.
     * @param outValue Receives the value when found.
     * @return true when the key exists; false otherwise.
     */
    bool GetBlackboardVariable(const std::string& key, float& outValue) const;

    /**
     * @brief Returns true when the given key has been set.
     * @param key Variable identifier.
     */
    bool HasBlackboardVariable(const std::string& key) const;

    // -----------------------------------------------------------------------
    // Dynamic pin values
    // -----------------------------------------------------------------------

    /**
     * @brief Stores or overwrites the runtime value for a dynamic data pin.
     * @param pinID Pin UUID (matches DynamicDataPin::id or NodeConditionRef::leftPinID / rightPinID).
     * @param value Float value delivered by the connected node.
     */
    void SetDynamicPinValue(const std::string& pinID, float value);

    /**
     * @brief Looks up the runtime value of a dynamic data pin.
     * @param pinID    Pin UUID.
     * @param outValue Receives the value when found.
     * @return true when the pin ID exists; false otherwise.
     */
    bool GetDynamicPinValue(const std::string& pinID, float& outValue) const;

    // -----------------------------------------------------------------------
    // Utility
    // -----------------------------------------------------------------------

    /**
     * @brief Removes all stored variables and pin values.
     */
    void Clear();

private:

    std::map<std::string, float> m_blackboardVariables; ///< Blackboard key → value
    std::map<std::string, float> m_dynamicPinValues;    ///< Pin UUID → value
};

} // namespace Olympe
