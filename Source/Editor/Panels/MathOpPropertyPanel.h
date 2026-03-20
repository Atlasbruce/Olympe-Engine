/**
 * @file MathOpPropertyPanel.h
 * @brief UI Properties panel for a MathOp node – operand editor.
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * @details
 * MathOpPropertyPanel renders the Properties panel for a selected MathOp node.
 * It provides inline editors for:
 *   - Left operand  (Variable/Const/Pin mode selector + value)
 *   - Operator      (dropdown: "+", "-", "*", "/", "%", "^")
 *   - Right operand (Variable/Const/Pin mode selector + value)
 *
 * Changes are immediately applied and trigger callbacks to regenerate
 * dynamic pins and update the canvas.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../ConditionPreset/ConditionPresetRegistry.h"
#include "../ConditionPreset/DynamicDataPinManager.h"
#include "../ConditionPreset/DynamicDataPin.h"
#include "../../BlueprintEditor/MathOpOperand.h"

namespace Olympe {

/**
 * @class MathOpPropertyPanel
 * @brief ImGui sub-panel for editing MathOp operands and operator.
 *
 * @details
 * The panel does NOT own the data — it holds references.
 * The caller is responsible for persisting changes from GetMathOpRef()
 * back to the node definition.
 *
 * Dependency injection:
 *   MathOpPropertyPanel panel(registry, dynamicPinManager);
 *   panel.SetMathOpRef(node.mathOpRef);
 *   panel.SetDynamicPins(node.dynamicPins);
 *   panel.SetNodeName(node.NodeName);
 *   panel.Render();
 *   if (panel.IsDirty()) {
 *       node.mathOpRef = panel.GetMathOpRef();
 *       panel.ClearDirty();
 *   }
 */
class MathOpPropertyPanel {
public:

    /**
     * @brief Constructs the panel bound to a preset registry.
     * @param registry  Global ConditionPresetRegistry (must outlive this panel).
     * @param dynamicPinMgr  DynamicDataPinManager for Pin-mode generation.
     */
    explicit MathOpPropertyPanel(
        ConditionPresetRegistry& registry,
        DynamicDataPinManager& dynamicPinMgr);

    ~MathOpPropertyPanel() = default;

    // Non-copyable
    MathOpPropertyPanel(const MathOpPropertyPanel&) = delete;
    MathOpPropertyPanel& operator=(const MathOpPropertyPanel&) = delete;

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the MathOpRef to edit.
     * @param ref  MathOpRef from node.mathOpRef.
     */
    void SetMathOpRef(const MathOpRef& ref);

    /**
     * @brief Returns the current (possibly modified) MathOpRef.
     */
    const MathOpRef& GetMathOpRef() const;

    /**
     * @brief Provides the read-only list of dynamic pins for display.
     * @param pins  Current dynamic pins associated with this MathOp node.
     */
    void SetDynamicPins(const std::vector<DynamicDataPin>& pins);

    /**
     * @brief Sets the node name displayed in the title section.
     * @param name  Human-readable node name (e.g., "MathOp_1").
     */
    void SetNodeName(const std::string& name);

    /**
     * @brief Returns true if the operand or operator has been modified
     *        since the last call to ClearDirty().
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * @brief Resets the dirty flag.
     */
    void ClearDirty() { m_dirty = false; }

    /**
     * @brief Registers a callback to be invoked when Pin-mode operands change.
     *
     * The callback should regenerate dynamic pins via DynamicDataPinManager
     * and trigger a canvas refresh.
     *
     * @param cb  Callback function.
     */
    void SetOnOperandChange(std::function<void()> cb)
    {
        m_onOperandChange = cb;
    }

    // -----------------------------------------------------------------------
    // Rendering
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the panel into the current ImGui context.
     *
     * Must be called between ImGui::Begin() / ImGui::End() or within
     * a child window. Handles layout, operand editors, and state synchronization.
     */
    void Render();

private:

    // -----------------------------------------------------------------------
    // Private rendering methods
    // -----------------------------------------------------------------------

    /// Renders the title section (node name in blue background).
    void RenderTitleSection();

    /// Renders the operand editor (collapsible section with 3 rows).
    void RenderInlineOperandEditor();

    /// Renders one operand row with mode selector and value input.
    /// @param rowIndex 0 = left operand, 1 = right operand
    /// @param operand  MathOpOperand to edit
    void RenderOperandRow(int rowIndex, MathOpOperand& operand);

    /// Renders the operator dropdown ("+", "-", "*", "/", "%", "^").
    void RenderOperatorSelector();

    /// Renders the dynamic pins section (read-only display).
    void RenderDynamicPinsSection();

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /// Helper to render Variable mode input field.
    void RenderVariableModeInput(MathOpOperand& operand);

    /// Helper to render Const mode input field.
    void RenderConstModeInput(MathOpOperand& operand);

    /// Helper to render Pin mode selector (list of available pins).
    void RenderPinModeSelector(MathOpOperand& operand);

    // -----------------------------------------------------------------------
    // Data members
    // -----------------------------------------------------------------------

    ConditionPresetRegistry& m_registry;         ///< Global preset registry
    DynamicDataPinManager& m_dynamicPinMgr;     ///< Pin generation manager

    std::string m_nodeName;                      ///< Node name for display
    MathOpRef m_mathOpRef;                       ///< Current operand configuration
    std::vector<DynamicDataPin> m_dynamicPins;   ///< Read-only dynamic pins display

    bool m_operandEditorOpen = true;             ///< Collapsible section state
    bool m_dirty = false;                        ///< Changed since last ClearDirty()

    std::function<void()> m_onOperandChange;     ///< Callback when operands change
};

} // namespace Olympe
