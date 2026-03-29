# 📑 Function-to-File Mapping Guide

## Overview
This document maps every method in `VisualScriptEditorPanel` to its target file after refactoring.

**Legend**:
- ✅ Public method (in .h header)
- 🔒 Private method
- 🏗️ Helper/utility function
- 📝 State member

---

## PUBLIC API (No Changes)

### Graph Management
| Method | File | Notes |
|--------|------|-------|
| `LoadTemplate()` | **_FileOps** | Loads graph into editor |
| `Save()` | **_FileOps** | Save to current path |
| `SaveAs()` | **_FileOps** | Save to new path |
| `GetCurrentPath()` | **_FileOps** | Returns current file path |
| `IsDirty()` | **_FileOps** | Returns unsaved changes flag |
| `GetTemplate()` const | **_Canvas** | Returns m_template |

### Node Management
| Method | File | Notes |
|--------|------|-------|
| `AddNode()` | **_Interaction** | Create node at position |
| `RemoveNode()` | **_Interaction** | Delete node from canvas |
| `ConnectExec()` | **_Connections** | Create exec link |
| `ConnectData()` | **_Connections** | Create data link |

### Phase 24.3 Execution Testing (NEW)
| Method | File | Notes |
|--------|------|-------|
| `RunExecutionTest()` | **_Verification** | Execute simulation |
| `GetExecutionTestPanel()` | **_Verification** | Access test panel |
| `RenderVerificationLogsPanel()` | **_Verification** | Phase 24.3 UI |

### Core Lifecycle
| Method | File | Notes |
|--------|------|-------|
| `Initialize()` | **Core.cpp** | Create ImNodes context, initialize sub-panels |
| `Shutdown()` | **Core.cpp** | Destroy ImNodes context |
| `Render()` | **Core.cpp** | Dispatcher: calls RenderToolbar, RenderCanvas, etc. |
| `RenderContent()` | **Core.cpp** | Panel content without window wrapper |
| `IsVisible()` const | **Core.cpp** | Returns m_visible |
| `SetVisible()` | **Core.cpp** | Sets m_visible |

---

## PRIVATE METHODS (Implementation Files)

### Canvas & ImNodes Management

#### VisualScriptEditorPanel_Canvas.cpp

```cpp
// ImNodes Context
void Initialize()                               // Constructor, Initialize ImNodes
void Shutdown()                                 // Destructor, Destroy ImNodes

// Rendering Loop
void RenderCanvas()                             // Main canvas rendering with ImNodes

// State Synchronization (BiDirectional)
void SyncCanvasFromTemplate()                  // Template → Editor visual state
void SyncTemplateFromCanvas()                  // Editor visual → Template data
void SyncEditorNodesFromTemplate()             // Rebuild editor nodes (post Undo/Redo)
void SyncNodePositionsFromImNodes()            // Pull positions from ImNodes to editor nodes

// Rebuilding
void RebuildLinks()                            // Rebuild link list from template connections

// ID Allocation
int AllocNodeID()                              // Generate next node ID
int AllocLinkID()                              // Generate next link ID

// Attribute UID Mapping (ImNodes internals)
int ExecInAttrUID(int nodeID) const            // Calculate exec-in pin UID
int ExecOutAttrUID(int nodeID, int pinIndex) const  // Calculate exec-out pin UID
int DataInAttrUID(int nodeID, int pinIndex) const   // Calculate data-in pin UID
int DataOutAttrUID(int nodeID, int pinIndex) const  // Calculate data-out pin UID

// Position Conversion
ImVec2 ScreenToCanvasPos(ImVec2 screenPos) const    // Convert screen coords to canvas coords

// Viewport Management (BUG-003)
void ResetViewportBeforeSave()                 // Save viewport pan before reset
void AfterSave()                               // Restore viewport pan after save
```

#### State Members (_Canvas)
```cpp
ImNodesEditorContext* m_imnodesContext;        // Per-panel ImNodes context
std::vector<VSEditorNode> m_editorNodes;       // Visual node positions/state
std::vector<VSEditorLink> m_editorLinks;       // Visual link state
std::unordered_set<int> m_positionedNodes;     // Set of positioned node IDs
int m_nextNodeID;                              // Counter for node ID allocation
int m_nextLinkID;                              // Counter for link ID allocation
bool m_needsPositionSync;                      // Flag: sync positions on first render
bool m_skipPositionSyncNextFrame;              // Flag: skip position sync (post Undo)
bool m_justPerformedUndoRedo;                  // Flag: skip drag tracking (post Undo)
Vector m_lastViewportPanning;                  // Viewport pan (BUG-003)
bool m_viewportResetDone;                      // Viewport reset flag (BUG-003)
```

---

### Link & Pin Management

#### VisualScriptEditorPanel_Connections.cpp

```cpp
// Link Operations
void ConnectExec()                             // Create exec connection
void ConnectData()                             // Create data connection
void RemoveLink(int linkID)                    // Delete link by ImNodes ID

// Link Rebuild
void RebuildLinks()                            // Reconstruct editor links from template

// Pin Queries (Static)
static std::vector<std::string> GetExecOutputPins(TaskNodeType type)
static std::vector<std::string> GetExecInputPins(TaskNodeType type)
static std::vector<std::string> GetDataInputPins(TaskNodeType type)
static std::vector<std::string> GetDataOutputPins(TaskNodeType type)

// Pin Queries (Instance)
std::vector<std::string> GetExecOutputPinsForNode(const TaskNodeDefinition& def) const
    // Returns exec-out pin names including dynamic pins (VSSequence)
```

#### State Members (_Connections)
```cpp
std::vector<VSEditorLink> m_editorLinks;       // Shared with _Canvas
int m_contextLinkID;                           // Selected link for context menu
int m_nextLinkID;                              // Shared with _Canvas
```

---

### UI Rendering

#### VisualScriptEditorPanel_Rendering.cpp

```cpp
// Main Rendering Dispatcher
void Render()                                  // Full panel window (calls components)
void RenderContent()                           // Panel content without window

// Toolbar & Menus
void RenderToolbar()                           // Top toolbar with buttons
void RenderNodePalette()                       // Node creation palette
void RenderContextMenus()                      // Right-click context menus (must call AFTER EndNodeEditor)
void RenderValidationOverlay()                 // Overlay showing validation errors

// Canvas
void RenderCanvas()                            // [MOVED TO _Canvas.cpp]
```

#### State Members (_Rendering)
```cpp
bool m_visible;                                // Panel visibility
bool m_paletteOpen;                            // Palette expansion state
float m_contextMenuX, m_contextMenuY;          // Right-click menu position
int m_contextNodeID;                           // Node ID for context menu
```

---

### Node Properties Panel

#### VisualScriptEditorPanel_NodeProperties.cpp

```cpp
// Properties Dispatcher
void RenderProperties()                        // Main properties panel (dispatcher)
void RenderNodePropertiesPanel()               // Part A: Node properties

// Type-Specific Property Renderers
void RenderBranchNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def)
    // Branch/While node properties with condition editor
void RenderMathOpNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def)
    // MathOp node properties via MathOpPropertyPanel
void RenderNodeDataParameters(TaskNodeDefinition& def)
    // Generic parameters for data nodes

// Phase 23-B.4 Condition UI Helpers
void RenderConditionEditor(Condition& condition, int conditionIndex,
                          const std::vector<BlackboardEntry>& allVars,
                          const std::vector<std::string>& availablePins)
    // Full condition editor (left/right/operator/preview)

void RenderVariableSelector(std::string& selectedVar,
                           const std::vector<BlackboardEntry>& allVars,
                           VariableType expectedType, const char* label)
    // Type-filtered variable combo box

void RenderConstValueInput(TaskValue& value, VariableType varType, const char* label)
    // Type-aware const value input (checkbox/int/float/string/vector)

void RenderPinSelector(std::string& selectedPin,
                      const std::vector<std::string>& availablePins, const char* label)
    // Pin reference combo box

static std::string BuildConditionPreview(const Condition& cond)
    // Generate human-readable condition string
```

#### State Members (_NodeProperties)
```cpp
int m_selectedNodeID;                          // Currently selected node for properties
int m_propEditNodeIDOnFocus;                   // Node ID when focus detected
std::string m_propEditOldName;                 // Snapshot: old node name (for undo)
std::string m_propEditOldTaskID;               // Snapshot: old task ID
std::string m_propEditOldConditionID;          // Snapshot: old condition ID
std::string m_propEditOldBBKey;                // Snapshot: old blackboard key
std::string m_propEditOldMathOp;               // Snapshot: old math operator
std::string m_propEditOldSubGraphPath;         // Snapshot: old subgraph path
float m_propEditOldDelay;                      // Snapshot: old delay value

// Switch-specific state
std::string m_propEditSwitchVar;               // Live edit buffer for switch var
std::vector<SwitchCaseDefinition> m_propEditSwitchCases;  // Case labels for editing

// Sub-panels
std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
std::unique_ptr<MathOpPropertyPanel> m_mathOpPanel;
std::unique_ptr<GetBBValuePropertyPanel> m_getBBPanel;
std::unique_ptr<SetBBValuePropertyPanel> m_setBBPanel;
std::unique_ptr<VariablePropertyPanel> m_variablePanel;
int m_condPanelNodeID;                         // Node currently loaded in condition panel

float m_nodePropertiesPanelHeight;             // Part A height adjustment
```

---

### Blackboard Management

#### VisualScriptEditorPanel_Blackboard.cpp

```cpp
// Blackboard Rendering
void RenderBlackboard()                        // Main blackboard panel
void RenderLocalVariablesPanel()               // Local variables tab (Phase 24)
void RenderGlobalVariablesPanel()              // Global variables tab (Phase 24)

// Blackboard Maintenance
void CommitPendingBlackboardEdits()            // Flush deferred edits (BUG-002)
void ValidateAndCleanBlackboardEntries()       // Remove invalid entries (Fix #1)

// Utilities
static std::vector<BlackboardEntry> GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars, VariableType expectedType)
    // Filter variables by type
```

#### State Members (_Blackboard)
```cpp
std::unordered_map<int, std::string> m_pendingBlackboardEdits;  // Deferred edits
std::unique_ptr<EntityBlackboard> m_entityBlackboard;           // Local + global vars
int m_blackboardTabSelection;                  // Tab: 0=Local, 1=Global
float m_verificationLogsPanelHeight;           // Phase 24.3 log panel height
```

---

### Verification & Validation

#### VisualScriptEditorPanel_Verification.cpp

```cpp
// Verification
void RunVerification()                         // Execute VSGraphVerifier
void RenderValidationOverlay()                 // Show validation overlay
void RenderVerificationPanel()                 // Phase 21-B verification results
void RenderVerificationLogsPanel()             // Phase 24.3 execution test logs

// Phase 24.3 Execution Testing (NEW)
std::vector<ValidationError> RunExecutionTest()
    // Execute GraphExecutionSimulator and capture results
ExecutionTestPanel& GetExecutionTestPanel()
    // Accessor for test panel
```

#### State Members (_Verification)
```cpp
std::vector<std::string> m_validationWarnings;   // Warning messages
std::vector<std::string> m_validationErrors;     // Error messages
VSVerificationResult m_verificationResult;       // Latest verification results
bool m_verificationDone;                         // Flag: verification executed
std::vector<std::string> m_verificationLogs;     // Phase 24.3 execution logs
int m_focusNodeID;                               // Node to scroll to on next render

// Phase 24.3
std::unique_ptr<ExecutionTestPanel> m_executionTestPanel;
```

---

### File Operations & Serialization

#### VisualScriptEditorPanel_FileOps.cpp

```cpp
// Graph Loading
void LoadTemplate(const TaskGraphTemplate* tmpl, const std::string& path)
    // Load graph into editor canvas

// Graph Saving
bool Save()                                    // Save to m_currentPath
bool SaveAs(const std::string& path)          // Save to new path
bool SerializeAndWrite(const std::string& path)  // Write JSON v4 to file

// UI
void RenderSaveAsDialog()                      // SaveAs modal dialog

// Accessors
const std::string& GetCurrentPath() const      // Returns m_currentPath
bool IsDirty() const                           // Returns m_dirty flag

// Utilities
void SyncPresetsFromRegistryToTemplate()       // Phase 24: sync presets to template
void ResetViewportBeforeSave()                 // BUG-003: save viewport pan
void AfterSave()                               // BUG-003: restore viewport pan
```

#### State Members (_FileOps)
```cpp
std::string m_currentPath;                     // Current file path
bool m_dirty;                                  // Unsaved changes flag
bool m_showSaveAsDialog;                       // SaveAs dialog visibility
char m_saveAsFilename[256];                    // SaveAs filename buffer
std::string m_saveAsDirectory;                 // SaveAs directory
std::string m_saveAsExtension;                 // SaveAs file extension
Vector m_lastViewportPanning;                  // Viewport pan (BUG-003)
bool m_viewportResetDone;                      // Viewport reset flag (BUG-003)
TaskGraphTemplate m_template;                  // Current graph (shared state)
```

---

### User Interaction & Undo/Redo

#### VisualScriptEditorPanel_Interaction.cpp

```cpp
// Node Operations
int AddNode(TaskNodeType type, float x, float y)    // Create new node
void RemoveNode(int nodeID)                         // Delete node

// Undo/Redo
void PerformUndo()                             // Undo last command
void PerformRedo()                             // Redo last undone command

// [Phase 24.3] Context Menu Actions
// (Handled within RenderContextMenus callback)
```

#### State Members (_Interaction)
```cpp
// Dynamic pin operations
bool m_pendingAddPin;                          // Flag: add pin pending
int m_pendingAddPinNodeID;                     // Target node for new pin
bool m_pendingRemovePin;                       // Flag: remove pin pending
int m_pendingRemovePinNodeID;                  // Target node for pin removal
int m_pendingRemovePinDynIdx;                  // Index of pin to remove

// Drag & Drop
bool m_pendingNodeDrop;                        // Flag: node drop pending
TaskNodeType m_pendingNodeType;                // Type of pending node
float m_pendingNodeX, m_pendingNodeY;          // Position of pending node

// Drag Tracking
std::unordered_map<int, std::pair<float, float>> m_nodeDragStartPositions;
    // For generating single MoveNodeCommand per drag

// Undo/Redo Stack
UndoRedoStack m_undoStack;
bool m_skipPositionSyncNextFrame;              // Skip pos sync after undo
bool m_justPerformedUndoRedo;                  // Skip drag after undo
```

---

### Condition Presets (Phase 24)

#### VisualScriptEditorPanel_Presets.cpp

```cpp
// Rendering
void RenderPresetBankPanel()                   // Part B: Preset bank display
void RenderPresetItemCompact(const ConditionPreset& preset, size_t index)
    // Compact preset item renderer

bool RenderOperandEditor(Operand& operand, const char* labelSuffix)
    // Operand (mode + value) editor with dropdown
```

#### State Members (_Presets)
```cpp
ConditionPresetRegistry m_presetRegistry;      // Global preset registry
std::unique_ptr<DynamicDataPinManager> m_pinManager;   // Dynamic pin manager
std::unique_ptr<NodeBranchRenderer> m_branchRenderer;  // Branch node renderer
std::unique_ptr<ConditionPresetLibraryPanel> m_libraryPanel;  // Library panel
int m_condPanelNodeID;                         // Shared with _NodeProperties

float m_presetBankPanelHeight;                 // Part B height adjustment
```

---

## Core Implementations (VisualScriptEditorPanel.cpp + VisualScriptEditorPanel_Core.cpp)

```cpp
// Constructor
VisualScriptEditorPanel::VisualScriptEditorPanel()

// Destructor
VisualScriptEditorPanel::~VisualScriptEditorPanel()

// Lifecycle
void Initialize()                              // [Initializes ALL sub-modules]
void Shutdown()                                // [Destroys ALL sub-modules]

// Main Render Dispatcher
void Render()                                  // [Calls all Render* methods]
void RenderContent()                           // [Calls Render internals]

// Accessors
bool IsVisible() const
void SetVisible(bool v)
const TaskGraphTemplate& GetTemplate() const
```

---

## Summary by File

| File | Method Count | State Members | Purpose |
|------|--------------|---------------|---------|
| Core.cpp | 6 | 0 | Constructor, lifecycle, dispatcher |
| _Canvas.cpp | 13 | 9 | ImNodes management, canvas sync |
| _Connections.cpp | 8 | 2 | Link/pin operations |
| _Rendering.cpp | 5 | 4 | UI rendering dispatchers |
| _NodeProperties.cpp | 10 | 14 | Properties panel, conditions |
| _Blackboard.cpp | 5 | 4 | Variables management |
| _Verification.cpp | 5 | 6 | Validation, Phase 24.3 testing |
| _FileOps.cpp | 9 | 8 | Save/load, serialization |
| _Interaction.cpp | 4 | 12 | Node manipulation, undo/redo |
| _Presets.cpp | 4 | 5 | Phase 24 presets |
| **TOTAL** | **69** | **64** | **~6200 LOC split** |

---

**Document Version**: 1.0  
**Last Updated**: 2026-03-09  
**Used During**: Phase 2 & 3 (Header creation & implementation split)
