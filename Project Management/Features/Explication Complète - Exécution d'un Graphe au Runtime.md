Explication Complète : Exécution d'un Graphe au Runtime
Basé sur l'analyse de votre architecture Olympe-Engine Phase 24, voici comment fonctionne l'exécution des graphes visuels au runtime :

1) LIAISON GRAPH ↔ ENTITÉ/PRÉFAB
Code
┌─────────────────────────────────────────────────────────────────┐
│                     ENTITÉ / PRÉFAB                              │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  TaskRunnerComponent (attach au démarrage/spawnage)        │  │
│  │  ┌──────────────────────────────────────────────────────┐  │  │
│  │  │  GraphTemplateID    → AssetID du .json chargé        │  │  │
│  │  │  graphAssetPath     → Chemin du fichier .json        │  │  │
│  │  │  CurrentNodeID      → Node en exécution (ID)         │  │  │
│  │  │  LocalBlackboard    → Dict vars locales/graphe      │  │  │
│  │  │  activeTask         → IAtomicTask en cours           │  │  │
│  │  │  LastStatus         → Success/Failure/Running        │  │  │
│  │  └──────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────┘  │
│                                                                    │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  Components associés (MovementComponent, etc.)            │  │
│  │  - PositionComponent (position mondiale)                 │  │
│  │  - MovementComponent (vélocité, MaxSpeed)               │  │
│  └────────────────────��───────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                           ↓ REFERENCE
┌─────────────────────────────────────────────────────────────────┐
│          TaskGraphTemplate (partagé, READ-ONLY)                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │  GraphType = "VisualScript" | "BehaviorTree"             │  │
│  │  LocalVariables[] → schéma des vars locales              │  │
│  │  Nodes[] → toutes les définitions de nodes               │  │
│  │  ExecConnections[] → flux de contrôle                    │  │
│  │  DataConnections[] → flux de données                     │  │
│  │  Presets[] (Phase 24) → conditions embarquées             │  │
│  │  GlobalVariableValues (Phase 24) → overrides globals     │  │
│  └────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
Conditions d'attachement :

Le TaskRunnerComponent est toujours créé pour les entités "AI-controllées"
GraphTemplateID est défini lors du spawn (à partir du préfab)
graphAssetPath stocke le chemin pour fallback async
Une entité = UN TaskRunnerComponent = UN graphe en exécution
UN TaskGraphTemplate = partagé par N entités (optimisation mémoire)
2) SYSTÈMES IMPLIQUÉS & DÉPENDANCES
Code
┌────────────────────────────────────────────────────────────────────────┐
│                   MAIN EXECUTION LOOP (Chaque frame)                   │
│                                                                         │
│  World::Update()                                                       │
│    ↓                                                                   │
│  TaskSystem::Process()  [ECS_System]                                  │
│    FOR EACH entity WITH TaskRunnerComponent:                          │
│      ├─ (1) AssetManager::Get(GraphTemplateID)                        │
│      │   → Récupère TaskGraphTemplate du cache/disque                 │
│      │                                                                 │
│      ├─ (2) IF template->GraphType == "VisualScript":                │
│      │       ExecuteVSFrame()                                         │
│      │       ↓                                                         │
│      │   ┌─────────────────────────────────────────────┐              │
│      │   │ VSGraphExecutor::ExecuteFrame()             │              │
│      │   │ ┌───────────────────────────────────────┐  │              │
│      │   │ │ TANT QUE CurrentNodeID != NONE:     │  │              │
│      │   │ │   (1) ResolveDataPins()              │  │              │
│      │   │ │   (2) GetNode(CurrentNodeID)         │  │              │
│      │   │ │   (3) Dispatch par type (Handle*)    │  │              │
│      │   │ │   (4) Update CurrentNodeID           │  │              │
│      │   │ └───────────────────────────────────────┘  │              │
│      │   └─────────────────────────────────────────────┘              │
│      │                                                                 │
│      ├─ (3) LocalBlackboard est LU/ÉCRIT par le graphe                │
│      └─ (4) Components (Position, Movement, etc.) sont MAJ par       │
│            les tâches atomiques                                       │
└────────────────────────────────────────────────────────────────────────┘
3) ARCHITECTURE DÉTAILLÉE DU SYSTÈME
A) Composantes principales :
C++
// ========= ENTITÉ/RUNTIME =========
struct TaskRunnerComponent {
    AssetID GraphTemplateID;                    // → reference
    std::string graphAssetPath;                 // → fallback
    int32_t CurrentNodeID;                      // → state machine
    std::unordered_map LocalBlackboard;         // → vars du graphe
    std::unique_ptr<IAtomicTask> activeTask;    // → task en cours
    TaskStatus LastStatus;                      // → résultat
    std::unordered_map DataPinCache;            // → cache éval
    std::unordered_map DoOnceFlags;             // → pour DoOnce nodes
};

// ========= TEMPLATE (shared) =========
class TaskGraphTemplate {
    std::vector<VariableDefinition> LocalVariables;  // schéma
    std::vector<TaskNodeDefinition> Nodes;           // définitions
    std::vector<ExecPinConnection> ExecConnections;  // flux contrôle
    std::vector<DataPinConnection> DataConnections;  // flux données
    std::vector<ConditionPreset> Presets;            // Phase 24
    json GlobalVariableValues;                       // Phase 24
};

// ========= NODE DEFINITION =========
struct TaskNodeDefinition {
    int32_t NodeID;                              // identifiant unique
    TaskNodeType Type;                           // EntryPoint, Branch, etc.
    std::vector<TaskNodeDefinition> ChildrenIDs;
    
    // Pour AtomicTask:
    std::string AtomicTaskID;
    std::unordered_map<std::string, ParameterBinding> Parameters;
    
    // Pour VS (Branch, Switch, etc.):
    std::vector<Condition> conditions;           // Phase 23
    std::vector<NodeConditionRef> conditionRefs; // Phase 24
    std::vector<DataPinDefinition> DataPins;
    
    // Pour GetBBValue/SetBBValue:
    std::string BBKey;  // "local:varname" ou "global:key"
    
    // Pour Delay:
    float DelaySeconds;
};
B) Flux de contrôle (Control Flow)
Code
┌─────────────────────────────────────────────────────────────────┐
│                  NODE TYPE HANDLERS                              │
│                                                                  │
│  Phase 1: EntryPoint                                            │
│    └─ retourne l'ID du 1er successeur exec                     │
│                                                                  │
│  Phase 2: Branch (If/Else)                                      │
│    ├─ (1) EvalCondition (Phase 24 presets / Phase 23 / datapins)│
│    ├─ (2) FindExecTarget("Then" ou "Else")                      │
│    └─ retourne NextNodeID                                       │
│                                                                  │
│  Phase 3: VSSequence                                            │
│    ├─ Exécute ChildrenIDs[SequenceChildIndex]                   │
│    ├─ Si enfant = Completed → SequenceChildIndex++              │
│    └─ Quand tous: retourne suivant                              │
│                                                                  │
│  Phase 4: AtomicTask                                            │
│    ├─ Crée IAtomicTask si 1ère fois                             │
│    ├─ Appelle task->ExecuteWithContext(ctx, params)             │
│    ├─ Si Running → retourne NODE_INDEX_NONE (garde state)      │
│    ├─ Si Success/Failure → retourne NextOnSuccess/Failure       │
│    └─ Destroy task                                              │
│                                                                  │
│  Phase 5: GetBBValue / SetBBValue                               │
│    ├─ (1) ResolveDataPins() → charge inputs                     │
│    ├─ (2) Read/Write LocalBlackboard                            │
│    └─ retourne suivant (quasi immédiat)                         │
│                                                                  │
│  Phase 6: Delay                                                 │
│    ├─ StateTimer += dt                                          │
│    ├─ Si StateTimer >= DelaySeconds → Completed                 │
│    └─ Sinon → NODE_INDEX_NONE (stay in Delay)                   │
│                                                                  │
│  Phase 7: While                                                 │
│    ├─ EvalCondition()                                           │
│    ├─ Si true → FindExecTarget("Loop")                          │
│    └─ Si false → FindExecTarget("Completed")                    │
│                                                                  │
│  Phase 8: Switch                                                │
│    ├─ Évalue switchVariable depuis BB                           │
│    ├─ Match avec switchCases[]                                  │
│    └─ Retourne correspondant                                    │
│                                                                  │
│  Phase 9: MathOp (data-pure)                                    │
│    ├─ ResolveDataPins() → charges opérandes                     │
│    ├─ Calcul: left operator right                               │
│    └─ Stocke dans DataPinCache (data-out)                       │
│                                                                  │
│  Phase 10: SubGraph (récursion)                                 │
│    ├─ Charge sub-template depuis fichier                        │
│    ├─ Initialise LocalBlackboard du sub                         │
│    ├─ VSGraphExecutor::ExecuteFrame() récursif                  │
│    └─ Récupère OutputParams vers BB parent                      │
└─────────────────────────────────────────────────────────────────┘
4) INTERACTION AVEC BLACKBOARD & COMPONENTS
Code
┌────────────────────────────────────────────────────────────────┐
│        GRAPHE READING/WRITING ENTITY STATE                      │
│                                                                 │
│  LocalBlackboard (dans TaskRunnerComponent)                     │
│    = copy du schéma de template->LocalVariables                │
│    = vars PRIVÉES au graphe + entité                           │
│                                                                 │
│    GetBBValue("local:health")  →  lit LocalBlackboard          │
│    SetBBValue("local:health", 50)  →  écrit LocalBlackboard    │
│                                                                 │
│  Accès aux Components de l'entité:                             │
│    ├─ IAtomicTask reçoit AtomicTaskContext:                    │
│    │   ├─ EntityID entity                                      │
│    │   ├─ LocalBlackboard* bb                                  │
│    │   ├─ World* world                                         │
│    │   └─ Parameters (nom=valeur depuis template)              │
│    │                                                            │
│    ├─ Depuis World, la task accède :                           │
│    │   ├─ world->GetComponent<MovementComponent>(entity)       │
│    │   ├─ world->GetComponent<PositionComponent>(entity)       │
│    │   ├─ world->GetComponent<StatusComponent>(entity)         │
│    │   └─ Peut modifier directement ces données                │
│    │                                                            │
│    ├─ Exemple (Task_MoveToLocation):                           │
│    │   movementComp->Velocity = direction * speed;             │
│    │   return (distance < threshold) ? Success : Running;      │
│    │                                                            │
│    └─ LocalBlackboard peut servir d'intermédiaire:             │
│        SetBBValue("target_x", targetX);  // depuis task A      │
│        target_x = GetBBValue("target_x");  // lu par task B    │
│                                                                 │
│  Global Blackboard (EntityBlackboard - Phase 24):              │
│    = données partagées entre TOUTES entités                    │
│    GetBBValue("global:turn_count")                             │
│    (actuellement lecture; écriture future)                     │
└────────────────────────────────────────────────────────────────┘
5) FLOT DE DONNÉES (Data Flow)
Code
DATA PIN RESOLUTION (réursif, depth-first)

Avant d'exécuter un node, VSGraphExecutor::ResolveDataPins():

┌─ Node: SetBBValue
│  Input pin "Value" doit être connecté
│  FindDataConnection(SetBBValue.NodeID, "Value")  →  source
│  
├─ Source = MathOp node
│  │ Input pins: LeftOperand, RightOperand
│  │ 
│  ├─ Résoudre LeftOperand
│  │  FindDataConnection(MathOp.NodeID, "LeftOperand")  →  GetBBValue
│  │  Résoudre GetBBValue("local:health")  →  lit BB  → TaskValue(95)
│  │  Cache[MathOp:LeftOperand] = TaskValue(95)
│  │
│  └─ Résoudre RightOperand
│     FindDataConnection(MathOp.NodeID, "RightOperand")  →  GetBBValue
│     Résoudre GetBBValue("local:bonus")  →  lit BB  → TaskValue(10)
│     Cache[MathOp:RightOperand] = TaskValue(10)
│
├─ MathOp: health + bonus = 95 + 10 = 105
│  Cache[MathOp:Result] = TaskValue(105)
│
└─ SetBBValue lit cache[MathOp:Result]
   Écrit LocalBlackboard["local:final_damage"] = TaskValue(105)
6) DIAGRAMME DE DÉPENDANCE COMPLÈTE
Code
┌──────────────────────────────────────────────────────────────────────┐
│                        RUNTIME DEPENDENCY GRAPH                       │
│                                                                       │
│  ┌─ World ───────────────────────────────────────────────────────┐  │
│  │                                                                 │  │
│  ├─ (1) ECS_System[]                                              │  │
│  │  ├─ TaskSystem (our system)                                    │  │
│  │  ├─ MovementSystem (integrates MovementComponent velocity)     │  │
│  │  ├─ RenderSystem                                               │  │
│  │  └─ ...                                                        │  │
│  │                                                                 │  │
│  ├─ (2) Entities []                                               │  │
│  │  └─ Entity (ID) ─────────────┬─────────────────────────────┐  │  │
│  │                             │                             │  │  │
│  │                    TaskRunnerComponent              MovementComponent
│  │                    ├─ GraphTemplateID ──────────┐   ├─ Velocity
│  │                    ├─ CurrentNodeID              │   └─ MaxSpeed
│  │                    ├─ LocalBlackboard            │
│  │                    ├─ activeTask ────────┐       │
│  │                    ├─ LastStatus         │       │
│  │                    └─ DataPinCache       │       │
│  │                                          │       │
│  │    ┌──────────────────────────────────────┘       │
│  │    │                                              │
│  │    ├─ Resolved TaskGraphTemplate (cached)         │
│  │    │  ├─ LocalVariables[]                         │
│  │    │  ├─ Nodes[] ─────────────────────────────┐   │
│  │    │  ├─ ExecConnections[]  ──────────┐       │   │
│  │    │  └─ DataConnections[]   ───┐     │       │   │
│  │    │                             │     │       │   │
│  │    └─ IAtomicTask (factory)      │     │       │   │
│  │       ├─ AtomicTaskRegistry      │     │       │   │
│  │       └─ context: {entity, bb}   │     │       │   │
│  │                                  │     │       │   │
│  │    ┌──────────────────────────────┘     │       │   │
│  │    │                                    │       │   │
│  │    ├─ Condition Evaluator (Phase 23-24) │       │   │
│  │    │  ├─ conditions[]: Phase 23          │       │   │
│  │    │  ├─ conditionRefs[]: Phase 24       │       │   │
│  │    │  ├─ ConditionPresetRegistry         │       │   │
│  │    │  └─ ConditionPresetEvaluator        │       │   │
│  │    │                                    │       │   │
│  │    ├─ DataPinEvaluator                   │       │   │
│  │    │  ├─ Recursive resolution            │       │   │
│  │    │  ├─ Cycle detection                │       │   │
│  │    │  └─ DataPinCache update            │       │   │
│  │    │                                    │       │   │
│  │    └─ TaskNodeDefinition ─────────────────┘       │   │
│  │       ├─ NodeID, Type                            │   │
│  │       ├─ Parameters[]                            │   │
│  │       ├─ ChildrenIDs[]                           │   │
│  │       └─ Specific data (BBKey, Condition, etc.)  │   │
│  │                                                 │   │
│  │    ┌───────────────────────────────────────────────┘   │
│  │    │                                                   │
│  │    └─ VSGraphExecutor::HandleAtomicTask               │
│  │       ├─ Resolves parameters from LocalBlackboard    │
│  │       ├─ Creates IAtomicTask instance               │
│  │       ├─ Calls ExecuteWithContext(ctx)               │
│  │       ├─ Task modifies MovementComponent             │
│  │       └─ MovementSystem integrates next frame        │
│  │                                                       │
│  │  PositionComponent                                   │
│  │  └─ Position (updated by MovementSystem)             │
│  │                                                       │
│  └────────────────────────────────────────────────────────┘
│
│  (3) AssetManager (singleton)
│     └─ Cache<TaskGraphTemplate> (loaded from .json files)
│
│  (4) EntityBlackboard (global, Phase 24)
│     └─ Global variables shared across all entities
│
└──────────────────────────────────────────────────────────────────────┘
7) MISE À JOUR EN CAS D'ÉVOLUTION
A) Si vous modifiez les NODES/GRAPHES :
C++
// ============ LORS DE MODIFICATION DE GRAPHE ============

// 1. Phase d'édition (dans l'éditeur)
//    VisualScriptEditorPanel::SerializeAndWrite()
//    └─ Sauvegarde le JSON v4 avec:
//       - ExecConnections[] mises à jour
//       - DataConnections[] mises à jour
//       - Nodes[] avec nouvelles définitions
//       - Presets[] (Phase 24)

// 2. Phase de déploiement/reload
//    TaskGraphLoader::ParseSchemaV4()
//    └─ Parse le nouveau JSON
//       └─ Crée nouveau TaskGraphTemplate
//          └─ BuildLookupCache() (O(1) node lookups)
//          └─ Validate() (structural checks)

// 3. Éditeur runtime: le graphe en cours se RÉINITIALISE
//    AssetManager::Reload(graphAssetPath)
//    └─ Les entities s'arrêtent (CurrentNodeID = NONE)
//    └─ LocalBlackboard conserve MAIS restart du graphe

// ⚠️ IMPACT:
//    - Tasks Running sont Abort()ed
//    - État perdu (mais persiste si sauvegardé en composant)
//    - Prochaine frame: nouveau template utilisé
B) Si vous modifiez les BLACKBOARDS :
C++
// ============ LORS DE MODIFICATION DE SCHÉMA BB ============

// 1. TaskGraphTemplate::LocalVariables (schéma)
//    └─ Changez les VariableDefinition[]
//       ├─ Type change:   TaskValue doit être recasté
//       ├─ Nouveau:       DefaultValue appliqué au reset
//       └─ Suppression:   BB perd la variable (données orphelines)

// 2. LocalBlackboard::Reset() (appelé au spawn)
//    └─ Initialise tous les LocalVariables[].DefaultValue
//       └─ Chaque entité récupère sa copie

// 3. EntityBlackboard (Phase 24, global)
//    └─ RegisterVariable(key, type)
//       └─ Persiste à travers tous graphes
//       └─ Accès via "global:key"

// ⚠️ IMPACT:
//    - Changement type: risque exception si data incompatible
//    - Suppression: perte silencieuse si pas handled
//    - Solution: Versioning + Migration (TaskGraphMigrator_v3_to_v4)
C) Si vous ajoutez/modifiez les COMPONENTS :
C++
// ============ LORS D'AJOUT COMPONENT =========
//
// Exemple: Vous ajoutez HealthComponent au système
//
// 1. Définir la structure (Source/ECS/Components/HealthComponent.h)
// 2. TaskRunnerComponent persiste (c'est par entité)
// 3. Dans IAtomicTask, vous accédez maintenant:
//
//    context.world->GetComponent<HealthComponent>(entity)
//    └─ MAIS: vous devez vérifier HasComponent() d'abord
//       (sinon nullptr → crash)
//
// 4. Nodes qui manipulent HealthComponent:
//    └─ SetBBValue("local:current_health", health->Current)
//    └─ GetBBValue → GetBBValue("local:current_health")
//       └─ Déclenche Damage task si < threshold
//
// ⚠️ IMPACT:
//    - Backward compatible IF vous gardez getComponent checks
//    - Tasks doivent vérifier component existence
//    - Presets/Conditions peuvent référencer Component fields
//    - Phase 24: Condition presets doivent être revalidés
D) Si vous modifiez les CONDITIONS :
C++
// ============ LORS DE MODIFICATION CONDITIONS ============
//
// Phase 23: conditions[] (in-graph)
//    └─ TaskGraphTemplate::Nodes[i].conditions[]
//       ├─ Modif in-editor → JSON rewrite
//       ├─ Nouvelle condition type (if missing handler)
//       └─ VSGraphExecutor::HandleBranch() évalue

// Phase 24: Condition Presets (embedded)
//    └─ TaskGraphTemplate::Presets[]
//       ├─ ConditionPreset Bank dans le JSON
//       ├─ NodeConditionRef relie node aux presets
//       ├─ ConditionPresetEvaluator::Evaluate() résout
//       └─ Fallback automatique si preset missing

// ⚠️ IMPACT:
//    - Presets manquants: log warning + fallback
//    - Syntax error: exception (handled gracefully)
//    - Performance: mêmes éval que Phase 23
//    - Nécessite recompile du graph JSON
8) RÉCAPITULATIF : QUESTIONS CLÉ
Q1: Comment une entité reçoit instructions du graphe?
Code
Entity spawn() → TaskRunnerComponent attaché → GraphTemplateID défini
                                                          ↓
TaskSystem::Process()  frame N → ExecuteVSFrame() → VSGraphExecutor
                                                          ↓
Lecture TaskGraphTemplate (partagé) → Éval currentNodeID
                                          ↓
Si AtomicTask → Crée IAtomicTask → task->ExecuteWithContext()
                                   └─ Modifie MovementComponent
                                      ↓
MovementSystem::Process()  frame N+1 → Intègre Velocity → Position MAJ
Q2: Comment persiste l'état pendant exécution?
Code
TaskRunnerComponent.CurrentNodeID  → état contrôle-flow
TaskRunnerComponent.LocalBlackboard → état données
TaskRunnerComponent.activeTask     → tâche en cours (Running)
TaskRunnerComponent.StateTimer     → temps dans node
TaskRunnerComponent.DoOnceFlags    → flags exécution

Si entité détruite → tout perdu (pas de persistence)
Si graphe reloadé → CurrentNodeID reset (graphe restart)
Si task Running → persiste jusqu'à completion
Q3: Comment les systèmes intéragissent?
Code
TaskSystem (orchestration graphe)
    ↑ Lit/écrit TaskRunnerComponent
    ↓
VSGraphExecutor (logique nodale)
    ↑ Résout DataPins, Conditions
    ↓
IAtomicTask (implémentation métier)
    ↑ Accède World, Components
    ↓
MovementSystem/RenderSystem/... (autres systèmes ECS)
    ↑ Consomment données components modifiées par tasks
CONCLUSION
La Phase 24 a centralisé l'architecture autour de :

TaskRunnerComponent = State Machine par entité
TaskGraphTemplate = Immutable blueprint partagé
VSGraphExecutor = Engine d'exécution frame-by-frame
LocalBlackboard = Vars privées/graphe
Condition Presets = Conditions embarquées en JSON
DataPinEvaluator = Résolution automatique flux données
Les graphes peuvent évoluer (JSON rewrite), les presets peuvent changer (fallback automatique), les components peuvent s'ajouter (getComponent checks), et tout reste synchronisé via TaskRunnerComponent = le pivot de l'architecture.