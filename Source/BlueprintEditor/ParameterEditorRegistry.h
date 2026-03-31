/**
 * @file ParameterEditorRegistry.h
 * @brief Centralized registry mapping node types to their parameter descriptors.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * ParameterEditorRegistry provides the properties panel with the full
 * description of each parameter for every node type — its name, binding type,
 * whether it is required, etc.  The panel uses these descriptors to choose
 * the right widget (text field, dropdown, file picker, etc.) for each field.
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
 * @struct ParameterDescriptor
 * @brief Full description of one parameter on a node type.
 */
struct ParameterDescriptor {
    std::string          name;             ///< Parameter key (e.g. "taskType", "bbKey")
    ParameterBindingType bindingType;      ///< How the parameter value is provided
    VariableType         literalValueType; ///< Type hint when bindingType == Literal
    std::string          description;      ///< Tooltip text
    bool                 required;         ///< Must be filled before the node can execute

    /// Optional filter — restricts variable/ID dropdowns to a specific category.
    std::string filterCategory;

    /// Optional filter — restricts BB-variable dropdowns to a specific VariableType.
    VariableType filterVarType = VariableType::None;

    /// Whether the parameter can accept multiple values (future use).
    bool allowMultiple = false;
};

/**
 * @class ParameterEditorRegistry
 * @brief Singleton mapping TaskNodeType -> vector of ParameterDescriptor.
 */
class ParameterEditorRegistry {
public:

    /**
     * @brief Returns the singleton instance.
     *        Calls InitializeBuiltInParameters() on first access.
     */
    static ParameterEditorRegistry& Get();

    /**
     * @brief Registers a set of parameter descriptors for a node type.
     *
     * Overwrites any previously registered descriptors for @p nodeType.
     *
     * @param nodeType   The node type.
     * @param descriptors Ordered list of parameter descriptors.
     */
    void RegisterNodeType(TaskNodeType nodeType,
                          const std::vector<ParameterDescriptor>& descriptors);

    /**
     * @brief Returns the parameter descriptors for the given node type.
     *
     * Returns an empty vector if no descriptors have been registered for
     * @p nodeType.
     *
     * @param nodeType The node type.
     * @return Reference to the descriptors vector.
     */
    const std::vector<ParameterDescriptor>& GetNodeParameters(TaskNodeType nodeType) const;

    /**
     * @brief Returns the descriptor for a specific parameter of a node type.
     *
     * @param nodeType  The node type.
     * @param paramName Parameter name.
     * @return Pointer to the descriptor, or nullptr if not found.
     */
    const ParameterDescriptor* GetParameterDescriptor(TaskNodeType nodeType,
                                                       const std::string& paramName) const;

private:

    ParameterEditorRegistry();

    void InitializeBuiltInParameters();

    std::unordered_map<uint8_t, std::vector<ParameterDescriptor>> m_params;

    /// Returned by GetNodeParameters when a type has no descriptors.
    static const std::vector<ParameterDescriptor> s_empty;
};

} // namespace Olympe
