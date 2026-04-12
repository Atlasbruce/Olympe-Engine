# BehaviorTree Graph Simulation - Analyse Révisée (Framework Générique)

**Date** : 2026-03-24  
**Phase** : BT-SIM-001 (BehaviorTree Graph Simulation - v2 REVISED)  
**Statut** : Analyse révisée avec réutilisation du framework générique  
**Constraint** : Réutiliser GraphExecutionSimulator, GraphExecutionTracer, ExecutionTestPanel comme base générique

---

## 1. ARCHITECTURE RÉVISÉE : "GRAPH-AS-DOCUMENT" PATTERN

### 1.1 Paradigme Shift

**Ancien approche (❌ Rejetée)** :
```
BehaviorTree → BehaviorTreeExecutionSimulator
             → BehaviorTreeExecutionTracer
             → BTSimulationPanel
(Duplicates le code déjà existant pour VisualScript)
```

**Nouveau approche (✅ Framework Générique)** :
```
BehaviorTree (Document)
    ↓
BehaviorTreeGraphAdapter (Convertisseur)
    ├─ Transforme BTNode → TaskNodeDefinition
    ├─ Adapte BTStatus → Execution Status
    ├─ Mappe Connections (parent-child)
    ↓
GraphExecutionSimulator (Generic Framework)
    └─ Traite BT comme "graph document"
    
    ↓
GraphExecutionTracer (Generic)
    └─ Enregistre events (même format que VS)
    
    ↓
ExecutionTestPanel (Generic)
    └─ Affiche résultats (même UI que VS)
```

### 1.2 Avantages de cette Architecture

| Aspect | Bénéfice |
|--------|---------|
| **Code Reuse** | ✅ 0 duplication : GraphExecutionSimulator/Tracer/Panel réutilisés |
| **Maintenance** | ✅ Bug fixes au framework bénéficient aux 2 systèmes |
| **Consistency** | ✅ VisualScript et BT partagent même UI, même format trace |
| **Extensibilité** | ✅ Framework peut supporter d'autres "graph documents" futures |
| **Performance** | ✅ Codepaths optimisés au niveau framework (centralisé) |
| **Testing** | ✅ Tests du framework valident VS + BT automatiquement |

---

## 2. ANALYSE DÉTAILLÉE : ADAPTATION LAYER

### 2.1 Conversion BehaviorTree ↔ GraphFormat

**Concept** :
- BehaviorTree est un **arbre hiérarchique** (parent-child)
- GraphExecutionSimulator attend un **graph with connections** (TaskNodeDefinition + pins)
- Adapter convertit la hiérarchie BT en graph plat pour le framework

#### **Mapping BTNode → TaskNodeDefinition**

```cpp
// BehaviorTree native
struct BTNode {
    uint32_t id;
    BTNodeType type;  // Selector, Sequence, Condition, Action, Inverter, Repeater
    std::vector<uint32_t> childIds;  // Hierarchical
    std::string name;
    BTConditionType conditionType;
    BTActionType actionType;
};

// Conversion vers Graph Format
TaskNodeDefinition graphNode;
graphNode.NodeID = btNode.id;
graphNode.NodeName = btNode.name;

// TYPE MAPPING
switch (btNode.type) {
    case BTNodeType::Selector:
        graphNode.Type = TaskNodeType::Selector;  // Hypothetical or custom
    case BTNodeType::Sequence:
        graphNode.Type = TaskNodeType::Sequence;
    case BTNodeType::Condition:
        graphNode.Type = TaskNodeType::Condition;
    case BTNodeType::Action:
        graphNode.Type = TaskNodeType::Action;
    case BTNodeType::Inverter:
        graphNode.Type = TaskNodeType::Inverter;  // Custom decorator type
    case BTNodeType::Repeater:
        graphNode.Type = TaskNodeType::Repeater;
}

// CHILDREN → CONNECTIONS (implicit in graph traversal)
for (uint32_t childId : btNode.childIds) {
    graphNode.Dependencies.push_back(childId);
}

// STATUS MAPPING
// BTStatus::Success → Execution success
// BTStatus::Failure → Execution failure
// BTStatus::Running → Execution running
```

#### **Hierarchical Connections → Graph Connections**

```cpp
// BehaviorTree: Parent-Child relationship (implicit)
Selector (ID=1)
├─ Sequence (ID=2)    // child
│  ├─ Condition (ID=3)
│  └─ Action (ID=4)
└─ Sequence (ID=5)    // child

// Graph Format: Explicit connections
TaskConnection connections[] = {
    { SourceNodeID=1, SourcePinName="Control",  
      TargetNodeID=2, TargetPinName="In" },
    { SourceNodeID=1, SourcePinName="Control",
      TargetNodeID=5, TargetPinName="In" },
    { SourceNodeID=2, SourcePinName="Control",
      TargetNodeID=3, TargetPinName="In" },
    { SourceNodeID=2, SourcePinName="Control",
      TargetNodeID=4, TargetPinName="In" }
};

// Simulator interpretes as:
// - Node 1 (Selector) controls nodes 2, 5
// - Node 2 (Sequence) controls nodes 3, 4
// - Normal recursive descent applies
```

### 2.2 BehaviorTreeGraphAdapter Class

**Fichier** : `Source/NodeGraphShared/BehaviorTreeGraphAdapter.h/cpp`

```cpp
#pragma once

#include "../TaskSystem/TaskGraphTemplate.h"
#include "../AI/BehaviorTree.h"
#include <memory>

namespace Olympe {

/**
 * @class BehaviorTreeGraphAdapter
 * @brief Adapts BehaviorTreeAsset to TaskGraphTemplate format for generic simulation.
 *
 * @details
 * Converts hierarchical BehaviorTree structure into a flat graph representation
 * compatible with GraphExecutionSimulator framework. Allows reusing generic
 * simulation infrastructure (simulator, tracer, UI panel) for behavior trees.
 *
 * PATTERN: Adapter - Converts BT interface to Graph interface.
 * PURPOSE: Enable "graph-as-document" simulation for heterogeneous graph types.
 */
class BehaviorTreeGraphAdapter {
public:
    BehaviorTreeGraphAdapter();
    ~BehaviorTreeGraphAdapter();

    /**
     * @brief Converts a BehaviorTreeAsset to TaskGraphTemplate format.
     * @param btAsset       Source behavior tree asset.
     * @return              TaskGraphTemplate suitable for GraphExecutionSimulator.
     *
     * @details
     * This is the core conversion method. Creates a new TaskGraphTemplate where:
     * - Each BTNode becomes a TaskNodeDefinition
     * - BTNode.type → TaskNodeDefinition.Type (Selector, Sequence, etc.)
     * - BTNode.childIds → Explicit TaskConnection entries
     * - BTNode properties (name, params) preserved in metadata
     *
     * The resulting graph can be directly passed to GraphExecutionSimulator
     * without any BehaviorTree-specific code.
     */
    static std::unique_ptr<TaskGraphTemplate> AdaptToTaskGraph(
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Reverse: Maps TaskGraphTemplate results back to BehaviorTree context.
     * @param graphTemplate     Execution results from GraphExecutionSimulator.
     * @param btAsset           Original BehaviorTree for context.
     * @return                  Human-readable trace with BT-specific formatting.
     *
     * @details
     * Interprets generic graph execution trace and re-formats it with:
     * - BT-specific keywords: "Selector", "Sequence", "Condition", "Action"
     * - Status symbols: ✓ (SUCCESS), ✗ (FAILURE), ⊙ (RUNNING)
     * - Hierarchical indentation matching BT structure
     */
    static std::string FormatTraceForBehaviorTree(
        const GraphExecutionTracer& tracer,
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Validates BT structure before simulation.
     * @param btAsset   BehaviorTree to validate.
     * @return          true if valid (no orphaned nodes, valid connections).
     */
    static bool ValidateTreeStructure(const BehaviorTreeAsset& btAsset);

private:
    /**
     * @brief Helper: Recursively adds node and its children to graph.
     */
    static void AddNodeToGraph(const BTNode& btNode,
                              const BehaviorTreeAsset& btAsset,
                              TaskGraphTemplate& outGraph,
                              std::map<uint32_t, int32_t>& btToGraphIdMap);

    /**
     * @brief Helper: Creates TaskNodeDefinition from BTNode.
     */
    static TaskNodeDefinition CreateGraphNode(const BTNode& btNode);

    /**
     * @brief Helper: Maps BTStatus to execution status string.
     */
    static std::string StatusToString(BTStatus status);
};

}  // namespace Olympe
```

### 2.3 Implementation Strategy

#### **Step 1: Convert BT to Graph Format**
```cpp
// User clicks "Run Graph" on BehaviorTreeRenderer
void BehaviorTreeRenderer::OnRunGraphClicked() {
    // 1. Adapt BT to graph format
    auto taskGraph = BehaviorTreeGraphAdapter::AdaptToTaskGraph(m_currentBT);
    
    if (!taskGraph) {
        LogError("Failed to adapt BehaviorTree to graph format");
        return;
    }
    
    // 2. Use GENERIC framework
    GraphExecutionSimulator simulator;
    GraphExecutionTracer tracer;
    SimulationOptions options;
    options.maxStepsPerFrame = 1000;
    
    // 3. Simulate (framework doesn't know it's a BT!)
    auto errors = simulator.SimulateExecution(*taskGraph, options, tracer);
    
    // 4. Format trace back to BT context
    std::string btTrace = BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree(
        tracer, m_currentBT);
    
    // 5. Display via EXISTING panel
    m_executionTestPanel->DisplaySimulationResults(tracer, btTrace, "BT");
}
```

---

## 3. MODIFICATION DES TYPES EXISTANTS

### 3.1 Extension de TaskNodeType (Optional)

**Current** (VisualScript only):
```cpp
enum class TaskNodeType : uint8_t {
    Start = 0,
    End,
    Condition,
    Action,
    // ...
};
```

**Extended** (Support BT):
```cpp
enum class TaskNodeType : uint8_t {
    // VisualScript types
    Start = 0,
    End,
    Condition,
    Action,
    // ...
    
    // BehaviorTree types (NEW)
    BT_Selector = 50,      // Virtual: "OR" node
    BT_Sequence = 51,      // Virtual: "AND" node
    BT_Inverter = 52,      // Virtual: Decorator
    BT_Repeater = 53,      // Virtual: Decorator
};
```

**Alternative** (More Generic):
- Garder types actuels
- Utiliser `TaskNodeDefinition::Metadata` pour stocker type BT
- Framework agnostique quant aux types spécifiques

### 3.2 Extension de ExecutionEvent (Optional)

**Current**:
```cpp
struct ExecutionEvent {
    int32_t nodeId;
    std::string nodeName;
    float timestamp;
    std::string message;
};
```

**Extended** (Support BT semantics):
```cpp
struct ExecutionEvent {
    int32_t nodeId;
    std::string nodeName;
    float timestamp;
    std::string message;
    
    // NEW: BT-specific fields (optional, for richer trace)
    struct {
        std::string nodeType;    // "Selector", "Sequence", etc.
        std::string statusSymbol; // "✓", "✗", "⊙"
        int32_t branchTaken;     // For Selector: which child succeeded
        int32_t childCount;      // For composite: number of children
    } btMetadata;  // Only populated when source is BT
};
```

---

## 4. SIMULATION FLOW (Framework-based)

```
┌─────────────────────────────────────────────────────────┐
│ BehaviorTreeRenderer::OnRunGraphClicked()               │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ BehaviorTreeGraphAdapter::AdaptToTaskGraph(btAsset)     │
│ ├─ Iterate BTNode[] in btAsset                          │
│ ├─ For each: Create TaskNodeDefinition equivalent      │
│ ├─ Map childIds → TaskConnections                      │
│ └─ Return TaskGraphTemplate (generic format)            │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ GraphExecutionSimulator::SimulateExecution(taskGraph)   │
│ ├─ Framework DOESN'T KNOW it's a BehaviorTree          │
│ ├─ Traverses generic graph structure                   │
│ ├─ Calls generic node execution handlers               │
│ └─ Records generic ExecutionEvents                     │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ GraphExecutionTracer::RecordDataPinResolved(),etc.      │
│ └─ Builds event log (generic format)                   │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree()  │
│ ├─ Post-process tracer events                          │
│ ├─ Add BT-specific formatting:                         │
│ │  ├─ "Selector" instead of generic "Composite 50"    │
│ │  ├─ Status symbols (✓/✗/⊙)                         │
│ │  └─ Hierarchical indentation                         │
│ └─ Return human-readable BT trace                      │
└──────────────────┬──────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────┐
│ ExecutionTestPanel::Render()                            │
│ ├─ Tab "Trace Log" → shows BT trace                     │
│ ├─ Tab "Errors" → filtered errors                      │
│ ├─ Tab "Summary" → execution stats                     │
│ └─ All from SAME panel, just different content         │
└─────────────────────────────────────────────────────────┘
```

---

## 5. SIMULATION LOGIC ADAPTATION

### 5.1 Node Type Handling in Simulator

**Current** (VisualScript-only):
```cpp
// GraphExecutionSimulator::SimulateExecution()
switch (node->Type) {
    case TaskNodeType::Start: ... break;
    case TaskNodeType::End: ... break;
    case TaskNodeType::Condition: ... break;
    case TaskNodeType::Action: ... break;
}
```

**Extended** (Support BT via adapter types):
```cpp
// GraphExecutionSimulator::SimulateExecution()
// NO CHANGES NEEDED IF using metadata approach
// OR minimal case additions:
switch (node->Type) {
    case TaskNodeType::Start: ... break;
    case TaskNodeType::End: ... break;
    
    // NEW: BT types (only when graph is adapted from BT)
    case TaskNodeType::BT_Selector:
        // Selector logic: iterate children, return SUCCESS if any succeeds
        for (const auto& conn : ExitConnections) {
            status = SimulateChild(conn.TargetNodeID);
            if (status == ExecutionStatus::Success) return Success;
        }
        return Failure;
    
    case TaskNodeType::BT_Sequence:
        // Sequence logic: iterate children, return FAILURE if any fails
        for (const auto& conn : ExitConnections) {
            status = SimulateChild(conn.TargetNodeID);
            if (status == ExecutionStatus::Failure) return Failure;
        }
        return Success;
    
    // ... etc for Inverter, Repeater
}
```

### 5.2 Condition & Action Evaluation

**BehaviorTree-specific**:
```cpp
// Inside BehaviorTreeGraphAdapter::AdaptToTaskGraph()
// When converting BTNode::Condition to TaskNodeDefinition:

TaskNodeDefinition condNode = CreateGraphNode(btCondition);
condNode.NodeName = "Condition: " + btCondition.name;

// Store BT-specific eval info in metadata
condNode.Metadata["bt_condition_type"] = ConditionTypeToString(
    btCondition.conditionType);
condNode.Metadata["bt_condition_param"] = std::to_string(
    btCondition.conditionParam);

// Simulator will call generic evaluation, but we intercept via:
// Option A: Custom evaluation handler registered for BT node types
// Option B: Metadata interpretation during trace formatting
```

---

## 6. COMPONENTS & FILES

### 6.1 NEW Files

```
Source/NodeGraphShared/
└── BehaviorTreeGraphAdapter.h/cpp  (adapter layer)
    ├─ AdaptToTaskGraph()           (BT → Graph)
    ├─ FormatTraceForBehaviorTree() (Post-process trace)
    └─ ValidateTreeStructure()
```

### 6.2 MODIFIED Files

```
Source/BlueprintEditor/
├── BehaviorTreeRenderer.h/cpp
│   └─ Add OnRunGraphClicked() method
│       (calls adapter + simulator framework)
│
└── [OPTIONAL] TaskNodeType enum extension
    └─ Add BT_Selector, BT_Sequence, etc. (if not using metadata)

Source/BlueprintEditor/
└── ExecutionTestPanel.h/cpp
    └─ [OPTIONAL] Support BT-specific formatting in Render()
        (or delegate to adapter)
```

### 6.3 NO New Files Needed For

- ❌ BehaviorTreeExecutionSimulator (use GraphExecutionSimulator)
- ❌ BehaviorTreeExecutionTracer (use GraphExecutionTracer)
- ❌ BTSimulationPanel (use ExecutionTestPanel)

---

## 7. TRACE OUTPUT FORMAT (Post-Processing)

### 7.1 Generic Trace (from GraphExecutionTracer)
```
ExecutionEvent[0]: nodeId=1, name="Selector_1", timestamp=0.0, message="..."
ExecutionEvent[1]: nodeId=2, name="Sequence_2", timestamp=0.1, message="..."
ExecutionEvent[2]: nodeId=3, name="Condition_3", timestamp=0.2, message="..."
```

### 7.2 BT-Formatted Trace (via Adapter Post-Processing)
```cpp
std::string BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree(
    const GraphExecutionTracer& tracer,
    const BehaviorTreeAsset& btAsset)
{
    std::string output = "BehaviorTree Simulation Trace\n";
    output += "============================\n\n";
    
    for (const auto& event : tracer.GetEvents()) {
        // Find corresponding BT node for context
        const BTNode* btNode = btAsset.GetNode(event.nodeId);
        if (!btNode) continue;
        
        // Determine status symbol
        std::string symbol;
        if (event.message.find("SUCCESS") != std::string::npos)
            symbol = "✓ ";
        else if (event.message.find("FAILURE") != std::string::npos)
            symbol = "✗ ";
        else
            symbol = "⊙ ";
        
        // Format with indentation
        int depth = CalculateNodeDepth(btNode->id, btAsset);
        std::string indent(depth * 2, ' ');
        
        // Get BT-specific type name
        std::string typeName = GetBTNodeTypeName(btNode->type);
        
        // Build line
        output += indent + symbol + typeName + ": " + btNode->name 
                + " → " + event.message + "\n";
    }
    
    return output;
}
```

### 7.3 Example Output
```
BehaviorTree Simulation Trace
============================

✓ Selector: MainBehavior → SUCCESS
├─ ✗ Sequence: CombatSequence → FAILURE (condition failed)
│  ├─ ✓ Condition: TargetVisible → SUCCESS
│  ├─ ✗ Condition: TargetInRange → FAILURE
│  └─ [SKIPPED] Action: Attack
│
└─ ✓ Sequence: PatrolSequence → SUCCESS
   ├─ ✓ Condition: HasMoveGoal → SUCCESS
   ├─ ✓ Action: MoveToPatrolPoint → SUCCESS
   └─ ✓ Action: PatrolPickNext → SUCCESS
```

---

## 8. INTEGRATION POINTS

### 8.1 BehaviorTreeRenderer Changes

```cpp
// File: Source/BlueprintEditor/BehaviorTreeRenderer.h
class BehaviorTreeRenderer : public IGraphRenderer {
private:
    std::unique_ptr<ExecutionTestPanel> m_executionTestPanel;  // REUSED
    BehaviorTreeExecutionTracer m_lastTracer;                   // REUSED
    
public:
    void OnRunGraphClicked();  // NEW: trigger simulation
};

// File: Source/BlueprintEditor/BehaviorTreeRenderer.cpp
void BehaviorTreeRenderer::OnRunGraphClicked() {
    if (!m_currentBT) {
        LogError("No BehaviorTree loaded");
        return;
    }
    
    // Step 1: Adapt BT to graph format
    auto taskGraph = BehaviorTreeGraphAdapter::AdaptToTaskGraph(*m_currentBT);
    if (!taskGraph) {
        LogError("Failed to adapt BehaviorTree");
        return;
    }
    
    // Step 2: Use GENERIC simulator
    GraphExecutionSimulator simulator;
    GraphExecutionTracer tracer;
    SimulationOptions options;
    options.maxStepsPerFrame = 1000;
    options.validateConditions = true;
    
    // Step 3: Simulate (framework-agnostic)
    auto errors = simulator.SimulateExecution(*taskGraph, options, tracer);
    
    // Step 4: Format for BT context
    std::string btTrace = BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree(
        tracer, *m_currentBT);
    
    // Step 5: Display via EXISTING panel
    m_executionTestPanel->DisplaySimulationResults(
        tracer,
        m_currentBT->GetFileName(),
        0.0f  // execution time (no real code executed)
    );
    
    // Store trace for later reference
    m_lastTracer = tracer;
}
```

### 8.2 Toolbar Integration

```cpp
// In BehaviorTreeRenderer::RenderLayoutWithTabs()
ImGui::SetCursorPosX(10);
if (ImGui::Button("Run Graph", ImVec2(100, 0))) {
    OnRunGraphClicked();  // Trigger simulation via adapter
}

ImGui::SameLine();
if (ImGui::Button("Clear Results", ImVec2(100, 0))) {
    m_executionTestPanel->ClearResults();
}
```

---

## 9. REVISED IMPLEMENTATION PLAN

### Phase 1: Adapter Layer (Core)
```
Files: BehaviorTreeGraphAdapter.h/cpp
Effort: 4-5 hours
Tasks:
- Implement AdaptToTaskGraph()
- Implement FormatTraceForBehaviorTree()
- Implement ValidateTreeStructure()
- Unit tests for conversion accuracy
```

### Phase 2: BehaviorTree Renderer Integration
```
Files: BehaviorTreeRenderer.h/cpp
Effort: 1-2 hours
Tasks:
- Add OnRunGraphClicked() method
- Integrate toolbar button
- Connect to ExecutionTestPanel
- Error handling & logging
```

### Phase 3: [OPTIONAL] TaskNodeType Extension
```
Files: TaskNodeType enum (if needed)
Effort: 0.5 hours
Tasks:
- Add BT_Selector, BT_Sequence, etc.
- OR use metadata approach (simpler)
```

### Phase 4: Testing & Refinement
```
Effort: 2-3 hours
Tasks:
- Test with Patrol.bt.json
- Test with Combat.bt.json
- Verify trace output formatting
- Performance profiling
```

**Total Effort: ~8-10 hours** (vs 14h for duplicated approach)

---

## 10. ADVANTAGES OF FRAMEWORK-BASED APPROACH

### 10.1 Code Reduction
| Item | Duplicated | Framework | Savings |
|------|-----------|-----------|---------|
| Simulator | 400 LOC | 0 | 400 LOC |
| Tracer | 200 LOC | 0 | 200 LOC |
| Panel | 300 LOC | 0 | 300 LOC |
| **Total** | **900 LOC** | **200 LOC** | **700 LOC** |

### 10.2 Maintainability
- Bug fix in GraphExecutionSimulator → fixes both VS and BT simulation
- Performance improvement in tracer → benefits both systems
- UI enhancement in ExecutionTestPanel → applies everywhere

### 10.3 Consistency
- Same trace format for VS and BT
- Same UI/UX for both simulation systems
- Same execution semantics (except domain-specific details)

### 10.4 Extensibility
- Future graph types (HFSM, Dialogue Trees, etc.) can reuse framework
- Framework grows stronger with each new adapter
- Diminishing returns on new system implementation

---

## 11. NEXT STEPS

1. ✅ **Approved** : Framework-based adapter approach
2. **Implement** : BehaviorTreeGraphAdapter (conversion logic)
3. **Integrate** : Hook "Run Graph" button to adapter + framework
4. **Test** : Verify with known tree structures
5. **Document** : Update copilot-instructions.md with new phase

---

**Status**: ✅ Revised analysis ready for implementation  
**Pattern**: ✅ Adapter + Generic Framework (DRY principle)  
**Effort**: ✅ Reduced from 14h to ~8-10h  
**Impact**: ✅ Sustainable, maintainable, extensible  

---
