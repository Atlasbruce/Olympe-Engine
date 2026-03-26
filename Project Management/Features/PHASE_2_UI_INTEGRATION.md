# Phase 2 Implementation: UI Editor Integration

## Objective
Integrate GlobalTemplateBlackboard and EntityBlackboard into VisualScriptEditorPanel to provide split Local/Global variable editing interface.

## Changes Required

### 1. VisualScriptEditorPanel.h Updates

**Add member variables:**
```cpp
// Phase 24 Global Blackboard Integration
std::unique_ptr<EntityBlackboard> m_entityBlackboard;
```

**Add methods:**
```cpp
private:
    void RenderGlobalVariablesPanel();
    void RenderLocalVariablesPanel();
    void RenderBlackboardHeader();
    void RenderAddVariableDialog();
```

### 2. VisualScriptEditorPanel.cpp Updates

**Initialize() method:**
- Create EntityBlackboard instance
- Call EntityBlackboard::Initialize() with m_template

**RenderRightPanel() refactor:**
- Split Local Blackboard panel into two tabs: "Local Variables" and "Global Variables"
- Tab switching UI with ImGui::RadioButton() or similar
- Each tab renders appropriate variable list

**New method: RenderGlobalVariablesPanel()**
- Display global variables from GlobalTemplateBlackboard::Get()
- Show: Key, Type, Current Value (per entity), Global flag
- Add/Remove buttons (managed at global scope)
- Sync changes to EntityBlackboard m_globalVars

**New method: RenderLocalVariablesPanel()**
- Keep existing implementation but refactor to new location
- Rename if needed for consistency

**SaveAs() / Save() enhancement:**
- Serialize global variable values via EntityBlackboard::ExportGlobalsToJson()
- Store in graph JSON under "globalVariableValues" key
- Load on graph open via EntityBlackboard::ImportGlobalsFromJson()

### 3. Integration Points

**Blackboard panel layout (3-part split):**
```
┌─────────────────────────┐
│ Node Properties         │  (Part A - existing)
├─────────────────────────┤
│ [Local] [Global] Tabs   │  (NEW tab selector)
├─────────────────────────┤
│ Variable list content   │  (Part B - switched based on tab)
├─────────────────────────┤
│ Add/Remove buttons      │  (Part B footer)
└─────────────────────────┘
```

### 4. Global Variables Panel Features

**Display:**
- Variable name (read-only, from registry)
- Type (read-only, from registry)
- Current value (editable, entity-specific)
- Default value (read-only, from registry)
- Description (read-only, from registry)
- "Persistent" flag (read-only)

**Editing:**
- Double-click value field to edit
- Type-specific input (Bool checkbox, Int/Float spinners, String text box)
- Changes only affect current EntityBlackboard instance
- Changes NOT saved to GlobalTemplateBlackboard (registry is read-only)

**Actions:**
- No add/remove at entity level (global variables managed via GlobalTemplateBlackboard)
- If user wants new global: must go through Global Config editor (future feature)

### 5. Local Variables Panel (Existing, Refactored)

**No functional changes:**
- Keep current add/remove logic
- Keep current type selector
- Keep current value editor
- Just move to tab-based UI

### 6. Data Flow

**On graph load:**
```
LoadTemplate()
  ↓
m_entityBlackboard = new EntityBlackboard(0)  // entity ID 0 for editor context
  ↓
m_entityBlackboard->Initialize(m_template)
  ↓
LocalBlackboard initialized with local vars
GlobalTemplateBlackboard merged into m_entityBlackboard
EntityBlackboard state now contains both local + global
```

**On graph save:**
```
Save() / SaveAs()
  ↓
json globalVars = m_entityBlackboard->ExportGlobalsToJson()
  ↓
m_template.globalVariableValues = globalVars  (if we add this field)
  ↓
SerializeAndWrite() includes globalVariableValues in JSON
```

**On graph reopen:**
```
LoadTemplate()
  ↓
Parse localVars from m_template.Blackboard (existing)
  ↓
EntityBlackboard->Initialize(m_template)  (loads globals)
  ↓
if "globalVariableValues" exists in JSON:
    EntityBlackboard->ImportGlobalsFromJson()  (restore entity-specific overrides)
```

## Implementation Checklist

- [ ] Add EntityBlackboard member to VisualScriptEditorPanel
- [ ] Initialize EntityBlackboard in Initialize()
- [ ] Refactor RenderRightPanel() to add tab selector
- [ ] Implement RenderGlobalVariablesPanel()
- [ ] Move existing local variables code to RenderLocalVariablesPanel()
- [ ] Add JSON serialization for global variable values in Save/SaveAs
- [ ] Add JSON deserialization in LoadTemplate
- [ ] Test global variable persistence across save/load cycles
- [ ] Test scope resolution in GetBBValue/SetBBValue nodes with (G) prefix
- [ ] Add validation for duplicate names across local+global scope

## Risk Mitigation

**Risk:** Global variables appear editable but are actually read-only at entity scope
- **Mitigation:** Make value fields read-only initially, with "override" toggle if needed in Phase 4

**Risk:** Unsaved global changes lost on reload
- **Mitigation:** Make clear in UI that global edits require GlobalTemplateBlackboard config update (separate tool)

**Risk:** Performance impact from loading EntityBlackboard for every graph
- **Mitigation:** EntityBlackboard is lightweight (just maps), should be negligible

## Testing Strategy

1. **Unit tests:**
   - EntityBlackboard initialization with globals
   - Scope resolution (G)VarName vs bareVarName
   - JSON import/export round-trip

2. **Integration tests:**
   - Load graph with local + global variables
   - Edit local variable value
   - Save and reload, verify both locals + globals persist
   - Open second graph with different globals, verify no cross-contamination

3. **UI tests:**
   - Tab switching between Local/Global views
   - Verify correct variables appear in each tab
   - Verify override behavior (if implemented)

## Timeline Estimate
- UI layout & tab system: 1-2 hours
- GlobalVariablesPanel rendering: 1-2 hours  
- JSON serialization integration: 1 hour
- Testing & iteration: 1-2 hours
- **Total: 4-7 hours**
