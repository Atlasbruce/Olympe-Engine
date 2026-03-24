/**
 * @file ActionParametersPanel.h
 * @brief UI Properties panel for atomic task (action) node parameters.
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * Displays editable parameters for selected action nodes.
 * Supports various parameter types (String, Float, Int, Bool, etc.).
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Olympe {

/**
 * @struct ActionParameter
 * @brief Represents a single parameter of an action task.
 */
struct ActionParameter
{
    std::string name;        ///< Parameter name (e.g., "message", "speed")
    std::string type;        ///< Parameter type (String, Float, Int, Bool, etc.)
    std::string value;       ///< Current value
    std::string defaultValue; ///< Default value
    bool isDirty = false;    ///< True if value has been modified
};

/**
 * @class ActionParametersPanel
 * @brief ImGui sub-panel for editing action node parameters.
 */
class ActionParametersPanel
{
public:
    explicit ActionParametersPanel();
    ~ActionParametersPanel() = default;

    // Non-copyable
    ActionParametersPanel(const ActionParametersPanel&) = delete;
    ActionParametersPanel& operator=(const ActionParametersPanel&) = delete;

    // -----------------------------------------------------------------------
    // State setters
    // -----------------------------------------------------------------------

    /**
     * @brief Set the action task ID (e.g., "log_message", "patrol_path")
     */
    void SetActionTaskID(const std::string& taskID);

    /**
     * @brief Set the node name for display purposes
     */
    void SetNodeName(const std::string& name);

    /**
     * @brief Set the parameters from a map of name->value pairs
     */
    void SetParameters(const std::unordered_map<std::string, std::string>& params);

    /**
     * @brief Get all parameters with their current values
     */
    const std::vector<ActionParameter>& GetParameters() const;

    /**
     * @brief Get a specific parameter value by name
     */
    std::string GetParameterValue(const std::string& paramName) const;

    /**
     * @brief Check if any parameter has been modified
     */
    bool IsDirty() const;

    /**
     * @brief Clear the dirty flag after changes have been applied
     */
    void ClearDirty();

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------

    void Render();

private:
    void RenderTitleSection();
    void RenderParametersSection();
    void RenderParameter(ActionParameter& param);

    std::string m_taskID;
    std::string m_nodeName;
    std::vector<ActionParameter> m_parameters;
    bool m_dirty = false;
};

} // namespace Olympe
