🗺️ GRAPHE COMPLET DES DÉPENDANCES (AVEC DÉCOUVERTES)
┌─────────────────────────────────────────────────────────────────────────┐
│                      EXTERNAL SINGLETONS                                 │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌──────────────────────┐  ┌──────────────────────┐                     │
│  │ EventQueue           │  │ GameEngine / World   │                     │
│  │ (event dispatcher)   │  │ (main loop, time)    │                     │
│  └────────────┬─────────┘  └──────────────┬───────┘                     │
│               │                           │                             │
│               └────────────────┬──────────┘                              │
│                                │                                         │
│  ┌──────────────────────────────▼────────────────────────────────────┐  │
│  │ GlobalBlackboard (UNIFIED)                                       │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │ - Singleton storage of game-wide state                           │  │
│  │ - Examples:                                                      │  │
│  │   * "gamePhase": Combat | Exploration | Cinematic               │  │
│  │   * "playerHealth": Float                                        │  │
│  │   * "levelTime": Float                                           │  │
│  │   * "cameraTarget": EntityID                                     │  │
│  │   * "eventFlags": String (quest state, doors, etc.)             │  │
│  │ - Accessed via scope "global:key"                                │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                          │
│  ┌──────────────────────┐  ┌──────────────────────┐                     │
│  │ AssetManager         │  │ BehaviorTreeManager  │                     │
│  │ (loads .ats graphs)  │  │ (legacy BT trees)    │                     │
│  └────────────┬─────────┘  └──────────────┬───────┘                     │
│               │                           │                             │
│               └────────────────┬──────────┘ (DEPRECATE!)                 │
│                                │                                         │
│               ┌────────────────┘                                         │
│               │                                                          │
│               ▼                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ TaskGraphTemplate (cached schema)                                │  │
│  ├──────────────────────────────────────────────────────────────────┤  │
│  │ - Immutable, shared by all entities using same graph             │  │
│  │ - Schema:                                                        │  │
│  │   * GraphType: "VisualScript" (or legacy "BehaviorTree")        │  │
│  │   * Blackboard: [{ key, type, default }, ...]                   │  │
│  │   * Nodes: EntryPoint, Branch, AtomicTask, GetBBValue, etc.     │  │
│  │   * ExecConnections: flow control                                │  │
│  │   * DataConnections: data flow                                   │  │
│  │   * EntryPointID: starting node                                  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
         │
         │ (Initialize from)
         │
┌────────▼──────────────────────────────────────────────────────────────┐
│                    ENTITY LAYER (PER-ENTITY)                           │
├────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐ │
│  │ TaskRunnerComponent (Persistent - NOT recreated each frame)      │ │
│  ├──────────────────────────────────────────────────────────────────┤ │
│  │ - CurrentNodeID: int32_t (execution state)                       │ │
│  │ - GraphTemplateID: AssetID (→ TaskGraphTemplate lookup)          │ │
│  │ - LocalBlackboard: LocalBlackboard* (THE BACKBONE!)              │ │
│  │   * Holds all entity variable state                              │ │
│  │   * Includes AI state, perception, intents, etc.                 │ │
│  │ - DataPinCache: map<string, TaskValue> (node output buffer)      │ │
│  │ - activeTask: IAtomicTask* (current running task, if any)        │ │
│  └──────────────────────────────────────────────────────────────────┘ │
│               │ (stores)                                               │
│               ▼                                                        │
│  ┌──────────────────────────────────────────────────────────────────┐ │
│  │ LocalBlackboard* (UNIFIED ENTITY STATE)                          │ │
│  ├──────────────────────────────────────────────────────────────────┤ │
│  │ - Instance created at entity spawn                               │ │
│  │ - Persists for lifetime of entity                                │ │
│  │ - Contents:                                                      │ │
│  │   LOCAL VARIABLES (from TaskGraphTemplate.Blackboard):           │ │
│  │   * "aiMode": Enum (Idle, Combat, etc.)       [WAS AIBlackboard] │ │
│  │   * "targetEntity": EntityID                  [WAS AIBlackboard] │ │
│  │   * "health": Float                           [WAS Health_data]  │ │
│  │   * "position": Vector                        [WAS Position_data]│ │
│  │   * "taskCounter": Int                        [Custom graph var] │ │
│  │                                                                  │ │
│  │   COMPONENT PROPERTIES (auto-synced):                            │ │
│  │   * "visionRange": Float    ← AISenses_data::visionRadius       │ │
│  │   * "spritePath": String    ← VisualSprite_data::texture        │ │
│  │   * "velocity": Vector      ← PhysicsBody_data::velocity        │ │
│  │                                                                  │ │
│  │ - API:                                                           │ │
│  │   * SetValue(key, value)    (with type-checking)                │ │
│  │   * GetValue(key)           (returns TaskValue)                 │ │
│  │   * SetValueScoped(local:key | global:key)                      │ │
│  │   * GetValueScoped(...)     (forwards to GlobalBB if global:)   │ │
│  │   * Serialize() / Deserialize() (save/load)                     │ │
│  │                                                                  │ │
│  └──────────────────────────────────────────────────────────────────┘ │
│               │ (read/write)                                           │
│               ├─────────────┬──────────────────────┬──────────────────┤ │
│               │             │                      │                  │ │
│               ▼             ▼                      ▼                  ▼ │
│   ┌─────────────────┐ ┌──────────────┐ ┌──────────────┐ ┌───────────┐│ │
│   │ VSGraphExecutor │ │ ECS Getters  │ │ Components   │ │ AI Systems││ │
│   │ (node exec)     │ │ (read state) │ │ (Properties) │ │ (legacy)  ││ │
│   └─────────────────┘ └──────────────┘ └──────────────┘ └───────────┘│ │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐ │
│  │ ECS Components (Synchronized from LocalBlackboard)               │ │
│  ├──────────────────────────────────────────────────────────────────┤ │
│  │ [DEPRECATED: Don't add new ones]                                 │ │
│  │                                                                  │ │
│  │ ❌ AIBlackboard_data → Remove, use LocalBB instead              │ │
│  │ ❌ AISenses_data → Move to LocalBB properties                   │ │
│  │ ❌ AIState_data → Move to LocalBB properties                    │ │
│  │                                                                  │ │
│  │ ✅ Position_data → Keep (performance), sync from LocalBB        │ │
│  │ ✅ Health_data → Keep (hot path), sync from LocalBB            │ │
│  │ ✅ PhysicsBody_data → Keep (physics), sync from LocalBB        │ │
│  │ ✅ TaskRunnerComponent → CORE (holds LocalBB + execution state) │ │
│  │                                                                  │ │
│  └──────────────────────────────────────────────────────────────────┘ │
│                                                                         │
└────────────────────────────────────────────────────────────────────────┘
         │ (execute graph)
         │
┌────────▼──────────────────────────────────────────────────────────────┐
│                    RUNTIME SYSTEMS (FRAME LOOP)                        │
├────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  GameEngine::Run()                                                     │
│    ├─ EventSystem::Dispatch()  → EventQueue populated                 │
│    │   (triggers: collision, damage, input, etc.)                     │
│    │                                                                   │
│    ├─ World::Update(dt)                                               │
│    │   ├─ TaskSystem::Process()  ★ MAIN SCRIPT EXECUTION              │
│    │   │   for each entity with TaskRunnerComponent:                  │
│    │   │     ├─ Get TaskGraphTemplate from AssetManager               │
│    │   │     ├─ Execute VSGraphExecutor::ExecuteFrame(...)            │
│    │   │     │   * Loop nodes (max 64 per frame)                      │
│    │   │     │   * Call HandleGetBBValue() → reads LocalBB            │
│    │   │     │   * Call HandleSetBBValue() → writes LocalBB           │
│    │   │     │   * Call HandleBranch() → evals condition from LB      │
│    │   │     │   * Call HandleAtomicTask() → runs tasks               │
│    │   │     │   * ... other node types ...                           │
│    │   │     └─ Store CurrentNodeID back in TaskRunnerComponent       │
│    │   │                                                               │
│    │   ├─ [DEPRECATED] BehaviorTreeSystem::Process()                 │
│    │   │   (Legacy: uses AIBlackboard_data, separate from VS)         │
│    │   │   → REMOVE after migration                                   │
│    │   │                                                               │
│    │   ├─ [LEGACY] AIStimuliSystem::Process()                        │
│    │   │   (Reads EventQueue → writes AIBlackboard_data)              │
│    │   │   → REPLACE with EventToBlackboardBridge                     │
│    │   │                                                               │
│    │   ├─ [LEGACY] AIPerceptionSystem::Process()                     │
│    │   │   (Updates AIBlackboard perception state)                    │
│    │   │   → REPLACE with generic SensorSystem                        │
│    │   │                                                               │
│    │   ├─ [LEGACY] AIStateTransitionSystem::Process()                │
│    │   │   (Reads AIBlackboard, transitions AIState_data modes)       │
│    │   │   → REPLACE with VS graph logic + SetBBValue                 │
│    │   │                                                               │
│    │   ├─ PhysicsSystem::Process()                                    │
│    │   │   ├─ Reads PhysicsBody_data + Position_data                  │
│    │   │   └─ Can optionally sync Position changes to LocalBB        │ │
│    │   │                                                               │ │
│    │   ├─ RenderingSystem::Process()                                  │
│    │   │   ├─ Reads VisualSprite_data + Position_data                 │
│    │   │   └─ Can optionally sync to LocalBB for scripting            │
│    │   │                                                               │
│    │   └─ [OTHER SYSTEMS]                                             │
│    │                                                                   │
│    └─ Render frame                                                    │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
         │
         └─ Loop next frame


EDITOR LAYER (Parallel, No Execution)
═══════════════════════════════════════════════════════════════════════

VisualScriptEditorPanel
├─ Load .ats file → TaskGraphTemplate in-memory
├─ RenderBlackboardPanel()
│  ├─ Call BlackboardEditorPanel::Render()
│  │  ├─ Get variable list from template.Blackboard
│  │  ├─ ImGui inputs for each: name, type, default
│  │  ├─ User CRUD operations (add/remove/edit)
│  │  └─ Sync back to in-memory template
│  └─ RenderComponentPropertiesPanel()
│     ├─ Show all component fields (from ComponentRegistry schema)
│     ├─ User edits properties
│     └─ Saved to prefab JSON (not graph JSON)
├─ RenderNodeGraphPanel()
│  ├─ Render nodes (EntryPoint, Branch, GetBBValue, etc.)
│  ├─ Render connections (exec + data)
│  └─ Allow drag-connect, add/remove nodes
├─ On Save:
│  ├─ Serialize template to .ats JSON file
│  └─ AssetManager invalidates old cache
├─ DebugController (when running in editor preview)
│  ├─ Hook into VSGraphExecutor::OnNodeExecuting()
│  ├─ Capture LocalBlackboard snapshot
│  └─ Display variable values in UI (live watch)
└─ PerformanceProfiler
   ├─ Collect per-node execution times
   └─ Display hotspots
   
   📊 RÉSUMÉ: AVANT → APRÈS
Aspect	AVANT	APRÈS
Nombre de Blackboards	3 (AIBlackboard, LocalBB, TaskRunner.BB)	1 + LocalBlackboard + GlobalBlackboard
Synchronisation per-frame	3 × full-copy (150 LOC)	0 (persistent BB)
Duplicate AI State	AIBlackboard (30 fields)	LocalBlackboard (same 30 keys)
Systems for AI	6 (BehaviorTree, Stimuli, Perception, Transition, Motion, Debug)	2 (EventToBlackboardBridge, SensorSystem)
Event→State Path	EventQueue → AIBlackboard_data	EventQueue → EventToBlackboardBridge → LocalBB
Component Properties	ParameterSchema (hardcoded)	ComponentRegistry schema
Editor Component List	Hardcoded in UI	Auto-discovered from registry
Entity Variable Access	3 APIs (BB, Component, Intent)	1 API (LocalBlackboard getter/setter)
VS Graph Support	Partial (GetBBValue/SetBBValue)	Full (unified scoped API)
Code Lines (Systems)	~2000	~500
Test Coverage	Scattered	Unified (BB-centric)
✅ VALIDATION FINALE
Votre vision de "Single Point of Truth" est 100% valide. Les ajouts que j'ai trouvés :

✅ LocalBlackboard doit rester persistant (ne pas recréer chaque frame)
✅ EventQueue doit être formalisé (EventToBlackboardBridge)
✅ ComponentRegistry doit exposer schema (auto-discovery)
✅ ConditionRegistry doit se merger (ExpressionEvaluator sur BB)
✅ Toutes les données AI → LocalBlackboard (retire AIBlackboard_data)
✅ Perception/Stimuli → Systèmes génériques (SensorSystem, EventBridge)
✅ ParameterSchema → Mergé dans ComponentRegistry
La pipeline End-to-End sera :

Code
Editor (.ats) → TaskGraphTemplate → LocalBlackboard ↔ VSGraphExecutor
                                         ↓
                                   ECS Components
                                         ↓
                                   Singletons
                                   (GlobalBB)
▲


