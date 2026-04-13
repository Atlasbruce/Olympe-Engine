/**
 * @file FilePickerModal.h
 * @brief Centralized file picker modal for all file selection operations (Phase 40).
 * @author Olympe Engine
 * @date 2026-03-20
 *
 * Provides a unified, reusable ImGui modal dialog for file selection across all editors.
 * Supports multiple file types (BehaviorTree .bt.json, SubGraph .ats, etc.) via FilePickerType enum.
 * 
 * This modal consolidates BehaviorTreeFilePickerModal and SubGraphFilePickerModal into a single
 * implementation to avoid code duplication and provide consistent UX.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

/**
 * @enum FilePickerType
 * @brief Supported file types for the centralized file picker modal.
 */
enum class FilePickerType
{
    BehaviorTree,  ///< .bt.json files in ./Gamedata
    SubGraph,      ///< .ats files in Blueprints
    Audio,         ///< Future: .ogg, .wav files
    Tileset        ///< Future: .tsj tileset files
};

/**
 * @class FilePickerModal
 * @brief Centralized ImGui modal dialog for file selection across all editors.
 *
 * Usage:
 *   1. FilePickerModal modal(FilePickerType::BehaviorTree);
 *   2. modal.Open(currentPath);
 *   3. During ImGui loop: modal.Render();
 *   4. After frame: if (modal.IsConfirmed()) { path = modal.GetSelectedFile(); }
 *
 * Features:
 *   - Multi-file-type support via FilePickerType enum
 *   - Path navigation and editing
 *   - File list with scrolling
 *   - Search filter with case-insensitive matching
 *   - Selection confirmation (Select/Cancel buttons)
 *   - Centered modal with size constraints
 *   - Platform-specific file listing (Windows + Unix)
 */
class FilePickerModal {
public:
    /**
     * @brief Constructs a file picker modal for the given file type.
     * @param fileType Type of files to browse (BehaviorTree, SubGraph, etc.)
     */
    explicit FilePickerModal(FilePickerType fileType);
    ~FilePickerModal() = default;

    // ====================================================================
    // Modal Lifecycle
    // ====================================================================

    /**
     * @brief Opens the modal with optional initial path.
     * @param currentPath If non-empty, shows this path as starting location
     * 
     * Initializes all modal state and refreshes the file list.
     */
    void Open(const std::string& currentPath = "");

    /**
     * @brief Closes the modal without confirming changes.
     * 
     * User cancelled the dialog. IsConfirmed() will return false.
     */
    void Close();

    /**
     * @brief Renders the modal UI. Must be called every frame while open.
     * 
     * Call this within the main ImGui rendering loop.
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
     * 
     * Call this after modal closes to check if user made a selection.
     * Only use GetSelectedFile() if this returns true.
     */
    bool IsConfirmed() const { return m_confirmed; }

    /**
     * @brief Returns the selected file path (only valid if IsConfirmed() is true).
     * 
     * Path format depends on file type:
     *   - BehaviorTree: "./Gamedata/MyTree.bt.json"
     *   - SubGraph: "Blueprints/MyBlueprint.ats"
     */
    const std::string& GetSelectedFile() const { return m_selectedFile; }

    /**
     * @brief Returns the file type this modal handles.
     */
    FilePickerType GetFileType() const { return m_fileType; }

private:
    // ====================================================================
    // Configuration & State
    // ====================================================================

    FilePickerType m_fileType;              ///< Type of files to browse
    bool m_isOpen = false;                  ///< Is modal currently visible
    bool m_confirmed = false;               ///< Did user click Select
    std::string m_selectedFile = "";        ///< Full path to selected file
    std::string m_currentPath = "";         ///< Current directory being browsed

    // ====================================================================
    // File Listing State
    // ====================================================================

    std::vector<std::string> m_fileList;    ///< Files found in current directory
    std::vector<std::string> m_folderList;  ///< Folders in current directory
    int m_selectedIndex = -1;               ///< Currently highlighted file (-1 = none)

    // ====================================================================
    // UI State
    // ====================================================================

    char m_pathBuffer[512] = "";            ///< Path input text buffer
    char m_searchBuffer[128] = "";          ///< Search filter text buffer
    int m_selectedFilterIndex = 0;          ///< Current filter type (0=default, 1=.bt.json, etc.)
    std::string m_currentFilter = "";       ///< Current file extension filter

    // ====================================================================
    // Helper Methods
    // ====================================================================

    /**
     * @brief Returns the default directory for this file type.
     */
    std::string GetDefaultDirectory() const;

    /**
     * @brief Returns the file pattern for this file type (e.g., "*.bt.json").
     */
    std::string GetFilePattern() const;

    /**
     * @brief Returns the modal title for this file type (e.g., "Select BehaviorTree File").
     */
    std::string GetModalTitle() const;

    /**
     * @brief Returns the description text for this file type.
     */
    std::string GetDescriptionText() const;

    /**
     * @brief Refreshes file list from current directory.
     * 
     * Searches for files matching the pattern for this file type.
     * Logs results to SYSTEM_LOG.
     */
    void RefreshFileList();

    /**
     * @brief Renders the file list UI component with scrolling and selection.
     */
    void RenderFileList();

    /**
     * @brief Renders the action buttons (Select, Cancel).
     * 
     * Select button is disabled if no file is selected.
     */
    void RenderActionButtons();

    /**
     * @brief Filters and returns files matching the search buffer.
     * @return Vector of matching filenames (case-insensitive substring matching)
     */
    std::vector<std::string> GetFilteredFiles() const;

};

}  // namespace Olympe
