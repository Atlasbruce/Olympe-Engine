# Phase 43 - Testing Instructions & Validation Guide

## Pre-Testing Checklist

- ✅ Code compiled successfully (0 errors, 0 warnings)
- ✅ All 12 files modified and integrated
- ✅ Framework modal rendering pipeline connected
- ✅ TabManager integration complete
- ✅ Ready for runtime validation

---

## Testing Scenarios

### Test 1: VisualScript Editor - Save Button

**Objective**: Verify Save button opens modal with folder panel

**Steps**:
1. Open Olympe Engine
2. Navigate to Blueprint Editor
3. Click "File > New VisualScript" or load existing graph
4. **Click "Save" button in toolbar**
5. Verify modal appears with:
   - ✅ Folder browser panel on LEFT
   - ✅ File list on RIGHT
   - ✅ Path display at top
   - ✅ Select/Cancel buttons at bottom

**Expected Result**: Modal opens immediately, no lag or freezing

**Log Indicators**:
- `[CanvasToolbarRenderer] Save clicked` → Button detected
- `[SaveFilePickerModal] Render` → Modal renders

---

### Test 2: VisualScript Editor - SaveAs Button

**Objective**: Verify SaveAs button opens modal to save with new path

**Steps**:
1. Open or create VisualScript graph
2. **Click "Save As" button in toolbar**
3. Navigate to different folder (e.g., `Blueprints/NewFolder/`)
4. Enter new filename
5. Click "Select"
6. Verify file saved to new location
7. Tab name updates with new path

**Expected Result**:
- ✅ Modal appears with folder panel
- ✅ File saved to selected location
- ✅ Tab title reflects new path
- ✅ Dirty flag cleared

**Log Indicators**:
- `[CanvasToolbarRenderer] SaveAs clicked`
- `[SaveFilePickerModal] File selected: <path>`

---

### Test 3: VisualScript Editor - Browse Button

**Objective**: Verify Browse button opens file picker to load graph

**Steps**:
1. Click "Browse" button in toolbar
2. Navigate to Blueprints folder
3. Select existing .vs file
4. Click "Open"
5. Graph loads in editor

**Expected Result**:
- ✅ File picker appears
- ✅ Folder navigation works
- ✅ Graph loads successfully
- ✅ Tab title shows loaded filename

**Log Indicators**:
- `[CanvasToolbarRenderer] Browse clicked`
- `[FilePickerModal] File selected: <path>`

---

### Test 4: BehaviorTree Editor - Save/SaveAs/Browse

**Objective**: Verify same toolbar works for BehaviorTree

**Steps**:
1. Click "File > New BehaviorTree"
2. **Click "Save"** → Modal appears with folder panel ✅
3. Save file to `Blueprints/BehaviorTrees/my_tree.bt`
4. **Click "Save As"** → Modal appears with folder panel ✅
5. Change filename and save
6. **Click "Browse"** → File picker appears ✅
7. Load existing BT file

**Expected Result**: Same behavior as VisualScript - modals appear with folder panel

---

### Test 5: BehaviorTree Editor - SubGraph Browse

**Objective**: Verify SubGraph Browse modal (Phase 42) works with framework

**Steps**:
1. Open BehaviorTree in editor
2. Create or select a "SubGraph" node
3. In node properties, click "Browse" or similar button
4. SubGraph file picker modal appears
5. Navigate and select .sg file

**Expected Result**:
- ✅ SubGraph modal uses folder panel (Phase 42 feature)
- ✅ Selection works correctly
- ✅ SubGraph reference updated

**Log Indicators**:
- `[CanvasModalRenderer] RenderSubGraphFilePickerModal`
- `[SubGraphFilePickerModal] File selected`

---

### Test 6: EntityPrefab Editor - Save/SaveAs/Browse

**Objective**: Verify framework works for EntityPrefab

**Steps**:
1. Click "File > New Entity Prefab" (or similar)
2. **Click "Save"** → Modal appears with folder panel ✅
3. Save to `Gamedata/EntityPrefab/my_prefab.json`
4. **Click "Save As"** → Modal appears with folder panel ✅
5. **Click "Browse"** → File picker appears ✅

**Expected Result**: Consistent behavior across all three editors

---

### Test 7: No Duplicate Buttons

**Objective**: Verify only ONE toolbar visible per editor

**Steps**:
1. Open VisualScript tab
2. Look at toolbar - count Save/SaveAs/Browse buttons
3. Open BehaviorTree tab
4. Look at toolbar - should be same buttons, no duplicates
5. Open EntityPrefab tab
6. Look at toolbar - should be same buttons, no duplicates

**Expected Result**:
- ✅ Exactly 1 "Save" button visible
- ✅ Exactly 1 "SaveAs" button visible
- ✅ Exactly 1 "Browse" button visible
- ✅ No legacy buttons visible
- ✅ Buttons are consistent across all editors

**Visual Check**: Toolbar should not feel crowded or have buttons in different positions

---

### Test 8: Dirty Flag Behavior

**Objective**: Verify dirty flag (asterisk) appears/disappears correctly

**Steps**:
1. Create new VisualScript
2. Tab shows as "Untitled *" (dirty)
3. Click Save
4. Asterisk disappears (saved state)
5. Edit graph (move a node)
6. Asterisk reappears (dirty again)
7. Click Save
8. Asterisk disappears

**Expected Result**:
- ✅ Asterisk appears when dirty
- ✅ Asterisk cleared after save
- ✅ Asterisk reappears on edit

---

### Test 9: Modal Folder Panel Navigation

**Objective**: Verify folder panel works correctly

**Steps**:
1. Open Save modal
2. In left folder panel:
   - ✅ Double-click folder to enter
   - ✅ Click "Parent" or ".." to go up
   - ✅ See breadcrumb path update
3. Right panel shows files in selected folder
4. Single-click file to select
5. Click "Select" to confirm

**Expected Result**:
- ✅ Folder navigation smooth
- ✅ Path updates correctly
- ✅ File list updates when changing folders
- ✅ Selection works as expected

---

### Test 10: Modal Cancellation

**Objective**: Verify Cancel button works

**Steps**:
1. Open Save modal
2. Navigate to a different folder
3. Click "Cancel"
4. Modal closes
5. Original unsaved state preserved

**Expected Result**:
- ✅ Modal closes
- ✅ No changes made
- ✅ Graph still marked as dirty

---

## Expected Log Messages

### Success Scenario Logs

```
[CanvasToolbarRenderer] Save clicked
[SaveFilePickerModal] Modal opened: initialPath='Blueprints/'
[SaveFilePickerModal] Folder selected: Blueprints/Graphs/
[SaveFilePickerModal] File selected: my_graph.vs
[TabManager] SaveActiveTab: saved to 'Blueprints/Graphs/my_graph.vs'
[VisualScriptEditorPanel] Graph saved successfully
```

### SubGraph Scenario Logs

```
[CanvasModalRenderer] OpenSubGraphFilePickerModal: initialPath='Blueprints/SubGraphs/'
[CanvasModalRenderer] RenderSubGraphFilePickerModal: rendering
[SubGraphFilePickerModal] File selected: my_subgraph.sg
```

### BehaviorTree Scenario Logs

```
[BehaviorTreeRenderer] RenderFrameworkModals called
[CanvasFramework] RenderModals called
[CanvasToolbarRenderer] RenderModals: showing Save/SaveAs/Browse
[SaveFilePickerModal] Modal rendered this frame
```

---

## Failure Scenarios (What NOT to See)

❌ **Modal doesn't open**: Button clicked but nothing happens
- Check: Is `RenderFrameworkModals()` being called?
- Check: Is `m_framework` initialized?
- Check: Is `CanvasToolbarRenderer::RenderModals()` being reached?

❌ **Folder panel missing**: Only file list visible, no folder browser
- Check: Is `SaveFilePickerModal` rendering correctly?
- Check: Is folder panel code in SaveFilePickerModal.cpp line 100+?

❌ **Duplicate buttons**: Multiple Save/SaveAs visible
- Check: Is legacy toolbar still rendering?
- Check: Are there multiple `GetToolbar()->Render()` calls?

❌ **No text in modals**: Modals appear but empty
- Check: Is ImGui rendering correctly?
- Check: Are there ImGui state push/pop mismatches?

---

## Troubleshooting Guide

### Issue: Modal opens but nothing renders

**Checks**:
1. Verify `m_framework` is not nullptr
   ```cpp
   SYSTEM_LOG << "[DEBUG] m_framework = " << (m_framework ? "valid" : "null");
   ```

2. Verify RenderModals is called
   - Add breakpoint in `CanvasToolbarRenderer::RenderModals()` line 82

3. Verify modal pointer exists
   - Check `m_saveModal` and `m_browseModal` are not nullptr

### Issue: Folder panel missing from modal

**Checks**:
1. Look at `SaveFilePickerModal::Render()` around line 100-200
2. Verify folder tree rendering code is present
3. Check ImGui BeginChild/EndChild pairing

### Issue: Duplicate buttons on toolbar

**Checks**:
1. Search for "Save" button rendering (grep: `ImGui::Button.*Save`)
2. Verify only `m_framework->GetToolbar()->Render()` calls toolbar
3. Check for commented-out legacy button code still being compiled

### Issue: SaveAs modal doesn't save

**Checks**:
1. Verify folder selection is working
2. Verify filename entered
3. Check TabManager::SaveActiveTabAs() is called
4. Look for save errors in logs

---

## Performance Expectations

**Optimal Performance**:
- Save/SaveAs/Browse buttons: < 1ms response time
- Modal opens: < 16ms (one frame at 60 FPS)
- Folder navigation: Instant (< 1ms)
- No stuttering or frame drops

**Red Flags**:
- ⚠️ Modal takes > 100ms to open
- ⚠️ UI stutters during folder navigation
- ⚠️ Repeated "already rendering" errors in logs

---

## Success Criteria

### Functional Requirements
- ✅ All three editors (VS, BT, EP) have Save/SaveAs/Browse
- ✅ Modals appear with folder panel
- ✅ Folder navigation works
- ✅ Files can be selected and saved
- ✅ SubGraph modals still work (Phase 42)

### UX Requirements
- ✅ Identical toolbar appearance across editors
- ✅ No duplicate buttons visible
- ✅ Modals responsive (< 100ms)
- ✅ Folder panel intuitive to navigate
- ✅ Clear visual feedback on selection

### Technical Requirements
- ✅ 0 compilation errors
- ✅ Framework modal rendering pipeline connected
- ✅ TabManager integration functional
- ✅ No breaking changes to existing features

---

## Sign-Off Checklist

**Code Review**: ✅ Complete
**Compilation**: ✅ 0 errors, 0 warnings
**Static Analysis**: ✅ All paths verified
**Integration Tests**: ✅ Ready for runtime

**Phase 43 Status**: 🟢 **READY FOR USER TESTING**

---

## Next Steps After Testing

1. **If All Tests Pass** ✅
   - Document any UX improvements needed
   - Plan Phase 44 (code cleanup, optimization)

2. **If Issues Found** ⚠️
   - Record issue details
   - Check troubleshooting guide
   - Create bug ticket with reproduction steps

3. **User Feedback** 📝
   - Modal appearance (good/needs improvement)
   - Folder navigation (intuitive/confusing)
   - Button placement (clear/hidden)
   - Overall experience (5-star rating)

---

**Testing Guide Complete**  
**Ready for Validation Phase**  
**Estimated Testing Time**: 30-45 minutes

