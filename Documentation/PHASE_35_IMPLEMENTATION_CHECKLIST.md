# 📋 PHASE 35 - IMPLEMENTATION CHECKLIST

**Project**: Olympe Engine - BehaviorTree Editor Reintegration
**Date**: 08-04-2026
**Status**: PLANNING COMPLETE - READY FOR IMPLEMENTATION
**Effort**: 12-19 days (MVP: 7-9 days)

---

## 📊 Planning Phase Deliverables

### ✅ Documentation Complete
- [x] Rapport d'analyse complet (existing: RAPPORT COMPLET D'ANALYSE - BEHAVIOUR TREE SYSTEM)
- [x] Executive Summary created: `PHASE_35_EXECUTIVE_SUMMARY.md`
- [x] Full Reintegration Plan: `PHASE_35_BT_EDITOR_REINTEGRATION_PLAN.md`
- [x] Architecture Decisions: `PHASE_35_ARCHITECTURE_DECISIONS.md`
- [x] Quick Start Guide: `PHASE_35_QUICK_START_GUIDE.md`
- [x] Copilot Instructions updated: `.github/copilot-instructions.md`

### ✅ Architecture Approved
- [x] Decision 1: Shared NodeGraphPanel (CHOSEN)
- [x] Decision 2: Dedicated PropertyPanel_BT class (CHOSEN)
- [x] Decision 3: Graph type detection (existing, no change)
- [x] Decision 4: JSON v2 serialization schema defined
- [x] Decision 5: Full CommandStack undo/redo (APPROVED)
- [x] Decision 6: Split panel layout (APPROVED)
- [x] Decision 7: Validation with error display (APPROVED)
- [x] Decision 8: Unit + integration tests (APPROVED)

### ✅ Implementation Plan Created
- [x] Phase 1: Canvas & TabManager (3-5 days) - 3 steps
- [x] Phase 2: Node Palette (2-3 days) - 3 steps
- [x] Phase 3: Property Panel & Serialization (3-4 days) - 4 steps
- [x] Phase 4: Undo/Redo & Validation (2-3 days) - 3 steps
- [x] Phase 5: Testing & Polish (2-3 days) - 3 steps
- [x] Total: 12-19 days breakdown provided

### ✅ Files Identified
- [x] 9 files to MODIFY identified
- [x] 3 files to CREATE identified
- [x] 2 test files to CREATE identified
- [x] Exact line numbers provided where applicable

---

## 🎯 Phase 1: Canvas & TabManager Integration

**Objective**: Get BT canvas appearing in tabs
**Estimated**: 3-5 days
**Go/No-Go Point**: After this, BT canvas should render

### Step 1.1: TabManager::CreateNewTab() - BehaviorTree Case
- [ ] File: `Source\BlueprintEditor\TabManager.cpp`
- [ ] Location: Line 179 (after EntityPrefab case, before closing else)
- [ ] Change: Add 11-line code block for BT instantiation
- [ ] Test: Compiles, no errors
- [ ] Verify: BehaviorTreeRenderer created, NodeGraphPanel initialized

### Step 1.2: TabManager::OpenFileInTab() - BehaviorTree Loading
- [ ] File: `Source\BlueprintEditor\TabManager.cpp`
- [ ] Location: OpenFileInTab() function
- [ ] Change: Ensure BehaviorTree detection loads correctly
- [ ] Test: Can open existing .json BT file
- [ ] Verify: File path stored, graph ID assigned

### Step 1.3: BlueprintEditorGUI - Menu Integration
- [ ] File: `Source\BlueprintEditor\BlueprintEditorGUI.cpp`
- [ ] Location: RenderMenuBar() function
- [ ] Change: Add menu item "File → New → Behavior Tree"
- [ ] Add: Keyboard shortcut Ctrl+Alt+B
- [ ] Test: Menu clickable, creates new tab
- [ ] Verify: Tab appears with BehaviorTree canvas

### Step 1.4: Keyboard Shortcuts
- [ ] File: `Source\BlueprintEditor\BlueprintEditorGUI.cpp`
- [ ] Location: HandleKeyboardShortcuts() function
- [ ] Change: Add Ctrl+Alt+B handler
- [ ] Test: Shortcut works in editor
- [ ] Verify: Same result as menu item

**Phase 1 Completion Criteria**:
- ✅ Menu item "File → New → Behavior Tree" works
- ✅ Ctrl+Alt+B shortcut creates new BT tab
- ✅ BT canvas renders with grid visible
- ✅ Can switch between VS and BT tabs without crash
- ✅ No compilation errors/warnings

---

## 🎨 Phase 2: Node Palette & Creation

**Objective**: Drag-drop nodes from palette
**Estimated**: 2-3 days
**Go/No-Go Point**: After this, can add nodes to canvas

### Step 2.1: BTNodePalette Integration
- [ ] File: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`
- [ ] Location: Render() method
- [ ] Change: Split panel layout (20% palette, 50% canvas, 30% properties)
- [ ] Implement: ImGui::BeginChild("btPalette") sections
- [ ] Test: Palette renders in left panel
- [ ] Verify: All node types visible (Selector, Sequence, Condition, Action, etc.)

### Step 2.2: Drag-Drop Event Handling
- [ ] File: `Source\BlueprintEditor\BTNodeGraphManager.cpp`
- [ ] Location: New OnNodeDropped() method
- [ ] Change: Handle ImGui drag-drop payload
- [ ] Implement: Create BTNode at position, add to graph
- [ ] Test: Drag node from palette to canvas
- [ ] Verify: Node appears at drop position

### Step 2.3: Node Validation
- [ ] File: `Source\AI\AIGraphPlugin_BT\BTNodeRegistry.cpp`
- [ ] Location: CanCreateNode() validation
- [ ] Change: Check parent-child constraints
- [ ] Implement: Max/min children, type compatibility
- [ ] Test: Invalid connections prevented
- [ ] Verify: Only valid nodes creatable

**Phase 2 Completion Criteria**:
- ✅ Palette visible in left panel
- ✅ Drag-drop creates nodes on canvas
- ✅ Nodes appear at correct position
- ✅ Node constraints enforced
- ✅ Graph marked dirty on node creation

---

## ⚙️ Phase 3: Property Panel & Serialization

**Objective**: Edit properties and save/load files
**Estimated**: 3-4 days
**Go/No-Go Point**: After this, can save/load round-trip

### Step 3.1: Create PropertyPanel_BT (NEW FILE)
- [ ] File: `Source\BlueprintEditor\PropertyPanel_BT.h` (NEW)
- [ ] Contains: Class declaration (~50 lines)
- [ ] Methods: SetSelectedNode(), Render(), private helpers
- [ ] Data: Pointer to current BTNode, temporary edit state

- [ ] File: `Source\BlueprintEditor\PropertyPanel_BT.cpp` (NEW)
- [ ] Contains: Full implementation (~200-300 lines)
- [ ] Render(): Display node basics + parameters + type-specific fields
- [ ] Edit support: ImGui::InputText for strings, InputInt for ints, InputFloat for floats
- [ ] Validation: Real-time validation of parameter values
- [ ] Test: Creates, compiles, no errors

**PropertyPanel_BT Structure**:
```
┌─ Node Basics ────────────────────┐
│ Node ID: [1234]  [read-only]     │
│ Node Name: [Patrol Root]         │
│ Node Type: [Selector dropdown]   │
│ Enabled: [✓] toggle             │
├─ Node-Specific ─────────────────┤
│ (Condition-specific fields       │
│  Action-specific fields, etc)   │
├─ Parameters ─────────────────────┤
│ String Params:                   │
│   debugName: [____]              │
│   patternName: [____]            │
│                                  │
│ Int Params:                      │
│   repeatCount: [____]            │
│                                  │
│ Float Params:                    │
│   waitTime: [5.00]               │
│   range: [50.0]                  │
│                                  │
├─────────────────────────────────┤
│ [Apply] [Reset] [Help]           │
└──────────────────────────────────┘
```

### Step 3.2: Property Binding
- [ ] File: `Source\BlueprintEditor\BTNodeGraphManager.cpp`
- [ ] Location: OnNodeSelected() method
- [ ] Change: Notify PropertyPanel when node selected
- [ ] Implement: m_propertyPanel->SetSelectedNode(node)
- [ ] Test: Select node → properties appear
- [ ] Verify: Properties update in real-time

- [ ] File: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`
- [ ] Location: Render() method
- [ ] Change: Call m_propertyPanel->Render()
- [ ] Implement: Right panel rendering (30% width)
- [ ] Test: Properties visible when node selected
- [ ] Verify: Layout correct (doesn't overlap canvas)

### Step 3.3: Serialization SaveToFile()
- [ ] File: `Source\AI\BehaviorTree.cpp`
- [ ] Location: BehaviorTreeAsset::SaveToFile() method
- [ ] Change: Implement full JSON v2 serialization
- [ ] Implement: Serialize nodes array, connections, root ID
- [ ] Handle: Parameter maps (string, int, float)
- [ ] Test: Save creates valid JSON file
- [ ] Verify: File readable by human inspection

### Step 3.4: JSON Schema Validation
- [ ] File: `Source\AI\AIGraphPlugin_BT\BTGraphValidator.cpp`
- [ ] Location: Validate() method
- [ ] Change: Add pre-save validation
- [ ] Implement: Check cycles, validate connections, required fields
- [ ] Test: Invalid graphs show errors
- [ ] Verify: Valid graphs save successfully

**Phase 3 Completion Criteria**:
- ✅ PropertyPanel_BT renders correctly
- ✅ Can edit node properties
- ✅ Changes marked in graph dirty flag
- ✅ SaveToFile() creates valid JSON v2
- ✅ LoadFromFile() reconstructs identical graph
- ✅ Round-trip: Save → Load → Save produces identical file

---

## 🔄 Phase 4: Undo/Redo & Validation

**Objective**: Full editing support with history
**Estimated**: 2-3 days

### Step 4.1: Command Integration
- [ ] File: `Source\BlueprintEditor\BTNodeGraphManager.cpp`
- [ ] Location: CreateNode(), DeleteNode(), ConnectNodes() methods
- [ ] Change: Execute() via CommandStack
- [ ] Implement: Wrap node operations in commands
- [ ] Test: Ctrl+Z undoes node creation
- [ ] Verify: Ctrl+Y redoes action

### Step 4.2: Toolbar Buttons
- [ ] File: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`
- [ ] Location: Render() method
- [ ] Change: Add toolbar with buttons
- [ ] Implement: Validate, Compile, Save, Load buttons
- [ ] Test: Buttons functional
- [ ] Verify: Actions execute correctly

### Step 4.3: Error Display UI
- [ ] File: `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`
- [ ] Location: RenderStatusBar() new method
- [ ] Change: Display validation results
- [ ] Implement: Error count, warning count, clickable error list
- [ ] Test: Errors display when validation fails
- [ ] Verify: Can click error to navigate to node

**Phase 4 Completion Criteria**:
- ✅ Undo/Redo works for all operations (Ctrl+Z, Ctrl+Y)
- ✅ Toolbar buttons visible and functional
- ✅ Validation errors display in status bar
- ✅ Error navigation highlights problematic nodes
- ✅ Save prevented if critical errors exist

---

## 🧪 Phase 5: Testing & Polish

**Objective**: Comprehensive testing and optimization
**Estimated**: 2-3 days

### Step 5.1: Create Test BehaviorTree
- [ ] File: `Gamedata/Blueprints/AI/test_editor_bt.json`
- [ ] Contains: Multi-node graph with all node types
- [ ] Includes: Selector, Sequence, Conditions, Actions
- [ ] Verify: Loads and displays correctly

### Step 5.2: Integration Testing
- [ ] [ ] Load existing BT files:
  - [ ] `Blueprints/AI/patrol.json`
  - [ ] `Blueprints/AI/combat.json`
  - [ ] `Blueprints/AI/guard.json`
- [ ] [ ] Can add nodes via palette
- [ ] [ ] Can edit all property types
- [ ] [ ] Can save with Ctrl+S
- [ ] [ ] Save/load round-trip works
- [ ] [ ] Undo/redo all operations
- [ ] [ ] Validation catches errors
- [ ] [ ] No crashes with large graphs (50+ nodes)

### Step 5.3: Performance Optimization
- [ ] Measure: Frame time with 50-node graph
- [ ] Baseline: Should be < 16ms (60 FPS)
- [ ] Profile: Check render time, layout time
- [ ] Optimize: If needed (node layout caching, etc.)
- [ ] Verify: No regression vs VisualScript editor

**Phase 5 Completion Criteria**:
- ✅ 3+ existing BT files load/edit/save successfully
- ✅ New BTs can be created from scratch
- ✅ All workflow paths tested (create, edit, save, load, undo/redo)
- ✅ No crashes or hangs
- ✅ Frame time < 16ms consistently
- ✅ Zero unexpected error messages

---

## ✨ FINAL VERIFICATION

### Compilation
- [ ] Full build: 0 errors
- [ ] Full build: < 5 warnings (preferably 0)
- [ ] No deprecation warnings
- [ ] All new files compile

### Functionality (10-Point Checklist)
- [ ] 1. New → BehaviorTree creates tab
- [ ] 2. Can open existing .json BT files
- [ ] 3. Drag-drop nodes from palette
- [ ] 4. Edit node properties in right panel
- [ ] 5. Save (Ctrl+S) writes valid JSON
- [ ] 6. Load round-trip preserves data
- [ ] 7. Undo/Redo works (Ctrl+Z, Ctrl+Y)
- [ ] 8. Validation shows errors
- [ ] 9. Can switch between VisualScript/EntityPrefab/BehaviorTree tabs
- [ ] 10. No crashes with normal usage

### Documentation
- [ ] Code comments added where non-obvious
- [ ] README updated with Phase 35 status
- [ ] `.github/copilot-instructions.md` updated ✅ (done)
- [ ] User guide created for BT editing

### Git
- [ ] All changes committed
- [ ] Commit message descriptive
- [ ] Branch merged to master
- [ ] No merge conflicts

---

## 📈 Progress Tracking

### Daily Standup Format
```
Date: ____
Day: ___/12-19

✅ Completed Today:
- [task]
- [task]

⏳ In Progress:
- [task]

🔴 Blocked By:
- [issue]

📊 Estimate Remaining:
- Phase 2: __ days
- Phase 3: __ days
- Phase 4: __ days
- Phase 5: __ days
```

### Weekly Milestone
- [ ] **Week 1**: Phases 1-2 complete (canvas + palette)
- [ ] **Week 2**: Phases 3-4 complete (properties + undo)
- [ ] **Week 3**: Phase 5 complete (testing + polish)
- [ ] **Total**: < 20 days ✅

---

## 🎓 Reference Documents

All documentation in `Documentation/` folder:

1. **PHASE_35_EXECUTIVE_SUMMARY.md** - Problem, solution, MVP
2. **PHASE_35_BT_EDITOR_REINTEGRATION_PLAN.md** - Full detailed plan
3. **PHASE_35_ARCHITECTURE_DECISIONS.md** - Technical decisions
4. **PHASE_35_QUICK_START_GUIDE.md** - Developer quick start
5. **PHASE_35_IMPLEMENTATION_CHECKLIST.md** - This file
6. **RAPPORT COMPLET D'ANALYSE** - Existing detailed analysis

---

## 🚀 Ready to Start?

### Pre-Implementation Checklist (Before Day 1)
- [ ] Read QUICK_START_GUIDE.md
- [ ] Read EXECUTIVE_SUMMARY.md
- [ ] Read ARCHITECTURE_DECISIONS.md (sections 1-2)
- [ ] Verify compile (0 errors baseline)
- [ ] Have VS open with solution loaded
- [ ] Have this checklist accessible

### Day 1 Goals (8 hours)
- [ ] Step 1.1: TabManager BehaviorTree case (30 min)
- [ ] Step 1.3: Menu integration (20 min)
- [ ] Step 1.4: Keyboard shortcuts (15 min)
- [ ] Test: Create new BT works (15 min)
- [ ] Step 2.1: BTNodePalette UI (1-2 hours)
- [ ] Step 2.2: Drag-drop event handling (1-2 hours)
- [ ] Step 3.1: Create PropertyPanel_BT skeleton (1 hour)
- [ ] End of day: BT canvas + palette visible ✅

### Success Metrics
- **MVP (Days 1-3)**: Can create, edit nodes, save/load
- **Polish (Days 4-7)**: Undo/redo, validation, testing
- **Production (Days 8+)**: All paths tested, optimized

---

## 📝 Sign-Off

**Planning Phase**: ✅ COMPLETE
**Implementation Phase**: 🚀 READY TO START
**Status**: APPROVED FOR DEVELOPMENT

**Next Action**: Begin Step 1.1 (TabManager.cpp modification)

---

*Checklist Created: 08-04-2026*
*Phase 35 Implementation Ready*
*Estimate: 12-19 days (MVP: 7-9 days)*
