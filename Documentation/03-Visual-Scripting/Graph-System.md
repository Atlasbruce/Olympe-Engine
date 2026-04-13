# Visual Script Graph System (.ats files)

The ATS Visual Script System provides a node-based programming environment for entity behavior.

## Overview

Visual Script graphs are stored as `.ats` JSON files (schema v4). They define behavior trees executed at runtime by the `BehaviorTreeSystem`.

## Graph Structure

```
TaskGraphTemplate
    ├── nodes: TaskNode[]
    ├── connections: Connection[]
    ├── presets: ConditionPreset[]  (Phase 24 – embedded)
    └── localBlackboard: BlackboardData
```

## Key Classes

| Class | File | Description |
|-------|------|-------------|
| `VisualScriptEditorPanel` | `BlueprintEditor/VisualScript*.cpp` | Main editor panel |
| `TaskGraphTemplate` | `NodeGraphCore/` | In-memory graph model |
| `TaskGraphLoader` | `BlueprintEditor/` | JSON deserialization |
| `VisualScriptNodeRenderer` | `BlueprintEditor/` | Per-node ImGui drawing |
| `BlackboardSystem` | `NodeGraphCore/BlackboardSystem.h` | Local variable store |

## Editor Features

- **Undo/Redo** (Phase 14)
- **SubGraph navigation** – double-click to enter nested graphs
- **Condition Presets** (Phase 24) – reusable conditions embedded in JSON
- **Pin-based connections** – typed execution and data pins
- **Dynamic Data Pins** – runtime-resolved pin values

## Phase History

| Phase | Feature |
|-------|---------|
| 1-7 | Core ATS grammar and visual editor |
| 8 | SubGraph support |
| 14 | Undo/Redo system |
| 24 | Condition Presets migrated to graph JSON |
| 26 | Tab-based panel UX |

## Extensive Existing Docs

See `Documentation/Docs/` for detailed phase notes:
- `ATS_VS_Phase1_Migration_Notes.md` through `ATS_VS_Phase8_Subgraphs.md`
- `Olympe_ATS_VisualScript_Complete_Doc.md`
- `UserManual_ATS_VisualScripting.md`

## Related

- [Node Catalog](../../website/docs/user-guide/visual-scripting/node-catalog.md)
- [Task Execution](../../website/docs/user-guide/visual-scripting/task-execution.md)
- [Blackboard Architecture](../01-Core-Concepts/Blackboard-Architecture.md)
