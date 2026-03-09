# ATS Visual Scripting Phase 3 – Complete Reference

**Date:** 2026-03-09
**Status:** ✅ Production Ready

---

## Overview

Phase 3 completes the ATS Visual Scripting (VS) migration:

1. **GlobalBlackboard** scoped access (`global:key` / `local:key`)
2. **SubGraph** external files with parameter binding and anti-cycle safeguards
3. **Legacy cleanup** – deprecated `CurrentNodeIndex`, `LocalBlackboardData`,
   `ParseSchemaLegacy`, `ParseNodeLegacy` removed
4. **Asset migration** – all BehaviorTree JSON assets converted to schema v4
5. **Migration tool** – `tools/migrate_bt_to_vs.py`

---

## 1. GlobalBlackboard Scoped Access

`LocalBlackboard` now exposes `GetValueScoped()` / `SetValueScoped()` which
parse an optional `local:` or `global:` prefix:

| Prefix | Storage |
|--------|---------|
| `global:GamePhase` | `GlobalBlackboard` singleton |
| `local:Health` | this entity's `LocalBlackboard` |
| `Health` (no prefix) | same as `local:` |

### C++ usage

```cpp
// Write a global variable
GlobalBlackboard::Get().SetVar("GamePhase", TaskValue(std::string("Combat")));

// Read it from a VS graph GetBBValue node:
// { "nodeType": "GetBBValue", "BBKey": "global:GamePhase" }
```

### JSON usage (GetBBValue / SetBBValue node)

```json
{ "nodeType": "GetBBValue", "BBKey": "global:GamePhase" }
{ "nodeType": "SetBBValue", "BBKey": "local:AttackResult" }
```

---

## 2. SubGraph – External Files with Parameters

A SubGraph node loads and executes a separate VS graph JSON file within the
parent graph's execution frame.  Parameters are passed by value; the child
graph runs in an isolated `LocalBlackboard`.

### Parent graph node format

```json
{
  "id": 42,
  "type": "SubGraph",
  "label": "AttackSequence",
  "subGraphPath": "Blueprints/AI/SubGraphs/AttackSequence.json",
  "InputParams": {
    "TargetEntity": { "Type": "LocalVariable", "VariableName": "CurrentTarget" },
    "AttackDamage": { "Type": "Literal", "LiteralValue": 25.0 }
  },
  "OutputParams": {
    "Success": "local:AttackResult"
  }
}
```

### SubGraph file format

```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "AttackSequence",
  "isSubGraph": true,
  "inputParameters": [
    { "name": "TargetEntity", "type": "EntityID" },
    { "name": "AttackDamage", "type": "Float" }
  ],
  "outputParameters": [
    { "name": "Success", "type": "Bool" }
  ],
  "nodes": [ ... ],
  "execConnections": [ ... ]
}
```

### Parameter binding types

| `Type` field | Meaning | Required fields |
|---|---|---|
| `Literal` | A hard-coded value | `LiteralValue` |
| `LocalVariable` | Read from parent's blackboard | `VariableName` |

### Output binding

`OutputParams` maps a SubGraph-internal variable name to a target BB key in
the parent scope (supports `local:` / `global:` prefixes).

---

## 3. Anti-Error Safeguards

`VSGraphExecutor::HandleSubGraph` guards against all common failure modes:

| Error | Log message | Behaviour |
|---|---|---|
| File not found | `SubGraph file not found or invalid: 'path'` | Skip to Out pin |
| Recursion depth > 4 | `SubGraph recursion depth exceeded (max 4): 'path'` | Skip to Out pin |
| Cycle detected | `SubGraph cycle detected: A -> B -> A` | Skip to Out pin |
| Empty path | `SubGraph node N has no SubGraphPath` | Skip to Out pin |

The maximum nesting depth (`MAX_SUBGRAPH_DEPTH = 4`) is enforced via
`SubGraphCallStack`, a `thread_local` struct pushed/popped around each call.

---

## 4. Asset Migration

### Running the migration tool

```bash
# Migrate a single file:
python3 tools/migrate_bt_to_vs.py Blueprints/AI/guard_ai.json

# Migrate all BT assets in-place:
find Blueprints/AI -name "*.json" -exec python3 tools/migrate_bt_to_vs.py {} \;
```

The tool is idempotent — files already at `schema_version 4` are skipped.

### Node type mapping (v2 → v4)

| BT v2 type | VS v4 type | Notes |
|---|---|---|
| `Root` | `EntryPoint` | |
| `Selector` | `Selector` | Option0..N pins to children |
| `Sequence` | `VSSequence` | Start pin + Completed chain |
| `Action` | `AtomicTask` | `actionType` → `taskType` |
| `Condition` | `AtomicTask` | `conditionType` → `taskType` (leaf evaluator) |
| `Decorator` / `Repeater` | `Decorator` | |

### Migrated assets

All BehaviorTree assets in `Blueprints/AI/` have been converted to v4:

- `guard_ai.json`
- `guard_combat.json`
- `guard_patrol.json`
- `guardV2_ai.json`
- `GuardAI_V2_Complex_edited.json`
- `idle.json`
- `investigate.json`
- `npc_ai.json`
- `zombie_ai.json`
- `ZombieAITree_edited.json`
- `Tests/guardV2_ai.json`

---

## 5. Legacy Cleanup

### Removed from `TaskRunnerComponent`

| Field | Replacement |
|---|---|
| `CurrentNodeIndex` (int, default 0) | `CurrentNodeID` (int32_t, default `NODE_INDEX_NONE`) |
| `LocalBlackboardData` (vector<uint8_t>) | `LocalBlackboard` (unordered_map<string, TaskValue>) |

**Migration note:** Any code that used `runner.CurrentNodeIndex = 0` to start
execution from the first node must be updated to set
`runner.CurrentNodeID = <first node's NodeID>`.

### Removed from `TaskGraphLoader`

- `ParseSchemaLegacy()` – replaced by `ParseSchemaV4()` with nested
  `data.nodes` support.
- `ParseNodeLegacy()` – replaced by `ParseNodeV4()` with `actionType` /
  `conditionType` fallback fields.

Schema v2 JSON is now parsed directly by `ParseSchemaV4`, which handles both
the flat top-level `nodes` format and the legacy nested `data.nodes` format.

---

## 6. Build Instructions

```bash
# Linux (GCC / Clang)
cmake -S . -B build -DBUILD_TESTS=ON -DBUILD_RUNTIME_ENGINE=OFF
cmake --build build -- -j4

# Run all TaskSystem tests:
cd <repo_root>
./build/OlympeTaskSystemTests
./build/OlympeLocalBlackboardTests
./build/OlympeTaskSystemRunningTests
./build/OlympeTaskSystemE2ETests
./build/OlympeAssetManagerTests
```

---

## 7. Migration Checklist

- [x] GlobalBlackboard scoped access implemented (`local:` / `global:`)
- [x] `SubGraphParameterDef` struct added to `TaskGraphTypes.h`
- [x] `InputParams` / `OutputParams` fields added to `TaskNodeDefinition`
- [x] `IsSubGraph` / `InputParameters` / `OutputParameters` added to `TaskGraphTemplate`
- [x] `ParseNodeV4` parses `InputParams` / `OutputParams`
- [x] `ParseSchemaV4` parses `isSubGraph` / `inputParameters` / `outputParameters`
- [x] `SubGraphCallStack` struct added to `VSGraphExecutor.h`
- [x] `HandleSubGraph` fully implemented (cycle detection, depth limit, parameter binding)
- [x] `AssetManager::LoadTaskGraphFromFile()` convenience helper added
- [x] Deprecated `CurrentNodeIndex` and `LocalBlackboardData` removed
- [x] `ParseSchemaLegacy` / `ParseNodeLegacy` removed
- [x] `tools/migrate_bt_to_vs.py` migration script created
- [x] All BT assets in `Blueprints/AI/` migrated to schema v4
- [x] All tests updated and passing
- [x] Documentation complete
