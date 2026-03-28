# 🎯 PHASE 2 OPTIMIZED STRATEGY - Smart Incremental Migration

## Situation

The original `VisualScriptEditorPanel.cpp` is **5527 lines** with **61+ methods**. 

**Challenge**: Extracting and organizing all methods without error is time-consuming and risky.

**Solution**: Use a **hybrid smart approach** combining:
1. ✅ Automated file creation with method stubs
2. ✅ Systematic method extraction by domain
3. ✅ Verification at each step
4. ✅ Detailed documentation for manual review if needed

---

## 🚀 OPTIMIZED PHASE 2 PLAN

### Step 1: Create 9 File Skeletons (5 min)
Create all 9 .cpp files with proper structure, includes, and namespace wrappers.
- Each file ready to receive methods
- All necessary includes pre-added
- Organized sections marked with comments

### Step 2: Extract & Populate by Domain (30-45 min)
Using FUNCTION_MAPPING.md as guide, systematically add methods to each file:

#### File 1: _Canvas.cpp (600-800 LOC)
**Methods to include** (from FUNCTION_MAPPING.md):
- `AllocNodeID()` / `AllocLinkID()`
- `ExecInAttrUID()` / `ExecOutAttrUID()` / `DataInAttrUID()` / `DataOutAttrUID()`
- `GetExecOutputPins()` / `GetExecInputPins()` / `GetDataInputPins()` / `GetDataOutputPins()`
- `GetExecOutputPinsForNode()`
- `RenderCanvas()`
- `SyncCanvasFromTemplate()`
- `SyncTemplateFromCanvas()`
- `SyncEditorNodesFromTemplate()`
- `SyncNodePositionsFromImNodes()`
- `RebuildLinks()`
- `ScreenToCanvasPos()`
- `ResetViewportBeforeSave()` / `AfterSave()`

#### File 2: _Connections.cpp (500-700 LOC)
**Methods to include**:
- `ConnectExec()`
- `ConnectData()`
- `RemoveLink()`
- Pin query helpers (GetExecOutputPins etc - some shared with Canvas)

#### File 3: _Interaction.cpp (150-250 LOC)
**Methods to include**:
- `AddNode()`
- `RemoveNode()`
- `PerformUndo()`
- `PerformRedo()`

#### File 4: _Rendering.cpp (800-900 LOC)
**Methods to include**:
- `RenderToolbar()`
- `RenderNodePalette()`
- `RenderCanvas()` (main loop)
- `RenderContextMenus()`
- `RenderSaveAsDialog()`
- `RenderValidationOverlay()`

#### File 5: _NodeProperties.cpp (1000-1200 LOC)
**Methods to include**:
- `RenderProperties()`
- `RenderNodePropertiesPanel()`
- `RenderBranchNodeProperties()`
- `RenderMathOpNodeProperties()`
- `RenderNodeDataParameters()`
- `RenderConditionEditor()`
- `RenderVariableSelector()`
- `RenderConstValueInput()`
- `RenderPinSelector()`
- `BuildConditionPreview()`

#### File 6: _Blackboard.cpp (600-700 LOC)
**Methods to include**:
- `RenderBlackboard()`
- `RenderLocalVariablesPanel()`
- `RenderGlobalVariablesPanel()`
- `CommitPendingBlackboardEdits()`
- `ValidateAndCleanBlackboardEntries()`
- `GetVariablesByType()`

#### File 7: _FileOps.cpp (700-800 LOC)
**Methods to include**:
- `LoadTemplate()`
- `Save()`
- `SaveAs()`
- `SerializeAndWrite()`
- `SyncNodePositionsFromImNodes()`
- `SyncPresetsFromRegistryToTemplate()`
- `RenderSaveAsDialog()`
- `ResetViewportBeforeSave()`
- `AfterSave()`

#### File 8: _Verification.cpp (500-600 LOC)
**Methods to include**:
- `RunVerification()`
- `RenderVerificationPanel()`
- `RenderValidationOverlay()`
- `RenderVerificationLogsPanel()`
- `RunExecutionTest()` (Phase 24.3)
- `GetExecutionTestPanel()` (Phase 24.3)

#### File 9: _Presets.cpp (400-500 LOC)
**Methods to include**:
- `RenderPresetBankPanel()`
- `RenderPresetItemCompact()`
- `RenderOperandEditor()`

### Step 3: Verification & Compilation (10-15 min)
1. Check all 96 methods accounted for
2. Verify no orphaned code
3. Test compilation (syntax check)
4. Document any issues

---

## 📊 Timeline Estimate

```
Step 1 (Skeletons):        5 min
Step 2 (Extract/Populate):  40 min
Step 3 (Verify):            15 min
─────────────────────────────
TOTAL:                      60 min (1 hour)
```

---

## ✅ APPROACH: SMART EXTRACTION

I will:

1. **Create all 9 file skeletons** with proper structure
2. **Extract methods from original** by looking for method signatures
3. **Copy implementations** line-for-line (no logic changes)
4. **Add proper includes** for each domain
5. **Verify completeness** - ensure all 96 methods migrated
6. **Document results** - show what was moved where

---

## 🎯 SUCCESS CRITERIA FOR PHASE 2

- [x] Core.cpp created ✅
- [ ] 9 remaining .cpp files created
- [ ] All 96 methods migrated
- [ ] No orphaned code
- [ ] All necessary includes added
- [ ] Proper C++14 compliance
- [ ] Ready for Phase 3

---

## 🚀 PROCEEDING WITH SMART EXTRACTION

**Starting now**: Will create all 9 files systematically.

**Process**:
1. Create file skeleton with includes + section markers
2. Extract methods from original .cpp
3. Copy to destination file
4. Move to next file

**You will see**: Each file created, with line count and method list

---

**STATUS**: Ready to begin full Phase 2 implementation ✅

Proceed? **YES - CONTINUE** 🚀
