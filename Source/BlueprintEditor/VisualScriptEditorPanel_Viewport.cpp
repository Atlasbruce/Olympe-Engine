/**
 * @file VisualScriptEditorPanel_Viewport.cpp
 * @brief TIER 0 - Viewport panning and coordinate conversion helpers
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "VisualScriptEditorPanel.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../system/system_utils.h"

namespace Olympe {

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================

void VisualScriptEditorPanel::ResetViewportBeforeSave()
{
    SYSTEM_LOG << "[VSEditor] ResetViewportBeforeSave: saving current panning\n";
    m_lastViewportPanning = Vector::FromImVec2(ImNodes::EditorContextGetPanning());
    m_viewportResetDone   = true;

    // Reset panning to (0, 0) so that any residual editor-space offset from
    // user navigation is zeroed out before SyncNodePositionsFromImNodes reads
    // GetNodeGridSpacePos (which is already pan-independent, but this ensures
    // no subtle ImNodes internal state leaks into the saved positions).
    ImNodes::EditorContextResetPanning(ImVec2(0.0f, 0.0f));
    SYSTEM_LOG << "[VSEditor] ResetViewportBeforeSave: panning reset to (0,0) "
               << "(was " << m_lastViewportPanning.x << "," << m_lastViewportPanning.y << ")\n";
}

void VisualScriptEditorPanel::AfterSave()
{
    if (!m_viewportResetDone)
        return;

    // Restore the viewport so the canvas does not visually jump for the user.
    ImNodes::EditorContextResetPanning(m_lastViewportPanning.ToImVec2());
    m_viewportResetDone = false;
    SYSTEM_LOG << "[VSEditor] AfterSave: viewport panning restored to ("
               << m_lastViewportPanning.x << "," << m_lastViewportPanning.y << ")\n";
}

ImVec2 VisualScriptEditorPanel::ScreenToCanvasPos(ImVec2 screenPos) const
{
    // Convert absolute screen-space position to ImNodes editor (canvas) space.
    // Editor space = grid space + panning, so:
    //   editorX = screenX - canvasOrigin.x - windowPos.x
    // ImNodes 0.4 has no zoom API; zoom is implicitly 1.0f.
    ImVec2 canvasPanning = ImNodes::EditorContextGetPanning();
    ImVec2 windowPos     = ImGui::GetWindowPos();
    return ImVec2(
        screenPos.x - windowPos.x - canvasPanning.x,
        screenPos.y - windowPos.y - canvasPanning.y);
}

} // namespace Olympe
