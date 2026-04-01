/**
 * @file SubGraphFilePickerModal.h
 * @brief ImGui modal for selecting SubGraph files (Phase 26).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * Provides a file browser dialog for users to select .json blueprint files
 * to use as SubGraph nodes. Follows the same pattern as SwitchCaseEditorModal:
 * copy-on-open, confirm to apply, cancel to discard.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

/**
 * @class SubGraphFilePickerModal
 * @brief ImGui modal dialog for selecting SubGraph blueprint files.
 *
 * Usage:
 *   1. Call Open(currentPath) to initialize and show modal
 *   2. Call Render() each frame (returns true if still open)
 *   3. Check IsConfirmed() after close to see if user applied changes
 *   4. Call GetSelectedFile() to get the chosen file path
 *   5. Call Close() to dismiss modal
 */
class SubGraphFilePickerModal {
public:
    SubGraphFilePickerModal();
    ~SubGraphFilePickerModal() = default;

    // ====================================================================
    // Modal Lifecycle
    // ====================================================================

    /**
     * @brief Opens the modal with optional initial path.
     * @param currentPath If non-empty, shows this path as selected initially
     */
    void Open(const std::string& currentPath = "");

    /**
     * @brief Closes the modal without confirming changes.
     */
    void Close();

    /**
     * @brief Renders the modal UI. Call every frame while modal is open.
     */
    void Render();

    // ====================================================================
    // Query Results
    // ====================================================================

    /**
     * @brief Returns true if modal is currently visible.
     */
    bool IsOpen() const { return m_isOpen; }

    /**
     * @brief Returns true if user clicked "Select" button.
     * Call this after modal closes to check if changes should be applied.
     */
    bool IsConfirmed() const { return m_confirmed; }

    /**
     * @brief Returns the selected file path (only valid if IsConfirmed() is true).
     */
    const std::string& GetSelectedFile() const { return m_selectedFile; }

private:
    // ====================================================================
    // Modal State
    // ====================================================================

    bool        m_isOpen         = false;   ///< Is modal currently displayed
    bool        m_confirmed      = false;   ///< Did user click Select button
    std::string m_selectedFile   = "";      ///< The file path user selected
    std::string m_currentPath    = "";      ///< Current browse directory
    
    // ====================================================================
    // File Listing State
    // ====================================================================

    std::vector<std::string> m_blueprintFiles;  ///< .json files in current directory
    int                      m_selectedIndex = -1;  ///< Currently highlighted file (-1 = none)

    // ====================================================================
    // UI State
    // ====================================================================

    char m_pathBuffer[512] = "";           ///< Directory path input buffer
    char m_searchBuffer[128] = "";         ///< Search filter text

    // ====================================================================
    // Helper Methods
    // ====================================================================

    /**
     * @brief Scans current directory for .json blueprint files.
     * Populates m_blueprintFiles with file names.
     */
    void RefreshFileList();

    /**
     * @brief Renders the file list with scrolling support.
     */
    void RenderFileList();

    /**
     * @brief Renders action buttons (Select, Cancel).
     */
    void RenderActionButtons();

    /**
     * @brief Filters file list by search term.
     * @return Filtered file list
     */
    std::vector<std::string> GetFilteredFiles() const;
};

}  // namespace Olympe
