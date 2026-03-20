/**
 * @file SetBBValuePropertyPanel.h
 * @brief UI Properties panel for a SetBBValue node – variable and value editor.
 * @author Olympe Engine
 * @date 2026-03-18
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

/**
 * @class SetBBValuePropertyPanel
 * @brief ImGui sub-panel for editing SetBBValue node's variable selection and value.
 */
class SetBBValuePropertyPanel {
public:

    explicit SetBBValuePropertyPanel();

    ~SetBBValuePropertyPanel() = default;

    // Non-copyable
    SetBBValuePropertyPanel(const SetBBValuePropertyPanel&) = delete;
    SetBBValuePropertyPanel& operator=(const SetBBValuePropertyPanel&) = delete;

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    void SetNodeName(const std::string& name);
    void SetTemplate(const TaskGraphTemplate* tmpl);
    void SetBBKey(const std::string& key);

    const std::string& GetBBKey() const;

    bool IsDirty() const { return m_dirty; }
    void ClearDirty() { m_dirty = false; }

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------

    void Render();

private:

    void RenderTitleSection();
    void RenderVariableSelector();
    void RenderValueInput();

    std::string m_nodeName;
    std::string m_bbKey;
    const TaskGraphTemplate* m_template;
    bool m_dirty = false;
};

} // namespace Olympe
