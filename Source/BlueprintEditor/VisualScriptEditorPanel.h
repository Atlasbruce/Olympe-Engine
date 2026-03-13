/**
 * @file VisualScriptEditorPanel.h
 * @brief ImNodes-based graph editor for ATS Visual Script graphs (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * VisualScriptEditorPanel renders an interactive graph canvas using ImNodes.
 * It supports creating, connecting, editing and saving VS graph nodes (v4 schema).
 * It is loaded in place of NodeGraphPanel when the active graph has
 * graphType == "VisualScript".
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "../TaskSystem/TaskGraphTemplate.h"
#include "../TaskSystem/LocalBlackboard.h"
#include "VisualScriptNodeRenderer.h"
#include "UndoRedoStack.h"
#include "VSConnectionValidator.h"

// Forward-declare ImNodes context type (defined in imnodes.h) in the global
// namespace so it can be referenced from within the Olympe namespace below.
struct ImNodesEditorContext;

namespace Olympe {

/**
 * @struct VSEditorNode
 * @brief Editor-side representation of a node in the VS graph canvas.
 *
 * Holds position, selection state, and mirrors TaskNodeDefinition data for
 * display purposes.
 */
struct VSEditorNode {
    int          nodeID      = -1;
    float        posX        = 0.0f;
    float        posY        = 0.0f;
    bool         selected    = false;
    bool         positionSet = false;
    TaskNodeDefinition def;
};

/**
 * @struct VSEditorLink
 * @brief Editor-side representation of an exec or data connection.
 */
struct VSEditorLink {
    int         linkID    = -1;   ///< ImNodes link UID
    int         srcAttrID = -1;   ///< Source attribute UID
    int         dstAttrID = -1;   ///< Destination attribute UID
    bool        isData    = false; ///< false = exec link, true = data link
};

// ============================================================================
// VisualScriptEditorPanel
// ============================================================================

/**
 * @class VisualScriptEditorPanel
 * @brief ImNodes graph editor for ATS Visual Script v4 graphs.
 *
 * @details
 * Typical usage:
 * @code
 *   // Editor initialization
 *   VisualScriptEditorPanel panel;
 *   panel.Initialize();
 *
 *   // Each frame
 *   if (panel.IsVisible())
 *       panel.Render();
 *
 *   // Shutdown
 *   panel.Shutdown();
 * @endcode
 */
class VisualScriptEditorPanel {
public:

    VisualScriptEditorPanel();
    ~VisualScriptEditorPanel();

    void Initialize();
    void Shutdown();

    /**
     * @brief Renders the full panel window.
     * Calls RenderToolbar(), RenderCanvas(), RenderProperties(), RenderBlackboard().
     */
    void Render();

    /**
     * @brief Renders panel content without window wrapper - for fixed layout.
     */
    void RenderContent();

    /** @brief Returns true if the panel window is visible. */
    bool IsVisible() const { return m_visible; }

    /** @brief Show / hide the panel. */
    void SetVisible(bool v) { m_visible = v; }

    // -----------------------------------------------------------------------
    // Graph management
    // -----------------------------------------------------------------------

    /**
     * @brief Loads a VS graph template into the editor canvas.
     * @param tmpl   Non-null pointer to the template to load.
     * @param path   File path associated with the template (for Save).
     */
    void LoadTemplate(const TaskGraphTemplate* tmpl, const std::string& path);

    /**
     * @brief Saves the current canvas state to JSON v4 at the loaded path.
     * @return true on success.
     */
    bool Save();

    /**
     * @brief Saves the current canvas state to a new JSON v4 file.
     * @param path  Destination file path.
     * @return true on success.
     */
    bool SaveAs(const std::string& path);

    /**
     * @brief Returns the currently loaded file path (empty if unsaved).
     */
    const std::string& GetCurrentPath() const { return m_currentPath; }

    /**
     * @brief Returns true when there are unsaved modifications.
     */
    bool IsDirty() const { return m_dirty; }

    // -----------------------------------------------------------------------
    // Node management (called by palette / tests)
    // -----------------------------------------------------------------------

    /**
     * @brief Creates a new node on the canvas.
     * @param type  Node type.
     * @param x     Canvas X position.
     * @param y     Canvas Y position.
     * @return The new node's ID.
     */
    int AddNode(TaskNodeType type, float x, float y);

    /**
     * @brief Removes a node from the canvas.
     * @param nodeID  Node ID to remove.
     */
    void RemoveNode(int nodeID);

    /**
     * @brief Creates an exec connection between two nodes.
     * @param srcNodeID   Source node ID.
     * @param srcPinName  Source exec-out pin name (e.g. "Then").
     * @param dstNodeID   Destination node ID.
     * @param dstPinName  Destination exec-in pin name (usually "In").
     */
    void ConnectExec(int srcNodeID, const std::string& srcPinName,
                     int dstNodeID, const std::string& dstPinName);

    /**
     * @brief Creates a data connection between two nodes.
     */
    void ConnectData(int srcNodeID, const std::string& srcPinName,
                     int dstNodeID, const std::string& dstPinName);

    /**
     * @brief Returns a reference to the internal template being edited.
     * Used by tests to inspect the state.
     */
    const TaskGraphTemplate& GetTemplate() const { return m_template; }

private:

    // -----------------------------------------------------------------------
    // Undo/Redo wrappers
    // -----------------------------------------------------------------------

    /**
     * @brief Undoes the last command and syncs the canvas (nodes + links).
     * Calls SyncEditorNodesFromTemplate() + RebuildLinks() so that ghost
     * links are eliminated and node positions are restored correctly.
     */
    void PerformUndo();

    /**
     * @brief Re-applies the last undone command and syncs the canvas.
     */
    void PerformRedo();

    // -----------------------------------------------------------------------
    // Rendering sub-sections
    // -----------------------------------------------------------------------

    void RenderToolbar();
    void RenderSaveAsDialog();
    void RenderCanvas();
    void RenderNodePalette();

    /**
     * @brief Render node/link context menus opened by right-click detection.
     * Must be called AFTER EndNodeEditor() so popups are in correct ImGui scope.
     */
    void RenderContextMenus();

    void RenderProperties();
    void RenderBlackboard();
    void RenderValidationOverlay();
    void RenderBreakpoints();

    // -----------------------------------------------------------------------
    // Canvas helpers
    // -----------------------------------------------------------------------

    int  AllocNodeID();
    int  AllocLinkID();

    /** Maps node ID → ImNodes attribute UID for an exec-in pin. */
    int  ExecInAttrUID(int nodeID) const;
    /** Maps node ID + pin index → ImNodes attribute UID for exec-out pins. */
    int  ExecOutAttrUID(int nodeID, int pinIndex) const;
    /** Maps node ID + data pin index → ImNodes attribute UID for data-in pins. */
    int  DataInAttrUID(int nodeID, int pinIndex) const;
    /** Maps node ID + data pin index → ImNodes attribute UID for data-out pins. */
    int  DataOutAttrUID(int nodeID, int pinIndex) const;

    /** Returns the exec-out pin names for a node type. */
    static std::vector<std::string> GetExecOutputPins(TaskNodeType type);

    /**
     * @brief Returns exec-out pin names for a node definition,
     *        including any dynamically-added pins (VSSequence).
     */
    std::vector<std::string> GetExecOutputPinsForNode(const TaskNodeDefinition& def) const;

    /** Returns the exec-in pin names for a node type. */
    static std::vector<std::string> GetExecInputPins(TaskNodeType type);

    /** Returns the data-in pin names for a node type. */
    static std::vector<std::string> GetDataInputPins(TaskNodeType type);

    /** Returns the data-out pin names for a node type. */
    static std::vector<std::string> GetDataOutputPins(TaskNodeType type);

    /** Builds the in-memory TaskGraphTemplate from the editor nodes/links. */
    void SyncTemplateFromCanvas();

    /** Builds the editor canvas from the in-memory TaskGraphTemplate. */
    void SyncCanvasFromTemplate();

    /** Rebuilds ImNodes exec/data link arrays from the template. */
    void RebuildLinks();

    /**
     * @brief Rebuilds m_editorNodes from m_template, preserving existing node positions.
     * Called after Undo/Redo to synchronise the canvas with the template state.
     */
    void SyncEditorNodesFromTemplate();

    /**
     * @brief Removes an ImNodes link (and its underlying template connection) by link ID.
     * @param linkID  The ImNodes link UID to remove.
     */
    void RemoveLink(int linkID);

    /** Serializes the template to JSON v4 and writes to a file. */
    bool SerializeAndWrite(const std::string& path);

    /**
     * @brief Pulls the current node positions from ImNodes into m_editorNodes.
     * Only nodes that have been rendered at least once (present in
     * m_positionedNodes) are updated to avoid ImNodes assertions.
     * Must be called before serialization to capture user-moved positions.
     */
    void SyncNodePositionsFromImNodes();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    bool m_visible        = true;
    bool m_dirty          = false;
    bool m_paletteOpen    = false;

    // Per-instance ImNodes editor context.
    // Created in Initialize() and destroyed in Shutdown().
    // Set as the active context at the start of every RenderCanvas() call so
    // that node positions / panning are preserved independently per tab.
    ImNodesEditorContext* m_imnodesContext = nullptr;

    // True after LoadTemplate() until the first RenderCanvas() call that
    // applies SetNodeEditorSpacePos for all loaded nodes.
    bool m_needsPositionSync = false;

    /// Set to true by Undo/Redo; causes next frame to skip SyncNodePositionsFromImNodes()
    /// so that the positions applied by SyncEditorNodesFromTemplate() are not overwritten
    /// by stale ImNodes state before the new positions have been rendered once.
    bool m_skipPositionSyncNextFrame = false;

    /// Set to true immediately after Undo/Redo; blocks node movement tracking
    /// for 1 frame to allow ImNodes to render the new positions before resuming
    /// normal position sync. Prevents stale drag-start positions.
    bool m_justPerformedUndoRedo = false;

    std::string m_currentPath;

    /// The template currently being edited
    TaskGraphTemplate m_template;

    /// Editor nodes (mirrors m_template.Nodes + position/selection state)
    std::vector<VSEditorNode> m_editorNodes;

    /// Editor links (exec + data)
    std::vector<VSEditorLink> m_editorLinks;

    /// Nodes for which ImNodes has been given a position
    std::unordered_set<int> m_positionedNodes;

    /// Next available node ID
    int m_nextNodeID = 1;

    /// Next available ImNodes link ID
    int m_nextLinkID = 1;

    /// Currently selected node (for properties panel)
    int m_selectedNodeID = -1;

    /// Validation messages (rebuilt each frame)
    std::vector<std::string> m_validationWarnings;
    std::vector<std::string> m_validationErrors;

    /// Right-click paste position
    float m_contextMenuX = 0.0f;
    float m_contextMenuY = 0.0f;

    /// Node ID captured at the moment a right-click context menu was opened on a node
    int m_contextNodeID = -1;

    /// Link ID captured at the moment a right-click context menu was opened on a link
    int m_contextLinkID = -1;

    // -----------------------------------------------------------------------
    // Properties panel — undo snapshot state
    // "commit on release" pattern: snapshot the field value when the widget
    // receives focus (IsItemActivated), push undo only on IsItemDeactivatedAfterEdit.
    // -----------------------------------------------------------------------

    /// Node ID that was selected when RenderProperties() last entered focus
    int m_propEditNodeIDOnFocus = -1;

    /// Snapshot values captured at focus time for each editable field
    std::string m_propEditOldName;
    std::string m_propEditOldTaskID;
    std::string m_propEditOldConditionID;
    std::string m_propEditOldBBKey;
    std::string m_propEditOldMathOp;
    std::string m_propEditOldSubGraphPath;
    float       m_propEditOldDelay = 0.0f;

    /// Undo/Redo command stack for reversible graph editing operations
    UndoRedoStack m_undoStack;

    /// Pending dynamic pin addition (from [+] button clicked in canvas)
    bool        m_pendingAddPin       = false;
    int         m_pendingAddPinNodeID = -1;

    /// Per-node drag-start positions used to record a single MoveNodeCommand
    /// per drag gesture instead of one command per frame.
    /// Key: nodeID  Value: (posX, posY) at the moment the drag was detected.
    std::unordered_map<int, std::pair<float, float> > m_nodeDragStartPositions;

    // -----------------------------------------------------------------------
    // Drag & drop pending state (two-phase node creation)
    // Phase 1: detect drop inside BeginDragDropTarget, store params
    // Phase 2: create node after EndNodeEditor (outside editor scope)
    // -----------------------------------------------------------------------

    /// True when a node drop is pending processing this frame
    bool          m_pendingNodeDrop  = false;
    TaskNodeType  m_pendingNodeType  = TaskNodeType::EntryPoint;
    float         m_pendingNodeX     = 0.0f;
    float         m_pendingNodeY     = 0.0f;

    // -----------------------------------------------------------------------
    // Save As dialog state
    // -----------------------------------------------------------------------

    /// True when the "Save As" modal should be opened next frame
    bool        m_showSaveAsDialog  = false;
    /// Buffer for the user-entered filename (without extension)
    char        m_saveAsFilename[256];
    /// Currently selected destination directory
    std::string m_saveAsDirectory   = "Blueprints/AI";
    /// Extension to append when saving (derived from m_currentPath; defaults to ".ats")
    std::string m_saveAsExtension   = ".ats";
};

} // namespace Olympe
