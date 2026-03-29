# 📍 METHOD LINE MAPPING - VisualScriptEditorPanel.cpp

## File Analysis
- **Total Lines**: 5527
- **Methods Found**: 61+
- **File Sections**: 10 major domains

---

## Line-by-Line Method Mapping

### Group 1: CORE (Constructor, Destructor, Initialize, Shutdown)
✅ **ALREADY MIGRATED to _Core.cpp**

```
Lines  42-51  : VisualScriptEditorPanel::VisualScriptEditorPanel()
Lines  52-139 : void VisualScriptEditorPanel::Initialize()
Lines 140-166 : void VisualScriptEditorPanel::Shutdown()
Lines 167-181 : (misc helpers for Core)
```

---

### Group 2: CANVAS (ImNodes, positions, sync)
➡️ **TO MIGRATE to _Canvas.cpp**

```
Lines 185-200 : Attribute UID mapping (ExecInAttrUID, ExecOutAttrUID, DataInAttrUID, DataOutAttrUID)
Lines 209-243 : Pin queries (GetExecInputPins, GetExecOutputPins)
Lines 245-256 : GetExecOutputPinsForNode
Lines 257-283 : GetDataInputPins, GetDataOutputPins
Lines 457-528 : SyncCanvasFromTemplate()
Lines 529-650+ : SyncTemplateFromCanvas()
```

**Methods**: ~15 methods | LOC: ~600-800

---

### Group 3: CONNECTIONS (Links, pins, validation)
➡️ **TO MIGRATE to _Connections.cpp**

```
Lines 413-432 : void VisualScriptEditorPanel::ConnectExec()
Lines 433-456 : void VisualScriptEditorPanel::ConnectData()
Lines ??? : void VisualScriptEditorPanel::RemoveLink()
Lines ??? : void VisualScriptEditorPanel::RebuildLinks()
```

**Methods**: ~6 methods | LOC: ~500-700

---

### Group 4: NODE MANAGEMENT (Add/Remove)
➡️ **TO MIGRATE to _Interaction.cpp**

```
Lines 284-394 : int VisualScriptEditorPanel::AddNode()
Lines 395-412 : void VisualScriptEditorPanel::RemoveNode()
```

**Methods**: 2 core + helpers | LOC: ~150

---

### Group 5: RENDERING (Toolbar, palette, canvas, menus)
➡️ **TO MIGRATE to _Rendering.cpp**

```
Lines ??? : void RenderToolbar()
Lines ??? : void RenderNodePalette()
Lines ??? : void RenderCanvas()
Lines ??? : void RenderContextMenus()
Lines ??? : void RenderSaveAsDialog()
Lines ??? : void RenderValidationOverlay()
```

**Methods**: ~6 methods | LOC: ~800-900

---

### Group 6: PROPERTIES PANEL (node properties, conditions, variable selectors)
➡️ **TO MIGRATE to _NodeProperties.cpp**

```
Lines ??? : void RenderProperties()
Lines ??? : void RenderNodePropertiesPanel()
Lines ??? : void RenderBranchNodeProperties()
Lines ??? : void RenderMathOpNodeProperties()
Lines ??? : void RenderNodeDataParameters()
Lines ??? : void RenderConditionEditor()
Lines ??? : void RenderVariableSelector()
Lines ??? : void RenderConstValueInput()
Lines ??? : void RenderPinSelector()
Lines ??? : static std::string BuildConditionPreview()
```

**Methods**: ~11 methods | LOC: ~1000-1200 (LARGEST)

---

### Group 7: BLACKBOARD (variables, local/global)
➡️ **TO MIGRATE to _Blackboard.cpp**

```
Lines ??? : void RenderBlackboard()
Lines ??? : void RenderLocalVariablesPanel()
Lines ??? : void RenderGlobalVariablesPanel()
Lines ??? : void CommitPendingBlackboardEdits()
Lines ??? : void ValidateAndCleanBlackboardEntries()
Lines ??? : static std::vector<BlackboardEntry> GetVariablesByType()
```

**Methods**: ~5-6 methods | LOC: ~600-700

---

### Group 8: FILE OPERATIONS (Save, Load, Serialize)
➡️ **TO MIGRATE to _FileOps.cpp**

```
Lines ??? : void LoadTemplate()
Lines ??? : bool Save()
Lines ??? : bool SaveAs()
Lines ??? : bool SerializeAndWrite()
Lines ??? : void SyncNodePositionsFromImNodes()
Lines ??? : void SyncPresetsFromRegistryToTemplate()
Lines ??? : void RenderSaveAsDialog()
Lines ??? : void ResetViewportBeforeSave()
Lines ??? : void AfterSave()
```

**Methods**: ~9 methods | LOC: ~700-800

---

### Group 9: VERIFICATION (validation, testing - Phase 24.3)
➡️ **TO MIGRATE to _Verification.cpp**

```
Lines ??? : void RunVerification()
Lines ??? : void RenderVerificationPanel()
Lines ??? : void RenderValidationOverlay()
Lines ??? : void RenderVerificationLogsPanel()
Lines ??? : std::vector<ValidationError> RunExecutionTest()
Lines ??? : ExecutionTestPanel& GetExecutionTestPanel()
```

**Methods**: ~5-6 methods | LOC: ~500-600

---

### Group 10: PRESETS (Phase 24 - condition presets, operands)
➡️ **TO MIGRATE to _Presets.cpp**

```
Lines ??? : void RenderPresetBankPanel()
Lines ??? : void RenderPresetItemCompact()
Lines ??? : bool RenderOperandEditor()
```

**Methods**: ~3 methods | LOC: ~400-500

---

## 📊 Summary

```
_Core.cpp               ✅ 180 LOC   (Constructor, Init, Shutdown, Render)
_Canvas.cpp            ⏳ 600-800 LOC
_Connections.cpp       ⏳ 500-700 LOC
_Rendering.cpp         ⏳ 800-900 LOC
_NodeProperties.cpp    ⏳ 1000-1200 LOC  (LARGEST)
_Blackboard.cpp        ⏳ 600-700 LOC
_FileOps.cpp           ⏳ 700-800 LOC
_Verification.cpp      ⏳ 500-600 LOC
_Presets.cpp           ⏳ 400-500 LOC
_Interaction.cpp       ⏳ 150-200 LOC
─────────────────────────────
TOTAL                  ~6,380 LOC
```

**Next**: Extract exact line ranges and create each file systematically.
