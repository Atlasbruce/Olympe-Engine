/**
 * @file BlackboardPanel.h
 * @brief ImGui panel for editing blackboard variables (Phase 2.1)
 * @author Olympe Engine
 * @date 2026-02-19
 */

#pragma once

#include "../../NodeGraphCore/BlackboardSystem.h"
#include <string>

namespace Olympe {
namespace AI {

/**
 * @class BlackboardPanel
 * @brief Full-featured ImGui panel for editing BlackboardSystem variables
 *
 * @details
 * Allows listing, creating, editing, and deleting blackboard entries.
 * All types (Int, Float, Bool, String, Vector3) are supported with
 * appropriate widgets.
 */
class BlackboardPanel {
public:
    BlackboardPanel();
    ~BlackboardPanel() = default;

    /**
     * @brief Render the panel
     * @param blackboard Blackboard system to display/edit
     * @param pOpen Pointer to visibility flag (nullptr = always visible)
     */
    void Render(Olympe::BlackboardSystem* blackboard, bool* pOpen = nullptr);

private:
    void RenderEntryList(Olympe::BlackboardSystem* blackboard);
    void RenderAddDialog(Olympe::BlackboardSystem* blackboard);
    void RenderEditPopup(Olympe::BlackboardSystem* blackboard);

    bool m_showAddDialog;
    bool m_showEditPopup;
    char m_newEntryName[256];
    int  m_newEntryTypeIndex;  // index into type combo
    std::string m_editTargetName;

    // Temporary edit buffer for add/edit
    Olympe::BlackboardValue m_editBuffer;
    char m_editStringBuf[1024];
    char m_renameBuffer[256];
};

} // namespace AI
} // namespace Olympe
