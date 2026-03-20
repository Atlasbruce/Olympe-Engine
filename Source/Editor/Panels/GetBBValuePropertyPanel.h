/**
 * @file GetBBValuePropertyPanel.h
 * @brief UI Properties panel for a GetBBValue node – variable selector with preview.
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * @details
 * GetBBValuePropertyPanel renders the Properties panel for a selected GetBBValue node.
 * It provides:
 *   - Variable dropdown (filtered by type)
 *   - Variable type display
 *   - Default value preview
 *   - Real-time canvas updates
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
 * @class GetBBValuePropertyPanel
 * @brief ImGui sub-panel for editing GetBBValue node's blackboard variable selection.
 *
 * @details
 * The panel does NOT own the template — it holds a reference.
 * The caller is responsible for persisting changes back to the node definition.
 */
class GetBBValuePropertyPanel {
public:

    /**
     * @brief Constructs the panel.
     */
    explicit GetBBValuePropertyPanel();

    ~GetBBValuePropertyPanel() = default;

    // Non-copyable
    GetBBValuePropertyPanel(const GetBBValuePropertyPanel&) = delete;
    GetBBValuePropertyPanel& operator=(const GetBBValuePropertyPanel&) = delete;

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
    // Private rendering methods
    // -----------------------------------------------------------------------

    /// Renders the title section (node name in blue background).
    void RenderTitleSection();

    /// Renders the variable dropdown and info.
    void RenderVariableSelector();

    /// Renders variable type and default value information.
    void RenderVariableInfo();

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    std::string m_nodeName;                ///< Node name for display
    std::string m_bbKey;                   ///< Current BB key selection
    const TaskGraphTemplate* m_template;   ///< Template reference (not owned)
    bool m_dirty = false;                  ///< Changed since last ClearDirty()
};

} // namespace Olympe
