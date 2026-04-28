# Phase 44.2 Functional Testing Guide

**Testing Objective**: Verify that toolbar Save/SaveAs buttons now work correctly across all three editors (VisualScript, BehaviorTree, EntityPrefab) after document unification fixes.

**Status**: Build verified clean ✅ (0 errors, 0 warnings)

---

## Test Environment Setup

**Prerequisites**:
- Build successful ("Génération réussie")
- Application running with all three editors available
- Sample graphs available for testing (Blueprints/, BehaviorTrees/, EntityPrefab/)

**What Changed**: 
- TabManager now uses `renderer->GetDocument()` instead of creating duplicate documents
- Framework and tab system reference the same document object
- Toolbar Save operations now target the correct document

---

## Test Case 1: VisualScript - New File + Save

**Steps**:
1. Launch Olympe Blueprint Editor
2. File → New → VisualScript (Ctrl+N)
3. Add at least 2-3 nodes to the graph (e.g., Input, Math, Output)
4. Verify window title shows `*` (dirty flag) before filename
5. **Toolbar Save Button**: Click "Save" in toolbar
6. Expected Result: 
   - Dialog appears asking for filename
   - File saves successfully
   - `*` disappears from window title (dirty flag clears)
   - Status bar shows "Saved: [filename]"

**Verification**:
- [ ] Toolbar Save button clicked
- [ ] Save dialog appeared
- [ ] File saved with correct name
- [ ] Dirty flag cleared after save
- [ ] File actually exists in file system

**If Failed**: 
- Check if save dialog appears but doesn't save → framework issue
- Check if dirty flag doesn't clear → document tracking issue
- Check file system to verify file created

---

## Test Case 2: VisualScript - Edit File + SaveAs

**Steps**:
1. File → Open → Select existing VisualScript file
2. Make an edit (move a node, change a property)
3. Verify `*` appears in title (dirty flag)
4. **Toolbar SaveAs Button**: Click "SaveAs" in toolbar
5. Enter new filename (e.g., "MyGraph_Copy.ats")
6. Expected Result:
   - New file created with new name
   - Tab title updates to show new filename
   - Dirty flag clears
   - Old file unchanged

**Verification**:
- [ ] SaveAs dialog appeared
- [ ] New filename entered
- [ ] New file created in correct directory
- [ ] Tab title updated to new filename
- [ ] Dirty flag cleared
- [ ] Old file still exists and unchanged

**If Failed**:
- Check if SaveAs dialog appears → UI working
- Check file system for new file → save operation
- Check tab title updated → framework feedback

---

## Test Case 3: BehaviorTree - New File + Save

**Steps**:
1. File → New → BehaviorTree (Ctrl+Alt+B or menu)
2. Add 3-5 nodes to the tree (e.g., Selector, Sequence, Action)
3. Connect nodes with behaviors
4. Verify `*` appears in title (dirty flag)
5. **Toolbar Save Button**: Click "Save" in toolbar
6. Expected Result:
   - Dialog appears asking for filename
   - File saves successfully
   - `*` disappears from window title
   - File appears in BehaviorTrees/ directory

**Verification**:
- [ ] Toolbar Save button clicked
- [ ] Save dialog appeared
- [ ] File saved with .bts extension
- [ ] Dirty flag cleared after save
- [ ] File exists in BehaviorTrees/ directory

**If Failed**:
- Verify BehaviorTreeRenderer::GetDocument() is accessible
- Check BehaviorTree save path (should be BehaviorTrees/)
- Verify no access level errors

---

## Test Case 4: BehaviorTree - Browse + Select

**Steps**:
1. Create new BehaviorTree
2. Add BT_SubGraph node
3. In property panel, click "Browse..." button next to SubGraph field
4. Expected Result:
   - FilePickerModal opens with folder tree on left
   - Folder tree shows BehaviorTrees/ directories
   - Can navigate and select existing BehaviorTree file
   - Selection updates property

**Verification**:
- [ ] Browse button clicked
- [ ] FilePickerModal opened (not old SubGraphFilePickerModal)
- [ ] Folder tree visible on LEFT side
- [ ] Can navigate folders
- [ ] Can select file from right side
- [ ] Property updated with selection

**If Failed**:
- Check CanvasModalRenderer is using FilePickerModal
- Verify folder tree displays correctly
- Check file selection updates properties

---

## Test Case 5: EntityPrefab - New File + Save

**Steps**:
1. File → New → Entity Prefab
2. In ComponentPalette, double-click "Transform" to add node
3. Double-click "Identity_data" to add second node
4. Drag second node to different position
5. Verify `*` appears in title
6. **Toolbar Save Button**: Click "Save" in toolbar
7. Expected Result:
   - Dialog appears asking for filename
   - File saves with .json extension
   - `*` disappears from title
   - File stored in correct prefab directory

**Verification**:
- [ ] Toolbar Save clicked
- [ ] Save dialog appeared
- [ ] Filename entered
- [ ] File saved successfully
- [ ] Dirty flag cleared
- [ ] JSON file created and contains correct graph data

**If Failed**:
- EntityPrefab was already fixed (prior session)
- Verify changes persist to file

---

## Test Case 6: Multi-Tab Scenario

**Steps**:
1. Open VisualScript file → Tab 1
2. Open different VisualScript file → Tab 2
3. Switch to Tab 1, make edit, Save
4. Switch to Tab 2, verify changes don't appear (isolated)
5. Make different edit in Tab 2, SaveAs new file
6. Switch back to Tab 1, verify unchanged

**Verification**:
- [ ] Each tab has separate document (not cross-contamination)
- [ ] Save in Tab 1 doesn't affect Tab 2
- [ ] Dirty flags independent
- [ ] Each file saves with own changes

**If Failed**:
- Documents may be sharing state
- Check TabManager creates separate GetDocument() calls
- Verify each tab has unique renderer instance

---

## Test Case 7: Dirty Flag Lifecycle

**Steps**:
1. New VisualScript file
2. Note: `*` NOT in title (clean file)
3. Add one node
4. Note: `*` NOW in title (dirty)
5. Click Save
6. Note: `*` GONE from title (clean after save)
7. Add another node
8. Note: `*` BACK in title (dirty again)
9. Don't save, create new file
10. Verify old file not modified

**Verification**:
- [ ] Dirty flag appears after first edit
- [ ] Dirty flag clears after save
- [ ] Dirty flag reappears after next edit
- [ ] Unsaved file doesn't modify original

**Expected Behavior**:
```
NEW → Clean (no *) → Edit → Dirty (*) → Save → Clean (no *) → Edit → Dirty (*) → ...
```

**If Failed**:
- Dirty flag not tracking properly
- Check IGraphDocument::IsDirty() implementation
- Verify framework sets dirty after edits

---

## Quick Test - All Three Editors

**Fast Verification** (5 minutes):

```
1. File → New → VisualScript
2. Add 2 nodes
3. Toolbar Save → File saves ✓
4. 
5. File → New → BehaviorTree
6. Add 2 nodes
7. Toolbar Save → File saves ✓
8.
9. File → New → Entity Prefab
10. Add 1 node
11. Toolbar Save → File saves ✓
```

**Result**: If all three save operations succeed without errors, Phase 44.2 is working.

---

## Troubleshooting Checklist

### Save Button Doesn't Work
```
□ Toolbar Save button doesn't respond
  → Check CanvasToolbarRenderer::OnSaveClicked()
  → Verify m_document is not nullptr

□ Save dialog doesn't appear
  → Check CanvasToolbarRenderer::OpenSaveModal()
  → Verify DataManager::ShowSaveFilePickerModal() works

□ Save dialog appears but file doesn't save
  → Check IGraphDocument::Save() implementation
  → Verify file path permissions
```

### Dirty Flag Issues
```
□ Dirty flag doesn't appear after edit
  → Check IGraphDocument::SetDirty() called
  → Verify framework marks document dirty on edits

□ Dirty flag doesn't clear after save
  → Check IGraphDocument::ClearDirty() called
  → Verify toolbar Save calls ClearDirty() after success
```

### Document References Wrong
```
□ Saves to wrong document
  → Check GetDocument() returns correct adapter
  → Verify TabManager uses r->GetDocument()
  → Check framework uses tab.document (not separate instance)

□ Multi-tab interference
  → Each tab should have SEPARATE renderer
  → Each renderer has OWN GetDocument()
  → Verify no static/global document sharing
```

### File System Issues
```
□ File saved but in wrong directory
  → Check default save path in CanvasToolbarRenderer
  → Verify FilePickerModal shows correct default dir

□ File not persisting across sessions
  → Verify full path in save dialog
  → Check file actually written to disk
```

---

## Success Criteria

✅ **PASS** if ALL of these work:
1. New files can be created and saved
2. Existing files can be saved with SaveAs
3. Dirty flags appear/clear correctly
4. Multiple tabs don't interfere
5. Files persist to disk and reload correctly
6. All three editor types work identically

❌ **FAIL** if ANY of these don't work:
1. Toolbar Save button doesn't save
2. Dirty flag doesn't track correctly
3. Multi-tab saves corrupts wrong file
4. File operations crash or freeze

---

## Test Result Template

```
TEST CASE: [1-7 or Custom]
EDITOR TYPE: [VisualScript / BehaviorTree / EntityPrefab]
ACTION: [What user did]
EXPECTED: [What should happen]
ACTUAL: [What actually happened]
RESULT: [PASS / FAIL]
NOTES: [Any observations]
```

---

## Reporting Issues

If any test fails:

1. **Save Details**:
   - Which editor (VS / BT / EP)?
   - New file or existing file?
   - Did dialog appear?
   - Did file get created?

2. **Provide Evidence**:
   - Screenshot of error
   - File system check (does file exist?)
   - Output window logs

3. **Root Cause Clues**:
   - "Dialog appeared but no save" → Framework issue
   - "No dialog at all" → Toolbar issue
   - "File created but wrong content" → Document serialization
   - "Dirty flag broken" → Document tracking

---

## Timeline

**Phase 44.2 Completion**: Code changes applied ✅

**Phase 44.2.1** (This Testing): Functional verification ⏳
- Estimated: 15-20 minutes

**Phase 44.2.2** (After Testing): Documentation
- Estimated: 5-10 minutes

---

*Document Version: 44.2*  
*Last Updated: Session continuing from 44.2 completion*  
*Status: Ready for Functional Testing*
