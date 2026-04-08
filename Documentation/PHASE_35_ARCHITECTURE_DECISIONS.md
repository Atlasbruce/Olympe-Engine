# ⚙️ PHASE 35 - Architecture Decision Document

**Date**: 08-04-2026
**Status**: PLANNING
**Audience**: Development Team

---

## Decision 1: Shared NodeGraphPanel vs Separate BTCanvas

### Context
We have two successful canvas implementations in the codebase:
1. **VisualScriptEditorPanel** with imnodes native grid
2. **PrefabCanvas** with custom zoom/pan implementation

For BehaviorTree, we must decide: use existing NodeGraphPanel (like VisualScript) or create new BTCanvas?

### Options Analyzed

#### Option A: Shared NodeGraphPanel (RECOMMENDED ✅)
**Approach**: BehaviorTreeRenderer uses same NodeGraphPanel as VisualScript

**Pros**:
- ✅ Minimal code duplication
- ✅ Consistent rendering pipeline
- ✅ Faster implementation (reuse existing)
- ✅ NodeGraphPanel already integrated with TabManager
- ✅ Same coordinate system/zoom/pan behavior as VisualScript

**Cons**:
- ❌ NodeGraphPanel may need slight adaptations for BT node types
- ❌ Shared state management (need separate graph ID)

**Implementation**: 
```cpp
// TabManager::CreateNewTab("BehaviorTree")
static NodeGraphPanel s_btPanel;
BehaviorTreeRenderer* r = new BehaviorTreeRenderer(s_btPanel);
```

**Verdict**: **CHOSEN** - Simplest, fastest, follows existing pattern

---

#### Option B: Separate BTCanvas
**Approach**: Create new dedicated PrefabCanvas-like implementation

**Pros**:
- ✅ Full control over rendering
- ✅ Can optimize for BT-specific features
- ✅ No potential conflicts with VisualScript

**Cons**:
- ❌ ~1000 LOC of new code (coordinate system, zoom, pan, grid)
- ❌ 3-5 extra days of implementation
- ❌ Maintenance burden (duplicate code paths)
- ❌ Performance: two separate rendering pipelines

**Verdict**: **REJECTED** - Over-engineered for current needs

---

## Decision 2: Property Panel Architecture

### Context
NodeGraphPanel is good for rendering, but property editing needs separate dedicated panel.

### Options Analyzed

#### Option A: Dedicated PropertyPanel_BT Class (RECOMMENDED ✅)
**Approach**: New class similar to EntityPrefab's PropertyEditorPanel

**Architecture**:
```cpp
class PropertyPanel_BT
{
    void Render(const BTNode* selectedNode);
    void SetSelectedNode(const BTNode* node);
    
private:
    void RenderNodeBasics();
    void RenderNodeParameters();
    void RenderConditionProperties();
    void RenderActionProperties();
};
```

**Integration**:
```cpp
// In BehaviorTreeRenderer::Render()
ImGui::BeginChild("btProperties", ImVec2(300, 0));
m_propertyPanel.Render(m_selectedNode);
ImGui::EndChild();
```

**Pros**:
- ✅ Clean separation of concerns
- ✅ Testable in isolation
- ✅ Follows Entity Prefab pattern
- ✅ Extensible for future properties

**Cons**:
- ❌ New file to maintain

**Verdict**: **CHOSEN** - Best architecture

---

#### Option B: Integrate into NodeGraphPanel
**Approach**: Add property editing directly to NodeGraphPanel

**Cons**:
- ❌ NodeGraphPanel becomes bloated (already 500 LOC)
- ❌ Mixes concerns (rendering + editing)
- ❌ Harder to test

**Verdict**: **REJECTED**

---

## Decision 3: Graph Type Detection Logic

### Current State
`TabManager::DetectGraphType()` already handles BehaviorTree:

```cpp
if (root.contains("blueprintType") && root["blueprintType"].get<std::string>() == "BehaviorTree")
    return "BehaviorTree";

if (root.contains("rootNodeId") && root.contains("nodes"))
    return "BehaviorTree";
```

### Decision: Use Existing Logic (APPROVED ✅)
No changes needed - detection is already correct. Just implement creation.

---

## Decision 4: Serialization Strategy

### BehaviorTree JSON v2 Schema
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "patrol_ai",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Selector",
        "name": "Root",
        "children": [2, 3],
        "parameters": {
          "string": { "debugName": "Patrol Root" },
          "int": {},
          "float": {}
        }
      },
      {
        "id": 2,
        "type": "Condition",
        "name": "HasTarget?",
        "conditionType": "TargetVisible",
        "parameters": {
          "string": {},
          "int": {},
          "float": { "range": 50.0 }
        }
      },
      {
        "id": 3,
        "type": "Action",
        "name": "Patrol",
        "actionType": "PatrolPickNextPoint",
        "parameters": {
          "string": { "patternName": "square" },
          "int": {},
          "float": { "waitTime": 2.5 }
        }
      }
    ]
  }
}
```

### Mapping to C++ BTNode Structure
```cpp
struct BTNode {
    uint32_t id;
    BTNodeType type;
    std::string name;
    std::vector<uint32_t> children;  // Only for Selector/Sequence
    
    // Parameters
    std::map<std::string, std::string> stringParams;
    std::map<std::string, int> intParams;
    std::map<std::string, float> floatParams;
    
    // Type-specific
    BTConditionType conditionType;
    BTActionType actionType;
};
```

### Serialization Implementation
```cpp
bool BehaviorTreeAsset::SaveToFile(const std::string& path)
{
    nlohmann::json root;
    root["schema_version"] = 2;
    root["blueprintType"] = "BehaviorTree";
    root["name"] = m_name;
    
    // Serialize nodes
    auto nodesArray = nlohmann::json::array();
    for (const auto& node : m_nodes)
    {
        auto nodeJson = nlohmann::json::object();
        nodeJson["id"] = node.id;
        nodeJson["type"] = static_cast<int>(node.type);
        nodeJson["name"] = node.name;
        
        // Add parameters
        nodeJson["parameters"]["string"] = node.stringParams;
        nodeJson["parameters"]["int"] = node.intParams;
        nodeJson["parameters"]["float"] = node.floatParams;
        
        // Type-specific
        if (node.type == BTNodeType::Condition)
            nodeJson["conditionType"] = static_cast<int>(node.conditionType);
        if (node.type == BTNodeType::Action)
            nodeJson["actionType"] = static_cast<int>(node.actionType);
        
        // Children
        if (!node.children.empty())
            nodeJson["children"] = node.children;
        
        nodesArray.push_back(nodeJson);
    }
    
    root["data"]["nodes"] = nodesArray;
    root["data"]["rootNodeId"] = m_rootNodeId;
    
    // Write to file
    std::ofstream ofs(path);
    if (!ofs.good()) return false;
    ofs << root.dump(2);
    return true;
}
```

**Decision**: Use JSON v2 schema as documented in RAPPORT. No breaking changes to existing format.

---

## Decision 5: Undo/Redo Integration

### Current State
BTEditorCommand classes exist but are not wired to UI.

### Decision: Full CommandStack Integration (APPROVED ✅)

**Implementation Pattern**:
```cpp
// When creating node
CommandStack::Get().Execute(
    std::make_unique<AddBTNodeCommand>(
        graphId, 
        nodeType, 
        position.x, 
        position.y
    )
);

// When connecting nodes
CommandStack::Get().Execute(
    std::make_unique<ConnectBTNodesCommand>(
        graphId, 
        sourceNodeId, 
        targetNodeId
    )
);
```

**Why**: Users expect Ctrl+Z to work everywhere. Standard editor behavior.

---

## Decision 6: Node Palette Integration

### Current State
BTNodePalette exists but isolated from editor.

### Decision: Split Panel Layout (APPROVED ✅)

**Layout**:
```
┌────────────────────────────────────────────┐
│ BehaviorTree Editor Tab                     │
├────────────────────────────────────────────┤
│                                              │
│  ┌──────────┐              ┌───────────┐   │
│  │ Palette  │              │ Properties│   │
│  │          │              │           │   │
│  │ Selector │              │ Node ID:  │   │
│  │ Sequence │              │ Type:     │   │
│  │ Condition│    Canvas    │ Name:     │   │
│  │ Action   │              │ Params:   │   │
│  │ Inverter │              │           │   │
│  │ Repeater │              │           │   │
│  │          │              │           │   │
│  └──────────┘              └───────────┘   │
│                                              │
├────────────────────────────────────────────┤
│ Status: 3 nodes, 2 connections, No errors  │
└────────────────────────────────────────────┘
```

**Implementation**:
- Left (20%): BTNodePalette (drag-drop source)
- Middle (50%): NodeGraphPanel canvas
- Right (30%): PropertyPanel_BT (property editor)

---

## Decision 7: Error Handling Strategy

### Validation Errors
Use existing BTGraphValidator:

```cpp
BTGraphValidationResult result = BTGraphValidator::Validate(asset);
if (!result.isValid)
{
    for (const auto& error : result.errors)
    {
        // Display in status bar
        // Highlight nodes in red
        // Allow click-to-navigate
    }
}
```

### Save Validation
Prevent save if critical errors exist:

```cpp
bool BehaviorTreeRenderer::Save(const std::string& path)
{
    auto result = BTGraphValidator::Validate(m_asset);
    if (!result.isValid && result.hasCriticalErrors)
    {
        ImGui::OpenPopup("SaveValidationError");
        return false;
    }
    
    return m_asset.SaveToFile(path);
}
```

---

## Decision 8: Testing Strategy

### Unit Tests
Create `Tests/BlueprintEditor/Test_BehaviorTreeEditor.cpp`:

```cpp
TEST(BehaviorTreeEditor, CreateNewBTGraph)
{
    TabManager::Get().CreateNewTab("BehaviorTree");
    ASSERT_EQ(1, TabManager::Get().GetAllTabs().size());
}

TEST(BehaviorTreeEditor, SaveLoadRoundTrip)
{
    // Create graph
    // Add nodes
    // Save to file
    // Load from file
    // Verify data matches
}

TEST(BehaviorTreeEditor, PropertyPanel)
{
    // Select node
    // Edit property
    // Verify data updated
}
```

### Integration Tests
Load existing BT files and verify:
- [ ] Can load Blueprints/AI/patrol.json
- [ ] Can load Blueprints/AI/combat.json
- [ ] Can load Blueprints/AI/guard.json

---

## Risk Analysis

### Risk 1: NodeGraphPanel Conflicts
**Risk**: Shared NodeGraphPanel state corruption between VisualScript and BehaviorTree tabs

**Mitigation**: 
- Use separate graph IDs in BTNodeGraphManager
- Test switching between VS and BT tabs
- Add assertions for graph type validation

### Risk 2: Serialization Format Mismatch
**Risk**: Old BT v1 files not compatible with v2

**Mitigation**:
- Implement migration code (BTtoVSMigrator already exists)
- Add schema version check in LoadFromFile()
- Log warnings for v1 files

### Risk 3: Performance Regression
**Risk**: NodeGraphPanel slows down with large BT graphs (100+ nodes)

**Mitigation**:
- Profile before/after with test graphs
- Monitor frame times
- Optimize node layout algorithm if needed

---

## Success Criteria

✅ **Phase 35 Complete When**:
1. [ ] New BT can be created from menu
2. [ ] Existing BT files can be opened and displayed
3. [ ] Nodes can be created via drag-drop from palette
4. [ ] Node properties can be edited
5. [ ] Save/load round-trip preserves all data
6. [ ] Undo/redo works for all operations
7. [ ] Validation errors display with navigation
8. [ ] No compilation errors/warnings
9. [ ] 3+ existing BT files tested
10. [ ] Performance: no regression (frame time < 16ms)

---

## Appendix A: Files Checklist

### To Modify (9 files)
- [ ] `Source\BlueprintEditor\TabManager.cpp`
- [ ] `Source\BlueprintEditor\BlueprintEditorGUI.cpp`
- [ ] `Source\BlueprintEditor\BehaviorTreeRenderer.cpp`
- [ ] `Source\BlueprintEditor\BTNodeGraphManager.cpp`
- [ ] `Source\AI\BehaviorTree.cpp`
- [ ] `Source\AI\AIGraphPlugin_BT\BTGraphValidator.cpp`
- [ ] `Source\AI\AIGraphPlugin_BT\BTNodeRegistry.cpp`
- [ ] `.github/copilot-instructions.md`
- [ ] (Optional) Project files if PropertyPanel_BT not auto-discovered

### To Create (3 files)
- [ ] `Source\BlueprintEditor\PropertyPanel_BT.h`
- [ ] `Source\BlueprintEditor\PropertyPanel_BT.cpp`
- [ ] `Tests/BlueprintEditor/Test_BehaviorTreeEditor.cpp` (optional)

### Test Files
- [ ] `Gamedata/Blueprints/AI/test_editor_bt.json`

---

*Decision Document Created: 08-04-2026*
*All decisions APPROVED for implementation*
