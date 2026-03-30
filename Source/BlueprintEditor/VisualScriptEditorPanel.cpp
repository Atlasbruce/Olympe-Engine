/**
 * @file VisualScriptEditorPanel.cpp
 * @brief ImNodes graph editor implementation for ATS VS graphs (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unordered_set>

namespace Olympe {

// ============================================================================
// Lifecycle Methods
// ============================================================================
// NOTE: Core lifecycle methods (Constructor, Destructor, Initialize, Shutdown)
// have been extracted to VisualScriptEditorPanel_Core.cpp for better organization
// and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_Core.cpp:
//   - VisualScriptEditorPanel() Constructor
//   - ~VisualScriptEditorPanel() Destructor
//   - Initialize() — ImNodes context, UI helpers, preset loading
//   - Shutdown() — Resource cleanup and registry management
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_Core.cpp

// ============================================================================
// Helper Methods
// ============================================================================
// NOTE: 6 helper methods have been extracted to VisualScriptEditorPanel_Helpers.cpp
// for better code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_Helpers.cpp:
//   - AllocNodeID() — Allocate unique node IDs
//   - AllocLinkID() — Allocate unique link IDs
//   - ExecInAttrUID(int nodeID) — UID for execution input pins
//   - ExecOutAttrUID(int nodeID, int pinIndex) — UID for execution output pins
//   - DataInAttrUID(int nodeID, int pinIndex) — UID for data input pins
//   - DataOutAttrUID(int nodeID, int pinIndex) — UID for data output pins
//
// UID Scheme (nodeID * 10000 + offset):
//   0–99:   Reserved for exec-in
//   100–199: Exec-out pins
//   200–299: Data-in pins
//   300–399: Data-out pins
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_Helpers.cpp

// ============================================================================
// Pin Name Helpers
// ============================================================================
// NOTE: 5 pin helper methods have been extracted to VisualScriptEditorPanel_PinHelpers.cpp
// for better code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_PinHelpers.cpp:
//   - GetExecInputPins(TaskNodeType type) — Static exec input pins by node type
//   - GetExecOutputPins(TaskNodeType type) — Static exec output pins by node type
//   - GetExecOutputPinsForNode(const TaskNodeDefinition& def) — Inc. dynamic pins
//   - GetDataInputPins(TaskNodeType type) — Static data input pins by node type
//   - GetDataOutputPins(TaskNodeType type) — Static data output pins by node type
//
// Pin Categories (by node type):
//   - EntryPoint: exec-out only {"Out"}
//   - Branch: exec-out {"Then", "Else"} + dynamic data-in pins
//   - While: exec-out {"Loop", "Completed"}
//   - ForEach: exec-out {"Loop Body", "Completed"}
//   - AtomicTask: exec-in/out {"In"} / {"Completed"}
//   - MathOp: data-in {"A", "B"}, data-out {"Result"} (data-pure)
//   - GetBBValue: data-out {"Value"} (data-pure)
//   - SetBBValue: exec-in/out, data-in {"Value"}
//   - SubGraph: exec-in/out
//   - VSSequence: exec-out + dynamic pins
//   - Switch: exec-out + dynamic pins
//   - Delay: exec-in/out
//   - DoOnce: exec-in/out
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_PinHelpers.cpp

// ============================================================================
// Node management
// ============================================================================

// ============================================================================
// Node Management Methods
// ============================================================================
// NOTE: 4 node management methods (AddNode, RemoveNode, ConnectExec, ConnectData)
// have been extracted to VisualScriptEditorPanel_NodeManagement.cpp for better
// code organization and maintainability (Phase 24 refactoring).
//
// Methods included in VisualScriptEditorPanel_NodeManagement.cpp:
//   - AddNode() — Create a new node with type-specific data pin initialization
//   - RemoveNode() — Delete a node and all associated connections
//   - ConnectExec() — Create an execution pin connection between nodes
//   - ConnectData() — Create a data pin connection between nodes
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_NodeManagement.cpp
//
// These methods integrate with the undo/redo command system (ICommand/UndoStack)
// to ensure all graph modifications (node creation, deletion, link creation) can
// be reversed via Ctrl+Z. All operations update both m_editorNodes (canvas state)
// and m_template (model), with link graph rebuilding after each change.

// ============================================================================
// Template / Canvas Sync
// ============================================================================
// NOTE: 5 template synchronization methods have been extracted to 
// VisualScriptEditorPanel_TemplateSync.cpp (Phase 6).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_TemplateSync.cpp
//
// Methods included:
//   - SyncCanvasFromTemplate() — Load nodes from template into editor canvas
//   - SyncTemplateFromCanvas() — Update template with current editor node state
//   - RebuildLinks() — Rebuild all visual links from template connections
//   - SyncEditorNodesFromTemplate() — Restore editor nodes during undo/redo
//   - RemoveLink(int linkID) — Delete a link and push undo command

// ============================================================================
// Load / Save
// ============================================================================
// NOTE: 6 file operation methods have been extracted to 
// VisualScriptEditorPanel_FileOperations.cpp (Phase 7).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_FileOperations.cpp
//
// Methods included:
//   - LoadTemplate() — Load blueprint from file/memory with preset loading (Phase 24)
//   - Save() — Save current graph to m_currentPath
//   - SaveAs() — Save graph to new path
//   - SyncNodePositionsFromImNodes() — Sync grid-space positions (BUG-003 Fix)
//   - SyncPresetsFromRegistryToTemplate() — Phase 24 preset synchronization
//   - SerializeAndWrite() — Complete JSON v4 serialization with all Phase 24 features
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_FileOperations.cpp

// ============================================================================
// Blackboard validation helpers (BUG-002 Fix #1)
// ============================================================================
// NOTE: ValidateAndCleanBlackboardEntries() and CommitPendingBlackboardEdits()
// have been extracted to VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================
// NOTE: ResetViewportBeforeSave(), AfterSave(), and ScreenToCanvasPos()
// have been extracted to VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// UX Enhancement #3 — Type-filtered variable utility
// ============================================================================
// NOTE: GetVariablesByType() has been extracted to 
// VisualScriptEditorPanel_Utilities.cpp (Phase 5).
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Utilities.cpp

// ============================================================================
// Rendering
// ============================================================================

// ============================================================================
// Undo/Redo wrappers, Rendering (extracted to VisualScriptEditorPanel_RenderingCore.cpp)
// ============================================================================
// NOTE: Core rendering methods have been extracted to VisualScriptEditorPanel_RenderingCore.cpp
// (Phase 8 refactoring).
//
// Methods implemented in VisualScriptEditorPanel_RenderingCore.cpp:
//   - PerformUndo() — Undo operation with position restoration
//   - PerformRedo() — Redo operation with position restoration
//   - Render() — Main render function (delegates to RenderContent)
//   - RenderContent() — Main content layout (toolbar, canvas, properties)
//   - RenderToolbar() — Toolbar with save/load/verify buttons
//   - RenderSaveAsDialog() — Save-As modal dialog
//
// Implementation file: Source/BlueprintEditor/VisualScriptEditorPanel_RenderingCore.cpp



// ============================================================================
// Canvas Rendering, Node Palette, Context Menus (Phase 9 Extraction)
// ============================================================================
// Implementation extracted to VisualScriptEditorPanel_Canvas.cpp
//
// Extracted methods (~1053 LOC total):
//   - void RenderCanvas() — ImNodes graph rendering, node palette detection, link management
//   - void RenderNodePalette() — Right-click context menu for adding nodes
//   - void RenderContextMenus() — Node/link right-click context menus
//
// See: Source/BlueprintEditor/VisualScriptEditorPanel_Canvas.cpp
//
// Forward declarations (implemented in Canvas.cpp):
//   void RenderCanvas();
//   void RenderNodePalette();
//   void RenderContextMenus();


// ============================================================================
// Branch / While node — dedicated Properties panel renderer
// ============================================================================

// ============================================================================
// Branch node — dedicated Properties panel renderer
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Properties.cpp
//       as part of Phase 10 (Properties Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderBranchNodeProperties(VSEditorNode& eNode,
//                                                            TaskNodeDefinition& def);
//
// Renders the Properties panel content for a selected Branch (or While) node.
// Displays a blue header with the node name, then delegates to
// NodeConditionsPanel::Render() for structured-conditions list, and finishes
// with a Breakpoint checkbox.
// ============================================================================

// ============================================================================
// MathOp node — dedicated Properties panel renderer
// ============================================================================

// ============================================================================
// MathOp node — dedicated Properties panel renderer
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Properties.cpp
//       as part of Phase 10 (Properties Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderMathOpNodeProperties(VSEditorNode& eNode,
//                                                           TaskNodeDefinition& def);
//
// Renders the Properties panel content for a selected MathOp node.
// Displays a blue header with the node name, then delegates to
// MathOpPropertyPanel::Render() for operand and operator editing.
// ============================================================================

// ============================================================================
// Generic parameter editor for data nodes
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Properties.cpp
//       as part of Phase 10 (Properties Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderNodeDataParameters(TaskNodeDefinition& def);
//
// Renders node parameters for data nodes (GetBBValue, SetBBValue, MathOp).
// Displays a parameters section that allows editing generic parameters that
// can be stored and serialized alongside node-specific properties.
//
// Phase 24 — Generic parameter editor for data nodes (GetBBValue, SetBBValue, MathOp)
// Allows storing and serializing additional parameters on data nodes
// ============================================================================

// ============================================================================
// Main Properties panel dispatcher
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Properties.cpp
//       as part of Phase 10 (Properties Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderProperties();
//
// Main properties panel dispatcher (~100 LOC).
// Renders the Properties panel content for the currently selected node.
// Dispatches to type-specific renderers (RenderBranchNodeProperties, RenderMathOpNodeProperties, etc.).
// Handles node name editing, type-specific field editing, breakpoint toggling, and undo/redo integration.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderBlackboard() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Blackboard.cpp
//       as part of Phase 12 (Blackboard Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Blackboard.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderBlackboard();
//
// Purpose: Render local blackboard with BUG-001 validation, variable editing.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderValidationOverlay() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderValidationOverlay();
//
// Purpose: Validate graph connections and SubGraph paths.
// ============================================================================

// ============================================================================
// PHASE 12 — RunVerification() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RunVerification();
//
// Purpose: Execute VSGraphVerifier and populate verification logs for display.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderVerificationPanel() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderVerificationPanel();
//
// Purpose: Render verification panel with error/warning/info status and issue list.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderVerificationLogsPanel() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderVerificationLogsPanel();
//
// Purpose: Render verification logs panel with severity-based issue grouping.
// ============================================================================

// ============================================================================
// Phase 23-B.4 — Condition Editor UI helpers (MIGRATED to VisualScriptEditorPanel_ConditionUI.cpp)
// ============================================================================

// ============================================================================
// Alternative Node Properties Panel renderer
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Properties.cpp
//       as part of Phase 10 (Properties Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderNodePropertiesPanel();
//
// Alternative renderer for the Node Properties panel (~80 LOC).
// Provides a second interface for editing node properties with type-specific fields
// displayed in a more structured format.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderPresetBankPanel() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderPresetBankPanel();
//
// Purpose: Render preset bank panel with add/list presets functionality.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderPresetItemCompact() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Verification.cpp
//       as part of Phase 12 (Verification Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Verification.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderPresetItemCompact(const ConditionPreset& preset, size_t index);
//
// Purpose: Render single preset item in compact horizontal layout with edit/dup/delete buttons.
// ============================================================================

// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_ConditionUI.cpp
//       as part of Phase 11 (Condition UI Helpers Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_ConditionUI.cpp
//
// Original signature:
//   bool VisualScriptEditorPanel::RenderOperandEditor(Operand& operand, const char* labelSuffix);
//
// Unified operand selector (Pin/Const/Variable) with Phase 24 global support
// ============================================================================
// PHASE 24 Panel Integration — Part C: Local Variables Reference
// ============================================================================

// ============================================================================
// PHASE 12 — RenderLocalVariablesPanel() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Blackboard.cpp
//       as part of Phase 12 (Blackboard Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Blackboard.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderLocalVariablesPanel();
//
// Purpose: Render local variables panel with BUG-001 validation and type-aware editing.
// ============================================================================

// ============================================================================
// PHASE 12 — RenderGlobalVariablesPanel() MIGRATED
// ============================================================================
// NOTE: This method has been MIGRATED to VisualScriptEditorPanel_Blackboard.cpp
//       as part of Phase 12 (Blackboard Panel Extraction).
//       See: Source/BlueprintEditor/VisualScriptEditorPanel_Blackboard.cpp
//
// Original signature:
//   void VisualScriptEditorPanel::RenderGlobalVariablesPanel();
//
// Purpose: Render global variables panel with Phase 24 registry integration.
// ============================================================================

} // namespace Olympe
