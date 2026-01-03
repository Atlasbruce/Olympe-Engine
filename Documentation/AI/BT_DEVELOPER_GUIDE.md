# Behavior Tree AI Developer Guide

> **Olympe Engine V2 - 2025**  
> ECS-based Behavior Tree AI System

---

## Overview

This guide explains how to create, configure, and test AI NPCs using the Behavior Tree (BT) system integrated into Olympe Engine's ECS architecture.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Creating a New NPC with AI](#creating-a-new-npc-with-ai)
3. [Defining Behavior Trees](#defining-behavior-trees)
4. [Configuring Patrol Waypoints](#configuring-patrol-waypoints)
5. [Testing and Debugging](#testing-and-debugging)
6. [Available Actions and Conditions](#available-actions-and-conditions)

---

## Quick Start

### Prerequisites

Behavior trees are automatically loaded at engine initialization from:
- `Blueprints/AI/idle.json` (Tree ID: 1)
- `Blueprints/AI/guard_patrol.json` (Tree ID: 2)
- `Blueprints/AI/guard_combat.json` (Tree ID: 3)
- `Blueprints/AI/investigate.json` (Tree ID: 5)

### Example: Creating a Guard NPC

```cpp
#include "World.h"
#include "ECS_Components.h"
#include "ECS_Components_AI.h"
#include "prefabfactory.h"

// Create guard using the prefab
EntityID guard = PrefabFactory::Get().CreateEntity("GuardNPC");

// Set position
Position_data& pos = World::Get().GetComponent<Position_data>(guard);
pos.position = Vector(400.0f, 300.0f, 0.0f);

// Configure patrol waypoints
AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(guard);
blackboard.patrolPoints[0] = Vector(300.0f, 200.0f, 0.0f);
blackboard.patrolPoints[1] = Vector(500.0f, 200.0f, 0.0f);
blackboard.patrolPoints[2] = Vector(500.0f, 400.0f, 0.0f);
blackboard.patrolPoints[3] = Vector(300.0f, 400.0f, 0.0f);
blackboard.patrolPointCount = 4;
```

---

## Creating a New NPC with AI

### Step 1: Register a Prefab

Add your NPC prefab in `VideoGame::RegisterPrefabItems()`:

```cpp
PrefabFactory::Get().RegisterPrefab("MyNPC", [](EntityID id) {
    World& world = World::Get();
    
    // Core components
    world.AddComponent<Position_data>(id, Vector(0, 0, 0));
    world.AddComponent<VisualSprite_data>(id, /* sprite data */);
    world.AddComponent<Movement_data>(id);
    world.AddComponent<PhysicsBody_data>(id, 1.0f, 120.0f);
    world.AddComponent<Health_data>(id, 100, 100);
    
    // AI components (required for BT AI)
    world.AddComponent<AIBlackboard_data>(id);
    world.AddComponent<AISenses_data>(id);
    world.AddComponent<AIState_data>(id);
    world.AddComponent<BehaviorTreeRuntime_data>(id);
    world.AddComponent<MoveIntent_data>(id);
    world.AddComponent<AttackIntent_data>(id);
    
    // Configure AI
    AISenses_data& senses = world.GetComponent<AISenses_data>(id);
    senses.visionRadius = 300.0f;
    senses.hearingRadius = 500.0f;
    
    AIState_data& state = world.GetComponent<AIState_data>(id);
    state.currentMode = AIMode::Patrol;
    
    BehaviorTreeRuntime_data& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(id);
    btRuntime.treeAssetId = 2; // Patrol tree
    btRuntime.isActive = true;
});
```

### Step 2: Spawn the NPC

```cpp
EntityID npc = PrefabFactory::Get().CreateEntity("MyNPC");
```

---

## Defining Behavior Trees

### JSON Structure

Behavior trees are defined in JSON format:

```json
{
    "name": "MyBehaviorTree",
    "rootNodeId": 1,
    "nodes": [
        {
            "id": 1,
            "name": "Root Selector",
            "type": "Selector",
            "children": [2, 3]
        },
        {
            "id": 2,
            "name": "Has Target?",
            "type": "Condition",
            "conditionType": "HasTarget"
        },
        {
            "id": 3,
            "name": "Move To Goal",
            "type": "Action",
            "actionType": "MoveTo",
            "param1": 1.0
        }
    ]
}
```

### Node Types

- **Selector**: OR node - succeeds if any child succeeds
- **Sequence**: AND node - succeeds if all children succeed
- **Condition**: Checks a condition (leaf node)
- **Action**: Performs an action (leaf node)
- **Inverter**: Decorator - inverts child result
- **Repeater**: Decorator - repeats child execution

### Loading Custom Trees

```cpp
BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/my_tree.json", 10);
```

Tree IDs are mapped to AI modes in `AIStateTransitionSystem`:
- 1: Idle
- 2: Patrol
- 3: Combat
- 4: Flee
- 5: Investigate

---

## Configuring Patrol Waypoints

### Setting Waypoints

The `AIBlackboard_data` component holds patrol waypoints:

```cpp
AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);

// Define up to 8 waypoints
blackboard.patrolPoints[0] = Vector(100.0f, 100.0f, 0.0f);
blackboard.patrolPoints[1] = Vector(200.0f, 100.0f, 0.0f);
blackboard.patrolPoints[2] = Vector(200.0f, 200.0f, 0.0f);
blackboard.patrolPoints[3] = Vector(100.0f, 200.0f, 0.0f);
blackboard.patrolPointCount = 4;
blackboard.currentPatrolPoint = 0; // Start at first waypoint
```

### Coordinate System

- Coordinates are in screen space
- Units are pixels
- Typical conversion: 100 units = 1 meter

### Patrol Behavior

The patrol behavior tree uses these actions:
1. `PatrolPickNextPoint` - Cycles to next waypoint
2. `SetMoveGoalToPatrolPoint` - Sets current patrol point as move goal
3. `MoveToGoal` - Moves toward the goal

Example patrol tree:
```json
{
    "name": "GuardPatrolTree",
    "rootNodeId": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Sequence",
            "children": [2, 3, 4]
        },
        {
            "id": 2,
            "type": "Action",
            "actionType": "PatrolPickNextPoint"
        },
        {
            "id": 3,
            "type": "Action",
            "actionType": "SetMoveGoalToPatrolPoint"
        },
        {
            "id": 4,
            "type": "Action",
            "actionType": "MoveTo",
            "param1": 0.5
        }
    ]
}
```

---

## Testing and Debugging

### Debug Logging

The BT system logs execution every 2 seconds:

```
BT[Entity 12345]: Mode=Patrol, Tree=2, Node=Move to Goal, Status=Running, Target=0, Dist=0
```

### Testing Detection Range

Configure detection in `AISenses_data`:

```cpp
AISenses_data& senses = World::Get().GetComponent<AISenses_data>(entity);
senses.visionRadius = 200.0f; // Detection range (200 units = ~2m)
senses.visionAngle = 180.0f;  // Field of view in degrees
```

### Testing Attack Behavior

The combat behavior tree engages when:
1. NPC has a target (player detected)
2. Target is within attack range

```cpp
AIState_data& state = World::Get().GetComponent<AIState_data>(entity);
state.combatEngageDistance = 200.0f; // Switch to combat mode at 2m
```

### Validation and Hot-Reload

Trees are validated on load:
```cpp
std::string errorMessage;
if (!BehaviorTreeManager::Get().ValidateTree(tree, errorMessage))
{
    SYSTEM_LOG << "Validation error: " << errorMessage << "\n";
}
```

Hot-reload a tree:
```cpp
BehaviorTreeManager::Get().ReloadTree(2); // Reload patrol tree
```

---

## Available Actions and Conditions

### Conditions

| Type | Alias | Description | Parameter |
|------|-------|-------------|-----------|
| `HasTarget` | `TargetVisible` | Checks if NPC has a visible target | - |
| `IsTargetInAttackRange` | `TargetInRange` | Checks if target is within range | `param`: range in units |
| `HealthBelow` | - | Checks if health is below threshold | `param`: health percentage (0-1) |
| `HasMoveGoal` | - | Checks if NPC has a movement goal | - |
| `CanAttack` | - | Checks if attack is off cooldown | - |
| `HeardNoise` | - | Checks if NPC heard a noise | - |

### Actions

| Type | Alias | Description | Parameters |
|------|-------|-------------|------------|
| `MoveTo` | `MoveToGoal` | Move toward current goal | `param1`: speed multiplier |
| `AttackMelee` | `AttackIfClose` | Melee attack if in range | `param1`: range, `param2`: damage |
| `SetMoveGoalToTarget` | - | Set move goal to target position | - |
| `SetMoveGoalToPatrolPoint` | - | Set move goal to current patrol point | - |
| `PatrolPickNextPoint` | - | Advance to next patrol waypoint | - |
| `ClearTarget` | - | Clear current target | - |
| `Idle` | - | Do nothing | - |

### Example: Guard Behavior

```json
{
    "name": "GuardCombatTree",
    "rootNodeId": 1,
    "nodes": [
        {
            "id": 1,
            "type": "Selector",
            "children": [2, 5]
        },
        {
            "id": 2,
            "type": "Sequence",
            "children": [3, 4]
        },
        {
            "id": 3,
            "type": "Condition",
            "conditionType": "IsTargetInAttackRange",
            "param": 60.0
        },
        {
            "id": 4,
            "type": "Action",
            "actionType": "AttackMelee",
            "param1": 60.0,
            "param2": 15.0
        },
        {
            "id": 5,
            "type": "Action",
            "actionType": "SetMoveGoalToTarget"
        }
    ]
}
```

---

## Acceptance Demo: "Garde" NPC

The test scene includes a guard NPC named "garde" that demonstrates the full AI system:

### Behavior
1. **Patrol**: Guard patrols 4 waypoints in a square pattern
2. **Detection**: If player comes within 200 units (~2m), guard detects player
3. **Combat**: Guard switches to combat mode, chases and attacks player
4. **Resume**: After losing sight of player for 5 seconds, guard resumes patrol

### Testing
1. Run the engine
2. Add a player entity (press F1 or use menu)
3. Move player near the guard (within 200 units)
4. Observe guard behavior in logs:
   - Patrol mode -> Combat mode transition
   - Chase and attack behavior
   - Return to patrol after player leaves

---

## Tips and Best Practices

1. **Performance**: Use `perceptionHz` and `thinkHz` in `AISenses_data` to control update frequency
2. **Waypoints**: Keep patrol paths simple (4-8 waypoints)
3. **Detection**: Adjust `visionRadius` based on game design (100 units = ~1m)
4. **Testing**: Use debug logs to verify BT execution
5. **Validation**: Always validate trees after editing JSON files

---

## Troubleshooting

### NPC Not Moving
- Check that `Movement_data` and `PhysicsBody_data` components exist
- Verify patrol waypoints are set (`patrolPointCount > 0`)
- Ensure BT tree ID is valid and loaded

### NPC Not Detecting Player
- Check `PlayerBinding_data` component exists on player
- Verify `visionRadius` is large enough
- Check AI perception update frequency (`perceptionHz`)

### BT Validation Errors
- Ensure all referenced node IDs exist
- Check that composite nodes have children
- Verify decorator nodes have a child

---

## See Also

- [AI Architecture](../../ARCHITECTURE.md#ai-system)
- [ECS Components Guide](../ECS/COMPONENTS_GUIDE.md)
- [Example Code](../../Examples/AI_Example.cpp)

---

**Olympe Engine Team - 2025**
