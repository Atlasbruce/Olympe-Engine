# TaskGraph Assets Directory

This directory contains ATS (Advanced Task System) Visual Scripting graphs for the Olympe Engine.

## Directory Structure

- **Examples/** - Example task graphs demonstrating various patterns
- **Templates/** - Empty template graphs for quick start

## File Format

TaskGraph files use the `.ats` extension and JSON format (schema version 4):

```json
{
  "schema_version": 4,
  "graphType": "BehaviorTree",
  "name": "ExampleGraph",
  "entryPointId": 1,
  "nodes": [
    {
      "id": 1,
      "type": "Sequence",
      "name": "Root",
      "children": [2, 3]
    }
  ],
  "execConnections": [],
  "dataConnections": []
}
```

### Supported `graphType` values

| Value | Description |
|-------|-------------|
| `"BehaviorTree"` | Classic Behavior Tree (Sequence / Selector / Decorator / AtomicTask) |
| `"VisualScript"` | ATS Visual Script (EntryPoint / Branch / While / SubGraph …) |

### Supported node `type` values

| Type | Description |
|------|-------------|
| `Sequence` | Executes children in order; stops on first failure |
| `Selector` | Executes children in order; stops on first success |
| `AtomicTask` | Leaf – executes a single atomic task (`taskType` field) |
| `Decorator` | Wraps a single child and modifies its behaviour |
| `Root` | BT entry point (legacy; prefer `entryPointId`) |
| `EntryPoint` | VS entry point (unique per graph) |
| `Branch` | If/Else conditional (Then / Else exec outputs) |
| `While` | Conditional loop |
| `SubGraph` | Nested graph call (`subGraphPath` field) |

## Quick Start

1. Copy a template from `Templates/` directory
2. Open in Blueprint Editor (Node Graph panel)
3. Add nodes and connect them
4. Save with `.ats` extension
5. Reference from entity prefab's AIBlackboard component

## Examples

- **simple_patrol.ats** – Basic patrol between waypoints
- **guard_behavior.ats** – Guard that patrols and chases enemies
- **boss_combat.ats** – Complex boss AI using subgraphs (Phase 8)

## Templates

- **empty_behavior_tree.ats** – Minimal BehaviorTree with a single root Selector
- **empty_utility_ai.ats** – Minimal VisualScript with a single EntryPoint
