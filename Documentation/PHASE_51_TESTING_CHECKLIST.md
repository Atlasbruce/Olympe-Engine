# Phase 51: Testing Checklist and Diagnostics Guide

## Pre-Test Verification

✅ **Build Status**: Successful (0 errors, 0 warnings)
✅ **Code Changes**: All modifications compiled successfully
✅ **Diagnostics**: Comprehensive logging added to:
   - TabManager::OpenFileInTab()
   - EntityPrefabRenderer::Load()
   - BehaviorTreeRenderer::Load()

---

## Test Scenario 1: EntityPrefab Loading

**FILE**: `Gamedata\Simple_NPC.ats`
**TYPE**: EntityPrefab (7 nodes, 5 connections)

### Test Steps
1. Launch Olympe Blueprint Editor
2. Open Asset Browser (if not visible)
3. Navigate to Gamedata folder
4. **Double-click Simple_NPC.ats**
5. Capture console output to file:
   ```powershell
   # Copy console output and save to test_log_ep.txt
   ```

### Expected Logs (in order)

```
[TabManager::OpenFileInTab] ENTRY: filePath=...Simple_NPC.ats
[TabManager::OpenFileInTab] File not open, detected type: EntityPrefab
[TabManager::OpenFileInTab] EntityPrefabRenderer created for tab: tab_1
  [EntityPrefabRenderer::Load] ENTRY: path=...Simple_NPC.ats (call depth: 1)
  [EntityPrefabRenderer::Load] About to call PrefabLoader::LoadJsonFromFile()
  [EntityPrefabRenderer::Load] JSON loaded, size=... bytes
  [EntityPrefabRenderer::Load] Document obtained, about to call document->LoadFromFile()
  [EntityPrefabRenderer::Load] SUCCESS: loaded prefab, returning true (call depth: 1)
[TabManager::OpenFileInTab] EntityPrefabRenderer::Load() SUCCESS
[TabManager::OpenFileInTab] EntityPrefab document assigned to tab
[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs, tabID=tab_1
[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs, total tabs now: 1
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_1
```

### Expected Visual Results
- ✅ Tab appears in tab bar: "Simple_NPC"
- ✅ Canvas shows 7 component nodes
- ✅ Connections drawn between nodes
- ✅ Grid visible with nodes positioned
- ✅ ComponentPalette visible on right side

### Pass/Fail Criteria
- **PASS**: All logs appear in order, tab visible, 7 nodes rendered
- **FAIL**: Any logs missing, tab doesn't appear, nodes not rendered

---

## Test Scenario 2: BehaviorTree Loading

**FILE**: Any BehaviorTree file (e.g., `Gamedata\AI\SampleBT.ats`)
**TYPE**: BehaviorTree

### Test Steps
1. In same editor session (if possible)
2. Open Asset Browser
3. Navigate to Gamedata/AI folder
4. **Double-click BehaviorTree file** (or create new if needed)
5. Capture console output

### Expected Logs (in order)

```
[TabManager::OpenFileInTab] ENTRY: filePath=...BT_File.ats
[TabManager::OpenFileInTab] File not open, detected type: BehaviorTree
[TabManager::OpenFileInTab] BehaviorTreeRenderer created for tab: tab_2
  [BehaviorTreeRenderer::Load] ENTRY: path=...BT_File.ats (call depth: 1)
  [BehaviorTreeRenderer::Load] Closing previous graph (if applicable)
  [BehaviorTreeRenderer::Load] About to call NodeGraphManager::LoadGraph()
  [BehaviorTreeRenderer::Load] Graph loaded, id=..., setting as active
  [BehaviorTreeRenderer::Load] SUCCESS: loaded BT graph, returning true (call depth: 1)
[TabManager::OpenFileInTab] BehaviorTreeRenderer::Load() SUCCESS
[TabManager::OpenFileInTab] BehaviorTree document assigned to tab
[TabManager::OpenFileInTab] DEBUG: About to add tab to m_tabs, tabID=tab_2
[TabManager::OpenFileInTab] DEBUG: Tab successfully added to m_tabs, total tabs now: 2
[TabManager::OpenFileInTab] SUCCESS: Returning tabID=tab_2
```

### Expected Visual Results
- ✅ New tab appears in tab bar
- ✅ BehaviorTree nodes rendered on canvas
- ✅ Node connections visible
- ✅ Toolbar visible with Save button

### Pass/Fail Criteria
- **PASS**: All logs appear, tab visible, BT nodes rendered
- **FAIL**: Any logs missing, tab doesn't appear, BT nodes not rendered

---

## Test Scenario 3: Multiple Load Detection

**FILE**: Same EntityPrefab file twice
**PURPOSE**: Verify caching and detect duplicate loads

### Test Steps
1. Double-click Simple_NPC.ats
2. Wait for logs to complete
3. **IMPORTANT**: Check console log for any "DUPLICATE LOAD" or "WARNING: Already loading" messages
4. Close tab or switch tab
5. Double-click Simple_NPC.ats again (should reuse existing tab)
6. Capture console output

### Expected Behavior
- **First Load**: Full set of logs
- **Second Load**: Should see "File already open in tab" message
  ```
  [TabManager::OpenFileInTab] File already open in tab: tab_1, activating
  ```

### Pass/Fail Criteria
- **PASS**: Second load shows "already open" message, no duplicate loading
- **FAIL**: Second load repeats full loading sequence

---

## Test Scenario 4: Load Failure Detection

**FILE**: Non-existent file or corrupt JSON
**PURPOSE**: Verify error handling and logging

### Test Steps
1. Manually create tab with invalid file path (or modify console)
2. Try to load non-existent file
3. Capture error logs

### Expected Logs (on failure)
```
[TabManager::OpenFileInTab] ENTRY: filePath=...NonExistent.ats
[TabManager::OpenFileInTab] ERROR: (renderer)::Load() failed for ...
[TabManager::OpenFileInTab] Returning empty tabID (FAILURE)
[BlueprintEditor] TabManager failed to open: ...NonExistent.ats
```

### Pass/Fail Criteria
- **PASS**: Error logged clearly, no tab created, no crash
- **FAIL**: Crash, incorrect error message, or silent failure

---

## Diagnostic Log Analysis Guide

### Check 1: Is Load Being Called?
```
Search for: "[EntityPrefabRenderer::Load] ENTRY"
If found: ✅ Load() is called
If not found: ❌ Load never called - problem upstream
```

### Check 2: Is JSON Being Loaded?
```
Search for: "[EntityPrefabRenderer::Load] JSON loaded"
If found: ✅ JSON parsing works
If not found: ❌ JSON load failed - check file path, permissions
```

### Check 3: Is Document Being Processed?
```
Search for: "[EntityPrefabRenderer::Load] Document obtained"
If found: ✅ Document accessible
If not found: ❌ Document creation failed
```

### Check 4: Does Load() Return True?
```
Search for: "[EntityPrefabRenderer::Load] SUCCESS"
If found: ✅ Load() returned true
If not found: ❌ Load() failed somewhere
```

### Check 5: Is Tab Being Added?
```
Search for: "[TabManager::OpenFileInTab] Tab successfully added to m_tabs"
If found: ✅ Tab added to list
If not found: ❌ Exception during emplace_back()
```

### Check 6: Is TabID Returned?
```
Search for: "[TabManager::OpenFileInTab] SUCCESS: Returning tabID="
If found: ✅ Tab should appear in UI
If not found: ❌ Return value lost somewhere
```

### Check 7: Are There Multiple Loads?
```
Count occurrences of: "[EntityPrefabRenderer::Load] ENTRY"
If count == 1: ✅ Single load (expected)
If count > 1: ⚠️ Multiple loads detected - check for recursive calls
```

---

## Common Issues and Solutions

### Issue: Tab appears but nodes don't render
```
Cause: Tab created correctly, but Render() not called or failing
Solution: 
- Check if RenderActiveCanvas() is being called
- Verify canvas dimensions > 0
- Check for exception in renderer Render() method
```

### Issue: Multiple loads detected
```
Cause: File loaded multiple times per user action
Solution:
- Check indentation in load logs for nesting
- If nested, identify which function calls which
- Add early return or caching check
```

### Issue: Load fails silently (no error log)
```
Cause: Exception caught upstream
Solution:
- Add try-catch wrapper around renderer creation
- Log all exception messages
- Check for silent nullptr returns
```

### Issue: TabID returns empty
```
Cause: Could be several issues:
  1. std::move() issue (now FIXED)
  2. Exception in move constructor
  3. Exception in emplace_back()
  4. SetActiveTab() throws
Solution:
- Review move semantics in EditorTab (should be safe now)
- Check if exception is caught
- Add logging before/after each critical operation
```

---

## Performance Metrics

After loading file, record:
- Time from file selection to tab appearance: _____ ms
- Time from load start to "JSON loaded": _____ ms
- Time from "JSON loaded" to "SUCCESS": _____ ms
- Total load time: _____ ms

### Target Performance
- Total load time for 10-node EntityPrefab: < 100ms
- Total load time for BehaviorTree: < 200ms
- Tab appearance: < 50ms after load completion

---

## Post-Test Checklist

- [ ] EntityPrefab loads and renders (7 nodes visible)
- [ ] BehaviorTree loads and renders
- [ ] Multiple load caching works (same file→reuse tab)
- [ ] Error handling works (invalid file→no crash)
- [ ] No duplicate loads in single user action
- [ ] All expected logs appear in console
- [ ] Tab bar shows correct tab names
- [ ] Canvas shows expected node count
- [ ] No exceptions or crashes
- [ ] Performance within target range

---

## Log Export Instructions

### To capture complete log:

**Visual Studio Output Window:**
1. Run → Start Debugging (F5)
2. Perform test action
3. Right-click Output window → Select All
4. Right-click → Copy
5. Paste into test_results.txt

**Or redirect to file (if using console app):**
```
app.exe > test_log.txt 2>&1
```

---

## Success Criteria (Final)

✅ **ALL** of the following must be true:
1. Build compiles with 0 errors
2. EntityPrefab files load and tabs appear
3. BehaviorTree files load and tabs appear
4. Nodes render on canvas
5. No duplicate loads for same file
6. All diagnostic logs appear in console
7. No crashes or exceptions
8. Performance < 100ms for EntityPrefab load

---

**Phase 51 Testing**: Ready to execute
**Build Status**: ✅ Verified Successful
**Diagnostics**: ✅ Comprehensive
**Expected Outcome**: Rendering working correctly with clear log visibility
