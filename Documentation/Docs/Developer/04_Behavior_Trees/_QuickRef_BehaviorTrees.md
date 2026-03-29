# Behavior Trees Quick Reference

**What:** Data-driven AI decision trees executed by BehaviorTreeSystem  
**Where:** `Blueprints/AI/*.json`  
**Purpose:** Define NPC behavior logic without C++ recompilation

---

## Node Types

| Type | Symbol | Behavior |
|------|--------|----------|
| **Sequence** | → | Returns Success if ALL children succeed (AND logic) |
| **Selector** | ? | Returns Success if ANY child succeeds (OR logic) |
| **Action** | ⚡ | Executes game logic; returns Running/Success/Failure |
| **Condition** | ❓ | Evaluates state; returns Success/Failure only |
| **Inverter** | ! | Flips child result (Success↔Failure) |
| **Repeater** | ↻ | Repeats child N times (or infinite if -1) |

---

## Built-in Actions

### Movement
| Action | Parameters | Description |
|--------|------------|-------------|
| `SetMoveGoalToTarget` | - | Sets move goal to current target's position |
| `SetMoveGoalToLastKnownTargetPos` | - | Sets move goal to last seen target position |
| `SetMoveGoalToPatrolPoint` | - | Sets move goal to current patrol waypoint |
| `MoveToGoal` | `param1: speed` | Moves entity toward blackboard move goal |

### Combat
| Action | Parameters | Description |
|--------|------------|-------------|
| `AttackIfClose` | - | Attacks target if within range |

### Patrol
| Action | Parameters | Description |
|--------|------------|-------------|
| `PatrolPickNextPoint` | - | Advances to next patrol waypoint |

### Utility
| Action | Parameters | Description |
|--------|------------|-------------|
| `ClearTarget` | - | Clears blackboard target entity |
| `Idle` | - | Does nothing (always returns Success) |

### Wander System
| Action | Parameters | Description |
|--------|------------|-------------|
| `WaitRandomTime` | `param1: min`, `param2: max` | Waits random seconds between min-max |
| `ChooseRandomNavigablePoint` | `param1: radius`, `param2: attempts` | Picks random navigable point within radius |
| `RequestPathfinding` | - | Requests A* pathfinding to wander destination |
| `FollowPath` | - | Follows pathfinding result (returns Running until arrival) |

---

## Built-in Conditions

### Perception
| Condition | Description |
|-----------|-------------|
| `TargetVisible` | True if target entity is visible (line of sight) |
| `HeardNoise` | True if noise stimulus detected recently |
| `TargetInRange` | True if target within attack range |

### Status
| Condition | Description |
|-----------|-------------|
| `HealthBelow` | True if current health < threshold |
| `CanAttack` | True if attack cooldown expired |
| `HasMoveGoal` | True if blackboard has valid move goal |

### Wander System
| Condition | Description |
|-----------|-------------|
| `IsWaitTimerExpired` | True if wander wait timer elapsed |
| `HasNavigableDestination` | True if valid wander destination chosen |
| `HasValidPath` | True if pathfinding succeeded |
| `HasReachedDestination` | True if entity reached wander destination |

---

## Tree File Format (Schema Version 2)

```json
{
  "schema_version": 2,
  "type": "BehaviorTree",
  "blueprintType": "BehaviorTree",
  "name": "TreeName",
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "name": "Root Sequence",
        "type": "Sequence",
        "children": [2, 3],
        "parameters": {}
      },
      {
        "id": 2,
        "name": "Check Condition",
        "type": "Condition",
        "conditionType": "TargetVisible",
        "parameters": {}
      },
      {
        "id": 3,
        "name": "Attack Action",
        "type": "Action",
        "actionType": "AttackIfClose",
        "parameters": {}
      }
    ]
  }
}
```

---

## Common Patterns

### Patrol Behavior
```
Sequence
├─ PatrolPickNextPoint
├─ SetMoveGoalToPatrolPoint
└─ MoveToGoal (param1: 0.5)
```

### Wander Behavior
```
Repeater (infinite loop)
└─ Sequence
   ├─ WaitRandomTime (2-6s)
   ├─ ChooseRandomNavigablePoint (radius: 500)
   ├─ RequestPathfinding
   └─ FollowPath
```

### Chase Target
```
Selector
├─ Sequence
│  ├─ TargetVisible (Condition)
│  └─ SetMoveGoalToTarget (Action)
└─ SetMoveGoalToLastKnownTargetPos (fallback)
```

### Combat Decision
```
Sequence
├─ TargetInRange (Condition)
├─ CanAttack (Condition)
└─ AttackIfClose (Action)
```

---

## Blackboard Integration

**Blackboard Component:** `AIBlackboard_data`

Shared memory between BT nodes:

| Field | Type | Used By |
|-------|------|---------|
| `targetEntity` | EntityID | Perception → Combat actions |
| `lastKnownTargetPosition` | Vector | Perception → Movement |
| `moveGoal` | Vector | Movement actions → AIMotionSystem |
| `hasMoveGoal` | bool | Conditions → Movement validation |
| `canAttack` | bool | Conditions → Combat actions |
| `wanderDestination` | Vector | Wander actions → Pathfinding |
| `wanderWaitTimer` | float | WaitRandomTime → Timer checks |

---

## Execution Model

1. **BehaviorTreeSystem** ticks active trees (default: 10 Hz)
2. **Recursive execution** from root node
3. **Returns status:** Running, Success, or Failure
4. **Composite nodes** process children left-to-right
5. **Actions** can span multiple frames (Running status)
6. **Conditions** evaluate instantly (no Running status)

---

## Parameters

Actions and conditions support generic parameters:

```json
{
  "type": "Action",
  "actionType": "MoveToGoal",
  "parameters": {
    "param1": 0.5,    // Speed multiplier
    "param2": 10.0    // Custom value
  }
}
```

**Common usage:**
- `param1` for primary value (speed, duration, radius)
- `param2` for secondary value (max value, attempts)

---

## Quick Tips

✅ **Use Repeater** for continuous behaviors (patrol, wander)  
✅ **Sequence = AND** (all must succeed)  
✅ **Selector = OR** (first success wins)  
✅ **Conditions check state**, Actions modify state  
✅ **Blackboard persists** across tree ticks  
✅ **Timeslicing:** Reduce `thinkHz` in `AISenses_data` for performance  

❌ **Don't nest too deeply** (prefer flat structures)  
❌ **Don't mix Actions and Conditions** in same category  
❌ **Don't forget Running status** for multi-frame actions  

---

## Related Documentation

- **Detailed Guide:** `Creating_BT_Actions_Guide.md`
- **AI Pipeline:** `05_AI_Systems/_QuickRef_AI_Pipeline.md`
- **Blackboard:** `05_AI_Systems/AI_Blackboard_Guide.md`
- **Systems:** `02_ECS_Systems/_QuickRef_Systems.md`
