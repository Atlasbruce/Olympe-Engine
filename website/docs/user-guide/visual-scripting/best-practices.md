---
id: best-practices
title: Visual Scripting Best Practices
sidebar_label: Best Practices
sidebar_position: 4
---

# Visual Scripting Best Practices

## Graph Organization

### Keep graphs focused
- One graph per behavior (e.g., `patrol.ats`, `combat.ats`, `idle.ats`)
- Use SubGraph nodes to compose complex behaviors

### Use meaningful node labels
- Right-click a node → **Rename** to give it a descriptive label
- Good: `"Check Player In Range"` instead of `"Condition_12"`

## Blackboard Usage

### Prefer typed keys
- Use consistent naming: `snake_case` for all blackboard keys
- Group related keys: `player_pos`, `player_health`, `player_detected`

### Avoid redundant reads
- Cache frequently-accessed values in local blackboard
- Use condition presets for reusable condition checks

## Performance Tips

- **Avoid deep nesting**: Limit depth to 5-6 levels
- **Use Cooldown decorators**: Prevent expensive checks from running every frame
- **SubGraph caching**: SubGraph files are cached after first load

## Common Patterns

### Patrol Loop
```
Selector
├── [Condition: IsPlayerDetected] → Sequence
│   ├── MoveToPlayer
│   └── Attack
└── Sequence (default patrol)
    ├── MoveToWaypoint(A)
    ├── Wait(1.0s)
    ├── MoveToWaypoint(B)
    └── Wait(1.0s)
```

### Alert System
```
Sequence
├── CheckBlackboard(health < 30)
├── SetBlackboard(isLowHealth = true)
└── PlayAnimation("hurt")
```

## Related

- [Node Catalog](node-catalog)
- [Task Execution](task-execution)
