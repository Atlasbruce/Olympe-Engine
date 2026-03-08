# ATS Visual Scripting — Phase 2 Runtime Notes

**Date:** 2026-03-08  
**Author:** Olympe Engine

---

## 1. Overview

Phase 2 implements the **Visual Scripting (VS) runtime executor** for the Atomic Task System (ATS). The new `VSGraphExecutor` class processes task graphs of type `"VisualScript"` (schema v4) each frame, dispatched from `TaskSystem::Process()`.

---

## 2. Frame Execution Flow

```
TaskSystem::Process()
│
├─ For each entity with TaskRunnerComponent:
│   ├─ Retrieve TaskGraphTemplate via AssetManager
│   ├─ if tmpl->GraphType == "VisualScript"
│   │   └─ TaskSystem::ExecuteVSFrame()
│   │       ├─ Init runner.LocalBlackboard from tmpl->Blackboard (first frame)
│   │       ├─ Bridge runner.LocalBlackboard → LocalBlackboard class instance
│   │       ├─ VSGraphExecutor::ExecuteFrame(entity, runner, tmpl, localBB, world, dt)
│   │       │   │
│   │       │   ├─ if CurrentNodeID == NONE → reset to EntryPointID, return
│   │       │   │
│   │       │   └─ Loop [max 64 steps/frame]:
│   │       │       ├─ GetNode(CurrentNodeID)
│   │       │       ├─ ResolveDataPins(nodeID, ...)
│   │       │       ├─ dispatch to handler by node->Type
│   │       │       │   ├─ HandleEntryPoint   → FindExecTarget("Out")
│   │       │       │   ├─ HandleBranch       → FindExecTarget("Then"/"Else")
│   │       │       │   ├─ HandleSwitch       → FindExecTarget(caseLabel/"Default")
│   │       │       │   ├─ HandleVSSequence   → ChildrenIDs[SequenceChildIndex++]
│   │       │       │   ├─ HandleWhile        → FindExecTarget("Loop"/"Completed")
│   │       │       │   ├─ HandleDoOnce       → DoOnceFlags check → "Out"
│   │       │       │   ├─ HandleDelay        → accumulate StateTimer → "Completed"
│   │       │       │   ├─ HandleGetBBValue   → ReadBBValue → DataPinCache["N:Value"]
│   │       │       │   ├─ HandleSetBBValue   → WriteBBValue from DataPinCache
│   │       │       │   ├─ HandleMathOp       → A op B → DataPinCache["N:Result"]
│   │       │       │   ├─ HandleAtomicTask   → IAtomicTask lifecycle
│   │       │       │   └─ HandleSubGraph     → depth-limited recursion (Phase 3)
│   │       │       │
│   │       │       ├─ if nextID == NONE → end frame
│   │       │       ├─ if nextID == CurrentNodeID → end frame (multi-frame node)
│   │       │       └─ else → CurrentNodeID = nextID, continue loop
│   │       │
│   │       └─ Resync localBB → runner.LocalBlackboard
│   │
│   └─ else (BehaviorTree)
│       └─ TaskSystem::ExecuteNode() [legacy path, unchanged]
```

---

## 3. LocalBlackboard Bridging (Phase 2 Transitoire)

`TaskRunnerComponent` stores blackboard state as:
```cpp
std::unordered_map<std::string, TaskValue> LocalBlackboard;
```

`VSGraphExecutor` uses the typed `LocalBlackboard` class (which validates types).  
`TaskSystem::ExecuteVSFrame()` bridges between them each frame:

**Before execution:**
1. If `runner.LocalBlackboard` is empty and `tmpl->Blackboard` is not empty: seed `runner.LocalBlackboard` from template defaults.
2. Create a `LocalBlackboard` instance, call `InitializeFromEntries(tmpl->Blackboard)`.
3. Populate the `LocalBlackboard` instance with current `runner.LocalBlackboard` values (override defaults).

**After execution:**
4. Resync all variable values from `LocalBlackboard` back into `runner.LocalBlackboard`.

> **TODO Phase 3:** Unify `runner.LocalBlackboard` (raw map) and the `LocalBlackboard` class — likely by storing a `LocalBlackboard` instance directly in `TaskRunnerComponent`.

---

## 4. Handler Summary Table

| Node Type     | Exec Outputs           | Description |
|---------------|------------------------|-------------|
| `EntryPoint`  | `Out`                  | Immediately follows `Out` pin |
| `Branch`      | `Then`, `Else`         | Evaluates `DataPinCache["N:Condition"]`; falls back to `ConditionID` (not implemented) |
| `Switch`      | *case labels*, `Default` | Routes on `DataPinCache["N:Value"]` string/int |
| `VSSequence`  | *children* + `Out`     | Iterates `ChildrenIDs` via `SequenceChildIndex`, then `Out` |
| `While`       | `Loop`, `Completed`    | Re-evaluates condition each step |
| `DoOnce`      | `Out`                  | Single-fire; blocked by `DoOnceFlags[nodeID]` |
| `Delay`       | `Completed`            | Accumulates `StateTimer`; multi-frame node |
| `GetBBValue`  | `Out`                  | Writes to `DataPinCache["N:Value"]` |
| `SetBBValue`  | `Out`                  | Reads from `DataPinCache["N:Value"]` |
| `MathOp`      | `Out`                  | A op B → `DataPinCache["N:Result"]`; supports `+`, `-`, `*`, `/` |
| `AtomicTask`  | `Out`, `Failure`       | Full IAtomicTask lifecycle (create→tick→Running/Success/Failure) |
| `SubGraph`    | `Out`                  | Depth-limited (max 4); loading deferred to Phase 3 |

---

## 5. Data Pin Resolution

Before each node executes, `ResolveDataPins()` processes all `DataPinConnection` entries in the template where `TargetNodeID == currentNodeID`:

1. Look up `runner.DataPinCache["<sourceNodeID>:<sourcePinName>"]`.
2. If found: copy to `runner.DataPinCache["<nodeID>:<targetPinName>"]`.
3. If not found: use the `Default` value from the target node's `DataPinDefinition` for that pin.

---

## 6. Blackboard Scope Rules

| Key Format         | Behavior |
|--------------------|----------|
| `"local:key"`      | Strip prefix, read/write `LocalBlackboard` |
| `"global:key"`     | Log warning "global BB not yet implemented", return `TaskValue()` |
| `"key"` (no prefix)| Treated as `"local:key"` for legacy compatibility |

---

## 7. Anti-Infinite-Loop Guard

The `ExecuteFrame` main loop is bounded to **64 steps per frame** (`MAX_STEPS_PER_FRAME = 64`). If this limit is reached, a warning is logged and execution halts until the next frame. This prevents infinite loops in VS graphs (e.g., `While true` with no Delay or AtomicTask inside).

Multi-frame nodes (`AtomicTask Running`, `Delay`) return `runner.CurrentNodeID` from their handler, which triggers the "same node → end frame" early exit in the loop.

---

## 8. SubGraph (Phase 3 Stub)

`HandleSubGraph()` is depth-limited to `MAX_SUBGRAPH_DEPTH = 4`. Currently it only logs a message and skips to the `Out` pin. Full sub-graph loading and recursive execution is deferred to Phase 3.

---

## 9. NodeStyleRegistry VS Extensions

`NodeStyleRegistry` now provides VS node styles via `GetStyleByVSTypeName(const std::string&)`:

| VS Type Name  | Color          | Icon |
|---------------|----------------|------|
| `EntryPoint`  | Dark violet    | `>`  |
| `Branch`      | Orange         | `?`  |
| `Switch`      | Dark orange    | `/`  |
| `VSSequence`  | Light blue     | `->` |
| `While`       | Dark green     | `@`  |
| `ForEach`     | Medium green   | `#`  |
| `DoOnce`      | Dark grey      | `1`  |
| `Delay`       | Blue-violet    | `T`  |
| `GetBBValue`  | Turquoise      | `R`  |
| `SetBBValue`  | Dark teal      | `W`  |
| `MathOp`      | Pink           | `=`  |
| `SubGraph`    | Golden         | `G`  |
| `AtomicTask`  | Orange (reuses BT_Action) | `A` |

The existing `GetStyle(NodeType)` is unchanged.

---

## 10. TODO Phase 3

- [ ] Inject real `World*` instance in `TaskSystem::ExecuteVSFrame()`.
- [ ] Implement `GlobalBlackboard` service and connect `global:` scope in `LocalBlackboard::GetValueScoped` / `SetValueScoped`.
- [ ] Implement sub-graph loading in `VSGraphExecutor::HandleSubGraph()` via `AssetManager`.
- [ ] Migrate existing AI entities to VS graphs where applicable.
- [ ] Integrate `World::GetComponent` in `AtomicTaskContext` for full ECS access.
- [ ] Unify `runner.LocalBlackboard` raw map with `LocalBlackboard` class (store typed instance in `TaskRunnerComponent`).
- [ ] Implement `ConditionID` evaluation (Branch/While fallback) via a `ConditionRegistry`.
