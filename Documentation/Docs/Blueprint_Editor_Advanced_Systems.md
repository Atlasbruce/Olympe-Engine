# Olympe Blueprint Editor — Advanced Systems Documentation

> **Version**: 2.0 (Phase 5-8)  
> **Last Updated**: 2026-03-12  
> **Status**: ✅ Current — Reflects production v4 system

---

## Table of Contents

1. [Visual Pipeline Diagrams](#visual-pipeline-diagrams)
2. [SubGraph System (Phase 8)](#subgraph-system-phase-8)
3. [Performance Profiler (Phase 5)](#performance-profiler-phase-5)
4. [Debug System (Phase 5)](#debug-system-phase-5)
5. [Blackboard System (Phase 2.1)](#blackboard-system-phase-21)
6. [Template Manager (Phase 5)](#template-manager-phase-5)
7. [Validation System](#validation-system)
8. [Command System (Undo/Redo)](#command-system-undoredo)

---

## Visual Pipeline Diagrams

### 1. Complete Editor Pipeline (Creation → Execution)

```
╔════════════════════════════════════════════════════════════════════════════════╗
║                     OLYMPE BLUEPRINT EDITOR v4 — COMPLETE PIPELINE            ║
╚════════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 1: ASSET MANAGEMENT                                                       │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: Launch Standalone Editor
    │
    ▼
  BlueprintEditor::InitializeStandaloneEditor()
    │
    ├── PreloadATSGraphs()  ────────────►  Scans Blueprints/ + Gamedata/
    │                                       Loads all .ats/.json via TaskGraphLoader
    │                                       Validates schema (v2/v3/v4 auto-migration)
    │
    └── RefreshAssets()  ──────────────►  Builds AssetNode tree
                                           Parses metadata (name, type, nodes)
                                           Updates AssetBrowser display


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 2: GRAPH CREATION                                                         │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: View → VS Graph Editor → New Graph
    │
    ▼
  VisualScriptEditorPanel::NewGraph()
    │
    ├── Create TaskGraphTemplate
    │    ├── schema_version = 4
    │    ├── graphType = "VisualScript"
    │    ├── Nodes = []
    │    ├── ExecConnections = []
    │    └── DataConnections = []
    │
    ├── LoadTemplate(template, "")  ──────►  Initializes ImNodes context
    │                                         Clears m_Nodes, m_SelectedNodes
    │
    └── CommandStack::Clear()  ───────────►  Resets undo/redo history


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 3: GRAPH EDITING                                                          │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: Right-click → Flow Control → Branch
    │
    ▼
  VisualScriptEditorPanel::AddNode(TaskNodeType::Branch, x, y)
    │
    ├── CommandStack::Execute(CreateNodeCommand)
    │    ├── Generate NodeID (auto-increment)
    │    ├── Create VSEditorNode
    │    │    ├── nodeID
    │    │    ├── nodeType = TaskNodeType::Branch
    │    │    ├── position = (x, y)
    │    │    ├── inputPins = ["In", "Condition"]
    │    │    └── outputPins = ["True", "False"]
    │    │
    │    ├── Create TaskNodeDefinition
    │    │    ├── NodeID
    │    │    ├── NodeType = "Branch"
    │    │    ├── NodeName = "Branch_1"
    │    │    └── Properties["ConditionKey"] = ""
    │    │
    │    └── Add to m_CurrentTemplate.Nodes
    │
    └── MarkAsModified()  ────────────────►  m_HasUnsavedChanges = true


  User: Drag from "Out" pin to "In" pin
    │
    ▼
  ImNodes::IsLinkCreated(&startPinID, &endPinID)
    │
    ▼
  VisualScriptEditorPanel::ConnectExec(srcNode, srcPin, dstNode, dstPin)
    │
    ├── CommandStack::Execute(CreateLinkCommand)
    │    ├── Validate connection (no cycles, compatible types)
    │    ├── Create ExecPinConnection
    │    │    ├── FromNodeID = srcNode
    │    │    ├── FromPinName = "Out"
    │    │    ├── ToNodeID = dstNode
    │    │    └── ToPinName = "In"
    │    │
    │    └── Add to m_CurrentTemplate.ExecConnections
    │
    └── MarkAsModified()


  User: Ctrl+Z (Undo)
    │
    ▼
  BlueprintEditor::Undo()
    │
    └── CommandStack::Undo()  ────────────►  DeleteNodeCommand::Undo()
                                              Restores node to graph
                                              Refreshes canvas


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 4: PERSISTENCE                                                            │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: Ctrl+S (Save)
    │
    ▼
  VisualScriptEditorPanel::Save()
    │
    ├── Serialize to JSON
    │    {
    │      "schema_version": 4,
    │      "graphType": "VisualScript",
    │      "name": "Guard AI",
    │      "description": "Patrol and combat logic",
    │      "nodes": [
    │        {
    │          "NodeID": 1,
    │          "NodeType": "EntryPoint",
    │          "NodeName": "Entry",
    │          "Position": {"x": 100, "y": 200}
    │        },
    │        {
    │          "NodeID": 2,
    │          "NodeType": "Branch",
    │          "NodeName": "Check Enemy",
    │          "Properties": {"ConditionKey": "local:HasEnemy"},
    │          "Position": {"x": 300, "y": 200}
    │        }
    │      ],
    │      "ExecConnections": [
    │        {"FromNodeID": 1, "FromPinName": "Out", "ToNodeID": 2, "ToPinName": "In"}
    │      ],
    │      "DataConnections": [],
    │      "Blackboard": {
    │        "Variables": [
    │          {"Key": "HasEnemy", "Type": "Bool", "DefaultValue": false}
    │        ]
    │      }
    │    }
    │
    └── Write to "Blueprints/AI/guard_ai.ats"


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 5: ASSET LOADING                                                          │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: Double-click "guard_ai.ats" in Asset Browser
    │
    ▼
  BlueprintEditor::OpenGraphInEditor("Blueprints/AI/guard_ai.ats")
    │
    ├── DetectAssetType(filepath)  ───────►  Reads "graphType": "VisualScript"
    │
    └── VisualScriptEditorPanel::Load(path)
         │
         ├── TaskGraphLoader::LoadFromFile(path, errors)
         │    │
         │    ├── Read JSON file
         │    ├── Detect schema_version
         │    │    │
         │    │    ├── [v4] ParseSchemaV4() ──────►  Primary path
         │    │    ├── [v3] Delegate to TaskGraphMigrator_v3_to_v4::MigrateJson()
         │    │    └── [v2] Delegate to BTtoVSMigrator::Convert()
         │    │
         │    └── Return TaskGraphTemplate
         │
         └── LoadTemplate(template, path)  ─────►  Populates canvas
                                                    Creates VSEditorNode for each node
                                                    Creates ImNodes links
                                                    Initializes Blackboard panel


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 6: RUNTIME EXECUTION                                                      │
└─────────────────────────────────────────────────────────────────────────────────┘

  GameEngine::Run()  [Game Loop]
    │
    └── World::Update(dt)
         │
         └── TaskSystem::Process(dt)
              │
              ├── For each entity with TaskRunnerComponent:
              │    │
              │    ├── Get TaskGraphTemplate (preloaded from .ats file)
              │    │
              │    └── VSGraphExecutor::ExecuteFrame(entity, runner, tmpl, BB, world, dt)
              │         │
              │         ├── Resolve CurrentNodeID
              │         │
              │         ├── [EntryPoint] ──────────►  HandleEntryPoint()
              │         │                             → Advance to first successor
              │         │
              │         ├── [Branch] ──────────────►  HandleBranch()
              │         │                             → Evaluate ConditionKey in BB
              │         │                             → Follow True/False exec pin
              │         │
              │         ├── [AtomicTask] ─────────►  HandleAtomicTask()
              │         │                             → IAtomicTask::ExecuteWithContext()
              │         │                             → If Running, keep CurrentNodeID
              │         │                             → If Success, follow "Completed"
              │         │
              │         ├── [GetBBValue] ─────────►  HandleGetBBValue()
              │         │                             → Read from LocalBlackboard
              │         │                             → Store in DataPinCache[outputPin]
              │         │
              │         ├── [SetBBValue] ─────────►  HandleSetBBValue()
              │         │                             → Read from DataPinCache[inputPin]
              │         │                             → Write to LocalBlackboard
              │         │
              │         ├── [SubGraph] ──────────►  HandleSubGraph()
              │         │                             → Load child .ats graph
              │         │                             → Recursive ExecuteFrame (depth-limited)
              │         │                             → Cycle detection via CallStack
              │         │
              │         └── [While] ──────────────►  HandleWhile()
              │                                       → Evaluate condition
              │                                       → Follow "Loop" or "Completed"
              │
              └── PerformanceProfiler::EndFrame()  ──►  Record execution metrics


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 7: DEBUGGING                                                               │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: F9 on node (set breakpoint)
    │
    ▼
  DebugController::ToggleBreakpoint(nodeID)
    │
    └── runner.Breakpoints[nodeID] = true


  Runtime: VSGraphExecutor encounters breakpoint node
    │
    ▼
  DebugController::Get().OnBreakpointHit(entity, nodeID)
    │
    ├── Set DebugState::Paused
    ├── Capture call stack
    ├── Snapshot LocalBlackboard
    └── Notify DebugPanel


  User: F5 (Continue) or F10 (Step Over)
    │
    ▼
  DebugController::Continue() / StepOver()
    │
    └── Resume execution with updated state


┌─────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 8: PROFILING                                                               │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: View → Profiler → Begin Profiling
    │
    ▼
  PerformanceProfiler::Get().BeginProfiling()


  Runtime: Before node execution
    │
    ▼
  PerformanceProfiler::Get().BeginNodeExecution(nodeID, nodeName)
    │
    └── Start high-resolution timer


  Runtime: After node execution
    │
    ▼
  PerformanceProfiler::Get().EndNodeExecution(nodeID)
    │
    ├── Calculate elapsed time
    ├── Update NodeExecutionMetrics
    │    ├── executionCount++
    │    ├── avgTimeMs = (avgTimeMs * (count-1) + newTime) / count
    │    ├── maxTimeMs = max(maxTimeMs, newTime)
    │    └── totalTimeMs += newTime
    │
    └── Store in FrameProfile


  User: View → Profiler Panel
    │
    ▼
  ProfilerPanel::Render()
    │
    ├── RenderFrameTimeline()  ──────────►  Bar chart (last 60 frames)
    │
    ├── RenderHotspotTable()  ───────────►  Table sorted by avgTimeMs DESC
    │    │
    │    └── Columns: Node Name, Count, Avg (ms), Max (ms), Total (ms)
    │
    └── Export CSV  ──────────────────────►  PerformanceProfiler::SaveToFile()
```

---

### 2. SubGraph Execution Pipeline

```
╔════════════════════════════════════════════════════════════════════════════════╗
║                       SUBGRAPH SYSTEM (Phase 8) — Execution Flow              ║
╚════════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────────┐
│ CREATION: Parent Graph with SubGraph Node                                       │
└─────────────────────────────────────────────────────────────────────────────────┘

  User: Right-click → Advanced → SubGraph
    │
    ▼
  VisualScriptEditorPanel::AddNode(TaskNodeType::SubGraph, x, y)
    │
    ├── Create VSEditorNode
    │    ├── nodeType = SubGraph
    │    ├── inputPins = ["In", "InputData1", "InputData2"]
    │    ├── outputPins = ["Success", "Failure", "OutputData"]
    │    └── properties["SubGraphPath"] = ""  (empty initially)
    │
    └── Properties Panel: Select child .ats file
         │
         └── properties["SubGraphPath"] = "Blueprints/AI/patrol_behavior.ats"


  Parent Graph JSON Structure (schema_version 5):
  {
    "schema_version": 5,
    "graphType": "VisualScript",
    "name": "Guard AI",
    "data": {
      "rootGraph": {
        "nodes": [
          {
            "NodeID": 1,
            "NodeType": "EntryPoint"
          },
          {
            "NodeID": 2,
            "NodeType": "SubGraph",
            "Properties": {
              "SubGraphPath": "Blueprints/AI/patrol_behavior.ats"
            }
          }
        ],
        "ExecConnections": [...]
      },
      "subgraphs": {
        "<uuid-patrol>": {
          "uuid": "<uuid-patrol>",
          "name": "Patrol Behavior",
          "graphType": "VisualScript",
          "nodes": [...],
          "ExecConnections": [...],
          "inputPins": ["start", "patrolRadius"],
          "outputPins": ["success", "failure"]
        }
      }
    }
  }


┌─────────────────────────────────────────────────────────────────────────────────┐
│ RUNTIME EXECUTION: Depth-Limited Recursive Execution                            │
└─────────────────────────────────────────────────────────────────────────────────┘

  VSGraphExecutor::ExecuteFrame(entity, runner, parentTemplate, BB, world, dt)
    │
    ▼ CurrentNodeID = 2 (SubGraph node)
    │
    ▼
  VSGraphExecutor::HandleSubGraph(entity, nodeID=2, runner, tmpl, BB, world, dt, callStack)
    │
    ├── [1] Cycle Detection
    │    │
    │    ├── SubGraphCallStack::Contains("patrol_behavior.ats")?
    │    │    └── Yes → ERROR: Circular dependency detected!
    │    │    └── No → Continue
    │    │
    │    └── callStack.Depth >= MAX_SUBGRAPH_DEPTH (4)?
    │         └── Yes → ERROR: Max recursion depth exceeded!
    │         └── No → Continue
    │
    ├── [2] Load Child Graph
    │    │
    │    └── TaskGraphLoader::LoadFromFile("Blueprints/AI/patrol_behavior.ats", errors)
    │         │
    │         └── Returns childTemplate
    │
    ├── [3] Copy Input Data
    │    │
    │    ├── For each DataConnection to SubGraph node:
    │    │    ├── Resolve source pin value from parent DataPinCache
    │    │    └── Write to childBB["input:patrolRadius"] = 50.0f
    │    │
    │    └── childBB inherits parent LocalBlackboard (scoped "local:" variables)
    │
    ├── [4] Push Call Stack
    │    │
    │    └── callStack.Push("patrol_behavior.ats")
    │         callStack.Depth = 1
    │
    ├── [5] Recursive Execution
    │    │
    │    └── VSGraphExecutor::ExecuteFrame(entity, childRunner, childTemplate, childBB, world, dt)
    │         │
    │         ├── Execute child graph nodes (EntryPoint → Branch → AtomicTask → ...)
    │         │
    │         └── When child completes:
    │              └── childRunner.CurrentNodeID = NODE_INDEX_NONE
    │
    ├── [6] Pop Call Stack
    │    │
    │    └── callStack.Pop()
    │         callStack.Depth = 0
    │
    ├── [7] Copy Output Data
    │    │
    │    ├── Read childBB["output:enemyFound"] = true
    │    └── Write to parent DataPinCache[SubGraphNode.outputPins["OutputData"]]
    │
    └── [8] Follow Exec Output
         │
         ├── Child succeeded? → Follow "Success" exec pin
         └── Child failed?    → Follow "Failure" exec pin


┌─────────────────────────────────────────────────────────────────────────────────┐
│ VALIDATION: Cycle Detection & Depth Limit                                       │
└─────────────────────────────────────────────────────────────────────────────────┘

  Example Cycle:
    Guard AI (guard.ats)
      └── SubGraph → Patrol Behavior (patrol.ats)
            └── SubGraph → Guard AI (guard.ats)  ❌ CYCLE DETECTED!


  SubGraphCallStack State:
    PathStack = ["guard.ats", "patrol.ats"]
    Depth = 2

    When attempting to load "guard.ats" again:
      callStack.Contains("guard.ats") → TRUE
      → ERROR: "Circular SubGraph dependency detected: guard.ats"


  Example Max Depth:
    A.ats → B.ats → C.ats → D.ats → E.ats  ❌ DEPTH LIMIT EXCEEDED!
                                    (MAX_SUBGRAPH_DEPTH = 4)

    callStack.Depth = 4
    When attempting to load E.ats:
      → ERROR: "Max SubGraph recursion depth exceeded (4)"
```

---

### 3. Data Flow Architecture

```
╔════════════════════════════════════════════════════════════════════════════════╗
║                       DATA PIN RESOLUTION — Execution Order                    ║
╚════════════════════════════════════════════════════════════════════════════════╝

  Graph Example:
    [EntryPoint] ──Out──► [GetBBValue: "EnemyHealth"] ──Out──► [Branch: Condition="Health > 50"]
                              │                                      │
                              └── DataPin: Value (Float) ────────────┘


  Execution Flow:

  1. CurrentNodeID = 1 (EntryPoint)
     VSGraphExecutor::HandleEntryPoint()
       → nextNodeID = 2 (GetBBValue)


  2. CurrentNodeID = 2 (GetBBValue)
     │
     ├── ResolveDataPins(nodeID=2, runner, tmpl, BB)
     │    └── No input DataConnections → Skip
     │
     ├── HandleGetBBValue(nodeID=2, runner, tmpl, BB)
     │    ├── Read BB["local:EnemyHealth"] = 75.0f
     │    └── runner.DataPinCache[2]["Value"] = 75.0f  ✅ Store in cache
     │
     └── Follow "Out" exec pin → nextNodeID = 3 (Branch)


  3. CurrentNodeID = 3 (Branch)
     │
     ├── ResolveDataPins(nodeID=3, runner, tmpl, BB)
     │    │
     │    ├── Find DataConnection: FromNodeID=2, FromPin="Value", ToNodeID=3, ToPin="Condition"
     │    │
     │    ├── Read runner.DataPinCache[2]["Value"] = 75.0f
     │    │
     │    └── runner.DataPinCache[3]["Condition"] = 75.0f  ✅ Propagate to Branch input
     │
     ├── HandleBranch(nodeID=3, runner, tmpl, BB)
     │    ├── conditionValue = runner.DataPinCache[3]["Condition"] = 75.0f
     │    ├── Evaluate: 75.0f > 50.0f → TRUE
     │    └── Follow "True" exec pin → nextNodeID = 4
     │
     └── nextNodeID = 4


  DataPinCache Structure (per TaskRunnerComponent):
    std::map<int32_t, std::map<std::string, TaskValue>> DataPinCache;

    Example at step 3:
    {
      2: {  // NodeID = 2 (GetBBValue)
        "Value": TaskValue(Float, 75.0f)
      },
      3: {  // NodeID = 3 (Branch)
        "Condition": TaskValue(Float, 75.0f)
      }
    }


  Rule: Data pins are resolved BEFORE node execution
    → Ensures all input data is available when node logic runs
    → Supports complex data flow graphs with multiple hops
```

---

### 4. Blackboard Scoping System

```
╔════════════════════════════════════════════════════════════════════════════════╗
║                       BLACKBOARD SCOPING — local: vs global:                   ║
╚════════════════════════════════════════════════════════════════════════════════╝

┌─────────────────────────────────────────────────────────────────────────────────┐
│ Hierarchy: World → Entity → TaskRunner → LocalBlackboard                        │
└─────────────────────────────────────────────────────────────────────────────────┘

  World
    ├── GlobalBlackboard (shared across all entities)
    │    ├── "global:GameTime" = 123.45f
    │    ├── "global:PlayerHealth" = 100
    │    └── "global:QuestState" = "Active"
    │
    └── Entity[42] (Guard NPC)
         └── TaskRunnerComponent
              └── LocalBlackboard (per-entity scope)
                   ├── "local:EnemyHealth" = 75.0f
                   ├── "local:PatrolIndex" = 2
                   └── "local:IsAggro" = true


┌─────────────────────────────────────────────────────────────────────────────────┐
│ Read Operation: GetBBValue Node                                                 │
└─────────────────────────────────────────────────────────────────────────────────┘

  Node Properties:
    BBKey = "local:EnemyHealth"


  VSGraphExecutor::HandleGetBBValue(nodeID, runner, tmpl, BB)
    │
    └── VSGraphExecutor::ReadBBValue("local:EnemyHealth", localBB)
         │
         ├── Parse scope prefix:
         │    scope = "local"
         │    key = "EnemyHealth"
         │
         ├── [local:] → LocalBlackboard::GetValue("EnemyHealth")
         │    └── Returns TaskValue(Float, 75.0f)
         │
         ├── [global:] → World::GetGlobalBlackboard()->GetValue("GameTime")
         │    └── Returns TaskValue(Float, 123.45f)
         │
         └── [no prefix] → Defaults to "local:" scope


┌─────────────────────────────────────────────────────────────────────────────────┐
│ Write Operation: SetBBValue Node                                                │
└─────────────────────────────────────────────────────────────────────────────────┘

  Node Properties:
    BBKey = "local:IsAggro"
    Value = (from DataPinCache)


  VSGraphExecutor::HandleSetBBValue(nodeID, runner, tmpl, BB)
    │
    └── VSGraphExecutor::WriteBBValue("local:IsAggro", TaskValue(Bool, true), localBB)
         │
         ├── Parse scope prefix:
         │    scope = "local"
         │    key = "IsAggro"
         │
         ├── [local:] → LocalBlackboard::SetValue("IsAggro", true)
         │    └── Updates entity-specific state
         │
         └── [global:] → World::GetGlobalBlackboard()->SetValue("GameTime", 124.0f)
              └── Updates world-shared state (affects all entities)


┌─────────────────────────────────────────────────────────────────────────────────┐
│ Use Cases                                                                        │
└─────────────────────────────────────────────────────────────────────────────────┘

  ┌─────────────────┬──────────────────────────────────────────────────────────┐
  │ Scope           │ Use Case                                                 │
  ├─────────────────┼──────────────────────────────────────────────────────────┤
  │ local:          │ Entity-specific state (health, ammo, patrol index)       │
  │ global:         │ World-shared state (time, weather, quest flags)          │
  │ (no prefix)     │ Defaults to local: (entity scope)                        │
  └─────────────────┴──────────────────────────────────────────────────────────┘


  Example Graph:
    [GetBBValue: "global:GameTime"] ──► [Branch: Time > 600] ──True──► [Aggro Mode]
    [GetBBValue: "local:Health"]    ──► [Branch: Health < 30] ──True──► [Retreat]
```

---

## SubGraph System (Phase 8)

### Overview

The SubGraph system enables **modular, reusable AI logic** through nested graph composition. Parent graphs can invoke child graphs stored either:

1. **Inline** (within the same `.ats` file under `data.subgraphs`)
2. **External** (separate `.ats` files via `SubGraphPath` property)

### Schema Migration (v4 → v5)

**Legacy Format (v4)**:
```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "data": {
    "nodes": [...],
    "ExecConnections": [...]
  }
}
```

**New Format (v5)**:
```json
{
  "schema_version": 5,
  "graphType": "VisualScript",
  "data": {
    "rootGraph": {
      "nodes": [...],
      "ExecConnections": [...]
    },
    "subgraphs": {
      "<uuid>": {
        "uuid": "<uuid>",
        "name": "Patrol Logic",
        "nodes": [...],
        "inputPins": ["start", "target"],
        "outputPins": ["success", "failure"]
      }
    }
  }
}
```

**Migration** is automatic on load via `SubgraphMigrator::MigrateToV5()`.

### SubGraph Node Type

```cpp
// Node properties
{
  "NodeID": 5,
  "NodeType": "SubGraph",
  "NodeName": "Call Patrol",
  "Properties": {
    "SubGraphPath": "Blueprints/AI/patrol_behavior.ats",  // External path
    "SubGraphUUID": "<uuid>"  // OR inline reference
  },
  "Position": {"x": 400, "y": 300}
}
```

### Execution Rules

| Feature | Implementation |
|---------|---------------|
| **Recursion Limit** | `MAX_SUBGRAPH_DEPTH = 4` (prevents stack overflow) |
| **Cycle Detection** | DFS via `SubGraphCallStack::Contains()` |
| **Input Passing** | Parent `DataPinCache` → Child `LocalBlackboard["input:*"]` |
| **Output Passing** | Child `LocalBlackboard["output:*"]` → Parent `DataPinCache` |
| **Exec Flow** | `Success` / `Failure` output pins based on child result |

### Tab-Based Navigation

**NodeGraphPanel** now supports multi-tab editing:

```cpp
struct GraphTab {
    std::string tabID;        // "root" or UUID
    std::string displayName;  // "Root" or subgraph name
    std::string graphPath;    // "root" or "subgraphs/<uuid>"
    bool isDirty;
};
```

| Action | Result |
|--------|--------|
| Double-click SubGraph node | Opens subgraph in new tab |
| Click "+ New SubGraph" | Creates empty subgraph tab |
| Click × on tab | Closes tab (root tab cannot be closed) |

### Validation

`SubgraphMigrator::ValidateSubgraphReferences()` checks:

1. **UUID Existence**: Every `subgraphUUID` must exist in `data.subgraphs`
2. **Circular Dependencies**: DFS to detect cycles (A → B → A)
3. **Orphaned Subgraphs**: Warns about unused subgraphs

**Example Error**:
```
ERROR: SubGraph node 5 references UUID "<uuid-abc>" which does not exist in data.subgraphs
ERROR: Circular SubGraph dependency detected: guard.ats → patrol.ats → guard.ats
```

### Code Example

```cpp
// Create SubGraph node in VisualScriptEditorPanel
void OnAddSubGraphNode()
{
    auto cmd = std::make_unique<CreateNodeCommand>(
        TaskNodeType::SubGraph,
        ImVec2(400, 300),
        "Call Patrol"
    );
    
    m_CommandStack->Execute(std::move(cmd));
    
    // Set properties
    auto* node = FindNode(cmd->GetNodeID());
    node->properties["SubGraphPath"] = "Blueprints/AI/patrol_behavior.ats";
}

// Runtime execution in VSGraphExecutor
int32_t VSGraphExecutor::HandleSubGraph(
    EntityID entity,
    int32_t nodeID,
    TaskRunnerComponent& runner,
    const TaskGraphTemplate& tmpl,
    LocalBlackboard& localBB,
    World* worldPtr,
    float dt,
    SubGraphCallStack& callStack)
{
    auto* node = tmpl.FindNode(nodeID);
    std::string path = node->GetProperty("SubGraphPath");
    
    // Cycle detection
    if (callStack.Contains(path))
    {
        LOG_ERROR("Circular SubGraph dependency: %s", path.c_str());
        return FindExecTarget(nodeID, "Failure", tmpl);
    }
    
    // Depth limit
    if (callStack.Depth >= MAX_SUBGRAPH_DEPTH)
    {
        LOG_ERROR("Max SubGraph depth exceeded: %d", callStack.Depth);
        return FindExecTarget(nodeID, "Failure", tmpl);
    }
    
    // Load child graph
    std::vector<std::string> errors;
    TaskGraphTemplate childTmpl = TaskGraphLoader::LoadFromFile(path, errors);
    
    if (!errors.empty())
        return FindExecTarget(nodeID, "Failure", tmpl);
    
    // Copy input data
    LocalBlackboard childBB = localBB; // Inherit parent scope
    // ... (copy DataPinCache values to childBB["input:*"])
    
    // Push call stack
    callStack.Push(path);
    
    // Recursive execution
    TaskRunnerComponent childRunner;
    childRunner.CurrentNodeID = 0; // Start at EntryPoint
    VSGraphExecutor::ExecuteFrame(entity, childRunner, childTmpl, childBB, worldPtr, dt);
    
    // Pop call stack
    callStack.Pop();
    
    // Copy output data
    // ... (copy childBB["output:*"] to parent DataPinCache)
    
    // Follow exec output
    if (childRunner.State == TaskState::Success)
        return FindExecTarget(nodeID, "Success", tmpl);
    else
        return FindExecTarget(nodeID, "Failure", tmpl);
}
```

### Files Modified

| File | Changes |
|------|---------|
| `Source/BlueprintEditor/SubgraphMigrator.h/cpp` | ✅ **NEW** — Migration and validation logic |
| `Source/BlueprintEditor/NodeGraphPanel.h/cpp` | Multi-tab support, `OpenSubgraphTab()` |
| `Source/TaskSystem/VSGraphExecutor.h/cpp` | `HandleSubGraph()`, `SubGraphCallStack` |
| `Documentation/ATS_VS_Phase8_Subgraphs.md` | Full technical spec |

---

## Performance Profiler (Phase 5)

### Architecture

The **PerformanceProfiler** is a singleton that collects per-node execution metrics across frames. It is **opt-in** (must call `BeginProfiling()`) and thread-safe via `std::mutex`.

### Data Structures

```cpp
struct NodeExecutionMetrics {
    int         nodeID;
    std::string nodeName;
    float       executionTimeMs;   // Last execution time
    uint64_t    executionCount;    // Total executions
    float       avgTimeMs;         // Running average
    float       maxTimeMs;         // Peak execution time
    float       totalTimeMs;       // Accumulated time
};

struct FrameProfile {
    uint64_t frameNumber;
    float    totalFrameTimeMs;
    std::vector<NodeExecutionMetrics> nodeMetrics;
};
```

### Usage Pattern

```cpp
// 1. Enable profiling (from editor menu)
PerformanceProfiler::Get().BeginProfiling();

// 2. Frame lifecycle (from TaskSystem)
PerformanceProfiler::Get().BeginFrame();

// 3. Node instrumentation (from VSGraphExecutor)
PerformanceProfiler::Get().BeginNodeExecution(node->NodeID, node->NodeName);
// ... execute node logic ...
PerformanceProfiler::Get().EndNodeExecution(node->NodeID);

// 4. End frame (commits data to history)
PerformanceProfiler::Get().EndFrame();

// 5. Export data
PerformanceProfiler::Get().SaveToFile("profiling_results.csv");
```

### ProfilerPanel UI

**Location**: `View → Profiler`

**Features**:
- **Frame Timeline**: Bar chart showing last 60 frames (total execution time)
- **Hotspot Table**: Sorted by average execution time (descending)
  - Columns: Node Name, Count, Avg (ms), Max (ms), Total (ms)
- **Export CSV**: Dumps all metrics to CSV file

**Example Output**:
```
Frame Timeline:
████████████████████████████████████████  Frame 1: 2.34ms
█████████████████████████████████         Frame 2: 1.89ms
█████████████████████████████████████████ Frame 3: 2.45ms

Hotspot Table:
┌─────────────────┬───────┬──────────┬──────────┬───────────┐
│ Node Name       │ Count │ Avg (ms) │ Max (ms) │ Total (ms)│
├─────────────────┼───────┼──────────┼──────────┼───────────┤
│ PathfindToTarget│  120  │  0.45    │  1.23    │   54.0    │
│ UpdateVision    │  240  │  0.12    │  0.34    │   28.8    │
│ Branch_CheckHP  │  480  │  0.02    │  0.05    │    9.6    │
└─────────────────┴───────┴──────────┴──────────┴───────────┘
```

### Performance Tips

| Node Type | Typical Time | Optimization |
|-----------|--------------|--------------|
| AtomicTask (Pathfinding) | 0.5-2.0ms | Cache paths, use lazy updates |
| AtomicTask (Physics Raycast) | 0.1-0.5ms | Reduce raycast frequency |
| Branch | <0.01ms | Negligible (cache condition values if expensive) |
| GetBBValue/SetBBValue | <0.01ms | Negligible |
| SubGraph | Varies | Avoid deep nesting (MAX_DEPTH=4) |

---

## Debug System (Phase 5)

### DebugController

**Singleton** managing debug state and breakpoints.

```cpp
enum class DebugState {
    NotDebugging,  // No debug session active
    Running,       // Executing with breakpoints enabled
    Paused         // Hit breakpoint, execution suspended
};

class DebugController {
public:
    static DebugController& Get();
    
    // Lifecycle
    void StartDebugging(EntityID entity);
    void StopDebugging();
    
    // State
    DebugState GetState() const;
    EntityID GetDebuggedEntity() const;
    
    // Breakpoints
    void ToggleBreakpoint(int32_t nodeID);
    bool HasBreakpoint(int32_t nodeID) const;
    void ClearAllBreakpoints();
    
    // Step controls
    void Continue();       // F5: Resume execution
    void StepOver();       // F10: Execute current node, pause at next
    void StepInto();       // F11: If SubGraph, enter and pause at first node
    void StepOut();        // Shift+F11: Exit current SubGraph
    
    // Callbacks (called by VSGraphExecutor)
    void OnBreakpointHit(EntityID entity, int32_t nodeID);
    void OnNodeEnter(int32_t nodeID);
    void OnNodeExit(int32_t nodeID);
};
```

### Breakpoint Workflow

```cpp
// User: F9 on node in VisualScriptEditorPanel
void VisualScriptEditorPanel::OnKeyPressed(ImGuiKey key)
{
    if (key == ImGuiKey_F9 && m_SelectedNodeID != -1)
    {
        DebugController::Get().ToggleBreakpoint(m_SelectedNodeID);
        
        // Visual feedback: Red circle on node
        auto* node = FindNode(m_SelectedNodeID);
        node->hasBreakpoint = !node->hasBreakpoint;
    }
}

// Runtime: Breakpoint check in VSGraphExecutor
void VSGraphExecutor::ExecuteFrame(...)
{
    auto& dc = DebugController::Get();
    
    // Check breakpoint before node execution
    if (dc.HasBreakpoint(runner.CurrentNodeID) && dc.GetState() == DebugState::Running)
    {
        dc.OnBreakpointHit(entity, runner.CurrentNodeID);
        // Execution pauses here until Continue/StepOver/StepInto
        return; // Exit frame early
    }
    
    // Execute node...
}
```

### DebugPanel UI

**Location**: `View → Debugger`

**Sections**:

1. **Toolbar**
   - ▶ Continue (F5)
   - ⏸ Pause
   - ⏭ Step Over (F10)
   - ⏬ Step Into (F11)
   - ⏫ Step Out (Shift+F11)
   - ⏹ Stop

2. **Breakpoints List**
   ```
   ✓ Node 5: Branch_CheckEnemy (guard_ai.ats:25)
   ✓ Node 12: AtomicTask_Attack (combat.ats:42)
   ```

3. **Call Stack**
   ```
   #0  AtomicTask_Pathfind (guard_ai.ats:30)
   #1  SubGraph_Patrol (patrol.ats:15)
   #2  EntryPoint (guard_ai.ats:5)
   ```

4. **Watch Variables** (Live Blackboard)
   ```
   local:EnemyHealth = 75.0f
   local:PatrolIndex = 2
   global:GameTime = 123.45f
   ```

5. **Profiler Summary** (if profiling enabled)
   ```
   Current Node: AtomicTask_Pathfind
   Execution Time: 0.45ms
   Frame Time: 2.34ms
   ```

### Keyboard Shortcuts

| Key | Action | Behavior |
|-----|--------|----------|
| **F5** | Continue | Resume execution until next breakpoint |
| **F9** | Toggle Breakpoint | Add/remove breakpoint on selected node |
| **F10** | Step Over | Execute current node, pause at next node in sequence |
| **F11** | Step Into | If SubGraph, enter child graph and pause at EntryPoint |
| **Shift+F11** | Step Out | Exit current SubGraph, pause in parent after return |

---

## Blackboard System (Phase 2.1)

### BlackboardPanel

**Location**: `View → Blackboard` (docked right of VisualScriptEditorPanel)

**Features**:
- **CRUD Operations**: Create, Read, Update, Delete variables
- **Type Support**: Int, Float, Bool, String, Vector3
- **Default Values**: Set initial values in template
- **Live Editing**: Modify runtime values during debug

### Supported Types

```cpp
enum class BlackboardValueType {
    Int,
    Float,
    Bool,
    String,
    Vector3
};

struct BlackboardEntry {
    std::string key;
    BlackboardValueType type;
    TaskValue defaultValue;
};
```

### Variable Scoping

| Scope | Syntax | Storage | Lifetime |
|-------|--------|---------|----------|
| **Local** | `local:EnemyHealth` | `LocalBlackboard` (per-entity) | Duration of entity |
| **Global** | `global:GameTime` | `World::GlobalBlackboard` | Duration of world session |
| **Input** | `input:targetPos` | SubGraph input parameters | Duration of SubGraph call |
| **Output** | `output:success` | SubGraph return values | Duration of SubGraph call |

### Usage in Graphs

**GetBBValue Node**:
```json
{
  "NodeID": 10,
  "NodeType": "GetBBValue",
  "Properties": {
    "BBKey": "local:EnemyHealth"
  },
  "OutputPins": [
    {"Name": "Value", "Type": "Float"}
  ]
}
```

**SetBBValue Node**:
```json
{
  "NodeID": 11,
  "NodeType": "SetBBValue",
  "Properties": {
    "BBKey": "local:IsAggro"
  },
  "InputPins": [
    {"Name": "Value", "Type": "Bool"}
  ]
}
```

### Code Example

```cpp
// Create variable in BlackboardPanel
void BlackboardPanel::OnAddVariable()
{
    if (ImGui::Button("Add Variable"))
    {
        BlackboardEntry entry;
        entry.key = m_newEntryName;  // "EnemyHealth"
        entry.type = BlackboardValueType::Float;
        entry.defaultValue = TaskValue(0.0f);
        
        m_Blackboard->AddEntry(entry);
    }
}

// Runtime read in VSGraphExecutor
TaskValue VSGraphExecutor::ReadBBValue(const std::string& scopedKey, LocalBlackboard& localBB)
{
    // Parse "local:EnemyHealth" → scope="local", key="EnemyHealth"
    auto [scope, key] = ParseScope(scopedKey);
    
    if (scope == "local")
        return localBB.GetValue(key);
    else if (scope == "global")
        return World::Get().GetGlobalBlackboard()->GetValue(key);
    else
        return TaskValue(); // Invalid scope
}
```

---

## Template Manager (Phase 5)

### BlueprintTemplate Structure

```cpp
struct BlueprintTemplate {
    std::string id;              // UUID
    std::string name;            // "Guard AI Base"
    std::string description;     // "Basic patrol and combat logic"
    std::string category;        // "AI", "Character", "Enemy"
    std::string author;          // Creator name
    std::string version;         // "1.0"
    json blueprintData;          // Complete graph JSON
    std::string thumbnailPath;   // "Templates/guard_ai_thumb.png"
    time_t createdDate;
    time_t modifiedDate;
};
```

### Template Catalog

**Storage Location**: `Blueprints/Templates/`

**File Format**: `.template` (JSON)

```json
{
  "id": "<uuid>",
  "name": "Guard AI Base",
  "description": "Basic patrol and combat logic",
  "category": "AI",
  "author": "John Doe",
  "version": "1.0",
  "createdDate": 1678900000,
  "modifiedDate": 1678900000,
  "thumbnailPath": "Templates/guard_ai_thumb.png",
  "blueprintData": {
    "schema_version": 4,
    "graphType": "VisualScript",
    "nodes": [...],
    "ExecConnections": [...]
  }
}
```

### TemplateManager API

```cpp
class TemplateManager {
public:
    static TemplateManager& Get();
    
    // Catalog management
    void Initialize(const std::string& templatesPath = "Blueprints/Templates");
    bool LoadTemplates(const std::string& templatesPath);
    bool RefreshTemplates();  // Rescan directory
    
    // CRUD operations
    bool SaveTemplate(const BlueprintTemplate& tpl);
    bool DeleteTemplate(const std::string& templateId);
    const BlueprintTemplate* FindTemplate(const std::string& id) const;
    
    // Queries
    const std::vector<BlueprintTemplate>& GetAllTemplates() const;
    std::vector<BlueprintTemplate> GetTemplatesByCategory(const std::string& category) const;
    std::vector<std::string> GetAllCategories() const;
    
    // Template application
    bool ApplyTemplateToBlueprint(const std::string& templateId, json& targetBlueprint);
    
    // Template creation
    BlueprintTemplate CreateTemplateFromBlueprint(
        const json& blueprint,
        const std::string& name,
        const std::string& description,
        const std::string& category
    );
};
```

### Usage Workflow

**1. Save Current Graph as Template**

```cpp
// VisualScriptEditorPanel menu: "File → Save as Template"
void VisualScriptEditorPanel::OnSaveAsTemplate()
{
    // Show dialog
    ImGui::OpenPopup("Save as Template");
    
    if (ImGui::BeginPopupModal("Save as Template"))
    {
        ImGui::InputText("Name", m_templateName, 256);
        ImGui::InputTextMultiline("Description", m_templateDesc, 1024);
        ImGui::InputText("Category", m_templateCategory, 128);
        
        if (ImGui::Button("Save"))
        {
            json blueprintData = m_CurrentTemplate.ToJson();
            
            BlueprintTemplate tpl = TemplateManager::Get().CreateTemplateFromBlueprint(
                blueprintData,
                m_templateName,
                m_templateDesc,
                m_templateCategory
            );
            
            TemplateManager::Get().SaveTemplate(tpl);
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}
```

**2. Apply Template to Current Graph**

```cpp
// VisualScriptEditorPanel menu: "File → Load Template"
void VisualScriptEditorPanel::OnLoadTemplate(const std::string& templateId)
{
    json currentBlueprint = m_CurrentTemplate.ToJson();
    
    if (TemplateManager::Get().ApplyTemplateToBlueprint(templateId, currentBlueprint))
    {
        // Reload graph from modified JSON
        TaskGraphTemplate newTemplate = TaskGraphLoader::ParseSchemaV4(currentBlueprint, errors);
        LoadTemplate(newTemplate, m_CurrentFilepath);
        
        MarkAsModified();
    }
}
```

**3. Browse Template Catalog**

```cpp
// TemplateLibraryPanel UI
void TemplateLibraryPanel::Render()
{
    auto& tm = TemplateManager::Get();
    auto categories = tm.GetAllCategories();
    
    for (const auto& category : categories)
    {
        if (ImGui::CollapsingHeader(category.c_str()))
        {
            auto templates = tm.GetTemplatesByCategory(category);
            
            for (const auto& tpl : templates)
            {
                ImGui::Selectable(tpl.name.c_str());
                
                if (ImGui::IsItemClicked())
                {
                    m_SelectedTemplateID = tpl.id;
                }
                
                // Preview thumbnail
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", tpl.description.c_str());
                    ImGui::Text("Author: %s", tpl.author.c_str());
                    ImGui::Text("Version: %s", tpl.version.c_str());
                    // ImGui::Image(tpl.thumbnail);
                    ImGui::EndTooltip();
                }
            }
        }
    }
}
```

---

## Validation System

### BlueprintValidator

**Purpose**: Static analysis of graph structure to detect errors before runtime.

### Validation Rules

| Rule | Severity | Description |
|------|----------|-------------|
| **Unreachable Nodes** | Warning | Nodes with no incoming exec connections (except EntryPoint) |
| **Missing EntryPoint** | Critical | Graph has no EntryPoint node |
| **Dangling Connections** | Error | ExecConnection/DataConnection references non-existent node |
| **Type Mismatch** | Error | DataConnection connects incompatible types (Float → Bool) |
| **Circular ExecFlow** | Warning | Exec connections form a cycle (infinite loop risk) |
| **Circular SubGraphs** | Critical | SubGraph references form a cycle (runtime crash) |
| **Missing Required Properties** | Error | Node missing required property (e.g., Branch without ConditionKey) |
| **Invalid Blackboard Reference** | Error | GetBBValue/SetBBValue references undefined BB variable |

### ValidationError Structure

```cpp
struct ValidationError {
    ValidationSeverity severity;  // Critical, Error, Warning, Info
    int32_t nodeID;               // Affected node (-1 if graph-level)
    std::string message;          // Human-readable description
    std::string fixSuggestion;    // Optional fix hint
};

enum class ValidationSeverity {
    Critical,  // Blocks execution (missing EntryPoint, circular SubGraphs)
    Error,     // Will cause runtime failures (type mismatch, dangling connections)
    Warning,   // Potential issues (unreachable nodes, infinite loops)
    Info       // Suggestions (unused variables, optimization hints)
};
```

### ValidationPanel UI

**Location**: `View → Validation` (docked bottom of editor)

**Real-time Validation**: Auto-validates every 1 second when graph is modified.

**Example Output**:
```
╔═══════════════════════════════════════════════════════════════════════╗
║ Validation Results                                             [Auto] ║
╠═══════════════════════════════════════════════════════════════════════╣
║ 🔴 CRITICAL (1)                                                       ║
║   • Missing EntryPoint node                                           ║
║     Fix: Add an EntryPoint node from Flow Control palette             ║
╠═══════════════════════════════════════════════════════════════════════╣
║ ⚠️  ERROR (2)                                                         ║
║   • Node 5 (Branch): Missing required property 'ConditionKey'         ║
║     Fix: Set ConditionKey in Properties panel                         ║
║   • Node 12: DataConnection type mismatch (Float → Bool)              ║
║     Fix: Insert type conversion node or change pin type               ║
╠═══════════════════════════════════════════════════════════════════════╣
║ ⚠️  WARNING (1)                                                       ║
║   • Node 8 (AtomicTask): Unreachable (no incoming exec connections)   ║
║     Fix: Connect an exec pin to this node or delete it                ║
╚═══════════════════════════════════════════════════════════════════════╝

[Click error to jump to node]
```

### Code Example

```cpp
// Validate current graph
void ValidationPanel::ValidateActiveGraph()
{
    m_Errors.clear();
    
    auto& editor = BlueprintEditor::Get();
    const auto& tmpl = editor.GetCurrentBlueprint();
    
    // Run validator
    m_Errors = m_Validator.Validate(tmpl);
    
    // Update UI
    if (HasCriticalErrors())
    {
        LOG_ERROR("Graph validation failed with %d critical errors", GetCriticalErrorCount());
    }
}

// Click handler to focus node
void ValidationPanel::OnErrorClicked(const ValidationError& error)
{
    if (error.nodeID != -1)
    {
        // Focus VisualScriptEditorPanel on error node
        auto& editor = BlueprintEditor::Get();
        editor.FocusNode(error.nodeID);
        
        // Highlight node in red
        auto* panel = editor.GetVSEditorPanel();
        panel->SetNodeHighlight(error.nodeID, ImColor(255, 0, 0));
    }
}
```

---

## Command System (Undo/Redo)

### CommandStack Architecture

```cpp
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual std::string GetDescription() const = 0;
};

class CommandStack {
public:
    void Execute(std::unique_ptr<ICommand> cmd);
    void Undo();
    void Redo();
    void Clear();
    
    bool CanUndo() const { return m_CurrentIndex > 0; }
    bool CanRedo() const { return m_CurrentIndex < m_Commands.size(); }
    
    std::string GetLastCommandDescription() const;
    std::string GetNextRedoDescription() const;

private:
    std::vector<std::unique_ptr<ICommand>> m_Commands;
    size_t m_CurrentIndex = 0;  // Points to next command to redo
};
```

### Command Types

**1. CreateNodeCommand**

```cpp
class CreateNodeCommand : public ICommand {
public:
    CreateNodeCommand(TaskNodeType type, ImVec2 pos, const std::string& name);
    
    void Execute() override {
        m_NodeID = m_Editor->AddNode(m_NodeType, m_Position, m_NodeName);
    }
    
    void Undo() override {
        m_Editor->RemoveNode(m_NodeID);
    }
    
    std::string GetDescription() const override {
        return "Create " + m_NodeName;
    }

private:
    VisualScriptEditorPanel* m_Editor;
    TaskNodeType m_NodeType;
    ImVec2 m_Position;
    std::string m_NodeName;
    int32_t m_NodeID;  // Populated after Execute()
};
```

**2. DeleteNodeCommand**

```cpp
class DeleteNodeCommand : public ICommand {
public:
    DeleteNodeCommand(int32_t nodeID);
    
    void Execute() override {
        // Backup node data before deletion
        m_NodeBackup = m_Editor->GetNode(m_NodeID);
        m_ConnectionsBackup = m_Editor->GetConnectionsForNode(m_NodeID);
        
        m_Editor->RemoveNode(m_NodeID);
    }
    
    void Undo() override {
        // Restore node and connections
        m_Editor->RestoreNode(m_NodeBackup);
        m_Editor->RestoreConnections(m_ConnectionsBackup);
    }
    
    std::string GetDescription() const override {
        return "Delete " + m_NodeBackup.NodeName;
    }

private:
    VisualScriptEditorPanel* m_Editor;
    int32_t m_NodeID;
    TaskNodeDefinition m_NodeBackup;
    std::vector<ExecPinConnection> m_ConnectionsBackup;
};
```

**3. CreateLinkCommand**

```cpp
class CreateLinkCommand : public ICommand {
public:
    CreateLinkCommand(int32_t fromNode, const std::string& fromPin,
                      int32_t toNode, const std::string& toPin);
    
    void Execute() override {
        m_Editor->ConnectExec(m_FromNode, m_FromPin, m_ToNode, m_ToPin);
    }
    
    void Undo() override {
        m_Editor->DisconnectExec(m_FromNode, m_FromPin, m_ToNode, m_ToPin);
    }
    
    std::string GetDescription() const override {
        return "Connect " + m_FromPin + " → " + m_ToPin;
    }

private:
    VisualScriptEditorPanel* m_Editor;
    int32_t m_FromNode, m_ToNode;
    std::string m_FromPin, m_ToPin;
};
```

### Command Batching

**Composite Command** for multi-operation transactions:

```cpp
class CompositeCommand : public ICommand {
public:
    void AddCommand(std::unique_ptr<ICommand> cmd) {
        m_Commands.push_back(std::move(cmd));
    }
    
    void Execute() override {
        for (auto& cmd : m_Commands)
            cmd->Execute();
    }
    
    void Undo() override {
        // Reverse order for undo
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it)
            (*it)->Undo();
    }
    
    std::string GetDescription() const override {
        return "Batch Operation (" + std::to_string(m_Commands.size()) + " commands)";
    }

private:
    std::vector<std::unique_ptr<ICommand>> m_Commands;
};
```

**Example: Delete multiple nodes**

```cpp
void VisualScriptEditorPanel::DeleteSelectedNodes()
{
    auto composite = std::make_unique<CompositeCommand>();
    
    for (int32_t nodeID : m_SelectedNodes)
    {
        auto cmd = std::make_unique<DeleteNodeCommand>(nodeID);
        composite->AddCommand(std::move(cmd));
    }
    
    m_CommandStack->Execute(std::move(composite));
}
```

### History Panel

**Location**: `View → History`

**Features**:
- List of all commands (undo/redo)
- Current position indicator (▶)
- Click to revert to specific state

**Example UI**:
```
╔═══════════════════════════════════════════════════════════════╗
║ Command History                                               ║
╠═══════════════════════════════════════════════════════════════╣
║   Create EntryPoint                                           ║
║   Create Branch_CheckEnemy                                    ║
║   Connect Out → In                                            ║
║ ▶ Create AtomicTask_Attack                                    ║
║   [Redo] Delete Branch_CheckEnemy                             ║
╚═══════════════════════════════════════════════════════════════╝

[Ctrl+Z: Undo] [Ctrl+Y: Redo] [Clear History]
```

---

## Summary

This document covers all advanced systems in Olympe Blueprint Editor v4:

1. **Visual Pipelines** — Complete flow from creation → execution → debugging
2. **SubGraphs** — Modular, reusable graph composition with cycle detection
3. **Profiler** — Per-node execution metrics with hotspot analysis
4. **Debugger** — Breakpoints, step controls, call stack inspection
5. **Blackboard** — Variable scoping (local/global/input/output)
6. **Templates** — Reusable blueprint catalog with categorization
7. **Validation** — Static analysis with real-time error detection
8. **Command System** — Full undo/redo with composite operations

All systems are **fully integrated** and follow the **v4 ATS Visual Script schema**.

---

**Related Documentation**:
- [Blueprint Editor User Guide v4](Blueprint_Editor_User_Guide_v4.md) — Basic workflows
- [ATS Visual Script Complete Documentation](../Documentation/Olympe_ATS_VisualScript_Complete_Doc.md) — Node reference
- [ATS VS Phase 8 Subgraphs](../Documentation/ATS_VS_Phase8_Subgraphs.md) — Technical spec
