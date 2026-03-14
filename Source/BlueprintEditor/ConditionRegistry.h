/**
 * @file ConditionRegistry.h
 * @brief Registry of available condition types for Branch/While node dropdowns.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * ConditionRegistry stores metadata (id, displayName, description, parameters)
 * for each supported condition type. Used by the properties panel to populate
 * the ConditionID dropdown on Branch and While nodes.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "../TaskSystem/TaskGraphTypes.h"

namespace Olympe {

/**
 * @struct ConditionParamSpec
 * @brief Describes one parameter of a condition (e.g. the Key or Operator).
 */
struct ConditionParamSpec {
    std::string          name;           ///< Parameter name (e.g. "Key", "Operator")
    ParameterBindingType bindingType;    ///< Expected binding type
    std::string          description;    ///< Short description
    bool                 required;       ///< Whether the parameter is mandatory
};

/**
 * @struct ConditionSpec
 * @brief Full metadata for a single condition type.
 */
struct ConditionSpec {
    std::string                     id;           ///< Condition type ID (e.g. "CompareValue")
    std::string                     displayName;  ///< Human-readable name
    std::string                     description;  ///< Short description
    std::vector<ConditionParamSpec> parameters;   ///< Expected parameters in order
};

/**
 * @class ConditionRegistry
 * @brief Singleton registry of available condition types.
 */
class ConditionRegistry {
public:

    /**
     * @brief Returns the singleton instance.
     *        Calls InitializeBuiltInConditions() on first access.
     */
    static ConditionRegistry& Get();

    /**
     * @brief Registers a ConditionSpec.
     * @param spec Spec to register (id must be non-empty).
     */
    void Register(const ConditionSpec& spec);

    /**
     * @brief Returns the ConditionSpec for the given id, or nullptr if not found.
     * @param id Condition type ID.
     */
    const ConditionSpec* GetConditionSpec(const std::string& id) const;

    /**
     * @brief Returns all registered condition IDs.
     */
    std::vector<std::string> GetAllConditionIds() const;

    /**
     * @brief Returns all registered condition specs, sorted by id.
     */
    std::vector<ConditionSpec> GetAllConditions() const;

private:

    ConditionRegistry();

    void InitializeBuiltInConditions();

    std::unordered_map<std::string, ConditionSpec> m_specs;
};

} // namespace Olympe
