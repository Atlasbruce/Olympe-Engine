# ATS Visual Scripting — User Manual

**Version**: 4.0  
**System**: Olympe Engine — ATS (Autonomous Task System) Visual Scripting  
**Audience**: Game developers, AI designers, QA engineers

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Node Reference](#2-node-reference)
3. [Blackboard System](#3-blackboard-system)
4. [SubGraphs](#4-subgraphs)
5. [Debugging](#5-debugging)
6. [Performance Tips](#6-performance-tips)
7. [Migration Guide (BT v2 → VS v4)](#7-migration-guide)
8. [Troubleshooting](#8-troubleshooting)

---

## 1. Getting Started

### 1.1 Installation

ATS Visual Scripting ships as part of Olympe Engine.  No separate install step is
required.  Open the Olympe Blueprint Editor from the main editor toolbar.

```
Main Menu → Tools → Blueprint Editor (Ctrl+Shift+B)
```

### 1.2 Creating Your First Graph

1. **New graph**: Click **File → New Visual Script** or press `Ctrl+N`.
2. **Add an EntryPoint**: Right-click on the canvas → *Add Node* → **EntryPoint**.
   Every graph must have exactly one EntryPoint node.
3. **Add a Sequence**: Right-click → *Control Flow* → **Sequence**.
4. **Connect nodes**: Drag from the EntryPoint `out` pin to the Sequence `in` pin.
5. **Add tasks**: Right-click → *Actions* → **AtomicTask**, choose a task type in the
   Properties panel.
6. **Save**: `Ctrl+S` — choose a path under `Blueprints/AI/`.

### 1.3 Canvas Navigation

| Action | Input |
|--------|-------|
| Pan | Middle-mouse drag or `Alt+LMB` drag |
| Zoom | Mouse wheel |
| Zoom to Fit | `F` |
| Open Search Palette | `Ctrl+Space` |
| Toggle Grid Snap | `G` (default 16 px) |
| Multi-select | `Shift+Click` or rubber-band drag |

### 1.4 Quick Node Search (Ctrl+Space)

Press `Ctrl+Space` to open the Node Search Palette.  Type any partial name — fuzzy
matching finds the closest nodes.  Press `Enter` to create the top result at the cursor
position.

**Example**: type `brnch` to find **Branch**; type `mov` to find **MoveToLocation**.

---

## 2. Node Reference

ATS VS v4 provides **28 node types** in three categories.

### 2.1 Control Flow Nodes

#### EntryPoint
- **Purpose**: Defines where graph execution begins.
- **Pins**: `out` (exec)
- **Parameters**: none
- **Rules**: Exactly one per graph. The graph will fail validation without it.
- **Tooltip**: "The unique start of this graph. Execution always begins here."

#### Sequence
- **Purpose**: Executes child branches in order; fails as soon as one child fails.
- **Pins**: `in` (exec), `child0..childN` (exec), `out` (exec)
- **Parameters**: none
- **Tooltip**: "Runs children left-to-right. Stops and fails on first failure."

#### Selector
- **Purpose**: Tries child branches in order; succeeds as soon as one child succeeds.
- **Pins**: `in` (exec), `child0..childN` (exec), `out` (exec)
- **Parameters**: none
- **Tooltip**: "Runs children left-to-right. Stops and succeeds on first success."

#### Branch
- **Purpose**: Conditional branch — evaluates a Blackboard boolean key.
- **Pins**: `in` (exec), `true` (exec), `false` (exec)
- **Parameters**:
  - `conditionKey` — Blackboard key to evaluate (bool)
  - `operator` — optional: `>`, `<`, `>=`, `<=`, `==`, `!=`
  - `threshold` — optional: numeric threshold for non-bool comparisons
- **Tooltip**: "Reads conditionKey from the Blackboard. Fires 'true' or 'false' pin."

#### Repeat
- **Purpose**: Re-runs the previous node or sequence N times (or indefinitely).
- **Pins**: `in` (exec), `out` (exec)
- **Parameters**:
  - `count` — number of repetitions; `-1` = loop forever
- **Tooltip**: "Repeats the connected subgraph. count=-1 means infinite loop."

#### Wait (Control)
- **Purpose**: Pauses execution for a fixed duration.
- **Pins**: `in` (exec), `out` (exec)
- **Parameters**:
  - `duration` — seconds (float)
- **Tooltip**: "Suspends this branch for 'duration' seconds then continues."

#### SubGraph
- **Purpose**: Embeds and executes another `.json` VS graph as a sub-routine.
- **Pins**: `in` (exec), `out` (exec)
- **Parameters**:
  - `subGraphPath` — relative path to the child graph file
- **Hover preview**: tooltip shows child graph path and node count.
- **Tooltip**: "Loads and executes the referenced VS graph. Output fires when child completes."

#### ZoomToFit (editor-only)
- **Purpose**: Fit all nodes in view (press `F`).
- Not a runtime node; editor utility only.

### 2.2 Action Nodes (AtomicTask)

Each AtomicTask node wraps a registered `IAtomicTask` implementation.

#### MoveToLocation
- `targetKey` — Blackboard key containing the destination (string or Vector2).
- `speedKey` — optional Blackboard key for movement speed.
- `speed` — override float speed (default `3.5`).
- **Tooltip**: "Commands the agent to move to the position stored in targetKey."

#### Attack
- `targetKey` — Blackboard key for the attack target entity ID.
- `damage` — damage per hit (float).
- **Tooltip**: "Performs an attack on the entity identified by targetKey."

#### Flee
- `awayFromKey` — Blackboard key for the threat to flee from.
- `speed` — flee speed (float, default `6.0`).
- **Tooltip**: "Moves the agent away from the entity/position in awayFromKey."

#### Patrol
- `waypointKey` — Blackboard key for the next waypoint.
- **Tooltip**: "Advances to the next waypoint in the patrol route."

#### SetVariable
- `targetKey` — Blackboard key to write.
- `value` — literal string/float/bool value, or:
- `expression` — math/bool expression referencing other keys (e.g. `Health < 20`).
- **Tooltip**: "Writes a value or expression result into a Blackboard key."

#### LogMessage
- `message` — string to emit to the system log.
- **Tooltip**: "Logs a message at runtime. Useful for debugging graph flow."

#### Wait (Action)
- `duration` — seconds to wait.
- **Tooltip**: "Pauses this branch for the specified number of seconds."

#### RequestPathfinding
- `fromKey` — start position key.
- `toKey` — destination key.
- `resultKey` — Blackboard key to write the path result.
- **Tooltip**: "Requests an async pathfinding operation and stores the result."

### 2.3 Data Nodes

#### GetVariable
- `sourceKey` — Blackboard key to read.
- `outputKey` — Blackboard key to write the value to.
- **Tooltip**: "Reads sourceKey and copies it to outputKey."

#### SetBlackboardKey
- `key` — key name.
- `type` — `string`, `float`, `int`, `bool`.
- `value` — initial value.
- **Tooltip**: "Creates or updates a Blackboard entry with the given type and value."

#### Compare
- `leftKey`, `rightKey` — keys to compare.
- `operator` — `>`, `<`, `>=`, `<=`, `==`, `!=`.
- `resultKey` — bool Blackboard key for the result.
- **Tooltip**: "Compares two Blackboard values and writes a bool result."

#### Math
- `leftKey`, `rightKey` — numeric keys.
- `op` — `+`, `-`, `*`, `/`.
- `resultKey` — float output key.
- **Tooltip**: "Performs arithmetic on two Blackboard float values."

---

## 3. Blackboard System

### 3.1 Overview

The Blackboard is the shared memory store for an AI agent's runtime state.  ATS VS v4
supports two scopes:

| Scope | Class | Lifetime |
|-------|-------|----------|
| Local | `LocalBlackboard` | Per-agent, per-execution |
| Global | `GlobalBlackboard` | Shared across all agents |

### 3.2 Declaring Keys

Keys are declared in the `blackboard.local` and `blackboard.global` sections of the
graph JSON:

```json
"blackboard": {
  "local": {
    "Health":    { "type": "float",  "value": "100.0" },
    "TargetPos": { "type": "string", "value": "" }
  },
  "global": {
    "global:TeamAlert": { "type": "bool", "value": "false" }
  }
}
```

### 3.3 Accessing Global Keys at Runtime

Prefix the key name with `global:` to read/write the global scope:

```cpp
bb.SetVar("global:TeamAlert", TaskValue("true"));
```

### 3.4 Blackboard Auto-Complete

In the node property editor, any field marked as type `BlackboardKey` renders a
dropdown populated with all declared local and global keys.  Select from the list
to avoid typos.

### 3.5 Type Safety

Values are stored as `TaskValue` (a discriminated union with `string`, `float`, `int`,
`bool` variants).  The runtime coerces types on read; mismatches are logged as warnings.

---

## 4. SubGraphs

### 4.1 What Is a SubGraph?

A SubGraph is a reusable VS graph referenced from a parent graph via a relative file
path.  SubGraphs execute to completion before the parent continues.

### 4.2 Creating a SubGraph

1. Build your sub-routine as a standalone VS graph with its own EntryPoint.
2. Save it as `Blueprints/AI/<name>.json`.
3. In the parent graph, add a **SubGraph** node and set `subGraphPath` to the relative
   path (e.g. `Blueprints/AI/patrol_loop.json`).

### 4.3 Best Practices

- Keep SubGraphs focused on a single behaviour (patrol, health check, etc.).
- Use local Blackboard keys for SubGraph-internal state; pass data between graphs via
  global keys.
- Avoid circular SubGraph references (A calls B calls A) — the validator will detect
  these as cycle warnings.
- Use the hover tooltip on a SubGraph node to verify the referenced path before running.

### 4.4 SubGraph Nesting

SubGraphs can be nested to any depth.  The call stack is visible in the **Debug Panel**
under *Call Stack*.

---

## 5. Debugging

### 5.1 Enabling the Debugger

Open the Debug Panel via **View → Debug Panel** or the toolbar bug icon.  Click
**Attach** to connect the debugger to a running AI agent.

### 5.2 Breakpoints

- Click the node header area in the canvas, or press `F9`, to toggle a breakpoint.
- Breakpointed nodes show a red indicator.
- When execution reaches a breakpoint, the graph pauses and the Debug Panel shows
  the current Blackboard state.

### 5.3 Step-by-Step Execution

| Button | Action |
|--------|--------|
| Continue (F5) | Resume until next breakpoint |
| Step Next (F10) | Execute next node, stay at same SubGraph level |
| Step Into (F11) | Enter SubGraph on the next SubGraph node |
| Step Out (Shift+F11) | Complete current SubGraph and return to parent |
| Stop (Shift+F5) | Terminate execution |

### 5.4 Watch Variables

The *Watch* panel in the Debug window lists all Blackboard keys and their live values,
updated each frame while paused.

### 5.5 Call Stack

Shows the SubGraph call hierarchy:

```
[0] guard_ai.json (current)
[1] patrol_loop.json
```

Click any entry to jump the canvas to that SubGraph.

### 5.6 Execution Highlight

The currently executing node is highlighted in green.  Previously executed nodes fade
to a dimmer tint.

---

## 6. Performance Tips

### 6.1 Graph Size Limits

- Recommended maximum: **200 nodes** per graph (60+ FPS with viewport culling enabled).
- Large graphs (200–500 nodes): use SubGraphs to split the graph — each SubGraph is
  only loaded when executed.
- Above 500 nodes: consider splitting into multiple asset files.

### 6.2 Viewport Culling

Ensure `ViewportCulling` is enabled (default `ON`) in the editor settings.  Only nodes
within the visible canvas area are rendered, reducing draw calls for large graphs.

### 6.3 Async JSON Loading

Large graphs are loaded in the background via `AsyncJSONLoader`.  A loading spinner
appears while the load is in progress.  Do not use the graph until the spinner clears.

### 6.4 Connection Cache

Bézier control points for connections are cached.  If you observe stale connection
curves after moving many nodes, use **Edit → Refresh Connections** to rebuild the cache.

### 6.5 Profiler Sampling Mode

Enable *Sampling Mode* in the Profiler Panel to collect metrics every 10th frame
instead of every frame.  This reduces CPU overhead by ~40% on large graphs.

---

## 7. Migration Guide

### 7.1 BT v2 → VS v4 Auto-Migration

ATS VS v4 includes an automatic migrator for legacy BehaviorTree v2 JSON assets.

**Via the editor**:

1. Open **File → Open Blueprint**.
2. Select a v2 `.json` file.
3. The editor detects the `blueprintType: "BehaviorTree"` marker and runs
   `BTtoVSMigrator::Migrate()` automatically.
4. Review the converted graph and save as a new v4 asset.

**Via the command-line tool**:

```bash
python3 Tools/migrate_bt_to_vs.py --input Blueprints/AI/old_guard.json \
                                    --output Blueprints/AI/guard_vs.json
```

### 7.2 Node Type Mapping

| BT v2 Type | VS v4 Type |
|------------|-----------|
| `BT_Sequence` | `Sequence` |
| `BT_Selector` | `Selector` |
| `BT_Action` | `AtomicTask` |
| `BT_Condition` | `Branch` |
| `BT_Decorator_Repeat` | `Repeat` |

### 7.3 Known Migration Limitations

- Decorator parameters (repeat count, timeout) are migrated on a best-effort basis.
- Custom `BT_Action` types not registered in `AtomicTaskRegistry` appear as
  `LogMessage` nodes with a "TODO: implement" message — replace them manually.
- BT v2 `BT_Parallel` has no direct VS v4 equivalent; manual rework is required.

---

## 8. Troubleshooting

### Q: The graph shows "No EntryPoint found" validation error.
**A**: Every VS graph must contain exactly one `EntryPoint` node.  Add one via
`Ctrl+Space` → type "entry" → Enter.

### Q: A SubGraph node shows an orange "missing path" icon.
**A**: The `subGraphPath` parameter is empty or the file does not exist.
Open the node's Properties panel and set a valid path.

### Q: Execution seems to loop infinitely.
**A**: Check for `Repeat` nodes with `count = -1`.  The graph validator will warn
about `Repeat` cycles without a `Branch` escape condition.  Add a Branch node with
a termination condition to break the loop.

### Q: Copy/paste produces nodes with no connections.
**A**: Only connections *between* the selected nodes are preserved.  Connections to
nodes outside the selection are intentionally dropped.

### Q: The minimap is showing an empty canvas.
**A**: `MinimapWidget::UpdateNodes()` must be called each frame with the current node
positions.  Verify the graph editor is calling `MinimapWidget::Get().UpdateNodes(…)`.

### Q: Breakpoints are not pausing execution.
**A**: Ensure the debugger is in `Running` state (not `NotDebugging`).  Call
`DebugController::Get().StartDebugging()` before beginning graph execution.

### Q: JSON load fails with "invalid JSON payload".
**A**: Validate the file with a JSON linter (e.g. `python3 -m json.tool <file>`).
Common issues: trailing commas, missing closing braces, BOM characters.

### Q: Fuzzy search does not find a node I expect.
**A**: The search requires at least one character from the node name to appear in the
query.  Try a different substring.  Use `NodeSearchCategory::All` (default) to avoid
category filtering.

---

*Olympe Engine — ATS Visual Scripting v4.0 — 2026*
