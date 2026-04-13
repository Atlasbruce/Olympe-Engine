#pragma once

#include <string>
#include <memory>
#include <functional>

namespace Olympe {

// Forward declarations
class IGraphDocument;
class SaveFilePickerModal;
class FilePickerModal;

/**
 * @class CanvasToolbarRenderer
 * @brief Unified toolbar renderer for all graph editor types
 * 
 * Provides consistent Save/SaveAs/Browse buttons across VisualScript,
 * BehaviorTree, and EntityPrefab editors.
 * 
 * Design Pattern: Strategy pattern for toolbar UI rendering
 * Responsibilities:
 * 1. Render toolbar buttons (Save, SaveAs, Browse)
 * 2. Manage button states (enable/disable based on dirty flag)
 * 3. Handle button click callbacks
 * 4. Integrate with DataManager for modals
 * 5. Display file path and status
 * 
 * Usage:
 * ```cpp
 * CanvasToolbarRenderer toolbar(graphDocument);
 * ImGui::SetNextItemWidth(300.0f);
 * toolbar.Render();
 * toolbar.RenderModal();  // Call after ImGui::Render()
 * ```
 * 
 * Modal Flow:
 * - Save: If path empty → SaveAs modal, else save to path
 * - SaveAs: Show SaveFilePickerModal with folder panel (LEFT) + files (RIGHT)
 * - Browse: Show FilePickerModal for opening existing documents
 * 
 * Integration:
 * - Works with IGraphDocument polymorphic interface
 * - Delegates modal operations to DataManager
 * - Manages modal state and completion callbacks
 */
class CanvasToolbarRenderer {
public:
    /**
     * Constructor
     * @param document Pointer to IGraphDocument (non-owning reference)
     */
    explicit CanvasToolbarRenderer(IGraphDocument* document);

    /**
     * Destructor
     */
    ~CanvasToolbarRenderer();

    /**
     * Render toolbar UI
     * Call this in your render loop before ImGui::Render()
     * 
     * Renders buttons: [Save] [SaveAs] [Browse] | [Path Display]
     * Button states depend on document dirty flag and file path
     */
    void Render();

    /**
     * Render modals
     * Call this AFTER ImGui::Render() to render any open modals
     * Handles SaveFilePickerModal and FilePickerModal rendering
     */
    void RenderModals();

    /**
     * Check if any modal is currently open
     * @return true if Save/Browse modal is open
     */
    bool IsModalOpen() const;

    /**
     * Get currently selected file (after modal completion)
     * @return Full path to selected file, empty if modal cancelled
     */
    std::string GetSelectedFilePath() const;

    /**
     * Get toolbar display text
     * @return Status string (e.g., "Saved: /path/to/file.json")
     */
    std::string GetStatusText() const;

    /**
     * Force toolbar to update (call after external document changes)
     */
    void Invalidate();

    /**
     * Set custom callback for save completion
     * @param callback Function called after successful save
     * 
     * Callback signature: void(const std::string& filePath)
     */
    using SaveCallback = std::function<void(const std::string&)>;
    void SetOnSaveComplete(SaveCallback callback);

    /**
     * Set custom callback for browse completion
     * @param callback Function called after file selection
     */
    using BrowseCallback = std::function<void(const std::string&)>;
    void SetOnBrowseComplete(BrowseCallback callback);

    /**
     * Get supported file filter for Save/SaveAs operations
     * @return File filter string (e.g., "*.json")
     */
    std::string GetSaveFileFilter() const;

    /**
     * Get supported file filter for Browse operations
     * @return File filter string for this document type
     */
    std::string GetBrowseFileFilter() const;

private:
    // ========== STATE ==========
    IGraphDocument* m_document;                    ///< Non-owning reference to document
    std::string m_selectedFilePath;                ///< Result from modal
    SaveCallback m_onSaveComplete;                 ///< User callback on save
    BrowseCallback m_onBrowseComplete;             ///< User callback on browse
    
    // ========== MODAL STATE ==========
    bool m_showSaveAsModal = false;                ///< SaveFilePickerModal open?
    bool m_showBrowseModal = false;                ///< FilePickerModal open?
    SaveFilePickerModal* m_saveModal = nullptr;    ///< Owned modal instance
    FilePickerModal* m_browseModal = nullptr;      ///< Owned modal instance

    // ========== UI STATE ==========
    bool m_isDirty = false;                        ///< Last known dirty state
    std::string m_lastSavePath;                    ///< Path displayed in toolbar

    // ========== PRIVATE METHODS ==========

    /**
     * Render toolbar buttons
     * Internal method called by Render()
     */
    void RenderButtons();

    /**
     * Render file path display
     * Shows current file path or "Untitled" if unsaved
     */
    void RenderPathDisplay();

    /**
     * Handle Save button clicked
     * Opens SaveAs if no path, else saves directly
     */
    void OnSaveClicked();

    /**
     * Handle SaveAs button clicked
     * Opens SaveFilePickerModal
     */
    void OnSaveAsClicked();

    /**
     * Handle Browse button clicked
     * Opens FilePickerModal to load existing document
     */
    void OnBrowseClicked();

    /**
     * Handle SaveAs modal completion
     * Called when user confirms file save in modal
     */
    void OnSaveAsComplete(const std::string& filePath);

    /**
     * Handle Browse modal completion
     * Called when user selects file in Browse modal
     */
    void OnBrowseComplete(const std::string& filePath);

    /**
     * Update button enabled states based on document state
     */
    void UpdateButtonStates();

    /**
     * Get directory for modal initialization
     * @return Default directory or document's current path
     */
    std::string GetInitialDirectory() const;

    /**
     * Get suggested filename for SaveAs modal
     * @return Current filename or "Untitled"
     */
    std::string GetSuggestedFilename() const;

    /**
     * Perform actual save operation
     * @param filePath Path to save to
     * @return true if save succeeded
     */
    bool ExecuteSave(const std::string& filePath);

    /**
     * Perform actual load operation
     * @param filePath Path to load from
     * @return true if load succeeded
     */
    bool ExecuteLoad(const std::string& filePath);
};

} // namespace Olympe
