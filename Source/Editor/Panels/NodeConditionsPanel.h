/**
 * @file NodeConditionsPanel.h
 * @brief UI Properties panel for a NodeBranch – 4-section layout (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * NodeConditionsPanel renders the Properties panel for a selected NodeBranch.
 * It is divided into four read-only / static sections, with actual condition
 * editing delegated to NodeConditionsEditModal:
 *
 * @code
 * ╔══════════════════════════════════════════════════════════╗
 * ║  Section 1 — Title bar (blue background)                 ║
 * ║    Is Health Critical?                     Name          ║
 * ╠══════════════════════════════════════════════════════════╣
 * ║  Section 2 — Exec pins (static, never editable)          ║
 * ║    >> In                        Then >>                  ║
 * ║                                 Else >>                  ║
 * ╠══════════════════════════════════════════════════════════╣
 * ║  Section 3 — Conditions preview (READ-ONLY, green text)  ║
 * ║      [mHealth] <= [2]                                    ║
 * ║  And [mSpeed] <= [100.00]                                ║
 * ║  Or  [mSpeed] == [Pin : 1]                               ║
 * ║  [ Edit Conditions ]                                     ║
 * ╠══════════════════════════════════════════════════════════╣
 * ║  Section 4 — Dynamic data pins (yellow, only when used)  ║
 * ║  * In #1: [mSpeed] == [Pin : 1]                         ║
 * ╚══════════════════════════════════════════════════════════╝
 * @endcode
 *
 * Editing is opened via the "Edit Conditions" button which sets
 * m_editModalOpen = true; the caller renders NodeConditionsEditModal separately.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../ConditionPreset/ConditionPreset.h"
#include "../ConditionPreset/ConditionPresetRegistry.h"
#include "../ConditionPreset/NodeConditionRef.h"
#include "../ConditionPreset/DynamicDataPin.h"
#include "../Modals/NodeConditionsEditModal.h"
#include "../../BlueprintEditor/ConditionRef.h"

namespace Olympe {

/**
 * @class NodeConditionsPanel
 * @brief ImGui sub-panel for managing the condition list of a single NodeBranch.
 *
 * @details
 * The panel does NOT own the registry — it holds a reference.
 * The caller is responsible for persisting changes from GetConditionRefs()
 * back to the node definition.
 *
 * Dependency injection:
 *   NodeConditionsPanel panel(registry);
 *   panel.SetConditionRefs(node.conditions);
 *   panel.SetDynamicPins(node.dynamicPins);   // optional read-only display
 *   panel.Render();
 *   if (panel.IsDirty()) {
 *       node.conditions  = panel.GetConditionRefs();
 *       panel.ClearDirty();
 *   }
 */
class NodeConditionsPanel {
public:

    /**
     * @brief Constructs the panel bound to a global preset registry.
     * @param registry  Global ConditionPresetRegistry (must outlive this panel).
     */
    explicit NodeConditionsPanel(ConditionPresetRegistry& registry);

    ~NodeConditionsPanel() = default;

    // Non-copyable
    NodeConditionsPanel(const NodeConditionsPanel&) = delete;
    NodeConditionsPanel& operator=(const NodeConditionsPanel&) = delete;

    // -----------------------------------------------------------------------
    // State accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Replaces the panel's internal condition list.
     *
     * Typically called when a new node is selected in the editor.
     *
     * @param refs  Condition references from node.conditions.
     */
    void SetConditionRefs(const std::vector<NodeConditionRef>& refs);

    /**
     * @brief Returns the current (possibly modified) condition list.
     */
    const std::vector<NodeConditionRef>& GetConditionRefs() const;

    /**
     * @brief Replaces the panel's inline operand list (parallel to conditionRefs).
     *
     * Each entry corresponds 1:1 with a NodeConditionRef and stores the
     * editable operand data (mode, variable name, const value, dynamicPinID).
     * Typically called alongside SetConditionRefs() when a node is selected.
     *
     * @param refs  Inline operand refs from node.conditionOperandRefs.
     */
    void SetConditionOperandRefs(const std::vector<ConditionRef>& refs);

    /**
     * @brief Returns the current inline operand list.
     *
     * The host should read this after IsDirty() returns true and persist it
     * to node.conditionOperandRefs.
     */
    const std::vector<ConditionRef>& GetConditionOperandRefs() const;

    /**
     * @brief Provides the read-only list of dynamic pins for display.
     *
     * The panel shows these pins in a read-only section.  Ownership stays
     * with the DynamicDataPinManager; the panel only renders them.
     *
     * @param pins  Current dynamic pins associated with this node.
     */
    void SetDynamicPins(const std::vector<DynamicDataPin>& pins);

    /**
     * @brief Returns true if the condition list has been modified since the
     *        last call to ClearDirty().
     */
    bool IsDirty() const { return m_dirty; }

    /**
     * @brief Resets the dirty flag.
     */
    void ClearDirty() { m_dirty = false; }

    // -----------------------------------------------------------------------
    // Node name (for title section)
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the node name displayed in the title section.
     * @param name  Display name (e.g. "Is Health Critical?").
     */
    void SetNodeName(const std::string& name) { m_nodeName = name; }

    /**
     * @brief Returns the current node name shown in the title bar.
     */
    const std::string& GetNodeName() const { return m_nodeName; }

    // -----------------------------------------------------------------------
    // Edit-modal interaction
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the "Edit Conditions" button was clicked this frame.
     *
     * The caller should open NodeConditionsEditModal when this is true, then
     * call ClearEditModalRequest() to reset the flag.
     */
    bool IsEditModalRequested() const { return m_editModalRequested; }

    /**
     * @brief Resets the edit-modal request flag.
     */
    void ClearEditModalRequest() { m_editModalRequested = false; }

    // -----------------------------------------------------------------------
    // Rendering (ImGui — guarded by OLYMPE_HEADLESS in tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the condition list and dynamic-pin display using ImGui.
     *
     * Must be called once per frame inside an ImGui window context.
     * Internally wrapped in `#ifndef OLYMPE_HEADLESS` so tests can call
     * the logic methods without linking ImGui.
     */
    void Render();

    /**
     * @brief Renders all conditions inline as an editable list.
     *
     * Each condition is displayed as a row containing:
     *   - Condition preview (green text, from ConditionPreset::GetPreview)
     *   - Logical-operator dropdown ("And" / "Or") — skipped for the first row
     *   - Delete button ("X") that removes the condition immediately
     *
     * Below the list a "[+ Add Condition]" button opens a popup that lets
     * the user pick a preset from the registry (filtered by name).
     *
     * Modifications set the dirty flag so the host can sync via GetConditionRefs().
     *
     * Layout:
     * @code
     * ┌─ "Structured Conditions (evaluated with implicit AND)" ──┐
     * │ [mHealth] <= [2]           [And▼] [X]                    │
     * │ [mSpeed]  <= [100.00]      [Or▼]  [X]                    │
     * │ [+ Add Condition]                                         │
     * └──────────────────────────────────────────────────────────┘
     * @endcode
     *
     * Guarded by `#ifndef OLYMPE_HEADLESS` — no-op in headless test builds.
     */
    void RenderConditionList();

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /** Fired when a preset is selected or changed.  Arg: preset ID. */
    std::function<void(const std::string&)> OnPresetChanged;

    /**
     * @brief Fired after the edit modal is confirmed and condition refs are
     *        updated, signalling that dynamic pins must be regenerated.
     *
     * The host should connect this to DynamicDataPinManager::RegeneratePinsFromConditions()
     * or NodeBranchRenderer::TriggerPinRegeneration() so that the canvas
     * updates immediately on the next render frame.
     *
     * Example:
     * @code
     *   panel.OnDynamicPinsNeedRegeneration = [&]() {
     *       pinManager.RegeneratePinsFromConditions(node.conditionRefs);
     *       node.dynamicPins = pinManager.GetAllPins();
     *   };
     * @endcode
     */
    std::function<void()> OnDynamicPinsNeedRegeneration;

    /**
     * @brief Should be called by the host when a preset is deleted from the
     *        registry.  Removes any NodeConditionRef pointing to that preset.
     *
     * @param deletedPresetID  UUID of the preset that was removed.
     */
    void OnPresetDeleted(const std::string& deletedPresetID);

    // -----------------------------------------------------------------------
    // Condition management (also testable without ImGui)
    // -----------------------------------------------------------------------

    /**
     * @brief Appends a new condition reference with the given preset.
     *
     * The logicalOp is automatically set to LogicalOp::And (except for
     * the first entry, which receives LogicalOp::Start).
     *
     * @param presetID  UUID of the ConditionPreset to reference.
     */
    void AddCondition(const std::string& presetID);

    /**
     * @brief Removes the condition at the given index.
     * @param index  Zero-based index in the condition list.
     */
    void RemoveCondition(size_t index);

    /**
     * @brief Changes the logical operator for the condition at the given index.
     *
     * Index 0 is always forced to LogicalOp::Start regardless of the value
     * passed in.
     *
     * @param index  Zero-based index.
     * @param op     New LogicalOp (And or Or; Start is ignored for index > 0).
     */
    void SetLogicalOp(size_t index, LogicalOp op);

    /**
     * @brief Returns the number of conditions currently in the list.
     */
    size_t GetConditionCount() const;

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true when every condition ref points to an existing preset.
     */
    bool IsValid() const;

    /**
     * @brief Returns a list of error strings describing validation failures.
     *
     * Empty if IsValid() returns true.
     */
    std::vector<std::string> Validate() const;

    // -----------------------------------------------------------------------
    // Search helper (used by the dropdown)
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the search/filter string used by the "Add Condition" dropdown.
     * @param filter  Substring filter (empty = show all).
     */
    void SetDropdownFilter(const std::string& filter);

    /**
     * @brief Returns the current dropdown filter.
     */
    const std::string& GetDropdownFilter() const { return m_dropdownFilter; }

    /**
     * @brief Returns presets matching the current dropdown filter.
     *
     * Delegates to ConditionPresetRegistry::FindPresetsByName when the filter
     * is non-empty; returns all presets otherwise.
     */
    std::vector<ConditionPreset> GetFilteredPresetsForDropdown() const;

private:

    // -----------------------------------------------------------------------
    // ImGui rendering helpers — 4-section layout
    // -----------------------------------------------------------------------

    /** Section 1: title bar (blue background, node name). */
    void RenderTitleSection();

    /** Section 2: static exec pins (In / Then / Else, never editable). */
    void RenderExecPinsSection();

    /** Section 3: read-only conditions preview (green text) + "Edit Conditions" button. */
    void RenderConditionsPreview();

    /** Section 4: dynamic data pins (yellow, only when non-empty). */
    void RenderDynamicPinsSection();

    /**
     * @brief Renders the operand mode selector + value field for one operand.
     *
     * Inline widget used inside RenderConditionList() for each condition row.
     * Renders a three-button radio group (Variable | Const | Pin) followed by
     * a mode-specific input:
     *   - Variable: text field for the blackboard key (e.g. "mHealth")
     *   - Const:    numeric text field (e.g. "100.0")
     *   - Pin:      read-only label "Pin-in #N" (auto-assigned)
     *
     * @param cref   ConditionRef to modify (mode and value fields).
     * @param isLeft True for the left operand, false for the right operand.
     * @param pinLabel Short "Pin-in #N" label for Pin-mode display (may be empty).
     */
    void RenderOperandDropdown(ConditionRef& cref, bool isLeft,
                               const std::string& pinLabel = "");

    /**
     * @brief Renders the operator combo box (==, !=, <, <=, >, >=).
     *
     * Inline widget used inside RenderConditionList() for each condition row.
     * Updates `cref.operatorStr` on selection.
     *
     * @param cref  ConditionRef to modify.
     */
    void RenderOperatorDropdown(ConditionRef& cref);

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /** Fixes up the first condition's logicalOp to always be Start. */
    void NormalizeLogicalOps();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    ConditionPresetRegistry&       m_registry;           ///< Shared global registry
    std::vector<NodeConditionRef>  m_conditionRefs;      ///< Current node's conditions
    std::vector<ConditionRef>      m_conditionOperandRefs; ///< Inline operand data (parallel to m_conditionRefs)
    std::vector<DynamicDataPin>    m_dynamicPins;        ///< Read-only dynamic pins for display
    std::string                    m_nodeName;           ///< Node display name for title section
    bool                           m_dirty = false;      ///< Modification flag
    bool                           m_editModalRequested = false; ///< Set when Edit button is clicked

    NodeConditionsEditModal  m_editModal;         ///< Owned modal for condition editing

    std::string  m_dropdownFilter;    ///< Filter text for "Add Condition" dropdown
    bool         m_dropdownOpen = false; ///< Whether the add-dropdown is shown (legacy, kept for API compat)
};

} // namespace Olympe
