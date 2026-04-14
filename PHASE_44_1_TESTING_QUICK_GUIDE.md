# Phase 44.1 Testing Guide - Quick Reference

**Objective**: Verify hybrid modal consolidation works in all 3 editors

**Prerequisites**: 
- Visual Studio with project loaded
- Build successful (✅ verified)
- Application running in editor mode

---

## Quick Test Matrix

| Test | Editor | Expected Result | Status |
|------|--------|-----------------|--------|
| SubGraph Browse | VisualScript | Modal appears, folder tree visible | [ ] |
| SubGraph Browse | BehaviorTree | Modal appears, folder tree visible | [ ] |
| BehaviorTree Browse | EntityPrefab | Modal appears, folder tree visible | [ ] |
| SaveAs Button | All Editors | Modal appears, file is saved | [ ] |
| Browse Button | Toolbar | Modal appears, file selection works | [ ] |

---

## Test 1: VisualScript SubGraph Selection ✓

**Steps**:
1. Open Visual Script blueprint (or create new)
2. Add a SubGraph node to canvas
3. Click property panel "Browse" button
4. **VERIFY**: 
   - [ ] FilePickerModal appears
   - [ ] "Folders:" label visible (left panel)
   - [ ] Folder tree shows Blueprints/ directory
   - [ ] "Available Files:" shows on right
   - [ ] Can navigate folders
   - [ ] Can select .ats file
   - [ ] Property updates after selection

**Expected UI**:
```
┌─────────────────────────────────┐
│ Select File               [X]   │
├─────────────────┬───────────────┤
│ Folders:        │ Available:    │
│ ├─ Blueprints/ │ • File1.ats   │
│ └─ .. (parent) │ • File2.ats   │
└─────────────────┴───────────────┘
```

**Pass Criteria**: Modal appears with folder tree and file list

---

## Test 2: BehaviorTree SubGraph Selection ✓

**Steps**:
1. Open BehaviorTree or create new
2. Add BT_SubGraph node
3. Click property panel "Browse" button
4. **VERIFY**:
   - [ ] FilePickerModal appears
   - [ ] Folder tree visible
   - [ ] Can select .ats file
   - [ ] Parameter "subgraphPath" updates
   - [ ] Modal closes after selection

**Pass Criteria**: Same folder tree UI appears, parameter updates

---

## Test 3: EntityPrefab BehaviorTree Selection ✓

**Steps**:
1. Open EntityPrefab or create new
2. Add component with BehaviorTree property
3. Click "Browse##bt_browse" button
4. **VERIFY**:
   - [ ] FilePickerModal appears
   - [ ] Folder tree visible
   - [ ] Can select .bts file
   - [ ] Property updates in panel
   - [ ] Modal closes after selection

**Pass Criteria**: Modal appears, property updates correctly

---

## Test 4: Toolbar SaveAs ✓

**Steps**:
1. Open any blueprint/graph
2. Make edit (mark as dirty)
3. Click toolbar "SaveAs" button
4. **VERIFY**:
   - [ ] SaveFilePickerModal appears
   - [ ] Can navigate to save directory
   - [ ] Can enter filename
   - [ ] Can confirm save
   - [ ] File is saved to correct path
   - [ ] Modal closes after save

**Pass Criteria**: File saved successfully, no errors

---

## Test 5: Toolbar Browse ✓

**Steps**:
1. Click toolbar "Browse" button (general button, not editor-specific)
2. **VERIFY**:
   - [ ] FilePickerModal appears
   - [ ] Folder tree visible
   - [ ] Can navigate folders
   - [ ] Can select file
   - [ ] Callback fires (file selection processed)
   - [ ] Modal closes

**Pass Criteria**: Modal works with folder tree, file selection processed

---

## Troubleshooting

### Issue: Modal Doesn't Appear

**Possible Causes**:
- [ ] Check if dispatcher is initialized: `CanvasModalRenderer::Get()`
- [ ] Verify modal render call exists in main loop
- [ ] Check for modal collision (multiple renders per frame)
- [ ] See build output for any initialization errors

**Check**: Open Output window → Build tab for any warnings

---

### Issue: Folder Tree Not Visible

**Possible Causes**:
- [ ] FilePickerModal not being used (check which modal appears)
- [ ] Folder tree rendering disabled
- [ ] ImGui child window sizing issue

**Check**: 
- Verify "Folders:" label appears in modal
- Check left panel width (should be ~150px)

---

### Issue: File Selection Not Applied

**Possible Causes**:
- [ ] Modal confirmed but property not updated
- [ ] Path prefix not handled correctly
- [ ] Property setter not called

**Check**:
- Look in console for "[PropertyEditorPanel]" or similar logs
- Verify `ApplyChanges()` was called
- Check if file path is correct in property

---

## Expected Output in Visual Studio

### Console Logs (When Tests Run)

```
[CanvasModalRenderer] Opened SubGraph file picker: Blueprints/
[PropertyEditorPanel] Selected BehaviorTree: BehaviorTrees/MyTree.bts
[CanvasToolbarRenderer] SaveAs confirmed: C:\path\to\file.ats
```

### Expected Build Output

```
Génération réussie ✅
0 erreurs, 0 avertissements
```

---

## Test Results Template

```markdown
# Phase 44.1 Testing Results

**Date**: [TODAY]
**Tester**: [YOUR NAME]
**Build**: ✅ Successful (0 errors)

## Results

- [ ] Test 1: VisualScript SubGraph Browse
  - Status: [ ] PASS [ ] FAIL
  - Notes: ___________________

- [ ] Test 2: BehaviorTree SubGraph Browse
  - Status: [ ] PASS [ ] FAIL
  - Notes: ___________________

- [ ] Test 3: EntityPrefab BehaviorTree Browse
  - Status: [ ] PASS [ ] FAIL
  - Notes: ___________________

- [ ] Test 4: Toolbar SaveAs
  - Status: [ ] PASS [ ] FAIL
  - Notes: ___________________

- [ ] Test 5: Toolbar Browse
  - Status: [ ] PASS [ ] FAIL
  - Notes: ___________________

## Summary

Total Tests: 5
Passed: [ ]
Failed: [ ]
Status: [ ] APPROVED [ ] NEEDS FIXES

## Issues Found

- Issue 1: ___________________
  Resolution: ___________________

## Sign-off

Phase 44.1 Modal Consolidation: [ ] APPROVED FOR DEPLOYMENT
```

---

## Quick Checklist

Before declaring tests complete:

- [ ] All 5 test scenarios run successfully
- [ ] No modal collision issues (modals don't overlap)
- [ ] File selections properly update properties
- [ ] SaveAs saves files to correct paths
- [ ] No unexpected console errors
- [ ] Build still clean (0 errors)
- [ ] No performance regressions observed

---

## Time Estimate

- Test 1 (VisualScript): ~2 minutes
- Test 2 (BehaviorTree): ~2 minutes  
- Test 3 (EntityPrefab): ~2 minutes
- Test 4 (Toolbar SaveAs): ~2 minutes
- Test 5 (Toolbar Browse): ~2 minutes

**Total**: ~10 minutes for complete validation

---

*Quick Reference for Phase 44.1 Testing*
*See PHASE_44_1_VERIFICATION_REPORT.md for detailed testing guide*
