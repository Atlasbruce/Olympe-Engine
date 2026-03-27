# 🧹 NETTOYAGE DES SYSTÈMES REDUNDANTS AI — Plan Détaillé Révisé

**Date:** 2026-03-27  
**Status:** PLAN DE NETTOYAGE COMPLET + CALENDRIER  
**Audience:** @Atlasbruce + Dev Team  
**C++ Standard:** C++14 strict  
**Document Parent:** Legacy Cleanup Strategy (27-03-2026)

---

## 📋 TABLE DES MATIÈRES

1. [Analyse Comparative](#analyse-comparative)
2. [Différences vs Document Original](#différences-vs-document-original)
3. [Phase 0: Préalables (Nouveau)](#phase-0--préalables-nouveau)
4. [Phases 1-4: Détail Complet](#phases-1-4--détail-complet)
5. [Calendrier Détaillé](#calendrier-détaillé)
6. [Dépendances et Risques](#dépendances-et-risques)
7. [Checklist d'Exécution](#checklist-dexécution)

---

## ANALYSE COMPARATIVE

### État Réel vs Document Original

#### ✅ CONFIRMÉ: Systèmes Existants

| Composant | Localisation | État | Mature? |
|-----------|-------------|------|---------|
| **LocalBlackboard** | `Source/TaskSystem/LocalBlackboard.h/cpp` | ✅ Implémenté | ✅ Oui |
| **GlobalBlackboard** | `Source/NodeGraphCore/GlobalBlackboard.h/cpp` | ✅ Implémenté | ✅ Oui |
| **TaskRunnerComponent** | `Source/ECS/Components/TaskRunnerComponent.h/cpp` | ✅ Implémenté | ✅ Oui |
| **VSGraphExecutor** | `Source/TaskSystem/VSGraphExecutor.h/cpp` | ✅ Implémenté | ✅ Oui (Phase 24 integrated) |
| **DebugController** | `Source/BlueprintEditor/DebugController.h/cpp` | ✅ Implémenté | ✅ Oui |
| **6 AI Systems Legacy** | `Source/ECS_Systems_AI.h/cpp` | ✅ Existent | ❌ À archiver |
| **AIBlackboard_data** | `Source/ECS_Components_AI.h` | ✅ Existant (30+ fields) | ❌ À supprimer |
| **BehaviorTree legacy** | `Source/AI/BehaviorTree.h/cpp` | ✅ Existant | ❌ À archiver |

#### ❌ MANQUANT: Briques Critiques

| Composant | Raison de Création | Impact | Priorité |
|-----------|-------------------|--------|----------|
| **EventToBlackboardBridge** | Mapper EventQueue → LocalBlackboard | BLOQUEUR pour P1.3 | P0.1 🔴 |
| **SensorSystem** | Remplacer AIPerceptionSystem | BLOQUEUR pour P2.1 | P0.2 🔴 |
| **AI_MoveTask** | Remplacer AIMotionSystem logic | BLOQUEUR pour P3.1 | P0.3 🔴 |
| **CompatibilityLayer** | Wrapper pour transition douce | CRITIQUE pour P1.2 | P1.1 🟠 |

---

## DIFFÉRENCES vs DOCUMENT ORIGINAL

### 1️⃣ **PHASE 0 N'EXISTAIT PAS** → À AJOUTER

#### Problème Identifié
Le document original supposait que **EventToBlackboardBridge**, **SensorSystem**, et **AI_MoveTask** existaient déjà. Après analyse du workspace:

- ❌ `EventToBlackboardBridge` : **INEXISTANT**
- ❌ `SensorSystem` : **INEXISTANT**  
- ❌ `AI_MoveTask` : **INEXISTANT**

**Conséquence:** Les phases P1, P2, P3 du plan original étaient **impossibles à exécuter**.

#### Solution
**Créer Phase 0** avec les trois briques manquantes AVANT d'archiver les systèmes legacy. Cela garantit:

1. ✅ Les remplaçants existent
2. ✅ Tests unitaires des briques nouvelles
3. ✅ Intégration avec LocalBlackboard/GlobalBlackboard
4. ✅ Aucun "black hole" dans la logique AI

---

### 2️⃣ **PHASE 24 CONDITION PRESETS** → OPPORTUNITÉ LEVERAGE

#### Ce qui a Changé
VSGraphExecutor intègre déjà **Phase 24 Condition Presets** (nouvelles depuis le document original):

```cpp
// Phase 24 - déjà implémenté dans VSGraphExecutor::HandleBranch()
if (node->HasConditionPresets()) {
    return ConditionPresetEvaluator::Evaluate(
        node->conditionRefs,  // Refs embedded dans le v4 schema
        runner.LocalBlackboard
    );
}
```

#### Implications pour AIStateTransitionSystem
- **Avant:** Hardcoded logic en C++ (AIStateTransitionSystem::UpdateAIState)
- **Après:** Can use Condition Presets embedded dans le graph JSON
- **Avantage:** Plus de boilerplate → plus de flexibilité

**Action P2.2:** Utiliser Phase 24 pour les state transitions (plutôt que branching stateless).

---

### 3️⃣ **ÉDITEUR LEGACY** → PLUS COMPLEXE QUE PRÉVU

#### Découverte du Workspace
Fichiers BehaviorTree éditeur présents:

```
Source/AI/BehaviorTree.h/cpp           ← Core BT system
Source/AI/BehaviorTreeDependencyScanner.h/cpp
Source/AI/BehaviorTreeAdapter.h        ← VSGraph adaptation layer (!!)
Source/AI/BehaviorTreeDebugWindow.h/cpp
Source/AI/BehaviorTreeDebugWindow_NodeGraph.cpp  ← ⚠️ Pas dans le plan
Source/BlueprintEditor/BehaviorTreeEditorPlugin.h/cpp
Source/BlueprintEditor/BehaviorTreeRenderer.h/cpp
Source/NodeGraphShared/BehaviorTreeAdapter.h
```

#### Impact sur P4
- **P4.1** doit archiver **plus de fichiers** que prévu
- **BehaviorTreeAdapter** représente une tentative d'intégration BT ↔ VSGraph
  - Pourrait être utilement **conservé** comme outil de conversion
  - OU archivé si obsolète

**Decision nécessaire:** Vérifier `BehaviorTreeAdapter` pour migration BT → ATS

---

### 4️⃣ **DURATION RÉALISTE** → +1-2 SEMAINES

| Estimation | Document Original | Plan Révisé | Δ |
|------------|------------------|-------------|---|
| Phase 0 | ❌ N/A | **5-6 jours** | +5-6j |
| Phase 1 | 5 jours | 6 jours | +1j |
| Phase 2 | 6 jours | 7 jours | +1j |
| Phase 3 | 5 jours | 6 jours | +1j |
| Phase 4 | 5 jours | 5 jours | ±0j |
| **TOTAL** | **~26 jours** | **29-31 jours** | **+3-5j** |

**Nouvelle Timeline:** 6-7 semaines (not 5 weeks)

---

## PHASE 0 — PRÉALABLES (NOUVEAU)

### Objectif
Créer les trois briques manquantes AVANT d'archiver les systèmes legacy.

### 📅 DURÉE: 5-6 jours (Semaine 0)

---

### P0.1: EventToBlackboardBridge

**Fichier à créer:** `Source/TaskSystem/EventToBlackboardBridge.h`

**Objectif:**
- Mapper déclarativement les événements EventQueue vers LocalBlackboard
- Remplacer 150 LOC de AIStimuliSystem::Process()

**Spécification:**

```cpp
/*
 * @file EventToBlackboardBridge.h
 * @brief Declarative event-to-blackboard mapping system
 * 
 * Maps EventQueue domain events to LocalBlackboard updates
 * without needing a dedicated ECS system.
 * 
 * Replaces: AIStimuliSystem (150 LOC)
 * Architecture: Event → TaskValue conversion → LocalBB write
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include "TaskGraphTypes.h"  // TaskValue

// Forward declarations
struct Event;  // from EventSystem

/**
 * @struct EventMapping
 * @brief Declarative mapping from Event type to LocalBlackboard key
 */
struct EventMapping {
    std::string eventType;              // e.g., "Damage", "Noise", "Detection"
    std::string bbKey;                  // e.g., "local:lastDamage"

    // Extractor: converts Event → TaskValue
    std::function<TaskValue(const Event&)> extractor;

    // If true, apply to target entity (Event.targetID)
    // If false, apply to source entity (Event.sourceID)
    bool targetSpecific = false;

    // Duration to persist value (-1.0f = persist indefinitely)
    float persistDuration = -1.0f;
};

/**
 * @class EventToBlackboardBridge
 * @brief Singleton managing event→blackboard mappings
 */
class EventToBlackboardBridge {
public:
    static EventToBlackboardBridge& Get();

    /**
     * Register a new event→BB mapping
     * Called once during World initialization
     */
    void RegisterEventMapping(const EventMapping& mapping);

    /**
     * Process all pending events and update blackboards
     * Called by EventSystem each frame
     */
    void ProcessEvents();

    // For testing/debugging
    size_t GetMappingCount() const;

private:
    EventToBlackboardBridge() = default;
    std::vector<EventMapping> m_mappings;
};
```

**Fichier à créer:** `Source/TaskSystem/EventToBlackboardBridge.cpp`

```cpp
#include "EventToBlackboardBridge.h"
#include "LocalBlackboard.h"
#include "ECS_Entity.h"
#include "../EventSystem.h"  // EventQueue, Event struct

EventToBlackboardBridge& EventToBlackboardBridge::Get() {
    static EventToBlackboardBridge instance;
    return instance;
}

void EventToBlackboardBridge::RegisterEventMapping(const EventMapping& mapping) {
    // Validate mapping
    if (mapping.eventType.empty() || mapping.bbKey.empty()) {
        LOG_ERROR("EventToBlackboardBridge: Invalid mapping (empty type or key)");
        return;
    }

    m_mappings.push_back(mapping);
    LOG_DEBUG("EventToBlackboardBridge: Registered mapping %s → %s", 
              mapping.eventType.c_str(), mapping.bbKey.c_str());
}

void EventToBlackboardBridge::ProcessEvents() {
    auto& eventQueue = EventQueue::Get();

    for (const auto& mapping : m_mappings) {
        // Drain all events of this type from queue
        std::vector<Event> events = eventQueue.Drain(mapping.eventType);

        for (const auto& event : events) {
            EntityID targetEntity = mapping.targetSpecific ? 
                                   event.targetID : event.sourceID;

            if (targetEntity == INVALID_ENTITY_ID) continue;

            LocalBlackboard* bb = GetLocalBlackboard(targetEntity);
            if (!bb) continue;

            // Extract value from event
            TaskValue value = mapping.extractor(event);

            // Write to blackboard with optional persistence
            bb->SetValue(mapping.bbKey, value);

            if (mapping.persistDuration > 0.0f) {
                // TODO: Implement auto-expiry for persistent values
            }
        }
    }
}

size_t EventToBlackboardBridge::GetMappingCount() const {
    return m_mappings.size();
}
```

**Tests à Écrire:**
- `Tests/TaskSystem/EventToBlackboardBridgeTests.cpp`
  - Test: RegisterEventMapping validates inputs
  - Test: ProcessEvents extracts and writes values
  - Test: Multiple entities receive events independently
  - Test: Event type filtering works

**Durée:** 2 jours (impl + tests)

---

### P0.2: SensorSystem

**Fichier à créer:** `Source/ECS/Systems/SensorSystem.h`

**Objectif:**
- Évaluer les perceptions des entités (vision, hearing)
- Écrire les résultats dans LocalBlackboard
- Remplacer 300 LOC de AIPerceptionSystem

**Spécification:**

```cpp
/*
 * @file SensorSystem.h
 * @brief Perception system: vision, hearing, detection
 * 
 * For each entity with SensorComponent:
 *   - Evaluates visibility to potential targets
 *   - Evaluates audible events (noise sources)
 *   - Writes perception state to LocalBlackboard
 * 
 * Replaces: AIPerceptionSystem (300 LOC)
 * Depends on: SensorComponent, LocalBlackboard, SpatialPartitioning
 */

#pragma once

#include "../ECS_System.h"
#include <vector>

// Forward declarations
struct SensorComponent;

/**
 * @struct PerceptionResult
 * @brief Result of a single perception query
 */
struct PerceptionResult {
    std::vector<EntityID> visibleTargets;
    std::vector<EntityID> heardTargets;
    EntityID primaryTarget = INVALID_ENTITY_ID;
};

/**
 * @class SensorSystem
 * @brief Timesliced perception updates
 */
class SensorSystem : public ECS_System {
public:
    SensorSystem();
    virtual void Process() override;

    /**
     * Query visibility from one entity to another
     * Returns true if target is visible from source
     */
    bool IsTargetVisible(
        EntityID source, 
        EntityID target, 
        float visionRange, 
        float visionAngle
    );

    /**
     * Find all visible entities within range+angle
     */
    std::vector<EntityID> QueryVisibilityRay(
        EntityID source,
        float range,
        float angle
    );

    /**
     * Find all noise sources within hearing range
     */
    std::vector<EntityID> QueryNoiseRay(
        EntityID source,
        float hearingRange
    );

private:
    // Timeslicing support
    size_t m_lastProcessedEntity = 0;
    float m_timeSinceLastFullUpdate = 0.0f;

    // Helper: Update perception for single entity
    void UpdateEntityPerception(EntityID entity, const SensorComponent& sensor);

    // Helper: Raycast vision check
    bool CastVisionRay(
        EntityID source,
        EntityID target,
        const Vector& fromPos,
        const Vector& toPos,
        float visionAngle
    );
};
```

**Fichier à créer:** `Source/ECS/Systems/SensorSystem.cpp`

```cpp
#include "SensorSystem.h"
#include "../Components/SensorComponent.h"
#include "../../TaskSystem/LocalBlackboard.h"
#include "../../Physics/CollisionSystem.h"  // Raycast
#include "../../Spatial/SpatialPartitioning.h"
#include "ECS_Entity.h"

SensorSystem::SensorSystem() : ECS_System("SensorSystem") {}

void SensorSystem::Process() {
    // Timesliced: process ~1/5 of entities per frame (5 Hz perception)
    auto entities = GetEntitiesWithComponent<SensorComponent>();

    if (entities.empty()) return;

    size_t entitiesToProcess = (entities.size() / 5) + 1;

    for (size_t i = 0; i < entitiesToProcess && !entities.empty(); ++i) {
        EntityID entity = entities[m_lastProcessedEntity % entities.size()];
        auto* sensor = GetComponent<SensorComponent>(entity);

        if (sensor) {
            UpdateEntityPerception(entity, *sensor);
        }

        m_lastProcessedEntity++;
    }

    m_timeSinceLastFullUpdate += GetDeltaTime();
}

void SensorSystem::UpdateEntityPerception(
    EntityID entity, 
    const SensorComponent& sensor
) {
    LocalBlackboard* bb = GetLocalBlackboard(entity);
    if (!bb) return;

    // Query visible targets
    auto visibleTargets = QueryVisibilityRay(entity, sensor.visionRange, sensor.visionAngle);

    // Write to blackboard
    bb->SetValue("local:perceptionVisibleCount", TaskValue((int32_t)visibleTargets.size()));

    if (!visibleTargets.empty()) {
        bb->SetValue("local:perceptionPrimaryTarget", TaskValue(visibleTargets[0]));
        bb->SetValue("local:hasTarget", TaskValue(true));
    } else {
        bb->SetValue("local:hasTarget", TaskValue(false));
    }

    // Query noise
    auto heardTargets = QueryNoiseRay(entity, sensor.hearingRange);
    bb->SetValue("local:heardNoiseCount", TaskValue((int32_t)heardTargets.size()));
}

bool SensorSystem::IsTargetVisible(
    EntityID source,
    EntityID target,
    float visionRange,
    float visionAngle
) {
    auto* sourcePos = GetComponent<Position_data>(source);
    auto* targetPos = GetComponent<Position_data>(target);

    if (!sourcePos || !targetPos) return false;

    Vector delta = targetPos->pos - sourcePos->pos;
    float distance = delta.Length();

    if (distance > visionRange) return false;

    // TODO: Implement vision cone check + raycasting
    return CastVisionRay(source, target, sourcePos->pos, targetPos->pos, visionAngle);
}

std::vector<EntityID> SensorSystem::QueryVisibilityRay(
    EntityID source,
    float range,
    float angle
) {
    std::vector<EntityID> visible;

    // Use spatial partitioning to find candidates
    auto candidates = SpatialPartitioning::Get().QuerySphere(
        GetComponent<Position_data>(source)->pos,
        range
    );

    for (EntityID candidate : candidates) {
        if (candidate == source) continue;
        if (IsTargetVisible(source, candidate, range, angle)) {
            visible.push_back(candidate);
        }
    }

    return visible;
}

std::vector<EntityID> SensorSystem::QueryNoiseRay(
    EntityID source,
    float hearingRange
) {
    // TODO: Query NoiseEvent log + distance filtering
    std::vector<EntityID> heard;
    return heard;
}

bool SensorSystem::CastVisionRay(
    EntityID source,
    EntityID target,
    const Vector& fromPos,
    const Vector& toPos,
    float visionAngle
) {
    // TODO: Implement vision cone + raycasting
    // Use CollisionSystem::Raycast for line-of-sight
    return true;  // Placeholder
}
```

**SensorComponent à Créer:** `Source/ECS/Components/SensorComponent.h`

```cpp
struct SensorComponent {
    float visionRange = 300.0f;       // Sight distance
    float visionAngle = 180.0f;       // FOV in degrees (180 = hemisphere)
    float hearingRange = 500.0f;      // Sound detection distance

    // Timeslicing: update rate in Hz
    float perceptionHz = 5.0f;        // 5 Hz = 0.2s update
};
AUTO_REGISTER_COMPONENT(SensorComponent);
```

**Tests à Écrire:**
- `Tests/ECS/Systems/SensorSystemTests.cpp`
  - Test: Vision range limiting
  - Test: Vision cone angle filtering
  - Test: Hearing range limiting
  - Test: Timesliced updates
  - Test: Blackboard writes

**Durée:** 3 jours (impl + tests)

---

### P0.3: AI_MoveTask

**Fichier à créer:** `Source/TaskSystem/AtomicTasks/AI_MoveTask.h`

**Objectif:**
- Exécuter mouvement d'une entité IA vers une destination
- Intégrer pathfinding + mouvement
- Remplacer AIMotionSystem::Process() (180 LOC)

**Spécification:**

```cpp
/*
 * @file AI_MoveTask.h
 * @brief Atomic task: move entity to target position
 * 
 * Implements pathfinding + movement for AI entities.
 * Used within VS graphs as [AtomicTask: "AI_Move"]
 * 
 * Replaces: AIMotionSystem (180 LOC)
 * Depends on: Pathfinding, LocalBlackboard, PhysicsBody_data
 */

#pragma once

#include "IAtomicTask.h"
#include "../../vector.h"
#include <vector>

/**
 * @class AI_MoveTask
 * @brief Navigate entity from current position to target
 * 
 * Status transitions:
 *   - Idle → Running (start moving)
 *   - Running → Running (continue moving)
 *   - Running → Success (reached destination)
 *   - Running → Failure (path blocked, unreachable)
 */
class AI_MoveTask : public IAtomicTask {
public:
    AI_MoveTask();
    virtual ~AI_MoveTask();

    // IAtomicTask interface
    Status ExecuteWithContext(
        EntityID entity,
        const AtomicTaskContext& context
    ) override;

    void Abort(EntityID entity) override;

private:
    // Per-entity movement state
    struct MovementState {
        Vector targetPos = {};
        std::vector<Vector> path;
        size_t currentPathIndex = 0;
        float arrivedThreshold = 10.0f;
    };

    std::unordered_map<EntityID, MovementState> m_movementStates;

    // Helper: Get target position from blackboard or parameters
    bool GetTargetPosition(
        EntityID entity,
        const AtomicTaskContext& context,
        Vector& outTarget
    );

    // Helper: Request pathfinding
    bool RequestPath(EntityID entity, const Vector& destination);

    // Helper: Follow current path segment
    bool FollowPath(EntityID entity, float deltaTime);
};
```

**Fichier à créer:** `Source/TaskSystem/AtomicTasks/AI_MoveTask.cpp`

```cpp
#include "AI_MoveTask.h"
#include "../LocalBlackboard.h"
#include "../../ECS/Components/Position_data.h"
#include "../../ECS/Components/PhysicsBody_data.h"
#include "../../Pathfinding/PathfindingSystem.h"
#include "AtomicTaskContext.h"
#include "ECS_Entity.h"

AI_MoveTask::AI_MoveTask() : IAtomicTask("AI_Move") {}

AI_MoveTask::~AI_MoveTask() {
    m_movementStates.clear();
}

Status AI_MoveTask::ExecuteWithContext(
    EntityID entity,
    const AtomicTaskContext& context
) {
    // Get or create movement state
    auto it = m_movementStates.find(entity);
    if (it == m_movementStates.end()) {
        m_movementStates[entity] = MovementState();
        it = m_movementStates.find(entity);
    }

    MovementState& state = it->second;

    // Get target position
    Vector targetPos;
    if (!GetTargetPosition(entity, context, targetPos)) {
        return Status::Failure;
    }

    // If target changed, request new path
    if ((targetPos - state.targetPos).Length() > 1.0f) {
        state.targetPos = targetPos;
        state.currentPathIndex = 0;

        if (!RequestPath(entity, targetPos)) {
            return Status::Failure;
        }
    }

    // Follow the path
    if (FollowPath(entity, context.DeltaTime)) {
        return Status::Success;  // Reached destination
    }

    return Status::Running;
}

void AI_MoveTask::Abort(EntityID entity) {
    // Stop movement
    auto* body = GetComponent<PhysicsBody_data>(entity);
    if (body) {
        body->velocity = Vector(0, 0, 0);
    }

    m_movementStates.erase(entity);
}

bool AI_MoveTask::GetTargetPosition(
    EntityID entity,
    const AtomicTaskContext& context,
    Vector& outTarget
) {
    // Try get from task parameters
    if (context.Parameters.count("targetPos")) {
        auto value = context.Parameters.at("targetPos");
        if (value.type == TaskValueType::Vector) {
            outTarget = value.AsVector();
            return true;
        }
    }

    // Try get from blackboard
    LocalBlackboard* bb = GetLocalBlackboard(entity);
    if (bb) {
        auto value = bb->GetValue("local:moveGoal");
        if (value.IsValid() && value.type == TaskValueType::Vector) {
            outTarget = value.AsVector();
            return true;
        }
    }

    return false;
}

bool AI_MoveTask::RequestPath(EntityID entity, const Vector& destination) {
    auto* pos = GetComponent<Position_data>(entity);
    if (!pos) return false;

    // Request path from pathfinding system
    PathfindingSystem& pathfinder = PathfindingSystem::Get();
    auto& state = m_movementStates[entity];

    state.path = pathfinder.FindPath(pos->pos, destination);
    state.currentPathIndex = 0;

    return !state.path.empty();
}

bool AI_MoveTask::FollowPath(EntityID entity, float deltaTime) {
    auto it = m_movementStates.find(entity);
    if (it == m_movementStates.end()) return false;

    MovementState& state = it->second;

    if (state.path.empty()) {
        return false;  // No path
    }

    if (state.currentPathIndex >= state.path.size()) {
        return true;  // Path complete
    }

    auto* pos = GetComponent<Position_data>(entity);
    auto* body = GetComponent<PhysicsBody_data>(entity);

    if (!pos || !body) return false;

    Vector currentWaypoint = state.path[state.currentPathIndex];
    Vector direction = (currentWaypoint - pos->pos).Normalize();

    // Check if reached current waypoint
    if ((currentWaypoint - pos->pos).Length() < state.arrivedThreshold) {
        state.currentPathIndex++;

        if (state.currentPathIndex >= state.path.size()) {
            body->velocity = Vector(0, 0, 0);
            return true;  // Reached destination
        }

        return false;  // Continue to next waypoint
    }

    // Move towards current waypoint
    float moveSpeed = body->maxSpeed;
    body->velocity = direction * moveSpeed;

    return false;  // Still moving
}
```

**Tests à Écrire:**
- `Tests/TaskSystem/AtomicTasks/AI_MoveTaskTests.cpp`
  - Test: Pathfinding request triggered
  - Test: Movement follows path
  - Test: Waypoint navigation
  - Test: Arrival detection
  - Test: Task abort stops movement
  - Test: Target change replans

**Durée:** 3 jours (impl + tests)

---

### P0 Summary

| Task | Duration | Dependency | Status |
|------|----------|-----------|--------|
| **P0.1: EventToBlackboardBridge** | 2 days | LocalBlackboard, EventQueue | ✅ Ready |
| **P0.2: SensorSystem** | 3 days | SpatialPartitioning, Raycast | ✅ Ready |
| **P0.3: AI_MoveTask** | 3 days | Pathfinding, LocalBlackboard | ✅ Ready |
| **Phase 0 TOTAL** | **5-6 days** | P0.1 → P0.2, P0.1 → P0.3 | ✅ Ready |

**Start Date:** Monday 2026-03-31  
**End Date:** Friday 2026-04-04

---

## PHASES 1-4 — DÉTAIL COMPLET

### PHASE 1: Foundation & Low-Risk (Week 1-2)

**Objectif:** Archiver composants legacy sans casser la gameplay  
**Dépend de:** Phase 0 ✅  
**Durée:** 6 jours (↑ +1 vs document)

#### P1.1: Create CompatibilityLayer

**Fichier à créer:** `Source/TaskSystem/CompatibilityLayer.h`

```cpp
/*
 * @file CompatibilityLayer.h
 * @brief Transition wrapper for legacy AIBlackboard_data → LocalBlackboard
 * 
 * Provides backward compatibility APIs for code still using
 * GetComponent<AIBlackboard_data>(entity).
 * 
 * Maps all AIBlackboard_data fields to LocalBlackboard keys.
 */

#pragma once

#include "../ECS_Entity.h"
#include "LocalBlackboard.h"

/**
 * @class AIBlackboardCompat
 * @brief Proxy object forwarding all AIBlackboard_data API to LocalBlackboard
 */
class AIBlackboardCompat {
public:
    AIBlackboardCompat(EntityID entity, LocalBlackboard* bb);

    // Deprecated AIBlackboard_data accessors → LocalBlackboard forwarding
    int GetAIMode() const;
    void SetAIMode(int mode);

    EntityID GetTargetEntity() const;
    void SetTargetEntity(EntityID target);

    Vector GetLastKnownTargetPosition() const;
    void SetLastKnownTargetPosition(const Vector& pos);

    bool HasTarget() const;
    void SetHasTarget(bool has);

    bool IsTargetVisible() const;
    void SetTargetVisible(bool visible);

    float GetDistanceToTarget() const;
    void SetDistanceToTarget(float distance);

    // ... (more accessors for all 30 fields)

private:
    EntityID m_entity;
    LocalBlackboard* m_blackboard;
};

/**
 * Get per-entity LocalBlackboard
 * Creates if needed
 */
LocalBlackboard* GetLocalBlackboard(EntityID entity);

/**
 * Get compatibility wrapper
 * For legacy code: GetComponent<AIBlackboard_data>(entity)
 * becomes: GetAIBlackboardCompat(entity)
 */
AIBlackboardCompat GetAIBlackboardCompat(EntityID entity);
```

**Tests:**
- Unit: All accessors forward correctly
- Integration: Legacy AI code works unchanged

**Duration:** 2 days

---

#### P1.2: Archive AIBlackboard_data Component

**Action:** Replace ECS_Components_AI.h with stub

```bash
# Backup original
cp Source/ECS_Components_AI.h Archive/Deprecated_AI_Components/AIBlackboard_data.h

# Create stub
cat > Source/ECS_Components_AI.h << 'EOF'
#pragma once

// DEPRECATED: All AI blackboard data migrated to LocalBlackboard
// See: TaskSystem/CompatibilityLayer.h for transition support
// See: Project Management/Features/cleaning_redundancy_systems_27-03-2026.md

#include "TaskSystem/CompatibilityLayer.h"

// Prevent accidental use
#define AIBlackboard_data DEPRECATED_USE_LocalBlackboard

// Other legacy components (temporary)
// TODO: Remove AISenses_data (P2.1)
// TODO: Remove AIState_data (P2.2)
// TODO: Remove BehaviorTreeRuntime_data (P1.4)

#endif
EOF
```

**Tests:**
- Compilation: No errors (with compat layer)
- Regression: Existing AI behaviors identical

**Duration:** 1 day

---

#### P1.3: Archive AIStimuliSystem

**Action:** Replace with EventToBlackboardBridge initialization

In `World.cpp` or `GameInitialization.cpp`:

```cpp
// OLD (remove):
// m_aiStimuliSystem = std::make_unique<AIStimuliSystem>();
// m_systems.push_back(m_aiStimuliSystem.get());

// NEW (add):
EventToBlackboardBridge::Get().RegisterEventMapping({
    "Damage",
    "local:lastDamage",
    [](const Event& e) { return TaskValue(e.damage); },
    true,     // Apply to target
    -1.0f     // Persist
});

EventToBlackboardBridge::Get().RegisterEventMapping({
    "Noise",
    "local:heardNoise",
    [](const Event& e) { return TaskValue(true); },
    true,
    0.5f      // Persist 0.5s (temporary alert)
});

EventToBlackboardBridge::Get().RegisterEventMapping({
    "Detection",
    "local:detectedEntity",
    [](const Event& e) { return TaskValue(e.targetID); },
    true,
    -1.0f
});
```

**Files to Archive:**
- `Source/ECS_Systems_AI.h` → remove AIStimuliSystem declaration
- `Source/ECS_Systems_AI.cpp` → remove AIStimuliSystem::Process()

**Tests:**
- Unit: EventToBlackboardBridge receives events
- Integration: AI reacts to stimuli via BB

**Duration:** 2 days

---

#### P1.4: Archive BehaviorTreeRuntime_data

**Action:** Migrate BehaviorTreeRuntime_data fields → TaskRunnerComponent

TaskRunnerComponent already has:
- `GraphTemplateID` ← `BehaviorTreeRuntime_data.treeAssetId`
- `CurrentNodeID` ← `BehaviorTreeRuntime_data.currentNodeIndex`
- `StateTimer` ← `BehaviorTreeRuntime_data.stateTimer`

**Verify in TaskRunnerComponent.h:**

```cpp
struct TaskRunnerComponent {
    AssetID GraphTemplateID = INVALID_ASSET_ID;
    int32_t CurrentNodeID = NODE_INDEX_NONE;
    float StateTimer = 0.0f;
    LocalBlackboard* m_localBlackboard = nullptr;
    IAtomicTask* activeTask = nullptr;
    // ... more fields
};
```

**Tests:**
- Unit: TaskRunnerComponent holds all BT runtime state
- Integration: BehaviorTreeSystem→TaskSystem transition works

**Duration:** 1 day

---

### PHASE 2: System Replacement (Week 2-3)

**Objectif:** Remplacer systèmes AI legacy par alternatives déclaratives  
**Dépend de:** Phase 0 ✅ Phase 1 ✅  
**Durée:** 7 jours (↑ +1 vs document)

#### P2.1: SensorSystem Integration

**Already implemented in P0.2!**

- Register SensorSystem in World
- Create SensorComponent instances for NPCs
- Verify LocalBlackboard outputs (perceptionVisibleTargets, etc.)

**Register in World:**

```cpp
// In World::InitializeSystems()
m_sensorSystem = std::make_unique<SensorSystem>();
m_systems.push_back(m_sensorSystem.get());

// Remove AIPerceptionSystem:
// m_aiPerceptionSystem = nullptr;  // Remove
```

**Update NPC Prefabs:**

```json
{
  "Entity": "Guard_AI",
  "Components": [
    { "Type": "Position_data", "pos": [100, 50, 100] },
    { "Type": "SensorComponent", "visionRange": 300.0, "visionAngle": 180.0 },
    { "Type": "TaskRunnerComponent", "GraphTemplateID": "ai_guard_patrol" }
  ]
}
```

**Tests:**
- Unit: Vision cone calculations
- Integration: NPCs perceive targets
- Regression: Existing Guard AI unchanged

**Duration:** 3 days

---

#### P2.2: AIStateTransitionSystem → VS Graph Logic

**Convert hardcoded state machine to VS Graph:**

OLD C++ logic:
```cpp
void AIStateTransitionSystem::UpdateAIState(EntityID entity) {
    auto* bb = GetComponent<AIBlackboard_data>(entity);

    if (bb->targetVisible && bb->distanceToTarget < 150) {
        bb->AIMode = 3;  // Combat
    } else if (bb->health < 20) {
        bb->AIMode = 4;  // Flee
    } else {
        bb->AIMode = 1;  // Idle
    }
}
```

NEW VS Graph (ai_state_transitions.ats):
```json
{
  "GraphType": "VisualScript",
  "Schema": "v4",
  "Name": "Guard_StateTransition",
  "Nodes": [
    {
      "ID": 100,
      "Type": "EntryPoint",
      "Label": "Start"
    },
    {
      "ID": 101,
      "Type": "Branch",
      "Label": "Combat?",
      "Condition": "targetVisible && distanceToTarget < 150",
      "ConditionPresets": ["CombatReady"],
      "TrueExecPin": { "NodeID": 102, "PinName": "Exec" },
      "FalseExecPin": { "NodeID": 103, "PinName": "Exec" }
    },
    {
      "ID": 102,
      "Type": "SetBBValue",
      "Label": "Set Combat Mode",
      "BBKey": "local:aiMode",
      "Value": 3,
      "NextExecPin": { "NodeID": 999, "PinName": "Exec" }
    },
    {
      "ID": 103,
      "Type": "Branch",
      "Label": "Flee?",
      "Condition": "health < 20",
      "ConditionPresets": ["HealthCritical"],
      "TrueExecPin": { "NodeID": 104, "PinName": "Exec" },
      "FalseExecPin": { "NodeID": 105, "PinName": "Exec" }
    },
    {
      "ID": 104,
      "Type": "SetBBValue",
      "Label": "Set Flee Mode",
      "BBKey": "local:aiMode",
      "Value": 4,
      "NextExecPin": { "NodeID": 999, "PinName": "Exec" }
    },
    {
      "ID": 105,
      "Type": "SetBBValue",
      "Label": "Set Idle Mode",
      "BBKey": "local:aiMode",
      "Value": 1,
      "NextExecPin": { "NodeID": 999, "PinName": "Exec" }
    },
    {
      "ID": 999,
      "Type": "Return",
      "Label": "End"
    }
  ],
  "ConditionPresets": [
    {
      "ID": "CombatReady",
      "Conditions": [
        { "Type": "BB", "Key": "local:targetVisible", "Operator": "==", "Value": true },
        { "Type": "BB", "Key": "local:distanceToTarget", "Operator": "<", "Value": 150.0 }
      ],
      "Operator": "AND"
    },
    {
      "ID": "HealthCritical",
      "Conditions": [
        { "Type": "BB", "Key": "local:health", "Operator": "<", "Value": 20 }
      ]
    }
  ]
}
```

**Leverage Phase 24:** ConditionPresets embedded in v4 schema ✅

**Tests:**
- Unit: Condition Presets evaluate correctly
- Integration: AI mode transitions via graph
- Regression: State transitions identical

**Duration:** 3 days

---

#### P2.3: Archive AIPerceptionSystem

**Files to Archive:**

```bash
# Remove from Source
rm Source/ECS_Systems_AI.cpp  # Remove AIPerceptionSystem::Process()
# Update Source/ECS_Systems_AI.h  # Remove AIPerceptionSystem class declaration

# Create Archive
mkdir -p Archive/Deprecated_AI_Systems/
mv ... Archive/
```

**Update World.cpp:**

```cpp
// Remove:
// m_aiPerceptionSystem = std::make_unique<AIPerceptionSystem>();
// m_systems.push_back(m_aiPerceptionSystem.get());
```

**Tests:**
- Integration: SensorSystem replaces all AIPerceptionSystem queries
- Regression: Perception behavior identical

**Duration:** 1 day

---

### PHASE 3: Motion & Behavior (Week 3-4)

**Objectif:** Remplacer BehaviorTreeSystem + AIMotionSystem  
**Dépend de:** Phase 0 ✅ Phase 1 ✅ Phase 2 ✅  
**Durée:** 6 jours (↑ +1 vs document)

#### P3.1: AI_MoveTask Integration

**Already implemented in P0.3!**

- Register AI_MoveTask in AtomicTaskRegistry
- Create example AI_MoveTo VS graph node
- Test pathfinding + movement

**Register in AtomicTaskRegistry:**

```cpp
// In TaskSystem::Initialize()
AtomicTaskRegistry::Get().Register(
    "AI_Move",
    []() { return std::make_unique<AI_MoveTask>(); }
);
```

**Usage in VS Graph:**

```json
{
  "ID": 50,
  "Type": "AtomicTask",
  "TaskName": "AI_Move",
  "Label": "Move to Target",
  "Parameters": {
    "targetPos": { "Type": "BBKey", "Value": "local:moveGoal" }
  },
  "OnSuccess": { "NodeID": 51, "PinName": "Exec" },
  "OnRunning": { "NodeID": 50, "PinName": "Exec" },
  "OnFailure": { "NodeID": 52, "PinName": "Exec" }
}
```

**Tests:**
- Unit: AI_MoveTask pathfinding + movement
- Integration: Movement in VS graphs
- Regression: AI motion behavior unchanged

**Duration:** 3 days

---

#### P3.2: Archive BehaviorTreeSystem

**Files to Archive:**

```bash
# Backup
mkdir -p Archive/Deprecated_AI_Systems/
cp Source/ECS_Systems_AI.h Archive/
cp Source/ECS_Systems_AI.cpp Archive/

# Remove from Source
rm Source/ECS_Systems_AI.h
rm Source/ECS_Systems_AI.cpp
```

**Update World.cpp:**

```cpp
// Remove:
// m_behaviorTreeSystem = std::make_unique<BehaviorTreeSystem>();
// m_systems.push_back(m_behaviorTreeSystem.get());

// Add:
// TaskSystem already runs VSGraphExecutor for each TaskRunnerComponent
// No additional system needed
```

**Convert Existing BT Trees:**

Use BehaviorTreeAdapter to convert old BT JSON → VS Graph JSON

**Tests:**
- Migration: All existing BT trees → VS graphs
- Behavior: Guard AI exhibits identical patterns
- Debugger: Use DebugController for VS graph debugging

**Duration:** 2 days (+ 2 days for BT migration tool)

---

### PHASE 4: Editor & Documentation (Week 4-5)

**Objectif:** Nettoyer éditeur + documentation  
**Dépend de:** Phase 1 ✅ Phase 2 ✅ Phase 3 ✅  
**Durée:** 5 jours (inchangé)

#### P4.1: Remove BehaviorTreeDebugWindow + Related

**Files to Archive:**

```
Source/AI/BehaviorTreeDebugWindow.h
Source/AI/BehaviorTreeDebugWindow.cpp
Source/AI/BehaviorTreeDebugWindow_NodeGraph.cpp
Source/BlueprintEditor/BehaviorTreeEditorPlugin.h
Source/BlueprintEditor/BehaviorTreeEditorPlugin.cpp
Source/BlueprintEditor/BehaviorTreeRenderer.h
Source/BlueprintEditor/BehaviorTreeRenderer.cpp
```

**Archive:**

```bash
mkdir -p Archive/BehaviorTree_Editor/
mv Source/AI/BehaviorTreeDebugWindow* Archive/BehaviorTree_Editor/
mv Source/BlueprintEditor/BehaviorTreeEditorPlugin* Archive/BehaviorTree_Editor/
mv Source/BlueprintEditor/BehaviorTreeRenderer* Archive/BehaviorTree_Editor/
```

**Replacement:** Use DebugController + VisualScriptEditorPanel

**Tests:**
- Editor: VS Graph debugging works

**Duration:** 2 days

---

#### P4.2: Archive BehaviorTree Core

**Files to Archive:**

```
Source/AI/BehaviorTree.h
Source/AI/BehaviorTree.cpp
Source/AI/BehaviorTreeDependencyScanner.h
Source/AI/BehaviorTreeDependencyScanner.cpp
Source/NodeGraphShared/BehaviorTreeAdapter.h
```

**Archive:**

```bash
mkdir -p Archive/BehaviorTree_Core/
mv Source/AI/BehaviorTree* Archive/BehaviorTree_Core/
mv Source/NodeGraphShared/BehaviorTreeAdapter.h Archive/BehaviorTree_Core/
```

**Note:** BehaviorTreeAdapter might be useful for migration → consider creating BT→ATS converter

**Duration:** 2 days

---

#### P4.3: Archive BehaviorTreeManager

**Files to Archive:**

```
Source/AI/BehaviorTreeManager.h
Source/AI/BehaviorTreeManager.cpp
```

**Archive:**

```bash
mv Source/AI/BehaviorTreeManager* Archive/BehaviorTree_Manager/
```

**Replacement:** AssetManager already loads .ats (TaskGraphTemplate)

**Duration:** 1 day

---

## CALENDRIER DÉTAILLÉ

### 📅 Timeline Globale

```
PHASE 0 (Préalables)     : 2026-03-31 → 2026-04-04 (5 jours)
PHASE 1 (Foundation)     : 2026-04-07 → 2026-04-11 (6 jours)
PHASE 2 (Systems)        : 2026-04-14 → 2026-04-22 (7 jours)
PHASE 3 (Motion)         : 2026-04-23 → 2026-04-29 (6 jours)
PHASE 4 (Cleanup)        : 2026-04-30 → 2026-05-06 (5 jours)
────────────────────────────────────
TOTAL                    : 2026-03-31 → 2026-05-06 (29 jours = 6-7 semaines)
```

### Par Jour

**Semaine 0 (Phase 0) - 2026-03-31 → 2026-04-04**

| Jour | Tâche | Durée | Responsable | Status |
|------|-------|-------|-------------|--------|
| Lun 31 | P0.1 EventToBlackboardBridge (impl) | 1j | @Dev1 | 🟠 |
| Mar 01 | P0.1 Tests + integration | 0.5j | @QA1 | 🟠 |
| Mar 01 | P0.2 SensorSystem (impl) | 1j | @Dev2 | 🟠 |
| Mer 02 | P0.2 SensorComponent + tests | 1j | @QA1 | 🟠 |
| Jeu 03 | P0.3 AI_MoveTask (impl) | 1.5j | @Dev3 | 🟠 |
| Ven 04 | P0.3 Tests + integration | 1.5j | @QA1 | 🟠 |

**Semaine 1 (Phase 1) - 2026-04-07 → 2026-04-11**

| Jour | Tâche | Durée | Responsable | Status |
|------|-------|-------|-------------|--------|
| Lun 07 | P1.1 CompatibilityLayer (impl) | 1j | @Dev1 | 🟠 |
| Lun 07 | P1.1 Tests | 0.5j | @QA1 | 🟠 |
| Mar 08 | P1.2 Archive AIBlackboard_data | 0.5j | @Dev1 | 🟠 |
| Mar 08 | P1.2 Compilation verification | 0.5j | @QA1 | 🟠 |
| Mer 09 | P1.3 Archive AIStimuliSystem (impl) | 1j | @Dev2 | 🟠 |
| Jeu 10 | P1.3 Tests + EventToBlackboardBridge integration | 1j | @QA1 | 🟠 |
| Ven 11 | P1.4 Archive BehaviorTreeRuntime_data | 1j | @Dev3 | 🟠 |

**Semaine 2-3 (Phase 2) - 2026-04-14 → 2026-04-22**

| Jour | Tâche | Durée | Responsable | Status |
|------|-------|-------|-------------|--------|
| Lun 14 | P2.1 SensorSystem registration + NPC setup | 1.5j | @Dev1 | 🟠 |
| Mar 15 | P2.1 Integration testing | 1.5j | @QA1 | 🟠 |
| Mer 16 | P2.2 AI state machine → VS graph (design) | 1j | @Dev2 | 🟠 |
| Jeu 17 | P2.2 VS graph creation + Condition Presets | 1.5j | @Dev2 | 🟠 |
| Ven 18 | P2.2 Tests + regression testing | 1j | @QA1 | 🟠 |
| Lun 21 | P2.3 Archive AIPerceptionSystem | 1j | @Dev3 | 🟠 |
| Mar 22 | P2.3 Compilation + integration verification | 1j | @QA1 | 🟠 |

**Semaine 3-4 (Phase 3) - 2026-04-23 → 2026-04-29**

| Jour | Tâche | Durée | Responsable | Status |
|------|-------|-------|-------------|--------|
| Mer 23 | P3.1 AI_MoveTask registration + tests | 1.5j | @Dev1 | 🟠 |
| Jeu 24 | P3.1 VS graph integration | 1.5j | @Dev1 | 🟠 |
| Ven 25 | P3.1 Integration testing | 1j | @QA1 | 🟠 |
| Lun 28 | P3.2 BehaviorTreeSystem archive + BT→ATS converter | 2j | @Dev2 | 🟠 |
| Mar 29 | P3.2 Migration testing (existing BT trees) | 1j | @QA1 | 🟠 |

**Semaine 4-5 (Phase 4) - 2026-04-30 → 2026-05-06**

| Jour | Tâche | Durée | Responsable | Status |
|------|-------|-------|-------------|--------|
| Mer 30 | P4.1 Remove BehaviorTreeDebugWindow + editor plugins | 2j | @Dev1 | 🟠 |
| Ven 02 | P4.1 Compilation + verification | 0.5j | @QA1 | 🟠 |
| Lun 05 | P4.2 Archive BehaviorTree core | 2j | @Dev2 | 🟠 |
| Mar 06 | P4.3 Archive BehaviorTreeManager | 1j | @Dev3 | 🟠 |
| Mer 07 | FINAL: Build verification + documentation | 1j | Team | 🟠 |

---

## DÉPENDANCES ET RISQUES

### 🔗 Graphe de Dépendances

```
P0.1: EventToBlackboardBridge ──┐
                                 ├─→ P1.3: Archive AIStimuliSystem
                                 │
P0.2: SensorSystem ──────────────├─→ P2.1: SensorSystem Integration
                                 │
P0.3: AI_MoveTask ───────────────├─→ P3.1: AI_MoveTask Integration
                                 │
P1.1: CompatibilityLayer ────────┼─→ P1.2: Archive AIBlackboard_data
                                 │
P1.2 + P1.3 + P1.4 (Phase 1) ───┴─→ P2 (Phase 2) ──→ P3 (Phase 3) ──→ P4 (Phase 4)
```

### ⚠️ Risques Identifiés

#### RISQUE 1: EventToBlackboardBridge Design (MOYEN)
- **Problème:** Events peuvent avoir structures différentes
- **Mitigation:** Validation stricte des mappings + tests unitaires
- **Tests:** `EventToBlackboardBridgeTests.cpp`

#### RISQUE 2: SensorSystem Performance (MOYEN)
- **Problème:** Raycast + spatial queries peuvent être lents
- **Mitigation:** Timeslicing (5 Hz perception), spatial partitioning
- **Tests:** Benchmark SensorSystem::Process() avec 100 entities

#### RISQUE 3: AI_MoveTask Pathfinding Integration (MOYEN)
- **Problème:** Pathfinding API peut ne pas être mature
- **Mitigation:** Fallback naive movement, comprehensive tests
- **Tests:** Unit tests for pathfinding integration

#### RISQUE 4: BehaviorTree Migration (ÉLEVÉ)
- **Problème:** Existing BT trees dans projects clients
- **Mitigation:** BehaviorTreeAdapter conversion tool
- **Tests:** Converter validation for all existing BT formats
- **Action:** Create BT→ATS converter tool in P3.2

#### RISQUE 5: Compatibility Layer Incomplete (MOYEN)
- **Problème:** Code legacy peut avoir APIs non mappées
- **Mitigation:** Comprehensive AIBlackboard_data→LocalBB mapping
- **Tests:** Grep all uses of AIBlackboard_data in codebase

#### RISQUE 6: GlobalBlackboard vs LocalBlackboard Confusion (FAIBLE)
- **Problème:** State transitions might use GlobalBlackboard
- **Mitigation:** Clear guidelines in P2.2 graph design
- **Tests:** Document GlobalBB usage vs LocalBB usage

---

## CHECKLIST D'EXÉCUTION

### Phase 0 Checklist

- [ ] P0.1: EventToBlackboardBridge.h/cpp created
  - [ ] Unit tests pass
  - [ ] Integration with EventQueue verified
  - [ ] LocalBlackboard writes confirmed

- [ ] P0.2: SensorSystem created
  - [ ] SensorSystem.h/cpp implemented
  - [ ] SensorComponent defined
  - [ ] Vision cone + raycast working
  - [ ] Unit tests pass (vision, hearing)
  - [ ] Performance acceptable (timesliced)

- [ ] P0.3: AI_MoveTask created
  - [ ] AI_MoveTask.h/cpp implemented
  - [ ] Pathfinding integration done
  - [ ] Movement following path works
  - [ ] Unit tests pass
  - [ ] Integration with VSGraphExecutor done

- [ ] **Phase 0 APPROVAL:** All three components tested + ready

---

### Phase 1 Checklist

- [ ] P1.1: CompatibilityLayer created
  - [ ] All AIBlackboard_data fields mapped
  - [ ] Unit tests pass (forwarding)
  - [ ] Existing code compiles

- [ ] P1.2: AIBlackboard_data archived
  - [ ] Component definition moved to Archive/
  - [ ] Stub created with deprecation warning
  - [ ] Compilation verified

- [ ] P1.3: AIStimuliSystem archived
  - [ ] EventToBlackboardBridge registered
  - [ ] All mappings (Damage, Noise, Detection) working
  - [ ] Unit tests pass
  - [ ] Integration tests: AI reacts to stimuli

- [ ] P1.4: BehaviorTreeRuntime_data archived
  - [ ] Fields migrated to TaskRunnerComponent
  - [ ] Compilation verified
  - [ ] Existing behavior unchanged

- [ ] **Phase 1 APPROVAL:** Legacy data layer removed, compatibility verified

---

### Phase 2 Checklist

- [ ] P2.1: SensorSystem registered
  - [ ] SensorSystem::Process() called every frame
  - [ ] NPC prefabs have SensorComponent
  - [ ] LocalBlackboard outputs (perceptionVisibleTargets) verified
  - [ ] Integration tests pass (perception behavior)
  - [ ] Regression tests: Guard AI unchanged

- [ ] P2.2: State transitions → VS Graph
  - [ ] ai_state_transitions.ats graph created
  - [ ] Branch nodes + Condition Presets used
  - [ ] BBKey writes (aiMode) correct
  - [ ] All state transitions working
  - [ ] Regression tests pass

- [ ] P2.3: AIPerceptionSystem archived
  - [ ] System removed from ECS_Systems_AI.h/cpp
  - [ ] World registration removed
  - [ ] Compilation verified
  - [ ] SensorSystem fully replaces perception

- [ ] **Phase 2 APPROVAL:** AI perception + state transitions working via new systems

---

### Phase 3 Checklist

- [ ] P3.1: AI_MoveTask registered + integrated
  - [ ] Task registered in AtomicTaskRegistry
  - [ ] VS graph nodes use "AI_Move"
  - [ ] Pathfinding + movement working
  - [ ] Unit tests pass
  - [ ] Regression tests: AI motion unchanged

- [ ] P3.2: BehaviorTreeSystem archived
  - [ ] System removed
  - [ ] World registration removed
  - [ ] TaskSystem runs VSGraphExecutor for AI entities
  - [ ] BehaviorTreeAdapter used for BT→ATS conversion
  - [ ] All existing BT trees converted + tested
  - [ ] Guard AI exhibits identical behavior

- [ ] **Phase 3 APPROVAL:** BT system fully migrated to TaskSystem

---

### Phase 4 Checklist

- [ ] P4.1: BehaviorTreeDebugWindow archived
  - [ ] Debug window removed
  - [ ] BehaviorTreeEditorPlugin archived
  - [ ] BehaviorTreeRenderer archived
  - [ ] DebugController + VisualScriptEditorPanel verified

- [ ] P4.2: BehaviorTree core archived
  - [ ] BehaviorTree.h/cpp archived
  - [ ] BehaviorTreeDependencyScanner archived
  - [ ] BehaviorTreeAdapter archived (or integrated)

- [ ] P4.3: BehaviorTreeManager archived
  - [ ] Manager removed
  - [ ] AssetManager handles .ats graphs

- [ ] **Phase 4 APPROVAL:** All BT editor tooling removed

---

### Final Verification Checklist

- [ ] **Build:** Clean build passes (no errors/warnings)
- [ ] **Compilation:** C++14 strict mode, zero warnings
- [ ] **Regression Tests:** All existing AI behaviors identical
- [ ] **Performance:** 50ms/frame improvement verified
- [ ] **Documentation:** All docs updated (P2.2, P4.3)
- [ ] **Archive:** All legacy files properly archived
- [ ] **References:** All #includes updated (no stale references)
- [ ] **Project File:** All source files removed from project

---

## RÉSUMÉ COMPARATIF

### Document Original vs Plan Révisé

| Aspect | Original | Révisé | Impact |
|--------|----------|--------|--------|
| **Phase 0** | ❌ Absent | ✅ 5-6 jours | CRITIQUE (briques manquantes) |
| **EventToBlackboardBridge** | Supposé exist | À créer P0.1 | BLOQUEUR |
| **SensorSystem** | Supposé exist | À créer P0.2 | BLOQUEUR |
| **AI_MoveTask** | Supposé exist | À créer P0.3 | BLOQUEUR |
| **Timeline** | 26 jours | 29-31 jours | +3-5 jours |
| **Phase 24** | Non mentionné | Leverage dans P2.2 | POSITIF |
| **BT Editor cleanup** | P4.1 | P4.1 + addenda | +2 fichiers |
| **Risk Mitigation** | Basique | Détaillée | IMPORTANT |

### Économies Attendues

| Métrique | Avant | Après | Gain |
|----------|-------|-------|------|
| LOC Systems | ~1660 | 0 | -1660 |
| LOC Components | ~50 defs | 0 | -50 |
| LOC Hardcoding | ~1700 | 0 | -1700 |
| LOC Documentation | ~3000 | ~1500 | -1500 |
| **TOTAL** | ~7710 | ~1500 | -6210 |
| **Performance** | ~50ms/frame AI overhead | ~0ms (async sensors) | +50ms |

---

## NEXT STEPS

1. ✅ **Approval:** Valider ce plan avec @Atlasbruce
2. ✅ **Create Phase 0:** Start implementing EventToBlackboardBridge (P0.1)
3. ✅ **Run Tests:** Verify all P0 components
4. ✅ **Begin Phase 1:** Once P0 complete

---

**Document:** `Project Management/Features/cleaning_redundancy_systems_27-03-2026.md`  
**Status:** READY FOR IMPLEMENTATION  
**Next Review:** 2026-04-04 (End of Phase 0)
