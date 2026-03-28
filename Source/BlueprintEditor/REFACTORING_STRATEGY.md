# 🏗️ REFACTORING STRATEGY: VisualScriptEditorPanel Modularization

## 📋 Table of Contents
1. [Overview](#overview)
2. [Refactoring Phases](#refactoring-phases)
3. [File Architecture](#file-architecture)
4. [Implementation Details](#implementation-details)
5. [Validation Strategy](#validation-strategy)
6. [Maintenance Guidelines](#maintenance-guidelines)

---

## Overview

### Current State
- **Single File**: `VisualScriptEditorPanel.cpp` (~6200 LOC, ~250 KB)
- **Single Class**: `VisualScriptEditorPanel` (~150+ methods)
- **Issues**:
  - Difficult to navigate and maintain
  - Long compilation times
  - Discourages parallel development
  - Mixes concerns: rendering, storage, UI logic, validation

### Target State
- **9 Specialized Files**: Each focused on a distinct responsibility
- **Same Class Interface**: External API unchanged, fully backward compatible
- **Improved**:
  - ✅ Maintainability & readability
  - ✅ Compilation speed (incremental builds)
  - ✅ Parallel development capability
  - ✅ Unit testing per domain
  - ✅ Future extensibility (Phase 24.3 Execution Testing)

### File Size Distribution
```
Original:  VisualScriptEditorPanel.cpp    6,200 LOC
                                          ↓
Split to:
├─ _Canvas.cpp              ~800 LOC   (ImNodes & canvas sync)
├─ _Connections.cpp         ~600 LOC   (Link & pin management)
├─ _Rendering.cpp           ~900 LOC   (Main UI rendering)
├─ _NodeProperties.cpp    ~1,200 LOC   (Properties panel & conditions)
├─ _Blackboard.cpp          ~700 LOC   (Variables local/global)
├─ _Verification.cpp        ~600 LOC   (Validation & verification)
├─ _FileOps.cpp             ~800 LOC   (Save/load & serialization)
├─ _Interaction.cpp         ~700 LOC   (Node manipulation & undo/redo)
├─ _Presets.cpp             ~500 LOC   (Phase 24 Condition Presets)
└─ Core.cpp (from main)     ~400 LOC   (Constructor, Init, Shutdown)
```

---

## Refactoring Phases

### Phase 1: Preparation & Analysis ✅
**Duration**: ~30 min  
**Deliverables**:
- ✅ Architecture documentation (this file)
- ✅ Function-to-file mapping
- ✅ State-to-file mapping
- ✅ Dependency graph

**Status**: COMPLETE

---

### Phase 2: Header Files Creation 🔄
**Duration**: ~2 hours  
**Deliverables**:
- [ ] Create 9 specialized `.h` files with method declarations
- [ ] Add `#pragma once` guards and includes
- [ ] Organize private methods by domain
- [ ] Document method groupings with doxygen comments

**Process**:
1. Create header skeleton for each domain
2. Declare all related methods
3. Add forward declarations and necessary includes
4. Validate header syntax (no compilation yet)

**Key Files**:
- `VisualScriptEditorPanel_Canvas.h`
- `VisualScriptEditorPanel_Connections.h`
- `VisualScriptEditorPanel_Rendering.h`
- `VisualScriptEditorPanel_NodeProperties.h`
- `VisualScriptEditorPanel_Blackboard.h`
- `VisualScriptEditorPanel_Verification.h`
- `VisualScriptEditorPanel_FileOps.h`
- `VisualScriptEditorPanel_Interaction.h`
- `VisualScriptEditorPanel_Presets.h`

---

### Phase 3: Implementation Split 🔄
**Duration**: ~4 hours  
**Deliverables**:
- [ ] Migrate implementations from `.cpp` to specialized `_*.cpp` files
- [ ] Preserve all method bodies exactly as-is
- [ ] Add proper `#include` guards in each file
- [ ] Maintain identical functionality

**Process**:
1. Extract methods from main `.cpp` by domain
2. Create corresponding `_*.cpp` files
3. Copy implementations line-for-line (no logic changes)
4. Add necessary `#include` statements

**Validation**:
- No behavioral changes
- All methods preserved
- Comments and formatting intact

---

### Phase 4: Compilation & Testing 🔄
**Duration**: ~1.5 hours  
**Deliverables**:
- [ ] All files compile without errors
- [ ] All existing tests pass
- [ ] No runtime behavior changes
- [ ] Verify inclusion in build system

**Process**:
1. Update `CMakeLists.txt` to include new `.cpp` files
2. Run full build
3. Fix any compilation errors
4. Run unit tests
5. Run integration tests

**Success Criteria**:
```
✅ Build: SUCCESS (0 errors, 0 warnings)
✅ Tests: 100% PASS
✅ Runtime: Identical behavior
```

---

### Phase 5: Documentation & Handoff 🔄
**Duration**: ~1 hour  
**Deliverables**:
- [ ] Architecture guide (this file - maintained)
- [ ] File organization diagram
- [ ] Method index by domain
- [ ] Troubleshooting guide
- [ ] Future enhancement roadmap

**Process**:
1. Create visual dependency graph
2. Document public vs private interfaces
3. Add maintenance guidelines
4. Update README if needed

---

## File Architecture

### Domain Mapping

#### 1. **VisualScriptEditorPanel_Canvas** 📐
**Responsibility**: ImNodes context and canvas synchronization

**Key Methods**:
```cpp
// ImNodes context management
void Initialize();              // Creates m_imnodesContext
void Shutdown();                // Destroys m_imnodesContext

// Canvas rendering
void RenderCanvas();            // Main canvas loop

// Synchronization (template ↔ canvas)
void SyncCanvasFromTemplate();           // Template → editor visual
void SyncTemplateFromCanvas();           // Editor visual → template
void SyncEditorNodesFromTemplate();      // Rebuild editor nodes after undo/redo
void SyncNodePositionsFromImNodes();     // Pull positions from ImNodes

// Utilities
int  AllocNodeID();
int  AllocLinkID();
int  ExecInAttrUID(int nodeID) const;
int  ExecOutAttrUID(int nodeID, int pinIndex) const;
int  DataInAttrUID(int nodeID, int pinIndex) const;
int  DataOutAttrUID(int nodeID, int pinIndex) const;
ImVec2 ScreenToCanvasPos(ImVec2 screenPos) const;
```

**State Members**:
```cpp
ImNodesEditorContext* m_imnodesContext;
std::vector<VSEditorNode> m_editorNodes;
std::vector<VSEditorLink> m_editorLinks;
std::unordered_set<int> m_positionedNodes;
int m_nextNodeID;
int m_nextLinkID;
bool m_needsPositionSync;
bool m_skipPositionSyncNextFrame;
bool m_justPerformedUndoRedo;
// ... viewport state
```

**Dependencies**:
- ImNodes library
- VSEditorNode, VSEditorLink structures
- TaskGraphTemplate

---

#### 2. **VisualScriptEditorPanel_Connections** 🔌
**Responsibility**: Link creation/deletion and pin management

**Key Methods**:
```cpp
// Link management
void ConnectExec(int srcNodeID, const std::string& srcPinName,
                 int dstNodeID, const std::string& dstPinName);
void ConnectData(int srcNodeID, const std::string& srcPinName,
                 int dstNodeID, const std::string& dstPinName);
void RemoveLink(int linkID);
void RebuildLinks();

// Pin queries
static std::vector<std::string> GetExecOutputPins(TaskNodeType type);
static std::vector<std::string> GetExecInputPins(TaskNodeType type);
static std::vector<std::string> GetDataInputPins(TaskNodeType type);
static std::vector<std::string> GetDataOutputPins(TaskNodeType type);
std::vector<std::string> GetExecOutputPinsForNode(const TaskNodeDefinition& def) const;
```

**State Members**:
```cpp
std::vector<VSEditorLink> m_editorLinks;
int m_contextLinkID;
// ... shared canvas state
```

**Dependencies**:
- VSConnectionValidator
- TaskNodeDefinition, ExecPinConnection, DataPinConnection
- UndoRedoStack (for commands)

---

#### 3. **VisualScriptEditorPanel_Rendering** 🎨
**Responsibility**: Main UI rendering infrastructure

**Key Methods**:
```cpp
void Render();                      // Full panel window
void RenderContent();               // Panel content (no window)
void RenderToolbar();               // Top toolbar
void RenderNodePalette();           // Node creation palette
void RenderContextMenus();          // Right-click menus
void RenderValidationOverlay();     // Validation messages overlay
```

**State Members**:
```cpp
bool m_visible;
bool m_paletteOpen;
float m_contextMenuX;
float m_contextMenuY;
int m_contextNodeID;
// ... dialog state
```

**Dependencies**:
- ImGui library
- ImNodes
- Node rendering components

---

#### 4. **VisualScriptEditorPanel_NodeProperties** 🏗️
**Responsibility**: Properties panel and node-specific UI

**Key Methods**:
```cpp
// Main properties panel
void RenderProperties();                // Dispatcher
void RenderNodePropertiesPanel();       // Part A: Node props

// Specialized renderers per node type
void RenderBranchNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);
void RenderMathOpNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);
void RenderNodeDataParameters(TaskNodeDefinition& def);

// Phase 23-B.4 Condition UI
void RenderConditionEditor(Condition& condition, int conditionIndex,
                          const std::vector<BlackboardEntry>& allVars,
                          const std::vector<std::string>& availablePins);
void RenderVariableSelector(std::string& selectedVar,
                           const std::vector<BlackboardEntry>& allVars,
                           VariableType expectedType, const char* label);
void RenderConstValueInput(TaskValue& value, VariableType varType, const char* label);
void RenderPinSelector(std::string& selectedPin,
                      const std::vector<std::string>& availablePins, const char* label);
static std::string BuildConditionPreview(const Condition& cond);
```

**State Members**:
```cpp
int m_selectedNodeID;
int m_propEditNodeIDOnFocus;
std::string m_propEditOldName;
std::string m_propEditOldTaskID;
std::string m_propEditOldConditionID;
std::string m_propEditOldBBKey;
std::string m_propEditOldMathOp;
std::string m_propEditOldSubGraphPath;
float m_propEditOldDelay;
// ... panel sub-widgets
std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
std::unique_ptr<MathOpPropertyPanel> m_mathOpPanel;
// ... etc
float m_nodePropertiesPanelHeight;
```

**Dependencies**:
- NodeConditionsPanel (Phase 24)
- MathOpPropertyPanel
- GetBBValuePropertyPanel, SetBBValuePropertyPanel, VariablePropertyPanel
- ConditionRegistry, OperatorRegistry

---

#### 5. **VisualScriptEditorPanel_Blackboard** 🔤
**Responsibility**: Local and global variable management

**Key Methods**:
```cpp
// Rendering
void RenderBlackboard();                // Main blackboard panel
void RenderLocalVariablesPanel();       // Local vars tab (Phase 24)
void RenderGlobalVariablesPanel();      // Global vars tab (Phase 24)

// Data maintenance
void CommitPendingBlackboardEdits();        // Flush pending edits (BUG-002)
void ValidateAndCleanBlackboardEntries();   // Remove invalid entries (Fix #1)

// Utilities
static std::vector<BlackboardEntry> GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType);
```

**State Members**:
```cpp
std::unordered_map<int, std::string> m_pendingBlackboardEdits;
std::unique_ptr<EntityBlackboard> m_entityBlackboard;
int m_blackboardTabSelection;  // 0 = Local, 1 = Global
float m_verificationLogsPanelHeight;
```

**Dependencies**:
- LocalBlackboard, EntityBlackboard (Phase 24)
- GlobalTemplateBlackboard
- BBVariableRegistry

---

#### 6. **VisualScriptEditorPanel_Verification** ✅
**Responsibility**: Validation and graph verification

**Key Methods**:
```cpp
// Verification
void RunVerification();                  // Execute VSGraphVerifier
void RenderValidationOverlay();          // Show validation results
void RenderVerificationPanel();          // Phase 21-B verification panel
void RenderVerificationLogsPanel();      // Phase 24.3 execution test logs

// Phase 24.3 Execution Testing
std::vector<ValidationError> RunExecutionTest();
ExecutionTestPanel& GetExecutionTestPanel();
```

**State Members**:
```cpp
VSVerificationResult m_verificationResult;
bool m_verificationDone;
std::vector<std::string> m_verificationLogs;
int m_focusNodeID;
// Phase 24.3
std::unique_ptr<ExecutionTestPanel> m_executionTestPanel;
```

**Dependencies**:
- VSGraphVerifier
- GraphExecutionSimulator (Phase 24.3)
- GraphExecutionTracer (Phase 24.3)
- ExecutionTestPanel (Phase 24.3)

---

#### 7. **VisualScriptEditorPanel_FileOps** 💾
**Responsibility**: File I/O and serialization

**Key Methods**:
```cpp
// Graph management
void LoadTemplate(const TaskGraphTemplate* tmpl, const std::string& path);
bool Save();
bool SaveAs(const std::string& path);
const std::string& GetCurrentPath() const;
bool IsDirty() const;

// Serialization
bool SerializeAndWrite(const std::string& path);
void SyncPresetsFromRegistryToTemplate();    // Phase 24 presets sync

// Viewport management (BUG-003)
void ResetViewportBeforeSave();
void AfterSave();

// UI
void RenderSaveAsDialog();
```

**State Members**:
```cpp
std::string m_currentPath;
bool m_dirty;
bool m_showSaveAsDialog;
char m_saveAsFilename[256];
std::string m_saveAsDirectory;
std::string m_saveAsExtension;
Vector m_lastViewportPanning;
bool m_viewportResetDone;
```

**Dependencies**:
- TaskGraphTemplate, TaskGraphLoader
- JSON serialization (json_helper.h)
- ConditionPresetRegistry

---

#### 8. **VisualScriptEditorPanel_Interaction** 🎮
**Responsibility**: Node manipulation and undo/redo

**Key Methods**:
```cpp
// Node operations
int AddNode(TaskNodeType type, float x, float y);
void RemoveNode(int nodeID);

// Undo/Redo
void PerformUndo();
void PerformRedo();

// Drag & Drop state
bool m_pendingNodeDrop;
TaskNodeType m_pendingNodeType;
float m_pendingNodeX, m_pendingNodeY;

// Dynamic pins
bool m_pendingAddPin;
int m_pendingAddPinNodeID;
bool m_pendingRemovePin;
int m_pendingRemovePinNodeID;
int m_pendingRemovePinDynIdx;

// Drag tracking
std::unordered_map<int, std::pair<float, float>> m_nodeDragStartPositions;
```

**Dependencies**:
- UndoRedoStack
- Undo/Redo Commands (AddNodeCommand, DeleteNodeCommand, MoveNodeCommand, etc.)
- TaskNodeType, TaskNodeDefinition

---

#### 9. **VisualScriptEditorPanel_Presets** 🔧
**Responsibility**: Condition Presets management (Phase 24)

**Key Methods**:
```cpp
// Rendering
void RenderPresetBankPanel();           // Part B: Preset bank
void RenderPresetItemCompact(const ConditionPreset& preset, size_t index);
bool RenderOperandEditor(Operand& operand, const char* labelSuffix);
```

**State Members**:
```cpp
ConditionPresetRegistry m_presetRegistry;
std::unique_ptr<DynamicDataPinManager> m_pinManager;
std::unique_ptr<NodeBranchRenderer> m_branchRenderer;
std::unique_ptr<ConditionPresetLibraryPanel> m_libraryPanel;
int m_condPanelNodeID;
float m_presetBankPanelHeight;
```

**Dependencies**:
- ConditionPresetRegistry
- DynamicDataPinManager
- NodeBranchRenderer
- ConditionPresetLibraryPanel

---

## Implementation Details

### Include Strategy

#### Main Header (VisualScriptEditorPanel.h)
```cpp
#pragma once

// Core dependencies
#include "../third_party/imgui/imgui.h"
#include "../TaskSystem/TaskGraphTemplate.h"
#include "../TaskSystem/LocalBlackboard.h"
// ... (all current includes)

class VisualScriptEditorPanel {
public:
    // Public API (unchanged)
    void Initialize();
    void Render();
    void LoadTemplate(const TaskGraphTemplate* tmpl, const std::string& path);

    // Phase 24.3 Execution Testing
    std::vector<ValidationError> RunExecutionTest();
    ExecutionTestPanel& GetExecutionTestPanel();

private:
    // All state members (unchanged from current)
    // ... methods split into _*.cpp files
};
```

#### Specialized Headers (e.g., VisualScriptEditorPanel_Canvas.h)
```cpp
#pragma once

// No separate header files needed!
// All implementations go directly into _*.cpp files
// The main class definition in VisualScriptEditorPanel.h remains the source of truth
// This prevents header duplication and circular dependencies

// Each _*.cpp file includes:
#include "VisualScriptEditorPanel.h"
// ... domain-specific includes
```

### File Inclusion Pattern

**VisualScriptEditorPanel.cpp** (Core):
```cpp
#include "VisualScriptEditorPanel.h"
// All core includes + dispatcher methods

// Include specialized implementations
#include "VisualScriptEditorPanel_Canvas.cpp"       // Not ideal, but simplest
// OR use build system with multiple .cpp files
```

**Better approach: Multiple .cpp files in build system**:
```cmake
add_library(VisualScriptEditor
    VisualScriptEditorPanel.cpp              # Constructor, Init, Shutdown, Render dispatcher
    VisualScriptEditorPanel_Canvas.cpp       # Canvas methods
    VisualScriptEditorPanel_Connections.cpp  # Connection methods
    # ... etc
)
```

---

## Validation Strategy

### Phase 4 Compilation Checklist

```
□ 1. Header Syntax Check
    - Run: clang -fsyntax-only *.h
    - Expected: 0 errors

□ 2. Individual File Compilation
    - Compile each _*.cpp separately
    - Check for missing includes
    - Verify no circular dependencies

□ 3. Full Build
    - cmake --build . --config Release
    - Expected: 0 errors, 0 warnings

□ 4. Link Check
    - Verify all symbols resolved
    - Check for ODR violations

□ 5. Runtime Tests
    - Execute existing unit tests
    - Run GUI tests (if available)
    - Verify no behavioral changes
```

### Success Metrics

```cpp
✅ Compilation
   - Build time: < 5 min (full)
   - Build time: < 30 sec (incremental)
   - Warnings: 0

✅ Functionality
   - All tests pass: 100%
   - No regression bugs
   - Performance: unchanged

✅ Code Quality
   - No code duplication
   - All methods accounted for
   - Comments preserved
```

---

## Maintenance Guidelines

### Adding New Features

1. **Identify the domain** (Canvas, Connections, NodeProperties, etc.)
2. **Add method to main header** (VisualScriptEditorPanel.h)
3. **Implement in corresponding _*.cpp** file
4. **Recompile only that file** (faster iteration)

### Renaming/Moving Methods

1. Update declaration in main header
2. Update implementation in corresponding _*.cpp
3. Update any cross-file calls
4. Verify compilation

### Debugging Issues

1. Check which domain is affected
2. Open the corresponding `_*.cpp` file
3. ~100-1200 LOC to search through (vs 6200)
4. Use `find_symbol` tool for cross-references

### Phase 24.3 Integration (Execution Testing)

Files to add:
```
Source/BlueprintEditor/
├── GraphExecutionTracer.h
├── GraphExecutionTracer.cpp
├── GraphExecutionSimulator.h
├── GraphExecutionSimulator.cpp
├── ExecutionTestPanel.h
├── ExecutionTestPanel.cpp
└── VisualScriptEditorPanel_ExecutionTest.cpp  (← calls from this file go here)
```

Integration points:
- `VisualScriptEditorPanel_Verification.cpp` ← Add Phase 24.3 methods
- `VisualScriptEditorPanel.h` ← Add m_executionTestPanel state member
- `VisualScriptEditorPanel_Rendering.cpp` ← Render "Test Execution" button

---

## Dependency Graph

```
┌──────────────────────────────────────────┐
│  VisualScriptEditorPanel.h               │
│  (Main class definition + public API)    │
└──────────────────────────────────────────┘
           ▲              ▲
           │              │
        includes         includes
           │              │
    ┌──────┴──────────────┴──────┐
    │                            │
    │ _Canvas.cpp           _Connections.cpp
    │  (ImNodes sync)       (Link management)
    │      ▲                      ▲
    │      │                      │
    └──────┼──────┬───────────────┼──────────┐
           │      │               │          │
    _Rendering.cpp   │    _NodeProperties.cpp
    (Main UI)        │    (Properties panel)
           │         │               │
           └────┬────┴───────────────┘
                │
         _Blackboard.cpp      _FileOps.cpp
         (Variables)          (Save/Load)
                │                 │
                └────────┬────────┘
                         │
                _Interaction.cpp    _Presets.cpp
                (Undo/Redo)        (Phase 24)
                         │              │
                         └──────┬───────┘
                                │
                    _Verification.cpp
                   (Validation + Phase 24.3)
```

---

## Future Work

### Phase 24.3 Integration
- [ ] Create GraphExecutionTracer.h/cpp
- [ ] Create GraphExecutionSimulator.h/cpp
- [ ] Create ExecutionTestPanel.h/cpp
- [ ] Add VisualScriptEditorPanel_ExecutionTest.cpp
- [ ] Integrate execution test button in toolbar
- [ ] Render test results in verification panel

### Additional Optimizations
- [ ] Profile compilation time before/after
- [ ] Identify further splitting opportunities
- [ ] Consider template specialization for performance-critical paths
- [ ] Add code generation for pin management helpers

---

## References

- Phase 24: Condition Preset serialization
- Phase 24.3: Execution Testing & Simulation
- Phase 21-B: Graph Verification
- Phase 23-B.4: Condition Editor
- BUG-002: Blackboard edit commit
- BUG-003: Viewport panning during save

---

**Document Version**: 1.0  
**Last Updated**: 2026-03-09  
**Status**: Ready for Phase 2 Implementation
