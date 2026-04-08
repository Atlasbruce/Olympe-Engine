# 🚀 PHASE 35 - QUICK START GUIDE

**For**: Development Team Ready to Implement
**Time to Read**: 5 minutes
**Status**: READY FOR ACTION

---

## 📌 The Problem (1 Sentence)
BehaviorTree system is complete at runtime but can't be edited graphically in the editor - it's 40% integrated and needs TabManager to instantiate the renderer.

## ✅ The Solution (1 Sentence)
Add BehaviorTree case to `TabManager::CreateNewTab()` + create PropertyPanel_BT + wire serialization.

## 🎯 Success = 3 Things Working
1. ✅ Can create new BT from "File → New → Behavior Tree" menu
2. ✅ Can edit nodes and properties
3. ✅ Can save/load files correctly

---

## 🔥 START HERE (TODAY)

### Step 1: Verify Setup (10 min)
```bash
cd C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine\

# Open in VS
start OlympeBlueprintEditor.sln

# Check files exist
dir Source\BlueprintEditor\TabManager.cpp          ✅
dir Source\BlueprintEditor\BehaviorTreeRenderer.h ✅
dir Source\AI\BehaviorTree.h                       ✅
```

### Step 2: Read Documentation (15 min)
Read in order:
1. **This file** (you're reading it!)
2. `Documentation/PHASE_35_EXECUTIVE_SUMMARY.md` (2 min)
3. `Documentation/PHASE_35_ARCHITECTURE_DECISIONS.md` (5 min, focus on Decisions 1-2)
4. `Documentation/PHASE_35_BT_EDITOR_REINTEGRATION_PLAN.md` (skim structure, ref during coding)

### Step 3: First Code Change (30 min)
**File**: `Source\BlueprintEditor\TabManager.cpp`
**Line**: 179 (after EntityPrefab case)
**Task**: Add BehaviorTree instantiation

```cpp
else if (graphType == "BehaviorTree")
{
    static NodeGraphPanel s_btPanel;
    static bool s_btPanelInit = false;
    if (!s_btPanelInit)
    {
        s_btPanel.Initialize();
        s_btPanelInit = true;
    }
    
    BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
    tab.renderer = r;
}
```

**Compile & Test**:
```
Build solution
- Expected: 0 errors
- Open menu: File → shouldn't crash
- No warnings about BehaviorTree
```

### Step 4: Menu Integration (20 min)
**File**: `Source\BlueprintEditor\BlueprintEditorGUI.cpp`
**Find**: `RenderMenuBar()` function
**Add**: 
```cpp
if (ImGui::MenuItem("Behavior Tree", "Ctrl+Alt+B"))
{
    TabManager::Get().CreateNewTab("BehaviorTree");
}
```

**Test**:
```
File menu appears → Click "Behavior Tree"
- Expected: New BT tab appears
- Canvas visible with grid
```

**If it works**: 🎉 You've completed MVP Phase 1!

---

## 📋 Quick Command Reference

### Useful Code Snippets

#### Find BehaviorTreeRenderer usage
```cpp
// How it's instantiated (same pattern as VisualScript)
static NodeGraphPanel s_btPanel;
BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
```

#### Find Condition Types (for PropertyPanel)
```cpp
// In BehaviorTree.h, find BTConditionType enum
enum class BTConditionType : uint8_t
{
    TargetVisible = 0,
    HealthBelow = 1,
    // ... etc
};
```

#### Find Action Types (for PropertyPanel)
```cpp
// In BehaviorTree.h, find BTActionType enum
enum class BTActionType : uint8_t
{
    MoveTo = 0,
    Attack = 1,
    PatrolPickNextPoint = 2,
    // ... etc
};
```

---

## 🛠️ Development Workflow

### Build Often
```bash
# Full build
Ctrl+Shift+B

# Incremental build (faster)
Ctrl+B

# Watch output window for errors
View → Output (Ctrl+Alt+O)
```

### Test as You Go
1. After each major change: Build
2. After each file edit: Compile
3. Test in editor before moving to next file

### Keep Editor Open
Don't close the editor window - just reload as you code:
- Save file
- Build
- Reload editor (Ctrl+Shift+Esc to close, reopen)

---

## 🎯 Today's Goals (8-Hour Plan)

| Time | Task | Deliverable |
|------|------|------------|
| 08:00-08:30 | Setup & read docs | Understanding phase 35 |
| 08:30-09:30 | TabManager.cpp change | BT case added, compiles |
| 09:30-10:00 | BlueprintEditorGUI.cpp | Menu item working |
| 10:00-10:30 | Basic test | Can create empty BT |
| 10:30-11:30 | PropertyPanel_BT skeleton | New file created, compiles |
| 11:30-12:30 | Lunch break | 😋 |
| 12:30-14:00 | PropertyPanel implementation | Show/edit node properties |
| 14:00-15:00 | BehaviorTree::SaveToFile() | JSON serialization |
| 15:00-16:00 | Testing & fixes | Load/save round-trip |
| 16:00-16:30 | Documentation update | Phase 35 status updated |

---

## ⚠️ Common Issues & Fixes

### Issue: "BehaviorTreeRenderer not found"
**Solution**: Include file missing
```cpp
#include "BehaviorTreeRenderer.h"
```

### Issue: "NodeGraphPanel not found"
**Solution**: Include file missing
```cpp
#include "NodeGraphPanel.h"
```

### Issue: Compilation error "conflicting data types"
**Solution**: Check enum types in serialization match BTNode struct

### Issue: Canvas doesn't appear
**Solution**: 
1. Check BehaviorTreeRenderer::Render() is called
2. Verify NodeGraphPanel::Initialize() called
3. Check tab is active (SetActiveTab called)

---

## 📞 Quick Reference

### Key Files Location
```
Core System:
├─ Source/AI/BehaviorTree.h/cpp               (data structures)
├─ Source/AI/BehaviorTree.System              (runtime execution)
├─ Source/AI/BehaviorTreeManager              (asset manager)
└─ Source/AI/BehaviorTreeDebugWindow          (runtime UI)

Editor UI:
├─ Source/BlueprintEditor/TabManager.h/cpp    ← MODIFY FIRST
├─ Source/BlueprintEditor/BehaviorTreeRenderer.h/cpp
├─ Source/BlueprintEditor/BTNodeGraphManager.h/cpp
├─ Source/BlueprintEditor/PropertyPanel_BT.h/cpp (CREATE NEW)
└─ Source/BlueprintEditor/BlueprintEditorGUI.cpp ← MODIFY SECOND

BT Plugin:
├─ Source/AI/AIGraphPlugin_BT/BTNodePalette.h/cpp
├─ Source/AI/AIGraphPlugin_BT/BTNodeRegistry.h/cpp
├─ Source/AI/AIGraphPlugin_BT/BTGraphValidator.h/cpp
└─ Source/AI/AIGraphPlugin_BT/BTGraphCompiler.h/cpp
```

### Key Classes
```cpp
BehaviorTreeRenderer     // IGraphRenderer adapter - INSTANTIATE IN TabManager
PropertyPanel_BT         // NEW - property editor
BehaviorTreeAsset        // Data model - implement SaveToFile()
BTNode                   // Node structure
BTNodeType              // enum: Selector, Sequence, Condition, Action, etc.
```

---

## 🧪 Minimal Test Case

Create test after Step 4:

```cpp
// In some test function or main()
{
    // 1. Create new BT
    std::string tabId = TabManager::Get().CreateNewTab("BehaviorTree");
    ASSERT_FALSE(tabId.empty());
    
    // 2. Verify tab created
    EditorTab* tab = TabManager::Get().GetTab(tabId);
    ASSERT_NE(nullptr, tab);
    ASSERT_EQ("BehaviorTree", tab->graphType);
    
    // 3. Verify renderer exists
    ASSERT_NE(nullptr, tab->renderer);
}
```

---

## 📚 Full Documentation Index

| Document | Read When | Time |
|----------|-----------|------|
| **QUICK_START_GUIDE** (this file) | Starting today | 5 min |
| EXECUTIVE_SUMMARY | Before coding | 10 min |
| ARCHITECTURE_DECISIONS | Choosing implementation | 15 min |
| BT_EDITOR_REINTEGRATION_PLAN | During coding (ref) | 20 min |
| BehaviorTree System Analysis | Deep dive later | 30 min |

---

## 🚦 Green Light Check

Before starting, verify you have:
- [ ] Visual Studio 2026 open
- [ ] Olympe-Engine repo cloned
- [ ] Can compile existing solution (0 errors baseline)
- [ ] Read this guide (5 min)
- [ ] Read EXECUTIVE_SUMMARY (10 min)
- [ ] Understand: BehaviorTree → IGraphRenderer → TabManager → Render

If all checked: **You're ready to code!** 🚀

---

## 💬 Need Help?

### Compile Error?
1. Check include files (BehaviorTreeRenderer.h, NodeGraphPanel.h)
2. Check enum types match
3. Check file paths in project

### Logic Question?
1. Check ARCHITECTURE_DECISIONS.md (Decision 1: Shared NodeGraphPanel)
2. Check existing code (VisualScriptRenderer pattern)
3. Check BehaviorTreeRenderer.h for interface

### Not Sure About Next Step?
1. Re-read today's goals timeline
2. Check REINTEGRATION_PLAN.md for your next phase
3. Verify previous step compiled successfully

---

## ✨ You've Got This!

**Remember**: This is mostly wiring together existing pieces. The heavy lifting (BT runtime, validation, etc.) is already done. You're just:
1. ✅ Instantiating the renderer (1 code block)
2. ✅ Adding UI menu items (1 function)
3. ✅ Creating property editor (200 LOC, straightforward)
4. ✅ Wiring serialization (1 function)

**Estimated time**: 12-19 days with proper testing/polish. MVP: 2-3 days.

**Let's ship Phase 35! 🚀**

---

*Quick Start Guide - Phase 35 BehaviorTree Editor Integration*
*Ready to implement: 08-04-2026*
