---
id: bt-editor-overview
title: "Behavior Tree Editor - Overview"
sidebar_label: "Overview"
---

# Behavior Tree Editor - Overview

The Behavior Tree Editor is a visual tool for creating and editing AI behavior trees in the Olympe Engine. It provides a node-based interface for designing complex AI decision-making systems without writing code.

## What is a Behavior Tree?

A Behavior Tree (BT) is a hierarchical structure used to control AI decision-making. Unlike Finite State Machines (FSM), behavior trees offer:

- **Modular design**: Reusable sub-trees and nodes
- **Clear hierarchy**: Parent-child relationships define execution flow
- **Easy to understand**: Visual representation of AI logic
- **Scalable**: From simple to complex behaviors
- **Reactive**: Re-evaluates decisions each frame

### Why Use Behavior Trees?

Behavior Trees are ideal for:
- **NPC AI**: Enemies, companions, civilians
- **Boss behaviors**: Complex multi-phase attacks
- **Patrol systems**: Guards, wandering NPCs
- **Combat AI**: Decision-making during fights
- **Interactive objects**: Door logic, traps, puzzles

## Editor Interface

The Behavior Tree Editor consists of several main areas:

### 1. Node Graph (Center)
The main canvas where you build your behavior tree by placing and connecting nodes.

**Features:**
- Visual node connections
- Pan and zoom controls
- Node selection and editing
- Real-time validation

### 2. Toolbar (Top)
Quick access to common operations:
- **New**: Create a new behavior tree
- **Open**: Load existing tree
- **Save**: Save current tree
- **Validate**: Check for errors
- **Test**: Run in debugger

### 3. Node Palette (Left)
All available node types organized by category:
- **Composite Nodes**: Selector, Sequence
- **Decorator Nodes**: Inverter, Repeater
- **Condition Nodes**: Target checks, health checks
- **Action Nodes**: Movement, combat, patrol

### 4. Properties Panel (Right)
Edit properties of the selected node:
- Node name (for clarity)
- Node-specific parameters
- Connection validation
- Documentation/help

## Key Concepts

### Tree Structure

Every behavior tree has:
- **Root Node**: The entry point (usually a Selector or Sequence)
- **Branch Nodes**: Composite nodes that have children
- **Leaf Nodes**: Conditions and Actions (no children)

### Execution Flow

Behavior trees execute from top to bottom (or left to right in horizontal layout):

1. **Start at root**: Execution begins at the root node
2. **Evaluate children**: Nodes evaluate their children in order
3. **Return status**: Each node returns Success, Failure, or Running
4. **Re-evaluate**: Tree is re-evaluated each frame

### Node Status

Every node returns one of three statuses:

| Status | Icon | Meaning |
|--------|------|---------|
| **Running** | 🔵 | Node is still executing (next frame continues) |
| **Success** | ✅ | Node completed successfully |
| **Failure** | ❌ | Node failed |

## Blueprint Format

Behavior trees are saved as JSON files in the `Blueprints/AI/` directory.

**File format:**
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "name": "MyAI",
  "data": {
    "rootNodeId": 1,
    "nodes": [...]
  }
}
```

### Schema Version

The editor uses **schema version 2**, which includes:
- Standardized blueprint type field
- Metadata (author, creation date, tags)
- Editor state (zoom, scroll position)
- Improved validation

## Integration with ECS

Behavior trees integrate with the Entity Component System:

### Required Components

To use a behavior tree, an entity needs:
- `AIBehaviorTree_data`: Links entity to tree file
- `AIBlackboard_data`: Stores runtime state (target, goals, timers)
- `AIRuntime_data`: Tracks current node and execution state

### Prefab Integration

Attach a behavior tree to an entity in the prefab JSON:
```json
{
  "components": [
    {
      "type": "AIBehaviorTree_data",
      "treeFile": "Blueprints/AI/guard_ai.json"
    },
    {
      "type": "AIBlackboard_data"
    }
  ]
}
```

## Runtime Debugger

The BT Debugger provides real-time visualization of AI execution:

- **Entity List**: View all entities with behavior trees
- **Node Graph**: See which nodes are currently executing
- **Execution Log**: Track decision history
- **Blackboard Inspector**: View runtime state variables
- **Performance Metrics**: Monitor execution time

See [BT Debugger Guide](bt-debugger.md) for details.

## Best Practices

### 1. Start Simple
Begin with a simple tree structure (3-5 nodes) before adding complexity.

**Example: Basic idle behavior**
```
Root (Action)
 └─ Idle
```

### 2. Use Meaningful Names
Give nodes descriptive names that explain what they do:
- ✅ "Check if target in attack range"
- ❌ "Condition 1"

### 3. Keep Trees Shallow
Aim for 2-4 levels of depth. Deep trees are hard to understand:
- ✅ Root → Category → Action (3 levels)
- ❌ Root → Cat1 → Cat2 → Cat3 → Cat4 → Action (6 levels)

### 4. Test Incrementally
Test your tree after adding each branch:
1. Add patrol behavior → Test
2. Add combat behavior → Test
3. Add investigation → Test

### 5. Reuse Sub-trees
Create separate tree files for reusable behaviors:
- `guard_patrol.json` - Patrol logic (reusable)
- `guard_combat.json` - Combat logic (reusable)
- `guard_ai.json` - Main tree that uses both

### 6. Document Complex Logic
Use node names and comments to explain:
- Why certain conditions exist
- What parameters mean
- Expected behavior flow

## Common Patterns

### Pattern 1: Priority Selector
Handle behaviors by priority (first success wins):
```
Selector (Root)
 ├─ Combat (high priority)
 ├─ Investigate (medium priority)
 └─ Patrol (low priority)
```

### Pattern 2: Sequence with Guards
Check conditions before executing actions:
```
Sequence
 ├─ Condition: Target in Range
 └─ Action: Attack Target
```

### Pattern 3: Repeating Action
Execute action multiple times:
```
Repeater (count=3)
 └─ Action: Fire Projectile
```

## File Organization

Organize your behavior trees logically:

```
Blueprints/
└── AI/
    ├── Common/
    │   ├── patrol.json       (reusable patrol)
    │   ├── combat.json       (reusable combat)
    │   └── investigate.json  (reusable search)
    ├── Enemies/
    │   ├── guard_ai.json
    │   ├── zombie_ai.json
    │   └── boss_ai.json
    └── NPCs/
        ├── merchant_ai.json
        └── villager_ai.json
```

## Performance Considerations

### CPU Cost
- Each entity evaluates its tree **every frame**
- Complex trees (30+ nodes) may impact performance with 100+ entities
- Use conditions to early-exit expensive branches

### Memory Usage
- Trees are loaded once and shared across entities
- Runtime state stored per-entity in AIRuntime_data (~100 bytes)
- Blackboard stores entity-specific data (~200 bytes)

### Optimization Tips
1. **Limit depth**: Shallow trees execute faster
2. **Early exit**: Put likely-to-fail conditions first
3. **Caching**: Store expensive calculations in blackboard
4. **Tree reuse**: Share tree files across entity types

## Next Steps

Ready to start building? Continue with:

- **[Node Types](bt-node-types.md)** - Learn about all available nodes
- **[Connection Rules](bt-connection-rules.md)** - Understand how to connect nodes
- **[Keyboard Shortcuts](bt-keyboard-shortcuts.md)** - Work more efficiently
- **[First BT Tutorial](bt-first-tree-tutorial.md)** - Create your first behavior tree

## Additional Resources

- [Technical Reference](../../technical-reference/behavior-trees/behavior-trees-overview.md) - Deep architecture details
- [Custom Nodes](../../technical-reference/behavior-trees/custom-nodes.md) - Extend the system with code
- [BT Debugger](bt-debugger.md) - Runtime debugging guide
- [Behavior Tree Basics Tutorial](../../tutorials/behavior-tree-basics.md) - Comprehensive tutorial

## Troubleshooting

### Tree not loading
- ✅ Check file path in prefab matches actual file
- ✅ Validate JSON syntax
- ✅ Ensure `schema_version: 2` is set

### AI not executing
- ✅ Entity has AIBehaviorTree_data component
- ✅ Entity has AIBlackboard_data component
- ✅ Tree file loaded successfully (check console)

### Nodes always failing
- ✅ Check condition parameters (range values, thresholds)
- ✅ Verify entity has required components (Navigation, Combat, etc.)
- ✅ Use debugger to see which node fails

For more help, see the [Troubleshooting section](../../tutorials/behavior-tree-basics.md#troubleshooting) in the tutorial.
