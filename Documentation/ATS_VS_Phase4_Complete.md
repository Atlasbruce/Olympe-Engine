# ATS Visual Scripting - Phase 4 Migration Complete

## Overview

Phase 4 finalizes the complete migration of the ATS Behavior Tree system to ATS Visual
Scripting. All 11 legacy JSON assets (schema v2 BehaviorTree) have been converted to
schema v4 VisualScript format. A Python migration tool, comprehensive SubGraph tests,
and full cleanup of deprecated code have been delivered.

---

## Migration Assets JSON v2 → v4

### Checklist

- [x] `idle.json`
- [x] `investigate.json`
- [x] `guard_patrol.json`
- [x] `guard_combat.json`
- [x] `guard_ai.json`
- [x] `guardV2_ai.json`
- [x] `GuardAI_V2_Complex_edited.json`
- [x] `npc_ai.json`
- [x] `zombie_ai.json`
- [x] `ZombieAITree_edited.json`
- [x] `Tests/guardV2_ai.json`

### Migration Script

**`Tools/migrate_bt_to_vs.py`** — Automates v2 → v4 conversion.

```bash
# Single file
python Tools/migrate_bt_to_vs.py Blueprints/AI/idle.json

# Batch directory
python Tools/migrate_bt_to_vs.py --batch Blueprints/AI/

# Dry run (no writes)
python Tools/migrate_bt_to_vs.py --dry-run Blueprints/AI/idle.json

# No backup
python Tools/migrate_bt_to_vs.py --no-backup Blueprints/AI/idle.json
```

---

## Mapping BT → VS

### Structure generale

```json
// v2 BehaviorTree (input)
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "data": {
    "rootNodeId": 1,
    "nodes": [ ... ]
  }
}

// v4 VisualScript (output)
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "GraphName",
  "description": "...",
  "blackboard": [ ... ],
  "nodes": [ ... ],
  "execConnections": [ ... ],
  "dataConnections": []
}
```

### Node type mapping

| v2 BehaviorTree | v4 VisualScript | Notes |
|---|---|---|
| `type: "Selector"` | `type: "Branch"` | Children → Then/Else exec_connections |
| `type: "Sequence"` | `type: "VSSequence"` | Children → ChildrenIDs array |
| `type: "Action"` | `type: "AtomicTask"` | `actionType` → `taskType` |
| `type: "Condition"` | `type: "Branch"` | `conditionType` → `conditionKey` |
| `rootNodeId` | EntryPoint node | New EntryPoint node added |

### Action type mapping

| v2 actionType | v4 taskType |
|---|---|
| `Idle` | `Task_Wait` |
| `MoveToGoal` | `Task_MoveToLocation` |
| `PatrolPickNextPoint` | `Task_PatrolPickPoint` |
| `SetMoveGoalToPatrolPoint` | `Task_SetMoveGoal` |
| `SetMoveGoalToTarget` | `Task_SetMoveGoal` |
| `SetMoveGoalToLastKnownTargetPos` | `Task_SetMoveGoalLastKnown` |
| `AttackIfClose` | `Task_Attack` |
| `WaitRandomTime` | `Task_Wait` |
| `ChooseRandomNavigablePoint` | `Task_ChooseRandomPoint` |
| `RequestPathfinding` | `Task_RequestPathfinding` |
| `FollowPath` | `Task_FollowPath` |
| `FindCover` | `Task_FindCover` |
| `HealSelf` | `Task_HealSelf` |
| `RotateToTarget` | `Task_RotateToTarget` |
| `FireAtTarget` | `Task_Attack` |
| `MeleeAttack` | `Task_Attack` |

---

## Conversion Examples

### Simple case: idle.json

**Before (v2)**:
```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "data": {
    "nodes": [
      { "id": 1, "type": "Action", "actionType": "Idle", "children": [] }
    ],
    "rootNodeId": 1
  }
}
```

**After (v4)**:
```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "nodes": [
    { "id": 2, "type": "EntryPoint", "name": "Start" },
    { "id": 1, "type": "AtomicTask", "name": "Idle Action", "taskType": "Task_Wait" }
  ],
  "execConnections": [
    { "fromNode": 2, "fromPin": "Out", "toNode": 1, "toPin": "In" }
  ]
}
```

### Patrol case: guard_patrol.json (Sequence)

**Before (v2)**: Sequence with 3 children [2, 3, 4]

**After (v4)**:
```json
{
  "nodes": [
    { "id": 5, "type": "EntryPoint" },
    { "id": 1, "type": "VSSequence", "children": [2, 3, 4] },
    { "id": 2, "type": "AtomicTask", "taskType": "Task_PatrolPickPoint" },
    { "id": 3, "type": "AtomicTask", "taskType": "Task_SetMoveGoal" },
    { "id": 4, "type": "AtomicTask", "taskType": "Task_MoveToLocation" }
  ],
  "execConnections": [
    { "fromNode": 5, "fromPin": "Out", "toNode": 1, "toPin": "In" }
  ]
}
```

### Complex case: guard_combat.json (Selector → nested Branch)

**Before (v2)**: Selector with 3 children [2, 5, 8]

**After (v4)**:
```json
{
  "nodes": [
    { "id": 11, "type": "EntryPoint" },
    { "id": 1, "type": "Branch", "name": "Root Selector" },
    { "id": 110, "type": "Branch", "name": "Selector_Branch_110" },
    ...
  ],
  "execConnections": [
    { "fromNode": 11, "fromPin": "Out",  "toNode": 1,   "toPin": "In" },
    { "fromNode": 1,  "fromPin": "Then", "toNode": 2,   "toPin": "In" },
    { "fromNode": 1,  "fromPin": "Else", "toNode": 110, "toPin": "In" },
    { "fromNode": 110,"fromPin": "Then", "toNode": 5,   "toPin": "In" },
    { "fromNode": 110,"fromPin": "Else", "toNode": 8,   "toPin": "In" }
  ]
}
```

---

## SubGraph System Guide

### SubGraph execution flow

```
Parent Graph
  └─► EntryPoint
        └─► SubGraph Node (subGraphPath: "Blueprints/AI/sub.json")
              │   ← loads sub.json via AssetManager
              │   ← creates child LocalBlackboard
              │   ← binds InputParams
              │   ← calls ExecuteFrame(child)
              │   ← writes OutputParams back to parent BB
              └─► Out → next node in parent
```

### Creating a reusable SubGraph

```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "PatrolSubGraph",
  "isSubGraph": true,
  "inputParameters": [
    { "name": "Speed", "type": "Float" }
  ],
  "outputParameters": [
    { "name": "Success", "type": "Bool" }
  ],
  "nodes": [ ... ],
  "execConnections": [ ... ]
}
```

### Calling a SubGraph with parameters

```json
{
  "id": 5,
  "type": "SubGraph",
  "name": "CallPatrol",
  "subGraphPath": "Blueprints/AI/patrol_sub.json",
  "InputParams": {
    "Speed": { "Type": "LocalVariable", "VariableName": "local:PatrolSpeed" }
  },
  "OutputParams": {
    "Success": "local:PatrolResult"
  }
}
```

---

## SubGraph Tests (6/6 passing)

File: `Tests/TaskSystem/SubGraphCompleteTest.cpp`

| Test | Description | Status |
|---|---|---|
| Test 1 | SubGraphCallStack basic push/pop/contains/depth | ✅ PASS |
| Test 2 | Depth counter tracks MAX_SUBGRAPH_DEPTH (4) | ✅ PASS |
| Test 3 | Cycle detection: A→B→C→A detected | ✅ PASS |
| Test 4 | Empty SubGraphPath skips gracefully | ✅ PASS |
| Test 5 | Missing file skips gracefully | ✅ PASS |
| Test 6 | Real file (idle.json) loads and executes | ✅ PASS |

### MAX_SUBGRAPH_DEPTH = 4

SubGraph nesting is limited to 4 levels. Exceeding this limit logs an error and
falls through to the "Out" connection of the SubGraph node.

```
A → B → C → D     (depth=4, OK)
A → B → C → D → E (depth=5, ERROR: Maximum subgraph depth (4) exceeded)
```

---

## Troubleshooting

### "Invalid schema_version"
- Check that `schema_version` is present and equals 4.
- Run `python Tools/migrate_bt_to_vs.py <file>` to migrate from v2.

### "Cyclic subgraph reference detected"
- Graph A calls SubGraph B which calls SubGraph A (directly or indirectly).
- Break the cycle by restructuring the graph logic.
- The callStack error message shows the full chain: `A -> B -> A`.

### "Maximum subgraph depth (4) exceeded"
- You have more than 4 levels of SubGraph nesting.
- Flatten the hierarchy or increase `MAX_SUBGRAPH_DEPTH` in `VSGraphExecutor.h`.

### "SubGraph X not found in AssetManager"
- Check that the `subGraphPath` points to a valid, existing JSON file.
- Verify the working directory when running the engine matches the expected relative path.

---

## Performance Recommendations

### SubGraph vs duplication
- Use SubGraph for behavior shared across 3+ graphs.
- For 1-2 uses, inline the nodes directly (avoids file I/O overhead).

### AssetManager caching
- SubGraph files are cached by `AssetManager` after first load.
- Repeated SubGraph calls within the same session use cache hits (no disk I/O).

### Blackboard scope
- Use `local:` scope for entity-specific data (thread-safe, per-entity).
- Use `global:` scope only for truly shared state (guards' team alert level, etc.).
- SubGraph InputParams/OutputParams use child LocalBlackboard (no global pollution).

---

## Migration Checklist Finale

- [x] All 10 production assets v2 → v4 (+ 1 test asset)
- [x] `Tools/migrate_bt_to_vs.py` batch migration script
- [x] `npc_wander_ai.json` created (was missing, caused pre-existing CI failure)
- [x] 6 SubGraph tests passing (100%)
- [x] Pre-existing CI build failure fixed (AssetManager added to OlympeVSGraphExecutorTests)
- [x] `Tests/TaskSystem/TaskGraphLoaderTest.cpp` updated for v4 structure
- [x] Backward compatibility maintained (v2 files still load via ParseSchemaV4 fallback)

---

## References

- `Source/TaskSystem/VSGraphExecutor.h` — SubGraphCallStack struct, MAX_SUBGRAPH_DEPTH
- `Source/TaskSystem/VSGraphExecutor.cpp` — HandleSubGraph implementation
- `Source/TaskSystem/TaskGraphLoader.cpp` — ParseSchemaV4 (handles v2/v3/v4)
- `Source/Core/AssetManager.h` — LoadTaskGraphFromFile, caching
- `Documentation/ATS_VS_Phase1_Migration_Notes.md`
- `Documentation/ATS_VS_Phase2_RuntimeNotes.md`
- `Documentation/Olympe_ATS_VisualScript_Complete_Doc.md`
