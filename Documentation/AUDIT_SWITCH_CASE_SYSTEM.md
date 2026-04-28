# AUDIT: Switch Node Case Editing System - Unified Architecture Fix

## Executive Summary
The Blueprint Editor has **TWO INCOMPATIBLE systems** for editing Switch node cases:
1. **Modal System**: `SwitchCaseEditorModal` - semantically rich (value, pinName, customLabel)
2. **Dynamic Pin System**: `DynamicExecOutputPins` - simple string list for rendering

These systems **NEVER SYNCHRONIZE**, causing data loss and inconsistent behavior.

---

## Current System Architecture (BROKEN)

### System A: Modal Editor (`SwitchCaseEditorModal.h/cpp`)
**Location**: `Source/Editor/Modals/SwitchCaseEditorModal`

**Data Structure**:
- Works with `std::vector<SwitchCaseDefinition>` (source of truth intent)
- `SwitchCaseDefinition` contains:
  - `value`: match value (e.g., "0", "10", "Patrol")
  - `pinName`: internal pin name (e.g., "Case_0")
  - `customLabel`: display label (e.g., "Default" instead of "Case_0")

**Lifecycle**:
1. Open() - copies node's switchCases into m_editingCases
2. Render() - displays editable table with [^] [v] [X] buttons
3. IsConfirmed() - returns true if user clicked Apply
4. GetSwitchCases() - returns modified copy

**CRITICAL BUG**: After user clicks "Apply", there's NO CODE to:
- Update the template's TaskNodeDefinition.switchCases
- Regenerate DynamicExecOutputPins
- Update editor node def
- Mark template dirty for serialization

### System B: Dynamic Pin Buttons (`VisualScriptNodeRenderer`)
**Location**: Canvas rendering in `VisualScriptEditorPanel_Canvas.cpp`

**Data Structure**:
- Uses `TaskNodeDefinition.DynamicExecOutputPins` (vector<string>)
- Only stores pin names, NOT semantic data (no value, no customLabel)

**Lifecycle**:
1. GetExecOutputPinsForNode() reads DynamicExecOutputPins
2. VisualScriptNodeRenderer renders [+] button
3. onAddPin callback fired when user clicks [+]
4. Creates new "Case_N" without semantic value assignment

**CRITICAL BUG**: Clicking [+] only appends to DynamicExecOutputPins, but:
- Does NOT create SwitchCaseDefinition entry
- Does NOT assign match value
- Orphaned pins on graph when cases modified via modal

### System C: Serialization (BROKEN)
**Location**: `TaskGraphLoader::ParseSchemaV4()` + `SerializeAndWrite()`

**Problem**:
- Saves/loads TaskNodeDefinition.switchCases (the rich data)
- **BUT** never touches DynamicExecOutputPins during load
- After loading, DynamicExecOutputPins is empty
- Canvas shows NO pins until user adds one dynamically
- Switching between modal and runtime loses pins

---

## Identified Bugs

### BUG-1: Two Sources of Truth (DATA LOSS)
**Severity**: CRITICAL

**Issue**: 
- switchCases = semantic case definitions
- DynamicExecOutputPins = rendering string list
- When user edits via modal → switchCases updated BUT DynamicExecOutputPins stale
- Canvas renders only DynamicExecOutputPins → pins disappear
- When user adds pin via [+] → DynamicExecOutputPins updated BUT switchCases missing entry
- Save/load: switchCases persisted, DynamicExecOutputPins lost

**Impact**:
- Users lose case definitions when switching modal ↔ buttons
- Graph execution fails due to missing case pins
- Undo/redo inconsistent

---

### BUG-2: No Properties Panel for Switch
**Severity**: HIGH

**Issue**:
- Branch has RenderBranchNodeProperties()
- MathOp has RenderMathOpNodeProperties()  
- **Switch has NO specialized properties panel**
- Generic renderer doesn't handle Switch semantics

**Impact**:
- No "Edit Switch Cases" button in Properties panel
- Users must use dynamic [+] buttons (incomplete UX)
- Cannot edit match values from Properties panel

---

### BUG-3: Modal Integration Missing
**Severity**: HIGH

**Issue**:
- Modal::IsConfirmed() returns true but caller doesn't update template
- No code in properties panel to apply modal changes to graph
- Modal data returned but orphaned

**Impact**:
- Modal edits silently discarded
- Users think changes saved but nothing persisted

---

### BUG-4: Inconsistent Serialization
**Severity**: HIGH

**Issue**:
- Saves switchCases correctly
- Loads switchCases correctly
- **BUT** DynamicExecOutputPins never regenerated from switchCases after load
- Canvas is empty post-load until user manually adds pins

**Impact**:
- Saved graphs load with NO visible case pins
- Execution finds cases but rendering broken
- User must manually recreate pins by clicking [+]

---

### BUG-5: Runtime Execution Broken
**Severity**: CRITICAL

**Issue**:
- VSGraphExecutor::ExecuteFrame() uses switchCases to find matching case output pin
- If DynamicExecOutputPins empty and switchCases populated → runtime finds case but canvas pin is orphaned
- If DynamicExecOutputPins populated but switchCases empty → rendering works but case matching fails

**Impact**:
- Graph executes wrong path or crashes
- Case selection logic detached from pin rendering

---

## Root Cause Analysis

### Architectural Decision Tree Failure
```
ORIGINAL DESIGN INTENT:
  Switch node can have user-added cases
  → Need dynamic pins (VSSequence model)
  → Used DynamicExecOutputPins (simple string list)

PHASE 22-A EXPANSION:
  Switch cases need semantic data (value, customLabel)
  → Added switchCases (vector<SwitchCaseDefinition>)
  → **BUT** developer assumed one source would feed the other
  → Developer was WRONG - both exist independently

CONSEQUENCE:
  Two parallel systems evolved without sync layer
  → Modal reads switchCases, modifies, returns orphaned data
  → Canvas reads DynamicExecOutputPins, shows stale pins
  → Serialization saves switchCases, ignores DynamicExecOutputPins
  → No code path to regenerate one from the other
```

### Why This Wasn't Caught
- **Phase boundaries**: Modal added in Phase 26, dynamic pins pre-existed
- **No regression test**: No test switches between modal and buttons
- **Incomplete code review**: Modal left as standalone without integration
- **Missing RenderSwitchNodeProperties**: No visible place to call modal

---

## Unified Solution Architecture

### Design Principle: Single Source of Truth
```
switchCases (vector<SwitchCaseDefinition>) = THE AUTHORITY
  ├─ Contains all semantic data (value, pinName, customLabel)
  ├─ Serialized to/from JSON (durable)
  └─ Generated from runtime as needed

DynamicExecOutputPins (vector<string>) = DERIVED CACHE
  ├─ Computed from switchCases on every render pass
  ├─ Used ONLY for canvas rendering
  └─ Rebuilt after any switchCases modification
```

### Step 1: Create Switch Case Manager API
**New Class**: `SwitchCaseManager` (in VisualScriptEditorPanel)

```cpp
class SwitchCaseManager {
public:
    // Authority operations - modify switchCases
    void AddCase(int nodeID, const std::string& value, const std::string& customLabel = "");
    void RemoveCase(int nodeID, size_t caseIndex);
    void UpdateCase(int nodeID, size_t caseIndex, 
                    const std::string& value, const std::string& customLabel);
    void ReorderCases(int nodeID, const std::vector<SwitchCaseDefinition>& newOrder);

    // Query operations
    std::vector<SwitchCaseDefinition> GetCases(int nodeID);
    std::vector<std::string> GetCasePinNames(int nodeID);  // Derived from switchCases

    // Sync operation - regenerate DynamicExecOutputPins from switchCases
    void SyncDynamicPinsFromCases(int nodeID);

    // Batch operation - called after modal Apply or undo/redo
    void ApplyCaseChanges(int nodeID, const std::vector<SwitchCaseDefinition>& newCases);

private:
    VisualScriptEditorPanel* m_editor;

    // Helper: find node by ID
    TaskNodeDefinition* FindSwitchNode(int nodeID);
};
```

### Step 2: Implement RenderSwitchNodeProperties()
**Location**: `VisualScriptEditorPanel_Properties.cpp`

```cpp
void VisualScriptEditorPanel::RenderSwitchNodeProperties(VSEditorNode& eNode, 
                                                          TaskNodeDefinition& def)
{
    // Blue header (matching Branch/MathOp pattern)
    ImGui::PushStyleColor(...);
    ImGui::Selectable(def.NodeName.c_str(), true, ...);
    ImGui::PopStyleColor(4);
    ImGui::Separator();
    ImGui::Spacing();

    // Switch variable selector
    ImGui::TextDisabled("Switch On Variable:");
    // Dropdown of Int/String blackboard variables

    ImGui::Separator();
    ImGui::TextDisabled("Cases (%zu):", def.switchCases.size());

    // "Edit Switch Cases" button
    if (ImGui::Button("Edit Switch Cases", ImVec2(200, 0)))
    {
        m_switchCaseModal->Open(def.switchCases);
    }

    // Handle modal
    m_switchCaseModal->Render();
    if (m_switchCaseModal->IsConfirmed())
    {
        // UPDATE both sources of truth
        std::vector<SwitchCaseDefinition> newCases = m_switchCaseModal->GetSwitchCases();
        m_switchCaseManager->ApplyCaseChanges(eNode.nodeID, newCases);
        m_switchCaseModal->Close();
        m_dirty = true;
    }

    ImGui::Separator();
    ImGui::Spacing();

    RenderVerificationPanel();
}
```

### Step 3: Fix Dynamic Pin Callbacks
**Location**: `VisualScriptEditorPanel_Canvas.cpp` (RenderCanvas)

```cpp
// Current (BROKEN):
VisualScriptNodeRenderer::RenderNode(
    eNode.nodeID, ...,
    [](int nid, void* ud) {
        VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);
        panel->m_pendingAddPin = true;  // ← Just marks flag, no update!
    },
    this, ...
);

// Fixed:
VisualScriptNodeRenderer::RenderNode(
    eNode.nodeID, ...,
    [](int nid, void* ud) {
        VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);
        // For Switch nodes, open modal instead of direct pin add
        TaskNodeDefinition* def = panel->FindNodeDef(nid);
        if (def && def->Type == TaskNodeType::Switch)
        {
            panel->m_switchCaseModal->Open(def->switchCases);
        }
        else if (def && def->Type == TaskNodeType::VSSequence)
        {
            // VSSequence: simple add without modal
            panel->m_pendingAddPin = true;
            panel->m_pendingAddPinNodeID = nid;
        }
    },
    this, ...
);
```

### Step 4: Fix Serialization
**Location**: `TaskGraphLoader::ParseSchemaV4()`

```cpp
// After loading node from JSON:
if (node.Type == TaskNodeType::Switch)
{
    // IMPORTANT: Regenerate DynamicExecOutputPins from switchCases
    node.DynamicExecOutputPins.clear();
    for (const auto& casedef : node.switchCases)
    {
        if (casedef.pinName.find("Case_") == 0)  // Skip base "Case_0"
            node.DynamicExecOutputPins.push_back(casedef.pinName);
    }
}
```

### Step 5: Fix Modal Integration
**Location**: `SwitchCaseEditorModal.h`

Add private fields for incomplete data:
```cpp
std::vector<std::string> m_caseValueBuffers;   // TEXT BUFFER synced with m_editingCases[i].value
std::vector<std::string> m_caseLabelBuffers;   // TEXT BUFFER synced with m_editingCases[i].customLabel
```

The modal already does this! Just need proper caller integration.

---

## Migration Path

### Phase 1: Compatibility Shim (No Breaking Changes)
1. Add SwitchCaseManager (wraps both systems)
2. Keep both switchCases + DynamicExecOutputPins populated
3. Add RenderSwitchNodeProperties() but make it optional

### Phase 2: Unified Editing
1. Wire modal to RenderSwitchNodeProperties()
2. Disable raw [+][-] buttons for Switch nodes (redirect to modal)
3. Sync after every modal Apply

### Phase 3: Complete Migration
1. Regenerate DynamicExecOutputPins on every switchCases change
2. Remove orphan dynamic pin code paths
3. Test full undo/redo cycle with cases

---

## Testing Checklist

- [ ] Add case via modal → case appears on canvas
- [ ] Delete case via modal → pin removed from canvas immediately
- [ ] Reorder cases via modal → canvas reflects new order
- [ ] Save graph → load graph → cases present and correct
- [ ] Edit via modal → switch to buttons [+][-] → data consistent
- [ ] Undo case changes → switchCases + canvas both reverted
- [ ] Runtime execution uses correct case path
- [ ] No orphaned pins after edits
- [ ] No data loss on modal Apply/Cancel cycle

---

## Files to Modify

1. **TaskGraphTemplate.h** - Consider deprecating DynamicExecOutputPins comment
2. **VisualScriptEditorPanel.h** - Add SwitchCaseManager member, RenderSwitchNodeProperties declaration
3. **VisualScriptEditorPanel_Properties.cpp** - Implement RenderSwitchNodeProperties()
4. **VisualScriptEditorPanel_Canvas.cpp** - Fix onAddPin callback for Switch
5. **VisualScriptEditorPanel_PinHelpers.cpp** - Update GetExecOutputPinsForNode() to use switchCases
6. **SwitchCaseEditorModal.cpp** - Ensure pinName auto-generation on Add
7. **TaskGraphLoader.cpp** - Add switchCases→DynamicExecOutputPins regeneration on load
8. **SerializeAndWrite()** - Verify switchCases correctly serialized

---

## Risk Assessment

| Risk | Mitigation |
|------|-----------|
| Undo/redo inconsistency | Wrap all SwitchCaseManager calls in Command pattern |
| Backward compatibility | Load switchCases, regenerate DynamicExecOutputPins (one-way migration) |
| Runtime regression | Add VSGraphExecutor test for case pin matching |
| Performance | DynamicExecOutputPins regeneration is O(n) once per render, acceptable |

---

## Estimated Effort

- Audit & Design: ✓ (this document)
- Implement SwitchCaseManager: 2-3 hours
- Implement RenderSwitchNodeProperties: 1 hour
- Fix serialization: 1 hour
- Fix callbacks: 1-2 hours
- Testing: 2-3 hours
- **Total: ~8-10 hours**

---

END AUDIT DOCUMENT
