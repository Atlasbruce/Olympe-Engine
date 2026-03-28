/**
 * @file VariablePropertyPanel.h
 * @brief UI Properties panel for a Variable node – variable selector.
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * @details
 * VariablePropertyPanel renders the Properties panel for a selected Variable (GetBBValue) node.
 * It provides a variable dropdown selector identical to GetBBValuePropertyPanel.
 *
 * Phase 24.1: Data Pure Nodes
 * 
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

/**
 * @class VariablePropertyPanel
 * @brief ImGui sub-panel for editing Variable node's blackboard variable selection.
 *
 * Identical to GetBBValuePropertyPanel - provides variable dropdown selector.
 */
class VariablePropertyPanel {
public:

    /**
     * @brief Constructs the panel.
     */
    explicit VariablePropertyPanel();

    ~VariablePropertyPanel() = default;

    // Non-copyable
    VariablePropertyPanel(const VariablePropertyPanel&) = delete;
    VariablePropertyPanel& operator=(const VariablePropertyPanel&) = delete;

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the node name displayed in the title section.
     */
    void SetNodeName(const std::string& name);

    /**
     * @brief Sets the template reference for blackboard variable lookup.
     */
    void SetTemplate(const TaskGraphTemplate* tmpl);

    /**
     * @brief Sets the currently selected BB key.
     */
    void SetBBKey(const std::string& key);

    /**
     * @brief Returns the current (possibly modified) BB key.
     */
    const std::string& GetBBKey() const;

    /**
     * @brief Returns true if the BB key has been modified since the
     *        last call to ClearDirty().
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * @brief Resets the dirty flag.
     */
    void ClearDirty() { m_dirty = false; }

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the panel into the current ImGui context.
     */
    void Render();

private:

    // -----------------------------------------------------------------------
    // Private state
    // -----------------------------------------------------------------------

    std::string m_nodeName;
    std::string m_selectedBBKey;
    const TaskGraphTemplate* m_template;
    bool m_dirty;
    std::vector<std::string> m_availableVariables;

    /// @brief Rebuilds the list of available blackboard variables.
    void RebuildVariableList();
};

} // namespace Olympe

