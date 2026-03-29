# 📊 DIAGRAMMES ARCHITECTURAUX ET FLUX D'IMPLÉMENTATION

---

## 1. Graphe de Dépendances Complet

```
┌─────────────────────────────────────────────────────────────────┐
│                          PHASE 0 (PRÉALABLES)                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────┐   ┌─────────────────┐  ┌────────────┐ │
│  │   EventQueue        │   │ Pathfinding     │  │ Spatial    │ │
│  │   (exists)          │   │ System (exists) │  │ Partition  │ │
│  │                     │   │                 │  │ (exists)   │ │
│  └──────────┬──────────┘   └────────┬────────┘  └─────┬──────┘ │
│             │                       │                 │        │
│             ↓                       ↓                 ↓        │
│  ┌─────────────────────┐   ┌─────────────────┐  ┌────────────┐ │
│  │ P0.1:               │   │ P0.3:           │  │ P0.2:      │ │
│  │ EventToBlackboard   │   │ AI_MoveTask     │  │ Sensor     │ │
│  │ Bridge              │   │ AtomicTask      │  │ System     │ │
│  │                     │   │                 │  │            │ │
│  │ • Event→TaskValue   │   │ • Pathfinding   │  │ • Vision   │ │
│  │   conversion        │   │ • Movement      │  │ • Hearing  │ │
│  │ • Event mapping     │   │ • Path follow   │  │ • Timesliced
│  │ • LocalBB writes    │   │ • Status return │  │            │ │
│  │                     │   │                 │  │            │ │
│  │ Duration: 2 days    │   │ Duration: 3 days│  │ Duration: 3d│ │
│  └─────────────────────┘   └─────────────────┘  └────────────┘ │
│       ✅ Tests Pass          ✅ Tests Pass      ✅ Tests Pass   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓↓↓
                   PHASE 0 COMPLETE & APPROVED
                              ↓↓↓
┌─────────────────────────────────────────────────────────────────┐
│                       PHASE 1-4 BEGINS                          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Data Flow: Legacy vs New

### BEFORE (Legacy - Parallel Systems)

```
EventQueue                AIBlackboard_data          SpatialPartition
    │                           │                           │
    │                           ↓                           ↓
    ├──→ AIStimuliSystem  ← per-entity data         (no integration)
    │        │
    │        ├──→ UpdateTargetEntity()
    │        ├──→ UpdateLastDamage()
    │        └──→ [hardcoded mappings]
    │
    └──→ AIPerceptionSystem ← per-entity perception state
             │
             ├──→ IsTargetVisible()
             ├──→ CastRays()
             └──→ [naive scan: O(n²)]

    And separately:
    AIStateTransitionSystem ← reads AIBlackboard_data
         │
         ├──→ if (targetVisible && health > 50) AIMode = Combat
         ├──→ [hardcoded state machine]
         └──→ Writes AIState_data
```

**Problem:** Multiple independent systems, synchronized blackboard, duplicated logic

---

### AFTER (New - Unified)

```
EventQueue                LocalBlackboard (per-entity)
    │                           ↑
    │                           │
    ├──→ EventToBlackboardBridge  ← Declarative mapping
    │        │
    │        ├──→ RegisterEventMapping(Damage → local:lastDamage)
    │        ├──→ RegisterEventMapping(Noise → local:heardNoise)
    │        └──→ [ProcessEvents() once per frame]
    │
    └──→ (legacy data gone)

SensorSystem (timesliced)  ← Per-entity sensor queries
    │
    ├──→ QueryVisibilityRay()
    ├──→ QueryNoiseRay()
    └──→ Write LocalBlackboard: local:perceptionVisibleCount, local:hasTarget

AI State Transitions       ← VS Graph (ConditionPresets)
    │
    ├──→ ai_state_transitions.ats (declarative)
    │     {
    │       "Nodes": [
    │         { "Type": "Branch", "Condition": "targetVisible && health > 50" },
    │         { "Type": "SetBBValue", "VarName": "local:aiMode", "Value": 1 }
    │       ]
    │     }
    │
    └──→ VSGraphExecutor::ExecuteFrame() ← TaskSystem already does this

Movement:                  ← VS Graph with AI_MoveTask node
    │
    ├──→ ai_patrol.ats
    │     {
    │       "Nodes": [
    │         { "Type": "AtomicTask", "TaskName": "AI_Move",
    │           "Parameters": { "targetPos": "local:moveGoal" } }
    │       ]
    │     }
    │
    └──→ AI_MoveTask::ExecuteWithContext() ← Integrated into TaskSystem
```

**Benefit:** Single source of truth (LocalBlackboard), declarative behavior, automatic debugging

---

## 3. Component Architecture: Before → After

### BEFORE: Per-Entity Data Explosion

```
┌────────────────────────────────────────┐
│ Entity: "Guard_1"                      │
├────────────────────────────────────────┤
│ Components:                            │
│ • Position_data                        │
│ • PhysicsBody_data                     │
│ • AIBlackboard_data        ← 30 fields │
│ • AISenses_data            ← 5 fields  │
│ • AIState_data             ← 3 fields  │
│ • BehaviorTreeRuntime_data ← 4 fields  │
│ • AnimationController_data │
│ • SoundEmitter_data        │
│ • HealthSystem_data        │
│                            │
│ Total: 12 components       │
│ Memory: ~2KB per entity    │
│         × 100 entities     │
│         = 200KB bloat      │
└────────────────────────────────────────┘
```

### AFTER: Consolidated

```
┌────────────────────────────────────────┐
│ Entity: "Guard_1"                      │
├────────────────────────────────────────┤
│ Components:                            │
│ • Position_data                        │
│ • PhysicsBody_data                     │
│ • TaskRunnerComponent ← 1 component   │
│   └─ LocalBlackboard (scoped keys)     │
│      ├─ "aiMode" → 3                   │
│      ├─ "targetEntity" → Guard_2       │
│      ├─ "distanceToTarget" → 50.0f     │
│      ├─ "hasTarget" → true             │
│      └─ ... (all 30+ fields as keys)   │
│                            │
│ • SensorComponent          │
│ • AnimationController_data │
│ • SoundEmitter_data        │
│ • HealthSystem_data        │
│                            │
│ Total: 6 components        │
│ Memory: ~1.2KB per entity  │
│         × 100 entities     │
│         = 120KB (-40% bloat)
└────────────────────────────────────────┘
```

---

## 4. System Lifecycle: Event Processing

### OLD: AIStimuliSystem::Process()

```
Frame N:
  ┌─────────────────────────────────────────────┐
  │ AIStimuliSystem::Process()                  │
  ├─────────────────────────────────────────────┤
  │ For each entity with AIBlackboard_data:     │
  │   1. Check EventQueue (all types)           │
  │   2. If Damage event:                       │
  │      component.lastDamageTaken = event.dmg  │ ← Hard-coded
  │   3. If Noise event:                        │
  │      component.heardNoise = true            │ ← Hard-coded
  │      component.lastNoisePosition = event.pos│ ← Hard-coded
  │                                              │
  │   Issues:                                    │
  │   • CPU: Scans all events, all entities     │
  │   • O(n*m) complexity                       │
  │   • No extensibility (add new stimulus?)    │
  │   • Data scattered (BB + Event)             │
  └─────────────────────────────────────────────┘
```

### NEW: EventToBlackboardBridge::ProcessEvents()

```
Frame N:
  ┌─────────────────────────────────────────────┐
  │ EventToBlackboardBridge::ProcessEvents()    │
  ├─────────────────────────────────────────────┤
  │ For each registered mapping:                │
  │   1. Drain EventQueue[mapping.eventType]    │
  │   2. For each event:                        │
  │      entity = mapping.targetSpecific ?      │
  │              event.targetID : event.sourceID│
  │      bb = GetLocalBlackboard(entity)        │
  │      value = mapping.extractor(event)       │
  │      bb→SetValue(mapping.bbKey, value)      │
  │                                              │
  │   Benefits:                                 │
  │   • Declarative (easy to add mappings)      │
  │   • O(m) only (m = event count)             │
  │   • Extensible (register new mappings)      │
  │   • Data unified (all in LocalBB)           │
  │   • Testable (unit test mappings)           │
  └─────────────────────────────────────────────┘
```

---

## 5. State Machine: AIStateTransitionSystem

### OLD: Hardcoded C++

```cpp
void AIStateTransitionSystem::UpdateAIState(EntityID entity) {
    auto* bb = GetComponent<AIBlackboard_data>(entity);
    auto* health = GetComponent<HealthSystem_data>(entity);

    // State machine logic embedded in code
    if (bb->targetVisible && bb->distanceToTarget < 150 && health->health > 50) {
        bb->AIMode = 3;  // Combat
    } else if (health->health < 20) {
        bb->AIMode = 4;  // Flee
    } else if (bb->hasPatrolPath) {
        bb->AIMode = 2;  // Patrol
    } else {
        bb->AIMode = 1;  // Idle
    }

    // To change logic:
    // 1. Modify C++ code
    // 2. Recompile
    // 3. Restart game
    // 4. Test
}
```

### NEW: VS Graph (Declarative)

```json
{
  "GraphType": "VisualScript",
  "Schema": "v4",
  "Name": "Guard_StateTransition",
  "EntryPointID": 100,

  "Nodes": [
    {
      "ID": 100,
      "Type": "EntryPoint",
      "NextExecPin": { "NodeID": 101 }
    },
    {
      "ID": 101,
      "Type": "Branch",
      "Label": "Combat Mode Check",
      "ConditionPresets": ["CombatReady"],  // Phase 24
      "TrueExecPin": { "NodeID": 102 },
      "FalseExecPin": { "NodeID": 103 }
    },
    {
      "ID": 102,
      "Type": "SetBBValue",
      "Label": "Enter Combat",
      "BBKey": "local:aiMode",
      "Value": 3,
      "NextExecPin": { "NodeID": 999 }
    },
    {
      "ID": 103,
      "Type": "Branch",
      "Label": "Flee Mode Check",
      "ConditionPresets": ["HealthCritical"],
      "TrueExecPin": { "NodeID": 104 },
      "FalseExecPin": { "NodeID": 105 }
    },
    {
      "ID": 104,
      "Type": "SetBBValue",
      "Label": "Enter Flee",
      "BBKey": "local:aiMode",
      "Value": 4,
      "NextExecPin": { "NodeID": 999 }
    },
    {
      "ID": 105,
      "Type": "Branch",
      "Label": "Patrol Check",
      "Condition": "local:hasPatrolPath",
      "TrueExecPin": { "NodeID": 106 },
      "FalseExecPin": { "NodeID": 107 }
    },
    {
      "ID": 106,
      "Type": "SetBBValue",
      "Label": "Enter Patrol",
      "BBKey": "local:aiMode",
      "Value": 2,
      "NextExecPin": { "NodeID": 999 }
    },
    {
      "ID": 107,
      "Type": "SetBBValue",
      "Label": "Enter Idle",
      "BBKey": "local:aiMode",
      "Value": 1,
      "NextExecPin": { "NodeID": 999 }
    },
    {
      "ID": 999,
      "Type": "Return"
    }
  ],

  "ConditionPresets": [
    {
      "ID": "CombatReady",
      "Operator": "AND",
      "Conditions": [
        { "Type": "BBKey", "Key": "local:targetVisible", "Op": "==", "Value": true },
        { "Type": "BBKey", "Key": "local:distanceToTarget", "Op": "<", "Value": 150 },
        { "Type": "BBKey", "Key": "local:health", "Op": ">", "Value": 50 }
      ]
    },
    {
      "ID": "HealthCritical",
      "Operator": "AND",
      "Conditions": [
        { "Type": "BBKey", "Key": "local:health", "Op": "<", "Value": 20 }
      ]
    }
  ]
}
```

**To change logic:**
1. Edit graph visually in VisualScriptEditorPanel
2. Set breakpoints, inspect variables with DebugController
3. Save .ats file
4. Reload → immediately live (no recompile)

---

## 6. Integration Flow: TaskRunnerComponent

```
┌──────────────────────────────────────────────────────────────┐
│ TaskSystem::Process()                                        │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│ For each entity with TaskRunnerComponent:                   │
│   1. Load TaskGraphTemplate (GraphTemplateID)               │
│   2. Get LocalBlackboard (runner.LocalBlackboard)           │
│   3. Call VSGraphExecutor::ExecuteFrame(...)                │
│                                                              │
│      ┌────────────────────────────────────────────────────┐ │
│      │ VSGraphExecutor::ExecuteFrame()                    │ │
│      ├────────────────────────────────────────────────────┤ │
│      │                                                    │ │
│      │ CurrentNodeID → 100 (EntryPoint)                  │ │
│      │   • No action, advance to successor               │ │
│      │                                                    │ │
│      │ CurrentNodeID → 101 (Branch)                      │ │
│      │   • Eval condition via Phase 24 presets           │ │
│      │   • true → advance to 102                         │ │
│      │                                                    │ │
│      │ CurrentNodeID → 102 (SetBBValue)                  │ │
│      │   • bb→SetValue("local:aiMode", 3)               │ │
│      │   • advance to successor                          │ │
│      │                                                    │ │
│      │ ... (continue frame)                              │ │
│      │                                                    │ │
│      │ Max 64 nodes per frame (safety limit)             │ │
│      │                                                    │ │
│      │ Status returned: Running, Success, Failure        │ │
│      │                                                    │ │
│      └────────────────────────────────────────────────────┘ │
│                                                              │
│   4. Update runner.CurrentNodeID for next frame            │
│   5. If Status == Success → reset to EntryPoint            │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## 7. AI Entity Lifecycle (Complete)

```
┌─────────────────────────────────────────────────────────────┐
│ ENTITY CREATION: Guard_1                                    │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ CreateEntity(Guard_1)                                       │
│   ├─ AddComponent<Position_data>(pos)                       │
│   ├─ AddComponent<PhysicsBody_data>(speed, mass)            │
│   ├─ AddComponent<HealthSystem_data>(hp)                    │
│   ├─ AddComponent<SensorComponent>(vision, hearing)         │
│   │                                                         │
│   └─ AddComponent<TaskRunnerComponent>()                    │
│      ├─ GraphTemplateID = AssetManager::LoadGraph(         │
│      │                     "Blueprints/AI/Guard_Behavior")  │
│      ├─ CurrentNodeID = ENTRY_POINT                         │
│      ├─ StateTimer = 0.0f                                   │
│      └─ LocalBlackboard = new LocalBlackboard()             │
│         ├─ "aiMode" = 1 (Idle)                             │
│         ├─ "targetEntity" = INVALID_ENTITY_ID              │
│         ├─ "distanceToTarget" = 0.0f                       │
│         └─ ... (other init values)                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
         ↓ EVERY FRAME ↓

┌─────────────────────────────────────────────────────────────┐
│ FRAME N: AI Update Sequence                                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│ 1. EventToBlackboardBridge::ProcessEvents()                 │
│    Events (Damage, Noise, Detection)                        │
│      → Map to LocalBlackboard keys                          │
│                                                             │
│ 2. SensorSystem::Process()  (timesliced @ 5Hz)              │
│    For Guard_1:                                            │
│      → QueryVisibilityRay(Guard_1, 300m, 180°)             │
│      → Writes LocalBB: perceptionVisibleCount, hasTarget    │
│                                                             │
│ 3. TaskSystem::Process()                                    │
│    For Guard_1 (has TaskRunnerComponent):                  │
│      → Load ai_state_transitions.ats graph                  │
│      → VSGraphExecutor::ExecuteFrame(...)                   │
│         • Branch: "CombatReady" preset?                    │
│         • YES → Set "local:aiMode" = 3 (Combat)            │
│      → Load ai_combat_behavior.ats graph                    │
│      → VSGraphExecutor::ExecuteFrame(...)                   │
│         • AtomicTask: AI_Move to targetEntity               │
│         • AI_MoveTask::Execute() → pathfinding + move       │
│                                                             │
│ 4. PhysicsSystem::Process()                                 │
│    Applies velocity from AI_MoveTask                        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. Memory & Performance Comparison

### BEFORE (Legacy)

```
Per AI Entity:
  • AIBlackboard_data:        ~120 bytes (30 fields)
  • AISenses_data:            ~16 bytes (5 fields)
  • AIState_data:             ~12 bytes (3 fields)
  • BehaviorTreeRuntime_data: ~16 bytes (4 fields)
  ───────────────────────────
  SUBTOTAL:                   ~164 bytes

Per Frame (100 AI entities):
  • AIStimuliSystem::Process():
    - EventQueue scan:        O(event_count)
    - BB update:              O(100) entities
    - Estimate:               2ms

  • AIPerceptionSystem::Process():
    - Naive visibility scan:  O(100²) = 10,000 raycasts!
    - Estimate:               15ms (⚠️ EXPENSIVE)

  • AIStateTransitionSystem::Process():
    - Condition check:        O(100)
    - Estimate:               1ms

  • BehaviorTreeSystem::Process():
    - Tree ticking:           O(100 * depth)
    - Estimate:               5ms

  TOTAL AI OVERHEAD:          ~23ms/frame

Memory (100 entities):        ~16.4 KB + overhead
```

### AFTER (Unified)

```
Per AI Entity:
  • TaskRunnerComponent:      ~64 bytes (ptr + IDs)
  • LocalBlackboard:          ~32 bytes (map size)
  • SensorComponent:          ~16 bytes (ranges)
  ───────────────────────────
  SUBTOTAL:                   ~112 bytes (-32% ✅)

Per Frame (100 AI entities):
  • EventToBlackboardBridge::ProcessEvents():
    - Drain events:           O(event_count)
    - BB updates:             O(events_processed)
    - Estimate:               0.5ms

  • SensorSystem::Process():  (timesliced 5Hz)
    - 20 entities/frame scan: O(20 * visible_count)
    - With spatial partition: ~2ms
    - Estimate:               2ms (-87% vs naive ✅)

  • TaskSystem::Process():
    - VSGraphExecutor:        O(100 * nodes_per_frame)
    - Batched, optimized:     ~3ms
    - Estimate:               3ms (-40% vs BT ✅)

  TOTAL AI OVERHEAD:          ~5.5ms/frame (-76% ✅)

Memory (100 entities):        ~11.2 KB (-32% ✅)

FREED:                        17.5ms/frame + 5KB ✅
```

---

## 9. Implementation Dependency Graph (Detailed)

```
┌─────────────────────────────────────────────────────────────────┐
│ WEEK 0: Phase 0 (Prerequisite Components)                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│ ┌──────────────────────┐                                        │
│ │ EventQueue (exists)  │                                        │
│ └──────────┬───────────┘                                        │
│            │                                                    │
│            ↓                                                    │
│ ┌─────────────────────────────────────────────────────┐        │
│ │ P0.1: EventToBlackboardBridge                       │        │
│ │ ├─ Impl EventToBlackboardBridge.h/cpp (2 days)     │        │
│ │ ├─ RegisterEventMapping() API                       │        │
│ │ ├─ ProcessEvents() each frame                       │        │
│ │ └─ Tests: EventToBlackboardBridgeTests.cpp          │        │
│ │   Status: ✅ Unit tests pass                        │        │
│ └─────────────────────────────────────────────────────┘        │
│            │ ✅ Tests Pass                                     │
│            └─→ UNBLOCKS P1.3 (AIStimuliSystem archive)         │
│                                                                 │
│ ┌──────────────────────┐                                        │
│ │ SpatialPartition     │                                        │
│ │ (exists)             │                                        │
│ └──────────┬───────────┘                                        │
│            │                                                    │
│            ↓                                                    │
│ ┌─────────────────────────────────────────────────────┐        │
│ │ P0.2: SensorSystem                                  │        │
│ │ ├─ Impl SensorSystem.h/cpp (2 days)               │        │
│ │ ├─ Impl SensorComponent (1 day)                   │        │
│ │ ├─ Vision + hearing queries                       │        │
│ │ ├─ LocalBlackboard writes                         │        │
│ │ └─ Tests: SensorSystemTests.cpp                   │        │
│ │   Status: ✅ Unit tests pass                       │        │
│ └─────────────────────────────────────────────────────┘        │
│            │ ✅ Tests Pass                                     │
│            └─→ UNBLOCKS P2.1 (SensorSystem integration)        │
│                                                                 │
│ ┌──────────────────────┐                                        │
│ │ PathfindingSystem    │                                        │
│ │ (exists)             │                                        │
│ └──────────┬───────────┘                                        │
│            │                                                    │
│            ↓                                                    │
│ ┌─────────────────────────────────────────────────────┐        │
│ │ P0.3: AI_MoveTask                                   │        │
│ │ ├─ Impl AI_MoveTask.h/cpp (3 days)                │        │
│ │ ├─ Pathfinding integration                         │        │
│ │ ├─ Movement following path                         │        │
│ │ ├─ AtomicTask interface                            │        │
│ │ └─ Tests: AI_MoveTaskTests.cpp                     │        │
│ │   Status: ✅ Unit tests pass                       │        │
│ └─────────────────────────────────────────────────────┘        │
│            │ ✅ Tests Pass                                     │
│            └─→ UNBLOCKS P3.1 (AI_MoveTask integration)         │
│                                                                 │
│            ⬇️⬇️⬇️ PHASE 0 COMPLETE ⬇️⬇️⬇️                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ WEEKS 1-2: Phase 1 (Foundation)                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│ ┌──────────────────────────────────────────────────────────┐   │
│ │ P1.1: CompatibilityLayer                               │   │
│ │ ├─ Impl TaskSystem/CompatibilityLayer.h (1 day)        │   │
│ │ ├─ All AIBlackboard_data fields → LocalBB forwarding   │   │
│ │ ├─ GetLocalBlackboard(), GetAIBlackboardCompat() APIs  │   │
│ │ └─ Tests: CompatibilityLayerTests.cpp                  │   │
│ │   Status: ✅ Unit tests pass                           │   │
│ └──────────────────────────────────────────────────────────┘   │
│            │ ✅ Tests Pass                                     │
│            │                                                   │
│            ├─→ UNBLOCKS P1.2 (AIBlackboard_data archive)      │
│            │                                                   │
│ ┌──────────↓──────────────────────────────────────────────┐   │
│ │ P1.2: Archive AIBlackboard_data                         │   │
│ │ ├─ Backup original → Archive/                          │   │
│ │ ├─ Create stub with deprecation warning                │   │
│ │ ├─ Compilation verification                            │   │
│ │ └─ Regression tests: Behavior unchanged                 │   │
│ │   Status: ✅ Clean build                               │   │
│ └──────────┬──────────────────────────────────────────────┘   │
│            │                                                   │
│            ├─→ UNBLOCKS P1.3 (AIStimuliSystem archive)        │
│            │                                                   │
│ ┌──────────↓──────────────────────────────────────────────┐   │
│ │ P1.3: Archive AIStimuliSystem                           │   │
│ │ ├─ Register EventToBlackboardBridge mappings (P0.1) ✅ │   │
│ │ ├─ Remove AIStimuliSystem from World                   │   │
│ │ ├─ Archive source files                                │   │
│ │ └─ Tests: Events → LocalBlackboard                     │   │
│ │   Status: ✅ Integration tests pass                    │   │
│ └──────────┬──────────────────────────────────────────────┘   │
│            │                                                   │
│            ├─→ UNBLOCKS P1.4                                  │
│            │                                                   │
│ ┌──────────↓──────────────────────────────────────────────┐   │
│ │ P1.4: Archive BehaviorTreeRuntime_data                  │   │
│ │ ├─ Verify TaskRunnerComponent has all fields           │   │
│ │ ├─ Remove component definition                         │   │
│ │ └─ Compilation verification                            │   │
│ │   Status: ✅ Clean build                               │   │
│ └──────────┬──────────────────────────────────────────────┘   │
│            │                                                   │
│            ⬇️ PHASE 1 COMPLETE ⬇️                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

[CONTINUE to WEEKS 2-4: Phases 2-4...]
```

This visualization shows:
- ✅ What exists
- 🔴 What must be created first
- 📊 How each piece depends on previous pieces
- ✅ Test gates between phases
