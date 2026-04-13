#pragma once

#include <string>
#include <memory>

namespace Olympe {

// Forward declaration
class IGraphRenderer;

/**
 * @enum DocumentType
 * @brief Classification of graph document types
 * 
 * Used for UI adaptation, modal configuration, and type-specific behavior
 */
enum class DocumentType {
    VISUAL_SCRIPT,    ///< Visual Script graph (imnodes-based)
    BEHAVIOR_TREE,    ///< Behavior Tree graph (imnodes-based)
    ENTITY_PREFAB,    ///< Entity Prefab graph (custom canvas)
    UNKNOWN           ///< Unknown or invalid type
};

/**
 * @class IGraphDocument
 * @brief Abstract base class for all graph document types
 * 
 * Defines unified interface for document lifecycle, serialization, and rendering.
 * Enables polymorphic handling of VisualScript, BehaviorTree, and EntityPrefab graphs.
 * 
 * Responsibilities:
 * 1. Load/Save document from/to file
 * 2. Track dirty state (unsaved changes)
 * 3. Provide metadata (name, type, path)
 * 4. Return renderer for canvas display
 * 5. Notify framework of document modifications
 * 
 * Implementations:
 * - EntityPrefabGraphDocument: Direct implementation (owns graph data)
 * - VisualScriptGraphDocument: Adapter wrapper (wraps existing tab data)
 * - BehaviorTreeGraphDocument: Adapter wrapper (wraps existing tab data)
 */
class IGraphDocument {
public:
    virtual ~IGraphDocument() = default;

    // ========== LIFECYCLE ==========

    /**
     * Load document from file
     * @param filePath Full or relative path to document file
     * @return true if load succeeded, false otherwise (check logs for details)
     * 
     * On success:
     * - Document state updated from file
     * - Dirty flag cleared
     * - Renderer available via GetRenderer()
     * 
     * On failure:
     * - Document state unchanged
     * - Error logged to system output
     * - Caller responsible for user notification
     */
    virtual bool Load(const std::string& filePath) = 0;

    /**
     * Save document to file
     * @param filePath Full or relative path to save location
     * @return true if save succeeded, false otherwise (check logs for details)
     * 
     * On success:
     * - Document written to file in current schema version
     * - FilePath updated (SetFilePath called internally)
     * - Dirty flag cleared
     * 
     * On failure:
     * - Original file unchanged
     * - Error logged to system output
     * - Caller responsible for user notification
     */
    virtual bool Save(const std::string& filePath) = 0;

    /**
     * Check if document has unsaved changes
     * @return true if document modified since last Load/Save, false otherwise
     * 
     * Used for:
     * - Determining if "Save" button is active/highlighted
     * - Showing unsaved indicator in tab name
     * - Warning before closing document
     */
    virtual bool IsDirty() const = 0;

    // ========== METADATA ==========

    /**
     * Get document display name
     * @return Name for UI display (e.g., "SimpleGraph.bt.json" or "Untitled-1")
     * 
     * Used in:
     * - Tab label
     * - Window title
     * - Recent files list
     */
    virtual std::string GetName() const = 0;

    /**
     * Get document type classification
     * @return DocumentType enum value (VISUAL_SCRIPT, BEHAVIOR_TREE, etc)
     * 
     * Used for:
     * - UI adaptation (which buttons to show in toolbar)
     * - Modal configuration (file type filter)
     * - Type-specific behavior in framework
     */
    virtual DocumentType GetType() const = 0;

    /**
     * Get current file path
     * @return Full or relative path to document file (empty if unsaved)
     * 
     * Used for:
     * - Display in window title or status bar
     * - Determining default save location for SaveAs
     * - Recent files tracking
     */
    virtual std::string GetFilePath() const = 0;

    /**
     * Update document file path
     * @param path New file path (called after SaveAs operation)
     * 
     * Called automatically by Save() after successful write.
     * Also called by framework when document moved/renamed externally.
     */
    virtual void SetFilePath(const std::string& path) = 0;

    // ========== RENDERING ==========

    /**
     * Get renderer for canvas display
     * @return Pointer to IGraphRenderer (valid for lifetime of document)
     * 
     * Contract:
     * - Returned pointer never becomes nullptr
     * - Renderer created during document construction
     * - Renderer manages all canvas drawing logic
     * 
     * Used by:
     * - CanvasFramework for rendering graph
     * - TabManager for delegating render calls
     */
    virtual IGraphRenderer* GetRenderer() = 0;

    /**
     * Get renderer (const version)
     * @return Const pointer to IGraphRenderer
     * 
     * Used for read-only renderer access (e.g., checking render state)
     */
    virtual const IGraphRenderer* GetRenderer() const = 0;

    // ========== NOTIFICATIONS ==========

    /**
     * Notify framework that document was modified
     * 
     * Called by:
     * - Editor whenever nodes/connections change
     * - Property panels when editing parameters
     * - Any operation that changes graph state
     * 
     * Typically sets internal dirty flag.
     * Framework uses this to update UI state (Save button, etc).
     */
    virtual void OnDocumentModified() = 0;

    /**
     * Get human-readable document type name
     * @param type DocumentType enum value
     * @return String representation (e.g., "Visual Script", "Behavior Tree")
     * 
     * Static helper for UI display.
     */
    static const char* GetTypeNameStatic(DocumentType type) {
        switch (type) {
            case DocumentType::VISUAL_SCRIPT:  return "Visual Script";
            case DocumentType::BEHAVIOR_TREE:  return "Behavior Tree";
            case DocumentType::ENTITY_PREFAB:  return "Entity Prefab";
            case DocumentType::UNKNOWN:
            default:                           return "Unknown";
        }
    }

    /**
     * Get document type name
     * @return String representation of this document's type
     */
    std::string GetTypeName() const {
        return GetTypeNameStatic(GetType());
    }
};

// Shared pointer type for document ownership
using GraphDocumentPtr = std::shared_ptr<IGraphDocument>;

} // namespace Olympe
