/**
 * @file SaveFilePickerModal.h
 * @brief Centralized Save As modal for all file save operations (Phase 40 Enhancement).
 * @author Olympe Engine
 * @date 2026-03-21
 *
 * Provides a unified ImGui modal dialog for file saving across all editors.
 * Automatically handles file extensions based on file type.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

/**
 * @enum SaveFileType
 * @brief Supported file types for the centralized save modal.
 */
enum class SaveFileType
{
    BehaviorTree,  ///< .bt.json files
    Blueprint,     ///< .ats files (SubGraph/VisualScript)
    EntityPrefab,  ///< .pref.json files (Entity Prefab)
    Audio          ///< .ogg files (audio assets)
};

/**
 * @class SaveFilePickerModal
 * @brief Centralized ImGui modal dialog for file saving across all editors.
 *
 * Usage:
 *   1. SaveFilePickerModal modal(SaveFileType::BehaviorTree);
 *   2. modal.Open(defaultDirectory, suggestedFilename);
 *   3. During ImGui loop: modal.Render();
 *   4. After frame: if (modal.IsConfirmed()) { path = modal.GetSelectedFile(); }
 *
 * Features:
 *   - Auto-extension handling based on SaveFileType
 *   - Path navigation
 *   - Filename input with optional extension preview
 *   - Overwrite confirmation if file exists
 *   - Folder navigation with ".." for parent directory
 *   - Platform-specific implementation (Windows + Unix)
 */
class SaveFilePickerModal {
public:
    /**
     * @brief Constructs a save file picker modal for the given file type.
     * @param fileType Type of file to save (BehaviorTree, Blueprint, etc.)
     */
    explicit SaveFilePickerModal(SaveFileType fileType);
    ~SaveFilePickerModal() = default;

    // ====================================================================
    // Modal Lifecycle
    // ====================================================================

    /**
     * @brief Opens the modal with initial directory and filename.
     * @param directory Starting directory path
     * @param suggestedFilename Suggested filename (without extension)
     * 
     * Extension will be automatically appended based on file type.
     */
    void Open(const std::string& directory, const std::string& suggestedFilename = "");

    /**
     * @brief Closes the modal without confirming changes.
     */
    void Close();

    /**
     * @brief Renders the modal UI. Must be called every frame while open.
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
     * @brief Returns true if user clicked "Save" button.
     */
    bool IsConfirmed() const { return m_confirmed; }

    /**
     * @brief Returns the selected file path with extension (only valid if IsConfirmed() is true).
     * 
     * Example: "Gamedata/MyTree.bt.json"
     */
    const std::string& GetSelectedFile() const { return m_selectedFile; }

    /**
     * @brief Returns the file type this modal handles.
     */
    SaveFileType GetFileType() const { return m_fileType; }

private:
    // ====================================================================
    // Configuration & State
    // ====================================================================

    SaveFileType m_fileType;                ///< Type of file to save
    bool m_isOpen = false;                  ///< Is modal currently visible
    bool m_confirmed = false;               ///< Did user click Save
    std::string m_selectedFile = "";        ///< Full path with extension
    std::string m_currentPath = "";         ///< Current directory
    
    // ====================================================================
    // File Listing State
    // ====================================================================

    std::vector<std::string> m_fileList;    ///< Files in current directory
    std::vector<std::string> m_folderList;  ///< Folders in current directory

    // ====================================================================
    // UI State
    // ====================================================================

    char m_pathBuffer[512] = "";            ///< Path input text buffer
    char m_filenameBuffer[256] = "";        ///< Filename input (without extension)
    bool m_showOverwriteConfirm = false;    ///< Show overwrite confirmation dialog
    std::string m_previousPath = "";        ///< Previous frame's path for change detection (PHASE 49 FIX)

    // ====================================================================
    // Helper Methods
    // ====================================================================

    /**
     * @brief Returns the default directory for this file type.
     */
    std::string GetDefaultDirectory() const;

    /**
     * @brief Returns the file extension for this file type (e.g., ".bt.json").
     */
    std::string GetFileExtension() const;

    /**
     * @brief Returns a description of this file type for the modal title.
     */
    std::string GetModalTitle() const;

    /**
     * @brief Returns user-friendly description text.
     */
    std::string GetDescriptionText() const;

    /**
     * @brief Refreshes the file and folder lists for the current directory.
     * PHASE 46 FIX: Logs only on initialization/state changes, not per-frame.
     */
    void RefreshFileList();

    /**
     * @brief Internal implementation of RefreshFileList with logging control.
     * @param bLog If true, log the directory scan results. If false, scan silently.
     * 
     * PHASE 46 FIX: This separates the actual file scanning (which is fast) 
     * from logging (which should only happen on state changes, not every frame).
     */
    void RefreshFileListInternal(bool bLog);

    /**
     * @brief Renders the file list in the modal.
     */
    void RenderFileList();

    /**
     * @brief Renders folder navigation.
     */
    void RenderFolderList();

    /**
     * @brief Renders filename input and extension display.
     */
    void RenderFilenameInput();

    /**
     * @brief Renders action buttons (Save/Cancel).
     */
    void RenderActionButtons();

    /**
     * @brief Checks if a file exists at the given path.
     */
    bool FileExists(const std::string& path) const;

    /**
     * @brief Renders the overwrite confirmation dialog.
     */
    void RenderOverwriteConfirmation();
};

}  // namespace Olympe
