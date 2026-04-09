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
#include <memory>

#include "../third_party/imgui/imgui.h"
#include "../TaskSystem/TaskGraphTemplate.h"
#include "../TaskSystem/LocalBlackboard.h"
#include "../TaskSystem/EntityBlackboard.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"
#include "VisualScriptNodeRenderer.h"
#include "SelectionEffectRenderer.h"
#include "UndoRedoStack.h"
#include "VSConnectionValidator.h"
#include "VSGraphVerifier.h"

// Phase 24 — Condition Preset UI
#include "../Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "../Editor/ConditionPreset/DynamicDataPinManager.h"
#include "../Editor/Panels/NodeConditionsPanel.h"
#include "../Editor/Panels/MathOpPropertyPanel.h"
#include "../Editor/Panels/GetBBValuePropertyPanel.h"
#include "../Editor/Panels/SetBBValuePropertyPanel.h"
#include "../Editor/Panels/VariablePropertyPanel.h"
#include "../Editor/Panels/ConditionPresetLibraryPanel.h"
#include "../Editor/Nodes/NodeBranchRenderer.h"

// Phase 26 — Switch Case Editor Modal
#include "../Editor/Modals/SwitchCaseEditorModal.h"

// Phase 26 — SubGraph File Picker Modal
#include "../Editor/Modals/SubGraphFilePickerModal.h"

// Phase 37 — Minimap integration
#include "Utilities/ImNodesCanvasEditor.h"

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

/**
 * @struct ExecutionToken
 * @brief Represents a single execution point in graph simulation (Phase 24).
 * Used for stack-based simulation of branching nodes (Sequence, Branch, etc.).
 * Allows multiple parallel execution paths to be tracked and resumed.
 */
struct ExecutionToken {
    int32_t nodeID = NODE_INDEX_NONE;  ///< Current node to execute
    int     depth  = 0;                ///< Nesting depth (for indentation)

    ExecutionToken() = default;
    ExecutionToken(int32_t id, int d) : nodeID(id), depth(d) {}
};

/**
 * @struct BlackboardValidationResult
 * @brief Result of blackboard key validation (Phase 26).
 * Contains error/warning messages and validity flag.
 */
struct BlackboardValidationResult {
    bool        IsValid         = true;
    std::string ErrorMessage    = "";   ///< Set if validation failed
    std::string WarningMessage  = "";   ///< Set if validation succeeded but has warnings
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

    /**
     * @brief Public render method for verification logs panel.
     * Called from BlueprintEditorGUI to display logs in the left panel.
     *
     * Phase 24.3 — For integration into the main layout's verification logs section.
     */
    void RenderVerificationLogsPanel();

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

    /**
     * @brief Phase 24 Global Blackboard — Renders global variables panel.
     * Shows variables from GlobalTemplateBlackboard with current entity values.
     * Called from RenderContent() as part of tab-based blackboard UI.
     */
    void RenderGlobalVariablesPanel();

    /**
     * @brief Renders the Properties panel content for a selected Branch (or While) node.
     *
     * @details
     * Displays a blue header with the node name (matching the canvas Section 1 title
     * bar styling), then delegates to NodeConditionsPanel::Render() for the compact
     * structured-conditions list, and finishes with a Breakpoint checkbox.
     *
     * The caller (RenderProperties) must @c return immediately after this call to
     * prevent the legacy condition UI from also rendering.
     *
     * @param eNode  Reference to the selected editor node (modified when dirty).
     * @param def    Reference to the node's TaskNodeDefinition (modified when dirty).
     */
    void RenderBranchNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);

    /**
     * @brief Renders the Properties panel content for a selected MathOp node.
     *
     * @details
     * Displays a blue header with the node name, then delegates to MathOpPropertyPanel::Render()
     * for operand and operator editing.
     *
     * The caller (RenderProperties) must @c return immediately after this call to
     * prevent any legacy UI from also rendering.
     *
     * @param eNode  Reference to the selected editor node (modified when dirty).
     * @param def    Reference to the node's TaskNodeDefinition (modified when dirty).
     */
    void RenderMathOpNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);

    /**
     * @brief Renders the Properties panel content for a selected Switch node (Phase 1).
     *
     * @details
     * Displays a blue header with the node name, then renders:
     * - "Switch On:" dropdown to select the blackboard variable that controls the switch
     * - Case count display
     * - "Edit Switch Cases" button to open SwitchCaseEditorModal
     * - Modal rendering with Apply integration (Phase 1 FIX: regenerates DynamicExecOutputPins)
     * - Breakpoint checkbox
     *
     * Phase 1 Key Fix: After modal Apply, regenerates DynamicExecOutputPins from switchCases
     * to ensure canvas pins are synchronized with semantic data.
     *
     * @param eNode  Reference to the selected editor node (modified when dirty).
     * @param def    Reference to the node's TaskNodeDefinition (modified when dirty).
     */
    void RenderSwitchNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);

    /**
     * @brief Renders node parameters for data nodes (GetBBValue, SetBBValue, MathOp).
     *
     * @details
     * Displays a parameters section that allows editing generic parameters that can
     * be stored and serialized alongside node-specific properties.
     *
     * @param def    Reference to the node's TaskNodeDefinition (modified when dirty).
     */
    void RenderNodeDataParameters(TaskNodeDefinition& def);

    void RenderBlackboard();
    void RenderValidationOverlay();
    void RenderBreakpoints();

    /// Part A: Node Properties panel (top-left of right panel)
    void RenderNodePropertiesPanel();

    /// Phase 31 — Content of Properties tab in Part A
    void RenderNodePropertiesPanelContent();

    /// Phase 31 — Nodes list tab in Part A (available nodes for dragging to canvas)
    void RenderAvailableNodesList();

    /// Phase 24 — Property panel renderers for While, ForEach, and SubGraph nodes
    void RenderWhileNodeProperties();
    void RenderForEachNodeProperties();
    void RenderSubGraphNodeProperties();

    /// Part B: Preset Bank panel (middle of right panel)
    void RenderPresetBankPanel();

    /// Render a single preset item in compact horizontal format with index
    void RenderPresetItemCompact(const ConditionPreset& preset, size_t index);

    /// Phase 26 — Tab system: Renders the tab bar for the 3-panel right section
    /// Displays tabs for Presets, Local Variables, and Global Variables
    void RenderRightPanelTabs();

    /// Phase 26 — Tab system: Renders the content of the active tab
    /// Dispatches to appropriate render function based on m_rightPanelTabSelection
    void RenderRightPanelTabContent();

    /// Render a single operand with dropdown for mode and value editor
    /// Returns true if the operand was modified
    bool RenderOperandEditor(Operand& operand, const char* labelSuffix);

    /// Part C: Local Variables reference panel (bottom of right panel)
    void RenderLocalVariablesPanel();

    /** @brief Renders the verification results panel (Phase 21-B). */
    void RenderVerificationPanel();

    /** @brief Runs VSGraphVerifier on the current graph and stores the result. */
    void RunVerification();

    /** @brief Simulates runtime execution of the current graph and logs traces. */
    void RunGraphSimulation();

private:
    // Phase 25 — Recursive SubGraph execution helper
    /**
     * @brief Internal recursive simulation function with cycle detection.
     * 
     * @param tmpl                 The graph template to simulate
     * @param blackboard           Current execution blackboard (isolated per SubGraph)
     * @param visitedGraphs        Set of visited graph file paths (for cycle detection)
     * @param recursionDepth       Current recursion depth (0 for root call)
     * @param traceIndent          Indentation prefix for trace messages
     * 
     * Phase 25: Tracks visited graph files and recursion depth to prevent infinite loops.
     * Warns at depth > 10, stops at depth >= 20.
     */
    void RunGraphSimulationRecursive(
        const TaskGraphTemplate* tmpl,
        std::map<std::string, TaskValue>& blackboard,
        std::unordered_set<std::string>& visitedGraphs,
        int recursionDepth,
        const std::string& traceIndent);

    // Phase 24 — Simulation helper methods

    /**
     * @brief Helper to recursively evaluate data nodes (MathOp, GetBBValue, etc.)
     * and trace their execution. Used when data pins reference other nodes.
     * @param nodeID      ID of the data node to evaluate
     * @param depth       Current trace indentation depth
     * @param indent      Indentation string prefix for logs
     */
    void EvaluateDataNode(int32_t nodeID, int depth, const std::string& indent);

    /**
     * @brief Recursively traces all upstream pure data nodes in the graph.
     * Walks data connections backward to show complete data dependency chains.
     * @param sourceNodeID         ID of the node to trace upstream from
     * @param indent               Indentation string prefix for logs
     * @param visitedDataNodes     Set of visited node IDs (prevents cycles)
     */
    void TraceUpstreamDataNodes(int32_t sourceNodeID,
                                const std::string& indent,
                                std::unordered_set<int>& visitedDataNodes);

    /**
     * @brief Gets a comprehensive property string for any node type.
     * Returns all relevant properties formatted for display in traces.
     * @param node                 The node definition to describe
     * @return                     Formatted property string with all relevant fields
     */
    std::string GetNodePropertyString(const TaskNodeDefinition& node);

    /**
     * @brief Format task parameters into a readable string.
     * Extracts parameter names and values for display in traces.
     * @param parameters           Parameter binding map from a task node
     * @param indent               Indentation string prefix for formatting
     * @return                     Formatted parameter string
     */
              std::string FormatTaskParameters(const std::unordered_map<std::string, ParameterBinding>& parameters,
                                               const std::string& indent);

         public:

    /**
     * @brief Removes blackboard entries with empty keys or VariableType::None.
     *
     * Called before serialization (Fix #1) to guarantee the blackboard is clean.
     * Logs each removed entry to SYSTEM_LOG.
     */
    void ValidateAndCleanBlackboardEntries();

    /**
     * @brief Commits any pending key-name edits stored in m_pendingBlackboardEdits.
     *
     * Flushes deferred InputText changes so that the template reflects the
     * most recent user input before Save is attempted.
     */
    void CommitPendingBlackboardEdits();

    /**
     * @brief Resets the ImNodes canvas panning to (0,0) before saving node positions.
     *
     * BUG-003 Fix: Saves current panning in m_lastViewportPanning for optional
     * restoration after save. Node positions are stored in grid space (independent
     * of viewport pan) via GetNodeGridSpacePos(), so this reset is a belt-and-
     * suspenders safety measure rather than strictly required.
     */
    void ResetViewportBeforeSave();

    /**
     * @brief Restores the ImNodes canvas panning saved by ResetViewportBeforeSave().
     *
     * BUG-003 Fix #5 (optional UX continuity): call after SerializeAndWrite()
     * so the viewport does not visually jump for the user.
     */
    void AfterSave();

    /**
     * @brief Converts a screen-space position to canvas (editor) space.
     *
     * @param screenPos  Position in absolute screen-pixel coordinates.
     * @return           Position in ImNodes editor (canvas) space.
     *
     * Correct conversion: removes canvas origin and viewport pan, then
     * divides by zoom (ImNodes 0.4 has no zoom, zoom is always 1.0f).
     */
    ImVec2 ScreenToCanvasPos(ImVec2 screenPos) const;

    /**
     * @brief Returns a filtered subset of blackboard entries matching a type.
     *
     * UX Enhancement #3 — used by type-filtered variable dropdowns so that,
     * e.g., a Switch node only shows Int variables in its combo box.
     *
     * @param allVars      Full blackboard variable list to filter.
     * @param expectedType The VariableType to keep.
     * @return             Vector containing only entries whose Type == expectedType.
     */
    static std::vector<BlackboardEntry> GetVariablesByType(
        const std::vector<BlackboardEntry>& allVars,
        VariableType expectedType);

    // -----------------------------------------------------------------------
    // Phase 26 — Blackboard Validation Helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Validates a blackboard key according to schema rules.
     *
     * Rules:
     * 1. Key must not be empty
     * 2. Key must not be a duplicate
     * 3. Global keys should follow "scope:key" format (warning)
     *
     * @param key           The key to validate
     * @param isGlobal      Whether this is a global variable
     * @param excludeIndex  Skip duplicate check for entry at index (-1 = check all)
     * @return              Validation result with IsValid flag and messages
     */
    BlackboardValidationResult ValidateBlackboardKey(
        const std::string& key,
        bool isGlobal,
        int excludeIndex = -1);

    /**
     * @brief Validates a complete blackboard entry.
     *
     * Checks that the entry has a non-empty key and a valid type.
     *
     * @param entry The entry to validate
     * @return      true if entry is valid, false otherwise
     */
    bool ValidateBlackboardEntry(const BlackboardEntry& entry);

    // -----------------------------------------------------------------------
    // Phase 23-B.4 — Condition Editor UI helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Renders the full editor UI for one Condition entry on a Branch/While node.
     *
     * Shows left/right mode selectors (Pin | Variable | Const), value inputs,
     * an operator combo, and a live preview line.
     *
     * @param condition      Reference to the condition being edited (modified in place).
     * @param conditionIndex 0-based index used for ImGui PushID uniqueness.
     * @param allVars        Full blackboard entry list for variable dropdowns.
     * @param availablePins  List of available pin references for Pin mode.
     */
    void RenderConditionEditor(Condition& condition,
                               int conditionIndex,
                               const std::vector<BlackboardEntry>& allVars,
                               const std::vector<std::string>& availablePins);

    /**
     * @brief Renders a type-filtered variable selector combo box.
     *
     * Only variables whose Type matches expectedType are shown.
     *
     * @param selectedVar  Currently selected variable name (modified on change).
     * @param allVars      Full blackboard entry list to filter.
     * @param expectedType VariableType used to filter entries.
     * @param label        ImGui widget label.
     */
    void RenderVariableSelector(std::string& selectedVar,
                                const std::vector<BlackboardEntry>& allVars,
                                VariableType expectedType,
                                const char* label);

    /**
     * @brief Renders a type-aware const value input widget.
     *
     * Bool -> Checkbox, Int -> InputInt, Float -> InputFloat,
     * String -> InputText, Vector -> InputFloat3.
     *
     * @param value    TaskValue to edit (modified on change).
     * @param varType  Determines which widget to display.
     * @param label    ImGui widget label.
     */
    void RenderConstValueInput(TaskValue& value,
                               VariableType varType,
                               const char* label);

    /**
     * @brief Renders a pin selector combo box.
     *
     * @param selectedPin   Currently selected pin reference (modified on change).
     * @param availablePins List of available pin reference strings.
     * @param label         ImGui widget label.
     */
    void RenderPinSelector(std::string& selectedPin,
                           const std::vector<std::string>& availablePins,
                           const char* label);

    /**
     * @brief Builds a human-readable preview string for a condition.
     *
     * Format: "[Left] <op> [Right]", e.g. "[Var: health] > [Const: 50]".
     *
     * @param cond  The condition to describe.
     * @return      Preview string.
     */
    static std::string BuildConditionPreview(const Condition& cond);

    // -----------------------------------------------------------------------
    // Canvas helpers
    // -----------------------------------------------------------------------

    int  AllocNodeID();
    int  AllocLinkID();

    /** Maps node ID -> ImNodes attribute UID for an exec-in pin. */
    int  ExecInAttrUID(int nodeID) const;
    /** Maps node ID + pin index -> ImNodes attribute UID for exec-out pins. */
    int  ExecOutAttrUID(int nodeID, int pinIndex) const;
    /** Maps node ID + data pin index -> ImNodes attribute UID for data-in pins. */
    int  DataInAttrUID(int nodeID, int pinIndex) const;
    /** Maps node ID + data pin index -> ImNodes attribute UID for data-out pins. */
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

    /**
     * @brief Syncs ALL presets from the registry to the template.
     * Phase 24 FIX: Ensures that presets created/modified via UI are included in save.
     * Previously, only presets modified via operand editor were synced; newly created
     * presets were missed. Now called before serialization to guarantee completeness.
     */
    void SyncPresetsFromRegistryToTemplate();

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

    // -----------------------------------------------------------------------
    // Phase 21-B — Graph Verification
    // -----------------------------------------------------------------------

    /// Latest verification result (produced by RunVerification())
    VSVerificationResult m_verificationResult;

    /// True once RunVerification() has been called at least once for the current graph
    bool m_verificationDone = false;

    /// Verification log messages (populated by RunVerification())
    /// Phase 24.3 — for display in the verification output panel
    std::vector<std::string> m_verificationLogs;

    /// Simulation execution traces (populated by RunGraphSimulation())
    /// Phase 24.4 — added to verification logs for display
    std::vector<std::string> m_simulationTraces;

    /// True if simulation has been run
    bool m_simulationDone = false;

    /// Phase 24.4 — Execution token stack for multi-branch simulation
    /// Enables proper handling of Sequence nodes with multiple branches
    std::vector<ExecutionToken> m_executionTokenStack;

    /// Node ID to focus/scroll to on next RenderCanvas() frame (-1 = none)
    int m_focusNodeID = -1;

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

    // Switch-specific edit state (Phase 22-A)
    std::string                       m_propEditSwitchVar;    ///< Live buffer for switchVariable field
    std::vector<SwitchCaseDefinition> m_propEditSwitchCases;  ///< Per-case label edit buffers

    /// Undo/Redo command stack for reversible graph editing operations
    UndoRedoStack m_undoStack;

    /// Pending dynamic pin addition (from [+] button clicked in canvas)
    bool        m_pendingAddPin       = false;
    int         m_pendingAddPinNodeID = -1;

    /// Pending dynamic pin removal (from [-] button clicked in canvas)
    bool        m_pendingRemovePin       = false;
    int         m_pendingRemovePinNodeID = -1;
    int         m_pendingRemovePinDynIdx = -1;  ///< 0-based index in DynamicExecOutputPins

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

    // -----------------------------------------------------------------------
    // Blackboard edit state (BUG-002)
    // -----------------------------------------------------------------------

    /// Deferred key-name edits for blackboard entries: index -> pending new key.
    /// Committed in CommitPendingBlackboardEdits() before Save.
    std::unordered_map<int, std::string> m_pendingBlackboardEdits;

    // -----------------------------------------------------------------------
    // Layout state (UX — resizable properties panel)
    // -----------------------------------------------------------------------

    /// Width of the properties+blackboard panel on the right.
    /// Adjusted by the drag-to-resize handle between the canvas and the panel.
    float m_propertiesPanelWidth = 0.0f;

    /// Height of the Node Properties panel (Part A) in the right panel.
    /// Adjusted by the drag-to-resize handle between Part A and Part B.
    float m_nodePropertiesPanelHeight = 0.0f;

    /// Height of the Preset Bank panel (Part B) in the right panel.
    /// Adjusted by the drag-to-resize handle between Part B and Part C.
    float m_presetBankPanelHeight = 0.0f;

    /// Height of the Verification Logs panel in the left panel (Blueprint Files).
    /// Adjusted by the drag-to-resize handle between Blueprint Files and Logs.
    /// Phase 24.3 — Added for verification output logging in left panel
    float m_verificationLogsPanelHeight = 0.0f;

    // -----------------------------------------------------------------------
    // Viewport save/restore state (BUG-003 Fix)
    // -----------------------------------------------------------------------

    /// Canvas panning saved by ResetViewportBeforeSave() for restoration in AfterSave().
    Vector m_lastViewportPanning = Vector(0.0f, 0.0f);

    /// True after ResetViewportBeforeSave() has been called and before AfterSave().
    bool m_viewportResetDone = false;

    // -----------------------------------------------------------------------
    // Phase 24 — Condition Preset UI (NodeConditionsPanel integration)
    // -----------------------------------------------------------------------

    /// Global registry of ConditionPreset objects.  Loaded from
    /// Blueprints/Presets/condition_presets.json on Initialize().
    ConditionPresetRegistry m_presetRegistry;

    /// Dynamic pin manager shared across all Branch nodes in this panel.
    std::unique_ptr<DynamicDataPinManager> m_pinManager;

    /// Specialized renderer for Branch nodes (4-section layout with conditions).
    std::unique_ptr<NodeBranchRenderer> m_branchRenderer;

    /// Properties-panel sub-widget for the selected Branch node.
    std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;

    /// Properties-panel sub-widget for the selected MathOp node.
    std::unique_ptr<MathOpPropertyPanel> m_mathOpPanel;

    /// Properties-panel sub-widget for the selected GetBBValue node.
    std::unique_ptr<GetBBValuePropertyPanel> m_getBBPanel;

    /// Properties-panel sub-widget for the selected SetBBValue node.
    std::unique_ptr<SetBBValuePropertyPanel> m_setBBPanel;

    /// Properties-panel sub-widget for the selected Variable node (data pure).
    std::unique_ptr<VariablePropertyPanel> m_variablePanel;

    /// Global condition preset library panel (UI for creating/editing/deleting presets).
    std::unique_ptr<ConditionPresetLibraryPanel> m_libraryPanel;

    /// Phase 26 — Switch Case Editor Modal
    std::unique_ptr<SwitchCaseEditorModal> m_switchCaseModal;

    /// Phase 26 — SubGraph File Picker Modal
    std::unique_ptr<SubGraphFilePickerModal> m_subGraphModal;

    /// ID of the node currently loaded into m_conditionsPanel (-1 = none).
    int m_condPanelNodeID = -1;

    // -----------------------------------------------------------------------
    // Phase 24 Global Blackboard Integration
    // -----------------------------------------------------------------------

    /// Per-entity blackboard instance (combines local + global variables)
    /// Created in Initialize() and manages scope-aware access to both local and global vars.
    std::unique_ptr<EntityBlackboard> m_entityBlackboard;

    /// Phase 26 — Right panel tab selection
    /// 0 = Presets, 1 = Local Variables, 2 = Global Variables
    int m_rightPanelTabSelection = 0;

    /// Top panel tab selection (Part A of right panel)
    /// 0 = Properties, 1 = Nodes
    int m_topPanelTabSelection = 0;

    // -----------------------------------------------------------------------
    // Phase 33 — Selection Effect Renderer (In-Loop Glow System)
    // -----------------------------------------------------------------------

    // -----------------------------------------------------------------------
    // Phase 33 — Selection Effect Renderer (In-Loop Glow System)
    // -----------------------------------------------------------------------

    /// Renders glow effect for selected nodes (cyan halo + thickened border).
    /// Integrated into node rendering loop for correct scope and z-ordering.
    /// Provides unified selection UX across all canvas types.
    SelectionEffectRenderer m_selectionRenderer;

    // -----------------------------------------------------------------------
    // Phase 37 — Canvas Editor Minimap Support
    // -----------------------------------------------------------------------

    /// Canvas editor adapter for minimap support (Phase 37)
    /// Abstracts imnodes minimap rendering through ICanvasEditor interface
    std::unique_ptr<ImNodesCanvasEditor> m_canvasEditor;

    /// Minimap visibility flag for VisualScript canvas
    bool m_minimapVisible = true;

    /// Minimap size ratio (0.05-0.5 of canvas)
    float m_minimapSize = 0.15f;

    /// Minimap position (0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight)
    int m_minimapPosition = 1;  // TopRight by default
};

} // namespace Olympe
