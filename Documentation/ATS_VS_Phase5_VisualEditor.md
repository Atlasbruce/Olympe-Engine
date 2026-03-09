# ATS Visual Scripting Phase 5 — Visual Editor & Debugger

## Overview

Phase 5 introduces a complete visual editor and runtime debugger for ATS Visual Scripting.  
It builds on the stable Phase 4 runtime (schema v4, SubGraphs, GlobalBlackboard) and adds:

- **VisualScriptEditorPanel** — ImNodes-based graph canvas for creating and editing VS graphs
- **VisualScriptNodeRenderer** — Coloured node styles and typed pin rendering
- **DebugController** — Singleton state machine for breakpoints and step-by-step execution
- **DebugPanel** — ImGui UI for debug toolbar, breakpoints list, call stack, and watch variables
- **PerformanceProfiler** — Opt-in per-node execution metrics with frame history
- **ProfilerPanel** — ImGui UI for frame timeline and hotspot table

---

## Architecture

### New Files

| File | Purpose |
|------|---------|
| `Source/BlueprintEditor/VisualScriptEditorPanel.h/.cpp` | ImNodes graph canvas for VS graphs |
| `Source/BlueprintEditor/VisualScriptNodeRenderer.h/.cpp` | Node style helpers (colours, pins, tooltips) |
| `Source/BlueprintEditor/DebugController.h/.cpp` | Debug state machine singleton (no ImGui) |
| `Source/BlueprintEditor/DebugPanel.h/.cpp` | ImGui debug UI panel |
| `Source/BlueprintEditor/PerformanceProfiler.h/.cpp` | Performance metrics singleton (no ImGui) |
| `Source/BlueprintEditor/ProfilerPanel.h/.cpp` | ImGui profiler UI panel |
| `Tests/BlueprintEditor/DebuggerTest.cpp` | DebugController unit tests (11 tests) |
| `Tests/BlueprintEditor/ProfilerTest.cpp` | PerformanceProfiler unit tests (10 tests) |
| `Tests/BlueprintEditor/VisualScriptEditorTest.cpp` | VS graph template/serialization tests (8 tests) |

### Component Relationships

```
BlueprintEditorGUI
  ├── VisualScriptEditorPanel  (graphType == "VisualScript")
  │     └── VisualScriptNodeRenderer  (node draw calls)
  ├── DebugPanel
  │     ├── DebugController::Get()   (state machine + breakpoints)
  │     └── PerformanceProfiler::Get()  (profiler summary)
  └── ProfilerPanel
        └── PerformanceProfiler::Get()  (frame data)

WorldBridge
  └── TaskExecutionBridge
        ├── DebugController::OnNodeExecuting()  (breakpoint hook)
        └── PerformanceProfiler::Begin/EndNodeExecution()  (metrics hook)
```

---

## DebugController

### State Machine

```
NotDebugging ──StartDebugging()──► Running ──Pause()──► Paused
                                     ▲                    │
                                     │                    ├── Continue() ──► Running
                                     │                    ├── StepNext() ──► StepNext ──OnNode()──► Paused
                                     │                    ├── StepInto() ──► StepInto ──OnNode()──► Paused
                                     │                    └── StepOut()  ──► StepOut  ──PopFrame()──► Paused
                                     │
                            StopDebugging() ──► NotDebugging
```

### Breakpoints

```cpp
DebugController& dc = DebugController::Get();

// Set a breakpoint
dc.SetBreakpoint(graphID, nodeID, "graph.json", "NodeName");

// Toggle (add if absent, remove if present)
dc.ToggleBreakpoint(graphID, nodeID);

// Check
if (dc.HasBreakpoint(graphID, nodeID)) { ... }

// Disable without removing
dc.SetBreakpointEnabled(graphID, nodeID, false);
```

### Integration with VSGraphExecutor

Hook `DebugController::OnNodeExecuting()` inside `VSGraphExecutor::ExecuteFrame()`:

```cpp
// At the top of each node execution:
if (DebugController::Get().IsDebugging()) {
    DebugController::Get().OnNodeExecuting(graphID, nodeID, &localBB);
}
```

For SubGraph calls, push/pop the call stack:

```cpp
// Before calling sub-graph:
DebugController::Get().PushCallFrame(subGraphID, nodeID, subGraphPath, nodeName);

// After sub-graph returns:
DebugController::Get().PopCallFrame();
```

### Thread Safety

All `DebugController` methods are mutex-protected (`std::mutex` + `std::lock_guard`).  
`OnNodeExecuting()` busy-waits when Paused unless `SetHeadlessMode(true)` is called  
(headless mode is used in unit tests to avoid blocking).

---

## PerformanceProfiler

### Usage (TaskSystem side)

```cpp
// Game loop:
PerformanceProfiler::Get().BeginFrame();

// Before each node:
PerformanceProfiler::Get().BeginNodeExecution(nodeID, nodeName);
// ... execute node ...
PerformanceProfiler::Get().EndNodeExecution(nodeID);

// After all nodes:
PerformanceProfiler::Get().EndFrame();
```

### Opt-in

Profiling is **disabled by default**.  
Call `BeginProfiling()` first (from ProfilerPanel toolbar or code).

```cpp
PerformanceProfiler::Get().BeginProfiling();
// ... collect data ...
PerformanceProfiler::Get().StopProfiling();
```

### Export CSV

```cpp
PerformanceProfiler::Get().SaveToFile("profiler_export_20260309_120000.csv");
```

CSV format:
```
FrameNumber,TotalFrameTimeMs,NodeID,NodeName,ExecTimeMs,AvgTimeMs,MaxTimeMs,ExecCount
1234,16.7,3,AttackCooldown,14.20,13.8,18.5,1234
```

---

## VisualScriptEditorPanel

### Graph Canvas

- **Right-click** on canvas → opens node palette
- **F9** (or Properties checkbox) → toggle breakpoint on selected node
- **Ctrl+S** → save to current file
- Double-click a link → destroys it (via ImNodes)

### Node Palette (right-click menu)

```
Flow Control → EntryPoint, Branch, Sequence, While, ForEach, DoOnce, Delay
Actions      → AtomicTask
Data         → GetBBValue, SetBBValue, MathOp
SubGraph     → SubGraph
```

### Node Styles

| Style | Color | Node Types |
|-------|-------|-----------|
| EntryPoint | 🟢 Green | EntryPoint |
| FlowControl | 🔵 Blue | Branch, Sequence, While, ForEach, DoOnce, Switch |
| Action | 🟠 Orange | AtomicTask |
| Data | 🟣 Purple | GetBBValue, SetBBValue, MathOp |
| SubGraph | 🩵 Teal | SubGraph |
| Delay | 🟡 Yellow | Delay |

### Data Pin Colors

| Type | Color |
|------|-------|
| Float | Blue |
| Int | Green |
| String | Yellow |
| Vector | Cyan |
| Bool | Magenta |
| EntityID | Red |

### Validation

- **Red title bar** — breakpoint set on this node
- **Green outline** — currently executing node (debug)
- **Validation errors** (bottom overlay) — node without exec-in, empty SubGraphPath

### Save / Load JSON v4

```cpp
// Load
panel.LoadTemplate(&tmpl, "Blueprints/AI/guard.json");

// Save
panel.Save();               // saves to loaded path
panel.SaveAs("new_path.json");
```

---

## DebugPanel

The DebugPanel window contains 4 collapsible sections:

### A. Toolbar

```
[▶ Continue (F5)] [⏸ Pause] [⏭ Step (F10)] [⤵ Into (F11)] [⤴ Out] [⏹ Stop]
```

Keyboard shortcuts are active when the panel is focused:

| Key | Action |
|-----|--------|
| F5 | Continue |
| F10 | Step Next |
| F11 | Step Into |
| Shift+F11 | Step Out |
| F9 | Toggle breakpoint (canvas only) |

### B. Breakpoints List

Shows all registered breakpoints with enable/disable checkboxes and per-item delete buttons.

### C. Call Stack

Live SubGraph call stack, most recent frame highlighted in green:

```
→ graph_guard.json : Node 3 "IsTargetInRange"  [CURRENT]
  graph_combat.json : Node 7 "Chase"  [SubGraph]
  graph_guard.json : Node 1 "Entry"  [Entry]
```

### D. Watch Variables

Live display of the LocalBlackboard snapshot from `DebugController::GetCurrentBlackboard()`.

---

## ProfilerPanel

### Frame Timeline

ASCII bar chart showing per-node execution time per frame (nodes > 1ms highlighted red).

### Hotspot Table

Sorted by average execution time descending.  
Nodes above `HOTSPOT_THRESHOLD_MS` (1.0ms) are highlighted.

---

## Integration Guide

### BlueprintEditorGUI

```cpp
// In Initialize():
m_VSEditorPanel = new VisualScriptEditorPanel();
m_VSEditorPanel->Initialize();

m_DebugPanel = new DebugPanel();
m_DebugPanel->Initialize();

m_ProfilerPanel = new ProfilerPanel();
m_ProfilerPanel->Initialize();

// In Render(), route by graph type:
if (activeGraph->GraphType == "VisualScript")
    m_VSEditorPanel->Render();
else
    m_NodeGraphPanel.Render();  // Legacy BT v2

m_DebugPanel->Render();
m_ProfilerPanel->Render();
```

### WorldBridge

```cpp
// After TaskExecutionBridge::Install(), also hook debugger:
TaskExecutionBridge::Install(
    [](int nodeID) {
        NodeGraphPanel::SetActiveDebugNode(nodeID);
        // DebugController hook is called from VSGraphExecutor directly
    },
    [](const LocalBlackboard* bb) {
        WorldBridge_SetBlackboard(bb);
    }
);
```

---

## CMakeLists.txt Test Targets

Three new test executables are defined in `CMakeLists.txt` under `BUILD_TESTS=ON`:

| Target | Test Name | Description |
|--------|-----------|-------------|
| `OlympeDebuggerTests` | `DebuggerTest` | 11 DebugController tests |
| `OlympeProfilerTests` | `ProfilerTest` | 10 PerformanceProfiler tests |
| `OlympeVSEditorTests` | `VSEditorTest` | 8 VS template/serialization tests |

Build and run:
```bash
cmake -S . -B /tmp/build -DBUILD_TESTS=ON -DBUILD_RUNTIME_ENGINE=OFF -DBUILD_BLUEPRINT_EDITOR_STANDALONE=OFF
make -C /tmp/build OlympeDebuggerTests OlympeProfilerTests OlympeVSEditorTests
ctest --test-dir /tmp/build -R "DebuggerTest|ProfilerTest|VSEditorTest"
```

---

## C++14 Compliance

All new files comply with the project's C++14 rules:

- ❌ No `std::optional`, structured bindings, `if constexpr`, `std::filesystem`
- ✅ `std::mutex` + `std::lock_guard` for all thread-safe singletons
- ✅ Meyers singleton pattern (`static instance` in `Get()`)
- ✅ `std::unique_ptr`, lambdas, range-for loops
- ✅ `json_helper::safeGet*()` for JSON access in TaskGraphLoader

---

## Security Notes

- `DebugController::OnNodeExecuting()` busy-waits on `Paused` in non-headless mode.  
  In headless mode (`SetHeadlessMode(true)`), the wait is skipped — required for unit tests.
- `PerformanceProfiler::SaveToFile()` uses `std::ofstream` — no path traversal check  
  (file path should be validated at the call site in production).
