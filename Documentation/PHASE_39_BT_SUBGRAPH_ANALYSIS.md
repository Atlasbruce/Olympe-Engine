# Phase 39: BehaviorTree SubGraph Node - Feasibility Analysis

## Executive Summary

**Status**: ✅ **HIGHLY FEASIBLE** — Low to medium complexity
**Estimated Effort**: 15-25 developer-days
**Complexity**: Medium (moderate dependencies, leverages existing patterns)
**Risk Level**: Low (well-established patterns in VisualScript to adapt)

**Key Finding**: The architecture for SubGraph execution already exists in VisualScript. BehaviorTree can leverage the same patterns with adaptation for BT-specific semantics.

---

## Current State Analysis

### 1. SubGraph in VisualScript (REFERENCE ARCHITECTURE)

#### Data Model
```cpp
// In TaskNodeDefinition (VisualScript)
TaskNodeType Type = TaskNodeType::SubGraph;
std::string SubGraphPath;           // File path to external .ats file
std::map<std::string, ParameterBinding> InputParams;   // Bindings to parent BB
std::map<std::string, std::string> OutputParams;        // Extract back to parent
```

#### Execution Model (VSGraphExecutor::HandleSubGraph)
```cpp
1. Load external graph file → TaskGraphTemplate
2. Create child LocalBlackboard
3. Bind input parameters (parent → child)
4. Execute child graph (single frame)
5. Extract output parameters (child → parent)
6. Handle recursion depth limit (max 32)
7. Detect circular dependencies
```

#### Key Characteristics
- **Execution Context**: Single-frame recursive execution
- **Blackboard Binding**: Explicit input/output parameter mapping
- **Depth Limit**: MAX_SUBGRAPH_DEPTH = 32 (prevent stack overflow)
- **Cycle Detection**: Call stack tracking (SubGraphCallStack)
- **Load Strategy**: Lazy load via AssetManager on execution
- **Error Handling**: Graceful fallback to next node on error

#### Editor Support (VisualScriptEditorPanel)
- ✅ Double-click navigation (Phase 38 - just implemented!)
- ✅ SubGraph properties panel
- ✅ Parameter binding UI
- ✅ File path picker modal
- ✅ Validation (VSGraphVerifier rule E024 - empty SubGraphPath)

#### Verification (VSGraphVerifier)
- ✅ **E003** — Circular dependency detection (DFS over subgraph refs)
- ✅ **E004** — Circular SubGraph reference (Phase 21-A)
- ✅ **W003** — SubGraph with empty SubGraphPath
- ✅ **Recursive validation** of external graphs

---

## 2. BehaviorTree Current Architecture

### Node Types (BTNodeType enum)
```cpp
Selector        // OR composite
Sequence        // AND composite
Condition       // Leaf - evaluates condition
Action          // Leaf - performs action
Inverter        // Decorator
Repeater        // Decorator
Root            // Phase 38b - entry point
OnEvent         // Phase 38b - event-driven entry
// MISSING: SubGraph  ← TO BE ADDED
```

### Execution Model (BehaviorTreeExecutor)
```cpp
BTStatus ExecuteTree(const BehaviorTreeAsset&)
  → BTStatus ExecuteNode(uint32_t nodeId)
      → ExecuteSelector / ExecuteSequence / ExecuteCondition / ExecuteAction
      → Recursive descent with depth limit
```

### Data Structures
```cpp
struct BTNode {
    BTNodeType type;
    uint32_t id;
    std::vector<uint32_t> childIds;    // For Selector/Sequence
    uint32_t decoratorChildId;         // For Inverter/Repeater
    BTActionType actionType;           // For Action nodes
    std::string name;
    std::map<std::string, std::string> stringParams;
    std::map<std::string, int> intParams;
    std::map<std::string, float> floatParams;
};

struct BehaviorTreeAsset {
    uint32_t rootNodeId;
    std::vector<BTNode> nodes;
    std::string name;
    // MISSING: Support for external graph references
};
```

### Storage Format (JSON)
- Native BT format (not ATS v4)
- Single tree per file (no embedded subgraphs)
- No parameter binding infrastructure
- No blackboard inheritance

---

## 3. Simulation & Verification Tool

### ExecutionTestPanel (Phase 24.3)
```cpp
class ExecutionTestPanel {
    void Render();                          // ImGui panel
    void DisplayTrace(const GraphExecutionTracer&);  // Show execution trace
    std::vector<ValidationError> RunExecutionTest();
};
```

### GraphExecutionTracer (For trace recording)
```cpp
void RecordNodeEntered(int32_t nodeId, const std::string& name, const std::string& type);
void RecordNodeExited(int32_t nodeId, BTStatus status);
void RecordError(int32_t nodeId, const std::string& location, const std::string& message);
void RecordExecutionCompleted(bool success, const std::string& reason);
```

### BehaviorTreeExecutor (Native BT Executor)
- ✅ Already integrated with GraphExecutionTracer
- ✅ Records node execution, status, decisions
- ✅ Can be extended to handle SubGraph nodes
- ✅ Depth limit tracking (m_maxDepth)

---

## Proposed Implementation Strategy

### Phase 39 - Part A: Data Model & Serialization

#### 1. Extend BTNodeType enum
```cpp
enum class BTNodeType {
    // ... existing types ...
    SubGraph,           // NEW: References external BT or ATS file
};
```

#### 2. Extend BTNode structure
```cpp
struct BTNode {
    // ... existing fields ...
    
    // Phase 39: SubGraph support
    std::string subgraphPath;           // Path to external file (.bt.json or .ats)
    std::map<std::string, std::string> subgraphInputs;   // "parentVar" → "childVar"
    std::map<std::string, std::string> subgraphOutputs;  // "childVar" → "parentVar"
};
```

#### 3. JSON Serialization
```json
{
  "id": 10,
  "type": "SubGraph",
  "name": "Call Patrol Behavior",
  "subgraphPath": "Blueprints/AI/patrol.bt.json",
  "subgraphInputs": {
    "patrolSpeed": "globalPatrolSpeed",
    "targetDist": "attackRange"
  },
  "subgraphOutputs": {
    "foundTarget": "targetSpotted"
  }
}
```

#### 4. Canvas Representation
- Node shape: **Rounded rectangle** (similar to VisualScript SubGraph)
- Color: **Blue** (distinct from Action/Condition)
- Double-click behavior: Open referenced graph in new tab
- Icon: **Database + Arrow** (indicates external reference)

---

### Phase 39 - Part B: Execution Engine

#### 1. BehaviorTreeExecutor Enhancement
```cpp
BTStatus BehaviorTreeExecutor::ExecuteSubGraph(
    const BTNode& node,
    const BehaviorTreeAsset& parentAsset,
    GraphExecutionTracer& tracer)
{
    // 1. Load external file
    BehaviorTreeAsset* childAsset = LoadBTAsset(node.subgraphPath);
    if (!childAsset) {
        tracer.RecordError(node.id, "SubGraph", "File not found: " + node.subgraphPath);
        return BTStatus::Failure;
    }
    
    // 2. Check depth limit & cycles
    if (++m_maxDepth > 32) {
        tracer.RecordError(node.id, "SubGraph", "Recursion depth exceeded");
        return BTStatus::Failure;
    }
    if (m_callStack.Contains(node.subgraphPath)) {
        tracer.RecordError(node.id, "SubGraph", "Circular reference detected");
        return BTStatus::Failure;
    }
    
    // 3. Create child executor (or reuse state)
    m_callStack.Push(node.subgraphPath);
    
    // 4. Execute child tree
    BTStatus result = ExecuteNode(childAsset->rootNodeId, *childAsset, tracer);
    
    // 5. Pop and return
    m_callStack.Pop();
    m_maxDepth--;
    
    tracer.RecordNodeExited(node.id, result);
    return result;
}
```

#### 2. Call Stack Management
```cpp
struct BTSubGraphCallStack {
    std::vector<std::string> paths;
    int Depth = 0;
    
    void Push(const std::string& path) { paths.push_back(path); Depth++; }
    void Pop() { if (!paths.empty()) paths.pop_back(); Depth--; }
    bool Contains(const std::string& path) const {
        for (const auto& p : paths) if (p == path) return true;
        return false;
    }
};
```

#### 3. Hybrid Execution (BT Referencing ATS)
```cpp
// Scenario: BT SubGraph node references an ATS file
if (node.subgraphPath.endswith(".ats")) {
    // Load as VisualScript, execute with VSGraphExecutor
    TaskGraphTemplate* vsGraph = LoadATSFile(node.subgraphPath);
    int result = VSGraphExecutor::ExecuteFrame(...);  // Reuse VisualScript execution
    return result ? BTStatus::Success : BTStatus::Failure;
}
```

---

### Phase 39 - Part C: Editor Integration

#### 1. Canvas Rendering (BTNodePalette)
- **Add to palette**: "SubGraph" action node type
- **Render** with distinct visual style (blue box, database icon)
- **Context menu**: Edit file path, view parameters

#### 2. Property Panel (BTNodePropertyPanel)
```cpp
// New section in properties for SubGraph nodes:
- File Path Input (with "Browse" button)
- Input Parameter Grid:
  - Parent Variable | Child Variable | Type
- Output Parameter Grid:
  - Child Variable | Parent Variable | Type
- "Load Graph" button to preview external tree
```

#### 3. Double-Click Navigation
- Double-click SubGraph node → open referenced file in new tab
- Uses DataManager::ResolveFilePath() (Phase 38 pattern)
- Supports both .bt.json and .ats files

#### 4. File Picker Modal
```cpp
// Extend SubGraphFilePicker (or create BTSubGraphPicker)
BehaviorTreeFilePicker::ShowDialog(
    onSelected = [&](const std::string& path) {
        selectedNode->subgraphPath = path;
        isDirty = true;
    }
);
```

---

### Phase 39 - Part D: Verification & Testing

#### 1. BehaviorTreeValidator Enhancement
```cpp
// New validation rules:
// E501 — SubGraph with empty path
// E502 — SubGraph references non-existent file
// E503 — SubGraph circular reference detected
// E504 — SubGraph recursion depth would exceed limit
// E505 — SubGraph mismatch (ATS expects different inputs)
```

#### 2. ExecutionTestPanel Integration
```cpp
void ExecutionTestPanel::RunBTTest(const BehaviorTreeAsset& asset) {
    BehaviorTreeExecutor executor;
    GraphExecutionTracer tracer;
    
    BTStatus result = executor.ExecuteTree(asset, tracer);
    
    // Display trace with SubGraph execution details
    DisplayTrace(tracer);
}
```

#### 3. Execution Trace for SubGraphs
```
[Node #5: Call Patrol SubGraph]
  → Loading: Blueprints/AI/patrol.bt.json
  → Depth: 2/32
  [Node #1: Root]
  [Node #2: Selector]
    [Node #3: Sequence]
      [Node #4: Check Target Visible]
        → Status: Failure
    [Node #5: Idle Action]
      → Status: Success
  → SubGraph Result: Success
[Node #6: Continue...]
```

---

## Dependencies & Integration Points

### Canvas Dependencies
| Component | Impact | Status |
|-----------|--------|--------|
| BTNodeRenderer | Add SubGraph visual style (blue, icon) | **Minor** - Simple render addition |
| BTNodePalette | Add "SubGraph" to action types | **Minor** - Registry entry |
| PrefabCanvas (generic) | No changes needed | ✅ Already supports generic node types |

### Editor Dependencies
| Component | Impact | Status |
|-----------|--------|--------|
| BTNodePropertyPanel | Add SubGraph parameter UI | **Medium** - New section needed |
| Node Graph Manager | Add SubGraph serialization | **Minor** - JSON handlers exist |
| File Picker | Reuse from VisualScript | **Minor** - Already works |
| Double-click handler | Add to BehaviorTreeRenderer | **Minor** - Pattern established |

### Execution Dependencies
| Component | Impact | Status |
|-----------|--------|--------|
| BehaviorTreeExecutor | Add ExecuteSubGraph() method | **Medium** - ~100 lines of code |
| BehaviorTreeAsset | Add call stack tracking | **Minor** - Add one member |
| GraphExecutionTracer | Already supports custom events | ✅ No changes needed |
| DataManager | Already has ResolveFilePath() | ✅ Phase 38 ready |

### Testing Dependencies
| Component | Impact | Status |
|-----------|--------|--------|
| ExecutionTestPanel | Add SubGraph trace display | ✅ Already supports DisplayTrace() |
| BehaviorTreeValidator | Add 5 new validation rules | **Minor** - Standard rules |

---

## Risk Assessment

### Low Risk
- ✅ **Execution logic** — Established pattern from VisualScript
- ✅ **Serialization** — JSON structure is straightforward
- ✅ **Tooling** — ExecutionTestPanel already ready
- ✅ **Dependencies** — No breaking changes to existing code

### Medium Risk
- ⚠️ **Hybrid execution** (BT → ATS) — Need careful context switching
- ⚠️ **Parameter mapping** — BT uses string maps, need type coercion
- ⚠️ **Blackboard scope** — Each graph has own scope, need clear passing semantics

### Mitigation Strategies
1. **Hybrid execution**: Treat ATS graphs as "atomic actions" in BT context
2. **Parameter mapping**: Use string-based exchange layer (like VisualScript)
3. **Blackboard scope**: Document strict isolation (no cross-reference)

---

## Dependency on Related Phases

### Phase 38 (COMPLETED ✅)
- SubGraph file path resolution via DataManager::ResolveFilePath()
- Double-click navigation infrastructure
- **Status**: Ready to use

### Phase 35 (COMPLETED ✅)
- ExecutionTestPanel
- GraphExecutionTracer
- Canvas state management
- **Status**: Ready to use

### Phase 37 (COMPLETED ✅)
- Minimap centralization (inheritance via CustomCanvasEditor)
- **Status**: Inherited automatically

### Phase 2 (COMPLETED ✅)
- Modern split-panel UI layout for BT editor
- BTNodePalette and BTNodePropertyPanel
- **Status**: Ready to extend

---

## Proposed Work Breakdown

### Sprint 1: Data Model & Serialization (3-5 days)
1. Add `SubGraph` to `BTNodeType` enum
2. Extend `BTNode` with subgraph fields
3. Update JSON serialization handlers
4. Create unit tests for serialization

### Sprint 2: Execution Engine (5-7 days)
1. Implement `ExecuteSubGraph()` in BehaviorTreeExecutor
2. Add call stack tracking
3. Implement cycle detection
4. Handle hybrid execution (BT ↔ ATS)
5. Create integration tests

### Sprint 3: Editor Integration (4-6 days)
1. Add SubGraph to node palette
2. Extend property panel with file picker
3. Implement double-click navigation
4. Add canvas rendering for SubGraph nodes
5. Editor validation tests

### Sprint 4: Verification & Testing (2-4 days)
1. Add validation rules to BehaviorTreeValidator
2. Integrate with ExecutionTestPanel
3. Create execution trace display
4. System integration tests

### Sprint 5: Documentation & Polish (1-2 days)
1. Update node documentation
2. Add example BT with SubGraphs
3. Create user guide for SubGraph feature
4. Performance profiling

---

## Success Criteria

- ✅ SubGraph nodes execute correctly (recursive, cyclic)
- ✅ Parameter binding works (input/output passing)
- ✅ File resolution uses DataManager (Phase 38 pattern)
- ✅ Editor supports double-click navigation
- ✅ Property panel allows path and parameter editing
- ✅ Validation detects cycles and missing files
- ✅ ExecutionTestPanel displays trace correctly
- ✅ Hybrid execution (BT ↔ ATS) works seamlessly
- ✅ Depth limit enforced (max 32 levels)
- ✅ Zero breaking changes to existing code

---

## Implementation Recommendations

### 1. Start with Execution Engine
- Core logic first, editor follows
- Easier to test without UI complexity
- Leverages established VisualScript patterns

### 2. Use String-Based Parameter Exchange
- No need for complex type system
- Blackboard values already string-serializable
- Matches VisualScript approach

### 3. Treat ATS as Atomic Action
- BT SubGraph (ATS reference) executes entire graph in one "step"
- Returns Success/Failure based on execution result
- Keeps BT semantics clean

### 4. Document Scope Isolation
- Parent and child graphs have separate blackboards
- Only explicitly passed parameters cross boundary
- Clear, predictable data flow

### 5. Reuse ExecutionTestPanel
- Don't build separate BT simulator
- Extend existing ExecutionTestPanel
- Display SubGraph nesting in trace

---

## Files to Create/Modify

### New Files
- `Source/BlueprintEditor/BTSubGraphValidator.cpp/h` (validation rules)
- `Tests/BlueprintEditor/BTSubGraphTest.cpp` (unit tests)

### Modified Files
- `Source/AI/BehaviorTree.h` — Add SubGraph to BTNodeType enum, extend BTNode
- `Source/AI/BehaviorTree.cpp` — JSON serialization for SubGraph
- `Source/BlueprintEditor/BehaviorTreeExecutor.h/cpp` — Add ExecuteSubGraph()
- `Source/BlueprintEditor/BehaviorTreeValidator.h/cpp` — Add validation rules
- `Source/BlueprintEditor/BTNodePropertyPanel.h/cpp` — Add SubGraph section
- `Source/BlueprintEditor/BTNodeRenderer.h/cpp` — Render SubGraph nodes
- `Source/BlueprintEditor/BehaviorTreeRenderer.cpp` — Double-click handler
- `Source/BlueprintEditor/ExecutionTestPanel.h/cpp` — Trace display (minor)

### No Changes Required
- BTNodePalette — Registry-based, auto-includes new type
- PrefabCanvas — Generic, already supports new node types
- DataManager — Phase 38 ready
- GraphExecutionTracer — Already sufficient
- TabManager — Already supports navigation

---

## Conclusion

**Phase 39 is highly feasible** with **low to medium complexity**. The VisualScript SubGraph implementation provides a solid blueprint, and BehaviorTree's architecture is similar enough that adaptation is straightforward. The main work is in execution logic and editor UI, both of which follow established patterns.

**Critical Success Factor**: Reuse existing infrastructure (ExecutionTestPanel, DataManager::ResolveFilePath, GraphExecutionTracer) rather than building new systems.

**Timeline**: 15-25 developer-days assuming continuous work on high-priority items in order.
