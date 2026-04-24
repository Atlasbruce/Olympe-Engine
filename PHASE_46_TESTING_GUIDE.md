# Phase 46: Quick Testing Guide - Save Flow Diagnostics

## 🎯 Objective
Run the editor and trace save operations to identify where the call chain breaks.

---

## Step 1: Build and Run
```bash
# Build completed successfully
✅ 0 Errors - Ready to run

# Run the application
- Launch OlympeBlueprintEditor
- Open a graph (create new or load existing)
```

---

## Step 2: Capture Console Output

### Option A: Visual Studio Console
1. Run app from Visual Studio (F5 or Ctrl+F5)
2. Console window shows real-time logs
3. Copy/paste to file for analysis

### Option B: Redirect to File
```powershell
# In PowerShell
.\OlympeBlueprintEditor.exe > save_test_output.txt 2>&1
```

### Option C: Real-Time Monitoring
1. Open Application Insights or Event Viewer
2. Monitor SYSTEM_LOG output during save

---

## Step 3: Perform Save Operation

### Test Sequence A: Save Dirty Graph
```
1. Create new BehaviorTree or Visual Script
2. Make changes (should see dirty flag = "*" in tab)
3. Click [Save] button
4. ✅ OBSERVE LOGS
```

### Test Sequence B: SaveAs to New Path
```
1. Create new graph
2. Click [Save As] button
3. Choose new path in dialog
4. Confirm save
5. ✅ OBSERVE LOGS
```

### Test Sequence C: Multiple Saves
```
1. Open graph
2. Make changes
3. Save (observe logs)
4. Make more changes
5. Save again (compare logs - should see same pattern)
```

---

## Step 4: Analyze Logs

### Look For: Complete Successful Save Log Sequence

```
[CanvasToolbarRenderer::OnSaveClicked] ENTER - User clicked Save button
[CanvasToolbarRenderer::OnSaveClicked] Document type is BEHAVIOR_TREE
[CanvasToolbarRenderer::OnSaveClicked] Current filepath: '...'
[CanvasToolbarRenderer::OnSaveClicked] About to call ExecuteSave()
[CanvasToolbarRenderer::ExecuteSave] ENTER - filePath='...'
[CanvasToolbarRenderer::ExecuteSave] Document type: BehaviorTree
[CanvasToolbarRenderer::ExecuteSave] Dirty state BEFORE save: TRUE
[CanvasToolbarRenderer::ExecuteSave] About to invoke m_document->Save(filepath)...
[CanvasToolbarRenderer::ExecuteSave] ✓ m_document->Save() returned TRUE  ← KEY LINE
[CanvasToolbarRenderer::ExecuteSave] Dirty state AFTER save: FALSE      ← KEY LINE
[CanvasToolbarRenderer::OnSaveClicked] ✓ ExecuteSave returned true
[CanvasToolbarRenderer::OnSaveClicked] Calling TabManager::OnGraphDocumentSaved()
[CanvasToolbarRenderer::OnSaveClicked] EXIT - Save complete
```

**✅ All logs present = Save flow working**

---

### Look For: Missing Logs (Break Points)

#### Problem: No OnSaveClicked entry log
```
❌ [CanvasToolbarRenderer::OnSaveClicked] ENTER NOT PRESENT
```
**Issue**: Button click not detected or handler not called
**Investigation**: Check if button is clickable, ImGui event handling

#### Problem: Document type check fails
```
[CanvasToolbarRenderer::OnSaveClicked] WARNING: Document type is not BEHAVIOR_TREE
```
**Issue**: Document is wrong type or not loaded
**Investigation**: Check what graph types are supported, tab content

#### Problem: ExecuteSave returns FALSE
```
[CanvasToolbarRenderer::ExecuteSave] ✗ m_document->Save() returned FALSE - save FAILED!
```
**Issue**: Backend serialization failed
**Investigation**: Check IGraphDocument::Save() implementation, file permissions, disk space

#### Problem: Dirty state NOT reset after save
```
[CanvasToolbarRenderer::ExecuteSave] Dirty state AFTER save: TRUE  ← Should be FALSE!
```
**Issue**: Serialization succeeded but dirty flag not updated
**Investigation**: Check IGraphDocument::Save() implementation, MarkClean() call

---

## Step 5: Identify Break Point

### Break Point Map

| Missing Logs | Break Point | Root Cause |
|-------------|-------------|-----------|
| No ENTER log | Button click | UI not responding, handler not called |
| No ExecuteSave call log | Conditional | Document type check failed, path validation |
| ExecuteSave returns FALSE | Backend | IGraphDocument::Save() implementation |
| Dirty state not reset | Serialization | Mark-clean not called after save |
| No callback execution | State update | TabManager or callback not invoked |

---

## Step 6: Report Findings

### Minimal Report Format
```
SAVE TEST RESULT: FAILED

Break Point: [Identify which log sequence stopped]

Logs Present: 
- [List actual logs from console]

Logs Missing:
- [List expected logs that didn't appear]

Last Successful Step: [Describe last log before break]

Issue Type: [UI | Conditional | Backend | Serialization | Other]

Console Output:
[Paste relevant log segment]
```

---

## Expected vs Actual

### Expected Flow for Successful Save
```
1. ✅ OnSaveClicked ENTER
2. ✅ Document check PASS
3. ✅ Path check PASS
4. ✅ ExecuteSave called
5. ✅ Backend Save returns TRUE
6. ✅ Dirty state FALSE
7. ✅ Callbacks executed
8. ✅ EXIT
```

### Diagnosis: If any ❌ is ✗ FAILED
- Find the first missing log
- Check that component implementation
- Report findings

---

## Log Levels Summary

### Entry/Exit Logs (Method lifecycle)
```
[Class::Method] ENTER
[Class::Method] EXIT
```

### State Logs (Conditions checked)
```
[Class::Method] Document type is BEHAVIOR_TREE
[Class::Method] Dirty state BEFORE: TRUE
```

### Action Logs (Operations performed)
```
[Class::Method] About to call ExecuteSave()
[Class::Method] Calling TabManager::OnGraphDocumentSaved()
```

### Result Logs (Success/Failure)
```
[Class::Method] ✓ Save succeeded
[Class::Method] ✗ Save FAILED
```

### Error Logs (Problems encountered)
```
[Class::Method] ERROR: No document loaded!
[Class::Method] WARNING: Document type not supported
```

---

## Performance Note

**No Console Spam**:
- ✅ Only 30+ logs per save operation
- ✅ Clear separation between saves
- ✅ No render loop spam
- ✅ Easy to read and follow

---

## Quick Commands

### View logs for save operation only
```powershell
# Capture, then filter for save operation
Get-Content save_test_output.txt | Select-String "OnSaveClicked|ExecuteSave|OnSaveAsClicked|SaveAs"
```

### Count log occurrences
```powershell
Get-Content save_test_output.txt | Measure-Object -Line
```

### Extract timeline
```powershell
Get-Content save_test_output.txt | Select-String "CanvasToolbarRenderer" | % { $_.Line }
```

---

## Troubleshooting

### Problem: No logs appear in console
- **Check**: Is console redirected correctly?
- **Fix**: Run directly in Visual Studio debug window
- **Verify**: SYSTEM_LOG macro is defined and working

### Problem: Too many logs (console spam)
- **Check**: Are logs appearing in render loops?
- **Fix**: Review copilot-instructions.md logging discipline
- **Verify**: No logs in RenderButtons(), RenderPathDisplay()

### Problem: Incomplete log sequence
- **Check**: Did operation complete or crash?
- **Fix**: Look for exceptions in output
- **Verify**: Check file permissions, disk space, path validity

---

## Reference

For detailed logging architecture and full call chain, see:
- **PHASE_46_SAVE_FLOW_LOGGING_GUIDE.md** - Complete logging guide
- **Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp** - Actual logging code
- **Source/BlueprintEditor/BlueprintEditorGUI.cpp** - GUI integration

---

**Ready to test? Run the application and perform a save operation!**

The logs will tell you exactly where the save flow breaks. 🔍
