# Phase 39: BehaviorTree SubGraph - Technical Design Document

## Document Overview
This TDD provides detailed implementation specifications for adding SubGraph Action nodes to the BehaviorTree system.

**Phase**: 39
**Category**: Architecture Enhancement
**Complexity**: Medium
**Timeline**: 15-25 days
**C++ Standard**: C++14 compliant
**Status**: Ready for implementation

---

## 1. Data Model Specification

### 1.1 BTNodeType Enum Extension

**File**: `Source/AI/BehaviorTree.h`

```cpp
enum class BTNodeType : uint8_t
{
    Selector = 0,       // OR node
    Sequence,           // AND node
    Condition,          // Leaf - condition check
    Action,             // Leaf - action execution
    Inverter,           // Decorator - invert result
    Repeater,           // Decorator - repeat N times
    Root,               // Phase 38b: Entry point
    OnEvent,            // Phase 38b: Event-driven entry
    SubGraph = 8        // Phase 39: External graph reference [NEW]
};
```

**Serialization String**: `"SubGraph"` (case-sensitive)

### 1.2 BTNode Structure Extension

**File**: `Source/AI/BehaviorTree.h`

```cpp
struct BTNode {
    // ... existing fields (type, id, name, childIds, etc.) ...
    
    // Phase 39: SubGraph fields
    std::string subgraphPath;           // Path to external .bt.json or .ats file
    
    // Input parameter mapping: child_var_name → parent_bb_key
    // Example: "patrolSpeed" → "globalPatrolSpeed"
    // Child blackboard gets value from parent at "globalPatrolSpeed"
    std::map<std::string, std::string> subgraphInputs;
    
    // Output parameter mapping: child_var_name → parent_bb_key
    // Example: "foundTarget" → "targetSpotted"
    // Parent blackboard gets value from child "foundTarget" and stores at "targetSpotted"
    std::map<std::string, std::string> subgraphOutputs;
    
    // Optional: metadata for validation
    std::string subgraphType;           // Expected type: "BehaviorTree" or "VisualScript"
};
```

### 1.3 JSON Serialization Format

**Location**: BehaviorTree file (`.bt.json`)

```json
{
  "id": 10,
  "type": "SubGraph",
  "name": "Call Patrol Behavior",
  "position": { "x": 400, "y": 300 },
  
  "subgraphPath": "Blueprints/AI/patrol.bt.json",
  "subgraphType": "BehaviorTree",
  
  "subgraphInputs": {
    "patrolSpeed": "globalPatrolSpeed",
    "searchRadius": "detectionRange",
    "maxWaitTime": "idleWaitTime"
  },
  
  "subgraphOutputs": {
    "foundTarget": "targetSpotted",
    "targetPosition": "lastKnownPosition"
  }
}
```

**Loading Code** (in `BehaviorTree::LoadFromJson` or similar):

```cpp
if (nodeJson["type"] == "SubGraph") {
    node.type = BTNodeType::SubGraph;
    
    if (nodeJson.contains("subgraphPath") && nodeJson["subgraphPath"].is_string())
        node.subgraphPath = nodeJson["subgraphPath"].get<std::string>();
    
    if (nodeJson.contains("subgraphType") && nodeJson["subgraphType"].is_string())
        node.subgraphType = nodeJson["subgraphType"].get<std::string>();
    
    if (nodeJson.contains("subgraphInputs") && nodeJson["subgraphInputs"].is_object()) {
        auto inputs = nodeJson["subgraphInputs"].items();
        for (auto& [childVar, parentKey] : inputs) {
            if (parentKey.is_string())
                node.subgraphInputs[childVar] = parentKey.get<std::string>();
        }
    }
    
    if (nodeJson.contains("subgraphOutputs") && nodeJson["subgraphOutputs"].is_object()) {
        auto outputs = nodeJson["subgraphOutputs"].items();
        for (auto& [childVar, parentKey] : outputs) {
            if (parentKey.is_string())
                node.subgraphOutputs[childVar] = parentKey.get<std::string>();
        }
    }
}
```

**Saving Code** (in `BehaviorTree::SaveToJson` or similar):

```cpp
if (node.type == BTNodeType::SubGraph) {
    nodeJson["type"] = "SubGraph";
    nodeJson["subgraphPath"] = node.subgraphPath;
    nodeJson["subgraphType"] = node.subgraphType;
    
    for (auto& [childVar, parentKey] : node.subgraphInputs)
        nodeJson["subgraphInputs"][childVar] = parentKey;
    
    for (auto& [childVar, parentKey] : node.subgraphOutputs)
        nodeJson["subgraphOutputs"][childVar] = parentKey;
}
```

### 1.4 BehaviorTreeAsset Extension

**File**: `Source/AI/BehaviorTree.h` (BehaviorTreeAsset struct or class)

```cpp
struct BehaviorTreeAsset {
    // ... existing fields ...
    uint32_t rootNodeId;
    std::vector<BTNode> nodes;
    std::string name;
    std::string filePath;
    
    // Phase 39: SubGraph support
    struct SubGraphCallStack {
        std::vector<std::string> pathStack;     // File paths of active SubGraphs
        int depth = 0;                          // Current nesting depth
        
        void Push(const std::string& path) {
            pathStack.push_back(path);
            depth++;
        }
        
        void Pop() {
            if (!pathStack.empty()) {
                pathStack.pop_back();
                depth--;
            }
        }
        
        bool Contains(const std::string& path) const {
            for (const auto& p : pathStack)
                if (p == path) return true;
            return false;
        }
        
        bool IsFull() const {
            return depth >= 32;  // MAX_SUBGRAPH_DEPTH
        }
    };
    
    // Static for executor to track nested calls
    // NOTE: Consider thread-safety if multi-threaded execution needed
    static SubGraphCallStack s_callStack;
};
```

---

## 2. Execution Engine Specification

### 2.1 BehaviorTreeExecutor Extension

**File**: `Source/BlueprintEditor/BehaviorTreeExecutor.h/cpp`

**New Public Method**:

```cpp
/**
 * @brief Execute a SubGraph node (load external tree and execute).
 * @param node      The SubGraph node containing path and parameter mappings
 * @param btAsset   Reference to parent BehaviorTree asset
 * @param tracer    Trace collection for debugging
 * @return         Execution status (Success/Failure/etc.)
 */
BTStatus ExecuteSubGraph(
    const BTNode& node,
    const BehaviorTreeAsset& btAsset,
    GraphExecutionTracer& outTracer);
```

**Implementation Pseudocode**:

```cpp
BTStatus BehaviorTreeExecutor::ExecuteSubGraph(
    const BTNode& node,
    const BehaviorTreeAsset& btAsset,
    GraphExecutionTracer& outTracer)
{
    // 1. Validate node configuration
    if (node.subgraphPath.empty()) {
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            "Empty SubGraph path",
            "Warnings");
        return BTStatus::Failure;
    }
    
    // 2. Check recursion depth
    if (BehaviorTreeAsset::s_callStack.IsFull()) {
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            "Maximum recursion depth (32) exceeded for: " + node.subgraphPath,
            "Critical");
        return BTStatus::Failure;
    }
    
    // 3. Check for circular references
    if (BehaviorTreeAsset::s_callStack.Contains(node.subgraphPath)) {
        std::string cycleStr = "Circular SubGraph reference: ";
        for (const auto& p : BehaviorTreeAsset::s_callStack.pathStack) {
            cycleStr += p + " → ";
        }
        cycleStr += node.subgraphPath;
        
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            cycleStr,
            "Critical");
        return BTStatus::Failure;
    }
    
    // 4. Resolve file path using DataManager (Phase 38)
    std::string resolvedPath = DataManager::Get().ResolveFilePath(node.subgraphPath);
    if (resolvedPath.empty()) {
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            "Could not resolve path: " + node.subgraphPath,
            "Critical");
        return BTStatus::Failure;
    }
    
    // 5. Load external asset
    BehaviorTreeAsset* childAsset = nullptr;
    
    if (resolvedPath.find(".ats") != std::string::npos) {
        // Load as VisualScript
        childAsset = LoadATSAsVisualScript(resolvedPath);
    } else if (resolvedPath.find(".bt.json") != std::string::npos ||
               resolvedPath.find(".json") != std::string::npos) {
        // Load as BehaviorTree
        childAsset = BehaviorTree::LoadFromFile(resolvedPath);
    } else {
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            "Unknown file type: " + resolvedPath,
            "Critical");
        return BTStatus::Failure;
    }
    
    if (!childAsset) {
        outTracer.RecordError(
            static_cast<int32_t>(node.id),
            "SubGraph",
            "Failed to load SubGraph: " + resolvedPath,
            "Critical");
        return BTStatus::Failure;
    }
    
    // 6. Push onto call stack
    BehaviorTreeAsset::s_callStack.Push(resolvedPath);
    
    // 7. Bind input parameters (parent → child)
    // NOTE: Parameter binding depends on blackboard implementation
    // For now, assume blackboard is global or passed separately
    
    // 8. Execute child tree
    BTStatus result = ExecuteNode(childAsset->rootNodeId, *childAsset, outTracer);
    
    // 9. Extract output parameters (child → parent)
    // NOTE: Similar to input binding
    
    // 10. Pop call stack
    BehaviorTreeAsset::s_callStack.Pop();
    
    // 11. Record result in trace
    outTracer.RecordNodeExited(static_cast<int32_t>(node.id), result);
    
    return result;
}
```

### 2.2 Integration with ExecuteNode

**In BehaviorTreeExecutor::ExecuteNode()**, add case for SubGraph:

```cpp
BTStatus BehaviorTreeExecutor::ExecuteNode(
    uint32_t nodeId,
    const BehaviorTreeAsset& btAsset,
    GraphExecutionTracer& outTracer)
{
    // ... existing code ...
    
    BTStatus status = BTStatus::Failure;
    
    switch (node->type) {
        case BTNodeType::Selector:
            status = ExecuteSelector(*node, btAsset, outTracer);
            break;
        
        case BTNodeType::Sequence:
            status = ExecuteSequence(*node, btAsset, outTracer);
            break;
        
        case BTNodeType::Condition:
            status = ExecuteCondition(*node, outTracer);
            break;
        
        case BTNodeType::Action:
            status = ExecuteAction(*node, outTracer);
            break;
        
        case BTNodeType::Inverter:
            status = ExecuteDecorator(*node, btAsset, outTracer);
            break;
        
        case BTNodeType::Repeater:
            status = ExecuteDecorator(*node, btAsset, outTracer);
            break;
        
        case BTNodeType::SubGraph:  // Phase 39 [NEW]
            status = ExecuteSubGraph(*node, btAsset, outTracer);
            break;
        
        default:
            SYSTEM_LOG << "[BehaviorTreeExecutor] Unknown node type: " 
                       << static_cast<int>(node->type) << "\n";
            status = BTStatus::Failure;
    }
    
    outTracer.RecordNodeExited(static_cast<int32_t>(nodeId), status);
    
    return status;
}
```

### 2.3 Hybrid Execution (BT ↔ ATS)

**New Method** (Private):

```cpp
/**
 * @brief Load and execute an ATS (VisualScript) file in BT context.
 * @details Adapts ATS execution result to BT status.
 */
BehaviorTreeAsset* BehaviorTreeExecutor::LoadATSAsVisualScript(const std::string& path)
{
    // Load ATS file
    std::vector<std::string> errors;
    TaskGraphTemplate* vsGraph = AssetManager::Get().LoadTaskGraphFromFile(path, errors);
    
    if (!vsGraph) {
        for (const auto& err : errors)
            SYSTEM_LOG << "[BehaviorTreeExecutor] ATS load error: " << err << "\n";
        return nullptr;
    }
    
    // Wrap in BehaviorTreeAsset-like structure
    // OR: Convert execution to BTStatus and return success/failure
    
    // Approach: Create adapter class BehaviorTreeAdapter that wraps ATS graph
    // For now, return nullptr (requires further design)
    
    return nullptr;  // TODO: Implement ATS→BT adapter
}
```

---

## 3. Editor Integration Specification

### 3.1 BTNodePropertyPanel Extension

**File**: `Source/BlueprintEditor/BTNodePropertyPanel.h/cpp`

**New UI Section** (in Render method):

```cpp
void BTNodePropertyPanel::RenderSubGraphProperties(const BTNode& node)
{
    ImGui::Separator();
    ImGui::Text("SubGraph Properties");
    
    // File Path Input
    static char pathBuffer[512] = {0};
    strcpy_s(pathBuffer, sizeof(pathBuffer), node.subgraphPath.c_str());
    
    if (ImGui::InputText("File Path##subgraph", pathBuffer, sizeof(pathBuffer))) {
        // Path changed
        m_selectedNode->subgraphPath = pathBuffer;
        MarkDirty();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Browse##subgraph")) {
        ShowSubGraphFilePicker();
    }
    
    // Input Parameters
    ImGui::SeparatorText("Input Parameters");
    ImGui::BeginChild("InputParams", ImVec2(-1, 100), true);
    
    for (auto& [childVar, parentKey] : m_selectedNode->subgraphInputs) {
        ImGui::Text("Child: %s", childVar.c_str());
        ImGui::SameLine();
        
        static char paramBuffer[256] = {0};
        strcpy_s(paramBuffer, sizeof(paramBuffer), parentKey.c_str());
        
        ImGui::SetNextItemWidth(150);
        if (ImGui::InputText(("##input_" + childVar).c_str(), paramBuffer, sizeof(paramBuffer))) {
            m_selectedNode->subgraphInputs[childVar] = paramBuffer;
            MarkDirty();
        }
        
        ImGui::SameLine();
        if (ImGui::Button(("Remove##input_" + childVar).c_str())) {
            m_selectedNode->subgraphInputs.erase(childVar);
            MarkDirty();
        }
    }
    
    if (ImGui::Button("+ Add Input")) {
        ImGui::OpenPopup("AddInputParam");
    }
    
    ImGui::EndChild();
    
    // Output Parameters (similar to Input)
    ImGui::SeparatorText("Output Parameters");
    // ... similar code for outputs ...
}
```

### 3.2 Canvas Rendering

**File**: `Source/BlueprintEditor/BTNodeRenderer.h/cpp`

**New Render Function**:

```cpp
void BTNodeRenderer::RenderSubGraphNode(
    const BTNode& node,
    ImVec2 nodePos,
    bool isSelected,
    bool isHovered)
{
    ImVec2 size(150, 80);
    ImVec2 screenPos = nodePos + m_canvasOffset;
    
    // Draw box
    ImU32 boxColor = isSelected ? IM_COL32(100, 150, 255, 255)  // Light blue
                                : IM_COL32(70, 120, 200, 255);   // Dark blue
    
    ImGui::GetWindowDrawList()->AddRect(
        screenPos,
        screenPos + size,
        isHovered ? IM_COL32(255, 200, 0, 255) : boxColor,
        5.0f);
    
    // Draw icon (database + arrow)
    DrawSubGraphIcon(screenPos + ImVec2(10, 10));
    
    // Draw title
    ImGui::GetWindowDrawList()->AddText(
        screenPos + ImVec2(40, 15),
        IM_COL32(255, 255, 255, 255),
        "SubGraph");
    
    // Draw file name (truncated)
    std::string displayName = node.subgraphPath;
    if (displayName.length() > 20)
        displayName = displayName.substr(0, 17) + "...";
    
    ImGui::GetWindowDrawList()->AddText(
        screenPos + ImVec2(10, 50),
        IM_COL32(200, 200, 200, 255),
        displayName.c_str());
}
```

### 3.3 Double-Click Navigation

**File**: `Source/BlueprintEditor/BehaviorTreeRenderer.cpp`

**In RenderCanvas() or similar**:

```cpp
// Handle double-click on SubGraph nodes
if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    if (hoveredNode != -1) {
        const BTNode* node = currentAsset.GetNode(hoveredNode);
        if (node && node->type == BTNodeType::SubGraph) {
            if (!node->subgraphPath.empty()) {
                // Resolve path
                std::string resolvedPath = 
                    DataManager::Get().ResolveFilePath(node->subgraphPath);
                
                if (!resolvedPath.empty()) {
                    // Open in new tab
                    TabManager::Get().OpenFileInTab(resolvedPath);
                }
            }
        }
    }
}
```

---

## 4. Verification & Validation Specification

### 4.1 New Validation Rules

**File**: `Source/BlueprintEditor/BehaviorTreeValidator.h/cpp`

```cpp
// New validation rule IDs:
// E501 — SubGraph with empty path
// E502 — SubGraph references non-existent file
// E503 — SubGraph circular reference detected
// E504 — SubGraph would exceed recursion depth
// E505 — SubGraph type mismatch (ATS field expected type, etc.)

void BehaviorTreeValidator::CheckSubGraphNodes(
    const BehaviorTreeAsset& asset,
    std::vector<ValidationError>& errors)
{
    for (const auto& node : asset.nodes) {
        if (node.type != BTNodeType::SubGraph)
            continue;
        
        // E501: Empty path
        if (node.subgraphPath.empty()) {
            errors.push_back(ValidationError{
                "E501",
                "SubGraph node '" + node.name + "' has empty path",
                node.id
            });
            continue;
        }
        
        // E502: File not found
        std::string resolved = DataManager::Get().ResolveFilePath(node.subgraphPath);
        if (resolved.empty()) {
            errors.push_back(ValidationError{
                "E502",
                "SubGraph path not found: " + node.subgraphPath,
                node.id
            });
            continue;
        }
        
        // E503: Circular reference
        if (IsSubGraphCircular(asset, node.subgraphPath)) {
            errors.push_back(ValidationError{
                "E503",
                "SubGraph has circular reference: " + node.subgraphPath,
                node.id
            });
        }
        
        // E504: Would exceed depth
        int depth = CalculateMaxSubGraphDepth(asset);
        if (depth > 32) {
            errors.push_back(ValidationError{
                "E504",
                "SubGraph nesting exceeds depth limit (32): depth=" + 
                    std::to_string(depth),
                node.id
            });
        }
    }
}
```

### 4.2 ExecutionTestPanel Integration

**File**: `Source/BlueprintEditor/ExecutionTestPanel.h/cpp`

**New Method**:

```cpp
void ExecutionTestPanel::DisplayBTTrace(const GraphExecutionTracer& tracer)
{
    // Use existing DisplayTrace() method
    // The tracer already contains SubGraph execution events
    // Just display normally (no special handling needed)
    
    DisplayTrace(tracer);
}
```

**Trace Display** (already works via BehaviorTreeExecutor integration):

```
[Node #1: Root]
  [Node #2: Selector]
    [Node #3: Condition - Check Target Visible]
      → Status: Failure
    
    [Node #4: SubGraph - Call Patrol]
      → Loading: Blueprints/AI/patrol.bt.json
      → Depth: 2/32
      
      [Child Node #1: Root]
        [Child Node #2: Sequence]
          [Child Node #3: Action - Pick Waypoint]
            → Status: Success
          [Child Node #4: Action - Move To]
            → Status: Success
      
      → SubGraph Result: Success
    
  → Selector Result: Success
```

---

## 5. Testing Specification

### 5.1 Unit Tests

**File**: `Tests/BlueprintEditor/BTSubGraphTest.cpp`

```cpp
TEST(BTSubGraphTest, SerializationRoundTrip) {
    // Create node with SubGraph data
    // Serialize to JSON
    // Deserialize from JSON
    // Verify all fields match
}

TEST(BTSubGraphTest, CircularReferenceDetection) {
    // Create A → B → A cycle
    // Verify executor detects cycle
}

TEST(BTSubGraphTest, RecursionDepthLimit) {
    // Create deeply nested SubGraphs (>32 levels)
    // Verify execution stops at limit
}

TEST(BTSubGraphTest, ParameterBinding) {
    // Create parent and child with parameter mapping
    // Verify input params passed correctly
    // Verify output params extracted correctly
}

TEST(BTSubGraphTest, FileNotFound) {
    // Reference non-existent file
    // Verify graceful error handling
}
```

### 5.2 Integration Tests

```cpp
TEST(BTSubGraphIntegration, ExecutionWithTrace) {
    // Load BT with SubGraph nodes
    // Execute tree
    // Verify trace contains SubGraph execution events
}

TEST(BTSubGraphIntegration, EditorNavigation) {
    // Double-click SubGraph node
    // Verify correct file opens in editor
}

TEST(BTSubGraphIntegration, ValidationChecks) {
    // Test all 5 new validation rules
    // Verify errors are caught
}
```

---

## 6. Dependencies & Imports

### 6.1 Required Includes

**In BehaviorTree.h**:
```cpp
#include <map>
#include <vector>
```

**In BehaviorTreeExecutor.cpp**:
```cpp
#include "DataManager.h"          // For ResolveFilePath()
#include "GraphExecutionTracer.h" // For trace recording
#include "AssetManager.h"         // For LoadTaskGraphFromFile() (ATS loading)
```

**In BehaviorTreeValidator.h/cpp**:
```cpp
#include "DataManager.h"
#include "BehaviorTree.h"
```

### 6.2 No Changes Required To

- ✅ PrefabCanvas (generic node rendering)
- ✅ TabManager (already supports file navigation)
- ✅ ExecutionTestPanel (already supports DisplayTrace)
- ✅ GraphExecutionTracer (already records events)
- ✅ DataManager (Phase 38 ready)

---

## 7. Implementation Checklist

### Phase 39a: Core Execution
- [ ] Add `SubGraph = 8` to `BTNodeType` enum
- [ ] Add `subgraphPath`, `subgraphInputs`, `subgraphOutputs` to `BTNode`
- [ ] Implement JSON load for SubGraph nodes
- [ ] Implement JSON save for SubGraph nodes
- [ ] Add `SubGraphCallStack` to `BehaviorTreeAsset`
- [ ] Implement `ExecuteSubGraph()` in `BehaviorTreeExecutor`
- [ ] Integrate SubGraph case in `ExecuteNode()` switch
- [ ] Unit tests for serialization
- [ ] Unit tests for execution + cycle detection
- [ ] Unit tests for depth limit

### Phase 39b: Editor & UI
- [ ] Add SubGraph section to `BTNodePropertyPanel`
- [ ] Implement file path input + browse button
- [ ] Implement input parameter grid UI
- [ ] Implement output parameter grid UI
- [ ] Add SubGraph rendering to `BTNodeRenderer`
- [ ] Add double-click handler to `BehaviorTreeRenderer`
- [ ] Implement file picker modal
- [ ] Integration tests for editor

### Phase 39c: Verification
- [ ] Implement `CheckSubGraphNodes()` validator
- [ ] Add 5 new validation rules (E501-E505)
- [ ] Integrate with `ExecutionTestPanel::DisplayBTTrace()`
- [ ] Add BehaviorTree validation tests
- [ ] System integration tests

### Phase 39d: Polish & Documentation
- [ ] Documentation for SubGraph feature
- [ ] Example BehaviorTree files with SubGraphs
- [ ] Performance profiling
- [ ] Edge case handling review
- [ ] Final QA

---

## 8. Notes & Considerations

### Thread Safety
Current implementation assumes single-threaded execution. If multi-threaded support is needed:
- Use thread-local storage for `SubGraphCallStack`
- Consider executor per-thread instead of global

### Blackboard Binding
Current design uses string-based parameter passing. Future enhancements:
- Type checking at parameter binding
- Automatic type coercion
- Nested parameter paths ("parent.child")

### ATS Integration
Hybrid execution (BT ↔ ATS) requires:
- Adapter class to wrap ATS execution in BT status
- Parameter mapping between BT blackboard and ATS variables
- Error propagation from ATS to BT trace

### Performance
- SubGraph loading is lazy (on execution, not on load)
- Consider caching loaded assets to avoid reload
- Profile execution trace overhead with deep nesting

---

## 9. Appendix: Code Examples

### Example 1: Simple Patrol BehaviorTree with SubGraph

```json
{
  "schema_version": 1,
  "name": "Guardian Behavior",
  "rootNodeId": 1,
  "nodes": [
    {
      "id": 1,
      "type": "Root",
      "name": "Root",
      "childIds": [2]
    },
    {
      "id": 2,
      "type": "Selector",
      "name": "Main Selector",
      "childIds": [3, 4]
    },
    {
      "id": 3,
      "type": "Condition",
      "name": "Check Target Visible",
      "conditionType": "TargetVisible"
    },
    {
      "id": 4,
      "type": "SubGraph",
      "name": "Patrol Behavior",
      "subgraphPath": "Blueprints/AI/patrol.bt.json",
      "subgraphInputs": {
        "patrolSpeed": "moveSpeed",
        "searchRadius": "detectionRange"
      },
      "subgraphOutputs": {
        "foundTarget": "targetSpotted"
      }
    }
  ]
}
```

### Example 2: Nested SubGraphs

```
Root
└─ Selector
   ├─ Sequence
   │  ├─ Condition: Player in range
   │  └─ SubGraph: Combat Behavior (combat.bt.json)
   │     └─ SubGraph: Special Attack (special_attack.bt.json)  ← Nested!
   └─ Idle
```

This would create call stack:
```
Depth 1: combat.bt.json
Depth 2: special_attack.bt.json
```

---

## Conclusion

Phase 39 provides detailed technical specifications for implementing BehaviorTree SubGraph nodes. The design leverages proven VisualScript patterns while adapting to BT semantics.

**Key Success Factors**:
1. Reuse ExecutionTestPanel and GraphExecutionTracer
2. Use DataManager::ResolveFilePath() for file resolution
3. Implement clean parameter passing via string mapping
4. Maintain depth and cycle detection from VisualScript

**Estimated Implementation Time**: 15-25 days across 4 sprints

**Next Step**: Begin Phase 39a (Core Execution) implementation
