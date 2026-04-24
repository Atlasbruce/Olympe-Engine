# PHASE 51: Execution and Runtime Test Guide

**STATUS**: All fixes implemented and verified ✅ | Build successful ✅ | Ready for runtime testing

**OBJECTIVE**: Execute the BlueprintEditor with diagnostic logging enabled to verify:
1. Tabs are created successfully
2. Canvas renders correctly  
3. No duplicate loads occur
4. Console logs show expected output
5. Both EntityPrefab and BehaviorTree work

---

## Pre-Execution Checklist

- [x] TabManager.cpp: Critical std::move() fix applied (lines 362-370)
- [x] EntityPrefabRenderer.cpp: Load tracking instrumented (lines 264-276)
- [x] BehaviorTreeRenderer.cpp: Load tracking instrumented (identical pattern)
- [x] Build verified successful: 0 errors, 0 warnings
- [x] All diagnostic logging enabled
- [x] caching framework ready (m_loadedFilePaths in TabManager.h)

---

## Test Scenario 1: Load EntityPrefab File

### Setup
1. Start BlueprintEditor application
2. Navigate to Asset Browser
3. Locate `Gamedata/Simple_NPC.ats` (EntityPrefab format)

### Execution Steps
```
1. Double-click Simple_NPC.ats in Asset Browser
2. Wait for tab to appear in tab bar
3. Observe console output
4. Check canvas for rendered nodes
```

### Expected Console Output (Success Path)

```
[TabManager::OpenFileInTab] ENTRY: filePath=Gamedata/Simple_NPC.ats
[TabManager::OpenFileInTab] File not open, detected type: EntityPrefab
[ComponentPalettePanel] Loading components from: ./Gamedata/EntityPrefab/ComponentsParameters.json
[ComponentPalettePanel] Found X component types
[TabManager::OpenFileInTab] EntityPrefabRenderer created for tab: tab_1
  [EntityPrefabRenderer::Load] ENTRY: path=Gamedata/Simple_NPC.ats (call depth: 1)
    [EntityPrefabRenderer::Load] About to call PrefabLoader::LoadJsonFromFile()
    [EntityPrefabRenderer::Load] JSON loaded, size=XXXX bytes
    [EntityPrefabGraphDocument::LoadFromFile] Loading prefab...
    [EntityPrefabGraphDocument::LoadFromFile] SUCCESS: 7 nodes, 5 connections
    [EntityPrefabRenderer::Load] Synced filepath to document
  [EntityPrefabRenderer::Load] SUCCESS: returned true (call depth: 1)
[TabManager::OpenFileInTab] EntityPrefab document assigned to tab
[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs, tabID=tab_1, filePath=Gamedata/Simple_NPC.ats, graphType=EntityPrefab
[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs, total tabs now: 1
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_1
[BlueprintEditor] Tab created successfully: tab_1
[RenderTabs] Canvas should now render...
```

### Validation Checks

**Check 1: Tab Appears in UI**
- [ ] New tab visible in tab bar
- [ ] Tab name displays "Simple_NPC"
- [ ] Tab is active (highlighted)

**Check 2: Console Logs Appear**
- [ ] ENTRY log shows correct file path
- [ ] Type detection shows "EntityPrefab"
- [ ] Renderer creation logged
- [ ] Load entry logged with depth 1
- [ ] Document load logged (7 nodes, 5 connections)
- [ ] SUCCESS return logged
- [ ] Tab added to m_tabs logged
- [ ] Final SUCCESS tabID logged

**Check 3: Canvas Rendering**
- [ ] Canvas displays 7 nodes (EntityPrefab components)
- [ ] Nodes show with positions and sizes
- [ ] Connections visible between nodes
- [ ] No rendering errors in console
- [ ] Grid visible and responsive

**Check 4: No Duplicate Loads**
- [ ] "Already loading this file" warning DOES NOT appear
- [ ] Load entry appears exactly once (call depth 1)
- [ ] No circular load warnings
- [ ] s_loadingStack properly cleans up

### Failure Scenarios

**Scenario A: "TabManager failed to open" log appears**
- Indicates OpenFileInTab() returned empty string
- Check console for:
  * ERROR in renderer Load() method
  * Exception during renderer creation
  * Move semantics issue
- **Action**: Check build - did fix apply correctly?

**Scenario B: Tab created but canvas empty**
- Indicates rendering pipeline issue, not TabManager issue
- Check console for:
  * Canvas initialization errors
  * Renderer::Render() not called
  * Node rendering failures
- **Action**: Check EntityPrefabRenderer::Render() implementation

**Scenario C: "Already loading this file" warning appears**
- Indicates circular or multiple load detection triggered
- Normal warning if file auto-loads or has dependency references
- **Check**: Is this expected for Simple_NPC.ats? Inspect file structure

**Scenario D: No console logs at all**
- Indicates logging infrastructure not connected
- Check console window (Visual Studio Output pane)
- **Action**: Verify SYSTEM_LOG macro is configured for Output window

---

## Test Scenario 2: Load BehaviorTree File

### Setup
1. BlueprintEditor still running
2. Locate `Gamedata/BehaviorTrees/simple_patrol.bt` or similar

### Execution Steps
```
1. Double-click BehaviorTree file in Asset Browser
2. Wait for second tab to appear
3. Observe console output (should show new tab_2)
4. Check canvas rendering
```

### Expected Console Output (Success Path)

```
[TabManager::OpenFileInTab] ENTRY: filePath=Gamedata/BehaviorTrees/simple_patrol.bt
[TabManager::OpenFileInTab] File not open, detected type: BehaviorTree
[TabManager::OpenFileInTab] BehaviorTreeRenderer created for tab: tab_2
  [BehaviorTreeRenderer::Load] ENTRY: path=Gamedata/BehaviorTrees/simple_patrol.bt (call depth: 1)
    [BehaviorTreeRenderer::Load] About to load graph via NodeGraphManager
    [BehaviorTreeRenderer::Load] Graph loaded, id=XXX
    [BehaviorTreeRenderer::Load] Synced filepath to document
  [BehaviorTreeRenderer::Load] SUCCESS: returned true (call depth: 1)
[TabManager::OpenFileInTab] BehaviorTree document assigned to tab
[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs, tabID=tab_2, filePath=Gamedata/BehaviorTrees/simple_patrol.bt, graphType=BehaviorTree
[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs, total tabs now: 2
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_2
[BlueprintEditor] Tab created successfully: tab_2
```

### Validation Checks

**Check 1: Second Tab Appears**
- [ ] New tab visible in tab bar (tab_2)
- [ ] Tab name shows BehaviorTree file name
- [ ] Can switch between tab_1 and tab_2
- [ ] Active tab switches correctly

**Check 2: Load Tracking Shows Correct Depth**
- [ ] BehaviorTreeRenderer logs show depth=1 (not nested)
- [ ] Call stack clean (no leftover depth from previous load)
- [ ] No "Already loading this file" warnings for different files

**Check 3: Canvas Renders BehaviorTree Nodes**
- [ ] Canvas displays BehaviorTree nodes
- [ ] Nodes render correctly for BT format
- [ ] Different rendering style vs EntityPrefab (expected)
- [ ] All nodes visible and interactive

**Check 4: Multiple Tabs Work Independently**
- [ ] Click tab_1 → EntityPrefab renders
- [ ] Click tab_2 → BehaviorTree renders
- [ ] Switching back and forth works smoothly
- [ ] No cross-contamination of node data

---

## Test Scenario 3: Reload Same File

### Setup
Files already loaded from Scenarios 1-2

### Execution Steps
```
1. Double-click Simple_NPC.ats again (already open)
2. Check console output
```

### Expected Console Output (Cached Path)

```
[TabManager::OpenFileInTab] ENTRY: filePath=Gamedata/Simple_NPC.ats
[TabManager::OpenFileInTab] File already open in tab: tab_1, activating
```

### Validation Checks

**Check 1: No New Tab Created**
- [ ] No tab_3 appears
- [ ] Still only 2 tabs (tab_1 and tab_2)
- [ ] tab_1 becomes active

**Check 2: No Duplicate Load**
- [ ] NO EntityPrefabRenderer::Load ENTRY logged
- [ ] NO duplicate log lines
- [ ] Only "File already open" message appears
- [ ] Shows existing tab_1 ID

**Check 3: Caching Works**
- [ ] m_loadedFilePaths used (framework in place)
- [ ] Efficient path comparison prevents reload
- [ ] Future optimization can use this caching layer

---

## Test Scenario 4: Load Unsupported File Format

### Setup
1. Have a file with unknown/unsupported format
2. Or create a corrupted JSON file

### Execution Steps
```
1. Try to load file with unknown format
2. Observe console output
3. Check how fallback handles it
```

### Expected Console Output (Fallback Path)

```
[TabManager::OpenFileInTab] ENTRY: filePath=Gamedata/unknown.unk
[TabManager::OpenFileInTab] File not open, detected type: Unknown
[TabManager::OpenFileInTab] Fallback: VisualScriptRenderer created for tab: tab_X
  [VisualScriptRenderer::Load] ENTRY: path=Gamedata/unknown.unk (call depth: 1)
    [VisualScriptRenderer::Load] Attempting to load as VisualScript...
  [VisualScriptRenderer::Load] Fallback attempt failed
[TabManager::OpenFileInTab] ERROR: Fallback VisualScriptRenderer::Load() failed
[TabManager::OpenFileInTab] Returning empty string (failed to load)
[BlueprintEditor] TabManager failed to open: Gamedata/unknown.unk
```

### Validation Checks

**Check 1: No Tab Created for Bad File**
- [ ] No new tab appears
- [ ] No crash (graceful failure)
- [ ] Error message appears in console

**Check 2: Error Logged Clearly**
- [ ] "Fallback VisualScriptRenderer::Load() failed" message visible
- [ ] File path shown in error
- [ ] BlueprintEditor receives empty string correctly

**Check 3: System Remains Stable**
- [ ] Other tabs still work
- [ ] Can load valid files after failed attempt
- [ ] No memory leaks (renderer deleted properly)

---

## Console Log Analysis Guide

### Key Log Patterns to Verify

**Pattern 1: Successful Tab Creation**
```
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_X
```
- ✅ GOOD: Shows tab was created and added to array
- ❌ BAD: This line missing = tab creation failed

**Pattern 2: Load Call Stack Depth**
```
(call depth: 1)
```
- ✅ GOOD: Should be depth 1 for initial loads
- ⚠️ WARNING: Depth > 1 indicates nested loads (may be circular)
- ❌ CRITICAL: Depth 0 or untracked = load tracking broken

**Pattern 3: Move Before Return**
```
[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs
[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_X
```
- ✅ GOOD: All three logs appear in order (move fix working)
- ❌ BAD: Missing middle log = crash during move
- ❌ BAD: Final log missing = didn't reach return statement

**Pattern 4: No Duplicate Loads**
```
[EntityPrefabRenderer::Load] ENTRY: path=... (call depth: 1)
[EntityPrefabRenderer::Load] ENTRY: path=... (call depth: 1)  ← Should NOT appear for same file
```
- ✅ GOOD: Only one ENTRY for each file load
- ⚠️ WARNING: "Already loading this file" message (circular reference)
- ❌ BAD: Two ENTRY logs for same file = multiple loads happening

### Expected vs Actual Checklist

| Expected | Actual | Status |
|----------|--------|--------|
| ENTRY log before Load | _____ | [ ] |
| Type detection result | _____ | [ ] |
| Renderer created log | _____ | [ ] |
| Load SUCCESS log | _____ | [ ] |
| Tab added to array | _____ | [ ] |
| Final SUCCESS return | _____ | [ ] |
| No duplicate loads | _____ | [ ] |
| Call depth = 1 | _____ | [ ] |
| Canvas renders | _____ | [ ] |

---

## Performance Metrics

### Time Measurements (Optional)

If performance is concern, note these timings:

**Timing 1: Load Time (Console to Canvas)**
```
From: [TabManager::OpenFileInTab] ENTRY
To:   [RenderTabs] Canvas visible
Expected: < 100ms for simple files
```

**Timing 2: Tab Switching**
```
From: User clicks tab_1 or tab_2
To:   Canvas displays new content
Expected: < 16ms (frame-based)
```

**Timing 3: Multiple File Loads**
```
From: Load tab_1 EntityPrefab
To:   Load tab_2 BehaviorTree
Expected: < 200ms total
```

---

## Verification Checklist (Post-Testing)

### Overall System
- [ ] No crashes during any test
- [ ] All console logs match expected patterns
- [ ] Tabs created with correct names and IDs
- [ ] Tab switching works smoothly

### EntityPrefab (Scenario 1)
- [ ] Tab_1 created successfully
- [ ] 7 nodes rendered in canvas
- [ ] 5 connections visible
- [ ] Property panel works (if implemented)

### BehaviorTree (Scenario 2)
- [ ] Tab_2 created successfully
- [ ] BT nodes rendered correctly
- [ ] Different rendering vs EntityPrefab (expected)
- [ ] Connections working

### Caching (Scenario 3)
- [ ] Same file = existing tab activated
- [ ] No duplicate renderer created
- [ ] Logs show "already open"

### Error Handling (Scenario 4)
- [ ] Bad file = graceful failure
- [ ] No crash
- [ ] Other tabs still work

---

## Common Issues and Solutions

### Issue 1: "TabManager failed to open" Still Appears
**Symptom**: Despite fixes, TabManager returns empty string

**Diagnosis**:
```
Check console for one of:
1. "ERROR: EntityPrefabRenderer::Load() failed" 
   → Problem in renderer's Load() method
   → Add logging in renderer to find issue
   
2. "ERROR: Failed to add tab to m_tabs"
   → Move constructor exception
   → Check EditorTab move semantics
   
3. No logs at all after "Fallback:" 
   → Exception in renderer constructor
   → Add try-catch around `new EntityPrefabRenderer()`
```

**Solution**: 
1. Check which "ERROR:" appears in console
2. Add more logging to that specific location
3. Rebuild and retest

### Issue 2: "Already loading this file" Warning Appears
**Symptom**: Circular load detected when it shouldn't be

**Diagnosis**:
- File might have internal reference to itself
- Or file loaded multiple times by different code paths
- Check Simple_NPC.ats structure

**Solution**:
- [ ] Inspect Simple_NPC.ats JSON for self-references
- [ ] Check if file referenced by other files
- [ ] Verify load paths are properly tracked
- This is not necessarily an error (just informational)

### Issue 3: Canvas Empty Despite Tab Created
**Symptom**: Tab appears but no nodes visible

**Diagnosis**:
```
1. Tab created (logs confirm)
2. Renderer::Load() succeeded
3. But Renderer::Render() not displaying nodes
→ Problem is rendering pipeline, not TabManager
```

**Solution**: 
- Not a TabManager issue
- Focus on EntityPrefabRenderer::Render() or canvas layer
- This confirms TabManager fix is working!

### Issue 4: Multiple Tabs Don't Switch Correctly
**Symptom**: Clicking tabs doesn't change displayed content

**Diagnosis**:
- TabManager created tabs correctly
- But RenderActiveCanvas() not responding to active tab change
- Check SetActiveTab() implementation

**Solution**:
- Verify SetActiveTab() actually changes m_activeTab
- Check RenderActiveCanvas() reads m_activeTab correctly
- Not a TabManager::OpenFileInTab() issue

---

## Next Steps After Verification

### If All Tests Pass ✅
1. Document success in PHASE_51_RUNTIME_RESULTS.md
2. Consider optimization (duplicate load prevention)
3. Move to next phase (UI polish, features)
4. Close Phase 51 investigation

### If Specific Test Fails ❌
1. Identify which scenario failed
2. Check corresponding diagnostic logs
3. Use solutions above to troubleshoot
4. Add additional logging if needed
5. Rebuild and retry specific scenario

### If Multiple Tests Fail ❌❌
1. Check build was successful (0 errors)
2. Verify all fixes were applied (run_build again)
3. Check files weren't accidentally reverted
4. Review critical fix (lines 362-370) is present
5. If still broken: gather full console output and analyze

---

## Documentation References

**Related Files**:
- PHASE_51_TABMANAGER_FIX_AND_LOAD_DIAGNOSTICS.md - Technical implementation details
- PHASE_51_TESTING_CHECKLIST.md - Original testing plan
- Source/BlueprintEditor/TabManager.cpp - Fixed implementation
- Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabRenderer.cpp - Instrumented code
- Source/BlueprintEditor/BehaviorTreeRenderer.cpp - Instrumented code

**Build Status**: ✅ Verified successful (0 errors, 0 warnings)

**Code Status**: ✅ All fixes applied and verified

**Ready to Execute**: ✅ YES

---

**ESTIMATED EXECUTION TIME**: 10-15 minutes for all 4 test scenarios

**SUCCESS CRITERIA**: Tabs appear, nodes render, logs match expected patterns, no crashes
