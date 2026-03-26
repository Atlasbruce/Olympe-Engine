/**
 * @file ConditionPresetEditDialog.h
 * @brief Modal dialog for creating and editing Condition Presets (Phase 24.1).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * ConditionPresetEditDialog presents a modal form where the user can
 * configure a condition expression by selecting:
 *   - Left operand  (Variable | Const | Pin)
 *   - Comparison operator (== != < <= > >=)
 *   - Right operand (Variable | Const | Pin)
 *
 * Modes:
 *   - Create: opens with a blank preset; confirmed result is a new preset.
 *   - Edit:   opens with an existing preset; confirmed result replaces it.
 *
 * Usage example:
 * @code
 *   // Open in Edit mode:
 *   ConditionPresetEditDialog dlg(ConditionPresetEditDialog::Mode::Edit, &myPreset);
 *   dlg.Open();
 *
 *   // In render loop:
 *   dlg.Render();
 *   if (dlg.IsConfirmed()) {
 *       ConditionPreset result = dlg.GetResult();
 *       registry.UpdatePreset(result.id, result);
 *   }
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../ConditionPreset/ConditionPreset.h"
#include "../../TaskSystem/LocalBlackboard.h"

namespace Olympe {

/**
 * @class ConditionPresetEditDialog
 * @brief Modal dialog for creating or editing a single ConditionPreset.
 */
class ConditionPresetEditDialog {
public:

    /**
     * @enum Mode
     * @brief Whether the dialog creates a new preset or edits an existing one.
     */
    enum class Mode {
        Create, ///< New preset (form starts empty)
        Edit    ///< Existing preset (form pre-populated)
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Constructs the dialog in Create mode with a blank preset.
     */
    ConditionPresetEditDialog();

    /**
     * @brief Constructs the dialog in the given mode.
     *
     * @param mode            Create or Edit.
     * @param existingPreset  If Mode::Edit, the preset to edit (copied).
     *                        If nullptr or Mode::Create, the form is blank.
     */
    explicit ConditionPresetEditDialog(Mode mode,
                                        const ConditionPreset* existingPreset = nullptr);

    ~ConditionPresetEditDialog() = default;

    // -----------------------------------------------------------------------
    // Visibility
    // -----------------------------------------------------------------------

    /** @brief Returns true if the dialog is open. */
    bool IsOpen() const { return m_isOpen; }

    /** @brief Opens the dialog. */
    void Open()  { m_isOpen = true; m_isConfirmed = false; }

    /** @brief Closes the dialog without confirming. */
    void Close() { m_isOpen = false; }

    // -----------------------------------------------------------------------
    // State queries
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the user pressed "Save" / "OK".
     *
     * Reset to false each time Open() is called.
     */
    bool IsConfirmed() const { return m_isConfirmed; }

    /**
     * @brief Returns the dialog mode (Create or Edit).
     */
    Mode GetMode() const { return m_mode; }

    /**
     * @brief Returns a copy of the working preset (contains the current form state).
     */
    ConditionPreset GetResult() const;

    // -----------------------------------------------------------------------
    // Operand accessors (also used by tests to inspect / drive state)
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the current left operand.
     */
    const Operand& GetLeftOperand() const { return m_workingCopy.left; }

    /**
     * @brief Returns the current right operand.
     */
    const Operand& GetRightOperand() const { return m_workingCopy.right; }

    /**
     * @brief Returns the current comparison operator.
     */
    ComparisonOp GetOperator() const { return m_workingCopy.op; }

    // -----------------------------------------------------------------------
    // Programmatic setters (for testing and host-driven pre-population)
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the left operand mode.
     * @param mode "Variable", "Const", or "Pin".
     */
    void SetLeftMode(const std::string& mode);

    /**
     * @brief Sets the left variable name (relevant when leftMode == "Variable").
     */
    void SetLeftVariable(const std::string& varName);

    /**
     * @brief Sets the left constant value.
     */
    void SetLeftConst(double value);

    /**
     * @brief Sets the left pin reference.
     */
    void SetLeftPin(const std::string& pinRef);

    /**
     * @brief Sets the comparison operator.
     * @param op One of "==", "!=", "<", "<=", ">", ">=".
     */
    void SetOperator(const std::string& op);

    /**
     * @brief Sets the right operand mode.
     * @param mode "Variable", "Const", or "Pin".
     */
    void SetRightMode(const std::string& mode);

    /**
     * @brief Sets the right variable name.
     */
    void SetRightVariable(const std::string& varName);

    /**
     * @brief Sets the right constant value.
     */
    void SetRightConst(double value);

    /**
     * @brief Sets the right pin reference (relevant when rightMode == "Pin").
     */
    void SetRightPin(const std::string& pinRef);

    /**
     * @brief Sets the display name of the working preset.
     */
    void SetName(const std::string& name);

    // -----------------------------------------------------------------------
    // Preview
    // -----------------------------------------------------------------------

    /**
     * @brief Returns a live preview string of the current condition.
     *
     * Example: "[mHealth] <= [2]"
     */
    std::string GetPreview() const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the current condition is valid and can be saved.
     *
     * A condition is valid when:
     *   - left  mode is non-empty and the corresponding operand is filled
     *   - operator is a valid comparison string
     *   - right mode is non-empty and the corresponding operand is filled
     */
    bool IsValid() const;

    // -----------------------------------------------------------------------
    // Programmatic confirmation (for testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Confirms the dialog programmatically (equivalent to pressing Save).
     *
     * Only succeeds if IsValid() returns true.
     * @return true if confirmation succeeded, false if the condition is invalid.
     */
    bool Confirm();

    // -----------------------------------------------------------------------
    // Rendering (ImGui — not called in tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the modal dialog using ImGui.
     *
     * Must be called once per frame while the dialog is open.
     * No-op when !IsOpen().
     */
    void Render();

    // -----------------------------------------------------------------------
    // Phase 24 Global Blackboard Integration
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the list of local variables from EntityBlackboard.
     * @param localVars Vector of local variable definitions.
     */
    void SetLocalVariables(const std::vector<BlackboardEntry>& localVars)
    {
        m_localVariables = localVars;
    }

private:

    // -----------------------------------------------------------------------
    // ImGui rendering helpers
    // -----------------------------------------------------------------------

    void RenderOperandSelector(const char* label, bool isLeft);
    void RenderOperatorSelector();
    void RenderPreview();
    void RenderConfirmButtons();

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** @brief Returns true if the given operator string is recognised. */
    static bool IsValidOperator(const std::string& op);

    /** @brief Returns true if the given mode string is recognised. */
    static bool IsValidMode(const std::string& mode);

    /** @brief Returns true if the operand is sufficiently filled. */
    static bool IsOperandFilled(const Operand& operand);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    Mode            m_mode;
    ConditionPreset m_workingCopy; ///< In-progress edits
    bool            m_isOpen      = false;
    bool            m_isConfirmed = false;
    std::vector<BlackboardEntry> m_localVariables; ///< Local variables from entity (Phase 24)
};

} // namespace Olympe
