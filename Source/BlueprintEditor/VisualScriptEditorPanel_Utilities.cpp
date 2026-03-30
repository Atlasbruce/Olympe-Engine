/**
 * @file VisualScriptEditorPanel_Utilities.cpp
 * @brief Utility helper methods for VisualScriptEditorPanel (Phase 5 extraction).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * This file contains 6 utility helper methods extracted from VisualScriptEditorPanel.cpp
 * for improved code organization and maintainability:
 *
 *   1. ValidateAndCleanBlackboardEntries() — Remove invalid blackboard entries (BUG-002 Fix #1)
 *   2. CommitPendingBlackboardEdits() — Flush deferred blackboard key-name edits
 *   3. ResetViewportBeforeSave() — Save and reset ImNodes viewport panning (BUG-003 Fix)
 *   4. AfterSave() — Restore ImNodes viewport panning after save (BUG-003 Fix #5)
 *   5. ScreenToCanvasPos() — Convert screen-space to ImNodes editor-space coordinates
 *   6. GetVariablesByType() — Type-filtered variable utility (UX Enhancement #3)
 *
 * Phase 24 Refactoring: Extract utility helpers to separate compilation unit for
 * cleaner separation of concerns and faster iteration during UI development.
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

#include <algorithm>
#include <iostream>

namespace Olympe {

// ============================================================================
// Blackboard validation helpers (BUG-002 Fix #1)
// ============================================================================

/**
 * @brief ValidateAndCleanBlackboardEntries
 *
 * Remove invalid blackboard entries from the template's blackboard list.
 * Entries with empty keys or VariableType::None are considered invalid and
 * are filtered out to prevent serialization crashes and UI inconsistencies.
 *
 * Invalid entries can occur when:
 *   - User creates a variable entry but doesn't specify a name
 *   - Type deserialization failed due to malformed JSON
 *   - Legacy data migration left orphaned entries
 *
 * This method is called before Save() and SaveAs() to ensure a clean state.
 * Removed entries are logged for debugging; m_dirty is set to track changes.
 */
void VisualScriptEditorPanel::ValidateAndCleanBlackboardEntries()
{
    std::vector<BlackboardEntry>& entries = m_template.Blackboard;
    size_t before = entries.size();

    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const BlackboardEntry& e) {
                if (e.Key.empty()) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry with empty key\n";
                    return true;
                }
                if (e.Type == VariableType::None) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry '"
                               << e.Key << "' with VariableType::None\n";
                    return true;
                }
                return false;
            }),
        entries.end());

    size_t removed = before - entries.size();
    if (removed > 0)
    {
        SYSTEM_LOG << "[VSEditor] ValidateAndClean: removed " << removed
                   << " invalid blackboard entries\n";
        m_dirty = true;
    }
}

/**
 * @brief CommitPendingBlackboardEdits
 *
 * Flush any deferred blackboard entry key-name edits to the template.
 * When the user is editing a variable key name in the UI, the change is
 * stored in m_pendingBlackboardEdits (a map of index → new key name) to
 * defer costly operations until the edit is complete or save is triggered.
 *
 * This method is called before serialization (Save/SaveAs) to ensure all
 * user edits are persisted. After flushing, m_pendingBlackboardEdits is cleared.
 *
 * Bounds checking is performed to prevent out-of-range access.
 */
void VisualScriptEditorPanel::CommitPendingBlackboardEdits()
{
    for (std::unordered_map<int, std::string>::iterator it = m_pendingBlackboardEdits.begin();
         it != m_pendingBlackboardEdits.end(); ++it)
    {
        int idx = it->first;
        if (idx >= 0 && idx < static_cast<int>(m_template.Blackboard.size()))
        {
            m_template.Blackboard[static_cast<size_t>(idx)].Key = it->second;
        }
    }
    m_pendingBlackboardEdits.clear();
}

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================

/**
 * @brief ResetViewportBeforeSave
 *
 * Save the current ImNodes viewport panning state and reset panning to (0, 0).
 * This is part of BUG-003 fix to prevent viewport jump when saving.
 *
 * Rationale:
 *   1. SyncNodePositionsFromImNodes() is called during Save() to capture
 *      the current canvas state before serialization.
 *   2. GetNodeGridSpacePos() is pan-independent, but stale ImNodes internal
 *      state could corrupt the capture if the viewport offset is non-zero.
 *   3. By resetting panning to (0, 0) before the capture, we ensure a
 *      clean state for position serialization.
 *   4. AfterSave() restores the original panning so the user's viewport
 *      position is preserved (canvas doesn't visually jump).
 *
 * This is called at the very beginning of Save() / SaveAs().
 */
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

/**
 * @brief AfterSave
 *
 * Restore the ImNodes viewport panning to its pre-save state.
 * This completes BUG-003 fix #5: viewport restoration.
 *
 * Called at the end of Save() / SaveAs() after serialization completes.
 * Without this restoration, the user would see the canvas visually jump
 * to (0, 0) panning every time they save.
 *
 * If ResetViewportBeforeSave() was not called (or Save cancelled), this
 * is a no-op (guarded by m_viewportResetDone flag).
 */
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

/**
 * @brief ScreenToCanvasPos
 *
 * Convert absolute screen-space coordinates to ImNodes editor (canvas) space.
 * Used for context menu positioning, mouse hit testing, and UI interactions.
 *
 * The transformation is:
 *   canvasX = screenX - windowPos.x - panning.x
 *   canvasY = screenY - windowPos.y - panning.y
 *
 * where:
 *   - screenPos: absolute screen coordinate (e.g., from ImGui::GetMousePos())
 *   - windowPos: top-left corner of the ImGui window containing the canvas
 *   - panning: current ImNodes viewport pan offset
 *   - zoom: implicit 1.0f (ImNodes 0.4 does not expose zoom API)
 *
 * @param screenPos Absolute screen-space coordinate
 * @return ImVec2 Editor-space coordinate for hit testing or node placement
 */
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

// ============================================================================
// UX Enhancement #3 — Type-filtered variable utility
// ============================================================================

/**
 * @brief GetVariablesByType
 *
 * Static helper method to filter blackboard entries by variable type.
 * Returns a new vector containing only entries whose Type matches expectedType.
 *
 * This is used in UI panels to populate variable dropdowns with only
 * compatible types. For example, when connecting a data pin to a variable,
 * only variables of the matching type are shown in the dropdown.
 *
 * Example usage:
 *   std::vector<BlackboardEntry> intVars = GetVariablesByType(
 *       m_template.Blackboard, VariableType::Int);
 *
 * @param allVars Input list of all blackboard entries
 * @param expectedType Desired variable type to filter by
 * @return std::vector<BlackboardEntry> Filtered list (copies, not references)
 */
/*static*/
std::vector<BlackboardEntry> VisualScriptEditorPanel::GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType)
{
    std::vector<BlackboardEntry> filtered;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (allVars[i].Type == expectedType)
            filtered.push_back(allVars[i]);
    }
    return filtered;
}

}  // namespace Olympe
