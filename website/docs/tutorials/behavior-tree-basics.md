---
id: behavior-tree-basics
title: Behavior Tree Basics for NPCs
sidebar_label: Behavior Trees
sidebar_position: 3
---

# Behavior Tree Basics for NPCs

Learn how to create AI behaviors for NPCs using Olympe Engine's behavior tree system. This tutorial covers everything from understanding behavior tree concepts to implementing patrol, wander, and combat AI behaviors.

## What You'll Learn

- Understanding behavior tree fundamentals
- Node types: Selectors, Sequences, Conditions, Actions, Decorators
- Creating behavior tree JSON files
- Implementing patrol AI with waypoints
- Creating wander behavior with pathfinding
- Building combat AI with detection and attacking
- Combining behaviors for complex AI
- Debugging and testing behavior trees
- Best practices for AI design

## Prerequisites

- Understanding of entity creation (see [Creating Entities Tutorial](./creating-entities.md))
- Basic JSON knowledge
- Familiarity with the Olympe Engine ECS system

---

## Part 1: Understanding Behavior Trees

### What is a Behavior Tree?

A behavior tree is a hierarchical structure that controls AI decision-making. Unlike state machines, behavior trees are:

- **Modular**: Reuse subtrees across different AI types
- **Readable**: Tree structure is intuitive and visual
- **Data-Driven**: Define in JSON without coding
- **Composable**: Build complex behaviors from simple nodes

### The Tree Analogy

Think of a decision-making flowchart:

```
Should I attack the player?
├─ Can I see the player? (Condition)
│  ├─ YES → Is player in range? (Condition)
│  │  ├─ YES → Attack! (Action)
│  │  └─ NO → Move closer (Action)
│  └─ NO → Patrol (Action)
```

This is a behavior tree! Each decision point is a **node**, and execution flows from root to leaves.

### Behavior Tree Execution

Trees execute **top-to-bottom, left-to-right**:

1. Start at the **root node**
2. Evaluate child nodes in order
3. Each node returns a **status**: Success, Failure, or Running
4. Parent nodes react to child status
5. Repeat every frame

### Node Status Values

Every node returns one of three states:

- **Success** ✅: Node completed successfully
- **Failure** ❌: Node failed to complete
- **Running** 🔄: Node is still executing (multi-frame actions)

---

## Part 2: Node Types

### Composite Nodes

Composite nodes have multiple children and control execution flow.

#### Selector (OR Node)

**Purpose**: Try each child until one succeeds (like `OR` logic)

**Behavior**:
- Executes children left-to-right
- Returns **Success** if any child succeeds
- Returns **Failure** if all children fail
- Returns **Running** if a child is running

**Use Case**: Choosing between alternative behaviors

```json
{
    "type": "Selector",
    "name": "Attack or Patrol",
    "children": [
        {"type": "Sequence", "name": "Attack Player", "children": [...]},
        {"type": "Action", "actionType": "Patrol"}
    ]
}
```

**Example**: "Try to attack player. If you can't, patrol instead."

#### Sequence (AND Node)

**Purpose**: Execute all children in order (like `AND` logic)

**Behavior**:
- Executes children left-to-right
- Returns **Success** if all children succeed
- Returns **Failure** if any child fails
- Returns **Running** if a child is running

**Use Case**: Sequential steps that must all complete

```json
{
    "type": "Sequence",
    "name": "Chase and Attack",
    "children": [
        {"type": "Condition", "conditionType": "TargetVisible"},
        {"type": "Action", "actionType": "SetMoveGoalToTarget"},
        {"type": "Action", "actionType": "MoveTo"}
    ]
}
```

**Example**: "Check if target visible AND move to target AND attack."

### Leaf Nodes

Leaf nodes perform actual work.

#### Condition Nodes

**Purpose**: Check a condition and return Success/Failure

**Available Conditions**:

| Condition Type | Parameters | Description |
|----------------|------------|-------------|
| `TargetVisible` | - | Does entity have a target? |
| `TargetInRange` | `conditionParam` (range) | Is target within X units? |
| `HealthBelow` | `conditionParam` (threshold) | Is health below X%? |
| `HasMoveGoal` | - | Does entity have a movement goal? |
| `CanAttack` | - | Can entity attack now (cooldown ready)? |
| `IsWaitTimerExpired` | - | Has wait timer finished? |
| `HasNavigableDestination` | - | Is destination reachable? |
| `HasValidPath` | - | Does entity have a valid navigation path? |
| `HasReachedDestination` | - | Has entity reached its destination? |

**Example**:
```json
{
    "type": "Condition",
    "name": "Is Player Nearby?",
    "conditionType": "TargetInRange",
    "conditionParam": 150.0
}
```

#### Action Nodes

**Purpose**: Perform an action and return Success/Failure/Running

**Available Actions**:

| Action Type | Parameters | Description |
|-------------|------------|-------------|
| `MoveTo` | `param1` (arrival threshold) | Move to moveGoal position |
| `AttackMelee` | - | Perform melee attack on target |
| `SetMoveGoalToTarget` | - | Set moveGoal to target's position |
| `SetMoveGoalToLastKnownTargetPos` | - | Move to last seen target position |
| `SetMoveGoalToPatrolPoint` | - | Set goal to current patrol waypoint |
| `PatrolPickNextPoint` | - | Advance to next patrol point |
| `ClearTarget` | - | Clear current target |
| `Idle` | - | Do nothing (always succeeds) |
| `WaitRandomTime` | `param1` (min), `param2` (max) | Wait random duration (seconds) |
| `ChooseRandomNavigablePoint` | `param1` (radius), `param2` (attempts) | Choose random walkable position |
| `RequestPathfinding` | - | Calculate navigation path to moveGoal |
| `FollowPath` | - | Follow computed navigation path |

**Example**:
```json
{
    "type": "Action",
    "name": "Wait 2-5 seconds",
    "actionType": "WaitRandomTime",
    "actionParam1": 2.0,
    "actionParam2": 5.0
}
```

### Decorator Nodes

Decorators modify a single child's behavior.

#### Inverter

**Purpose**: Invert child's result (Success ↔ Failure)

```json
{
    "type": "Inverter",
    "name": "If NOT has target",
    "decoratorChildId": 2
}
```

**Use Case**: "Do X if condition is false"

#### Repeater

**Purpose**: Repeat child N times or indefinitely

```json
{
    "type": "Repeater",
    "name": "Loop Forever",
    "decoratorChildId": 2,
    "repeatCount": -1
}
```

- `repeatCount: -1` = infinite loop
- `repeatCount: 5` = repeat 5 times

---

## Part 3: Creating Your First Behavior Tree

Let's create a simple idle behavior where an NPC just stands still.

### Step 1: Create Idle Behavior JSON

Create `Blueprints/AI/my_idle_behavior.json`:

```json
{
    "schema_version": 2,
    "type": "BehaviorTree",
    "blueprintType": "BehaviorTree",
    "name": "IdleBehavior",
    "description": "NPC stands idle, doing nothing",
    "metadata": {
        "author": "YourName",
        "created": "2025-01-15T00:00:00Z",
        "tags": ["AI", "BehaviorTree", "Idle"]
    },
    "editorState": {
        "zoom": 1.0,
        "scrollOffset": {"x": 0, "y": 0}
    },
    "data": {
        "rootNodeId": 1,
        "nodes": [
            {
                "id": 1,
                "name": "Idle Action",
                "type": "Action",
                "position": {"x": 200.0, "y": 100.0},
                "actionType": "Idle",
                "parameters": {}
            }
        ]
    }
}
```

### Step 2: Create NPC Entity with Behavior Tree

Create `Blueprints/EntityPrefab/IdleNPC.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "IdleNPC",
    "description": "An NPC that stands idle",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Idle NPC",
                "tag": "NPC",
                "type": "IdleNPC",
                "entityType": "NPC"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {"x": 0.0, "y": 0.0, "z": 20000.0}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": -16.0, "y": -32.0, "w": 32.0, "h": 64.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/NPCs/villager.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 64.0},
                "hotSpot": {"x": 16.0, "y": 56.0}
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 0.0,
                "attackRadius": 0.0
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/my_idle_behavior.json"
            }
        }
    ]
}
```

### Step 3: Spawn and Test

```cpp
#include "PrefabFactory.h"

void TestIdleNPC()
{
    PrefabFactory::Get().PreloadAllPrefabs();
    
    EntityID npc = PrefabFactory::Get().CreateEntityFromPrefabName("IdleNPC");
    
    if (npc != INVALID_ENTITY_ID) {
        SYSTEM_LOG << "Spawned idle NPC: " << npc << std::endl;
        
        // Position the NPC
        World& world = World::GetInstance();
        Position_data* pos = world.GetComponent<Position_data>(npc);
        if (pos) {
            pos->position = Vector(400.0f, 300.0f, 20000.0f);
        }
    }
}
```

---

## Part 4: Creating a Wander Behavior

Wandering NPCs randomly explore an area.

### Step 1: Understand Wander Logic

```
Loop forever:
├─ Wait random time (1-3 seconds)
├─ Choose random navigable point within 300 units
├─ Request pathfinding to that point
└─ Follow the path until destination reached
```

### Step 2: Create Wander Behavior Tree

Create `Blueprints/AI/wander_behavior.json`:

```json
{
    "schema_version": 2,
    "type": "BehaviorTree",
    "blueprintType": "BehaviorTree",
    "name": "WanderBehavior",
    "description": "NPC wanders randomly with pathfinding",
    "metadata": {
        "author": "YourName",
        "created": "2025-01-15T00:00:00Z",
        "tags": ["AI", "BehaviorTree", "Wander", "Pathfinding"]
    },
    "editorState": {
        "zoom": 1.0,
        "scrollOffset": {"x": 0, "y": 0}
    },
    "data": {
        "rootNodeId": 1,
        "nodes": [
            {
                "id": 1,
                "name": "Wander Loop",
                "type": "Repeater",
                "position": {"x": 200.0, "y": 100.0},
                "decoratorChildId": 2,
                "repeatCount": -1,
                "parameters": {}
            },
            {
                "id": 2,
                "name": "Wander Sequence",
                "type": "Sequence",
                "position": {"x": 200.0, "y": 300.0},
                "children": [3, 4, 5, 6],
                "parameters": {}
            },
            {
                "id": 3,
                "name": "Wait 1-3 seconds",
                "type": "Action",
                "position": {"x": 550.0, "y": 300.0},
                "actionType": "WaitRandomTime",
                "parameters": {
                    "param1": 1.0,
                    "param2": 3.0
                }
            },
            {
                "id": 4,
                "name": "Choose Random Point (300 units)",
                "type": "Action",
                "position": {"x": 550.0, "y": 500.0},
                "actionType": "ChooseRandomNavigablePoint",
                "parameters": {
                    "param1": 300.0,
                    "param2": 10.0
                }
            },
            {
                "id": 5,
                "name": "Request Pathfinding",
                "type": "Action",
                "position": {"x": 550.0, "y": 700.0},
                "actionType": "RequestPathfinding",
                "parameters": {}
            },
            {
                "id": 6,
                "name": "Follow Path",
                "type": "Action",
                "position": {"x": 550.0, "y": 900.0},
                "actionType": "FollowPath",
                "parameters": {}
            }
        ]
    }
}
```

### Step 3: Understanding the Wander Tree

**Node Breakdown**:

1. **Repeater (id: 1)**: Loops the wander sequence forever
2. **Sequence (id: 2)**: Executes wander steps in order
3. **WaitRandomTime (id: 3)**: Pauses 1-3 seconds before moving
4. **ChooseRandomNavigablePoint (id: 4)**: 
   - `param1 = 300.0`: Search within 300 units
   - `param2 = 10.0`: Try up to 10 times to find valid point
5. **RequestPathfinding (id: 5)**: Calculate path to chosen point
6. **FollowPath (id: 6)**: Walk along the computed path

### Step 4: Create Wandering NPC Prefab

Create `Blueprints/EntityPrefab/WanderingNPC.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "WanderingNPC",
    "description": "An NPC that wanders randomly",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Wandering NPC",
                "tag": "NPC",
                "type": "WanderingNPC",
                "entityType": "NPC"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {"x": 0.0, "y": 0.0, "z": 20000.0}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": -12.0, "y": -28.0, "w": 24.0, "h": 48.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/NPCs/wanderer.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 48.0},
                "hotSpot": {"x": 16.0, "y": 40.0}
            }
        },
        {
            "type": "Movement",
            "properties": {
                "direction": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 1.0,
                "speed": 60.0
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 0.0,
                "attackRadius": 0.0
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/wander_behavior.json"
            }
        }
    ]
}
```

**Important**: Notice the **Movement** and **PhysicsBody** components—these are required for movement actions to work!

---

## Part 5: Creating a Patrol Behavior

Patrol AI follows a predefined route of waypoints.

### Step 1: Understand Patrol Logic

```
Loop forever:
├─ Pick next patrol point
├─ Set move goal to patrol point
└─ Move to goal
```

### Step 2: Create Patrol Behavior Tree

Create `Blueprints/AI/patrol_behavior.json`:

```json
{
    "schema_version": 2,
    "type": "BehaviorTree",
    "blueprintType": "BehaviorTree",
    "name": "PatrolBehavior",
    "description": "NPC patrols between waypoints",
    "metadata": {
        "author": "YourName",
        "created": "2025-01-15T00:00:00Z",
        "tags": ["AI", "BehaviorTree", "Patrol"]
    },
    "editorState": {
        "zoom": 1.0,
        "scrollOffset": {"x": 0, "y": 0}
    },
    "data": {
        "rootNodeId": 1,
        "nodes": [
            {
                "id": 1,
                "name": "Patrol Loop",
                "type": "Repeater",
                "position": {"x": 200.0, "y": 100.0},
                "decoratorChildId": 2,
                "repeatCount": -1,
                "parameters": {}
            },
            {
                "id": 2,
                "name": "Patrol Sequence",
                "type": "Sequence",
                "position": {"x": 200.0, "y": 300.0},
                "children": [3, 4, 5],
                "parameters": {}
            },
            {
                "id": 3,
                "name": "Pick Next Patrol Point",
                "type": "Action",
                "position": {"x": 550.0, "y": 300.0},
                "actionType": "PatrolPickNextPoint",
                "parameters": {}
            },
            {
                "id": 4,
                "name": "Set Move Goal to Patrol Point",
                "type": "Action",
                "position": {"x": 550.0, "y": 500.0},
                "actionType": "SetMoveGoalToPatrolPoint",
                "parameters": {}
            },
            {
                "id": 5,
                "name": "Move to Goal",
                "type": "Action",
                "position": {"x": 550.0, "y": 700.0},
                "actionType": "MoveToGoal",
                "parameters": {
                    "param1": 10.0
                }
            }
        ]
    }
}
```

### Step 3: Create Patrol Guard Prefab

Create `Blueprints/EntityPrefab/PatrolGuard.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "PatrolGuard",
    "description": "A guard that patrols waypoints",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Patrol Guard",
                "tag": "NPC",
                "type": "Guard",
                "entityType": "NPC"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {"x": 0.0, "y": 0.0, "z": 20000.0}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": -12.0, "y": -28.0, "w": 24.0, "h": 48.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/NPCs/guard.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 48.0},
                "hotSpot": {"x": 16.0, "y": 40.0}
            }
        },
        {
            "type": "Movement",
            "properties": {
                "direction": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 1.5,
                "speed": 80.0
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 150.0,
                "attackRadius": 0.0,
                "currentPatrolIndex": 0,
                "patrolPoints": [
                    {"x": 100.0, "y": 100.0},
                    {"x": 400.0, "y": 100.0},
                    {"x": 400.0, "y": 400.0},
                    {"x": 100.0, "y": 400.0}
                ]
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/patrol_behavior.json"
            }
        }
    ]
}
```

**Key**: The `patrolPoints` array defines the waypoints the guard follows!

### Step 4: Spawn Patrol Guard

```cpp
void SpawnPatrolGuard()
{
    EntityID guard = PrefabFactory::Get().CreateEntityFromPrefabName("PatrolGuard");
    
    if (guard != INVALID_ENTITY_ID) {
        SYSTEM_LOG << "Spawned patrol guard" << std::endl;
        
        // Override starting position
        World& world = World::GetInstance();
        Position_data* pos = world.GetComponent<Position_data>(guard);
        if (pos) {
            pos->position = Vector(100.0f, 100.0f, 20000.0f);
        }
    }
}
```

---

## Part 6: Creating Combat AI

Combine detection, chasing, and attacking for a complete enemy AI.

### Step 1: Understand Combat Logic

```
Selector (Choose first that succeeds):
├─ Sequence: Attack if close
│  ├─ Has target?
│  ├─ Target in attack range?
│  └─ Attack!
├─ Sequence: Chase if visible
│  ├─ Has target?
│  ├─ Set goal to target
│  └─ Move to goal
└─ Patrol (fallback behavior)
```

### Step 2: Create Combat Behavior Tree

Create `Blueprints/AI/combat_behavior.json`:

```json
{
    "schema_version": 2,
    "type": "BehaviorTree",
    "blueprintType": "BehaviorTree",
    "name": "CombatBehavior",
    "description": "Enemy that patrols, detects player, and attacks",
    "metadata": {
        "author": "YourName",
        "created": "2025-01-15T00:00:00Z",
        "tags": ["AI", "BehaviorTree", "Combat", "Enemy"]
    },
    "editorState": {
        "zoom": 1.0,
        "scrollOffset": {"x": 0, "y": 0}
    },
    "data": {
        "rootNodeId": 1,
        "nodes": [
            {
                "id": 1,
                "name": "Root Selector",
                "type": "Selector",
                "position": {"x": 200.0, "y": 100.0},
                "children": [2, 6, 10],
                "parameters": {}
            },
            {
                "id": 2,
                "name": "Attack Sequence",
                "type": "Sequence",
                "position": {"x": 550.0, "y": 100.0},
                "children": [3, 4, 5],
                "parameters": {}
            },
            {
                "id": 3,
                "name": "Has Target?",
                "type": "Condition",
                "position": {"x": 900.0, "y": 100.0},
                "conditionType": "TargetVisible",
                "parameters": {}
            },
            {
                "id": 4,
                "name": "In Attack Range?",
                "type": "Condition",
                "position": {"x": 900.0, "y": 250.0},
                "conditionType": "TargetInRange",
                "conditionParam": 50.0,
                "parameters": {}
            },
            {
                "id": 5,
                "name": "Melee Attack",
                "type": "Action",
                "position": {"x": 900.0, "y": 400.0},
                "actionType": "AttackMelee",
                "parameters": {}
            },
            {
                "id": 6,
                "name": "Chase Sequence",
                "type": "Sequence",
                "position": {"x": 550.0, "y": 550.0},
                "children": [7, 8, 9],
                "parameters": {}
            },
            {
                "id": 7,
                "name": "Has Target?",
                "type": "Condition",
                "position": {"x": 900.0, "y": 550.0},
                "conditionType": "TargetVisible",
                "parameters": {}
            },
            {
                "id": 8,
                "name": "Set Goal to Target",
                "type": "Action",
                "position": {"x": 900.0, "y": 700.0},
                "actionType": "SetMoveGoalToTarget",
                "parameters": {}
            },
            {
                "id": 9,
                "name": "Move to Target",
                "type": "Action",
                "position": {"x": 900.0, "y": 850.0},
                "actionType": "MoveTo",
                "parameters": {
                    "param1": 10.0
                }
            },
            {
                "id": 10,
                "name": "Patrol (Fallback)",
                "type": "Sequence",
                "position": {"x": 550.0, "y": 1000.0},
                "children": [11, 12, 13],
                "parameters": {}
            },
            {
                "id": 11,
                "name": "Pick Patrol Point",
                "type": "Action",
                "position": {"x": 900.0, "y": 1000.0},
                "actionType": "PatrolPickNextPoint",
                "parameters": {}
            },
            {
                "id": 12,
                "name": "Set Goal to Patrol",
                "type": "Action",
                "position": {"x": 900.0, "y": 1150.0},
                "actionType": "SetMoveGoalToPatrolPoint",
                "parameters": {}
            },
            {
                "id": 13,
                "name": "Move to Patrol Point",
                "type": "Action",
                "position": {"x": 900.0, "y": 1300.0},
                "actionType": "MoveToGoal",
                "parameters": {
                    "param1": 10.0
                }
            }
        ]
    }
}
```

### Step 3: Create Combat Enemy Prefab

Create `Blueprints/EntityPrefab/CombatEnemy.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "CombatEnemy",
    "description": "An enemy that patrols and attacks the player",
    "components": [
        {
            "type": "Identity",
            "properties": {
                "name": "Combat Enemy",
                "tag": "Enemy",
                "type": "CombatEnemy",
                "entityType": "Enemy"
            }
        },
        {
            "type": "Position",
            "properties": {
                "position": {"x": 0.0, "y": 0.0, "z": 20000.0}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": -16.0, "y": -28.0, "w": 32.0, "h": 48.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/Enemies/soldier.png",
                "srcRect": {"x": 0.0, "y": 0.0, "w": 32.0, "h": 48.0},
                "hotSpot": {"x": 16.0, "y": 40.0}
            }
        },
        {
            "type": "Movement",
            "properties": {
                "direction": {"x": 0.0, "y": 0.0},
                "velocity": {"x": 0.0, "y": 0.0}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 2.0,
                "speed": 120.0
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 100,
                "maxHealth": 100
            }
        },
        {
            "type": "Combat",
            "properties": {
                "attackDamage": 20,
                "attackCooldown": 1.0,
                "lastAttackTime": 0.0
            }
        },
        {
            "type": "AIBlackboard",
            "properties": {
                "detectionRadius": 200.0,
                "attackRadius": 50.0,
                "currentPatrolIndex": 0,
                "patrolPoints": [
                    {"x": 200.0, "y": 200.0},
                    {"x": 600.0, "y": 200.0},
                    {"x": 600.0, "y": 600.0},
                    {"x": 200.0, "y": 600.0}
                ]
            }
        },
        {
            "type": "AIBehaviorTree",
            "properties": {
                "behaviorTreePath": "Blueprints/AI/combat_behavior.json"
            }
        }
    ]
}
```

### Step 4: How Detection Works

The AI system automatically sets `targetEntity` in the blackboard when:

1. An entity with "Player" tag is within `detectionRadius`
2. Line of sight is clear (no obstacles)

The behavior tree checks `TargetVisible` condition which reads `targetEntity` from the blackboard.

---

## Part 7: Advanced Techniques

### Using Inverter for "If NOT"

Check if entity does NOT have a target:

```json
{
    "id": 1,
    "name": "If NO Target",
    "type": "Inverter",
    "position": {"x": 200.0, "y": 100.0},
    "decoratorChildId": 2,
    "parameters": {}
},
{
    "id": 2,
    "name": "Has Target?",
    "type": "Condition",
    "position": {"x": 200.0, "y": 300.0},
    "conditionType": "TargetVisible",
    "parameters": {}
}
```

### Combining Wander and Combat

Create a hybrid behavior:

```json
{
    "id": 1,
    "name": "Root Selector",
    "type": "Selector",
    "children": [2, 7],
    "comment": "Attack if player nearby, otherwise wander"
},
{
    "id": 2,
    "name": "Combat Sequence",
    "type": "Sequence",
    "children": [3, 4, 5, 6]
},
{
    "id": 3,
    "type": "Condition",
    "conditionType": "TargetVisible"
},
{
    "id": 4,
    "type": "Action",
    "actionType": "SetMoveGoalToTarget"
},
{
    "id": 5,
    "type": "Action",
    "actionType": "MoveTo"
},
{
    "id": 6,
    "type": "Action",
    "actionType": "AttackMelee"
},
{
    "id": 7,
    "name": "Wander Sequence",
    "type": "Sequence",
    "children": [8, 9, 10, 11],
    "comment": "Fallback to wandering"
}
```

### Flee Behavior

Run away when health is low:

```json
{
    "id": 1,
    "name": "Flee Sequence",
    "type": "Sequence",
    "children": [2, 3, 4, 5]
},
{
    "id": 2,
    "name": "Health Below 30%?",
    "type": "Condition",
    "conditionType": "HealthBelow",
    "conditionParam": 0.3
},
{
    "id": 3,
    "name": "Choose Escape Point",
    "type": "Action",
    "actionType": "ChooseRandomNavigablePoint",
    "parameters": {
        "param1": 400.0
    }
},
{
    "id": 4,
    "type": "Action",
    "actionType": "RequestPathfinding"
},
{
    "id": 5,
    "type": "Action",
    "actionType": "FollowPath"
}
```

---

## Part 8: Debugging Behavior Trees

### Enable AI Debug Logging

In your code, enable AI system logging:

```cpp
// In your initialization
#define AI_DEBUG_LOGGING
SYSTEM_LOG << "AI Debug enabled" << std::endl;
```

### Check Behavior Tree Loading

```cpp
void DebugBehaviorTree(EntityID entity)
{
    World& world = World::GetInstance();
    
    AIBehaviorTree_data* bt = world.GetComponent<AIBehaviorTree_data>(entity);
    if (!bt) {
        SYSTEM_LOG << "ERROR: Entity has no AIBehaviorTree component!" << std::endl;
        return;
    }
    
    SYSTEM_LOG << "Behavior Tree: " << bt->behaviorTreePath << std::endl;
    SYSTEM_LOG << "  Root Node ID: " << bt->rootNode.id << std::endl;
    SYSTEM_LOG << "  Node Count: " << bt->nodes.size() << std::endl;
}
```

### Visualize AI State

```cpp
void DebugAIBlackboard(EntityID entity)
{
    World& world = World::GetInstance();
    
    AIBlackboard_data* bb = world.GetComponent<AIBlackboard_data>(entity);
    if (!bb) {
        SYSTEM_LOG << "ERROR: Entity has no AIBlackboard!" << std::endl;
        return;
    }
    
    SYSTEM_LOG << "=== AI Blackboard ===" << std::endl;
    SYSTEM_LOG << "  Target: " << bb->targetEntity << std::endl;
    SYSTEM_LOG << "  Move Goal: (" << bb->moveGoal.x << ", " << bb->moveGoal.y << ")" << std::endl;
    SYSTEM_LOG << "  Detection Radius: " << bb->detectionRadius << std::endl;
    SYSTEM_LOG << "  Attack Radius: " << bb->attackRadius << std::endl;
    SYSTEM_LOG << "  Patrol Index: " << bb->currentPatrolIndex 
               << "/" << bb->patrolPoints.size() << std::endl;
}
```

### Common Issues

#### AI Not Moving

**Check**:
- Entity has `Movement_data` and `PhysicsBody_data` components
- `speed` is greater than 0
- Behavior tree has movement actions

#### AI Not Detecting Player

**Check**:
- `detectionRadius` is large enough
- Player entity has tag "Player"
- No obstacles blocking line of sight
- Player is within detection range

#### Behavior Tree Not Loading

**Check**:
- File path is correct in `behaviorTreePath`
- JSON syntax is valid
- File exists in `Blueprints/AI/` directory
- Schema version is 2

---

## Part 9: Best Practices

### Design Tips

1. **Start Simple**: Begin with Idle, add complexity gradually
2. **Test Each Node**: Build tree incrementally, test after each addition
3. **Use Descriptive Names**: Clear node names help debugging
4. **Fallback Behaviors**: Always have a default action (like Idle or Patrol)
5. **Avoid Deep Nesting**: Prefer wide trees over deep trees

### Performance Considerations

1. **Limit Condition Checks**: Don't check expensive conditions every frame
2. **Use Appropriate Detection Radii**: Smaller = better performance
3. **Reasonable Pathfinding**: Don't pathfind too frequently
4. **Prune Dead Branches**: Remove unused nodes from JSON

### Organization

```
Blueprints/AI/
├── Basic/
│   ├── idle.json
│   ├── wander_behavior.json
│   └── patrol_behavior.json
├── Combat/
│   ├── melee_attack.json
│   ├── ranged_attack.json
│   └── flee_behavior.json
├── NPC/
│   ├── merchant_idle.json
│   ├── villager_daily_routine.json
│   └── quest_giver.json
└── Boss/
    ├── boss_phase1.json
    ├── boss_phase2.json
    └── boss_enraged.json
```

---

## Next Steps

You've learned the fundamentals of behavior trees! Continue with:

### Related Tutorials:
- [**Creating Entities**](./creating-entities.md) - Build NPCs with AI components
- [**First Tiled Map**](./first-tiled-map.md) - Place AI entities in levels

### Advanced Topics:
- [**Custom Behavior Nodes**](../technical-reference/behavior-trees/custom-nodes.md) - Extend the system
- [**AI Systems Deep Dive**](../technical-reference/architecture/ai-systems.md) - How AI processing works
- [**Pathfinding Guide**](../user-guide/navigation/pathfinding.md) - Navigation system details

### Reference:
- [**Behavior Tree Nodes**](../technical-reference/behavior-trees/nodes.md) - Complete node reference
- [**Behavior Tree Overview**](../technical-reference/behavior-trees/overview.md) - System architecture
- [**AI Components**](../api-reference/components.md) - AIBlackboard and AIBehaviorTree details

---

## Summary

In this tutorial, you learned:

✅ Behavior tree fundamentals and node types  
✅ Creating JSON behavior tree files  
✅ Implementing idle, wander, and patrol behaviors  
✅ Building combat AI with detection and attacking  
✅ Using Selectors, Sequences, Conditions, and Actions  
✅ Debugging and testing behavior trees  
✅ Best practices for AI design  

You can now create sophisticated AI behaviors for your Olympe Engine games!

---

## Additional Resources

- **Example Behaviors**: `Blueprints/AI/` directory
- **Example Code**: `Examples/AI_Example.cpp`
- **Behavior Tree Source**: `Source/AI/BehaviorTree.h`
- **AI Systems**: `Source/ECS_Systems_AI.h`

**Happy AI Designing! 🤖**
