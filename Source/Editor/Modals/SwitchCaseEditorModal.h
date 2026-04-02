/**
 * @file SwitchCaseEditorModal.h
 * @brief Dedicated modal dialog for editing a Switch node's case definitions (Phase 26).
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * @details
 * SwitchCaseEditorModal provides a modal overlay to add, remove, reorder, and
 * configure the case definitions on a Switch node. Each case maps a value to a pin.
 * It is separate from the properties panel so that edits are confirmed in one atomic
 * Apply step.
 *
 * Usage:
 * @code
 *   SwitchCaseEditorModal modal;
 *
 *   // When user clicks "Edit Switch Cases" in the Properties panel:
 *   modal.Open(node.switchCases);
 *
 *   // In render loop:
 *   modal.Render();
 *   if (modal.IsConfirmed()) {
 *       node.switchCases = modal.GetSwitchCases();
 *       modal.Close();
 *   }
 * @endcode
 *
 * The modal works on a COPY of the node's switch cases. On "Apply" it marks
 * itself as confirmed; on "Cancel" the copy is discarded.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../../TaskSystem/TaskGraphTypes.h"

namespace Olympe {

/**
 * @class SwitchCaseEditorModal
 * @brief Modal dialog for adding, removing, and reordering switch cases on a node.
 *
 * @details
 * The modal contains:
 *   - A scrollable list of current cases, each with:
 *       - Value input field (match value as string)
 *       - Custom label input field (optional display name)
 *       - Delete [X] button
 *       - Move Up / Move Down buttons
 *   - An "Add Case" button
 *   - Apply and Cancel buttons at the bottom
 */
class SwitchCaseEditorModal {
public:

    /**
     * @brief Constructs the modal.
     */
    SwitchCaseEditorModal();

    ~SwitchCaseEditorModal() = default;

    // Non-copyable
    SwitchCaseEditorModal(const SwitchCaseEditorModal&) = delete;
    SwitchCaseEditorModal& operator=(const SwitchCaseEditorModal&) = delete;

    // -----------------------------------------------------------------------
    // Visibility / lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the modal is currently open.
     */
    bool IsOpen() const { return m_isOpen; }

    /**
     * @brief Opens the modal, loading the given switch cases for editing.
     *
     * Resets the confirmed flag and takes a COPY of the cases to edit.
     *
     * @param currentCases      Switch cases from the selected node.
     * @param switchVarName     Name of the variable being switched (e.g. "mHealth")
     * @param switchVarType     Type of the variable (e.g. "Int")
     * @param currentVarValue   Current value of the variable for display (optional)
     */
    void Open(const std::vector<SwitchCaseDefinition>& currentCases,
              const std::string& switchVarName = "",
              const std::string& switchVarType = "",
              const std::string& currentVarValue = "");

    /**
     * @brief Closes the modal without confirming changes.
     */
    void Close();

    /**
     * @brief Returns true if the modal was closed via "Apply" rather than "Cancel".
     * After reading, call Close() to reset the flag for the next edit.
     */
    bool IsConfirmed() const { return m_confirmed; }

    /**
     * @brief Returns the edited switch cases (only valid after IsConfirmed() == true).
     */
    const std::vector<SwitchCaseDefinition>& GetSwitchCases() const { return m_editingCases; }

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the modal dialog. Must be called once per frame while IsOpen().
     * Handles ImGui::OpenPopupOnItemClick, ImGui::BeginPopupModal, etc.
     */
    void Render();

private:

    // -----------------------------------------------------------------------
    // Render helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the scrollable list of case rows.
     */
    void RenderCaseList();

    /**
     * @brief Renders a single case row with all controls.
     * @param caseIndex  0-based index in m_editingCases
     * @return           True if any field was modified
     */
    bool RenderCaseRow(size_t caseIndex);

    /**
     * @brief Renders the "Add Case" and action buttons (Apply / Cancel).
     */
    void RenderActionButtons();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    /// True if the modal is currently open
    bool m_isOpen = false;

    /// True if the user clicked "Apply" (set to false on Close)
    bool m_confirmed = false;

    /// Working copy of switch cases being edited
    std::vector<SwitchCaseDefinition> m_editingCases;

    /// Temporary edit buffers for value and label fields
    std::vector<std::string> m_caseValueBuffers;  ///< One per case, synced before render
    std::vector<std::string> m_caseLabelBuffers;  ///< One per case, synced before render

    // Phase 26-A: Context information for better UX
    std::string m_switchVarName;     ///< Variable being switched (e.g. "mHealth")
    std::string m_switchVarType;     ///< Variable type (e.g. "Int", "String")
    std::string m_currentVarValue;   ///< Current value for context display
    bool m_hasValidationError = false; ///< Set to true if any case has errors (prevents Apply)
};

} // namespace Olympe
