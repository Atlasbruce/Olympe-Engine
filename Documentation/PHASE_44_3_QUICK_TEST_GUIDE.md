# Phase 44.3 - Quick Testing Guide

## What Changed
✅ **Legacy Save/SaveAs buttons removed from BehaviorTree**  
✅ **Framework toolbar now visible (was blocked by legacy UI)**  
✅ **All three editors unified on same framework**  
✅ **Build successful (0 errors)**

---

## Quick Test (2 minutes)

### BehaviorTree Editor
1. Click **[SaveAs]** button
2. Should see modal with:
   - **LEFT**: Folder navigation panel
   - **RIGHT**: File list with .json files
   - **INPUT**: Suggested filename
3. Select folder → Enter name → Click Save
4. **Verify**: Tab name updates, no asterisk

### VisualScript Editor
1. Click **[SaveAs]** button
2. **Verify**: Same modal structure as BehaviorTree
3. Select folder → Enter name → Click Save
4. **Verify**: Tab name updates

### EntityPrefab Editor
1. Click **[SaveAs]** button
2. **Verify**: Same modal structure
3. Select folder → Enter name → Click Save
4. **Verify**: Tab name updates

---

## What Should Work

| Feature | BehaviorTree | VisualScript | EntityPrefab | Status |
|---------|--------------|--------------|--------------|--------|
| [Save] button | ✅ | ✅ | ✅ | Framework |
| [SaveAs] button | ✅ | ✅ | ✅ | Framework |
| [Browse] button | ✅ | ✅ | ✅ | Framework |
| SaveAs modal | ✅ | ✅ | ✅ | Framework |
| Folder panel | ✅ | ✅ | ✅ | Framework |
| File list | ✅ | ✅ | ✅ | Framework |
| Tab updates | ✅ | ✅ | ✅ | Framework |
| Validation | ✅ | ✅ | ✅ | Framework |
| Dirty flag | ✅ | ✅ | ✅ | Framework |

---

## Files Touched

**Removed**:
- `Source/BlueprintEditor/NodeGraphPanel.cpp` - Removed 160 lines of legacy UI

**Modified (Previous)**:
- `Source/BlueprintEditor/Framework/BehaviorTreeGraphDocument.cpp` - Backend integration
- `Source/BlueprintEditor/BehaviorTreeRenderer.h` - Added GetNodeGraphPanel()

**No changes needed**:
- VisualScriptEditorPanel - Already working ✅
- EntityPrefabRenderer - Already working ✅

---

## Expected Behavior

### Save Button (Existing File)
```
Graph has filepath
  ↓
User clicks [Save]
  ↓
Saves immediately (no modal)
  ↓
Tab name updates
  ↓
Success message in output
```

### SaveAs Button (New File)
```
User clicks [SaveAs]
  ↓
SaveFilePickerModal opens
  ├─ Folder panel on LEFT
  ├─ File list on RIGHT  
  └─ Filename input
  ↓
User selects folder + enters name
  ↓
User clicks Save
  ↓
Graph saved to new location
  ↓
Tab name updates
  ↓
Success message in output
```

### Browse Button
```
User clicks [Browse]
  ↓
File picker modal opens
  ├─ Filters for .json files
  └─ Shows Gamedata/TaskGraph or Gamedata/EntityPrefab
  ↓
User selects file
  ↓
Graph loads
  ↓
Tab name updated with filename
  ↓
Success message in output
```

---

## Troubleshooting

### Issue: Save buttons don't appear
**Solution**: 
- Verify build succeeded (0 errors)
- Check that framework was initialized in renderer
- Verify NodeGraphPanel legacy buttons removed

### Issue: SaveAs modal doesn't show folder panel
**Solution**:
- Check SaveFilePickerModal.cpp for folder rendering
- Verify CanvasModalRenderer::OpenSaveFilePickerModal() is called
- Check ImGui child window rendering

### Issue: Buttons work but tab doesn't update
**Solution**:
- Verify TabManager::OnGraphDocumentSaved() called
- Check CanvasToolbarRenderer::OnSaveAsComplete()
- Look for errors in output window

### Issue: Build fails
**Solution**:
- Clean solution and rebuild
- Check NodeGraphPanel.cpp line 206 for proper activeGraph declaration
- Verify no syntax errors in modified files

---

## Output Window Expected Messages

### On SaveAs Success
```
[BehaviorTreeGraphDocument::Save] Saving to: Gamedata/TaskGraph/mynewgraph.json
[BehaviorTreeGraphDocument::Save] SUCCESS: Saved to Gamedata/TaskGraph/mynewgraph.json
[TabManager] OnGraphDocumentSaved: Updated tab for graph X
```

### On Browse Success
```
[DataManager] File picker selected: Gamedata/TaskGraph/existinggraph.json
[BehaviorTreeGraphDocument] Loading from: Gamedata/TaskGraph/existinggraph.json
[BehaviorTreeGraphDocument] Loaded successfully
```

### On Error
```
[BehaviorTreeGraphDocument::Save] VALIDATION ERROR: Root node not found
[BehaviorTreeGraphDocument::Save] Failed to save
```

---

## Quick Verification Checklist

✅ **Build** - 0 errors, 0 warnings  
⬜ **BehaviorTree** - [SaveAs] button visible and works  
⬜ **BehaviorTree** - SaveAs modal shows folder panel (LEFT) + files (RIGHT)  
⬜ **BehaviorTree** - Tab name updates after save  
⬜ **VisualScript** - [SaveAs] button visible and works  
⬜ **VisualScript** - SaveAs modal identical to BehaviorTree  
⬜ **VisualScript** - Tab name updates after save  
⬜ **EntityPrefab** - [SaveAs] button visible and works  
⬜ **EntityPrefab** - SaveAs modal identical to others  
⬜ **EntityPrefab** - Tab name updates after save  
⬜ **Consistency** - All three editors show same buttons in same order  

---

## Build Command
```powershell
cd C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine
dotnet build
```

Expected output:
```
Build succeeded.
  0 Error(s)
  0 Warning(s)
```

---

**Generated**: Phase 44.3  
**Status**: Ready for user testing  
**Build**: ✅ Successful  
