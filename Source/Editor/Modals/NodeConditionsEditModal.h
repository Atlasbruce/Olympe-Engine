/**
 * @file NodeConditionsEditModal.h
 * @brief Dedicated modal dialog for editing a NodeBranch's condition list (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * NodeConditionsEditModal provides a modal overlay to add, remove, reorder, and
 * configure the logical operators of a NodeBranch's ConditionPreset references.
 * It is separate from NodeConditionsPanel so that the Properties panel remains
 * READ-ONLY (view only), while all edits are confirmed in one atomic Apply step.
 *
 * Usage:
 * @code
 *   NodeConditionsEditModal modal(registry);
 *
 *   // When user clicks "Edit Conditions" in the Properties panel:
 *   modal.Open(node.conditionRefs);
 *
 *   // In render loop:
 *   modal.Render();
 *   if (modal.IsConfirmed()) {
 *       node.conditionRefs = modal.GetConditionRefs();
 *       modal.Close();
 *   }
 * @endcode
 *
 * The modal works on a COPY of the node's condition refs.  On "Apply" it marks
 * itself as confirmed; on "Cancel" the copy is discarded.
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

namespace Olympe {

/**
 * @class NodeConditionsEditModal
 * @brief Modal dialog for adding, removing, and reordering conditions on a node.
 *
 * @details
 * The modal contains:
 *   - A scrollable list of current conditions, each with:
 *       - Logical operator dropdown (And / Or; first entry is always fixed).
 *       - Preset selector dropdown (from global registry).
 *       - Delete [X] button.
 *       - Move Up / Move Down buttons.
 *   - An "Add Condition" button (opens preset picker inline).
 *   - Apply and Cancel buttons at the bottom.
 */
class NodeConditionsEditModal {
public:

    /**
     * @brief Constructs the modal bound to a global preset registry.
     * @param registry  Global ConditionPresetRegistry (must outlive this modal).
     */
    explicit NodeConditionsEditModal(ConditionPresetRegistry& registry);

    ~NodeConditionsEditModal() = default;

    // Non-copyable
    NodeConditionsEditModal(const NodeConditionsEditModal&) = delete;
    NodeConditionsEditModal& operator=(const NodeConditionsEditModal&) = delete;

    // -----------------------------------------------------------------------
    // Visibility / lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true if the modal is currently open.
     */
    bool IsOpen() const { return m_isOpen; }

    /**
     * @brief Opens the modal, loading the given condition refs for editing.
     *
     * Resets the confirmed flag and takes a COPY of the refs to edit.
     *
     * @param currentRefs  Condition refs from the selected node.
     */
    void Open(const std::vector<NodeConditionRef>& currentRefs);

    /**
     * @brief Closes the modal without confirming changes.
     */
    void Close();

    /**
     * @brief Returns true if the user clicked "Apply" and the modal was closed.
     *
     * Reset by Open().
     */
    bool IsConfirmed() const { return m_isConfirmed; }

    // -----------------------------------------------------------------------
    // Result
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the edited condition ref list.
     *
     * Only meaningful when IsConfirmed() is true.
     */
    const std::vector<NodeConditionRef>& GetConditionRefs() const;

    // -----------------------------------------------------------------------
    // Condition management (testable without ImGui)
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the number of conditions in the working copy.
     */
    size_t GetConditionCount() const;

    /**
     * @brief Appends a new condition for the given preset to the working copy.
     *
     * The logicalOp defaults to And (Start for the first condition).
     * @param presetID  UUID of the preset to add.
     */
    void AddCondition(const std::string& presetID);

    /**
     * @brief Removes the condition at the given index from the working copy.
     * @param index  Zero-based index.
     */
    void RemoveCondition(size_t index);

    /**
     * @brief Sets the logical operator for the condition at the given index.
     *
     * Index 0 is always forced to LogicalOp::Start.
     * @param index  Zero-based index.
     * @param op     Desired operator (And or Or).
     */
    void SetLogicalOp(size_t index, LogicalOp op);

    /**
     * @brief Moves the condition at `index` one position earlier (swap with index-1).
     *
     * No-op when index == 0.
     * @param index  Zero-based index.
     */
    void MoveConditionUp(size_t index);

    /**
     * @brief Moves the condition at `index` one position later (swap with index+1).
     *
     * No-op when index is the last element.
     * @param index  Zero-based index.
     */
    void MoveConditionDown(size_t index);

    /**
     * @brief Programmatically confirms the modal (equivalent to clicking Apply).
     *
     * Marks the modal as confirmed and closes it.  Always succeeds.
     */
    void Confirm();

    // -----------------------------------------------------------------------
    // Dropdown helper
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the search filter for the "Add Condition" preset picker.
     * @param filter  Substring filter (empty = show all).
     */
    void SetDropdownFilter(const std::string& filter);

    /**
     * @brief Returns the current dropdown filter string.
     */
    const std::string& GetDropdownFilter() const { return m_dropdownFilter; }

    /**
     * @brief Returns presets matching the current dropdown filter.
     */
    std::vector<ConditionPreset> GetFilteredPresetsForDropdown() const;

    // -----------------------------------------------------------------------
    // Rendering (ImGui — guarded by OLYMPE_HEADLESS in tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the modal dialog using ImGui.
     *
     * Must be called once per frame from the host's render loop.
     * No-op when !IsOpen().
     */
    void Render();

private:

    // -----------------------------------------------------------------------
    // ImGui rendering helpers
    // -----------------------------------------------------------------------

    void RenderConditionRow(size_t index, const NodeConditionRef& ref);
    void RenderAddConditionPicker();
    void RenderFooterButtons();

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /** Ensures the first condition's logicalOp is always Start. */
    void NormalizeLogicalOps();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    ConditionPresetRegistry&       m_registry;         ///< Shared global registry
    std::vector<NodeConditionRef>  m_workingCopy;       ///< Editable in-progress copy
    bool                           m_isOpen      = false;
    bool                           m_isConfirmed = false;

    std::string  m_dropdownFilter;         ///< Preset-picker search filter
    bool         m_pickerOpen = false;     ///< Whether the add-preset picker is open
};

} // namespace Olympe
