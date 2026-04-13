---
id: node-catalog
title: Node Catalog
sidebar_label: Node Catalog
sidebar_position: 2
---

# Node Catalog

Complete reference of all available nodes in the Visual Script editor.

## Control Flow Nodes

### Sequence
Executes children left-to-right. Fails if any child fails.
- **Inputs**: None
- **Outputs**: child execution pins

### Selector
Tries children in order. Succeeds when the first child succeeds.
- **Inputs**: None
- **Outputs**: child execution pins

### Parallel
Executes all children simultaneously.
- **Policy**: Success when N children succeed (configurable)

## Action Nodes

### MoveToTarget
Moves the entity toward a target position.
- **Parameters**: `speed` (float), `tolerance` (float)
- **Pins**: `targetPosition` (Vector3)

### PlayAnimation
Triggers an animation on the entity.
- **Parameters**: `animationName` (string), `loop` (bool)

### SetBlackboardValue
Writes a value to the blackboard.
- **Parameters**: `key` (string), `value` (any type)

### Wait
Pauses execution for a given duration.
- **Parameters**: `duration` (float, seconds)

## Condition Nodes

### CheckBlackboardValue
Reads a blackboard key and compares it.
- **Parameters**: `key`, `operator` (==, !=, <, >, <=, >=), `value`

### IsInRange
Returns true if target is within range.
- **Parameters**: `range` (float)

### HasLineOfSight
Raycasts to check visibility to target.

## Decorator Nodes

### Inverter
Inverts the child's return value.

### Repeater
Repeats child N times (or infinitely).
- **Parameters**: `count` (int, -1 = infinite)

### Cooldown
Prevents re-execution within a time window.
- **Parameters**: `cooldown` (float, seconds)

### Timeout
Fails the child if it doesn't complete in time.
- **Parameters**: `maxTime` (float)

## SubGraph Node

Embeds another `.ats` file as a reusable block.
- **Double-click** to navigate into the subgraph
- Path stored relative to root `GameData/` directory

## Related

- [Visual Scripting Overview](visual-scripting-overview)
- [Task Execution](task-execution)
