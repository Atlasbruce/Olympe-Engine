---
id: bt-nodes
title: Behavior Tree Nodes
sidebar_label: Node Types
sidebar_position: 1
---

# Behavior Tree Nodes Reference

**Version:** 2.0  
**Last Updated:** 2025-02-13

## Overview

Olympe Engine's behavior tree system provides a comprehensive set of node types for building sophisticated AI behaviors. This reference documents all available node types, their parameters, execution semantics, and usage examples.

The behavior tree system is designed for:
- **Data-driven AI**: Define behavior trees in JSON files
- **Modular composition**: Combine simple nodes into complex behaviors
- **Hot-reload support**: Update behavior trees without restarting
- **ECS integration**: Seamless integration with entity components

## Node Type Hierarchy

```
BTNode (Base)
├── Composite Nodes (Multiple children)
│   ├── Selector (OR logic)
│   └── Sequence (AND logic)
├── Decorator Nodes (Single child modifier)
│   ├── Inverter (Negates result)
│   └── Repeater (Repeats execution)
└── Leaf Nodes (Terminal execution)
    ├── Condition (Checks state)
    └── Action (Performs operations)
```

## Node Status Types

Every node execution returns one of three status values:

```cpp
enum class BTStatus : uint8_t
{
    Running = 0,   // Node is still executing (multi-frame)
    Success,       // Node completed successfully
    Failure        // Node failed
};
```

### Status Semantics

| Status | Meaning | Use Cases |
|--------|---------|-----------|
| `Running` | Node continues across frames | Movement, timers, animations |
| `Success` | Node completed successfully | Conditions met, actions completed |
| `Failure` | Node failed to complete | Conditions not met, invalid state |

---

## Composite Nodes

Composite nodes have multiple children and control the flow of execution through the tree.

### Selector Node

**Type:** `BTNodeType::Selector`  
**Logic:** OR operation - succeeds if **any** child succeeds

#### Execution Flow

1. Execute children **left-to-right**
2. If a child returns `Success`, immediately return `Success`
3. If a child returns `Running`, immediately return `Running`
4. If a child returns `Failure`, continue to next child
5. If all children fail, return `Failure`

#### Use Cases

- **Fallback behaviors**: Try preferred action, fall back to alternatives
- **Priority selection**: Choose first viable option from list
- **Decision making**: Select one behavior from multiple options

#### Example Structure

```json
{
  "id": 1,
  "type": "Selector",
  "name": "Combat or Patrol",
  "children": [2, 3]
}
```

#### Code Implementation

```cpp
case BTNodeType::Selector:
{
    // OR node: succeeds if any child succeeds
    for (uint32_t childId : node.childIds)
    {
        const BTNode* child = tree.GetNode(childId);
        if (!child) continue;
        
        BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
        if (status == BTStatus::Success)
            return BTStatus::Success;
        if (status == BTStatus::Running)
            return BTStatus::Running;
    }
    return BTStatus::Failure;
}
```

#### Example: Combat AI with Fallback

```json
{
  "name": "Combat AI",
  "rootNodeId": 1,
  "nodes": [
    {
      "id": 1,
      "type": "Selector",
      "name": "Root Selector",
      "children": [2, 6]
    },
    {
      "id": 2,
      "type": "Sequence",
      "name": "Attack Sequence",
      "children": [3, 4, 5]
    },
    {
      "id": 3,
      "type": "Condition",
      "name": "Has Target?",
      "conditionType": "HasTarget"
    },
    {
      "id": 4,
      "type": "Condition",
      "name": "In Attack Range?",
      "conditionType": "IsTargetInAttackRange",
      "param": 50.0
    },
    {
      "id": 5,
      "type": "Action",
      "name": "Attack",
      "actionType": "AttackMelee",
      "param1": 50.0,
      "param2": 15.0
    },
    {
      "id": 6,
      "type": "Action",
      "name": "Idle",
      "actionType": "Idle"
    }
  ]
}
```

**Behavior:** Try to attack if target exists and is in range, otherwise idle.

---

### Sequence Node

**Type:** `BTNodeType::Sequence`  
**Logic:** AND operation - succeeds only if **all** children succeed

#### Execution Flow

1. Execute children **left-to-right**
2. If a child returns `Failure`, immediately return `Failure`
3. If a child returns `Running`, immediately return `Running`
4. If a child returns `Success`, continue to next child
5. If all children succeed, return `Success`

#### Use Cases

- **Multi-step behaviors**: Execute actions in order
- **Conditional execution**: Check conditions before acting
- **Complex tasks**: Combine simple actions into complex behavior

#### Example Structure

```json
{
  "id": 1,
  "type": "Sequence",
  "name": "Chase and Attack",
  "children": [2, 3, 4]
}
```

#### Code Implementation

```cpp
case BTNodeType::Sequence:
{
    // AND node: succeeds if all children succeed
    for (uint32_t childId : node.childIds)
    {
        const BTNode* child = tree.GetNode(childId);
        if (!child) continue;
        
        BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
        if (status == BTStatus::Failure)
            return BTStatus::Failure;
        if (status == BTStatus::Running)
            return BTStatus::Running;
    }
    return BTStatus::Success;
}
```

#### Example: Patrol Sequence

```json
{
  "name": "Patrol Behavior",
  "rootNodeId": 1,
  "nodes": [
    {
      "id": 1,
      "type": "Sequence",
      "name": "Patrol Sequence",
      "children": [2, 3, 4]
    },
    {
      "id": 2,
      "type": "Action",
      "name": "Pick Next Patrol Point",
      "actionType": "PatrolPickNextPoint"
    },
    {
      "id": 3,
      "type": "Action",
      "name": "Set Move Goal",
      "actionType": "SetMoveGoalToPatrolPoint"
    },
    {
      "id": 4,
      "type": "Action",
      "name": "Move to Goal",
      "actionType": "MoveTo",
      "param1": 1.0
    }
  ]
}
```

**Behavior:** Pick next patrol point, set it as goal, then move to it.

---

## Decorator Nodes

Decorator nodes modify the behavior of a single child node.

### Inverter Node

**Type:** `BTNodeType::Inverter`  
**Logic:** Inverts the success/failure status of child

#### Status Transformation

| Child Status | Inverter Returns |
|--------------|------------------|
| `Success` | `Failure` |
| `Failure` | `Success` |
| `Running` | `Running` |

#### Use Cases

- **Negative conditions**: "If NOT condition"
- **Failure recovery**: Convert failure to success
- **Logic inversion**: Reverse behavior semantics

#### Example Structure

```json
{
  "id": 5,
  "type": "Inverter",
  "name": "Not In Range",
  "decoratorChildId": 6
}
```

#### Code Implementation

```cpp
case BTNodeType::Inverter:
{
    const BTNode* child = tree.GetNode(node.decoratorChildId);
    if (!child) return BTStatus::Failure;
    
    BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
    if (status == BTStatus::Success)
        return BTStatus::Failure;
    if (status == BTStatus::Failure)
        return BTStatus::Success;
    return status;
}
```

#### Example: Approach Target When Not in Range

```json
{
  "nodes": [
    {
      "id": 1,
      "type": "Sequence",
      "name": "Approach Target",
      "children": [2, 5, 6]
    },
    {
      "id": 2,
      "type": "Condition",
      "name": "Has Target?",
      "conditionType": "HasTarget"
    },
    {
      "id": 5,
      "type": "Inverter",
      "name": "NOT in Range",
      "decoratorChildId": 3
    },
    {
      "id": 3,
      "type": "Condition",
      "name": "In Range?",
      "conditionType": "IsTargetInAttackRange",
      "param": 50.0
    },
    {
      "id": 6,
      "type": "Action",
      "name": "Move to Target",
      "actionType": "SetMoveGoalToTarget"
    }
  ]
}
```

**Behavior:** If has target AND NOT in range, move to target.

---

### Repeater Node

**Type:** `BTNodeType::Repeater`  
**Logic:** Repeats child execution N times

#### Parameters

- `repeatCount` (int): Number of times to repeat child

#### Execution Flow

1. Execute child node
2. If child returns `Running`, return `Running`
3. If child completes (Success/Failure), check repeat count
4. If repeats remaining, execute child again
5. When all repeats complete, return last child status

#### Use Cases

- **Persistent behaviors**: Repeat action until interrupted
- **Sampling**: Execute check multiple times
- **Retry logic**: Attempt action multiple times

#### Example Structure

```json
{
  "id": 10,
  "type": "Repeater",
  "name": "Repeat 3 Times",
  "decoratorChildId": 11,
  "repeatCount": 3
}
```

#### Current Implementation

```cpp
case BTNodeType::Repeater:
{
    // Simplified repeater: just execute once per tick
    const BTNode* child = tree.GetNode(node.decoratorChildId);
    if (!child) return BTStatus::Failure;
    
    return ExecuteBTNode(*child, entity, blackboard, tree);
}
```

:::note Implementation Note
Current repeater implementation is simplified and executes once per tick. Full repeat counting is planned for future versions.
:::

---

## Condition Nodes

Condition nodes are leaf nodes that check game state and return `Success` or `Failure`.

### Built-in Conditions

```cpp
enum class BTConditionType : uint8_t
{
    TargetVisible = 0,           // Alias: HasTarget
    TargetInRange,               // Alias: IsTargetInAttackRange
    HealthBelow,
    HasMoveGoal,
    CanAttack,
    HeardNoise,
    IsWaitTimerExpired,
    HasNavigableDestination,
    HasValidPath,
    HasReachedDestination
};
```

---

### TargetVisible / HasTarget

**Type:** `BTConditionType::TargetVisible`  
**Parameters:** None  
**Returns:** `Success` if entity has a visible target

#### Blackboard Dependencies

```cpp
bool targetVisible;  // Must be set by perception system
```

#### Example

```json
{
  "id": 3,
  "type": "Condition",
  "name": "Has Target?",
  "conditionType": "HasTarget"
}
```

#### Implementation

```cpp
case BTConditionType::TargetVisible:
    return blackboard.targetVisible ? BTStatus::Success : BTStatus::Failure;
```

---

### TargetInRange / IsTargetInAttackRange

**Type:** `BTConditionType::TargetInRange`  
**Parameters:** `param` (float) - Range threshold in world units  
**Returns:** `Success` if target is within specified range

#### Blackboard Dependencies

```cpp
bool hasTarget;              // Must have target
float distanceToTarget;      // Distance to target
```

#### Example

```json
{
  "id": 4,
  "type": "Condition",
  "name": "Target in 50 units?",
  "conditionType": "IsTargetInAttackRange",
  "param": 50.0
}
```

#### Implementation

```cpp
case BTConditionType::TargetInRange:
    if (!blackboard.hasTarget) return BTStatus::Failure;
    return (blackboard.distanceToTarget <= param) ? BTStatus::Success : BTStatus::Failure;
```

---

### HealthBelow

**Type:** `BTConditionType::HealthBelow`  
**Parameters:** `param` (float) - Health percentage threshold (0.0 to 1.0)  
**Returns:** `Success` if health percentage is below threshold

#### Component Dependencies

Requires `Health_data` component on entity.

#### Example

```json
{
  "id": 10,
  "type": "Condition",
  "name": "Health Below 30%?",
  "conditionType": "HealthBelow",
  "param": 0.3
}
```

#### Implementation

```cpp
case BTConditionType::HealthBelow:
    if (World::Get().HasComponent<Health_data>(entity))
    {
        const Health_data& health = World::Get().GetComponent<Health_data>(entity);
        float healthPercent = static_cast<float>(health.currentHealth) / 
                             static_cast<float>(health.maxHealth);
        return (healthPercent < param) ? BTStatus::Success : BTStatus::Failure;
    }
    return BTStatus::Failure;
```

#### Use Case: Retreat When Low Health

```json
{
  "nodes": [
    {
      "id": 1,
      "type": "Selector",
      "name": "Survival Priority",
      "children": [2, 5]
    },
    {
      "id": 2,
      "type": "Sequence",
      "name": "Retreat When Hurt",
      "children": [3, 4]
    },
    {
      "id": 3,
      "type": "Condition",
      "name": "Low Health?",
      "conditionType": "HealthBelow",
      "param": 0.25
    },
    {
      "id": 4,
      "type": "Action",
      "name": "Flee",
      "actionType": "ClearTarget"
    },
    {
      "id": 5,
      "type": "Action",
      "name": "Continue Fighting",
      "actionType": "AttackMelee"
    }
  ]
}
```

---

### HasMoveGoal

**Type:** `BTConditionType::HasMoveGoal`  
**Parameters:** None  
**Returns:** `Success` if entity has a movement goal set

#### Blackboard Dependencies

```cpp
bool hasMoveGoal;  // Set by SetMoveGoal actions
```

#### Example

```json
{
  "id": 7,
  "type": "Condition",
  "name": "Has Move Goal?",
  "conditionType": "HasMoveGoal"
}
```

---

### CanAttack

**Type:** `BTConditionType::CanAttack`  
**Parameters:** None  
**Returns:** `Success` if entity can attack (attack cooldown ready)

#### Blackboard Dependencies

```cpp
bool canAttack;  // Managed by attack cooldown system
```

#### Example

```json
{
  "id": 8,
  "type": "Condition",
  "name": "Can Attack?",
  "conditionType": "CanAttack"
}
```

---

### HeardNoise

**Type:** `BTConditionType::HeardNoise`  
**Parameters:** None  
**Returns:** `Success` if entity heard a noise event

#### Blackboard Dependencies

```cpp
bool heardNoise;  // Set by sound perception system
```

#### Example

```json
{
  "id": 9,
  "type": "Condition",
  "name": "Heard Noise?",
  "conditionType": "HeardNoise"
}
```

---

### IsWaitTimerExpired

**Type:** `BTConditionType::IsWaitTimerExpired`  
**Parameters:** None  
**Returns:** `Success` if wander wait timer has expired

#### Blackboard Dependencies

```cpp
float wanderWaitTimer;        // Current timer value
float wanderTargetWaitTime;   // Target wait time
```

#### Example

```json
{
  "id": 12,
  "type": "Condition",
  "name": "Wait Done?",
  "conditionType": "IsWaitTimerExpired"
}
```

#### Use Case

Used in wander behaviors to wait at waypoints before moving to next location.

---

### HasNavigableDestination

**Type:** `BTConditionType::HasNavigableDestination`  
**Parameters:** None  
**Returns:** `Success` if entity has chosen a navigable wander destination

#### Blackboard Dependencies

```cpp
bool hasWanderDestination;  // Set by ChooseRandomNavigablePoint action
```

---

### HasValidPath

**Type:** `BTConditionType::HasValidPath`  
**Parameters:** None  
**Returns:** `Success` if entity has a valid pathfinding path

#### Component Dependencies

Requires `NavigationAgent_data` component with non-empty path.

#### Example

```json
{
  "id": 14,
  "type": "Condition",
  "name": "Path Found?",
  "conditionType": "HasValidPath"
}
```

---

### HasReachedDestination

**Type:** `BTConditionType::HasReachedDestination`  
**Parameters:** None  
**Returns:** `Success` if entity reached wander destination

#### Implementation

```cpp
case BTConditionType::HasReachedDestination:
    if (!blackboard.hasWanderDestination) return BTStatus::Failure;
    
    if (World::Get().HasComponent<Position_data>(entity))
    {
        const Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        Vector vDest = pos.position;
        vDest -= blackboard.wanderDestination;
        float dist = vDest.Magnitude();
        
        float threshold = 5.0f;
        if (World::Get().HasComponent<MoveIntent_data>(entity))
        {
            const MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
            threshold = intent.arrivalThreshold;
        }
        
        return (dist < threshold) ? BTStatus::Success : BTStatus::Failure;
    }
    return BTStatus::Failure;
```

---

## Action Nodes

Action nodes are leaf nodes that perform operations and may return `Running` for multi-frame execution.

### Built-in Actions

```cpp
enum class BTActionType : uint8_t
{
    SetMoveGoalToLastKnownTargetPos = 0,
    SetMoveGoalToTarget,
    SetMoveGoalToPatrolPoint,
    MoveToGoal,                      // Alias: MoveTo
    AttackIfClose,                   // Alias: AttackMelee
    PatrolPickNextPoint,
    ClearTarget,
    Idle,
    WaitRandomTime,
    ChooseRandomNavigablePoint,
    RequestPathfinding,
    FollowPath
};
```

---

### SetMoveGoalToLastKnownTargetPos

**Type:** `BTActionType::SetMoveGoalToLastKnownTargetPos`  
**Parameters:** None  
**Returns:** `Success` (instant)

Sets movement goal to target's last known position.

#### Example

```json
{
  "id": 20,
  "type": "Action",
  "name": "Go to Last Seen Position",
  "actionType": "SetMoveGoalToLastKnownTargetPos"
}
```

---

### SetMoveGoalToTarget

**Type:** `BTActionType::SetMoveGoalToTarget`  
**Parameters:** None  
**Returns:** `Success` if target exists, `Failure` otherwise

Sets movement goal to current target position.

#### Example

```json
{
  "id": 21,
  "type": "Action",
  "name": "Set Goal to Target",
  "actionType": "SetMoveGoalToTarget"
}
```

---

### SetMoveGoalToPatrolPoint

**Type:** `BTActionType::SetMoveGoalToPatrolPoint`  
**Parameters:** `param1` (float) - Patrol point index (optional)  
**Returns:** `Success` if patrol points exist

#### Example

```json
{
  "id": 22,
  "type": "Action",
  "name": "Set Goal to Patrol Point",
  "actionType": "SetMoveGoalToPatrolPoint",
  "param1": 0
}
```

---

### MoveToGoal / MoveTo

**Type:** `BTActionType::MoveToGoal`  
**Parameters:** `param1` (float) - Desired speed multiplier (default: 1.0)  
**Returns:** `Running` while moving, `Success` when arrived

#### Component Dependencies

Requires `MoveIntent_data` and `Position_data` components.

#### Example

```json
{
  "id": 23,
  "type": "Action",
  "name": "Move at Half Speed",
  "actionType": "MoveTo",
  "param1": 0.5
}
```

#### Implementation

```cpp
case BTActionType::MoveToGoal:
    if (!blackboard.hasMoveGoal) return BTStatus::Failure;
    
    if (World::Get().HasComponent<MoveIntent_data>(entity))
    {
        MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
        intent.targetPosition = blackboard.moveGoal;
        intent.desiredSpeed = (param1 > 0.0f) ? param1 : 1.0f;
        intent.hasIntent = true;
        
        if (World::Get().HasComponent<Position_data>(entity))
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            float dist = (pos.position - blackboard.moveGoal).Magnitude();
            if (dist < intent.arrivalThreshold)
            {
                blackboard.hasMoveGoal = false;
                intent.hasIntent = false;
                return BTStatus::Success;
            }
        }
        
        return BTStatus::Running;
    }
    return BTStatus::Failure;
```

---

### AttackIfClose / AttackMelee

**Type:** `BTActionType::AttackIfClose`  
**Parameters:**
- `param1` (float) - Attack range (default: 50.0)
- `param2` (float) - Attack damage (default: 10.0)

**Returns:** `Success` if attack issued, `Failure` otherwise

#### Component Dependencies

Requires `AttackIntent_data` component.

#### Example

```json
{
  "id": 24,
  "type": "Action",
  "name": "Melee Attack",
  "actionType": "AttackMelee",
  "param1": 50.0,
  "param2": 15.0
}
```

---

### PatrolPickNextPoint

**Type:** `BTActionType::PatrolPickNextPoint`  
**Parameters:** None  
**Returns:** `Success` if patrol points exist

Advances to next patrol point in circular order.

---

### ClearTarget

**Type:** `BTActionType::ClearTarget`  
**Parameters:** None  
**Returns:** `Success` (instant)

Clears current target from blackboard.

---

### Idle

**Type:** `BTActionType::Idle`  
**Parameters:** None  
**Returns:** `Success` (instant)

Does nothing. Useful as fallback action.

---

### WaitRandomTime

**Type:** `BTActionType::WaitRandomTime`  
**Parameters:**
- `param1` (float) - Minimum wait time in seconds (default: 2.0)
- `param2` (float) - Maximum wait time in seconds (default: 6.0)

**Returns:** `Running` while waiting, `Success` when timer expires

#### Example

```json
{
  "id": 30,
  "type": "Action",
  "name": "Wait 2-5 seconds",
  "actionType": "WaitRandomTime",
  "param1": 2.0,
  "param2": 5.0
}
```

---

### ChooseRandomNavigablePoint

**Type:** `BTActionType::ChooseRandomNavigablePoint`  
**Parameters:**
- `param1` (float) - Search radius (default: from blackboard)
- `param2` (float) - Max attempts (default: from blackboard)

**Returns:** `Success` if point found, `Failure` otherwise

Chooses random navigable point within radius using A* navigation mesh.

#### Example

```json
{
  "id": 31,
  "type": "Action",
  "name": "Choose Wander Point",
  "actionType": "ChooseRandomNavigablePoint",
  "param1": 200.0,
  "param2": 10.0
}
```

---

### RequestPathfinding

**Type:** `BTActionType::RequestPathfinding`  
**Parameters:** None  
**Returns:** `Success` (instant)

Requests pathfinding to current move goal via `MoveIntent` component.

#### Example

```json
{
  "id": 32,
  "type": "Action",
  "name": "Request Path",
  "actionType": "RequestPathfinding"
}
```

---

### FollowPath

**Type:** `BTActionType::FollowPath`  
**Parameters:** None  
**Returns:** `Running` while following, `Success` when arrived

Follows pathfinding path set by `RequestPathfinding`.

---

## Complete Example: Guard AI

```json
{
  "schema_version": 2,
  "name": "GuardAI",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "Selector",
        "name": "Root",
        "children": [2, 10, 15]
      },
      {
        "id": 2,
        "type": "Sequence",
        "name": "Combat",
        "children": [3, 4, 5]
      },
      {
        "id": 3,
        "type": "Condition",
        "name": "Has Target?",
        "conditionType": "HasTarget"
      },
      {
        "id": 4,
        "type": "Selector",
        "name": "Attack or Chase",
        "children": [6, 8]
      },
      {
        "id": 6,
        "type": "Sequence",
        "name": "Attack",
        "children": [7, 5]
      },
      {
        "id": 7,
        "type": "Condition",
        "name": "In Range?",
        "conditionType": "IsTargetInAttackRange",
        "parameters": { "param": 50.0 }
      },
      {
        "id": 5,
        "type": "Action",
        "name": "Melee Attack",
        "actionType": "AttackMelee",
        "parameters": { "param1": 50.0, "param2": 20.0 }
      },
      {
        "id": 8,
        "type": "Sequence",
        "name": "Chase",
        "children": [9]
      },
      {
        "id": 9,
        "type": "Action",
        "name": "Move to Target",
        "actionType": "SetMoveGoalToTarget"
      },
      {
        "id": 10,
        "type": "Sequence",
        "name": "Patrol",
        "children": [11, 12, 13]
      },
      {
        "id": 11,
        "type": "Action",
        "name": "Pick Next Point",
        "actionType": "PatrolPickNextPoint"
      },
      {
        "id": 12,
        "type": "Action",
        "name": "Set Goal to Point",
        "actionType": "SetMoveGoalToPatrolPoint"
      },
      {
        "id": 13,
        "type": "Action",
        "name": "Move",
        "actionType": "MoveTo",
        "parameters": { "param1": 1.0 }
      },
      {
        "id": 15,
        "type": "Action",
        "name": "Idle",
        "actionType": "Idle"
      }
    ]
  }
}
```

**Behavior:** Attack target if visible, otherwise patrol, otherwise idle.

---

## See Also

- [Custom Behavior Tree Nodes](./custom-nodes.md) - Creating custom node types
- [AI Components](../architecture/ai-components.md) - ECS components for AI
- [Navigation System](../navigation/pathfinding.md) - Pathfinding integration
