---
id: bt-node-types
title: "Behavior Tree Node Types"
sidebar_label: "Node Types"
---

# Behavior Tree Node Types

This page provides a complete reference of all node types available in the Behavior Tree Editor. Understanding these nodes is essential for creating effective AI behaviors.

## Node Categories

Nodes are organized into four main categories:

1. **Composite Nodes** - Control flow nodes with multiple children
2. **Decorator Nodes** - Modify behavior of a single child node
3. **Condition Nodes** - Check game state (return Success/Failure)
4. **Action Nodes** - Perform actions (movement, combat, etc.)

---

## Composite Nodes

Composite nodes control execution flow by evaluating multiple children in sequence or by priority.

### Selector (OR Node)

**Icon:** 🔀 | **Color:** Purple | **Type:** Composite

Executes children in order until one **succeeds**.

**Behavior:**
- Tries each child from left to right (or top to bottom)
- Returns **Success** if any child succeeds
- Returns **Failure** if all children fail
- Returns **Running** if current child is running

**Use Cases:**
- Priority-based behaviors (combat → investigate → patrol)
- Fallback logic (try A, if fails try B, if fails try C)
- Decision-making with multiple options

**Example:**
```
Selector "AI Decision"
 ├─ Sequence "Combat" (tries first)
 ├─ Sequence "Investigate" (tries if combat fails)
 └─ Sequence "Patrol" (tries if investigation fails)
```

**Real-world analogy:** "Try to attack, **OR** investigate, **OR** patrol"

---

### Sequence (AND Node)

**Icon:** ➡️ | **Color:** Blue | **Type:** Composite

Executes children in order until one **fails**.

**Behavior:**
- Tries each child from left to right (or top to bottom)
- Returns **Success** only if all children succeed
- Returns **Failure** if any child fails
- Returns **Running** if current child is running

**Use Cases:**
- Multi-step processes (check condition → perform action)
- Guarded actions (verify before executing)
- Sequential behaviors (step 1, then step 2, then step 3)

**Example:**
```
Sequence "Attack Target"
 ├─ Condition "Target in Range"
 ├─ Condition "Can Attack" (cooldown check)
 └─ Action "Attack Melee"
```

**Real-world analogy:** "Check if close **AND** check if ready **AND** attack"

---

## Decorator Nodes

Decorators modify the behavior of their single child node.

### Inverter (NOT Node)

**Icon:** ↩️ | **Color:** Orange | **Type:** Decorator

Inverts the result of its child node.

**Behavior:**
- Returns **Success** if child returns **Failure**
- Returns **Failure** if child returns **Success**
- Returns **Running** if child returns **Running**

**Use Cases:**
- "If NOT condition" logic
- Negating conditions (e.g., "target NOT in range")
- Reverse success/failure semantics

**Example:**
```
Inverter "Target NOT Visible"
 └─ Condition "Target Visible"
```

**Parameters:** None

**Real-world analogy:** "Do the opposite of what the child says"

---

### Repeater (Loop Node)

**Icon:** 🔁 | **Color:** Green | **Type:** Decorator

Repeats its child node N times or infinitely.

**Behavior:**
- Executes child node multiple times
- Returns **Success** after N successful executions
- Returns **Failure** if child fails (early exit)
- Returns **Running** while repeating

**Parameters:**
- `count` (int): Number of repetitions (-1 for infinite)

**Use Cases:**
- Firing multiple projectiles
- Patrolling waypoints repeatedly
- Continuous behaviors (infinite loop)

**Example:**
```
Repeater (count=3) "Fire 3 Shots"
 └─ Action "Fire Projectile"
```

**Real-world analogy:** "Do this action N times in a row"

---

## Condition Nodes

Conditions check game state and return Success or Failure (never Running).

### Target Visible (HasTarget)

**Type:** Condition | **Category:** Detection

Checks if the entity can see its target.

**Parameters:** None

**Returns:**
- **Success** if target exists and is visible
- **Failure** if no target or target not visible

**Use Cases:**
- Before pursuing target
- Entering combat mode
- Breaking from patrol

**Related Blackboard:**
- Reads: `targetEntity` (EntityID)

---

### Target In Range

**Type:** Condition | **Category:** Detection

Checks if target is within a specified distance.

**Parameters:**
- `range` (float): Maximum distance in pixels

**Returns:**
- **Success** if target within range
- **Failure** if no target or target too far

**Use Cases:**
- Before attacking (melee/ranged range check)
- Trigger special abilities
- Proximity detection

**Example:**
```json
{
  "type": "Condition",
  "conditionType": "TargetInRange",
  "conditionParam": 150.0
}
```

**Related Blackboard:**
- Reads: `targetEntity`, `lastKnownTargetPosition`

---

### Health Below

**Type:** Condition | **Category:** State

Checks if entity's health is below a threshold.

**Parameters:**
- `threshold` (float): Health value (0-100)

**Returns:**
- **Success** if health < threshold
- **Failure** otherwise

**Use Cases:**
- Trigger flee behavior
- Call for help
- Switch to defensive tactics

**Example:**
```json
{
  "type": "Condition",
  "conditionType": "HealthBelow",
  "conditionParam": 30.0
}
```

**Related Components:**
- Reads: `Health_data.current`

---

### Has Move Goal

**Type:** Condition | **Category:** Navigation

Checks if entity has an active movement goal.

**Parameters:** None

**Returns:**
- **Success** if moveGoal is set
- **Failure** if no movement goal

**Use Cases:**
- Before executing movement
- Check if pathfinding succeeded
- Validate navigation state

**Related Blackboard:**
- Reads: `moveGoal` (Vector)

---

### Can Attack

**Type:** Condition | **Category:** Combat

Checks if entity can perform an attack (cooldown check).

**Parameters:** None

**Returns:**
- **Success** if attack cooldown expired
- **Failure** if still on cooldown

**Use Cases:**
- Guard attack action
- Prevent spam attacks
- Check weapon readiness

**Related Components:**
- Reads: `Combat_data.attackCooldown`

---

### Heard Noise

**Type:** Condition | **Category:** Detection

Checks if entity detected a noise event.

**Parameters:** None

**Returns:**
- **Success** if noise detected recently
- **Failure** if no noise

**Use Cases:**
- Trigger investigation
- Alert state
- Sound-based detection

**Related Blackboard:**
- Reads: `heardNoise` (bool), `noisePosition` (Vector)

---

### Is Wait Timer Expired

**Type:** Condition | **Category:** Timing

Checks if a timer has expired.

**Parameters:** None

**Returns:**
- **Success** if timer <= 0
- **Failure** if timer > 0

**Use Cases:**
- Wander behavior (wait before moving)
- Timed pauses
- Delayed actions

**Related Blackboard:**
- Reads: `waitTimer` (float)

---

### Has Navigable Destination

**Type:** Condition | **Category:** Navigation

Checks if a valid navigable destination has been chosen.

**Parameters:** None

**Returns:**
- **Success** if destination is set and navigable
- **Failure** otherwise

**Use Cases:**
- Wander behaviors
- Random movement
- Pathfinding validation

**Related Blackboard:**
- Reads: `moveGoal` (Vector)

---

### Has Valid Path

**Type:** Condition | **Category:** Navigation

Checks if pathfinding succeeded and a path exists.

**Parameters:** None

**Returns:**
- **Success** if path calculated successfully
- **Failure** if pathfinding failed

**Use Cases:**
- Before following path
- Validate navigation
- Handle unreachable destinations

**Related Components:**
- Reads: `Navigation_data.hasPath`

---

### Has Reached Destination

**Type:** Condition | **Category:** Navigation

Checks if entity reached its movement goal.

**Parameters:** None

**Returns:**
- **Success** if arrived at destination
- **Failure** if still moving

**Use Cases:**
- After movement actions
- Patrol waypoint completion
- Trigger next behavior

**Related Blackboard:**
- Reads: `reachedDestination` (bool)

---

## Action Nodes

Actions perform actual behaviors and typically return Running until complete.

### Set Move Goal To Target

**Type:** Action | **Category:** Navigation

Sets the movement goal to the current target's position.

**Parameters:** None

**Effect:**
- Sets `moveGoal` to target entity's position
- Returns **Success** if target exists
- Returns **Failure** if no target

**Use Cases:**
- Pursue enemy
- Chase player
- Follow entity

---

### Set Move Goal To Last Known Target Position

**Type:** Action | **Category:** Navigation

Sets movement goal to the last known position of target.

**Parameters:** None

**Effect:**
- Sets `moveGoal` to `lastKnownTargetPosition`
- Returns **Success** if last position exists
- Returns **Failure** otherwise

**Use Cases:**
- Investigate where target was seen
- Search last known location
- Lost target behavior

---

### Set Move Goal To Patrol Point

**Type:** Action | **Category:** Navigation

Sets movement goal to the current patrol waypoint.

**Parameters:** None

**Effect:**
- Sets `moveGoal` to current patrol point
- Returns **Success** if patrol point exists
- Returns **Failure** if no patrol data

**Use Cases:**
- Patrol behaviors
- Waypoint navigation
- Guard routes

**Related Components:**
- Reads: `Patrol_data.waypoints`, `Patrol_data.currentIndex`

---

### Move To Goal

**Type:** Action | **Category:** Navigation

Executes movement toward the current move goal.

**Parameters:**
- `acceptanceRadius` (float): Distance to consider "arrived" (default: 10.0)

**Returns:**
- **Running** while moving
- **Success** when arrived at goal
- **Failure** if no goal or pathfinding fails

**Effect:**
- Requests pathfinding
- Follows calculated path
- Updates entity position

**Example:**
```json
{
  "type": "Action",
  "actionType": "MoveToGoal",
  "actionParam1": 20.0
}
```

---

### Attack If Close (Attack Melee)

**Type:** Action | **Category:** Combat

Performs a melee attack if target is in range.

**Parameters:** None

**Returns:**
- **Running** during attack animation
- **Success** if attack executed
- **Failure** if no target or out of range

**Effect:**
- Deals damage to target
- Triggers attack animation
- Starts attack cooldown

**Related Components:**
- Uses: `Combat_data`
- Checks: `targetEntity`, attack range

---

### Patrol Pick Next Point

**Type:** Action | **Category:** Navigation

Advances to the next patrol waypoint.

**Parameters:** None

**Returns:**
- **Success** if next point selected
- **Failure** if no patrol data

**Effect:**
- Increments `Patrol_data.currentIndex`
- Wraps around to start if at end

**Use Cases:**
- Cyclic patrol routes
- Waypoint progression
- Guard patterns

---

### Clear Target

**Type:** Action | **Category:** State

Clears the current target entity.

**Parameters:** None

**Returns:**
- **Success** always

**Effect:**
- Sets `targetEntity` to 0 (null)
- Clears related target data

**Use Cases:**
- After losing target
- End of combat
- Reset detection state

---

### Idle

**Type:** Action | **Category:** State

Does nothing (no-op action).

**Parameters:** None

**Returns:**
- **Success** always (instantly)

**Use Cases:**
- Placeholder node
- Explicit wait state
- Default behavior

---

### Wait Random Time

**Type:** Action | **Category:** Timing

Initializes a random timer between min and max seconds.

**Parameters:**
- `param1` (float): Minimum time in seconds
- `param2` (float): Maximum time in seconds

**Returns:**
- **Success** after initializing timer

**Effect:**
- Sets `waitTimer` to random value in [min, max]

**Example:**
```json
{
  "type": "Action",
  "actionType": "WaitRandomTime",
  "actionParam1": 2.0,
  "actionParam2": 5.0
}
```

**Use Cases:**
- Wander behavior pauses
- Variable timing
- Natural movement

---

### Choose Random Navigable Point

**Type:** Action | **Category:** Navigation

Chooses a random point within a radius that is navigable.

**Parameters:**
- `param1` (float): Search radius in pixels
- `param2` (int): Max attempts (default: 10)

**Returns:**
- **Success** if navigable point found
- **Failure** if no valid point after max attempts

**Effect:**
- Sets `moveGoal` to random navigable position

**Example:**
```json
{
  "type": "Action",
  "actionType": "ChooseRandomNavigablePoint",
  "actionParam1": 200.0,
  "actionParam2": 10
}
```

**Use Cases:**
- Wander behaviors
- Random exploration
- Idle movement

---

### Request Pathfinding

**Type:** Action | **Category:** Navigation

Requests pathfinding to the current move goal.

**Parameters:** None

**Returns:**
- **Running** while calculating path
- **Success** if path found
- **Failure** if pathfinding fails

**Effect:**
- Submits pathfinding request to NavigationSystem
- Updates `Navigation_data.path` when complete

**Use Cases:**
- Before following a path
- Long-distance navigation
- Dynamic obstacles

---

### Follow Path

**Type:** Action | **Category:** Navigation

Follows the currently calculated path.

**Parameters:** None

**Returns:**
- **Running** while following path
- **Success** when destination reached
- **Failure** if path lost or blocked

**Effect:**
- Moves entity along waypoints
- Updates position each frame
- Checks for path validity

**Use Cases:**
- After pathfinding succeeds
- Multi-waypoint navigation
- Dynamic movement

---

## Node Color Reference

In the debugger, nodes are color-coded by type:

| Node Type | Color | Hex Code |
|-----------|-------|----------|
| Selector | Purple | `#9C27B0` |
| Sequence | Blue | `#2196F3` |
| Condition | Yellow | `#FFC107` |
| Action | Green | `#4CAF50` |
| Inverter | Orange | `#FF9800` |
| Repeater | Teal | `#009688` |

### Status Colors

Nodes also show execution status with color overlays:

| Status | Color | Hex Code |
|--------|-------|----------|
| Idle | Gray | `#808080` |
| Running | Blue | `#2196F3` |
| Success | Green | `#4CAF50` |
| Failure | Red | `#F44336` |

---

## Choosing the Right Node

### Decision Tree

Follow this guide to choose the correct node:

**Do you need to:**
- **Make a decision?** → Use **Selector**
- **Do multiple things in order?** → Use **Sequence**
- **Check game state?** → Use **Condition**
- **Change the world?** → Use **Action**
- **Invert logic?** → Use **Inverter**
- **Repeat behavior?** → Use **Repeater**

### Common Mistakes

❌ **Using Actions as Conditions**
```
Action "Move To Goal" as a condition check
```
✅ **Use Conditions to check, Actions to execute**
```
Condition "Has Move Goal" → Action "Move To Goal"
```

❌ **Deep nesting without clear purpose**
```
Selector → Sequence → Selector → Sequence → Action (5 levels)
```
✅ **Keep hierarchies shallow and logical**
```
Selector → Sequence → Action (3 levels)
```

❌ **Forgetting Inverter for "NOT" logic**
```
Creating custom "NotInRange" condition
```
✅ **Use Inverter with existing conditions**
```
Inverter → Condition "Target In Range"
```

---

## Next Steps

Now that you understand all node types:

- **[Connection Rules](bt-connection-rules.md)** - Learn how to connect nodes properly
- **[First BT Tutorial](bt-first-tree-tutorial.md)** - Build a complete behavior tree
- **[BT Debugger](bt-debugger.md)** - Debug your trees at runtime

## Additional Resources

- [Custom Nodes](../../technical-reference/behavior-trees/custom-nodes.md) - Create your own node types in C++
- [Nodes Technical Reference](../../technical-reference/behavior-trees/nodes.md) - Deep implementation details
- [Behavior Tree Basics](../../tutorials/behavior-tree-basics.md) - Complete tutorial with examples
