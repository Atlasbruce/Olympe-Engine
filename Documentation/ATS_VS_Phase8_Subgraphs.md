# ATS Visual Scripting — Phase 8: Subgraph System Architecture

## Overview

Phase 8 introduces a **subgraph system** that lets users create reusable, modular AI/BT logic
through nested graph composition.  A parent graph can contain `BT_SubGraph` nodes that reference
self-contained child graphs stored in the same blueprint file under `data.subgraphs`.

---

## Data Format Migration (Flat Dictionary — Schema v5)

### Legacy Format (schema_version ≤ 4)

```json
{
  "schema_version": 2,
  "blueprintType": "BehaviorTree",
  "data": {
    "rootNodeId": 1,
    "nodes": [ ... ],
    "links": [ ... ]
  }
}
```

### New Format (schema_version 5)

```json
{
  "schema_version": 5,
  "blueprintType": "BehaviorTree",
  "data": {
    "rootGraph": {
      "rootNodeId": 1,
      "nodes": [ ... ],
      "links": [ ... ]
    },
    "subgraphs": {
      "<uuid>": {
        "uuid": "<uuid>",
        "name": "Patrol Behavior",
        "blueprintType": "BehaviorTree",
        "nodes": [ ... ],
        "links": [ ... ],
        "inputPins":  [ "start", "target" ],
        "outputPins": [ "success", "failure" ]
      }
    }
  }
}
```

### Migration Strategy

Migration is **automatic on load** via `SubgraphMigrator`.  The migrator:

1. Detects the legacy format (`data.nodes` exists directly).
2. Wraps the existing nodes/links under `data.rootGraph`.
3. Creates an empty `data.subgraphs` dictionary.
4. Bumps `schema_version` to 5.
5. Persists the migrated file so subsequent loads skip migration.

---

## New Node Types

```cpp
enum class NodeType {
    // …existing types…
    BT_SubGraph,    // References a subgraph by UUID (BehaviorTree)
    HFSM_SubGraph,  // References a subgraph by UUID (HFSM)
};

struct GraphNode {
    // …existing fields…
    std::string subgraphUUID;  // Non-empty for BT_SubGraph / HFSM_SubGraph nodes
};
```

A `BT_SubGraph` node serialises as:

```json
{
  "id": 5,
  "type": "SubGraph",
  "name": "Call Patrol",
  "subgraphUUID": "<uuid>",
  "position": { "x": 400, "y": 300 }
}
```

---

## Tab-Based Navigation

`NodeGraphPanel` now maintains a vector of `GraphTab` entries.

```cpp
struct GraphTab {
    std::string tabID;        // "root" or UUID string
    std::string displayName;  // "Root" or subgraph name
    std::string graphPath;    // "root" or "subgraphs/<uuid>"
    bool        isDirty;
};
```

| Action | Result |
|--------|--------|
| Double-click a `BT_SubGraph` node | Opens the referenced subgraph in a new tab |
| Click "+ New SubGraph" | Prompts for a name and creates an empty subgraph tab |
| Click × on a subgraph tab | Closes that tab (root tab cannot be closed) |

---

## Validation & Safety

`SubgraphMigrator::ValidateSubgraphReferences()` performs:

1. **UUID existence check** — every `subgraphUUID` referenced by a node must exist in
   `data.subgraphs`.
2. **Circular dependency detection** — DFS over the subgraph reference graph; any cycle
   is reported as an error.

`BehaviorTreeEditorPlugin::Validate()` now calls this validator automatically.

---

## Asset Browser

`BlueprintEditor::ScanDirectory()` already recurses into subdirectories
(e.g. `Blueprints/AI/`, `Blueprints/AI/test/`).  All JSON blueprint files found
in any nested folder appear in the Asset Browser tree view.

---

## Files Added / Modified

| File | Change |
|------|--------|
| `Source/BlueprintEditor/SubgraphMigrator.h` | **New** — SubgraphMigrator API |
| `Source/BlueprintEditor/SubgraphMigrator.cpp` | **New** — SubgraphMigrator implementation |
| `Source/BlueprintEditor/BTNodeGraphManager.h` | Added `BT_SubGraph`, `HFSM_SubGraph` enum values; `subgraphUUID` field on `GraphNode` |
| `Source/BlueprintEditor/BTNodeGraphManager.cpp` | `FromJson` handles new `rootGraph` format; `ToJson` serialises `subgraphUUID`; `LoadGraph` auto-migrates on load |
| `Source/BlueprintEditor/NodeGraphPanel.h` | Added `GraphTab` struct; subgraph tab system fields and methods |
| `Source/BlueprintEditor/NodeGraphPanel.cpp` | Implemented `RenderSubgraphTabBar`, `OpenSubgraphTab`, `CloseSubgraphTab`, `CreateEmptySubgraph` |
| `Source/BlueprintEditor/blueprinteditor.cpp` | `MigrateAllBlueprints` applies both v1→v2 and v2→v5 migrations |
| `Source/BlueprintEditor/BehaviorTreeEditorPlugin.cpp` | `Validate` calls `SubgraphMigrator::ValidateSubgraphReferences` |
| `Tests/BlueprintEditor/Phase8Test.cpp` | **New** — 18 unit tests for SubgraphMigrator |
| `CMakeLists.txt` | Added `OlympePhase8Tests` test target |

## Files Removed

| File | Reason |
|------|--------|
| `Source/BlueprintEditor/blueprint_test.cpp` | Legacy unit test (superseded by `Tests/` suite) |
| `Source/BlueprintEditor/asset_explorer_test.cpp` | Legacy unit test (superseded by `Tests/` suite) |
