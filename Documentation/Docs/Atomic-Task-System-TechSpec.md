# OLYMPE ENGINE - ATOMIC TASK SYSTEM
## SPECIFICATION TECHNIQUE COMPLÈTE

**Version:** 1.0  
**Date:** 20 Février 2026  
**Auteur:** Atlasbruce  
**Projet:** Olympe Engine - Task Tree System Refactoring

---

## TABLE DES MATIÈRES

1. [Vue d'Ensemble du Système](#1-vue-densemble-du-système)
2. [Architecture Conceptuelle](#2-architecture-conceptuelle)
3. [Structures de Données](#3-structures-de-données)
4. [Système d'Exécution Runtime](#4-système-dexécution-runtime)
5. [Éditeur et Interface Utilisateur](#5-éditeur-et-interface-utilisateur)
6. [Catalogue des Atomic Tasks](#6-catalogue-des-atomic-tasks)
7. [Plan d'Implémentation](#7-plan-dimplémentation)
8. [Intégration avec ECS Existant](#8-intégration-avec-ecs-existant)

---

## 1. VUE D'ENSEMBLE DU SYSTÈME

### 1.1 Objectifs

Le système **Atomic Task** transforme les Behavior Trees actuels d'Olympe en un moteur de scripting visuel polyvalent capable de :

- **IA NPC** : Patrouilles, combats, détection (usage actuel amélioré)
- **Level Design** : Séquences scriptées, cinématiques, puzzles
- **Game Rules** : Conditions de victoire/défaite, gestion d'événements
- **Object Scripting** : Portes, terminaux, pièges, objets interactifs

### 1.2 Terminologie

| Terme Actuel (v2.0) | Nouveau Terme (Atomic System) | Description |
|---------------------|-------------------------------|-------------|
| BehaviorTree | **TaskGraph** | Le modèle de graphe (Asset JSON) |
| BehaviorTreeRuntime | **TaskRunner** | Le composant ECS qui exécute |
| Action/Condition Node | **Atomic Task** | Brique élémentaire d'exécution |
| AIBlackboard | **LocalBlackboard** | Mémoire privée de l'instance |

### 1.3 Philosophie d'Architecture

**Pattern "Template & Runner" (Flyweight)** :
- Un **TaskGraphTemplate** (Asset) : partagé, immutable, chargé une seule fois
- Plusieurs **TaskRunner** (Components) : un par entité, contient l'état privé

Inspiré de :
- Unreal Engine : `BehaviorTree` (asset) vs `BehaviorTreeComponent` (instance)
- Unity : `Animator Controller` (asset) vs `Animator` (component)

---

## 2. ARCHITECTURE CONCEPTUELLE

### 2.1 Vue d'Ensemble (3 Couches)

```
┌─────────────────────────────────────────────────────────┐
│  LAYER 1: DESIGN TIME (Editor)                          │
│  ┌───────────────┐                                       │
│  │ Game Designer │ creates ──► TaskGraph.json           │
│  └───────────────┘              (Nodes, Variables, etc.)│
└─────────────────────────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│  LAYER 2: LOAD TIME (Compilation)                       │
│  TaskGraph.json ──► [Loader/Validator] ──► Template     │
│                     (Optimized Read-Only Asset)          │
│                     Cached in AssetManager               │
└─────────────────────────────────────────────────────────┘
                    ▼
┌─────────────────────────────────────────────────────────┐
│  LAYER 3: RUNTIME (Execution Per Frame)                 │
│  Entity A ──► TaskRunner A ──► LocalBlackboard A        │
│            │     ├─References─► Template (Shared)       │
│            │     └─Calls────────► Atomic Task Library   │
│                                                          │
│  Entity B ──► TaskRunner B ──► LocalBlackboard B        │
│            │     └─References─► Template (Shared)       │
│                                                          │
│  [TaskSystem::Update(dt)] orchestrates all runners      │
└─────────────────────────────────────────────────────────┘
```

### 2.2 Séparation des Préoccupations

| Couche | Responsabilité | Format | Quand |
|--------|----------------|--------|-------|
| **Design** | Authoring graphe | JSON human-readable | Éditeur |
| **Load** | Validation, optimisation | C++ structures | Chargement niveau |
| **Runtime** | Exécution, état | ECS Components | Chaque frame |

---

## 3. STRUCTURES DE DONNÉES

### 3.1 Format JSON (Layer 1 : Asset)

#### Structure Globale d'un TaskGraph

```json
{
  "schemaVersion": 3,
  "type": "TaskGraph",
  "name": "GuardPatrolAndCombat",
  "description": "AI comportement garde avec patrouille et combat",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-02-20T12:00:00Z",
    "tags": ["AI", "Guard", "Combat"]
  },
  "variables": [
    {
      "name": "PatrolIndex",
      "type": "int",
      "defaultValue": 0,
      "scope": "local"
    },
    {
      "name": "CurrentTarget",
      "type": "EntityID",
      "defaultValue": null,
      "scope": "local"
    },
    {
      "name": "AlertLevel",
      "type": "float",
      "defaultValue": 0.0,
      "scope": "local"
    }
  ],
  "nodes": [
    {
      "id": 1,
      "name": "Start",
      "type": "EventStart",
      "position": {"x": 100, "y": 100},
      "outputs": [2]
    },
    {
      "id": 2,
      "name": "Combat ou Patrouille",
      "type": "Selector",
      "position": {"x": 300, "y": 100},
      "children": [3, 5]
    },
    {
      "id": 3,
      "name": "Séquence Combat",
      "type": "Sequence",
      "children": [4]
    },
    {
      "id": 4,
      "name": "Attaquer Cible",
      "type": "AtomicTask",
      "atomicTaskId": "AttackTarget",
      "parameters": {
        "Target": {
          "bindingType": "Variable",
          "variableName": "CurrentTarget"
        },
        "Damage": {
          "bindingType": "Literal",
          "value": 15.0
        }
      }
    },
    {
      "id": 5,
      "name": "Patrouiller",
      "type": "AtomicTask",
      "atomicTaskId": "MoveToWaypoint",
      "parameters": {
        "WaypointIndex": {
          "bindingType": "Variable",
          "variableName": "PatrolIndex"
        },
        "Speed": {
          "bindingType": "Literal",
          "value": 80.0
        }
      }
    }
  ]
}
```

#### Système de Binding des Paramètres

Chaque paramètre peut être :
- **Literal** : Valeur en dur (`"value": 5.0`)
- **Variable** : Référence au Blackboard local (`"variableName": "RunSpeed"`)
- **GlobalVariable** : Référence au Blackboard global du niveau (futur)

### 3.2 Structures C++ (Layer 2 : Template)

#### TaskGraphTemplate (Asset)

```cpp
// File: Source/TaskSystem/TaskGraphTemplate.h

#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Core/Types.h"

namespace Olympe {

enum class TaskNodeType {
    EventStart,      // Point d'entrée
    Selector,        // Essaie les enfants dans l'ordre (OR)
    Sequence,        // Exécute tous les enfants (AND)
    Parallel,        // Exécute plusieurs branches en parallèle
    Branch,          // If/Else conditionnel
    Loop,            // Répète N fois ou tant que condition vraie
    AtomicTask,      // Feuille : appel à une tâche C++
    SetVariable,     // Modifie une variable du blackboard
    Compare          // Compare deux valeurs
};

enum class VariableType {
    Bool, Int, Float, Vector2, Vector3, EntityID, String
};

struct VariableDefinition {
    std::string Name;
    VariableType Type;
    Variant DefaultValue;  // Union-like type
    bool IsLocal;          // true = local BB, false = global BB
};

enum class ParameterBindingType {
    Literal,               // Valeur constante
    LocalVariable,         // Ref vers variable locale
    GlobalVariable         // Ref vers variable globale (future)
};

struct ParameterBinding {
    ParameterBindingType Type;
    Variant LiteralValue;
    std::string VariableName;
};

struct TaskNodeDefinition {
    int32 NodeID;
    std::string NodeName;
    TaskNodeType Type;
    
    // Pour les nœuds de contrôle (Selector, Sequence, etc.)
    std::vector<int32> ChildrenIDs;
    
    // Pour les AtomicTask
    std::string AtomicTaskID;  // Ex: "MoveToWaypoint"
    std::unordered_map<std::string, ParameterBinding> Parameters;
    
    // Navigation (optimisé à la compilation)
    int32 NextOnSuccess;
    int32 NextOnFailure;
};

class TaskGraphTemplate {
public:
    std::string Name;
    std::string Description;
    
    std::vector<VariableDefinition> LocalVariables;
    std::vector<TaskNodeDefinition> Nodes;
    
    int32 RootNodeID;
    
    // Cache pour accès rapide
    std::unordered_map<int32, const TaskNodeDefinition*> NodeLookup;
    
    // Chargement depuis JSON
    static TaskGraphTemplate* LoadFromFile(const std::string& path);
    
    // Validation
    bool Validate() const;
};

} // namespace Olympe
```

### 3.3 Structures Runtime (Layer 3)

#### TaskRunnerComponent (ECS Component)

```cpp
// File: Source/ECS/Components/TaskRunnerComponent.h

#pragma once
#include "Core/Types.h"
#include <vector>

namespace Olympe {

struct TaskRunnerComponent {
    // Référence vers le template (Asset ID)
    AssetID GraphTemplateID;
    
    // État d'exécution
    int32 CurrentNodeIndex;
    float StateTimer;           // Pour les Wait, animations, etc.
    
    // Blackboard local (mémoire brute)
    std::vector<uint8> LocalBlackboardData;
    
    // Status du nœud actuel
    enum class TaskStatus { Success, Failure, Running, Aborted };
    TaskStatus LastStatus;
    
    // Debug (optionnel, désactivable en Release)
    #if OLYMPE_DEBUG
    std::vector<int32> ExecutionTrace;  // Historique des nœuds visités
    #endif
};

} // namespace Olympe
```

#### LocalBlackboard (Memory Block)

Le Blackboard est un **bloc mémoire contigu** structuré selon les `VariableDefinition` du Template.

```cpp
// File: Source/TaskSystem/LocalBlackboard.h

#pragma once
#include <vector>
#include <unordered_map>
#include "Core/Types.h"

namespace Olympe {

class LocalBlackboard {
private:
    std::vector<uint8> Data;  // Mémoire brute
    std::unordered_map<std::string, size_t> VariableOffsets;
    
public:
    // Initialise depuis le Template
    void Initialize(const TaskGraphTemplate& tmpl);
    
    // Accesseurs typés
    template<typename T>
    T GetValue(const std::string& varName) const;
    
    template<typename T>
    void SetValue(const std::string& varName, const T& value);
    
    // Accesseur générique pour les Atomic Tasks
    Variant GetVariant(const std::string& varName) const;
    void SetVariant(const std::string& varName, const Variant& value);
};

} // namespace Olympe
```

---

## 4. SYSTÈME D'EXÉCUTION RUNTIME

### 4.1 TaskSystem (ECS System)

```cpp
// File: Source/TaskSystem/TaskSystem.h

#pragma once
#include "ECS/System.h"
#include "TaskGraphTemplate.h"
#include "AtomicTaskRegistry.h"

namespace Olympe {

class TaskSystem : public ISystem {
public:
    void Update(float deltaTime) override;
    
private:
    // Helper pour exécuter un nœud
    void ExecuteNode(
        EntityID entity,
        TaskRunnerComponent& runner,
        const TaskGraphTemplate& tmpl,
        const TaskNodeDefinition& node,
        LocalBlackboard& blackboard,
        float dt
    );
    
    // Navigation dans le graphe
    void TransitionToNextNode(
        TaskRunnerComponent& runner,
        const TaskGraphTemplate& tmpl,
        TaskRunnerComponent::TaskStatus status
    );
};

} // namespace Olympe
```

#### Pseudo-code d'Exécution

```cpp
void TaskSystem::Update(float dt) {
    // Itérer sur toutes les entités qui ont un TaskRunner
    for (auto [entity, runner] : World::Get().View<TaskRunnerComponent>()) {
        
        // 1. Récupérer le template depuis l'AssetManager
        const TaskGraphTemplate* tmpl = AssetManager::Get().GetTaskGraph(runner.GraphTemplateID);
        if (!tmpl) continue;
        
        // 2. Récupérer le nœud actuel
        const TaskNodeDefinition& currentNode = tmpl->Nodes[runner.CurrentNodeIndex];
        
        // 3. Récupérer le Blackboard local
        LocalBlackboard blackboard;
        blackboard.Initialize(*tmpl);  // Construit depuis le runner.LocalBlackboardData
        
        // 4. Exécuter le nœud selon son type
        TaskRunnerComponent::TaskStatus status;
        
        switch (currentNode.Type) {
            case TaskNodeType::Selector:
                status = ExecuteSelector(entity, runner, *tmpl, currentNode, blackboard, dt);
                break;
            case TaskNodeType::Sequence:
                status = ExecuteSequence(entity, runner, *tmpl, currentNode, blackboard, dt);
                break;
            case TaskNodeType::AtomicTask:
                status = ExecuteAtomicTask(entity, runner, *tmpl, currentNode, blackboard, dt);
                break;
            // ... autres types
        }
        
        // 5. Transition selon le résultat
        runner.LastStatus = status;
        if (status != TaskStatus::Running) {
            TransitionToNextNode(runner, *tmpl, status);
        }
    }
}

TaskSystem::TaskStatus ExecuteAtomicTask(
    EntityID entity, 
    TaskRunnerComponent& runner,
    const TaskGraphTemplate& tmpl,
    const TaskNodeDefinition& node,
    LocalBlackboard& blackboard,
    float dt
) {
    // 1. Récupérer la task C++ depuis le registry
    IAtomicTask* task = AtomicTaskRegistry::Get().GetTask(node.AtomicTaskID);
    if (!task) {
        LOG_ERROR("Unknown atomic task: " << node.AtomicTaskID);
        return TaskStatus::Failure;
    }
    
    // 2. Préparer le contexte d'exécution
    AtomicTaskContext ctx;
    ctx.Entity = entity;
    ctx.World = &World::Get();
    ctx.Blackboard = &blackboard;
    ctx.DeltaTime = dt;
    ctx.StateTimer = &runner.StateTimer;
    
    // 3. Résoudre les paramètres (Literal vs Variable Binding)
    std::unordered_map<std::string, Variant> resolvedParams;
    for (const auto& [paramName, binding] : node.Parameters) {
        if (binding.Type == ParameterBindingType::Literal) {
            resolvedParams[paramName] = binding.LiteralValue;
        } else if (binding.Type == ParameterBindingType::LocalVariable) {
            resolvedParams[paramName] = blackboard.GetVariant(binding.VariableName);
        }
    }
    
    // 4. Exécuter la tâche
    return task->Execute(ctx, resolvedParams);
}
```

### 4.2 Atomic Task Interface

```cpp
// File: Source/TaskSystem/IAtomicTask.h

#pragma once
#include "Core/Types.h"
#include <unordered_map>

namespace Olympe {

struct AtomicTaskContext {
    EntityID Entity;
    World* World;
    LocalBlackboard* Blackboard;
    float DeltaTime;
    float* StateTimer;  // Pointeur vers le timer du Runner
};

class IAtomicTask {
public:
    virtual ~IAtomicTask() = default;
    
    // Méthode principale d'exécution
    virtual TaskRunnerComponent::TaskStatus Execute(
        const AtomicTaskContext& ctx,
        const std::unordered_map<std::string, Variant>& params
    ) = 0;
    
    // Métadonnées pour l'éditeur
    virtual std::string GetName() const = 0;
    virtual std::string GetCategory() const = 0;
    virtual std::vector<ParameterDescriptor> GetParameters() const = 0;
};

struct ParameterDescriptor {
    std::string Name;
    VariableType Type;
    Variant DefaultValue;
    std::string Description;
    bool IsOptional;
};

} // namespace Olympe
```

### 4.3 Atomic Task Registry

```cpp
// File: Source/TaskSystem/AtomicTaskRegistry.h

#pragma once
#include "IAtomicTask.h"
#include <unordered_map>
#include <memory>

namespace Olympe {

class AtomicTaskRegistry {
public:
    static AtomicTaskRegistry& Get();
    
    // Enregistrement des tasks (appelé au startup)
    void Register(const std::string& id, std::unique_ptr<IAtomicTask> task);
    
    // Récupération pour exécution
    IAtomicTask* GetTask(const std::string& id) const;
    
    // Pour l'éditeur : liste de toutes les tasks disponibles
    std::vector<std::string> GetAllTaskIDs() const;
    const std::vector<ParameterDescriptor>& GetTaskParameters(const std::string& id) const;
    
private:
    std::unordered_map<std::string, std::unique_ptr<IAtomicTask>> Tasks;
};

// Macro helper pour l'enregistrement
#define REGISTER_ATOMIC_TASK(TaskClass, TaskID) \
    static bool _registered_##TaskClass = []() { \
        AtomicTaskRegistry::Get().Register(TaskID, std::make_unique<TaskClass>()); \
        return true; \
    }();

} // namespace Olympe
```

---

## 5. ÉDITEUR ET INTERFACE UTILISATEUR

### 5.1 Table de Correspondance des Nœuds

| Backend C++ Type | Nom Éditeur | Icône | Couleur | Description Designer |
|------------------|-------------|-------|---------|----------------------|
| `EventStart` | **Start / OnBegin** | ▶️ | Vert (#4CAF50) | Point d'entrée du graphe |
| `Selector` | **Try / Fallback** | ❓ | Violet (#9C27B0) | Essaie dans l'ordre, prend le 1er succès |
| `Sequence` | **Sequence / Do All** | ➡️ | Bleu (#2196F3) | Exécute tout, arrêt si échec |
| `Parallel` | **Do Together** | 🔀 | Orange (#FF9800) | Lance plusieurs branches simultanément |
| `Branch` | **Branch (If/Else)** | 💎 | Gris (#757575) | Test : si Vrai → A, sinon → B |
| `Loop` | **Repeat / Loop** | 🔁 | Bleu foncé (#1565C0) | Répète X fois ou tant que condition |
| `AtomicTask` | **Task / Action** | ⚡ | Jaune (#FFEB3B) | Brique d'action (Move, Attack, Wait...) |
| `SetVariable` | **Set Variable** | 📝 | Blanc (#FAFAFA) | Modifie une valeur du Blackboard |
| `Compare` | **Check Value** | ⚖️ | Gris clair (#BDBDBD) | Compare deux valeurs (>, <, ==) |

### 5.2 Panneau Variables (Nouveau)

Ajouter dans l'éditeur existant un panneau **"Variables"** (gauche, sous l'Asset Browser).

**Fonctionnalités** :
- Créer/Supprimer des variables
- Définir : Nom, Type (int/float/bool/Vector2/EntityID), Valeur par défaut
- Scope : Local (instance) ou Global (niveau)

**Implémentation** :
```cpp
// File: Source/BlueprintEditor/Panels/VariablePanel.h

class VariablePanel {
public:
    void Render();
    
private:
    void RenderVariableList();
    void RenderAddVariableDialog();
    void RenderVariableProperties(VariableDefinition& var);
};
```

### 5.3 Binding UI pour Paramètres

Dans l'Inspector d'un nœud AtomicTask, chaque paramètre affiche :

```
┌─────────────────────────────────────┐
│ Parameter: Speed                    │
│ ┌───────────────────┐               │
│ │ Literal ▼         │  ◄─ ComboBox │
│ └───────────────────┘               │
│ ┌───────────────────┐               │
│ │ 80.0              │  ◄─ Input    │
│ └───────────────────┘               │
└─────────────────────────────────────┘

Si on sélectionne "Variable" dans la ComboBox :

┌─────────────────────────────────────┐
│ Parameter: Speed                    │
│ ┌───────────────────┐               │
│ │ Variable ▼        │               │
│ └───────────────────┘               │
│ ┌───────────────────┐               │
│ │ RunSpeed ▼        │  ◄─ Liste    │
│ └───────────────────┘               │
└─────────────────────────────────────┘
```

**Implémentation ImGui** :
```cpp
void RenderParameterBinding(ParameterBinding& binding, const std::string& paramName, VariableType paramType) {
    ImGui::Text("%s", paramName.c_str());
    
    const char* bindingTypes[] = { "Literal", "Variable" };
    int currentType = (int)binding.Type;
    if (ImGui::Combo("##BindingType", &currentType, bindingTypes, 2)) {
        binding.Type = (ParameterBindingType)currentType;
    }
    
    if (binding.Type == ParameterBindingType::Literal) {
        // Afficher input selon le type (float, int, bool, etc.)
        RenderLiteralInput(binding.LiteralValue, paramType);
    } else {
        // Afficher dropdown des variables compatibles
        RenderVariableSelector(binding.VariableName, paramType);
    }
}
```

### 5.4 Intégration avec l'Éditeur Existant

**Modifications à apporter au Blueprint Editor actuel** :

1. **AssetBrowser** : Ajouter détection du type `"TaskGraph"` (actuellement gère BehaviorTree, HFSM, EntityPrefab)
2. **NodeGraphManager** : Étendre pour supporter les nouveaux types de nœuds (Start, Branch, Loop, etc.)
3. **Inspector** : Afficher les paramètres avec système de binding
4. **Nouveau Panneau** : Variables Panel (gestion des variables du graphe)

**Compatibilité rétroactive** :
- Les anciens BehaviorTree JSON restent supportés (schema v2)
- Les TaskGraph utilisent schema v3
- Un outil de migration peut être fourni ultérieurement

---

## 6. CATALOGUE DES ATOMIC TASKS

### 6.1 Structure d'Organisation

```
Source/TaskSystem/AtomicTasks/
├── Movement/
│   ├── Task_MoveToLocation.cpp
│   ├── Task_MoveToWaypoint.cpp
│   ├── Task_RotateTowards.cpp
│   └── Task_SetPatrolPoint.cpp
├── Perception/
│   ├── Task_CheckPerception.cpp
│   ├── Task_GetEntityPosition.cpp
│   └── Task_RaycastTest.cpp
├── GameLogic/
│   ├── Task_SetBlackboardBool.cpp
│   ├── Task_CheckBlackboardBool.cpp
│   ├── Task_SpawnEntity.cpp
│   ├── Task_DestroyEntity.cpp
│   └── Task_LoadLevel.cpp
├── Visual/
│   ├── Task_PlayAnimation.cpp
│   ├── Task_SetSpriteColor.cpp
│   └── Task_PlaySound.cpp
└── Utility/
    ├── Task_Wait.cpp
    ├── Task_LogMessage.cpp
    └── Task_DebugBreak.cpp
```

### 6.2 Liste Complète des Tasks Prioritaires

#### A. Movement (Mouvement)

| Task ID | Nom Éditeur | Paramètres | Components Requis | Retour |
|---------|-------------|------------|-------------------|--------|
| `MoveToLocation` | Move To Position | `Target` (Vec3), `Speed` (float), `AcceptanceRadius` (float) | Movement, Position | Running/Success |
| `MoveToWaypoint` | Move To Waypoint | `WaypointTag` (string) ou `Index` (int) | Movement, AIBlackboard | Running/Success |
| `RotateTowards` | Rotate To Target | `Target` (Vec3 ou EntityID), `Speed` (float) | Position (rotation) | Running/Success |
| `SetPatrolPoint` | Set Next Patrol Point | `Strategy` (enum: Loop/PingPong/Random) | AIBlackboard | Success |
| `Wait` | Wait / Delay | `Duration` (float), `RandomRange` (float, opt) | Aucun | Running/Success |

#### B. Perception (Sens & Détection)

| Task ID | Nom Éditeur | Paramètres | Components Requis | Retour |
|---------|-------------|------------|-------------------|--------|
| `CheckPerception` | Check If Perceived | `TargetTag` (string), `SenseType` (enum: Vision/Hearing) | AISenses | Success/Failure |
| `GetEntityPosition` | Get Entity Position | `TargetTag` (string), `OutputKey` (string) | Position (target) | Success/Failure |
| `RaycastTest` | Raycast Line of Sight | `Start` (Vec3), `End` (Vec3), `LayerMask` (int) | Physics | Success/Failure |

#### C. Game Logic (Règles de Jeu)

| Task ID | Nom Éditeur | Paramètres | Components Requis | Retour |
|---------|-------------|------------|-------------------|--------|
| `SetBlackboardBool` | Set Bool Variable | `Key` (string), `Value` (bool) | Blackboard | Success |
| `CheckBlackboardBool` | Check Bool Variable | `Key` (string) | Blackboard | Success/Failure |
| `SetBlackboardInt` | Set Int Variable | `Key` (string), `Value` (int) | Blackboard | Success |
| `SetBlackboardFloat` | Set Float Variable | `Key` (string), `Value` (float) | Blackboard | Success |
| `CompareValues` | Compare Two Values | `A` (Variant), `Operator` (enum), `B` (Variant) | Aucun | Success/Failure |
| `SpawnEntity` | Spawn Entity | `PrefabName` (string), `Location` (Vec3) | World | Success/Failure |
| `DestroyEntity` | Destroy Entity | `TargetEntity` (EntityID) | World | Success |
| `LoadLevel` | Load Level | `LevelPath` (string), `Transition` (enum) | GameEngine | Running/Success |

#### D. Visual & Feedback (Visuels & Sons)

| Task ID | Nom Éditeur | Paramètres | Components Requis | Retour |
|---------|-------------|------------|-------------------|--------|
| `PlayAnimation` | Play Animation | `AnimName` (string), `Loop` (bool) | SpriteAnimation | Success/Running |
| `SetSpriteColor` | Set Sprite Tint | `Color` (RGBA) | VisualSprite | Success |
| `PlaySound` | Play Sound | `SoundID` (string), `Volume` (float) | AudioSource | Success |
| `ShowUIMessage` | Display UI Message | `Text` (string), `Duration` (float) | UIManager | Running/Success |

#### E. Utility (Utilitaires & Debug)

| Task ID | Nom Éditeur | Paramètres | Components Requis | Retour |
|---------|-------------|------------|-------------------|--------|
| `LogMessage` | Log to Console | `Message` (string), `Level` (enum) | Aucun | Success |
| `DebugBreak` | Debug Breakpoint | Aucun | Aucun | Success |
| `SetGameSpeed` | Set Time Scale | `Scale` (float) | GameEngine | Success |

### 6.3 Exemple d'Implémentation : Task_MoveToLocation

```cpp
// File: Source/TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.cpp

#include "Task_MoveToLocation.h"
#include "ECS/Components/PositionComponent.h"
#include "ECS/Components/MovementComponent.h"

using namespace Olympe;

std::string Task_MoveToLocation::GetName() const {
    return "Move To Position";
}

std::string Task_MoveToLocation::GetCategory() const {
    return "Movement";
}

std::vector<ParameterDescriptor> Task_MoveToLocation::GetParameters() const {
    return {
        { "Target", VariableType::Vector3, Vector3(0,0,0), "Position cible", false },
        { "Speed", VariableType::Float, 100.0f, "Vitesse de déplacement", false },
        { "AcceptanceRadius", VariableType::Float, 5.0f, "Rayon d'acceptation", true }
    };
}

TaskRunnerComponent::TaskStatus Task_MoveToLocation::Execute(
    const AtomicTaskContext& ctx,
    const std::unordered_map<std::string, Variant>& params
) {
    // 1. Récupérer les composants nécessaires
    auto* pos = ctx.World->GetComponent<PositionComponent>(ctx.Entity);
    auto* move = ctx.World->GetComponent<MovementComponent>(ctx.Entity);
    
    if (!pos || !move) {
        LOG_ERROR("MoveToLocation: Entity missing Position or Movement component");
        return TaskStatus::Failure;
    }
    
    // 2. Extraire les paramètres
    Vector3 target = params.at("Target").AsVector3();
    float speed = params.at("Speed").AsFloat();
    float acceptRadius = params.count("AcceptanceRadius") 
        ? params.at("AcceptanceRadius").AsFloat() 
        : 5.0f;
    
    // 3. Calculer la distance
    Vector3 toTarget = target - pos->Position;
    float distance = toTarget.Length();
    
    // 4. Vérifier si on est arrivé
    if (distance <= acceptRadius) {
        move->Velocity = Vector3::Zero;
        return TaskStatus::Success;
    }
    
    // 5. Appliquer le mouvement
    Vector3 direction = toTarget.Normalized();
    move->Velocity = direction * speed;
    
    return TaskStatus::Running;
}

// Enregistrement automatique
REGISTER_ATOMIC_TASK(Task_MoveToLocation, "MoveToLocation");
```

---

## 7. PLAN D'IMPLÉMENTATION

### Phase 1 : Fondations (2 semaines)

**Objectifs** :
- Structures de données de base (TaskGraphTemplate, TaskRunner)
- Système de chargement JSON → Template
- Interface IAtomicTask + Registry

**Livrables** :
- `TaskGraphTemplate.h/cpp`
- `TaskRunnerComponent.h`
- `TaskGraphLoader.cpp` (parse JSON v3)
- `IAtomicTask.h`
- `AtomicTaskRegistry.h/cpp`

**Tests** :
- Charger un TaskGraph JSON simple
- Créer une entité avec TaskRunner
- Exécuter une tâche dummy (`Task_LogMessage`)

### Phase 2 : Moteur d'Exécution (2 semaines)

**Objectifs** :
- TaskSystem complet (Selector, Sequence, Branch, Loop)
- LocalBlackboard avec gestion typée
- Système de binding Literal/Variable

**Livrables** :
- `TaskSystem.cpp::Update()`
- `LocalBlackboard.h/cpp`
- Exécution des types de nœuds de contrôle
- Résolution des bindings de paramètres

**Tests** :
- Graphe avec Selector → deux branches
- Graphe avec Sequence → échoue au milieu
- Variable binding : lire/écrire dans BB

### Phase 3 : Atomic Tasks Essentielles (2 semaines)

**Objectifs** :
- Implémenter les 15-20 tasks prioritaires (voir catalogue)
- Focus : Movement, Perception, Game Logic

**Livrables** :
- Dossier `AtomicTasks/` avec toutes les implémentations
- Documentation inline (paramètres, comportement)

**Tests** :
- Recréer le Guard Patrol existant avec nouvelles tasks
- Tester détection (CheckPerception)
- Tester Load Level

### Phase 4 : Éditeur UI (3 semaines)

**Objectifs** :
- Support TaskGraph dans l'éditeur existant
- Panneau Variables
- Binding UI (Literal/Variable switch)
- Toolbox dynamique depuis Registry

**Livrables** :
- `VariablePanel.h/cpp`
- Modifications dans `NodeGraphManager`
- Binding UI dans Inspector
- Drag & drop depuis la toolbox

**Tests** :
- Créer un TaskGraph depuis l'éditeur
- Ajouter/modifier variables
- Changer binding d'un paramètre

### Phase 5 : Debug & Polish (1 semaine)

**Objectifs** :
- Debug visuel en runtime (highlight nœuds actifs)
- Performance profiling
- Documentation utilisateur

**Livrables** :
- Runtime overlay (nœuds verts/rouges)
- Timer par nœud
- Guide utilisateur PDF

**Tests** :
- Profiler 100 entités avec TaskRunner
- Vérifier memory leaks
- Test en conditions réelles (GDD Matrimandir)

---

## 8. INTÉGRATION AVEC ECS EXISTANT

### 8.1 Composants ECS à Conserver

Les composants actuels sont **compatibles** et seront utilisés par les Atomic Tasks :

| Component Actuel | Usage dans Atomic System |
|------------------|--------------------------|
| `PositionComponent` | Lecture/écriture par Movement tasks |
| `MovementComponent` | Appliqué par MoveToLocation, Patrol |
| `AISensesComponent` | Utilisé par CheckPerception |
| `AIBlackboardComponent` | **Remplacé** par LocalBlackboard (nouveau) |
| `BehaviorTreeRuntimeComponent` | **Remplacé** par TaskRunnerComponent |
| `HealthComponent` | Utilisé par tasks de combat |
| `SpriteAnimationComponent` | Utilisé par PlayAnimation |

### 8.2 Migration BehaviorTree → TaskGraph

**Option 1 : Cohabitation** (Recommandé Phase 1-3)
- Les deux systèmes tournent en parallèle
- Entités avec `BehaviorTreeRuntime` : ancien système
- Entités avec `TaskRunner` : nouveau système

**Option 2 : Migration Complète** (Phase 4+)
- Outil de conversion JSON BT v2 → TaskGraph v3
- Réécriture des graphes existants (Guard, Investigate...)
- Suppression de l'ancien code

### 8.3 Système de Scoping (Local vs Global Blackboard)

**Local Blackboard** (actuel) : Propre à chaque entité
- Variables : `PatrolIndex`, `CurrentTarget`, `AlertLevel`
- Stockées dans le `TaskRunnerComponent`

**Global Blackboard** (futur) : Partagé au niveau du niveau
- Variables : `GlobalAlert`, `HasCode`, `MissionState`
- Stocké dans un composant singleton `LevelBlackboard`

**Accès depuis les Tasks** :
```cpp
// Local
float alertLevel = ctx.Blackboard->GetValue<float>("AlertLevel");

// Global (futur)
bool hasCode = World::Get().GetGlobalBlackboard()->GetValue<bool>("HasCode");
```

---

## ANNEXES

### A. Checklist de Validation du Code

Avant de commit une nouvelle Atomic Task :

- [ ] Hérite de `IAtomicTask`
- [ ] Implémente `Execute()`, `GetName()`, `GetCategory()`, `GetParameters()`
- [ ] Enregistrée via `REGISTER_ATOMIC_TASK`
- [ ] Gère correctement les composants manquants (nullptr check)
- [ ] Retourne `Success/Failure/Running` de manière cohérente
- [ ] Documentation inline des paramètres
- [ ] Test unitaire créé dans `Tests/TaskSystem/`

### B. Format JSON Complet (Référence)

Voir fichier annexe `TaskGraphSchema_v3.json` pour le JSON Schema complet validable.

### C. Glossaire

| Terme | Définition |
|-------|------------|
| **Atomic Task** | Brique élémentaire d'exécution (fonction C++) appelée par un nœud |
| **TaskGraph** | Graphe de contrôle (Asset JSON) définissant la logique |
| **TaskRunner** | Composant ECS qui exécute un TaskGraph sur une entité |
| **LocalBlackboard** | Mémoire privée d'une instance (variables locales) |
| **Template** | Version compilée et optimisée d'un TaskGraph (read-only) |
| **Binding** | Lien entre un paramètre de task et une source (Literal/Variable) |

---

## CONTACT & QUESTIONS

Pour toute question sur cette spécification, contacter :
- **Atlasbruce** (Lead Developer - Olympe Engine)
- Repository : https://github.com/Atlasbruce/Olympe-Engine

**Version du document** : 1.0 (20/02/2026)
**Prochaine revue** : Après Phase 1 (Mi-Mars 2026)
