# Phase 39: VisualScript SubGraph Reference Implementation Guide

## Overview
This document maps VisualScript SubGraph implementation to help guide BehaviorTree SubGraph development.

---

## Core Architecture Files (VisualScript Reference)

### 1. Data Model Layer

**File**: `Source/TaskSystem/TaskGraphTemplate.h`
```cpp
enum class TaskNodeType { ..., SubGraph = 11 };

struct TaskNodeDefinition {
    TaskNodeType Type;
    std::string SubGraphPath;           // External file path
    std::map<std::string, ParameterBinding> InputParams;   // Parameter binding
    std::map<std::string, std::string> OutputParams;        // Output extraction
};

struct ParameterBinding {
    ParameterBindingType Type;  // Literal, LocalVariable, etc.
    std::string VariableName;   // Variable name if Type==LocalVariable
    TaskValue LiteralValue;     // Default value
};
```

**Mapping for BT**:
```cpp
// BehaviorTree version:
struct BTNode {
    std::string subgraphPath;
    std::map<std::string, std::string> subgraphInputs;    // Simplified: no type checking
    std::map<std::string, std::string> subgraphOutputs;
};
```

### 2. Execution Layer

**File**: `Source/TaskSystem/VSGraphExecutor.cpp`
```cpp
int32_t VSGraphExecutor::HandleSubGraph(
    EntityID entity,
    int32_t nodeID,
    TaskRunnerComponent& runner,
    const TaskGraphTemplate& tmpl,
    LocalBlackboard& localBB,
    World* worldPtr,
    float dt,
    SubGraphCallStack& callStack)
{
    // 1. Validate node configuration
    if (node->SubGraphPath.empty())
        return FindExecTarget(nodeID, "Out", tmpl);
    
    // 2. Check depth limit
    if (callStack.Depth >= MAX_SUBGRAPH_DEPTH)  // 32
        return FindExecTarget(nodeID, "Out", tmpl);
    
    // 3. Check for cycles
    if (callStack.Contains(node->SubGraphPath))
        return FindExecTarget(nodeID, "Out", tmpl);
    
    // 4. Load SubGraph template
    const TaskGraphTemplate* subGraphTmpl = 
        AssetManager::Get().LoadTaskGraphFromFile(node->SubGraphPath, loadErrors);
    
    if (subGraphTmpl == nullptr)
        return FindExecTarget(nodeID, "Out", tmpl);
    
    // 5. Create child LocalBlackboard
    LocalBlackboard childBB;
    childBB.InitializeFromEntries(subGraphTmpl->Blackboard);
    
    // 6. Bind input parameters (parent → child)
    for (auto& [paramName, binding] : node->InputParams) {
        TaskValue value;
        if (binding.Type == ParameterBindingType::Literal)
            value = binding.LiteralValue;
        else if (binding.Type == ParameterBindingType::LocalVariable)
            if (localBB.HasVariable(binding.VariableName))
                value = localBB.GetValue(binding.VariableName);
        
        if (childBB.HasVariable(paramName))
            childBB.SetValue(paramName, value);
    }
    
    // 7. Push call stack
    callStack.Push(node->SubGraphPath);
    
    // 8. Create child runner
    TaskRunnerComponent childRunner;
    childRunner.CurrentNodeID = subGraphTmpl->EntryPointID;
    
    // 9. Execute SubGraph (single frame)
    ExecuteFrame(entity, childRunner, *subGraphTmpl, childBB, worldPtr, dt);
    
    // 10. Pop call stack
    callStack.Pop();
    
    // 11. Extract output parameters (child → parent)
    for (auto& [paramName, targetBBKey] : node->OutputParams) {
        if (childBB.HasVariable(paramName)) {
            TaskValue outputValue = childBB.GetValue(paramName);
            localBB.SetValueScoped(targetBBKey, outputValue);
        }
    }
    
    return FindExecTarget(nodeID, "Out", tmpl);
}

struct SubGraphCallStack {
    std::vector<std::string> PathStack;
    int Depth = 0;
    
    void Push(const std::string& path) { PathStack.push_back(path); Depth++; }
    void Pop() { if (!PathStack.empty()) PathStack.pop_back(); Depth--; }
    bool Contains(const std::string& path) const { ... }
    
    static const int MAX_SUBGRAPH_DEPTH = 32;
};
```

**Mapping for BT**:
```cpp
// BehaviorTree version (simpler - no blackboard type checking):
BTStatus BehaviorTreeExecutor::ExecuteSubGraph(
    const BTNode& node,
    const BehaviorTreeAsset& btAsset,
    GraphExecutionTracer& outTracer)
{
    // Similar structure, but:
    // - No LocalBlackboard type system
    // - String-based parameter passing
    // - Return BTStatus instead of int32_t
    // - Use GraphExecutionTracer instead of execution state
}
```

---

## Verification & Validation

### 3. Validator Layer

**File**: `Source/BlueprintEditor/VSGraphVerifier.cpp`
```cpp
void VSGraphVerifier::CheckSubGraphCircular(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    // DFS-based circular reference detection
    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    
    for (const auto& node : g.Nodes) {
        if (node.Type == TaskNodeType::SubGraph) {
            if (HasCircularReference(node.SubGraphPath, visiting, visited, g)) {
                AddIssue(r, VSVerificationSeverity::Error, node.ID,
                    "E004", "Circular SubGraph reference: " + node.SubGraphPath);
            }
        }
    }
}

void VSGraphVerifier::CheckSubGraphPaths(const TaskGraphTemplate& g, VSVerificationResult& r)
{
    for (const auto& node : g.Nodes) {
        if (node.Type == TaskNodeType::SubGraph) {
            if (node.SubGraphPath.empty()) {
                AddIssue(r, VSVerificationSeverity::Warning, node.ID,
                    "W003", "SubGraph node with empty path");
            }
        }
    }
}
```

**Validation Rules**:
- **E003**: Exec cycle detected (applies to SubGraph chains)
- **E004**: Circular SubGraph reference
- **W003**: SubGraph with empty SubGraphPath

**Mapping for BT**:
```cpp
// New rules for BT SubGraph:
void BehaviorTreeValidator::CheckSubGraphNodes(
    const BehaviorTreeAsset& asset,
    std::vector<ValidationError>& errors)
{
    // E501: Empty path
    // E502: File not found
    // E503: Circular reference (similar to E004)
    // E504: Depth exceeds limit
    // E505: Type mismatch (optional)
}
```

---

## Editor Integration

### 4. Canvas & UI Layer

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel_RenderingCore.cpp`
```cpp
// SubGraph node rendering in imnodes
// Rendered as rectangular node with:
// - Title: "SubGraph"
// - Subtitle: filename (truncated)
// - Color: Blue/teal to distinguish from other node types
// - Input pin: execution in
// - Output pin: execution out + conditional pins
```

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp`
```cpp
// SubGraph properties in right panel:
void RenderSubGraphProperties() {
    // File Path Input + Browse Button
    // Input Parameters Grid:
    //   - Child Parameter | Parent Variable | Remove
    // Output Parameters Grid:
    //   - Child Parameter | Parent Variable | Remove
    // Add Parameter Buttons
}
```

**File**: `Source/BlueprintEditor/VisualScriptEditorPanel_Canvas.cpp`
```cpp
// Double-click detection for SubGraph nodes
if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    if (hoveredNode && IsSubGraphNode(hoveredNode)) {
        std::string path = GetNodeSubGraphPath(hoveredNode);
        if (!path.empty()) {
            // Resolve path
            std::string resolvedPath = DataManager::Get().ResolveFilePath(path);
            // Open in new tab
            TabManager::Get().OpenFileInTab(resolvedPath);
        }
    }
}
```

**Mapping for BT**:
```cpp
// BehaviorTree version:
// - Blue rectangular node with database icon
// - No multiple pins (simple Action node)
// - Same double-click navigation
// - Similar property panel with file picker
// - Parameter UI almost identical (no type checking)
```

---

## Testing & Simulation

### 5. Testing Layer

**File**: `Source/BlueprintEditor/ExecutionTestPanel.h/cpp`
```cpp
class ExecutionTestPanel {
    void DisplayTrace(const GraphExecutionTracer& tracer);
    
    // Displays execution flow with node entry/exit events
    // Already supports arbitrary node types!
    // SubGraph execution events automatically included
};
```

**File**: `Source/BlueprintEditor/GraphExecutionTracer.h`
```cpp
class GraphExecutionTracer {
    void RecordNodeEntered(int32_t nodeId, const std::string& name, 
                          const std::string& type);
    void RecordNodeExited(int32_t nodeId, int32_t status);
    void RecordError(int32_t nodeId, const std::string& location, 
                    const std::string& message);
};
```

**Example Trace Output**:
```
[Node #15: Call SubGraph]
  → Loading: Gamedata/TaskGraph/Templates/empty_utility_ai.ats
  [Node #1: EntryPoint]
  [Node #2: Branch]
    → Condition: False
  [Node #3: Idle]
  → SubGraph Result: Success
[Node #16: Continue...]
```

**Mapping for BT**:
```cpp
// Same ExecutionTestPanel works for BT!
// BehaviorTreeExecutor already records via GraphExecutionTracer
// Just needs DisplayTrace() call with BT trace data
void BehaviorTreeRenderer::ShowExecutionTrace(const GraphExecutionTracer& trace) {
    m_executionTestPanel.DisplayTrace(trace);
}
```

---

## File Path Resolution

### 6. DataManager Integration

**File**: `Source/DataManager.h/cpp`
```cpp
std::string DataManager::ResolveFilePath(const std::string& relativePath) const
{
    // 1. Try direct path
    if (file_exists(relativePath)) return relativePath;
    
    // 2. Try Blueprints + path
    std::string blueprintPath = "Blueprints/" + relativePath;
    if (file_exists(blueprintPath)) return blueprintPath;
    
    // 3. Try Gamedata + path
    std::string gamedataPath = "Gamedata/" + relativePath;
    if (file_exists(gamedataPath)) return gamedataPath;
    
    // 4. Recursive search for just filename
    std::string filename = extract_filename(relativePath);
    std::string found = FindResourceRecursive(filename, "Blueprints");
    if (!found.empty()) return found;
    
    // 5. Final fallback
    return "";
}
```

**Used by**:
- VisualScriptRenderer::ResolvePath() (Phase 38 just implemented)
- BehaviorTreeExecutor::ExecuteSubGraph() (Phase 39 to implement)

---

## JSON Serialization Examples

### VisualScript SubGraph (Reference)

```json
{
  "id": 15,
  "type": "SubGraph",
  "nodeType": 11,
  "name": "Call Patrol",
  "position": { "x": 800, "y": 400 },
  
  "subgraph_path": "Gamedata/TaskGraph/Templates/empty_utility_ai.ats",
  
  "inputParams": {
    "patrolSpeed": {
      "type": "LocalVariable",
      "variableName": "moveSpeed"
    },
    "targetRange": {
      "type": "Literal",
      "value": { "type": "Float", "value": 50.0 }
    }
  },
  
  "outputParams": {
    "foundTarget": "targetSpotted",
    "targetPos": "lastKnownPos"
  }
}
```

### BehaviorTree SubGraph (Proposed)

```json
{
  "id": 10,
  "type": "SubGraph",
  "name": "Call Patrol Behavior",
  
  "subgraphPath": "Blueprints/AI/patrol.bt.json",
  
  "subgraphInputs": {
    "patrolSpeed": "moveSpeed",
    "searchRadius": "detectionRange"
  },
  
  "subgraphOutputs": {
    "foundTarget": "targetSpotted"
  }
}
```

---

## Key Differences: VisualScript vs BehaviorTree SubGraph

| Aspect | VisualScript | BehaviorTree |
|--------|--------------|--------------|
| **Node Type** | TaskNodeType::SubGraph | BTNodeType::SubGraph |
| **Parameter Type** | ParameterBinding (typed) | String map (untyped) |
| **Blackboard** | LocalBlackboard + types | Simple string dict |
| **Execution** | Single-frame recursive | Same (BTStatus) |
| **Depth Limit** | 32 levels | 32 levels |
| **Cycle Detection** | Call stack + DFS | Call stack |
| **File Types** | Only .ats | .bt.json or .ats |
| **Validation** | VSGraphVerifier rules | BehaviorTreeValidator rules |
| **Testing** | ExecutionTestPanel | ExecutionTestPanel (reuse) |

---

## Implementation Roadmap

```
Week 1: Data Model (Reference: TaskGraphTemplate.h)
  ├─ Add BTNodeType::SubGraph enum
  ├─ Extend BTNode with subgraph fields
  └─ JSON serialization (reference: TaskGraphLoader.cpp)

Week 2: Execution (Reference: VSGraphExecutor.cpp)
  ├─ Implement ExecuteSubGraph()
  ├─ Add SubGraphCallStack
  └─ Integration with BehaviorTreeExecutor::ExecuteNode()

Week 3: Editor (Reference: VisualScriptEditorPanel_Properties.cpp)
  ├─ Add property panel section (reference: RenderSubGraphProperties)
  ├─ Add canvas rendering (reference: node rendering in imnodes)
  └─ Add double-click navigation (reference: VisualScriptEditorPanel_Canvas.cpp)

Week 4: Validation & Testing (Reference: VSGraphVerifier.cpp)
  ├─ Add validator rules (reference: CheckSubGraphCircular)
  └─ ExecutionTestPanel integration (reference: GraphExecutionTracer)
```

---

## Code Review Checklist

When reviewing Phase 39 implementation, compare against:

- ✅ **Data Model**: BTNode extends match TaskNodeDefinition structure
- ✅ **Execution**: ExecuteSubGraph() follows HandleSubGraph() pattern
- ✅ **Call Stack**: SubGraphCallStack matches logic in VSGraphExecutor
- ✅ **Depth Limit**: Use 32 (MAX_SUBGRAPH_DEPTH constant)
- ✅ **Cycle Detection**: DFS or call stack tracking
- ✅ **Path Resolution**: Use DataManager::ResolveFilePath()
- ✅ **Parameter Binding**: Simple string-based (not typed)
- ✅ **Error Handling**: Log to GraphExecutionTracer
- ✅ **Validation**: New E501-E505 rules implemented
- ✅ **Testing**: Uses ExecutionTestPanel::DisplayTrace()

---

## Files to Cross-Reference During Development

### Must-Read (Core Patterns)
1. `Source/TaskSystem/TaskGraphTemplate.h` — Node structure
2. `Source/TaskSystem/VSGraphExecutor.cpp` — Execution logic (HandleSubGraph)
3. `Source/BlueprintEditor/VSGraphVerifier.cpp` — Validation patterns
4. `Source/BlueprintEditor/ExecutionTestPanel.h/cpp` — Testing integration

### Recommended (UI Patterns)
5. `Source/BlueprintEditor/VisualScriptEditorPanel_Properties.cpp` — Property panel
6. `Source/BlueprintEditor/VisualScriptEditorPanel_Canvas.cpp` — Navigation
7. `Source/BlueprintEditor/GraphExecutionTracer.h` — Event recording

### Reference (File Resolution)
8. `Source/DataManager.h` — ResolveFilePath()
9. `Source/BlueprintEditor/VisualScriptRenderer.cpp` — Path resolution usage (Phase 38)

---

## Success Criteria (Validation)

Check these criteria against VisualScript SubGraph implementation:

- ✅ Subgraph nodes execute recursively
- ✅ Cycle detection prevents infinite loops
- ✅ Depth limit prevents stack overflow (max 32)
- ✅ Parameters pass from parent to child correctly
- ✅ Output parameters extracted back to parent
- ✅ Error handling is graceful (no crashes)
- ✅ Validation catches empty paths
- ✅ Validation detects circular references
- ✅ Execution trace shows subgraph nesting
- ✅ Double-click navigation works
- ✅ Property panel allows parameter editing
- ✅ File picker modal works for path selection

---

## Conclusion

VisualScript SubGraph (Phases 8, 21, 24) provides a **complete reference implementation** for Phase 39 BehaviorTree SubGraph. The core patterns are proven and can be adapted with minimal changes due to the similar execution models.

**Key Takeaway**: Reuse patterns, not reinvent. Most design decisions are already made and validated in VisualScript.
