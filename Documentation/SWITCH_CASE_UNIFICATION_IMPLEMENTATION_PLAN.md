# Switch/Case System Unification - Implementation Plan

## Phase 1: Add RenderSwitchNodeProperties to Properties Panel

### File: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp

Add after existing node property renderers (after RenderSubGraphNodeProperties):

```cpp
// ============================================================================
// Switch Node Properties (NEW - Phase 26.1)
// ============================================================================

void VisualScriptEditorPanel::RenderSwitchNodeProperties(VSEditorNode& eNode,
                                                         TaskNodeDefinition& def)
{
    // ── Blue header: node name ──────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Switch Variable ─────────────────────────────────────────────────────
    ImGui::TextDisabled("Switch On:");
    static char varNameBuf[256] = "";
    strncpy_s(varNameBuf, sizeof(varNameBuf), def.switchVariable.c_str(), _TRUNCATE);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##switch_var", varNameBuf, sizeof(varNameBuf)))
    {
        def.switchVariable = varNameBuf;
        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
        {
            if (m_template.Nodes[i].NodeID == m_selectedNodeID)
            {
                m_template.Nodes[i].switchVariable = varNameBuf;
                break;
            }
        }
        m_dirty = true;
    }

    ImGui::Separator();

    // ── Case count and edit button ──────────────────────────────────────────
    ImGui::TextDisabled("Cases: %zu", def.switchCases.size());
    ImGui::Spacing();

    if (ImGui::Button("Edit Switch Cases", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        if (m_switchCaseModal)
        {
            m_switchCaseModal->Open(def.switchCases);
        }
    }

    // Handle modal result
    if (m_switchCaseModal)
    {
        m_switchCaseModal->Render();

        if (m_switchCaseModal->IsConfirmed())
        {
            // CRITICAL FIX: Apply changes to BOTH sources of truth
            std::vector<SwitchCaseDefinition> newCases = m_switchCaseModal->GetSwitchCases();

            // 1. Update template switchCases
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].switchCases = newCases;

                    // 2. Regenerate DynamicExecOutputPins from switchCases
                    m_template.Nodes[i].DynamicExecOutputPins.clear();
                    for (size_t j = 1; j < newCases.size(); ++j)  // Skip "Case_0"
                    {
                        m_template.Nodes[i].DynamicExecOutputPins.push_back(newCases[j].pinName);
                    }
                    break;
                }
            }

            // 3. Update editor node def
            def.switchCases = newCases;
            def.DynamicExecOutputPins.clear();
            for (size_t j = 1; j < newCases.size(); ++j)  // Skip "Case_0"
            {
                def.DynamicExecOutputPins.push_back(newCases[j].pinName);
            }

            m_switchCaseModal->Close();
            m_dirty = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ── Breakpoint ──────────────────────────────────────────────────────────
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##vs_switch", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }

    RenderVerificationPanel();

    (void)eNode; // suppress unused-warning
}
```

## Phase 2: Update RenderNodePropertiesPanel Dispatcher

### File: Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp

Find `RenderNodePropertiesPanel()` and add Switch dispatch:

```cpp
void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    if (m_selectedNodeID < 0 || m_editorNodes.empty())
        return;

    // Find selected node
    TaskNodeDefinition* def = nullptr;
    VSEditorNode* eNode = nullptr;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        if (m_editorNodes[i].nodeID == m_selectedNodeID)
        {
            eNode = &m_editorNodes[i];
            def = &m_editorNodes[i].def;
            break;
        }
    }

    if (!def || !eNode)
        return;

    // Dispatch to type-specific renderer
    switch (def->Type)
    {
        case TaskNodeType::Branch:
            RenderBranchNodeProperties(*eNode, *def);
            return;

        case TaskNodeType::MathOp:
            RenderMathOpNodeProperties(*eNode, *def);
            return;

        case TaskNodeType::Switch:  // ← NEW
            RenderSwitchNodeProperties(*eNode, *def);
            return;

        case TaskNodeType::While:
            RenderWhileNodeProperties();
            return;

        case TaskNodeType::ForEach:
            RenderForEachNodeProperties();
            return;

        case TaskNodeType::SubGraph:
            RenderSubGraphNodeProperties();
            return;

        default:
            // Generic node properties (name, parameters, etc.)
            break;
    }

    // Generic properties for other node types
    ImGui::TextDisabled("Node Properties");
    ImGui::Separator();
    // ... other fields
}
```

## Phase 3: Fix Modal Members (Incomplete in Header)

### File: Source/Editor/Modals/SwitchCaseEditorModal.h

Add missing private members after line 149:

```cpp
private:
    /// ... existing state ...

    /// Text buffers for ImGui::InputText (one per case)
    std::vector<std::string> m_caseValueBuffers;
    std::vector<std::string> m_caseLabelBuffers;
};
```

## Phase 4: Fix Serialization

### File: Source/TaskSystem/TaskGraphLoader.cpp

In ParseSchemaV4(), after loading a Switch node:

```cpp
// After loading node from JSON (inside ParseSchemaV4):
if (node.Type == TaskNodeType::Switch)
{
    // CRITICAL FIX: Regenerate DynamicExecOutputPins from switchCases
    // This ensures that cases loaded from JSON are immediately visible on canvas
    node.DynamicExecOutputPins.clear();
    for (size_t i = 1; i < node.switchCases.size(); ++i)  // Skip "Case_0"
    {
        node.DynamicExecOutputPins.push_back(node.switchCases[i].pinName);
    }

    SYSTEM_LOG << "[TaskGraphLoader] Regenerated DynamicExecOutputPins for Switch node " 
               << node.NodeID << " from " << node.switchCases.size() << " switchCases\n";
}
```

## Phase 5: Fix Dynamic Pin Callbacks

### File: Source/BlueprintEditor/VisualScriptEditorPanel_Canvas.cpp

In RenderCanvas(), modify the onAddPin callback (around line 216):

```cpp
// BEFORE (line 216-223):
[](int nid, void* ud) {
    VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);
    panel->m_pendingAddPin = true;
    panel->m_pendingAddPinNodeID = nid;
},

// AFTER (FIX):
[](int nid, void* ud) {
    VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);

    // Find the node being edited
    TaskNodeDefinition* def = nullptr;
    for (size_t i = 0; i < panel->m_editorNodes.size(); ++i)
    {
        if (panel->m_editorNodes[i].nodeID == nid)
        {
            def = &panel->m_editorNodes[i].def;
            break;
        }
    }

    if (def && def->Type == TaskNodeType::Switch)
    {
        // For Switch: open modal instead of direct pin add
        // Modal provides full case definition UI
        if (panel->m_switchCaseModal)
        {
            panel->m_switchCaseModal->Open(def->switchCases);
        }
    }
    else if (def && def->Type == TaskNodeType::VSSequence)
    {
        // For VSSequence: simple add (no semantic data needed)
        panel->m_pendingAddPin = true;
        panel->m_pendingAddPinNodeID = nid;
    }
    // Other types don't support dynamic pins
},
```

Also apply similar fix to onRemovePin callback:

```cpp
// BEFORE (line 228-234):
[](int nid, int dynIdx, void* ud) {
    VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);
    panel->m_pendingRemovePin = true;
    panel->m_pendingRemovePinNodeID = nid;
    panel->m_pendingRemovePinDynIdx = dynIdx;
},

// AFTER (FIX):
[](int nid, int dynIdx, void* ud) {
    VisualScriptEditorPanel* panel = static_cast<VisualScriptEditorPanel*>(ud);

    // Find the node being edited
    TaskNodeDefinition* def = nullptr;
    for (size_t i = 0; i < panel->m_editorNodes.size(); ++i)
    {
        if (panel->m_editorNodes[i].nodeID == nid)
        {
            def = &panel->m_editorNodes[i].def;
            break;
        }
    }

    if (def && def->Type == TaskNodeType::Switch)
    {
        // For Switch: remove case via modal (safer than direct removal)
        // User must confirm deletion through modal UI
        if (panel->m_switchCaseModal && dynIdx + 1 < def->switchCases.size())
        {
            // Open modal showing current cases, user can delete
            panel->m_switchCaseModal->Open(def->switchCases);
        }
    }
    else
    {
        // For VSSequence: direct removal
        panel->m_pendingRemovePin = true;
        panel->m_pendingRemovePinNodeID = nid;
        panel->m_pendingRemovePinDynIdx = dynIdx;
    }
},
```

## Phase 6: Update Pin Helper

### File: Source/BlueprintEditor/VisualScriptEditorPanel_PinHelpers.cpp

Update comment in GetExecOutputPinsForNode (line 150):

```cpp
/**
 * @brief Get execution output pins for a node, including dynamic pins
 * 
 * This function provides the COMPLETE list of exec-out pins by:
 * 1. Getting static pins for the node type
 * 2. For Switch/VSSequence: adding dynamic pins from DynamicExecOutputPins
 *
 * PHASE 26.1 FIX: For Switch nodes, DynamicExecOutputPins are now 
 * automatically regenerated from switchCases on every modification 
 * and after load/undo. This ensures single source of truth.
 *
 * @param def The node definition to query
 * @return std::vector<std::string> List of exec output pin names including dynamic pins
 * @note Required for accurate pin counting during rendering and UID generation
 * @see GetExecOutputPins()
 */
std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPinsForNode(
    const TaskNodeDefinition& def) const
{
    std::vector<std::string> pins = GetExecOutputPins(def.Type);
    if (def.Type == TaskNodeType::VSSequence || def.Type == TaskNodeType::Switch)
    {
        for (size_t i = 0; i < def.DynamicExecOutputPins.size(); ++i)
            pins.push_back(def.DynamicExecOutputPins[i]);
    }
    return pins;
}
```

## Phase 7: Declare RenderSwitchNodeProperties

### File: Source/BlueprintEditor/VisualScriptEditorPanel.h

Add declaration in the properties panel section (around line 340):

```cpp
    /// Phase 26.1 — Switch node dedicated properties renderer
    void RenderSwitchNodeProperties(VSEditorNode& eNode, TaskNodeDefinition& def);
```

## Validation Checklist

### Basic Functionality
- [ ] Create new Switch node → shows default "Case_0" on canvas
- [ ] Edit → Properties panel shows "Edit Switch Cases" button
- [ ] Click button → modal opens with single case
- [ ] Add case via modal → shows new case name in modal table
- [ ] Click Apply → case appears on canvas as new pin
- [ ] Delete case via modal → pin removed from canvas
- [ ] Cancel modal → no changes applied

### Data Integrity
- [ ] Save graph after adding cases → file contains switchCases
- [ ] Load graph → all cases visible on canvas
- [ ] Load graph → case values/labels preserved
- [ ] Undo case addition → switchCases reverted
- [ ] Redo case addition → switchCases restored

### Pin Rendering
- [ ] Case pins show correct names (not just "Case_1", "Case_2")
- [ ] Case pins show custom labels if set
- [ ] No orphaned pins after modal edits
- [ ] Canvas panning doesn't affect pins

### Runtime
- [ ] Graph execution uses correct case path matching
- [ ] Case values tested with int/string/float variables
- [ ] No crashes during case evaluation

### Edge Cases
- [ ] Empty switchCases (just "Case_0") - valid state
- [ ] Large number of cases (20+) - renders with scrolling
- [ ] Special characters in case values - preserved correctly
- [ ] Duplicate case values - accepted (warning in validator)

---

## Commit Message

```
Phase 26.1: Unify Switch/Case editing systems (Modal + Dynamic Pins)

CRITICAL FIX: Switch node case management used two incompatible systems:
- Modal editor (SwitchCaseEditorModal) with rich semantic data
- Dynamic pin buttons with simple string list

These systems NEVER SYNCHRONIZED, causing data loss.

Changes:
1. Added RenderSwitchNodeProperties() - dedicated properties panel for Switch
2. Integrated SwitchCaseEditorModal into properties panel
3. Fixed modal Apply to sync BOTH switchCases + DynamicExecOutputPins
4. Fixed serialization to regenerate DynamicExecOutputPins from switchCases on load
5. Fixed [+][-] callbacks to open modal for Switch nodes
6. Updated comments to clarify single source of truth: switchCases

Fixes data loss when switching between modal and button editing.
Ensures case pins render correctly after load/undo/modal edits.
Graph execution now guaranteed consistent with visible pins.

Testing:
- Modal add/delete/reorder cases
- Save/load cycle preserves all case data
- Undo/redo maintains consistency
- Runtime case matching validated
```

---

## Success Criteria

✅ **Before Implementation**:
- [ ] Review AUDIT_SWITCH_CASE_SYSTEM.md with team
- [ ] Approve implementation plan
- [ ] Create feature branch: `feature/switch-unification`

✅ **After Implementation**:
- [ ] All 8 compilation errors fixed
- [ ] All 7 validation tests pass
- [ ] Modal + buttons + save/load cycle works end-to-end
- [ ] No data loss in any edit scenario
- [ ] Undo/redo consistent for case operations
- [ ] Code review approved
- [ ] Merge to master
- [ ] Documentation updated in Copilot instructions

---

END IMPLEMENTATION PLAN
