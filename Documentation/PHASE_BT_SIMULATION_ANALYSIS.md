# BehaviorTree Graph Simulation - Analyse Complète et Plan d'Implémentation

**Date** : 2026-03-24  
**Phase** : BT-SIM-001 (BehaviorTree Graph Simulation)  
**Statut** : Analyse complétée, Prêt pour implémentation

---

## 1. ANALYSE DE L'ARCHITECTURE ACTUELLE (VisualScript)

### 1.1 Composants du Système VisualScript "Run Graph"

#### **Button & Entry Point** 
- **Fichier** : `VisualScriptEditorPanel_RenderingCore.cpp`
- **Code** : Bouton "Run Graph" dans la toolbar
- **Comportement** : Déclenche la simulation du graphe complet
- **Raccourci** : `Ctrl+Shift+R` (implicite)

#### **Core Classes**
1. **GraphExecutionSimulator** (`GraphExecutionSimulator.h/cpp`)
   - Simule l'exécution d'un graphe sans effets runtime
   - Trace chaque étape d'exécution
   - Détecte erreurs logiques et chemins non atteignables
   - Options : `maxStepsPerFrame`, `maxSubGraphDepth`, `validateConditions`, `validateDataFlow`
   
2. **GraphExecutionTracer** (`GraphExecutionTracer.h/cpp`)
   - Enregistre les événements d'exécution
   - Structure : `ExecutionEvent` (nodeId, timestamp, status, message)
   - Gère l'indentation pour traçage hiérarchique
   
3. **ExecutionTestPanel** (`ExecutionTestPanel.h/cpp`)
   - UI ImGui pour afficher résultats
   - Tabs : Trace Log, Errors, Summary
   - Auto-scroll, filtrage par état

#### **Simulation Flow**
```
User clicks "Run Graph"
  ↓
VisualScriptEditorPanel::RunGraphSimulation()
  ↓
GraphExecutionSimulator::SimulateExecution()
  ├─ Validation (branches, data flow)
  ├─ Traversal (DFS/BFS de tous les nœuds)
  ├─ Condition évaluation
  └─ Trace enregistrement
  ↓
GraphExecutionTracer stocke événements
  ↓
ExecutionTestPanel::Render() affiche résultats
  └─ Code couleur : SUCCESS (vert), FAILURE (rouge), RUNNING (orange)
```

#### **Output Panel (Verification Outputs)**
- **Localisation** : `VisualScriptEditorPanel_Verification.cpp`
- **Contenu** :
  - État global (Errors, Warnings, OK)
  - Liste des issues triées (Errors → Warnings → Info)
  - Navigation "Go" pour sauter au nœud problématique
  - Indentation pour hiérarchie
  
#### **Trace & Messages**
- **Format de trace** : `[Indentation] [TYPE] NodeName (Details)`
- **Types** : `[GetBBValue]`, `[MathOp]`, `[SetBBValue]`, `[Condition]`, `[Action]`
- **Profondeur** : Indentation proportionnelle à la profondeur récursive
- **Coloration** : Via ImGui::TextColored (RGB per status)

---

## 2. ARCHITECTURE BehaviorTree (CIBLE)

### 2.1 Différences Fondamentales BT ↔ VS

| Aspect | VisualScript | BehaviorTree |
|--------|--------------|--------------|
| **Nœuds** | Task (logique séquentielle) | Composite (Selector/Sequence) + Leaf (Condition/Action) |
| **Flow** | Exécution linéaire | Recursive descent parsing |
| **Status** | Pas explicite | **BTStatus : Idle → Running → Success/Failure** |
| **Conditions** | Évaluation données | Checkpoints décisionnels |
| **Actions** | Tâches métier | Modificateurs d'état IA |
| **Trace** | Pas de succès/échec explicite | **MUST TRACK : Success vs Failure path** |
| **Simulation** | Stateless (idéal) | **Nécessite blackboard simulé** |

### 2.2 Nœuds BT et Résultats Attendus

**Selector (OR)**
```
┌─ Teste chaque enfant séquentiellement
├─ SUCCESS si UN enfant réussit → trace "✓ Selector <nom> → SUCCESS"
├─ FAILURE si TOUS échouent → trace "✗ Selector <nom> → FAILURE"
└─ RUNNING si enfant en cours
```

**Sequence (AND)**
```
┌─ Teste chaque enfant séquentiellement
├─ SUCCESS si TOUS réussissent → trace "✓ Sequence <nom> → SUCCESS"
├─ FAILURE si UN échoue → trace "✗ Sequence <nom> → FAILURE (child X failed)"
└─ RUNNING si enfant en cours
```

**Condition (Leaf)**
```
┌─ Évalue condition logique contre blackboard simulé
├─ SUCCESS si vraie → trace "✓ Condition <nom> <type> → SUCCESS"
└─ FAILURE si fausse → trace "✗ Condition <nom> <type> → FAILURE (reason)"
```

**Action (Leaf)**
```
┌─ N'exécute PAS l'action réelle (simulation)
├─ SUCCESS si logique validation OK → trace "✓ Action <nom> <type> → SUCCESS"
└─ FAILURE si erreur validation → trace "✗ Action <nom> <type> → FAILURE (reason)"
```

**Inverter (Decorator)**
```
┌─ Inverse résultat enfant
├─ Child SUCCESS → FAILURE → trace "✓ Inverter → FAILURE (inverted from SUCCESS)"
└─ Child FAILURE → SUCCESS → trace "✓ Inverter → SUCCESS (inverted from FAILURE)"
```

**Repeater (Decorator)**
```
┌─ Répète enfant N fois
├─ Chaque itération tracée : "✓ Repeater [1/3] → SUCCESS"
└─ Agrégé en résultat final
```

### 2.3 Blackboard Simulation

**Nécessaire pour** :
- Évaluation conditions (target visible, health below, etc.)
- Stockage paramètres d'action
- Initial state pour simulation

**Strategy** :
```cpp
struct SimulatedBlackboard {
    // Conditions courantes
    bool targetVisible = true;          // Default: assume visible
    float distanceToTarget = 10.0f;     // Default: medium range
    float healthPercent = 0.75f;        // Default: 75% health
    bool hasTarget = true;              // Default: assume has target
    bool canAttack = true;              // Default: assume can attack
    int AIMode = 1;                     // Default: active mode
    
    // Wander behavior
    bool hasWanderDestination = true;
    float wanderWaitTimer = 0.0f;
    float wanderTargetWaitTime = 5.0f;
};
```

**User Controls** (future UI) :
- Sliders pour distanceToTarget, healthPercent
- Toggles pour targetVisible, hasTarget, canAttack
- Custom values pour AIMode

---

## 3. ARCHITECTURE D'IMPLÉMENTATION PROPOSÉE

### 3.1 Nouveaux Fichiers à Créer

```
Source/BlueprintEditor/
├── BehaviorTreeExecutionSimulator.h/cpp
│   └── Simule exécution BT (équiv. GraphExecutionSimulator)
├── BehaviorTreeExecutionTracer.h/cpp
│   └── Enregistre events (équiv. GraphExecutionTracer)
├── BTSimulationPanel.h/cpp
│   └── UI ImGui pour afficher trace (équiv. ExecutionTestPanel)
└── BTSimulationBlackboard.h/cpp
    └── Blackboard simulé pour tests
```

### 3.2 Classes et Interfaces

#### **BehaviorTreeExecutionTracer**
```cpp
struct BTExecutionEvent {
    uint32_t nodeId;
    std::string nodeName;
    BTNodeType nodeType;
    BTStatus status;              // SUCCESS, FAILURE, RUNNING, IDLE
    int32_t depth;               // Pour indentation
    std::string message;         // Description complète
    float timestamp;             // Temps relatif simulation
    bool isEntering;             // true=enter node, false=exit node
};

class BehaviorTreeExecutionTracer {
public:
    void RecordNodeExecution(uint32_t nodeId, const std::string& nodeName,
                           BTNodeType type, BTStatus status, int depth,
                           const std::string& reason);
    
    void RecordConditionEvaluation(uint32_t nodeId, const std::string& name,
                                  BTConditionType condType, BTStatus result,
                                  int depth, const std::string& details);
    
    void RecordActionExecution(uint32_t nodeId, const std::string& name,
                              BTActionType actType, BTStatus result,
                              int depth);
    
    const std::vector<BTExecutionEvent>& GetEvents() const;
    std::string GetFormattedTrace() const;
};
```

#### **BehaviorTreeExecutionSimulator**
```cpp
struct BTSimulationOptions {
    int32_t maxDepth = 20;
    bool validateStructure = true;
    bool traceDetailLevel = 2;  // 0=minimal, 1=normal, 2=verbose
    std::shared_ptr<AIBlackboard_data> blackboard;  // Mock blackboard
};

class BehaviorTreeExecutionSimulator {
public:
    bool SimulateTreeExecution(const BehaviorTreeAsset& tree,
                               const BTSimulationOptions& options,
                               BehaviorTreeExecutionTracer& outTracer);
    
    BTStatus ExecuteNodeSimulation(uint32_t nodeId, const BehaviorTreeAsset& tree,
                                   AIBlackboard_data& blackboard,
                                   BehaviorTreeExecutionTracer& tracer,
                                   int depth);
    
    BTStatus SimulateCondition(const BTNode& condNode, 
                               AIBlackboard_data& blackboard,
                               BehaviorTreeExecutionTracer& tracer,
                               int depth);
    
    BTStatus SimulateAction(const BTNode& actNode,
                           AIBlackboard_data& blackboard,
                           BehaviorTreeExecutionTracer& tracer,
                           int depth);
};
```

#### **BTSimulationPanel**
```cpp
class BTSimulationPanel {
public:
    void Initialize();
    void Render();
    
    void DisplaySimulationResults(const BehaviorTreeExecutionTracer& tracer,
                                 const std::string& treeFileName,
                                 float executionTime);
    
    void ClearResults();
    
private:
    void RenderToolbar();
    void RenderTraceLog();
    void RenderNodeStatus();
    void RenderBlackboardEditor();
    
    std::vector<BTExecutionEvent> m_lastEvents;
    bool m_autoScroll = true;
    int32_t m_selectedEventIndex = -1;
};
```

### 3.3 Integration Points

#### **BehaviorTreeRenderer Changes**
```cpp
class BehaviorTreeRenderer {
private:
    std::unique_ptr<BehaviorTreeExecutionSimulator> m_simulator;
    std::unique_ptr<BTSimulationPanel> m_simulationPanel;
    BehaviorTreeExecutionTracer m_lastTracer;
    
public:
    void OnRunGraphClicked();  // Callback from toolbar button
    void RenderSimulationPanel();
};
```

#### **Toolbar Integration**
```cpp
// In BehaviorTreeRenderer::RenderLayoutWithTabs()
if (ImGui::Button("Run Graph", ImVec2(100, 0))) {
    OnRunGraphClicked();  // Trigger simulation
}
```

---

## 4. TRACE DISPLAY FORMAT

### 4.1 Exemples de Output

```
========== BehaviorTree Simulation: Patrol.bt.json ==========
Execution Time: 2.34ms
Graph Status: SUCCESS (root completed)

[Tree Execution]
├─ ✓ Selector: MainBehavior → SUCCESS (1st child succeeded)
│  ├─ ✗ Sequence: CombatSequence → FAILURE (condition failed)
│  │  ├─ ✓ Condition: TargetVisible → SUCCESS (target visible = true)
│  │  ├─ ✗ Condition: TargetInRange (10.0) → FAILURE (dist=25.3 > 10.0)
│  │  └─ [SKIPPED - parent failed]
│  │
│  └─ ✓ Sequence: PatrolSequence → SUCCESS (all children passed)
│     ├─ ✓ Condition: HasMoveGoal → SUCCESS
│     ├─ ✓ Action: MoveToGoal → SUCCESS (validation OK)
│     └─ ✓ Action: PatrolPickNextPoint → SUCCESS
│
├─ Inverter: IsHealthLow → SUCCESS (child FAILURE inverted)
│  └─ ✗ Condition: HealthBelow (0.3) → FAILURE (health=0.75 > 0.3)
│
└─ Repeater: WaitAction [1/3] → SUCCESS
   └─ ✓ Action: WaitRandomTime → SUCCESS
```

### 4.2 Node Status Coloring
- **✓ SUCCESS** : Verde (#00FF00)
- **✗ FAILURE** : Rojo (#FF0000)  
- **⊙ RUNNING** : Naranja (#FFA500)
- **○ IDLE** : Gris (#808080)
- **[SKIPPED]** : Gris claro (#B0B0B0)

### 4.3 Canvas Node Highlighting
Après simulation, les nœuds du canvas affichent :
- Bordure SUCCESS → vert clair (pas d'erreur)
- Bordure FAILURE → rouge clair (chemin échoué)
- Bordure RUNNING → orange clair (en cours)

---

## 5. SIMULATION LOGIC (Pseudo-code)

```cpp
BTStatus SimulateExecution(BehaviorTreeAsset tree) {
    AIBlackboard_data blackboard = CreateMockBlackboard();
    
    BTExecutionEvent rootEvent = {
        nodeId: tree.rootNodeId,
        nodeName: "Root",
        status: RUNNING,
        depth: 0,
        message: "Starting simulation"
    };
    tracer.Record(rootEvent);
    
    BTStatus rootStatus = ExecuteNodeSim(tree.rootNodeId, tree, blackboard, 0);
    
    tracer.Record({
        nodeId: tree.rootNodeId,
        nodeName: "Root",
        status: rootStatus,
        depth: 0,
        message: "Simulation complete: " + StatusToString(rootStatus)
    });
    
    return rootStatus;
}

BTStatus ExecuteNodeSim(nodeId, tree, blackboard, depth) {
    BTNode node = tree.GetNode(nodeId);
    
    case node.type:
        Selector:
            for each child in node.children {
                status = ExecuteNodeSim(child.id, tree, blackboard, depth+1);
                tracer.Record(nodeId, "Selector", "Testing child: " + child.name);
                if status == SUCCESS {
                    tracer.Record(nodeId, "Selector", "✓ SUCCESS (child succeeded)");
                    return SUCCESS;
                }
                if status == RUNNING {
                    return RUNNING;
                }
            }
            tracer.Record(nodeId, "Selector", "✗ FAILURE (all children failed)");
            return FAILURE;
        
        Sequence:
            for each child in node.children {
                status = ExecuteNodeSim(child.id, tree, blackboard, depth+1);
                if status == FAILURE {
                    tracer.Record(nodeId, "Sequence", "✗ FAILURE (child failed)");
                    return FAILURE;
                }
                if status == RUNNING {
                    return RUNNING;
                }
            }
            tracer.Record(nodeId, "Sequence", "✓ SUCCESS (all children passed)");
            return SUCCESS;
        
        Condition:
            result = EvaluateCondition(node, blackboard);
            tracer.Record(nodeId, node.name, 
                         "Condition " + node.conditionType + 
                         "→ " + StatusToString(result));
            return result;
        
        Action:
            result = ValidateAction(node);  // No real execution
            tracer.Record(nodeId, node.name,
                         "Action " + node.actionType +
                         "→ " + StatusToString(result));
            return result;
        
        Inverter:
            childStatus = ExecuteNodeSim(node.child, tree, blackboard, depth+1);
            result = (childStatus == SUCCESS) ? FAILURE : SUCCESS;
            tracer.Record(nodeId, node.name,
                         "Inverter: " + StatusToString(childStatus) +
                         "→ " + StatusToString(result));
            return result;
}
```

---

## 6. POINTS CLÉS D'IMPLÉMENTATION

### 6.1 Adaptations Nécessaires vs VisualScript

| Aspect | VisualScript | BehaviorTree Adaptation |
|--------|--------------|------------------------|
| **Entry point** | Single Start node | Root node (can be Selector/Sequence) |
| **Traversal** | DFS + state machine | Recursive descent + status per level |
| **Branching** | Linear tasks | Selector (try/fail), Sequence (all pass) |
| **Conditions** | Data evaluation | Binary success/failure |
| **Actions** | Task execution | Validation only (no side-effects) |
| **Trace depth** | Moderate | High (nested composites) |
| **Status tracking** | Implicit | **Explicit per node & parent** |

### 6.2 Critical Considerations

1. **Blackboard Initialization** 
   - Must provide sensible defaults
   - Allow user override via UI sliders
   - Store state between simulation runs

2. **Decorator Handling** (Inverter, Repeater)
   - Inverter simple : flip SUCCESS ↔ FAILURE
   - Repeater : loop N times, aggregate result
   - Must trace each iteration

3. **Cycle Detection**
   - BT tree is DAG (no cycles theoretically)
   - Add depth limit as safety (maxDepth=20)
   - Warn if limit reached

4. **Performance**
   - No actual entity/component access
   - Pure tree traversal
   - Should be <5ms for typical trees

---

## 7. UI LAYOUT (BTSimulationPanel)

```
┌─────────────────────────────────────────┐
│ [Run Graph]  [Clear]  [Export Log]      │ ← Toolbar
├─────────────────────────────────────────┤
│ Graph Status: SUCCESS (2.34ms)          │ ← Header
├─────────────────────────────────────────┤
│ ├─ ✓ Selector: MainBehavior             │
│ │  ├─ ✗ Sequence: CombatSequence        │
│ │  └─ ✓ Sequence: PatrolSequence        │ ← Tree Trace
│ ├─ Inverter: IsHealthLow                │    (Scrollable)
│ └─ Repeater: Wait [1/3]                 │
├─────────────────────────────────────────┤
│ [Events] [Blackboard] [Details]         │ ← Tabs
├─────────────────────────────────────────┤
│ Target Visible: [true]  ↔ [false]       │
│ Distance to Target: [10.0] ◄──► [50.0]  │ ← Blackboard
│ Health Percent: [0.75] ◄──► [1.0]       │    Editor
│ AI Mode: [1] (dropdown)                 │
└─────────────────────────────────────────┘
```

---

## 8. TIMELINE & EFFORT ESTIMATE

| Phase | Task | Effort | Notes |
|-------|------|--------|-------|
| 1 | Core simulator class | 4h | BehaviorTreeExecutionSimulator |
| 2 | Tracer & events | 2h | BehaviorTreeExecutionTracer |
| 3 | UI panel | 3h | BTSimulationPanel + rendering |
| 4 | Integration | 2h | BehaviorTreeRenderer hookup |
| 5 | Testing & debug | 3h | Various test trees |
| **Total** | | **~14h** | Ready for Phase 35 integration |

---

## 9. NEXT STEPS (Phase Implementation)

1. **Immediate** : Create 4 header files with class signatures
2. **Implementation** : Implement BehaviorTreeExecutionSimulator logic
3. **UI** : Build BTSimulationPanel rendering
4. **Integration** : Wire "Run Graph" button to simulator
5. **Testing** : Simulate known tree structures (Patrol.bt.json, etc.)

---

**Autorisation de procéder à l'implémentation ?** ✅ 

Tous les fichiers et architecture sont prêts. L'implémentation suit le pattern établi par GraphExecutionSimulator avec adaptations BT spécifiques.
