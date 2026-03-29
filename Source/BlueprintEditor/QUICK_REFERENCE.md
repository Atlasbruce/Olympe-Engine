# 🗺️ QUICK REFERENCE CARD - Developer's Cheat Sheet

## Print This! 📋

---

## ✅ WHERE DOES METHOD X GO?

### Use This Table

| If method name contains... | Goes to File | LOC |
|--------------------------|--------------|-----|
| Canvas, ImNodes, Render, Position, Sync (visual) | **_Canvas.cpp** | 800 |
| Connect, Link, Pin, Connection | **_Connections.cpp** | 600 |
| RenderToolbar, RenderPalette, RenderCanvas, UI | **_Rendering.cpp** | 900 |
| RenderProperties, RenderBranch, RenderMathOp, Condition | **_NodeProperties.cpp** | 1200 |
| RenderBlackboard, Variable, Blackboard, GetVariablesByType | **_Blackboard.cpp** | 700 |
| RenderVerification, RunVerification, Trace, Execute | **_Verification.cpp** | 600 |
| Save, Load, Serialize, File | **_FileOps.cpp** | 800 |
| AddNode, RemoveNode, Undo, Redo, Drag | **_Interaction.cpp** | 700 |
| Preset, Operand, Bank, NodeBranchRenderer | **_Presets.cpp** | 500 |
| Constructor, Destructor, Initialize, Shutdown, Render (dispatcher) | **Core.cpp** | 400 |

---

## 🔍 STATE MEMBERS BY DOMAIN

### Canvas (9 members)
```cpp
ImNodesEditorContext* m_imnodesContext;
std::vector<VSEditorNode> m_editorNodes;
std::vector<VSEditorLink> m_editorLinks;
std::unordered_set<int> m_positionedNodes;
int m_nextNodeID, m_nextLinkID;
bool m_needsPositionSync, m_skipPositionSyncNextFrame;
bool m_justPerformedUndoRedo;
```

### File Operations (8 members)
```cpp
std::string m_currentPath;
bool m_dirty;
bool m_showSaveAsDialog;
char m_saveAsFilename[256];
std::string m_saveAsDirectory, m_saveAsExtension;
Vector m_lastViewportPanning;
bool m_viewportResetDone;
```

### Properties (14 members)
```cpp
int m_selectedNodeID, m_propEditNodeIDOnFocus;
std::string m_propEditOldName, m_propEditOldTaskID, ...;
float m_propEditOldDelay;
std::string m_propEditSwitchVar;
std::vector<SwitchCaseDefinition> m_propEditSwitchCases;
std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
// ... other panels ...
```

### Blackboard (5 members)
```cpp
std::unordered_map<int, std::string> m_pendingBlackboardEdits;
std::unique_ptr<EntityBlackboard> m_entityBlackboard;
int m_blackboardTabSelection;
float m_verificationLogsPanelHeight;
```

### Interaction (12 members)
```cpp
bool m_pendingAddPin, m_pendingRemovePin;
int m_pendingAddPinNodeID, m_pendingRemovePinNodeID;
int m_pendingRemovePinDynIdx;
bool m_pendingNodeDrop;
TaskNodeType m_pendingNodeType;
float m_pendingNodeX, m_pendingNodeY;
std::unordered_map<int, std::pair<float, float>> m_nodeDragStartPositions;
UndoRedoStack m_undoStack;
```

### Verification (6 members)
```cpp
std::vector<std::string> m_validationWarnings, m_validationErrors;
VSVerificationResult m_verificationResult;
bool m_verificationDone;
std::vector<std::string> m_verificationLogs;
int m_focusNodeID;
```

### Presets (14 members)
```cpp
ConditionPresetRegistry m_presetRegistry;
std::unique_ptr<DynamicDataPinManager> m_pinManager;
std::unique_ptr<NodeBranchRenderer> m_branchRenderer;
std::unique_ptr<NodeConditionsPanel> m_conditionsPanel;
// ... 10+ more panel members ...
```

---

## 🏗️ FILE STRUCTURE TEMPLATE

### For Any New .cpp File

```cpp
/**
 * @file VisualScriptEditorPanel_DOMAIN.cpp
 * @brief [Domain] functionality for VS editor panel
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * Part of the VisualScriptEditorPanel refactoring (Phase 1).
 * This file contains all [Domain] related methods.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
// ... other domain-specific includes ...

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

#include <algorithm>
#include <vector>
// ... other std includes ...

namespace Olympe {

// ============================================================================
// [DOMAIN] functionality
// ============================================================================

// Method implementations here
// ...

} // namespace Olympe
```

---

## 📐 INCLUDE STRATEGY

### Every _*.cpp File Should Have

```cpp
#include "VisualScriptEditorPanel.h"  // Always first!
```

### Add Domain-Specific Includes

```cpp
// If using registries
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"

// If using validators
#include "VSConnectionValidator.h"
#include "VSGraphVerifier.h"

// If using UI panels
#include "../Editor/Panels/NodeConditionsPanel.h"

// Always needed for rendering
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

// Standard library
#include <algorithm>
#include <vector>
#include <string>
```

### Never Add

```cpp
❌ #include "VisualScriptEditorPanel_Canvas.h"  (no separate headers!)
❌ #include "VisualScriptEditorPanel_*.h"      (no separate headers!)
```

---

## 🔗 METHOD DEPENDENCY MAP

### Know What to Include Based on Method Type

```
CANVAS METHODS:
├─ Need: ImNodes, VSEditorNode, VSEditorLink
└─ Include: imnodes.h, VisualScriptEditorPanel.h

CONNECTION METHODS:
├─ Need: VSConnectionValidator, ExecPinConnection
└─ Include: VSConnectionValidator.h

RENDERING METHODS:
├─ Need: ImGui, task definitions
└─ Include: imgui.h, ImGui context

PROPERTY METHODS:
├─ Need: Panel classes, registries
└─ Include: NodeConditionsPanel.h, ConditionRegistry.h

BLACKBOARD METHODS:
├─ Need: EntityBlackboard, GlobalTemplateBlackboard
└─ Include: EntityBlackboard.h, GlobalTemplateBlackboard.h

FILE OPS METHODS:
├─ Need: JSON serialization, TaskGraphLoader
└─ Include: json_helper.h, TaskGraphLoader.h

INTERACTION METHODS:
├─ Need: UndoRedoStack, Commands
└─ Include: UndoRedoStack.h
```

---

## 🎯 DOMAIN QUICK REFERENCE

### Canvas
**What**: ImNodes rendering, position sync  
**Files**: _Canvas.cpp (800 LOC)  
**Key Classes**: VSEditorNode, VSEditorLink  
**Key Methods**: RenderCanvas, SyncCanvasFromTemplate, AllocNodeID

### Connections
**What**: Link creation/deletion, pin management  
**Files**: _Connections.cpp (600 LOC)  
**Key Classes**: ExecPinConnection, DataPinConnection  
**Key Methods**: ConnectExec, ConnectData, RemoveLink

### Rendering
**What**: Main UI rendering (toolbar, palette, menus)  
**Files**: _Rendering.cpp (900 LOC)  
**Key Classes**: ImGui  
**Key Methods**: RenderToolbar, RenderNodePalette, RenderContextMenus

### Node Properties
**What**: Properties panel & condition editor  
**Files**: _NodeProperties.cpp (1200 LOC)  
**Key Classes**: NodeConditionsPanel, MathOpPropertyPanel  
**Key Methods**: RenderProperties, RenderConditionEditor

### Blackboard
**What**: Local & global variable management  
**Files**: _Blackboard.cpp (700 LOC)  
**Key Classes**: EntityBlackboard, GlobalTemplateBlackboard  
**Key Methods**: RenderBlackboard, CommitPendingBlackboardEdits

### Verification
**What**: Graph validation & Phase 24.3 execution testing  
**Files**: _Verification.cpp (600 LOC)  
**Key Classes**: VSGraphVerifier, GraphExecutionSimulator  
**Key Methods**: RunVerification, RunExecutionTest

### File Operations
**What**: Save/load/serialize graphs  
**Files**: _FileOps.cpp (800 LOC)  
**Key Classes**: TaskGraphLoader, JSON serialization  
**Key Methods**: Save, SaveAs, SerializeAndWrite

### Interaction
**What**: Node manipulation, undo/redo, drag & drop  
**Files**: _Interaction.cpp (700 LOC)  
**Key Classes**: UndoRedoStack, AddNodeCommand  
**Key Methods**: AddNode, RemoveNode, PerformUndo

### Presets
**What**: Phase 24 condition presets management  
**Files**: _Presets.cpp (500 LOC)  
**Key Classes**: ConditionPresetRegistry, NodeBranchRenderer  
**Key Methods**: RenderPresetBankPanel, RenderOperandEditor

---

## 🔄 PHASE CHECKLIST

### Phase 2: Creating Headers (if applicable)
- [ ] Create all 9 .h files (NOTE: Actually merged into single class .h)
- [ ] Add method declarations
- [ ] Add forward declarations
- [ ] Verify no circular includes

### Phase 3: Creating Implementations
- [ ] Create _Canvas.cpp
- [ ] Create _Connections.cpp
- [ ] Create _Rendering.cpp
- [ ] Create _NodeProperties.cpp
- [ ] Create _Blackboard.cpp
- [ ] Create _Verification.cpp
- [ ] Create _FileOps.cpp
- [ ] Create _Interaction.cpp
- [ ] Create _Presets.cpp
- [ ] Create Core.cpp (from main file stub)
- [ ] Verify all 96 methods present

### Phase 4: Testing
- [ ] CMakeLists.txt updated
- [ ] Full build succeeds (0 errors, 0 warnings)
- [ ] All tests pass
- [ ] No regressions

---

## 🚨 COMMON MISTAKES TO AVOID

### ❌ DON'T
```cpp
// ❌ Don't include specialty headers
#include "VisualScriptEditorPanel_Canvas.h"

// ❌ Don't modify the main class structure
// (Keep class definition in VisualScriptEditorPanel.h)

// ❌ Don't change method signatures
// (Only move implementations)

// ❌ Don't forget #include "VisualScriptEditorPanel.h"
// (Must be first include in every .cpp)

// ❌ Don't add circular includes
#include "VisualScriptEditorPanel_Canvas.h"  // Wrong!
```

### ✅ DO
```cpp
// ✅ Do include VisualScriptEditorPanel.h first
#include "VisualScriptEditorPanel.h"

// ✅ Do include domain-specific headers
#include "VSConnectionValidator.h"

// ✅ Do preserve all method implementations exactly
// (Copy-paste is OK for this refactoring!)

// ✅ Do follow the namespace
namespace Olympe { ... }

// ✅ Do add file header comments
/**
 * @file VisualScriptEditorPanel_Canvas.cpp
 * ...
 */
```

---

## 📞 QUICK LOOKUP TABLE

| I Need To... | File | Method |
|-------------|------|--------|
| Add new rendering code | _Rendering.cpp | RenderXXX |
| Fix canvas sync issue | _Canvas.cpp | SyncCanvasFromTemplate |
| Debug connection problem | _Connections.cpp | ConnectExec / ConnectData |
| Fix properties panel | _NodeProperties.cpp | RenderProperties |
| Fix variable issue | _Blackboard.cpp | RenderBlackboard |
| Fix verification | _Verification.cpp | RunVerification |
| Fix save/load | _FileOps.cpp | Save / SerializeAndWrite |
| Fix node creation | _Interaction.cpp | AddNode |
| Fix Phase 24 presets | _Presets.cpp | RenderPresetBankPanel |
| Fix initialization | Core.cpp | Initialize |

---

## 💡 TIPS & TRICKS

### Tip 1: Use FUNCTION_MAPPING.md for Reference
Keep `FUNCTION_MAPPING.md` open while coding. It's your method-to-file bible.

### Tip 2: Search Patterns
```bash
# Find all canvas-related methods in original file
grep -n "RenderCanvas\|SyncCanvas\|AllocNode" VisualScriptEditorPanel.cpp

# Find methods by domain
grep -n "Verify\|Validate" VisualScriptEditorPanel.cpp  # → _Verification.cpp
```

### Tip 3: State Member Tracking
Don't move state members! They stay in the main class header.
Only move method implementations.

### Tip 4: Include Minimization
You can remove includes that your specific file doesn't use.
Example: _Canvas.cpp might not need ConditionRegistry.h

### Tip 5: Testing Incrementally
After creating each .cpp file, run: `cmake --build . --config Release`
Catch issues early before all files are done.

---

## 📊 FILE SIZE REFERENCE

```
Target sizes (guideline, not hard limit):
├─ _Canvas.cpp           ~800 LOC
├─ _Connections.cpp      ~600 LOC  
├─ _Rendering.cpp        ~900 LOC
├─ _NodeProperties.cpp  ~1200 LOC  (largest - that's OK!)
├─ _Blackboard.cpp       ~700 LOC
├─ _Verification.cpp     ~600 LOC
├─ _FileOps.cpp          ~800 LOC
├─ _Interaction.cpp      ~700 LOC
├─ _Presets.cpp          ~500 LOC
└─ Core.cpp              ~400 LOC
   ─────────────────────────────
   TOTAL              ~6,200 LOC  ✅
```

---

## 🎓 LEARNING RESOURCES

1. **Understanding the Structure**
   → Read `REFACTORING_STRATEGY.md` section "File Architecture"

2. **Locating Methods**
   → Use `FUNCTION_MAPPING.md` as your reference

3. **Detailed Method Info**
   → Find method in `FUNCTION_MAPPING.md`
   → Check target file
   → Look up in original `VisualScriptEditorPanel.cpp`

4. **State Members**
   → See "STATE MEMBERS BY DOMAIN" above (this page)
   → Or check `FUNCTION_MAPPING.md` "State Members (_XYZ)" sections

5. **Include Dependencies**
   → See "INCLUDE STRATEGY" above
   → Cross-reference with `REFACTORING_STRATEGY.md` "Include Strategy"

---

**Print & Keep Handy! 📋**

---

**Document Version**: 1.0  
**Last Updated**: 2026-03-09  
**Best For**: Phase 2-3 Implementation Work
