# Behavior Tree Concepts

Behavior Trees (BTs) are hierarchical structures for AI decision-making. Olympe Engine's BT system is visual, JSON-based, and integrates with the Blackboard System.

## Core Node Types

| Type | Symbol | Description |
|------|--------|-------------|
| **Sequence** | → | Execute children left-to-right; fail on first failure |
| **Selector** | ? | Try children in order; succeed on first success |
| **Parallel** | ‖ | Execute all children simultaneously |
| **Decorator** | ◇ | Modify a single child's behavior |
| **Action** | □ | Leaf node that performs work |
| **Condition** | ◎ | Leaf node that tests a condition |

## Return Values

| Status | Meaning |
|--------|---------|
| `SUCCESS` | Task completed |
| `FAILURE` | Task failed |
| `RUNNING` | Task in progress (multi-tick) |

## Tick Model

```
Frame N:   Root.Tick()
               ↓
           Sequence.Tick()
               ↓
           MoveToTarget.Tick() → RUNNING
               ↓
           (yield until frame N+1)
Frame N+1: Resume from RUNNING node
```

## Example Tree

```
Selector
├── Sequence (combat)
│   ├── Condition: IsPlayerVisible
│   ├── Action: FacePlayer
│   └── Action: Shoot
└── Sequence (patrol)
    ├── Action: MoveToWaypoint
    └── Action: Wait(2s)
```

## BT File Format

```json
{
  "root": {
    "type": "Selector",
    "children": [
      {
        "type": "Sequence",
        "children": [
          { "type": "Condition", "conditionId": "IsPlayerVisible" },
          { "type": "Action",    "actionId": "Shoot" }
        ]
      }
    ]
  }
}
```
