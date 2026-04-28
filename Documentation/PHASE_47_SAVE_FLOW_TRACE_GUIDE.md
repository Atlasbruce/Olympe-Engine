# Phase 47: Save Flow Trace Guide
## Diagnostic Logging - Code Path Identification

### Problem Identified 🔴
When you click Save on a BehaviorTree graph:
- Phase 46 framework logs **DO NOT APPEAR**
- `SaveFilePickerModal` logs repeat at 60 FPS (render loop spam)
- Graph modification state not updated
- File is not actually saved

**Hypothesis**: TabManager::SaveActiveTab() is **NOT** being called, or the renderer being saved is **NOT** using the framework integration.

### Solution: Trace Code Path with Strategic Logs

Added 3 new diagnostic entry points to trace which code path executes:

#### 1. **TabManager::SaveActiveTab()** (Entry Point)
**Location**: `Source/BlueprintEditor/TabManager.cpp` line 491

**Logs**:
```cpp
[TabManager::SaveActiveTab] ENTER
[TabManager::SaveActiveTab] Active tab: <name> (type: <type>)
[TabManager::SaveActiveTab] Calling tab->renderer->Save() for: <path>
[TabManager::SaveActiveTab] Save result: SUCCESS/FAILED
```

**What it tells you**:
- ✅ If you see "ENTER": Save is being routed through TabManager
- ❌ If you DON'T see it: Save button click is NOT reaching TabManager

#### 2. **BehaviorTreeRenderer::Save()** (Renderer-Level Save)
**Location**: `Source/BlueprintEditor/BehaviorTreeRenderer.cpp` line 275

**Logs**:
```cpp
[BehaviorTreeRenderer::Save] ENTER path: <path>
[BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(id=<id>, path=<path>)
[BehaviorTreeRenderer::Save] SaveGraph result: SUCCESS/FAILED
```

**What it tells you**:
- ✅ If you see this: The BehaviorTreeRenderer's Save method is executing
- ❌ If missing: The renderer is not being called
- ⚠️ If "SaveGraph result: FAILED": Issue is in NodeGraphManager save

#### 3. **CanvasToolbarRenderer::OnSaveClicked()** (Framework Button)
**Location**: `Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp` line 327

**Logs** (Already present):
```cpp
[CanvasToolbarRenderer::OnSaveClicked] ENTER - User clicked Save button
[CanvasToolbarRenderer::OnSaveClicked] Document type is BEHAVIOR_TREE
[CanvasToolbarRenderer::OnSaveClicked] Current filepath: '<path>'
[CanvasToolbarRenderer::OnSaveClicked] About to call ExecuteSave()
```

**What it tells you**:
- ✅ If you see "ENTER": Framework Save button was triggered
- ❌ If missing: Framework buttons are not being called
- ⚠️ If "No path yet": FilePath not properly set

---

## Test Procedure

### 1. **Load a BehaviorTree Graph**
```
File > New Behavior Tree (Ctrl+Shift+N)
  OR
File > Open Blueprint > Select .bt.json file
```

**Expected in console**:
```
[BehaviorTreeRenderer] Loaded BT graph: ...
```

### 2. **Make a Modification**
- Add/remove/edit a node in the graph
- Graph should show as modified (* on tab name)

### 3. **Click Save (Ctrl+S)**
```
File > Save
  OR
Press Ctrl+S
```

**CRITICAL: Analyze Console Output Order**

#### Scenario A: Framework Path is Working ✅
Expected console sequence:
```
[TabManager::SaveActiveTab] ENTER
[TabManager::SaveActiveTab] Active tab: <name> (type: BehaviorTree)
[TabManager::SaveActiveTab] Calling tab->renderer->Save() for: Gamedata\simpl_subgraph.bt.json
[BehaviorTreeRenderer::Save] ENTER path: Gamedata\simpl_subgraph.bt.json
[BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(id=1, path=...)
[BehaviorTreeRenderer::Save] SaveGraph result: SUCCESS
[TabManager::SaveActiveTab] Save result: SUCCESS
```

**If you see this**: Framework integration is working correctly

#### Scenario B: Framework Path NOT Triggered ❌
Console shows:
```
[SaveFilePickerModal] Found 1 files and 3 folders in .
[SaveFilePickerModal] Found 1 files and 3 folders in .
[SaveFilePickerModal] Found 1 files and 3 folders in .
(repeating at 60 FPS)

(NO TabManager logs)
(NO BehaviorTreeRenderer::Save logs)
(NO CanvasToolbarRenderer::OnSaveClicked logs)
```

**If this is your case**: Save button click is NOT reaching TabManager at all!

#### Scenario C: TabManager Path Works BUT BehaviorTreeRenderer::Save Fails ⚠️
Console shows:
```
[TabManager::SaveActiveTab] ENTER
[TabManager::SaveActiveTab] Active tab: simpl_subgraph.bt.json (type: BehaviorTree)
[TabManager::SaveActiveTab] Calling tab->renderer->Save() for: ...
[BehaviorTreeRenderer::Save] ENTER path: ...
[BehaviorTreeRenderer::Save] EXIT: Invalid graph ID
[TabManager::SaveActiveTab] Save result: FAILED
```

**If this is your case**: The renderer's graph ID is invalid (renderer not properly initialized)

---

## Expected Outcomes

### If Scenario A (Framework Working) ✅
- [x] See all TabManager and BehaviorTreeRenderer logs
- [x] SaveGraph result: SUCCESS
- [x] File should be saved
- [x] Tab dirty flag should reset

### If Scenario B (Framework NOT Called) ❌
**Next Phase**: Investigate why Save button doesn't call TabManager
- Check: Is save button wired to call TabManager::SaveActiveTab()?
- Check: Is the button even being rendered?
- Check: Are keyboard shortcuts routed correctly?

### If Scenario C (Framework Called But Renderer Fails) ⚠️
**Next Phase**: Fix BehaviorTreeRenderer initialization
- Check: Is graph ID properly set during Load()?
- Check: Is NodeGraphManager correctly managing the graph?

---

## Expected Log Appearance Timeline

| Timeframe | Log Source | Frequency |
|-----------|-----------|-----------|
| **Per Save Operation** | TabManager, BehaviorTreeRenderer | Once (event-driven) |
| **Frame Rendering** | SaveFilePickerModal | 60 FPS (render loop) |
| **Total**  | Should see 4-6 Phase 47 logs per save click | NOT per frame |

---

## Troubleshooting

### Problem: Phase 47 logs don't appear at all
**Possible causes**:
1. Save button not wired to TabManager::SaveActiveTab()
2. Keyboard shortcut not routing to correct handler
3. Different code path being used entirely

**Debug steps**:
1. Verify File > Save menu item calls TabManager::SaveActiveTab()
2. Verify Ctrl+S keyboard handler calls same function
3. Add log to Menu item handler to confirm it's clicked

### Problem: TabManager logs appear but BehaviorTreeRenderer::Save doesn't
**Possible causes**:
1. tab->renderer is null
2. tab->renderer is not a BehaviorTreeRenderer
3. Save method not being called on correct renderer type

**Debug steps**:
1. Check tab->graphType in TabManager log output
2. Verify renderer is correct type (BehaviorTree)
3. Add type check log in TabManager before calling Save()

### Problem: All logs appear but SaveGraph returns FAILED
**Possible causes**:
1. Graph ID is invalid (-1)
2. NodeGraphManager::SaveGraph() has a bug
3. File path is not writable

**Debug steps**:
1. Check graph ID logged in BehaviorTreeRenderer::Save
2. Check m_filePath is correctly initialized
3. Check file permissions on save path

---

## Next Phase Actions

Based on console output:

**If Scenario A**: ✅ Framework working
- Move to Phase 48: Verify file is actually written
- Check dirty flag reset
- Test SaveAs functionality

**If Scenario B**: ❌ Framework not called
- Phase 47B: Route save button to TabManager
- Wire menu item handler correctly
- Route keyboard shortcuts to framework

**If Scenario C**: ⚠️ Renderer issue
- Phase 47C: Debug BehaviorTreeRenderer initialization
- Verify graph ID assignment during Load()
- Fix renderer state management

---

## Quick Reference: Log Checklist

Run save operation and check for these logs:

```
[ ] [TabManager::SaveActiveTab] ENTER
[ ] [TabManager::SaveActiveTab] Active tab: simpl_subgraph.bt.json (type: BehaviorTree)
[ ] [TabManager::SaveActiveTab] Calling tab->renderer->Save() for: ...
[ ] [BehaviorTreeRenderer::Save] ENTER path: ...
[ ] [BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(...)
[ ] [BehaviorTreeRenderer::Save] SaveGraph result: SUCCESS
[ ] [TabManager::SaveActiveTab] Save result: SUCCESS
```

If any box is unchecked, it identifies the break point in your save flow.
