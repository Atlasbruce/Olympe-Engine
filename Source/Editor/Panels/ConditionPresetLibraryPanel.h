/**
 * @file ConditionPresetLibraryPanel.h
 * @brief UI panel for managing Condition Presets globally (Phase 24.1).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * ConditionPresetLibraryPanel is the centralized editor panel where users
 * create, edit, duplicate, delete, and search condition presets at the
 * project level.
 *
 * The panel owns the ImGui window "Condition Preset Library" and delegates
 * all data operations to the ConditionPresetRegistry it holds by reference.
 *
 * Layout:
 * @code
 * ╔══════════════════════════════════════════╗
 * ║  Condition Preset Library                ║
 * ├──────────────────────────────────────────┤
 * ║ [+] Add Condition Preset      [Search]   ║
 * ├──────────────────────────────────────────┤
 * ║ ▼ Condition #1          [Dup][Del]       ║
 * ║   [mHealth] <= [2]                       ║
 * ║   Used by: Node_A, Node_C               ║
 * ╚══════════════════════════════════════════╝
 * @endcode
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>

#include "../../Editor/ConditionPreset/ConditionPreset.h"
#include "../../Editor/ConditionPreset/ConditionPresetRegistry.h"

namespace Olympe {

/**
 * @class ConditionPresetLibraryPanel
 * @brief ImGui panel for creating, editing, duplicating, and deleting
 *        global condition presets.
 *
 * @details
 * The panel uses ConditionPresetEditDialog internally for the create/edit
 * workflow.  Deletion requires confirmation via a modal dialog.
 *
 * Optional callbacks allow the host (e.g. the Blueprint Editor) to react to
 * preset lifecycle events:
 *   - OnPresetCreated(id)   — a new preset was added to the registry
 *   - OnPresetModified(id)  — an existing preset was updated
 *   - OnPresetDeleted(id)   — a preset was removed from the registry
 */
class ConditionPresetLibraryPanel {
public:

    /**
     * @brief Constructs the panel with a reference to the preset registry.
     * @param registry  The global ConditionPresetRegistry to operate on.
     */
    explicit ConditionPresetLibraryPanel(ConditionPresetRegistry& registry);

    ~ConditionPresetLibraryPanel() = default;

    // Non-copyable
    ConditionPresetLibraryPanel(const ConditionPresetLibraryPanel&) = delete;
    ConditionPresetLibraryPanel& operator=(const ConditionPresetLibraryPanel&) = delete;

    // -----------------------------------------------------------------------
    // Visibility
    // -----------------------------------------------------------------------

    /** @brief Returns true if the panel window is open. */
    bool IsOpen() const { return m_isOpen; }

    /** @brief Opens the panel window. */
    void Open()  { m_isOpen = true;  }

    /** @brief Closes the panel window. */
    void Close() { m_isOpen = false; }

    // -----------------------------------------------------------------------
    // Rendering (ImGui — not called in tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the full panel window using ImGui.
     *
     * Must be called once per frame while the panel is open.
     * No-op when !IsOpen().
     */
    void Render();

    // -----------------------------------------------------------------------
    // Callbacks
    // -----------------------------------------------------------------------

    /** Invoked after a new preset is added.  Arg: preset ID. */
    std::function<void(const std::string&)> OnPresetCreated;

    /** Invoked after an existing preset is modified.  Arg: preset ID. */
    std::function<void(const std::string&)> OnPresetModified;

    /** Invoked after a preset is deleted.  Arg: preset ID. */
    std::function<void(const std::string&)> OnPresetDeleted;

    // -----------------------------------------------------------------------
    // Testable logic interface
    // -----------------------------------------------------------------------

    /**
     * @brief Sets the search filter string (case-insensitive substring).
     * @param filter  Filter text.  Empty string shows all presets.
     */
    void SetSearchFilter(const std::string& filter);

    /**
     * @brief Returns the current search filter string.
     */
    const std::string& GetSearchFilter() const { return m_searchFilter; }

    /**
     * @brief Selects the preset with the given ID.
     * @param id  Preset ID to select, or empty string to deselect.
     */
    void SetSelectedPresetID(const std::string& id);

    /**
     * @brief Returns the currently selected preset ID (empty if none).
     */
    const std::string& GetSelectedPresetID() const { return m_selectedPresetID; }

    /**
     * @brief Returns true if the delete confirmation dialog is currently shown.
     */
    bool IsDeleteConfirmationVisible() const { return m_showDeleteConfirmation; }

    /**
     * @brief Returns the ID of the preset pending deletion confirmation.
     */
    const std::string& GetPresetToDelete() const { return m_presetToDelete; }

    /**
     * @brief Returns a reference to the registry managed by this panel.
     */
    ConditionPresetRegistry& GetRegistry() { return m_registry; }

    /**
     * @brief Returns filtered presets based on the current search filter.
     *
     * Delegates to ConditionPresetRegistry::GetFilteredPresets().
     */
    std::vector<ConditionPreset> GetFilteredPresets() const;

    /**
     * @brief Sets the reference analysis map (nodeID → list of preset IDs used).
     *
     * This data is supplied externally by the host (e.g. Blueprint Editor)
     * and used to display "Used by: Node_A, Node_C" in the panel.
     *
     * @param refMap  Map from nodeID to vector of preset IDs that node uses.
     */
    void SetReferenceMap(const std::map<std::string, std::vector<std::string>>& refMap);

    /**
     * @brief Returns a list of node IDs that reference the given preset.
     * @param presetID  Preset ID to query.
     */
    std::vector<std::string> GetReferencingNodes(const std::string& presetID) const;

    // -----------------------------------------------------------------------
    // Action handlers (called from Render; also testable)
    // -----------------------------------------------------------------------

    /**
     * @brief Handles the "Add Preset" button.
     *
     * Creates a new empty ConditionPreset in the registry.
     * Fires OnPresetCreated with the new ID.
     *
     * @return The new preset's ID.
     */
    std::string OnAddPresetClicked();

    /**
     * @brief Handles the "Duplicate" button for a preset.
     *
     * Duplicates the preset in the registry.
     * Fires OnPresetCreated with the new ID.
     *
     * @param presetID  ID of the preset to duplicate.
     * @return The new preset's ID, or empty string if presetID not found.
     */
    std::string OnDuplicatePresetClicked(const std::string& presetID);

    /**
     * @brief Handles the "Delete" button for a preset.
     *
     * Sets m_showDeleteConfirmation = true and records m_presetToDelete.
     * Does NOT delete immediately; call OnDeleteConfirmed() to confirm.
     *
     * @param presetID  ID of the preset the user wants to delete.
     */
    void OnDeletePresetClicked(const std::string& presetID);

    /**
     * @brief Confirms the pending deletion.
     *
     * Removes the preset from the registry and fires OnPresetDeleted.
     * Resets m_showDeleteConfirmation to false.
     *
     * @param presetID  ID of the preset to delete (must match m_presetToDelete).
     */
    void OnDeleteConfirmed(const std::string& presetID);

    /**
     * @brief Cancels the pending deletion and hides the confirmation dialog.
     */
    void OnDeleteCancelled();

    /**
     * @brief Selects a preset (highlights it in the list).
     * @param presetID  ID of the preset to select.
     */
    void OnPresetSelected(const std::string& presetID);

private:

    // -----------------------------------------------------------------------
    // ImGui rendering helpers (not tested directly)
    // -----------------------------------------------------------------------

    void RenderToolbar();
    void RenderPresetList();
    void RenderPresetItem(const std::string& presetID, const ConditionPreset& preset);
    void RenderDeleteConfirmationDialog();
    void RenderReferenceAnalysis(const std::string& presetID);

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    ConditionPresetRegistry& m_registry;               ///< Global preset registry
    bool                     m_isOpen               = false;
    std::string              m_selectedPresetID;       ///< Highlighted preset ID
    std::string              m_searchFilter;           ///< Current search text
    bool                     m_showDeleteConfirmation = false;
    std::string              m_presetToDelete;         ///< ID queued for deletion

    /// External reference map: nodeID → list of preset IDs that node references
    std::map<std::string, std::vector<std::string>> m_refMap;
};

} // namespace Olympe
