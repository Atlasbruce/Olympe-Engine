# PHASE 44.4 - COMPREHENSIVE LOGGING GUIDE FOR SAVE FUNCTIONALITY

## Overview
Phase 44.4 adds extensive logging to the entire save workflow so we can identify **exactly** where the save functionality is breaking.

## Workflow with Logging Points

```
USER CLICKS SAVE BUTTON (in BehaviorTreeRenderer toolbar)
  ↓ (Frame N)
[CanvasToolbarRenderer::RenderButtons] Document dirty state: YES | Type: BehaviorTree | Path: ...
  ↓ (Button click detected)
ImGui::Button("Save") returns true
  ↓
[CanvasToolbarRenderer::RenderButtons] SAVE BUTTON CLICKED!
  ↓
OnSaveClicked() called
  ↓
[CanvasToolbarRenderer::OnSaveClicked] ENTER - document=OK
[CanvasToolbarRenderer::OnSaveClicked] Document type: BehaviorTree
[CanvasToolbarRenderer::OnSaveClicked] Current path: 'GameData/AI/test_bt.json'
  ↓ (Has filepath, proceed)
[CanvasToolbarRenderer::OnSaveClicked] Calling ExecuteSave()
  ↓
ExecuteSave() called
  ↓
[CanvasToolbarRenderer::ExecuteSave] ENTER - filePath='GameData/AI/test_bt.json'
[CanvasToolbarRenderer::ExecuteSave] Document type: BehaviorTree
[CanvasToolbarRenderer::ExecuteSave] Calling m_document->Save()
  ↓
BehaviorTreeGraphDocument::Save() called
  ↓
[BehaviorTreeGraphDocument::Save] ============ SAVE START ============
[BehaviorTreeGraphDocument::Save] Target filepath: 'GameData/AI/test_bt.json'
[BehaviorTreeGraphDocument::Save] Active graphId: 5
[BehaviorTreeGraphDocument::Save] Graph found with 8 nodes
[BehaviorTreeGraphDocument::Save] Validating graph...
[BehaviorTreeGraphDocument::Save] ✓ Validation passed
[BehaviorTreeGraphDocument::Save] Syncing node positions from ImNodes...
[BehaviorTreeGraphDocument::Save] ✓ Position sync complete
[BehaviorTreeGraphDocument::Save] Calling NodeGraphManager::SaveGraph()
  ↓
NodeGraphManager::SaveGraph() called (calls NodeGraph::ToJson() + file write)
  ↓
[BehaviorTreeGraphDocument::Save] ✓ ============ SAVE SUCCESS ============
[BehaviorTreeGraphDocument::Save] Saved to: 'GameData/AI/test_bt.json'
  ↓
[CanvasToolbarRenderer::ExecuteSave] ✓ Save succeeded
  ↓
[CanvasToolbarRenderer::OnSaveClicked] ✓ Successfully saved to: GameData/AI/test_bt.json
[CanvasToolbarRenderer::OnSaveClicked] Calling onSaveComplete callback
  ↓
SUCCESS - File written, dirty flag cleared, UI updated
```

## What to Look For - DEBUGGING CHECKLIST

### Level 1: Basic Button Clicks
- [ ] See "[CanvasToolbarRenderer::RenderButtons] Document dirty state:"?
  - If NO: m_document is null OR framework not initialized
  - If YES: Continue to Level 2

- [ ] See "SAVE BUTTON CLICKED!"?
  - If NO: Button click not being detected (ImGui issue?)
  - If YES: Continue to Level 2

### Level 2: Document State  
- [ ] See "[CanvasToolbarRenderer::OnSaveClicked] ENTER"?
  - If NO: OnSaveClicked() never called
  - If YES: Continue

- [ ] See "Document type: BehaviorTree"?
  - If NO: m_document->GetTypeName() failed
  - If YES: Continue

- [ ] See "Current path: '" with actual filepath?
  - If says empty: SaveAs dialog should open (no error - working as designed)
  - If has path: Continue to Level 3

### Level 3: Execution Path
- [ ] See "[CanvasToolbarRenderer::OnSaveClicked] Calling ExecuteSave()"?
  - If NO: Something failed before this
  - If YES: Continue

- [ ] See "[CanvasToolbarRenderer::ExecuteSave] ENTER"?
  - If NO: ExecuteSave() never called
  - If YES: Continue

- [ ] See "[CanvasToolbarRenderer::ExecuteSave] Document type: BehaviorTree"?
  - If NO: m_document is null OR became null between calls
  - If YES: Continue

- [ ] See "Calling m_document->Save()"?
  - If NO: m_document->Save() was skipped
  - If YES: Continue to Level 4

### Level 4: Backend Persistence
- [ ] See "[BehaviorTreeGraphDocument::Save] ============ SAVE START ============"?
  - If NO: BehaviorTreeGraphDocument::Save() never called - CRITICAL!
  - If YES: Continue

- [ ] See "Active graphId: X" (non-negative number)?
  - If says -1: No graph is active - CRITICAL!
  - If positive: Continue

- [ ] See "Graph found with X nodes"?
  - If NO: Graph pointer is null - CRITICAL!
  - If YES: Continue

- [ ] See "✓ Validation passed"?
  - If says "✗ VALIDATION ERROR": Graph is invalid (expected for some BTs)
  - If ✓ YES: Continue

- [ ] See "✓ Position sync complete" (optional if renderer is null)?
  - If missing but sees "⚠ Warning": Acceptable (renderer might be null)
  - If ✗ WARNING: Position sync failed (positions might be stale)
  - If YES: Continue

- [ ] See "Calling NodeGraphManager::SaveGraph()"?
  - If NO: Something failed before serialization
  - If YES: Continue

- [ ] See "✓ ============ SAVE SUCCESS ============"?
  - If YES: **FILE WAS ACTUALLY WRITTEN**!
  - If ✗ SAVE FAILED: NodeGraphManager::SaveGraph() returned false
  - If nothing after "Calling NodeGraphManager": Timeout or crash

### Level 5: UI Update
- [ ] See "[CanvasToolbarRenderer::ExecuteSave] ✓ Save succeeded"?
  - If NO: Framework got false from document
  - If YES: Continue

- [ ] See "[CanvasToolbarRenderer::OnSaveClicked] ✓ Successfully saved"?
  - If NO: Callback failed
  - If YES: Success!

- [ ] Tab still shows "*" (dirty flag)?
  - If YES: TabManager update might have failed
  - If NO: All good!

## Running the Test

1. **Open Output Window** → Build pane
2. **Click button sequence:**
   - Create new BehaviorTree (File → New → Behavior Tree)
   - Add a node (drag from palette)
   - Click [Save] button
3. **Scroll through Output** looking for the log pattern
4. **Compare** to the workflow above to find where it breaks

## Expected vs Actual

**EXPECTED (Successful Save)**:
```
[CanvasToolbarRenderer::RenderButtons] Document dirty state: YES
[CanvasToolbarRenderer::RenderButtons] SAVE BUTTON CLICKED!
[CanvasToolbarRenderer::OnSaveClicked] ENTER - document=OK
[CanvasToolbarRenderer::OnSaveClicked] Document type: BehaviorTree
[BehaviorTreeGraphDocument::Save] ✓ ============ SAVE SUCCESS ============
```

**ACTUAL (If Broken)**:
- Only shows partial logs (stops at certain point)
- Shows error messages (validation, null pointers)
- Shows "✗" instead of "✓"
- Shows nothing after certain point (crash or exception)

## Common Failures & Fixes

| Symptom | Cause | Fix |
|---------|-------|-----|
| Never see "SAVE BUTTON CLICKED!" | Button click not registered OR framework not rendering toolbar | Check if toolbar is visible, check ImGui button logic |
| Never see "[BehaviorTreeGraphDocument::Save]" | m_document is null OR OnSaveClicked not calling ExecuteSave | Check BeaviorTreeRenderer constructor creates document |
| "Active graphId: -1" | No graph is active | Create graph first OR SetActiveGraph not called |
| "✗ VALIDATION ERROR" | Graph structure is invalid | Expected for some graphs - not an error |
| "NodeGraphManager::SaveGraph() returned false" | Backend persistence failed | Check file permissions, path validity, disk space |
| Never see callback log | TabManager notification failed | Check if TabManager::OnGraphDocumentSaved exists |

## Files with New Logging

1. **Source/BlueprintEditor/Framework/CanvasToolbarRenderer.cpp**
   - RenderButtons() - Button click detection
   - OnSaveClicked() - Save workflow start
   - ExecuteSave() - Bridge to document

2. **Source/BlueprintEditor/Framework/BehaviorTreeGraphDocument.cpp**
   - Save() - Full backend workflow with validation, sync, persistence

## Next Steps After Logging

Once you run the test and see the logs:

1. **If logs show SUCCESS**: Framework works! Issue is elsewhere (maybe dirty flag not set?)
2. **If logs stop at button**: UI framework issue (ImGui rendering)
3. **If logs show ✗ validation error**: Graph might be invalid (expected for some BTs)
4. **If logs show document=NULL**: Initialization issue in BehaviorTreeRenderer
5. **If logs show graphId=-1**: NodeGraphManager not setting active graph properly

**Report back with the exact log output** and we can pinpoint the exact issue!
