# 🛠️ ACTION PLAN: Fix Node Panel Issues

## Quick Fix Guide

### 🔴 CRITICAL FIX #1: While Nodes Condition System (30 min)

**File:** `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`

**Location:** Find `RenderWhileNodeProperties()` function

**Current Code (BROKEN):**
```cpp
void VisualScriptEditorPanel::RenderWhileNodeProperties()
{
    // ... find node ...
    if (!nodePtr->conditions.empty())
    {
        for (size_t ci = 0; ci < nodePtr->conditions.size(); ++ci)
        {
            // OLD LEGACY CODE - Shows conditions[] only ❌
            Condition& cond = nodePtr->conditions[ci];
            // Manual condition UI rendering...
        }
    }
}
```

**Fixed Code (SHOULD BE):**
```cpp
void VisualScriptEditorPanel::RenderWhileNodeProperties()
{
    // ... find node ...
    ImGui::TextDisabled("While Loop Configuration");
    ImGui::Separator();

    // Display node name
    char nodeName[256] = "";
    strncpy_s(nodeName, sizeof(nodeName), nodePtr->NodeName.c_str(), 
              sizeof(nodeName) - 1);
    if (ImGui::InputText("##while_name", nodeName, sizeof(nodeName)))
    {
        nodePtr->NodeName = nodeName;
        m_dirty = true;
    }

    ImGui::Separator();
    ImGui::TextDisabled("Loop Conditions (Phase 24)");
    ImGui::Separator();

    // ✅ USE PHASE 24 SYSTEM LIKE BRANCH NODES DO:
    if (m_conditionsPanel)
    {
        // Update panel with current node's data
        if (m_condPanelNodeID != m_selectedNodeID)
        {
            m_conditionsPanel->SetConditionRefs(nodePtr->conditionRefs);
            m_conditionsPanel->SetConditionOperandRefs(nodePtr->conditionOperandRefs);
            m_conditionsPanel->SetDynamicPins(nodePtr->dynamicPins);
            m_conditionsPanel->SetNodeName(nodePtr->NodeName);
            m_condPanelNodeID = m_selectedNodeID;
        }

        // Render the conditions panel
        m_conditionsPanel->Render();

        // Check if dirty and sync back to node
        if (m_conditionsPanel->IsDirty())
        {
            nodePtr->conditionRefs = m_conditionsPanel->GetConditionRefs();
            nodePtr->conditionOperandRefs = m_conditionsPanel->GetConditionOperandRefs();
            m_conditionsPanel->ClearDirty();
            m_dirty = true;

            // Sync to template
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].conditionRefs = nodePtr->conditionRefs;
                    m_template.Nodes[i].conditionOperandRefs = nodePtr->conditionOperandRefs;
                    break;
                }
            }
        }
    }

    ImGui::Separator();
}
```

**Testing:**
```cpp
TEST(WhileNodeSync, ConditionsConsistent) {
    // Create While node with Phase 24 preset conditions
    auto node = CreateWhileNode();
    node.conditionRefs = {/* preset refs */};
    
    // Load in editor
    editor.LoadTemplate(&node);
    
    // Edit condition via panel
    ASSERT_TRUE(editor.GetNode(id).conditionRefs.size() > 0);
    ASSERT_TRUE(editor.GetNode(id).dynamicPins.size() > 0);
    
    // Save and reload
    editor.Save();
    auto reloaded = LoadTemplate(path);
    ASSERT_EQ(reloaded.conditionRefs, node.conditionRefs);
}
```

---

### 🔴 CRITICAL FIX #2: ForEach Node Dispatcher (15 min)

**File:** `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`

**Location:** Find `RenderNodePropertiesPanel()` dispatcher switch statement

**Current Code (MISSING):**
```cpp
void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    // ...
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:    { /* ... */ break; }
        case TaskNodeType::Delay:          { /* ... */ break; }
        case TaskNodeType::MathOp:         { /* ... */ break; }
        case TaskNodeType::GetBBValue:     { /* ... */ break; }
        case TaskNodeType::SetBBValue:     { /* ... */ break; }
        // ❌ MISSING: case TaskNodeType::ForEach
        case TaskNodeType::Switch:         { /* ... */ break; }
        default: { /* ... */ break; }
    }
}
```

**Fixed Code (ADD THIS):**
```cpp
void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    // ...
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:    { /* ... */ break; }
        case TaskNodeType::Delay:          { /* ... */ break; }
        case TaskNodeType::MathOp:         { /* ... */ break; }
        case TaskNodeType::GetBBValue:     { /* ... */ break; }
        case TaskNodeType::SetBBValue:     { /* ... */ break; }
        
        // ✅ ADD THIS CASE:
        case TaskNodeType::ForEach:
        {
            RenderForEachNodeProperties();
            break;
        }
        
        case TaskNodeType::Switch:         { /* ... */ break; }
        default: { /* ... */ break; }
    }
}
```

---

### 🔴 CRITICAL FIX #3: Audit GetBBValue/SetBBValue Panels (2-4 hours)

**Files:** 
- `Source/Editor/Panels/GetBBValuePropertyPanel.h/cpp`
- `Source/Editor/Panels/SetBBValuePropertyPanel.h/cpp`

**Checklist:**

```
GetBBValuePropertyPanel::Render()
├─ [ ] Displays current BBKey with tooltip
├─ [ ] Shows available variables dropdown
├─ [ ] Filters variables by type
├─ [ ] Updates BBKey on selection
├─ [ ] Syncs to template.Nodes[].BBKey
├─ [ ] Sets m_dirty = true
└─ [ ] Handles invalid keys gracefully

SetBBValuePropertyPanel::Render()
├─ [ ] Displays current BBKey with tooltip
├─ [ ] Shows available variables dropdown
├─ [ ] Filters variables by type
├─ [ ] Updates BBKey on selection
├─ [ ] Displays data value input (or via pin)
├─ [ ] Syncs to template.Nodes[].BBKey
├─ [ ] Sets m_dirty = true
└─ [ ] Handles invalid keys gracefully
```

**Audit Template:**
```cpp
// For each panel, verify:

// 1. Member variables initialized in constructor
GetBBValuePropertyPanel::GetBBValuePropertyPanel()
{
    m_selectedBBKey = "";
    m_dirty = false;
    // ... etc
}

// 2. SetNodeName, SetBBKey methods exist
void SetNodeName(const std::string& name) { m_nodeName = name; }
void SetBBKey(const std::string& key) { m_selectedBBKey = key; }

// 3. Render() updates UI AND tracks dirty
void Render()
{
    // Display selector
    if (ImGui::BeginCombo("Variable", m_selectedBBKey.c_str())) {
        // Dropdown with all variables
        if (ImGui::Selectable(var.c_str())) {
            if (var != m_selectedBBKey) {
                m_selectedBBKey = var;
                m_dirty = true;  // ← IMPORTANT
            }
        }
    }
}

// 4. GetBBKey() returns current selection
const std::string& GetBBKey() const { return m_selectedBBKey; }

// 5. IsDirty() / ClearDirty() for sync
bool IsDirty() const { return m_dirty; }
void ClearDirty() { m_dirty = false; }
```

---

### 🟡 HIGH PRIORITY FIX #1: Add Blackboard Validation (1.5 hours)

**File:** `Source/BlueprintEditor/VisualScriptEditorPanel_Blackboard.cpp`

**Location:** `RenderLocalVariablesPanel()` - InputText for keys

**Current Code (NO VALIDATION):**
```cpp
char keyBuf[64];
strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);
ImGui::SetNextItemWidth(140.0f);
if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
{
    entry.Key = keyBuf;  // ❌ NO VALIDATION
    m_dirty = true;
}
```

**Fixed Code (WITH VALIDATION):**
```cpp
char keyBuf[64];
strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);

// Add validation indicator
bool keyValid = true;
if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
{
    std::string newKey(keyBuf);
    
    // Validation rules
    if (newKey.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠️ Key cannot be empty");
        keyValid = false;
    }
    else if (newKey.find(':') == std::string::npos && 
             entry.IsGlobal) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "⚠️ Global key should have format 'scope:name'");
        // But still allow (warn only)
    }
    else {
        // Check for duplicates
        for (size_t i = 0; i < m_template.Blackboard.size(); ++i) {
            if (i != idx && m_template.Blackboard[i].Key == newKey) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚠️ Key already exists");
                keyValid = false;
                break;
            }
        }
    }
    
    if (keyValid) {
        entry.Key = newKey;
        m_dirty = true;
    }
}
```

---

### 🟡 HIGH PRIORITY FIX #2: SubGraph File Browser (1.5 hours)

**File:** `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`

**Location:** `RenderNodePropertiesPanel()` case TaskNodeType::SubGraph

**Current Code (READ-ONLY):**
```cpp
case TaskNodeType::SubGraph:
{
    ImGui::TextDisabled("SubGraph");
    ImGui::TextDisabled("Path: %s", def.SubGraphPath.c_str());
    break;
}
```

**Fixed Code (WITH FILE BROWSER):**
```cpp
case TaskNodeType::SubGraph:
{
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "SubGraph");
    ImGui::Separator();

    // Current path display
    ImGui::TextDisabled("Current path:");
    ImGui::TextDisabled("%s", def.SubGraphPath.c_str());
    
    ImGui::Separator();

    // File browser button
    if (ImGui::Button("Browse##subgraph_file", ImVec2(120, 0)))
    {
        ImGui::OpenPopup("SubGraphFileDialog");
    }
    ImGui::SameLine();
    
    // Clear path button
    if (ImGui::Button("Clear##subgraph_clear", ImVec2(80, 0)))
    {
        def.SubGraphPath = "";
        for (size_t i = 0; i < m_template.Nodes.size(); ++i) {
            if (m_template.Nodes[i].NodeID == m_selectedNodeID) {
                m_template.Nodes[i].SubGraphPath = "";
                break;
            }
        }
        m_dirty = true;
    }

    // File dialog (simplified version)
    if (ImGui::BeginPopupModal("SubGraphFileDialog", nullptr, 
                               ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char pathBuffer[512] = "";
        
        ImGui::TextDisabled("SubGraph path (relative to Blueprints/):");
        ImGui::InputText("##subgraph_path", pathBuffer, sizeof(pathBuffer));
        ImGui::TextDisabled("Example: AIBehaviors/patrol.ats");
        
        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            std::string selectedPath(pathBuffer);
            if (!selectedPath.empty() && selectedPath.find(".ats") != std::string::npos)
            {
                def.SubGraphPath = selectedPath;
                for (size_t i = 0; i < m_template.Nodes.size(); ++i) {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID) {
                        m_template.Nodes[i].SubGraphPath = selectedPath;
                        break;
                    }
                }
                m_dirty = true;
                memset(pathBuffer, 0, sizeof(pathBuffer));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            memset(pathBuffer, 0, sizeof(pathBuffer));
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }

    break;
}
```

---

## Testing Strategy

### Unit Tests

```cpp
// test_node_properties.cpp

TEST(NodePropertiesPanel, AtomicTaskSerialization) {
    auto node = CreateNode(TaskNodeType::AtomicTask);
    node.AtomicTaskID = "MyTask";
    node.Parameters["param1"].LiteralValue = TaskValue(42);
    
    template_.Nodes.push_back(node);
    
    editor.LoadTemplate(&template_);
    editor.SetSelectedNode(node.NodeID);
    
    // Simulate UI edit
    auto& eNode = editor.GetEditorNode(node.NodeID);
    eNode.def.Parameters["param1"].LiteralValue = TaskValue(99);
    
    // Trigger sync
    editor.SyncTemplateFromCanvas();
    
    // Verify sync
    auto& syncedNode = template_.Nodes[0];
    EXPECT_EQ(syncedNode.Parameters["param1"].LiteralValue.AsInt(), 99);
}

TEST(NodePropertiesPanel, BranchConditionSync) {
    auto node = CreateNode(TaskNodeType::Branch);
    node.conditionRefs.push_back({/* preset ref */});
    
    editor.LoadTemplate(&template_);
    editor.SetSelectedNode(node.NodeID);
    
    // Verify panel initialized
    EXPECT_FALSE(editor.GetConditionsPanel()->IsDirty());
    
    // Simulate condition edit via panel
    // (This would be triggered by callback in real UI)
    
    // Verify both eNode and template updated
    EXPECT_TRUE(template_.Nodes[0].conditionRefs.size() > 0);
}

TEST(NodePropertiesPanel, WhileNodePhase24Consistency) {
    auto node = CreateNode(TaskNodeType::While);
    node.conditionRefs.push_back({/* preset ref */});
    node.dynamicPins.push_back({/* pin */});
    
    editor.LoadTemplate(&template_);
    editor.SetSelectedNode(node.NodeID);
    
    // Verify WhileNodeProperties uses new system
    auto& eNode = editor.GetEditorNode(node.NodeID);
    EXPECT_EQ(eNode.def.conditionRefs.size(), 1);
    EXPECT_EQ(eNode.def.dynamicPins.size(), 1);
}
```

### Integration Tests

```cpp
TEST(NodePropertyIntegration, SaveLoadConsistency) {
    // Create complex graph with all node types
    auto template_ = CreateComplexGraph();
    
    // Load in editor
    editor.LoadTemplate(&template_);
    
    // Edit various nodes
    editAtomicTaskNode(editor, 0);
    editBranchNode(editor, 1);
    editWhileNode(editor, 2);
    
    // Save to file
    editor.Save();
    
    // Reload from file
    auto reloaded = LoadTemplate(editor.GetCurrentPath());
    
    // Verify all edits persisted
    EXPECT_EQ(reloaded.Nodes[0].AtomicTaskID, 
              template_.Nodes[0].AtomicTaskID);
    EXPECT_EQ(reloaded.Nodes[1].conditionRefs.size(),
              template_.Nodes[1].conditionRefs.size());
    EXPECT_EQ(reloaded.Nodes[2].conditionRefs.size(),
              template_.Nodes[2].conditionRefs.size());
}

TEST(NodePropertyIntegration, BlackboardValidation) {
    // Create template with blackboard
    auto template_ = CreateTemplate();
    template_.Blackboard.push_back({"health", VariableType::Int, TaskValue(100)});
    
    editor.LoadTemplate(&template_);
    
    // Try to add empty key
    addBlackboardEntry(editor, "", VariableType::Int);
    
    // Verify validation warning shown
    EXPECT_TRUE(editor.HasValidationWarning());
    
    // Verify empty key not added
    EXPECT_EQ(template_.Blackboard.size(), 1);
}
```

---

## Checklist for Verification

After applying fixes:

- [ ] While nodes render using NodeConditionsPanel
- [ ] While node edits sync to both conditions[] and conditionRefs[]
- [ ] ForEach nodes have property panel dispatcher
- [ ] GetBBValue/SetBBValue panels audited and documented
- [ ] Blackboard keys validated (no empty keys)
- [ ] Blackboard duplicates detected
- [ ] SubGraph path can be edited via file browser
- [ ] Switch cases editor modal works
- [ ] All nodes save and load consistently
- [ ] No data loss on save/load cycle
- [ ] Phase 24 presets work with While nodes
- [ ] Dynamic pins generated correctly for While nodes

---

## Files to Modify

| Priority | File | Changes | Time |
|----------|------|---------|------|
| 🔴 CRITICAL | `VisualScriptEditorPanel_Properties.cpp` | Fix While dispatcher, add ForEach, add SubGraph browser | 1.5h |
| 🔴 CRITICAL | `GetBBValuePropertyPanel.cpp/h` | Full audit & documentation | 1.5h |
| 🔴 CRITICAL | `SetBBValuePropertyPanel.cpp/h` | Full audit & documentation | 1.5h |
| 🟡 HIGH | `VisualScriptEditorPanel_Blackboard.cpp` | Add key validation | 1h |
| 🟡 HIGH | `VisualScriptEditorPanel_Properties.cpp` | Implement Switch editor modal | 1.5h |
| 🟢 MEDIUM | `VisualScriptEditorPanel.h` | Add validation tracking | 0.5h |

**Total Estimated Time:** 8-10 hours

---

## Success Criteria

✅ All 13 node types have functional property panels  
✅ No data loss on save/load cycles  
✅ Serialization complete for all parameters  
✅ Synchronization verified for all node types  
✅ Unit tests passing for all critical paths  
✅ Integration tests passing for save/load  
✅ While nodes using Phase 24 condition system  
✅ GetBBValue/SetBBValue panels fully audited  

---

**Report Generated:** 2026-04-01
