# Olympe Blueprint Editor — Complete Pipeline Summary

> **Version**: 2.0 (Phase 5-8)  
> **Last Updated**: 2026-03-12  
> **Purpose**: One-page reference for the entire editor pipeline

---

## 📊 System Overview

```
╔══════════════════════════════════════════════════════════════════════════════════════╗
║                    OLYMPE BLUEPRINT EDITOR v4 — COMPLETE ECOSYSTEM                   ║
╚══════════════════════════════════════════════════════════════════════════════════════╝

┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 1: ASSET MANAGEMENT                                                            │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  [Blueprints/]              [Gamedata/]              [Templates/]                    │
│      ├─ AI/                     ├─ Configs/              ├─ guard_base.template      │
│      │   ├─ guard_ai.ats        ├─ Items/               ├─ patrol_logic.template    │
│      │   └─ patrol.ats          └─ Quests/              └─ boss_combat.template     │
│      ├─ Characters/                                                                   │
│      └─ Enemies/                                                                      │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ AssetBrowser Panel                                                           │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  📁 Blueprints/                                                              │   │
│  │    📁 AI/                                                                     │   │
│  │      📄 guard_ai.ats         (Double-click → OpenGraphInEditor)              │   │
│  │      📄 patrol.ats                                                            │   │
│  │    📁 Characters/                                                             │   │
│  │  📁 Templates/                                                                │   │
│  │    📄 guard_base.template    (Drag → ApplyTemplate)                          │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  BlueprintEditor::PreloadATSGraphs() → Scans all .ats files → TaskGraphLoader       │
│  BlueprintEditor::RefreshAssets()    → Builds AssetNode tree → AssetBrowser          │
└──────────────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 2: VISUAL EDITING (VisualScriptEditorPanel)                                   │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ Menu Bar                                                                      │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │ File | Edit | View | Debug | Tools                                           │   │
│  │  │     │      │      │       └─ Profiler, Validate, Export                   │   │
│  │  │     │      │      └─ Step Over (F10), Continue (F5), Breakpoint (F9)      │   │
│  │  │     │      └─ Panels: Asset Browser, Blackboard, Profiler, History       │   │
│  │  │     └─ Undo (Ctrl+Z), Redo (Ctrl+Y), Cut/Copy/Paste                       │   │
│  │  └─ New Graph, Open, Save (Ctrl+S), Save As, Load Template                   │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ Canvas (ImNodes)                                                              │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │                                                                               │   │
│  │   ┌─────────────┐       ┌─────────────┐       ┌─────────────┐              │   │
│  │   │ EntryPoint  │       │   Branch    │       │ AtomicTask  │              │   │
│  │   │  [Out] ─────┼──────►│[In] [True] ─┼──────►│[In]  [Out]  │              │   │
│  │   └─────────────┘       │     [False] │       └─────────────┘              │   │
│  │                         │  [Condition]│                                      │   │
│  │                         └──────┬──────┘                                      │   │
│  │                                │                                             │   │
│  │                         ┌──────┴──────┐                                      │   │
│  │                         │ GetBBValue  │  (Data connection)                   │   │
│  │                         │   [Value] ──┘                                      │   │
│  │                         └─────────────┘                                      │   │
│  │                                                                               │   │
│  │  Right-click → Node Palette:                                                 │   │
│  │    • Flow Control: EntryPoint, Branch, Sequence, While, DoOnce               │   │
│  │    • Data: GetBBValue, SetBBValue, MathOp                                    │   │
│  │    • Tasks: AtomicTask                                                        │   │
│  │    • Advanced: SubGraph, Delay                                                │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ Properties Panel (Right-docked)                                               │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  Selected Node: Branch_1                                                      │   │
│  │    Node Name:       [Branch_CheckEnemy]                                       │   │
│  │    Condition Key:   [local:HasEnemy   ]  ← Blackboard variable               │   │
│  │    True Pin Name:   [True             ]                                       │   │
│  │    False Pin Name:  [False            ]                                       │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ Blackboard Panel (Right-docked)                                               │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  ┌─────────────────┬────────┬──────────────┬───────────────┐                │   │
│  │  │ Variable Name   │ Type   │ Default Value│ Scope         │                │   │
│  │  ├─────────────────┼────────┼──────────────┼───────────────┤                │   │
│  │  │ HasEnemy        │ Bool   │ false        │ local:        │                │   │
│  │  │ EnemyHealth     │ Float  │ 100.0        │ local:        │                │   │
│  │  │ PatrolIndex     │ Int    │ 0            │ local:        │                │   │
│  │  │ GameTime        │ Float  │ 0.0          │ global:       │                │   │
│  │  └─────────────────┴────────┴──────────────┴───────────────┘                │   │
│  │  [+ Add Variable]  [Delete]  [Edit]                                          │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  Commands executed via CommandStack (Undo/Redo):                                     │
│    • CreateNodeCommand(type, pos) → AddNode() → Update canvas                        │
│    • CreateLinkCommand(from, to)  → ConnectExec() → Draw link                        │
│    • DeleteNodeCommand(nodeID)    → RemoveNode() → Refresh canvas                    │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 3: PERSISTENCE (TaskGraphLoader)                                               │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  Save (Ctrl+S):                                                                       │
│    VisualScriptEditorPanel::Save()                                                    │
│      → TaskGraphTemplate::ToJson()                                                    │
│      → Write to "Blueprints/AI/guard_ai.ats"                                          │
│                                                                                       │
│  JSON v4 Format:                                                                      │
│  {                                                                                    │
│    "schema_version": 4,                                                               │
│    "graphType": "VisualScript",                                                       │
│    "name": "Guard AI",                                                                │
│    "nodes": [                                                                         │
│      {"NodeID": 1, "NodeType": "EntryPoint", "Position": {"x": 100, "y": 200}},      │
│      {"NodeID": 2, "NodeType": "Branch", "Properties": {"ConditionKey": "local:...}}  │
│    ],                                                                                 │
│    "ExecConnections": [                                                               │
│      {"FromNodeID": 1, "FromPinName": "Out", "ToNodeID": 2, "ToPinName": "In"}       │
│    ],                                                                                 │
│    "DataConnections": [...],                                                          │
│    "Blackboard": {"Variables": [...]}                                                 │
│  }                                                                                    │
│                                                                                       │
│  Load (Double-click .ats file):                                                       │
│    BlueprintEditor::OpenGraphInEditor(path)                                           │
│      → TaskGraphLoader::LoadFromFile(path)                                            │
│      → Detect schema_version:                                                         │
│          • v4 → ParseSchemaV4() ✅ (Primary path)                                     │
│          • v3 → TaskGraphMigrator_v3_to_v4::MigrateJson() ⚠️                         │
│          • v2 → BTtoVSMigrator::Convert() ⚠️ (Legacy BT → VS)                        │
│      → Return TaskGraphTemplate                                                       │
│      → VisualScriptEditorPanel::LoadTemplate(tmpl, path)                             │
│      → Display in canvas                                                              │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 4: RUNTIME EXECUTION (VSGraphExecutor)                                         │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  GameEngine::Run() → World::Update(dt) → TaskSystem::Process(dt)                     │
│                                                                                       │
│  For each entity with TaskRunnerComponent:                                            │
│    ┌──────────────────────────────────────────────────────────────────────────┐     │
│    │ VSGraphExecutor::ExecuteFrame(entity, runner, tmpl, BB, world, dt)       │     │
│    ├──────────────────────────────────────────────────────────────────────────┤     │
│    │                                                                           │     │
│    │  CurrentNodeID = 1 (EntryPoint)                                          │     │
│    │    → HandleEntryPoint() → Follow "Out" exec pin → nextNodeID = 2         │     │
│    │                                                                           │     │
│    │  CurrentNodeID = 2 (Branch)                                              │     │
│    │    → ResolveDataPins(2)  ────┐                                           │     │
│    │         ├─ Find DataConnection: GetBBValue(nodeID=3) → Branch(nodeID=2)  │     │
│    │         └─ Read DataPinCache[3]["Value"] = 75.0f                         │     │
│    │    → HandleBranch()                                                       │     │
│    │         ├─ conditionValue = DataPinCache[2]["Condition"] = 75.0f         │     │
│    │         ├─ Evaluate: 75.0f > 50.0f → TRUE                                │     │
│    │         └─ Follow "True" exec pin → nextNodeID = 4                       │     │
│    │                                                                           │     │
│    │  CurrentNodeID = 4 (AtomicTask)                                          │     │
│    │    → HandleAtomicTask()                                                   │     │
│    │         ├─ IAtomicTask::ExecuteWithContext(entity, BB, world, dt)        │     │
│    │         ├─ Returns: TaskState::Running                                   │     │
│    │         └─ Keep CurrentNodeID = 4, exit frame (resume next frame)        │     │
│    │                                                                           │     │
│    │  [Next Frame] CurrentNodeID = 4 (AtomicTask)                             │     │
│    │    → HandleAtomicTask()                                                   │     │
│    │         ├─ IAtomicTask::ExecuteWithContext() returns TaskState::Success  │     │
│    │         └─ Follow "Completed" exec pin → nextNodeID = 5                  │     │
│    │                                                                           │     │
│    │  CurrentNodeID = 5 (SubGraph: "patrol.ats")                              │     │
│    │    → HandleSubGraph()                                                     │     │
│    │         ├─ Cycle detection: CallStack.Contains("patrol.ats")? No ✅      │     │
│    │         ├─ Depth check: CallStack.Depth < 4? Yes ✅                      │     │
│    │         ├─ Load child: TaskGraphLoader::LoadFromFile("patrol.ats")       │     │
│    │         ├─ Copy input data: DataPinCache → childBB["input:*"]            │     │
│    │         ├─ Push: CallStack.Push("patrol.ats") → Depth = 1               │     │
│    │         ├─ Recursive: VSGraphExecutor::ExecuteFrame(child)               │     │
│    │         ├─ Pop: CallStack.Pop() → Depth = 0                              │     │
│    │         ├─ Copy output data: childBB["output:*"] → DataPinCache          │     │
│    │         └─ Follow "Success" exec pin → nextNodeID = 6                    │     │
│    │                                                                           │     │
│    └──────────────────────────────────────────────────────────────────────────┘     │
│                                                                                       │
│  Instrumentation (if profiling enabled):                                             │
│    PerformanceProfiler::BeginNodeExecution(nodeID, nodeName)                         │
│    [Execute node...]                                                                  │
│    PerformanceProfiler::EndNodeExecution(nodeID)                                     │
│      → Update NodeExecutionMetrics (count, avg, max, total)                          │
│                                                                                       │
│  Breakpoint check (if debugging enabled):                                            │
│    if (DebugController::Get().HasBreakpoint(CurrentNodeID))                          │
│      → DebugController::OnBreakpointHit(entity, CurrentNodeID)                       │
│      → Set DebugState::Paused                                                        │
│      → Capture call stack, snapshot Blackboard                                       │
│      → Exit frame early (resume when user clicks Continue/StepOver)                  │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 5: DEBUGGING & PROFILING                                                       │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ DebugPanel                                                                    │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  Toolbar: [▶ Continue (F5)] [⏸ Pause] [⏭ Step Over (F10)] [⏬ Step Into]   │   │
│  │                                                                               │   │
│  │  Breakpoints:                                                                 │   │
│  │    ✓ Node 5: Branch_CheckEnemy (guard_ai.ats:25)                             │   │
│  │    ✓ Node 12: AtomicTask_Attack (combat.ats:42)                              │   │
│  │                                                                               │   │
│  │  Call Stack:                                                                  │   │
│  │    #0  AtomicTask_Pathfind (guard_ai.ats:30)                                 │   │
│  │    #1  SubGraph_Patrol (patrol.ats:15)                                       │   │
│  │    #2  EntryPoint (guard_ai.ats:5)                                           │   │
│  │                                                                               │   │
│  │  Watch Variables:                                                             │   │
│  │    local:EnemyHealth   = 75.0f                                               │   │
│  │    local:PatrolIndex   = 2                                                   │   │
│  │    global:GameTime     = 123.45f                                             │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ ProfilerPanel                                                                 │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  Frame Timeline (Last 60 frames):                                            │   │
│  │    ████████████████████████████████████████  Frame 1: 2.34ms                │   │
│  │    █████████████████████████████████         Frame 2: 1.89ms                │   │
│  │    █████████████████████████████████████████ Frame 3: 2.45ms                │   │
│  │                                                                               │   │
│  │  Hotspot Table (Sorted by Avg Time):                                         │   │
│  │  ┌─────────────────┬───────┬──────────┬──────────┬───────────┐             │   │
│  │  │ Node Name       │ Count │ Avg (ms) │ Max (ms) │ Total (ms)│             │   │
│  │  ├─────────────────┼───────┼──────────┼──────────┼───────────┤             │   │
│  │  │ PathfindToTarget│  120  │  0.45    │  1.23    │   54.0    │             │   │
│  │  │ UpdateVision    │  240  │  0.12    │  0.34    │   28.8    │             │   │
│  │  │ Branch_CheckHP  │  480  │  0.02    │  0.05    │    9.6    │             │   │
│  │  └─────────────────┴───────┴──────────┴──────────┴───────────┘             │   │
│  │                                                                               │   │
│  │  [Export CSV]  [Clear Data]  [Auto-scroll: ON]                               │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │ ValidationPanel                                                               │   │
│  ├─────────────────────────────────────────────────────────────────────────────┤   │
│  │  🔴 CRITICAL (0)                                                              │   │
│  │  ⚠️  ERROR (2)                                                                │   │
│  │    • Node 5 (Branch): Missing required property 'ConditionKey'               │   │
│  │      Fix: Set ConditionKey in Properties panel                               │   │
│  │    • Node 12: DataConnection type mismatch (Float → Bool)                    │   │
│  │      Fix: Insert type conversion node or change pin type                     │   │
│  │  ⚠️  WARNING (1)                                                              │   │
│  │    • Node 8 (AtomicTask): Unreachable (no incoming exec connections)         │   │
│  │      Fix: Connect an exec pin to this node or delete it                      │   │
│  │                                                                               │   │
│  │  [Validate Now]  [Auto-validate: ON]                                         │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────┘


┌──────────────────────────────────────────────────────────────────────────────────────┐
│ LAYER 6: ADVANCED FEATURES                                                           │
├──────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                       │
│  ┌────────────────────────────────────────────────────────────────────────────┐     │
│  │ Template System                                                             │     │
│  ├────────────────────────────────────────────────────────────────────────────┤     │
│  │                                                                              │     │
│  │  1. Create graph → File → Save as Template                                  │     │
│  │     Input: Name, Description, Category                                      │     │
│  │     Output: Blueprints/Templates/uuid.template                              │     │
│  │                                                                              │     │
│  │  2. Browse catalog → TemplateLibraryPanel                                   │     │
│  │     Categories: AI, Character, Enemy, Environment                           │     │
│  │     Preview: Thumbnail, Description, Author, Version                        │     │
│  │                                                                              │     │
│  │  3. Apply template → Merge with current graph                               │     │
│  │     TaskGraphLoader::ParseSchemaV4() → LoadTemplate()                       │     │
│  └────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                       │
│  ┌────────────────────────────────────────────────────────────────────────────┐     │
│  │ SubGraph System (Phase 8)                                                   │     │
│  ├────────────────────────────────────────────────────────────────────────────┤     │
│  │                                                                              │     │
│  │  Schema v5 Format:                                                           │     │
│  │  {                                                                           │     │
│  │    "schema_version": 5,                                                      │     │
│  │    "data": {                                                                 │     │
│  │      "rootGraph": { "nodes": [...], "ExecConnections": [...] },             │     │
│  │      "subgraphs": {                                                          │     │
│  │        "<uuid-patrol>": {                                                    │     │
│  │          "uuid": "<uuid-patrol>",                                            │     │
│  │          "name": "Patrol Behavior",                                          │     │
│  │          "nodes": [...],                                                     │     │
│  │          "inputPins": ["start", "patrolRadius"],                             │     │
│  │          "outputPins": ["success", "failure"]                                │     │
│  │        }                                                                     │     │
│  │      }                                                                       │     │
│  │    }                                                                         │     │
│  │  }                                                                           │     │
│  │                                                                              │     │
│  │  Runtime Features:                                                           │     │
│  │    • Cycle detection: DFS via SubGraphCallStack::Contains()                 │     │
│  │    • Depth limit: MAX_SUBGRAPH_DEPTH = 4                                    │     │
│  │    • Input/output data passing via Blackboard scopes                        │     │
│  │    • Tab-based navigation: RootTab ↔ SubGraphTab1 ↔ SubGraphTab2           │     │
│  └────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                       │
│  ┌────────────────────────────────────────────────────────────────────────────┐     │
│  │ Command System (Undo/Redo)                                                  │     │
│  ├────────────────────────────────────────────────────────────────────────────┤     │
│  │                                                                              │     │
│  │  CommandStack:                                                               │     │
│  │    Commands = [Create Node, Create Link, Delete Node, ...]                  │     │
│  │    CurrentIndex = 2  (points to next redo)                                  │     │
│  │                                                                              │     │
│  │  Operations:                                                                 │     │
│  │    • Execute(cmd) → cmd->Execute() → push to stack → CurrentIndex++         │     │
│  │    • Undo() → CurrentIndex-- → Commands[index]->Undo()                      │     │
│  │    • Redo() → Commands[index]->Execute() → CurrentIndex++                   │     │
│  │                                                                              │     │
│  │  Composite Commands:                                                         │     │
│  │    BatchDelete(nodes[1,2,3]) → CompositeCommand([Delete1, Delete2, Delete3])│     │
│  │    Single Undo → reverts all 3 deletes                                      │     │
│  └────────────────────────────────────────────────────────────────────────────┘     │
│                                                                                       │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 🎯 Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| **BlueprintEditor** | `Source/BlueprintEditor/blueprinteditor.h/cpp` | Singleton backend (lifecycle, assets, state) |
| **BlueprintEditorGUI** | `Source/BlueprintEditor/BlueprintEditorGUI.h/cpp` | Frontend UI (menu bar, panels) |
| **VisualScriptEditorPanel** | `Source/BlueprintEditor/VisualScriptEditorPanel.h/cpp` | v4 editor (canvas, palette, properties) |
| **TaskGraphLoader** | `Source/TaskSystem/TaskGraphLoader.cpp` | Multi-version loader (v2/v3/v4 migration) |
| **VSGraphExecutor** | `Source/TaskSystem/VSGraphExecutor.h/cpp` | Runtime execution engine |
| **PerformanceProfiler** | `Source/BlueprintEditor/PerformanceProfiler.h/cpp` | Per-node profiling system |
| **DebugController** | `Source/BlueprintEditor/DebugController.h/cpp` | Breakpoint & step control system |
| **TemplateManager** | `Source/BlueprintEditor/TemplateManager.h/cpp` | Template catalog & CRUD |
| **CommandStack** | `Source/NodeGraphCore/CommandStack.h/cpp` | Undo/redo system |

---

## 📝 Data Formats

### Schema v4 (Current)
```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "name": "Guard AI",
  "nodes": [
    {"NodeID": 1, "NodeType": "EntryPoint", "Position": {"x": 100, "y": 200}},
    {"NodeID": 2, "NodeType": "Branch", "Properties": {"ConditionKey": "local:HasEnemy"}}
  ],
  "ExecConnections": [
    {"FromNodeID": 1, "FromPinName": "Out", "ToNodeID": 2, "ToPinName": "In"}
  ],
  "DataConnections": [],
  "Blackboard": {"Variables": [{"Key": "HasEnemy", "Type": "Bool", "DefaultValue": false}]}
}
```

### Schema v5 (SubGraphs)
```json
{
  "schema_version": 5,
  "data": {
    "rootGraph": { "nodes": [...], "ExecConnections": [...] },
    "subgraphs": {
      "<uuid>": {"uuid": "<uuid>", "name": "Patrol", "nodes": [...]}
    }
  }
}
```

---

## ⌨️ Keyboard Shortcuts

| Key | Action | Context |
|-----|--------|---------|
| **F5** | Continue (Resume execution) | Debugging |
| **F9** | Toggle Breakpoint | Selected node |
| **F10** | Step Over | Debugging |
| **F11** | Step Into (SubGraph) | Debugging |
| **Ctrl+S** | Save Graph | Editor |
| **Ctrl+Z** | Undo | Editor |
| **Ctrl+Y** | Redo | Editor |
| **Ctrl+C** | Copy Nodes | Editor |
| **Ctrl+V** | Paste Nodes | Editor |
| **Delete** | Delete Selected Nodes | Editor |

---

## 📚 Documentation Quick Links

| Document | Purpose | Read Time |
|----------|---------|-----------|
| [User Guide v4](Blueprint_Editor_User_Guide_v4.md) | Beginner workflows | 20 min |
| [Advanced Systems](Blueprint_Editor_Advanced_Systems.md) | Deep dive (SubGraphs, Profiler, etc.) | 45 min |
| [Visual Diagrams](Blueprint_Editor_Visual_Diagrams.md) | Interactive Mermaid flowcharts | 15 min |
| [Documentation Index](README_Documentation_Index.md) | Master index with learning paths | 5 min |

---

## 🚀 5-Minute Quick Start

```
1. View → VS Graph Editor → New Graph
2. Right-click → Flow Control → EntryPoint
3. Right-click → Flow Control → Branch
4. Drag from EntryPoint "Out" → Branch "In"
5. Select Branch → Properties → ConditionKey = "local:HasEnemy"
6. View → Blackboard → Add Variable: "HasEnemy" (Bool)
7. Right-click → Tasks → AtomicTask
8. Connect Branch "True" → AtomicTask "In"
9. Ctrl+S → Save as "my_first_ai.ats"
10. F9 on Branch → Set breakpoint
11. Play game → Watch execution pause at breakpoint
```

**Next Steps**: Read [User Guide v4](Blueprint_Editor_User_Guide_v4.md) Section 3 for full tutorial.

---

**Built with** ❤️ **by Olympe Engine Team**
