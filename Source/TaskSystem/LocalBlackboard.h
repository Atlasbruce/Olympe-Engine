/**
 * @file LocalBlackboard.h
 * @brief Runtime key-value store for task graph variables
 * @author Olympe Engine
 * @date 2026-02-20
 *
 * @details
 * LocalBlackboard provides a simple unordered_map-based storage for TaskValue
 * variables.  Each TaskRunner instance owns one LocalBlackboard that is
 * initialised from a TaskGraphTemplate (which provides variable names, types
 * and default values) and can be reset to those defaults at any time.
 *
 * C++14 compliant - no std::variant, std::optional, or C++17/20 features.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

#include "TaskGraphTypes.h"

namespace Olympe {

// Forward declaration
class TaskGraphTemplate;

/**
 * @class LocalBlackboard
 * @brief Simple map-based blackboard for task graph runtime state.
 *
 * @details
 * Variables must be registered via Initialize() before use.  SetValue()
 * enforces type compatibility: the new value must match the declared type of
 * the variable.  Throws std::runtime_error on unknown variable or type mismatch.
 */
class LocalBlackboard {
public:

    /**
     * @brief Default constructor.  Blackboard is empty until Initialize() is called.
     */
    LocalBlackboard();

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises the blackboard from a template.
     *
     * Registers all local variables defined in @p tmpl, copying their types
     * and default values.  Any previous state is discarded.
     *
     * @param tmpl The TaskGraphTemplate that owns this blackboard's schema.
     */
    void Initialize(const TaskGraphTemplate& tmpl);

    /**
     * @brief Resets all variables to their default values.
     *
     * Does not change the set of registered variables.
     */
    void Reset();

    // -----------------------------------------------------------------------
    // Variable access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the current value of a variable.
     * @param varName Name of the variable.
     * @throws std::runtime_error if the variable is not registered.
     */
    TaskValue GetValue(const std::string& varName) const;

    /**
     * @brief Sets the value of a variable.
     *
     * The type of @p value must match the declared type of the variable.
     *
     * @param varName Name of the variable.
     * @param value   New value.
     * @throws std::runtime_error if the variable is not registered.
     * @throws std::runtime_error if the type of @p value does not match the
     *         declared type of the variable.
     */
    void SetValue(const std::string& varName, const TaskValue& value);

    // -----------------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if a variable with the given name is registered.
     * @param varName Name of the variable.
     */
    bool HasVariable(const std::string& varName) const;

    /**
     * @brief Returns all registered variable names (useful for debugging / editor).
     */
    std::vector<std::string> GetVariableNames() const;

    // -----------------------------------------------------------------------
    // Persistence
    // -----------------------------------------------------------------------

    /**
     * @brief Serializes all variable names and typed values into a byte buffer.
     *
     * Binary format (little-endian):
     *   uint32_t  count                              - number of variables
     *   for each variable:
     *     uint32_t  nameLen                          - length of name in bytes
     *     <nameLen bytes>                            - variable name (UTF-8)
     *     uint8_t   type                             - VariableType tag
     *     <value bytes>:
     *       Bool    -> uint8_t  (0 = false, 1 = true)
     *       Int     -> int32_t
     *       Float   -> float
     *       Vector  -> float x, float y, float z
     *       EntityID-> uint64_t
     *       String  -> uint32_t len, <len bytes>
     *
     * @param outBytes Output byte buffer (cleared before writing).
     */
    void Serialize(std::vector<uint8_t>& outBytes) const;

    /**
     * @brief Restores variable values from a byte buffer produced by Serialize().
     *
     * The blackboard schema (m_types) must already be initialised via
     * Initialize() before calling this method.  Unknown variable names and
     * type-mismatched entries are skipped with a warning log.  The buffer
     * is consumed in one pass; parsing stops on any truncation error.
     *
     * @param inBytes Byte buffer previously produced by Serialize().
     */
    void Deserialize(const std::vector<uint8_t>& inBytes);

private:

    /// Current values for each registered variable.
    std::unordered_map<std::string, TaskValue>      m_variables;

    /// Default (initial) values used by Reset().
    std::unordered_map<std::string, TaskValue>      m_defaults;

    /// Declared type of each variable (used for type validation in SetValue).
    std::unordered_map<std::string, VariableType>   m_types;
};

} // namespace Olympe
