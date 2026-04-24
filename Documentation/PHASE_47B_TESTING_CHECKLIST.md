# Phase 47B: Testing & Verification Checklist

## ✅ Build Verification
- [x] Build succeeds with 0 errors
- [x] No compilation warnings
- [x] All new code compiles cleanly
- [x] No linking issues (SDL errors are pre-existing)

## 🧪 Save Operation Test

### Setup
1. Start Blueprint Editor (fresh instance)
2. Open Asset Browser → Navigate to `Gamedata`
3. Find `simpl_subgraph.bt.json` BehaviorTree file

### Test 1: Load & Save with Correct Filepath
**Goal**: Verify filepath is synchronized and Save works

**Steps**:
1. Double-click `simpl_subgraph.bt.json`
2. **Observe in Console**:
   ```
   [BlueprintEditorGUI::LoadBlueprint] Phase 47B: Set filepath on BehaviorTreeRenderer
   [BehaviorTreeRenderer::SetFilePath] Setting filepath to: Gamedata\simpl_subgraph.bt.json
   ```
   ✅ If seen: Filepath synchronization working

3. Modify the graph:
   - Click on a node
   - Press Delete key
   - Tab title should show `*` (asterisk = dirty)

4. Click **Save** button (in toolbar)
5. **Observe in Console**:
   ```
   [CanvasToolbarRenderer::OnSaveClicked] Current filepath: 'Gamedata\simpl_subgraph.bt.json'
   [BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(...)
   [BehaviorTreeRenderer::Save] SaveGraph result: SUCCESS
   ```
   ✅ If seen: Save executed with correct filepath

6. **Verify Results**:
   - [ ] Asterisk removed from tab (dirty flag reset)
   - [ ] File modified time updated in file explorer
   - [ ] No SaveAs modal appeared (should be direct save)

### Test 2: SaveAs Creates New File
**Goal**: Verify SaveAs functionality works

**Steps**:
1. Modify graph again (add/remove node)
2. Click **SaveAs** button (in toolbar)
3. Choose new filename: `test_save.bt.json`
4. Click **Save**
5. **Verify Results**:
   - [ ] New file created: `test_save.bt.json`
   - [ ] Tab title updated to `test_save.bt.json`
   - [ ] Asterisk removed (dirty flag reset)

### Test 3: Multiple Graph Types
**Goal**: Verify fix works across different graph types

**Steps**:

**For VisualScript**:
1. Create new VisualScript graph
2. Modify, click Save
3. Choose filename, save
4. [ ] File created successfully
5. [ ] Asterisk removed

**For EntityPrefab**:
1. Load entity prefab from `Gamedata` (e.g., `player.json`)
2. Modify, click Save
3. [ ] File saved successfully (or note: EntityPrefab uses different pattern)

## 📊 Diagnostic Logs to Check

When testing, look for these logs (search in console):

### Phase 47B Synchronization Logs
```
[BlueprintEditorGUI::LoadBlueprint] Phase 47B: Set filepath on BehaviorTreeRenderer
[BehaviorTreeRenderer::SetFilePath] Setting filepath to: Gamedata\simpl_subgraph.bt.json
[BehaviorTreeRenderer::SetFilePath] Document updated with filepath
```
✅ **Expected**: When loading legacy file through fallback

### Save Execution Logs
```
[CanvasToolbarRenderer::OnSaveClicked] ENTER - User clicked Save button
[CanvasToolbarRenderer::OnSaveClicked] Current filepath: 'Gamedata\simpl_subgraph.bt.json'
[BehaviorTreeRenderer::Save] Calling NodeGraphManager::SaveGraph(...)
[TabManager] SaveActiveTab: succeeded for 'Gamedata\simpl_subgraph.bt.json'
```
✅ **Expected**: When Save button works correctly

### SaveAs Execution Logs
```
[CanvasToolbarRenderer::OnSaveAsClicked] ENTER
[CanvasToolbarRenderer::OnSaveAsClicked] Setting m_showSaveAsModal = true
[TabManager::OnGraphDocumentSaved] Updated tab display name to: test_save.bt.json
```
✅ **Expected**: When SaveAs creates new file

## ⚠️ Known Issues to Watch For

1. **Empty filepath still showing**:
   - If `Current filepath: ''` in console
   - Phase 47B sync not working
   - Check: Is SetFilePath() being called?

2. **File not saving to disk**:
   - Check file permissions on `Gamedata` folder
   - Check disk space
   - Verify filepath is absolute, not relative

3. **Tab title not updating**:
   - Check `TabManager::OnGraphDocumentSaved` was called
   - Verify dirty flag management

## 📝 Success Criteria

✅ **All tests pass if**:
1. Graph loads with correct filepath visible in logs
2. Save button writes file to original location
3. Asterisk removed after save (dirty flag resets)
4. SaveAs creates new file and updates tab
5. All graph types work (BehaviorTree, VisualScript)

## 🔧 Rollback Instructions (if needed)

If tests fail, revert changes:

**File**: `Source/BlueprintEditor/BlueprintEditorGUI.cpp`
- Remove `#include "BehaviorTreeRenderer.h"` 
- Remove Phase 47B filepath sync code in LoadBlueprint() fallback

**File**: `Source/BlueprintEditor/BehaviorTreeRenderer.h`
- Remove SetFilePath() method declaration

**File**: `Source/BlueprintEditor/BehaviorTreeRenderer.cpp`
- Remove SetFilePath() implementation

Then rebuild.

## 📞 Troubleshooting

### Q: Console shows "phase 47B" message but Save still doesn't work?
**A**: Check that renderer is not null, and that the filepath is actually set. Add temporary log in SetFilePath to verify it's called.

### Q: Dynamic cast failing?
**A**: Ensure `BehaviorTreeRenderer.h` is included (already done). Check inheritance hierarchy.

### Q: File not being written?
**A**: Verify path is correct, check file permissions, verify NodeGraphManager::SaveGraph() succeeds.

### Q: Asterisk not removing?
**A**: Check that dirty flag is being reset by NodeGraphManager. May need additional fix in that system.

---

**Phase 47B Testing Checklist Ready**
