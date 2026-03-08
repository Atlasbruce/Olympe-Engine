# ATS Visual Scripting - Document Technique d'Implémentation
## Guide Complet pour Olympe Engine

**Version 1.0 - 08/03/2026**

---

## Table des matières

1. [Vue d'Ensemble du Système](#1-vue-densemble-du-système)
2. [Fondements Conceptuels](#2-fondements-conceptuels)
3. [Architecture d'Intégration Olympe Engine](#3-architecture-dintégration-olympe-engine)
4. [Structures de Données C++](#4-structures-de-données-c)
5. [Runtime et Exécution](#5-runtime-et-exécution)
6. [Blackboard et Composants ECS](#6-blackboard-et-composants-ecs)
7. [Cas d'Usage : IA Entités](#7-cas-dusage-ia-entités)
8. [Cas d'Usage : Scripts Système et Game Rules](#8-cas-dusage-scripts-système-et-game-rules)
9. [Cas d'Usage : Cinématiques et Level Scripts](#9-cas-dusage-cinématiques-et-level-scripts)
10. [Actions Système Complètes](#10-actions-système-complètes)
11. [Plan d'Implémentation](#11-plan-dimplémentation)
12. [Tests et Validation](#12-tests-et-validation)

---

## 1. Vue d'Ensemble du Système

### 1.1. Qu'est-ce que ATS Visual Scripting ?

**ATS Visual Scripting** est un système de **programmation visuelle algorithmique** intégré à Olympe Engine permettant de créer :

- **Comportements IA** pour entités (gardes, ennemis, NPCs)
- **Scripts de niveau** (triggers, portes, puzzles, séquences)
- **Game rules** (systèmes de score, conditions victoire/défaite, spawn waves)
- **Cinématiques** (séquences scriptées, cutscenes, dialogues)

**Paradigme** : Flowchart avec **flux Exec visuel** + **data flow typé** (inspiré Blueprint UE).

### 1.2. Positionnement dans Olympe Engine

```
Olympe Engine Architecture
├─ ECS Core (World, Entities, Components, Systems)
├─ Blueprint System (EntityPrefab, BehaviorTree JSON)
├─ Input System (InputsManager, Controller bindings)
├─ Data Manager (Assets, Resources)
└─ **ATS Visual Scripting** ← NOUVEAU
      ├─ Visual Script Assets (JSON)
      ├─ Runtime Executor (System ECS)
      ├─ Blackboard Bridge (ECS Components)
      └─ Editor Integration (Node Graph)
```

**Différence clé avec Behavior Tree** :

| Aspect | **BehaviorTree** | **ATS Visual Scripting** |
|--------|------------------|--------------------------|
| Structure | Hiérarchie parent→enfant fixe | Graphe flux libre Exec |
| Conditionnelles | Implicites (Selector) | Explicites (Branch True/False) |
| Boucles | Pas de support | While/For natifs |
| Data flow | Blackboard uniquement | Data pins typées + Blackboard |
| Usage | IA entité pure | IA + Level + Systèmes + Cinématiques |

---

## 2. Fondements Conceptuels

### 2.1. Modèle d'Exécution : Event Tick Loop

**Chaque frame, pour chaque entité avec graphe ATS actif** :

```
Frame N :
┌──────────────────────────────────────┐
│ 1. Event Tick déclenché              │
│    ↓                                  │
│ 2. Parcourt graphe depuis EntryPoint │
│    ↓                                  │
│ 3. Suit connexions Exec séquentiellement │
│    (Branch → True/False, While → Loop/Exit) │
│    ↓                                  │
│ 4. Exécute Actions (GotoPos, SetBB, etc.) │
│    ↓                                  │
│ 5. Fin flux (plus de connexions Exec) │
│    ↓                                  │
│ 6. Reboucle EntryPoint frame N+1     │
└──────────────────────────────────────┘

Interruptions :
- DoOnce : bloque flux après 1ère exécution
- Delay : pause flux X secondes (timer)
- Entity détruite : stop définitif
```

**Exemple concret** :

```
Guard AI chaque frame :
EntryPoint → GetBBValue("TargetDistance")
          → Branch(distance <= 60?)
              ├─ True → Attack sequence
              └─ False → Patrol waypoints
→ Fin → Reboucle prochain frame
```

### 2.2. Flux Exec vs Data Flow

#### Flux Exec (Exécution)

**Visuel** : Lignes blanches/bleues épaisses.
**Rôle** : Ordre d'exécution des nodes.

```
EntryPoint ───Exec──→ Action1 ───Exec──→ Branch
                                    ├─True──→ Action2
                                    └─False─→ Action3
```

#### Data Flow (Données)

**Visuel** : Lignes colorées fines (type = couleur).
**Rôle** : Transporte valeurs entre nodes.

```
GetBBValue("Health") ──float──→ Branch.left
                                (compare Health < 30)
```

**Types Data** :
- `bool` (rouge), `int` (cyan), `float` (vert)
- `string` (magenta), `vector2/3` (jaune/orange)
- `entity_handle` (blanc)

### 2.3. Blackboard : Pont avec ECS

**Blackboard** = dictionnaire clé-valeur pour état IA/entité.

**Deux scopes** :
- **Local** (`local:Key`) : Composants entité (AIState, Health, MoveIntent)
- **Global** (`global:Key`) : État monde/mission (phase, alert level)

**Mapping ECS automatique** :

| Clé Blackboard | Composant ECS | Exemple |
|----------------|---------------|---------|
| `local:CurrentState` | `AIStatedata::currentState` | "Patrol" |
| `local:Health` | `Healthdata::currentHealth` | 80.0 |
| `local:TargetActor` | `AIBlackboarddata::targetActor` | EntityID(42) |
| `local:MoveGoal` | `MoveIntentdata::target{X,Y}` | {500, 300} |
| `global:GamePhase` | `GlobalBlackboard["GamePhase"]` | "Combat" |

---

## 3. Architecture d'Intégration Olympe Engine

### 3.1. Composants ECS Existants (Réutilisation)

**Olympe Engine dispose déjà de composants ECS que ATS Visual Scripting va utiliser** :

```cpp
// SourceECSComponents.h (existant)

// Position 2D/3D
struct Positiondata {
    Vector3 position;  // x, y, z
};

// Boîte de collision
struct BoundingBoxdata {
    float x, y, width, height;
    float offsetX, offsetY;
};

// Sprite visuel
struct VisualSpritedata {
    std::string spritePath;
    float width, height;
    int layer;
};

// Mouvement
struct Movementdata {
    float speed;
    float acceleration;
    Vector2 direction;
};

// Santé
struct Healthdata {
    float currentHealth;
    float maxHealth;
};

// IA Blackboard (données custom)
struct AIBlackboarddata {
    bool initialized;
    EntityID targetActor;  // Cible détectée
    std::string patrolRoute;
    // Extensible avec map<string, variant>
};

// IA State
struct AIStatedata {
    std::string currentState;  // "Patrol", "Combat", "Flee"
    std::string previousState;
};

// IA Sensors
struct AISensesdata {
    float visionRange;
    float hearingRange;
    float alertLevel;
};

// Animation
struct Animationdata {
    std::string animationID;
    int currentFrame;
    float frameDuration;
    float elapsedTime;
};

// Audio
struct AudioSourcedata {
    std::string soundEffectID;
    float volume;
};

// Identity
struct Identitydata {
    std::string name;
    std::string tag;
    std::string entityType;
};
```

**Ces composants sont déjà utilisés dans Olympe** → ATS Visual Scripting les réutilise via Blackboard.

### 3.2. Nouveaux Composants ATS

**Ajouts nécessaires** :

```cpp
// SourceECSComponents.h (NOUVEAU)

// Référence au graphe Visual Script à exécuter
struct VisualScriptComponent {
    std::string scriptAssetPath;  // "Blueprints/VisualScript/guard_ai.json"
    bool enabled;
    bool runOnce;  // Si true, exécute 1 fois puis désactive
};

// État runtime du script (pointeur vers instance)
struct VisualScriptRuntimeComponent {
    void* runtimeInstance;  // Pointeur opaque vers VSRuntimeInstance
};

// Intent de mouvement (utilisé par Actions)
struct MoveIntentdata {
    float targetX, targetY;
    float speed;
    float tolerance;  // Distance arrêt
    bool active;
};

// Timer/Delay runtime
struct DelayTimerComponent {
    float duration;
    float elapsed;
    bool active;
    int nodeId;  // Node Delay source
};
```

### 3.3. Système ECS : VisualScriptSystem

**Nouveau système à créer** :

```cpp
// SourceSystemsVisualScriptSystem.h

class VisualScriptSystem {
public:
    void Initialize();
    void Update(float deltaTime);

private:
    void ExecuteEntityScripts(float deltaTime);
    void UpdateDelayTimers(float deltaTime);

    // Exécution d'un graphe
    void ExecuteGraph(EntityID entity, VSRuntimeInstance* runtime);

    // Exécution récursive nodes
    void ExecuteNode(EntityID entity, VSNode* node, VSRuntimeInstance* runtime);

    // Évaluation conditions
    bool EvaluateCondition(EntityID entity, const VSCondition& cond);

    // Actions
    void ExecuteAction(EntityID entity, const VSAction& action);
};
```

**Intégration dans GameEngine** :

```cpp
// SourceOlympeEngine.cpp

void GameEngine::Initialize() {
    // ... systèmes existants ...

    // NOUVEAU : Système Visual Scripting
    m_visualScriptSystem = std::make_unique<VisualScriptSystem>();
    m_visualScriptSystem->Initialize();
}

void GameEngine::Update(float deltaTime) {
    // ... update input, physics, etc. ...

    // NOUVEAU : Update Visual Scripts
    m_visualScriptSystem->Update(deltaTime);

    // ... render, etc. ...
}
```

### 3.4. Asset Manager : Chargement JSON

**Extension DataManager** :

```cpp
// SourceDataManager.h

class DataManager {
public:
    // NOUVEAU
    VSGraphAsset* LoadVisualScript(const std::string& path);
    void UnloadVisualScript(const std::string& path);

private:
    std::unordered_map<std::string, VSGraphAsset*> m_visualScripts;
};
```

**Chargement JSON** :

```cpp
VSGraphAsset* DataManager::LoadVisualScript(const std::string& path) {
    // Parse JSON via nlohmann::json (déjà utilisé Olympe)
    std::ifstream file(path);
    json j;
    file >> j;

    VSGraphAsset* graph = new VSGraphAsset();
    graph->name = j["name"];
    graph->type = j["type"];  // "VisualScript"

    // Parse nodes
    for (auto& nodeJson : j["nodes"]) {
        VSNode node;
        node.id = nodeJson["id"];
        node.type = nodeJson["type"];  // "Branch", "Action", etc.
        // ... parse params, execOut, dataOut ...
        graph->nodes.push_back(node);
    }

    // Parse connections
    for (auto& connJson : j["connections"]) {
        VSConnection conn;
        // ... parse from/to ...
        graph->connections.push_back(conn);
    }

    m_visualScripts[path] = graph;
    return graph;
}
```

---

## 4. Structures de Données C++

### 4.1. Asset (JSON parsé)

```cpp
// SourceVisualScriptVSGraphAsset.h

enum class VSNodeType {
    EntryPoint,
    Branch,
    Switch,
    Sequence,
    While,
    ForEach,
    DoOnce,
    Delay,
    Action,
    GetBBValue,
    SetBBValue,
    Math
};

enum class VSConditionType {
    CompareValue,
    IsSet,
    IsNotSet,
    InRange,
    StateIs
};

enum class VSActionType {
    GotoPosition,
    ChangeState,
    SetMoveGoalToTarget,
    AttackIfClose,
    GetNextWaypoint,
    PlayAnimation,
    SetSprite,
    PlaySound,
    StopSound,
    LoadLevel,
    DestroyEntity,
    LogMessage,
    AddPlayer,
    CameraShake
};

struct VSParameter {
    std::string name;
    std::variant<int, float, bool, std::string, Vector2> value;
};

struct VSCondition {
    VSConditionType type;
    std::vector<VSParameter> parameters;
};

struct VSNode {
    int id;
    VSNodeType type;
    std::string name;
    Vector2 position;  // Éditeur

    // Flux Exec
    std::vector<int> execOut;  // IDs nodes suivants (simple action)
    std::unordered_map<std::string, std::vector<int>> execOutBranch;  // Branch {"true":[4], "false":[10]}

    // Data pins
    std::vector<std::string> dataOut;  // Noms pins sortie
    std::unordered_map<std::string, int> dataIn;  // {"left": sourceNodeId}

    // Params nodes
    std::vector<VSParameter> parameters;
    VSCondition condition;  // Branch/While
    VSActionType actionType;  // Action
    std::string key;  // GetBBValue/SetBBValue
};

struct VSConnection {
    std::string from;  // "node4.dataOut.distance"
    std::string to;    // "node5.dataIn.left"
};

struct VSGraphAsset {
    std::string name;
    std::string type;
    std::vector<VSNode> nodes;
    std::vector<VSConnection> connections;
    int entryPointId;
};
```

### 4.2. Runtime Instance

```cpp
// SourceVisualScriptVSRuntimeInstance.h

struct VSRuntimeInstance {
    VSGraphAsset* graph;
    EntityID ownerEntity;

    // Exécution
    int currentNodeId;
    bool running;

    // DoOnce tracking
    std::unordered_set<int> doOnceExecuted;  // IDs nodes DoOnce déjà passés

    // Data cache (pins Data connectées)
    std::unordered_map<std::string, std::variant<int, float, bool, std::string, Vector2>> dataCache;

    // Delay timers actifs
    std::vector<int> activeDelays;  // IDs nodes Delay en cours
};
```

---

## 5. Runtime et Exécution

### 5.1. VisualScriptSystem::Update

```cpp
void VisualScriptSystem::Update(float deltaTime) {
    // 1. Update delay timers
    UpdateDelayTimers(deltaTime);

    // 2. Exécute scripts entités
    ExecuteEntityScripts(deltaTime);
}

void VisualScriptSystem::ExecuteEntityScripts(float deltaTime) {
    World& world = World::Get();

    // Itère toutes entités avec VisualScriptComponent
    for (EntityID entity : world.EntitiesWithComponent<VisualScriptComponent>()) {
        auto* scriptComp = world.GetComponent<VisualScriptComponent>(entity);
        if (!scriptComp->enabled) continue;

        // Récupère runtime instance (ou crée si première frame)
        auto* runtimeComp = world.GetComponent<VisualScriptRuntimeComponent>(entity);
        if (!runtimeComp) {
            // Première exécution : load graph + crée runtime
            VSGraphAsset* graph = DataManager::Get().LoadVisualScript(scriptComp->scriptAssetPath);

            VSRuntimeInstance* runtime = new VSRuntimeInstance();
            runtime->graph = graph;
            runtime->ownerEntity = entity;
            runtime->currentNodeId = graph->entryPointId;
            runtime->running = true;

            VisualScriptRuntimeComponent rtComp;
            rtComp.runtimeInstance = runtime;
            world.AddComponent<VisualScriptRuntimeComponent>(entity, rtComp);

            runtimeComp = world.GetComponent<VisualScriptRuntimeComponent>(entity);
        }

        VSRuntimeInstance* runtime = static_cast<VSRuntimeInstance*>(runtimeComp->runtimeInstance);

        // Event Tick : exécute graphe depuis EntryPoint
        ExecuteGraph(entity, runtime);

        // RunOnce : désactive après 1 frame
        if (scriptComp->runOnce) {
            scriptComp->enabled = false;
        }
    }
}
```

### 5.2. ExecuteGraph : Parcours Récursif

```cpp
void VisualScriptSystem::ExecuteGraph(EntityID entity, VSRuntimeInstance* runtime) {
    VSGraphAsset* graph = runtime->graph;

    // Start from EntryPoint
    int nodeId = graph->entryPointId;

    // Parcourt flux Exec jusqu'à fin
    while (nodeId != -1) {
        VSNode* node = FindNodeById(graph, nodeId);
        if (!node) break;

        int nextNodeId = ExecuteNode(entity, node, runtime);
        nodeId = nextNodeId;
    }
}

int VisualScriptSystem::ExecuteNode(EntityID entity, VSNode* node, VSRuntimeInstance* runtime) {
    switch (node->type) {
        case VSNodeType::EntryPoint:
            // Simple : retourne execOut[0]
            return node->execOut.empty() ? -1 : node->execOut[0];

        case VSNodeType::Branch: {
            // Évalue condition
            bool result = EvaluateCondition(entity, node->condition);
            std::string path = result ? "true" : "false";
            auto it = node->execOutBranch.find(path);
            if (it != node->execOutBranch.end() && !it->second.empty()) {
                return it->second[0];
            }
            return -1;
        }

        case VSNodeType::Sequence: {
            // Exécute then0, then1, ... dans l'ordre
            for (auto& kv : node->execOutBranch) {
                if (kv.first.starts_with("then") && !kv.second.empty()) {
                    ExecuteNodeById(entity, kv.second[0], runtime);
                }
            }
            return -1;
        }

        case VSNodeType::While: {
            // Boucle tant que condition vraie
            while (EvaluateCondition(entity, node->condition)) {
                auto it = node->execOutBranch.find("loop");
                if (it != node->execOutBranch.end() && !it->second.empty()) {
                    ExecuteNodeById(entity, it->second[0], runtime);
                } else break;
            }
            // Condition fausse → Exit
            auto exitIt = node->execOutBranch.find("exit");
            if (exitIt != node->execOutBranch.end() && !exitIt->second.empty()) {
                return exitIt->second[0];
            }
            return -1;
        }

        case VSNodeType::DoOnce: {
            if (runtime->doOnceExecuted.count(node->id)) {
                return -1;  // Déjà exécuté
            }
            runtime->doOnceExecuted.insert(node->id);
            return node->execOut.empty() ? -1 : node->execOut[0];
        }

        case VSNodeType::Delay: {
            // Crée timer component
            float duration = std::get<float>(GetParam(node, "duration").value);

            DelayTimerComponent timer;
            timer.duration = duration;
            timer.elapsed = 0.0f;
            timer.active = true;
            timer.nodeId = node->id;

            World::Get().AddComponent<DelayTimerComponent>(entity, timer);

            // Bloque flux (completed pin activé dans UpdateDelayTimers)
            return -1;
        }

        case VSNodeType::Action: {
            ExecuteAction(entity, node);
            return node->execOut.empty() ? -1 : node->execOut[0];
        }

        case VSNodeType::GetBBValue: {
            // Lit Blackboard → stocke dans dataCache
            std::string key = node->key;
            auto value = ReadBlackboard(entity, key);

            // Stocke pour data pins
            std::string dataOutKey = "node" + std::to_string(node->id) + ".dataOut." + node->dataOut[0];
            runtime->dataCache[dataOutKey] = value;

            return node->execOut.empty() ? -1 : node->execOut[0];
        }

        case VSNodeType::SetBBValue: {
            // Écrit dataIn → Blackboard
            std::string key = node->key;

            // Récupère valeur depuis data pin connectée
            auto dataInKey = "node" + std::to_string(node->dataIn["value"]) + ".dataOut";
            auto value = runtime->dataCache[dataInKey];

            WriteBlackboard(entity, key, value);

            return node->execOut.empty() ? -1 : node->execOut[0];
        }

        default:
            return -1;
    }
}
```

### 5.3. EvaluateCondition

```cpp
bool VisualScriptSystem::EvaluateCondition(EntityID entity, const VSCondition& cond) {
    switch (cond.type) {
        case VSConditionType::CompareValue: {
            // Params : left, operator, right
            auto left = ResolveValue(entity, GetCondParam(cond, "left"));
            auto op = std::get<std::string>(GetCondParam(cond, "operator").value);
            auto right = ResolveValue(entity, GetCondParam(cond, "right"));

            // Compare selon type (float ici)
            float leftVal = std::get<float>(left);
            float rightVal = std::get<float>(right);

            if (op == "==") return leftVal == rightVal;
            if (op == "!=") return leftVal != rightVal;
            if (op == "<") return leftVal < rightVal;
            if (op == "<=") return leftVal <= rightVal;
            if (op == ">") return leftVal > rightVal;
            if (op == ">=") return leftVal >= rightVal;

            return false;
        }

        case VSConditionType::IsSet: {
            std::string key = std::get<std::string>(GetCondParam(cond, "key").value);
            return IsBlackboardKeySet(entity, key);
        }

        case VSConditionType::InRange: {
            std::string key = std::get<std::string>(GetCondParam(cond, "key").value);
            float value = std::get<float>(ReadBlackboard(entity, key));
            float min = std::get<float>(GetCondParam(cond, "min").value);
            float max = std::get<float>(GetCondParam(cond, "max").value);
            return value >= min && value <= max;
        }

        default:
            return false;
    }
}
```

### 5.4. UpdateDelayTimers

```cpp
void VisualScriptSystem::UpdateDelayTimers(float deltaTime) {
    World& world = World::Get();

    for (EntityID entity : world.EntitiesWithComponent<DelayTimerComponent>()) {
        auto* timer = world.GetComponent<DelayTimerComponent>(entity);
        if (!timer->active) continue;

        timer->elapsed += deltaTime;

        if (timer->elapsed >= timer->duration) {
            // Timer expiré → active pin "completed"
            auto* runtimeComp = world.GetComponent<VisualScriptRuntimeComponent>(entity);
            VSRuntimeInstance* runtime = static_cast<VSRuntimeInstance*>(runtimeComp->runtimeInstance);

            // Trouve node Delay
            VSNode* delayNode = FindNodeById(runtime->graph, timer->nodeId);
            if (delayNode && !delayNode->execOut.empty()) {
                // Continue flux depuis completed pin
                int nextNodeId = delayNode->execOut[0];
                ExecuteNodeById(entity, nextNodeId, runtime);
            }

            // Supprime timer
            world.RemoveComponent<DelayTimerComponent>(entity);
        }
    }
}
```

---

## 6. Blackboard et Composants ECS

### 6.1. Bridge Blackboard ↔ Composants

```cpp
// SourceVisualScriptBlackboardBridge.h

class BlackboardBridge {
public:
    // Lit clé Blackboard → variant
    static std::variant<int, float, bool, std::string, Vector2, EntityID>
    ReadBlackboard(EntityID entity, const std::string& key);

    // Écrit variant → clé Blackboard
    static void WriteBlackboard(EntityID entity, const std::string& key,
                                const std::variant<...>& value);

    // Teste si clé définie
    static bool IsBlackboardKeySet(EntityID entity, const std::string& key);

private:
    // Parse scope:path
    static std::pair<std::string, std::string> ParseKey(const std::string& key);
};
```

**Implémentation** :

```cpp
std::variant<...> BlackboardBridge::ReadBlackboard(EntityID entity, const std::string& key) {
    auto [scope, path] = ParseKey(key);  // "local:Health" → {"local", "Health"}

    World& world = World::Get();

    if (scope == "local") {
        // Map clés BB → composants ECS
        if (path == "CurrentState") {
            auto* state = world.GetComponent<AIStatedata>(entity);
            return state ? state->currentState : "";
        }
        else if (path == "Health") {
            auto* health = world.GetComponent<Healthdata>(entity);
            return health ? health->currentHealth : 0.0f;
        }
        else if (path == "TargetActor") {
            auto* bb = world.GetComponent<AIBlackboarddata>(entity);
            return bb ? bb->targetActor : EntityID(0);
        }
        else if (path == "TargetDistance") {
            // Calculé depuis Position + TargetActor position
            auto* bb = world.GetComponent<AIBlackboarddata>(entity);
            if (!bb || bb->targetActor == 0) return 999999.0f;

            auto* pos = world.GetComponent<Positiondata>(entity);
            auto* targetPos = world.GetComponent<Positiondata>(bb->targetActor);
            if (!pos || !targetPos) return 999999.0f;

            float dx = targetPos->position.x - pos->position.x;
            float dy = targetPos->position.y - pos->position.y;
            return sqrtf(dx*dx + dy*dy);
        }
        else if (path == "MoveGoal") {
            auto* intent = world.GetComponent<MoveIntentdata>(entity);
            return intent ? Vector2{intent->targetX, intent->targetY} : Vector2{0,0};
        }
        // ... autres mappings ...
    }
    else if (scope == "global") {
        // Accès GlobalBlackboard singleton
        return GlobalBlackboard::Get().GetValue(path);
    }

    return 0;  // Défaut
}

void BlackboardBridge::WriteBlackboard(EntityID entity, const std::string& key, const std::variant<...>& value) {
    auto [scope, path] = ParseKey(key);

    World& world = World::Get();

    if (scope == "local") {
        if (path == "CurrentState") {
            auto* state = world.GetComponent<AIStatedata>(entity);
            if (state) state->currentState = std::get<std::string>(value);
        }
        else if (path == "Health") {
            auto* health = world.GetComponent<Healthdata>(entity);
            if (health) health->currentHealth = std::get<float>(value);
        }
        else if (path == "MoveGoal") {
            auto* intent = world.GetComponent<MoveIntentdata>(entity);
            if (!intent) {
                MoveIntentdata newIntent;
                world.AddComponent<MoveIntentdata>(entity, newIntent);
                intent = world.GetComponent<MoveIntentdata>(entity);
            }
            Vector2 goal = std::get<Vector2>(value);
            intent->targetX = goal.x;
            intent->targetY = goal.y;
            intent->active = true;
        }
        // ... autres ...
    }
    else if (scope == "global") {
        GlobalBlackboard::Get().SetValue(path, value);
    }
}
```

### 6.2. Global Blackboard Singleton

```cpp
// SourceVisualScriptGlobalBlackboard.h

class GlobalBlackboard {
public:
    static GlobalBlackboard& Get();

    void SetValue(const std::string& key, const std::variant<...>& value);
    std::variant<...> GetValue(const std::string& key);
    bool IsKeySet(const std::string& key);

private:
    std::unordered_map<std::string, std::variant<...>> m_data;
};
```

**Usage** :

```cpp
// Script level : global:GamePhase = "Combat"
GlobalBlackboard::Get().SetValue("GamePhase", std::string("Combat"));

// Tous scripts peuvent lire
std::string phase = std::get<std::string>(GlobalBlackboard::Get().GetValue("GamePhase"));
```

---

## 7. Cas d'Usage : IA Entités

### 7.1. Guard AI : Patrol → Combat → Investigate

**JSON Guard AI** (voir doc complète section 8.2).

**Composants entité** :

```cpp
// Création entité Guard runtime
EntityID guard = World::Get().CreateEntity();

// Position
Positiondata pos;
pos.position = {500, 300, 0};
World::Get().AddComponent<Positiondata>(guard, pos);

// AI State
AIStatedata state;
state.currentState = "Patrol";
World::Get().AddComponent<AIStatedata>(guard, state);

// AI Blackboard
AIBlackboarddata bb;
bb.initialized = true;
bb.patrolRoute = "RouteA";
World::Get().AddComponent<AIBlackboarddata>(guard, bb);

// AI Senses
AISensesdata senses;
senses.visionRange = 150.0f;
senses.hearingRange = 100.0f;
World::Get().AddComponent<AISensesdata>(guard, senses);

// Visual Script
VisualScriptComponent script;
script.scriptAssetPath = "Blueprints/VisualScript/guard_combat.json";
script.enabled = true;
World::Get().AddComponent<VisualScriptComponent>(guard, script);
```

**Exécution chaque frame** :

```
Frame 1 :
EntryPoint → GetBBValue("TargetActor") → Branch(IsSet?)
  └─ False (pas de cible) → Switch(CurrentState)
      └─ "Patrol" → GetNextWaypoint → SetMoveGoal → Move

MoveIntent component mis à jour → MoveSystem déplace entité

Frame 2 :
(joueur détecté par AISensesSystem → targetActor = PlayerID)
EntryPoint → GetBBValue("TargetActor") → Branch(IsSet?)
  └─ True (cible!) → GetBBValue("TargetDistance") → Branch(<= 60?)
      └─ False (loin) → ChangeState("Combat") → SetMoveGoalToTarget → Move

Frame 3...N :
(se rapproche du joueur)
Branch(<= 60?) → True (assez proche) → While(dist > 10) → MoveToGoal
                                        └─ Exit (dist <= 10) → AttackIfClose!
```

### 7.2. Fleeing Enemy (HP < 30%)

**JSON Flee Logic** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"GetBBValue",
      "key":"local:Health",
      "dataOut":[{"name":"health", "type":"float"}],
      "execOut":[3]
    },
    {
      "id":3, "type":"Branch", "name":"Low HP?",
      "conditionType":"CompareValue",
      "dataIn":[{"name":"left", "sourceNode":2, "sourcePin":"health"}],
      "parameters":[
        {"name":"operator", "value":"<"},
        {"name":"right", "value":30.0}
      ],
      "execOut":{"true":[4], "false":[10]}
    },
    {
      "id":4, "type":"Action", "name":"Calculate Flee Direction",
      "actionType":"CalculateFleeDirection",
      "execOut":[5]
    },
    {
      "id":5, "type":"Action", "name":"Move Away",
      "actionType":"MoveToGoal",
      "parameters":[{"name":"speed", "value":1.5}]
    },
    {
      "id":10, "type":"Action", "name":"Normal Combat",
      "actionType":"AttackIfClose"
    }
  ]
}
```

**Action Custom** :

```cpp
case VSActionType::CalculateFleeDirection: {
    World& world = World::Get();
    auto* bb = world.GetComponent<AIBlackboarddata>(entity);
    auto* pos = world.GetComponent<Positiondata>(entity);
    auto* targetPos = world.GetComponent<Positiondata>(bb->targetActor);

    // Direction opposée à cible
    float dx = pos->position.x - targetPos->position.x;
    float dy = pos->position.y - targetPos->position.y;
    float dist = sqrtf(dx*dx + dy*dy);
    dx /= dist;
    dy /= dist;

    // Flee point = position + direction * 200
    auto* intent = world.GetComponent<MoveIntentdata>(entity);
    intent->targetX = pos->position.x + dx * 200.0f;
    intent->targetY = pos->position.y + dy * 200.0f;
    intent->speed = 1.5f;
    intent->active = true;
    break;
}
```

### 7.3. Dialogue NPC

**JSON Dialogue** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"Branch", "name":"Dialogue Active?",
      "conditionType":"IsSet",
      "parameters":[{"name":"key", "value":"local:DialogueActive"}],
      "execOut":{"true":[3], "false":[]}
    },
    {
      "id":3, "type":"GetBBValue",
      "key":"local:CurrentLineIndex",
      "dataOut":[{"name":"index", "type":"int"}],
      "execOut":[4]
    },
    {
      "id":4, "type":"Branch", "name":"More Lines?",
      "conditionType":"CompareValue",
      "dataIn":[{"name":"left", "sourceNode":3, "sourcePin":"index"}],
      "parameters":[
        {"name":"operator", "value":"<"},
        {"name":"right", "value":5}
      ],
      "execOut":{"true":[5], "false":[10]}
    },
    {
      "id":5, "type":"Action", "name":"Display Line",
      "actionType":"DisplayDialogueLine",
      "execOut":[6]
    },
    {
      "id":6, "type":"Delay", "duration":3.0, "execOut":[7]},
    {
      "id":7, "type":"Action", "name":"Increment Line",
      "actionType":"IncrementDialogueIndex"
    },
    {
      "id":10, "type":"SetBBValue",
      "key":"local:DialogueActive",
      "dataIn":[{"name":"value", "literal":false}]
    }
  ]
}
```

**Actions** :

```cpp
case VSActionType::DisplayDialogueLine: {
    auto* bb = world.GetComponent<AIBlackboarddata>(entity);
    int index = bb->dialogueIndex;  // Supposons ajouté à AIBlackboard

    std::string line = GetDialogueLine(entity, index);

    // Affiche UI dialogue (via UI system)
    UIManager::Get().ShowDialogue(entity, line);
    break;
}

case VSActionType::IncrementDialogueIndex: {
    auto* bb = world.GetComponent<AIBlackboarddata>(entity);
    bb->dialogueIndex++;
    break;
}
```

---

## 8. Cas d'Usage : Scripts Système et Game Rules

### 8.1. Spawn Wave System

**Entité "WaveManager" (pas NPC, juste controller)** :

```cpp
EntityID waveManager = World::Get().CreateEntity();

Identitydata id;
id.name = "WaveManager";
id.entityType = "SystemController";
World::Get().AddComponent<Identitydata>(waveManager, id);

// Blackboard pour waves
AIBlackboarddata bb;
bb.initialized = true;
// Custom fields (extend struct ou use map)
// bb.customData["WaveNumber"] = 0;
// bb.customData["WaveList"] = {"Easy", "Medium", "Hard"};
World::Get().AddComponent<AIBlackboarddata>(waveManager, bb);

VisualScriptComponent script;
script.scriptAssetPath = "Blueprints/VisualScript/spawn_waves.json";
script.enabled = true;
World::Get().AddComponent<VisualScriptComponent>(waveManager, script);
```

**JSON Spawn Waves** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"ForEach", "name":"For Each Wave",
      "arrayKey":"local:WaveList",
      "execOut":{"loopBody":[3], "completed":[10]},
      "dataOut":[
        {"name":"currentItem", "type":"string"},
        {"name":"currentIndex", "type":"int"}
      ]
    },
    {
      "id":3, "type":"Action", "name":"Spawn Wave",
      "actionType":"SpawnWave",
      "dataIn":[{"name":"waveType", "sourceNode":2, "sourcePin":"currentItem"}],
      "execOut":[4]
    },
    {
      "id":4, "type":"Delay", "duration":10.0, "execOut":[2]},
    {
      "id":10, "type":"SetBBValue",
      "key":"global:AllWavesSpawned",
      "dataIn":[{"name":"value", "literal":true}]
    }
  ]
}
```

**Action SpawnWave** :

```cpp
case VSActionType::SpawnWave: {
    std::string waveType = std::get<std::string>(GetActionParam(action, "waveType"));

    // Spawn selon type
    if (waveType == "Easy") {
        SpawnEnemies(3, "Guard", {100, 100});
    } else if (waveType == "Medium") {
        SpawnEnemies(5, "Soldier", {200, 100});
    } else if (waveType == "Hard") {
        SpawnEnemies(2, "Boss", {300, 100});
    }
    break;
}

void SpawnEnemies(int count, const std::string& type, Vector2 spawnPos) {
    for (int i = 0; i < count; ++i) {
        EntityID enemy = World::Get().CreateEntity();

        // Load prefab
        DataManager::Get().InstantiateEntityPrefab("Blueprints/EntityPrefab/" + type + ".json", enemy);

        // Position
        auto* pos = World::Get().GetComponent<Positiondata>(enemy);
        pos->position.x = spawnPos.x + i * 50;
        pos->position.y = spawnPos.y;
    }
}
```

### 8.2. Victory Condition

**JSON Victory Logic** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"Branch", "name":"All Objectives Complete?",
      "conditionType":"IsSet",
      "parameters":[{"name":"key", "value":"global:AllObjectivesComplete"}],
      "execOut":{"true":[3], "false":[]}
    },
    {
      "id":3, "type":"DoOnce", "name":"Trigger Victory Once", "execOut":[4]},
    {
      "id":4, "type":"Action", "name":"Play Victory Sound",
      "actionType":"PlaySound",
      "parameters":[{"name":"soundId", "value":"victory_fanfare"}],
      "execOut":[5]
    },
    {
      "id":5, "type":"Delay", "duration":2.0, "execOut":[6]},
    {
      "id":6, "type":"Action", "name":"Load Victory Screen",
      "actionType":"LoadLevel",
      "parameters":[{"name":"levelPath", "value":"Levels/victory_screen.json"}]
    }
  ]
}
```

### 8.3. Global Alert Level

**Script Global Alert** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"GetBBValue",
      "key":"global:AlertLevelGlobal",
      "dataOut":[{"name":"alert", "type":"int"}],
      "execOut":[3]
    },
    {
      "id":3, "type":"Switch", "name":"Alert Level",
      "switchType":"Enum",
      "key":"global:AlertLevelGlobal",
      "execOut":{
        "0":[10],
        "1":[11],
        "2":[12],
        "3":[13]
      }
    },
    {"id":10, "type":"Action", "name":"Normal Patrol", "actionType":"SetGlobalState", "parameters":[{"name":"state", "value":"Calm"}]},
    {"id":11, "type":"Action", "name":"Increase Patrol", "actionType":"SetGlobalState", "parameters":[{"name":"state", "value":"Cautious"}]},
    {"id":12, "type":"Sequence", "execOut":{"then0":[14], "then1":[15]}},
    {"id":14, "type":"Action", "name":"Call Reinforcements", "actionType":"SpawnReinforcements"},
    {"id":15, "type":"Action", "name":"Sound Alarm", "actionType":"PlaySound", "parameters":[{"name":"soundId", "value":"alarm"}]},
    {"id":13, "type":"Action", "name":"Lockdown", "actionType":"ActivateLockdown"}
  ]
}
```

---

## 9. Cas d'Usage : Cinématiques et Level Scripts

### 9.1. Door Trigger Script

**Entité Trigger Zone** :

```cpp
EntityID doorTrigger = World::Get().CreateEntity();

Positiondata pos;
pos.position = {800, 768, 0};
World::Get().AddComponent<Positiondata>(doorTrigger, pos);

BoundingBoxdata bbox;
bbox.width = 128;
bbox.height = 128;
World::Get().AddComponent<BoundingBoxdata>(doorTrigger, bbox);

// Blackboard pour trigger state
AIBlackboarddata bb;
bb.initialized = true;
// bb.customData["PlayerInTrigger"] = false;
// bb.customData["DoorState"] = "Closed";
World::Get().AddComponent<AIBlackboarddata>(doorTrigger, bb);

VisualScriptComponent script;
script.scriptAssetPath = "Blueprints/VisualScript/door_trigger.json";
script.enabled = true;
World::Get().AddComponent<VisualScriptComponent>(doorTrigger, script);
```

**JSON Door** (voir doc complète section 7.2).

**Actions** :

```cpp
case VSActionType::PlaySound: {
    std::string soundId = std::get<std::string>(GetActionParam(action, "soundId"));
    AudioManager::Get().PlaySound(soundId);
    break;
}

case VSActionType::SetSprite: {
    std::string spritePath = std::get<std::string>(GetActionParam(action, "spritePath"));
    auto* sprite = World::Get().GetComponent<VisualSpritedata>(entity);
    if (sprite) {
        sprite->spritePath = spritePath;
    }
    break;
}
```

### 9.2. Cutscene Sequence

**JSON Cutscene** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {"id":2, "type":"Sequence", "name":"Cutscene Steps", "execOut":{"then0":[3], "then1":[4], "then2":[5], "then3":[6]}},
    {
      "id":3, "type":"Action", "name":"Fade To Black",
      "actionType":"FadeScreen",
      "parameters":[{"name":"color", "value":"black"}, {"name":"duration", "value":1.0}]
    },
    {
      "id":4, "type":"Action", "name":"Move Camera to Boss",
      "actionType":"MoveCameraToEntity",
      "parameters":[{"name":"targetEntity", "value":"BossEntityID"}],
      "execOut":[7]
    },
    {"id":7, "type":"Delay", "duration":0.5, "execOut":[8]},
    {
      "id":8, "type":"Action", "name":"Fade From Black",
      "actionType":"FadeScreen",
      "parameters":[{"name":"color", "value":"transparent"}, {"name":"duration", "value":1.0}]
    },
    {
      "id":5, "type":"Action", "name":"Play Boss Dialogue",
      "actionType":"DisplayDialogueLine",
      "parameters":[{"name":"line", "value":"You dare challenge me?"}],
      "execOut":[9]
    },
    {"id":9, "type":"Delay", "duration":3.0, "execOut":[10]},
    {
      "id":10, "type":"Action", "name":"Boss Animation",
      "actionType":"PlayAnimation",
      "parameters":[{"name":"animId", "value":"boss_roar"}]
    },
    {
      "id":6, "type":"Action", "name":"Start Combat",
      "actionType":"SetGlobalState",
      "parameters":[{"name":"key", "value":"global:BossFightActive"}, {"name":"value", "value":true}]
    }
  ]
}
```

### 9.3. Camera Shake + VFX

**JSON Camera Effects** :

```json
{
  "nodes": [
    {"id":1, "type":"EntryPoint", "execOut":[2]},
    {
      "id":2, "type":"Action", "name":"Explosion VFX",
      "actionType":"SpawnVFX",
      "parameters":[
        {"name":"effectType", "value":"explosion"},
        {"name":"position", "value":{"x":500, "y":300}}
      ],
      "execOut":[3]
    },
    {
      "id":3, "type":"Action", "name":"Camera Shake",
      "actionType":"CameraShake",
      "parameters":[
        {"name":"intensity", "value":0.8},
        {"name":"duration", "value":0.5}
      ],
      "execOut":[4]
    },
    {
      "id":4, "type":"Action", "name":"Explosion Sound",
      "actionType":"PlaySound",
      "parameters":[{"name":"soundId", "value":"explosion_large"}]
    }
  ]
}
```

---

## 10. Actions Système Complètes

### 10.1. Actions IA/Mouvement

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **GotoPosition** | targetKey (BB), speed, tolerance | Écrit MoveIntent vers position BB |
| **SetMoveGoalToTarget** | targetKey (default "local:TargetActor") | MoveGoal = TargetActor.position |
| **MoveToGoal** | speed | Active MoveIntent (MoveSystem traite) |
| **AttackIfClose** | range, damage, cooldown | Inflige dégâts si dist <= range |
| **GetNextWaypoint** | routeKey | Lit prochain waypoint patrouille |
| **CalculateFleeDirection** | distance | Calcule point fuite opposé cible |
| **ChangeState** | newState (string) | AIState.currentState = newState |

### 10.2. Actions Visuelles/Audio

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **PlayAnimation** | animId | Change Animation.animationID |
| **StopAnimation** | - | Reset animation currentFrame = 0 |
| **SetSprite** | spritePath | Change VisualSprite.spritePath |
| **PlaySound** | soundId, volume | AudioManager::PlaySound() |
| **StopSound** | soundId | AudioManager::StopSound() |
| **SpawnVFX** | effectType, position, duration | Crée entité FX temporaire |
| **FadeScreen** | color, duration | UI fade overlay |

### 10.3. Actions Caméra

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **CameraShake** | intensity, duration | Tremble caméra |
| **MoveCameraToEntity** | targetEntity | Camera suit entité |
| **ZoomCamera** | zoomLevel, duration | Change Camera.zoomLevel |
| **SetCameraTarget** | entityId | Camera.targetEntity = id |

### 10.4. Actions Système

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **LoadLevel** | levelPath | DataManager::LoadLevel() |
| **UnloadLevel** | - | Nettoie World entities |
| **DestroyEntity** | entityId | World::DestroyEntity() |
| **SpawnEntity** | prefabPath, position | Instantiate prefab |
| **AddPlayer** | playerIndex | Crée entité joueur + bindings |
| **LogMessage** | text, level (info/warn/error) | Console log |
| **SetGlobalState** | key, value | GlobalBlackboard::SetValue() |

### 10.5. Actions Dialogue/UI

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **DisplayDialogueLine** | line (string ou BB index) | UIManager::ShowDialogue() |
| **HideDialogue** | - | Ferme UI dialogue |
| **ShowNotification** | text, duration | UI notification popup |
| **UpdateObjective** | objectiveId, status | UI objective tracker |

### 10.6. Actions Combat/Gameplay

| Action | Paramètres | Effet |
|--------|-----------|--------|
| **DealDamage** | targetEntity, amount | Réduit Health.currentHealth |
| **HealEntity** | targetEntity, amount | Augmente Health (clampé max) |
| **GiveItem** | itemId | Ajoute Inventory.items |
| **RemoveItem** | itemId | Retire Inventory.items |
| **ActivateTrigger** | triggerId | Set TriggerZone.triggered = true |
| **SpawnWave** | waveType, count, spawnPoint | Boucle SpawnEntity N fois |

---

## 11. Plan d'Implémentation

### Phase 1 : Fondations (Semaine 1-2)

**Objectif** : Structures de données + parser JSON.

**Tasks** :
1. Créer structures `VSNode`, `VSGraphAsset`, `VSRuntimeInstance`
2. Implémenter `DataManager::LoadVisualScript()` (parse JSON)
3. Créer composants `VisualScriptComponent`, `VisualScriptRuntimeComponent`
4. Test : charger JSON simple (EntryPoint + 1 Action)

### Phase 2 : Runtime Basique (Semaine 3)

**Objectif** : Exécution linéaire simple.

**Tasks** :
1. Créer `VisualScriptSystem` skeleton
2. Implémenter `ExecuteGraph()` + `ExecuteNode()` pour EntryPoint, Action simple
3. Implémenter 2-3 actions basiques : `LogMessage`, `SetBBValue`
4. Test : script qui log "Hello" et écrit BB

### Phase 3 : Flux Control (Semaine 4)

**Objectif** : Branch, Sequence, DoOnce.

**Tasks** :
1. Implémenter `EvaluateCondition()` (CompareValue, IsSet)
2. Implémenter Branch, Sequence, DoOnce dans `ExecuteNode()`
3. Test : script avec Branch True/False

### Phase 4 : Blackboard Bridge (Semaine 5)

**Objectif** : Lire/écrire composants ECS.

**Tasks** :
1. Créer `BlackboardBridge` avec mappings clés → composants
2. Implémenter `ReadBlackboard()`, `WriteBlackboard()`
3. Implémenter `GetBBValue`, `SetBBValue` nodes
4. Test : lire Health, Branch si < 30, SetBBValue nouveau state

### Phase 5 : Actions IA (Semaine 6-7)

**Objectif** : Actions mouvement/combat.

**Tasks** :
1. Implémenter `GotoPosition`, `SetMoveGoalToTarget`, `MoveToGoal`
2. Implémenter `AttackIfClose`, `ChangeState`
3. Créer `MoveIntentdata` component + MoveSystem traite
4. Test : Guard patrol simple (GetNextWaypoint → Move)

### Phase 6 : While + Delay (Semaine 8)

**Objectif** : Boucles et timers.

**Tasks** :
1. Implémenter While loop/exit
2. Implémenter Delay + `UpdateDelayTimers()`
3. Créer `DelayTimerComponent`
4. Test : While(dist > 10) Move, Delay 2s entre waypoints

### Phase 7 : Actions Système (Semaine 9)

**Objectif** : LoadLevel, DestroyEntity, SpawnEntity, etc.

**Tasks** :
1. Implémenter actions système (10-15 actions)
2. Implémenter actions visuel/audio (PlaySound, SetSprite, etc.)
3. Test : Door trigger script complet

### Phase 8 : Switch + ForEach (Semaine 10)

**Objectif** : Multi-branches et boucles arrays.

**Tasks** :
1. Implémenter Switch enum/string
2. Implémenter ForEach (itération arrays BB)
3. Test : Switch states, Spawn waves ForEach

### Phase 9 : Data Pins + Math (Semaine 11)

**Objectif** : Connexions Data entre nodes.

**Tasks** :
1. Implémenter data connections parsing
2. Implémenter data cache runtime
3. Implémenter Math node (+, -, *, /)
4. Test : GetBBValue → Math → SetBBValue

### Phase 10 : Éditeur Intégration (Semaine 12-13)

**Objectif** : Éditer graphes dans Blueprint Editor.

**Tasks** :
1. Intégrer Visual Script dans `NodeGraphManager`
2. Ajouter pins True/False visuels pour Branch
3. Ajouter connexions Data colorées
4. Test : créer guard_ai.json via éditeur

### Phase 11 : Polish + Optimization (Semaine 14)

**Objectif** : Perf, debug tools.

**Tasks** :
1. Profiling exécution scripts
2. Debug visualizer (highlight node actif runtime)
3. Error handling robuste (cycles, missing nodes)
4. Documentation finale

---

## 12. Tests et Validation

### 12.1. Tests Unitaires

**Test 1 : Parser JSON** :
```cpp
TEST(VisualScript, ParseSimpleGraph) {
    auto* graph = DataManager::Get().LoadVisualScript("test_simple.json");
    ASSERT_NE(graph, nullptr);
    ASSERT_EQ(graph->nodes.size(), 3);
    ASSERT_EQ(graph->entryPointId, 1);
}
```

**Test 2 : Branch Condition** :
```cpp
TEST(VisualScript, BranchConditionTrue) {
    EntityID entity = World::Get().CreateEntity();
    Healthdata health;
    health.currentHealth = 20.0f;
    World::Get().AddComponent<Healthdata>(entity, health);

    VSCondition cond;
    cond.type = VSConditionType::CompareValue;
    // left = "local:Health", operator = "<", right = 30

    bool result = VisualScriptSystem::EvaluateCondition(entity, cond);
    ASSERT_TRUE(result);
}
```

### 12.2. Tests d'Intégration

**Test Guard AI Complet** :
1. Spawn Guard entité avec guard_combat.json
2. Pas de cible détectée → Patrol (vérifie MoveIntent vers waypoints)
3. Spawn Player proche → Target détecté → Chase (vérifie MoveIntent vers player)
4. Player à 50 unités → Attack (vérifie AttackIfClose exécuté)

**Test Spawn Waves** :
1. Spawn WaveManager entité
2. 3 vagues (Easy, Medium, Hard)
3. Vérifie 3+5+2 = 10 ennemis créés total
4. Vérifie global:AllWavesSpawned = true à la fin

### 12.3. Tests Performance

**Benchmark** : 100 entités avec scripts actifs, 60 FPS stable.

```cpp
void BenchmarkVisualScript() {
    for (int i = 0; i < 100; ++i) {
        EntityID entity = SpawnGuardWithScript();
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < 600; ++frame) {  // 10s @ 60 FPS
        VisualScriptSystem::Get().Update(0.016f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "600 frames in " << duration.count() << "ms" << std::endl;
    // Target : < 10000ms (< 16ms/frame)
}
```

---

## Conclusion

**ATS Visual Scripting** est un système puissant et flexible s'intégrant naturellement dans l'architecture ECS d'Olympe Engine. En réutilisant les composants existants (AIState, Health, MoveIntent) et en ajoutant une couche runtime légère (VisualScriptSystem), on obtient un outil capable de scripter :

- **IA entités** (gardes, ennemis, NPCs)
- **Level scripts** (triggers, portes, puzzles)
- **Game rules** (spawn waves, victory conditions)
- **Cinématiques** (cutscenes, dialogues, caméra)

**Avantages sur Behavior Tree** :
- Flux Exec explicite visuel (facile à suivre)
- Data pins typées (calculs complexes)
- Boucles While/For natives
- Applicable au-delà de l'IA pure

**Prochaine étape** : Implémenter Phase 1 (structures + parser JSON) puis Phase 2 (runtime basique).

---

**Fin du Document Technique d'Implémentation ATS Visual Scripting**

Pour questions : voir repo Olympe Engine GitHub.
