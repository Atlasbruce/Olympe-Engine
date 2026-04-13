# Behavior Tree Node Reference

## Composite Nodes

### Sequence
- **Symbol**: `→`
- **Children**: 1..N
- **Logic**: Execute children in order. Stop and return FAILURE on first child failure. Return SUCCESS if all succeed.

### Selector
- **Symbol**: `?`
- **Children**: 1..N
- **Logic**: Execute children in order. Stop and return SUCCESS on first child success. Return FAILURE if all fail.

### Parallel
- **Symbol**: `‖`
- **Parameters**: `successThreshold` (int, default = all)
- **Logic**: Execute all children concurrently. Return SUCCESS when `successThreshold` children succeed.

## Decorator Nodes

### Inverter
Flips SUCCESS ↔ FAILURE. RUNNING passes through.

### Repeater
- **Parameters**: `count` (int, -1 = infinite)
- Re-executes child node `count` times.

### Cooldown
- **Parameters**: `duration` (float, seconds)
- Returns FAILURE immediately if within cooldown window.

### Timeout
- **Parameters**: `maxTime` (float, seconds)
- Returns FAILURE if child hasn't succeeded within `maxTime`.

### ForceSuccess
Always returns SUCCESS regardless of child result.

### ForceFailure
Always returns FAILURE regardless of child result.

## Action Nodes

| Node | Parameters | Description |
|------|-----------|-------------|
| MoveToTarget | speed, tolerance | Navigate to target position |
| PlayAnimation | animName, loop | Trigger animation |
| SetBlackboard | key, value | Write blackboard variable |
| Wait | duration | Pause execution |
| Log | message | Debug output |
| SpawnEntity | prefabFile | Instantiate a prefab |

## Condition Nodes

| Node | Parameters | Description |
|------|-----------|-------------|
| CheckBlackboard | key, op, value | Compare blackboard value |
| IsInRange | range | Check distance to target |
| HasLineOfSight | — | Raycast visibility check |
| IsAnimationDone | animName | Check animation completion |
| IsHealthBelow | threshold | Health percentage check |
