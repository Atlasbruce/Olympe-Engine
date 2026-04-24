#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Olympe {

// Forward declarations
class IGraphDocument;
class IGraphRenderer;
class CanvasToolbarRenderer;

/**
 * @class CanvasFramework
 * @brief Unified orchestration framework for all graph editor types
 * 
 * Central hub that coordinates:
 * 1. Document lifecycle (Load/Save/Dirty tracking)
 * 2. Toolbar rendering and button handling
 * 3. Modal management (Save/Browse dialogs)
 * 4. Canvas rendering delegation
 * 5. Feature state management (undo/redo, zoom, pan)
 * 
 * Design Goals:
 * - Single Responsibility: Orchestrate interactions
 * - Polymorphism: Work with any IGraphDocument
 * - Centralization: No scattered modal/toolbar code
 * - Consistency: Identical UX across all graph types
 * 
 * Usage Pattern:
 * ```cpp
 * // Create framework for a specific document
 * CanvasFramework framework(graphDocument);
 * 
 * // In render loop:
 * framework.Render();        // Renders toolbar + canvas + modals
 * 
 * // After user action (optional):
 * framework.SaveDocument(filePath);
 * framework.LoadDocument(filePath);
 * ```
 * 
 * Integration Points:
 * - Works with VisualScriptEditorPanel, BehaviorTreeRenderer, EntityPrefabRenderer
 * - Documents provide IGraphDocument interface
 * - Toolbar provides unified Save/SaveAs/Browse buttons
 * - Modals handled through DataManager
 * 
 * Responsibilities:
 * 1. Document Management
 *    - Track current document
 *    - Handle Load/Save operations
 *    - Monitor dirty state
 *
 * 2. UI Layout
 *    - Arrange toolbar at top
 *    - Arrange canvas in center
 *    - Arrange modals as overlays
 *    - Optional: status bar at bottom
 *
 * 3. Feature Coordination
 *    - Zoom/Pan controls
 *    - Undo/Redo (future)
 *    - Search/Filter (future)
 *    - Export (future)
 *
 * 4. State Queries
 *    - IsDirty()
 *    - HasDocument()
 *    - GetCurrentPath()
 */
class CanvasFramework {
public:
    /**
     * Constructor
     * @param document Pointer to IGraphDocument (non-owning reference)
     */
    explicit CanvasFramework(IGraphDocument* document);

    /**
     * Destructor
     */
    ~CanvasFramework();

    // ========== LIFECYCLE ==========

    /**
     * Check if framework has a valid document
     * @return true if document is not null
     */
    bool HasDocument() const;

    /**
     * Set new document to manage
     * @param document Pointer to new document (non-owning reference)
     * 
     * Call this when switching between documents or opening a new one
     */
    void SetDocument(IGraphDocument* document);

    /**
     * Get current document
     * @return Pointer to current IGraphDocument (may be nullptr)
     */
    IGraphDocument* GetDocument() const;

    // ========== RENDERING ==========

    /**
     * Render complete framework (toolbar + canvas + modals)
     * Call once per ImGui frame
     * 
     * Layout:
     * [============ Toolbar ============]
     * [                                 ]
     * [         Canvas Area             ]
     * [     (delegates to renderer)     ]
     * [                                 ]
     * [===============================  ]
     * 
     * Modals rendered as overlays
     */
    void Render();

    /**
     * Render only the toolbar
     * Separate method for custom layout scenarios
     */
    void RenderToolbar();

    /**
     * Render only the canvas
     * Separate method for custom layout scenarios
     * 
     * Note: Delegates to document's renderer
     */
    void RenderCanvas();

    /**
     * Render only the modals
     * Must be called AFTER ImGui::Render()
     */
    void RenderModals();

    /**
     * Set canvas area size and position
     * @param x Left edge in screen space
     * @param y Top edge in screen space
     * @param width Canvas width in pixels
     * @param height Canvas height in pixels
     * 
     * Call before rendering if custom layout needed
     */
    void SetCanvasLayout(float x, float y, float width, float height);

    // ========== DOCUMENT OPERATIONS ==========

    /**
     * Save document to file
     * @param filePath Full path to save location
     * @return true if save succeeded
     * 
     * Displays appropriate UI feedback on success/failure
     */
    bool SaveDocument(const std::string& filePath);

    /**
     * Load document from file
     * @param filePath Full path to load from
     * @return true if load succeeded
     * 
     * Sets new document and displays UI feedback
     */
    bool LoadDocument(const std::string& filePath);

    /**
     * Check if document has unsaved changes
     * @return true if dirty, false otherwise (or no document)
     */
    bool IsDirty() const;

    /**
     * Get current file path
     * @return Full path to current file, empty if unsaved
     */
    std::string GetCurrentPath() const;

    /**
     * Get current document name
     * @return Display name (e.g., "graph.bt.json")
     */
    std::string GetDocumentName() const;

    /**
     * Get document type name
     * @return Human-readable type (e.g., "Behavior Tree")
     */
    std::string GetDocumentTypeName() const;

    // ========== MODAL STATE ==========

    /**
     * Check if any modal is currently open
     * @return true if Save/Browse modal is open
     */
    bool IsModalOpen() const;

    /**
     * Manually trigger SaveAs dialog
     * (Usually called by toolbar button, but available for custom workflows)
     */
    void OpenSaveAsDialog();

    /**
     * Manually trigger Browse dialog
     * (Usually called by toolbar button, but available for custom workflows)
     */
    void OpenBrowseDialog();

    // ========== FEATURE QUERIES ==========

    /**
     * Get toolbar renderer instance
     * @return Pointer to internal toolbar (may be used for customization)
     */
    CanvasToolbarRenderer* GetToolbar();

    /**
     * Get toolbar renderer (const version)
     */
    const CanvasToolbarRenderer* GetToolbar() const;

    /**
     * Get canvas renderer from document
     * @return Pointer to IGraphRenderer from document
     */
    IGraphRenderer* GetCanvasRenderer();

        /**
         * Get canvas renderer (const version)
         */
        const IGraphRenderer* GetCanvasRenderer() const;

        private:
            // ========== STATE ==========
            IGraphDocument* m_document;                    ///< Non-owning reference to document
            CanvasToolbarRenderer* m_toolbar;              ///< Owned toolbar instance
    
    // ========== LAYOUT ==========
    float m_canvasX = 0.0f;                        ///< Canvas layout X
    float m_canvasY = 0.0f;                        ///< Canvas layout Y
    float m_canvasWidth = 800.0f;                  ///< Canvas layout width
    float m_canvasHeight = 600.0f;                 ///< Canvas layout height
    bool m_customCanvasLayout = false;             ///< Custom layout requested?

    // ========== UI STATE ==========
    bool m_showStatusBar = true;                   ///< Show bottom status bar?
    std::string m_statusMessage;                   ///< Current status message
    float m_statusMessageTimeout = 0.0f;           ///< Timeout counter (0 = inactive)

    // ========== PRIVATE METHODS ==========

    /**
     * Initialize toolbar for new document
     */
    void InitializeToolbar();

    /**
     * Handle toolbar save completion callback
     */
    void OnToolbarSaveComplete(const std::string& filePath);

    /**
     * Handle toolbar browse completion callback
     */
    void OnToolbarBrowseComplete(const std::string& filePath);

    /**
     * Render status bar (optional)
     */
    void RenderStatusBar();

    /**
     * Display temporary status message
     * @param message Text to display
     * @param durationSeconds How long to show (0 = indefinite)
     */
    void SetStatusMessage(const std::string& message, float durationSeconds = 3.0f);
};

} // namespace Olympe
