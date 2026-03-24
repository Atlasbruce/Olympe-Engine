# Olympe Blueprint Editor — Interactive Visual Diagrams

> **Version**: 2.0 (Phase 5-8)  
> **Format**: Mermaid Diagrams (render in GitHub, VS Code, or Markdown viewers)  
> **Status**: ✅ Current — Reflects production v4 system

---

## Table of Contents

1. [Complete Editor Architecture](#complete-editor-architecture)
2. [Asset Loading Pipeline](#asset-loading-pipeline)
3. [Graph Creation Workflow](#graph-creation-workflow)
4. [Runtime Execution Flow](#runtime-execution-flow)
5. [SubGraph Call Stack](#subgraph-call-stack)
6. [Data Flow Architecture](#data-flow-architecture)
7. [Debug System State Machine](#debug-system-state-machine)
8. [Command Stack Operations](#command-stack-operations)
9. [Validation Pipeline](#validation-pipeline)
10. [Profiler Data Flow](#profiler-data-flow)

---

## Complete Editor Architecture

### Class Hierarchy

```mermaid
classDiagram
    class BlueprintEditor {
        <<Singleton Backend>>
        -TaskGraphTemplate m_CurrentBlueprint
        -string m_CurrentFilepath
        -vector~AssetNode~ m_AssetTree
        -CommandStack* m_CommandStack
        +Initialize()
        +OpenGraphInEditor(path)
        +RefreshAssets()
        +PreloadATSGraphs()
    }

    class BlueprintEditorGUI {
        <<Frontend UI>>
        -VisualScriptEditorPanel* m_VSEditorPanel
        -NodeGraphPanel* m_NodeGraphPanel
        -AssetBrowser* m_AssetBrowser
        -DebugPanel* m_DebugPanel
        -ProfilerPanel* m_ProfilerPanel
        +Render()
        +RenderMenuBar()
    }

    class VisualScriptEditorPanel {
        <<Current v4 Editor>>
        -TaskGraphTemplate m_CurrentTemplate
        -vector~VSEditorNode~ m_Nodes
        -CommandStack* m_CommandStack
        +AddNode(type, pos)
        +ConnectExec(from, to)
        +Save()
        +RenderCanvas()
    }

    class NodeGraphPanel {
        <<DEPRECATED v2>>
        -GraphDocument m_Document
        -vector~GraphTab~ m_Tabs
        +RenderCanvas()
        +OpenSubgraphTab()
    }

    class TaskGraphLoader {
        <<Universal Loader>>
        +LoadFromFile(path)
        +ParseSchemaV4(json)
        +MigrateV3toV4(json)
        +MigrateV2toV4(json)
    }

    class VSGraphExecutor {
        <<Runtime Engine>>
        +ExecuteFrame(entity, runner, tmpl, BB, world, dt)
        +HandleBranch()
        +HandleAtomicTask()
        +HandleSubGraph()
    }

    class PerformanceProfiler {
        <<Singleton Profiler>>
        -vector~FrameProfile~ m_FrameHistory
        -map~NodeExecutionMetrics~ m_Metrics
        +BeginNodeExecution(id)
        +EndNodeExecution(id)
        +SaveToFile(path)
    }

    class DebugController {
        <<Singleton Debugger>>
        -DebugState m_State
        -set~int~ m_Breakpoints
        +ToggleBreakpoint(nodeID)
        +StepOver()
        +Continue()
    }

    BlueprintEditor --> BlueprintEditorGUI : "Manages"
    BlueprintEditorGUI --> VisualScriptEditorPanel : "Contains"
    BlueprintEditorGUI --> NodeGraphPanel : "Contains (legacy)"
    BlueprintEditor --> TaskGraphLoader : "Uses"
    VisualScriptEditorPanel --> TaskGraphLoader : "Loads via"
    VSGraphExecutor --> PerformanceProfiler : "Instruments"
    VSGraphExecutor --> DebugController : "Checks breakpoints"
```

---

## Asset Loading Pipeline

```mermaid
flowchart TD
    A[User: Double-click guard_ai.ats] --> B[AssetBrowser::OnDoubleClick]
    B --> C[BlueprintEditor::OpenGraphInEditor]
    C --> D{DetectAssetType}
    
    D -->|graphType: VisualScript| E[VisualScriptEditorPanel::Load]
    D -->|graphType: BehaviorTree| F[NodeGraphPanel::Load - DEPRECATED]
    D -->|graphType: HFSM| G[HFSMEditorPanel::Load]
    
    E --> H[TaskGraphLoader::LoadFromFile]
    
    H --> I{Check schema_version}
    I -->|v4| J[ParseSchemaV4 - Primary Path]
    I -->|v3| K[TaskGraphMigrator_v3_to_v4::MigrateJson]
    I -->|v2| L[BTtoVSMigrator::Convert]
    
    J --> M[Return TaskGraphTemplate]
    K --> M
    L --> M
    
    M --> N[VisualScriptEditorPanel::LoadTemplate]
    N --> O[Create VSEditorNode for each node]
    O --> P[Create ImNodes links]
    P --> Q[Initialize Blackboard panel]
    Q --> R[Display in canvas]
    
    style J fill:#90EE90
    style K fill:#FFD700
    style L fill:#FFA500
    style F fill:#FF6B6B
```

---

## Graph Creation Workflow

```mermaid
sequenceDiagram
    participant User
    participant Menu as View Menu
    participant Panel as VisualScriptEditorPanel
    participant CS as CommandStack
    participant Template as TaskGraphTemplate

    User->>Menu: View → VS Graph Editor → New Graph
    Menu->>Panel: NewGraph()
    Panel->>Template: Create empty template
    Template-->>Panel: schema_version=4, graphType=VisualScript
    Panel->>CS: Clear()
    CS-->>Panel: History reset
    Panel-->>User: Show empty canvas

    User->>Panel: Right-click → Flow Control → Branch
    Panel->>CS: Execute(CreateNodeCommand)
    CS->>Template: AddNode(Branch, x, y)
    Template-->>CS: NodeID = 1
    CS->>Panel: Update canvas
    Panel-->>User: Node appears

    User->>Panel: Drag from "Out" pin to "In" pin
    Panel->>Panel: ImNodes::IsLinkCreated()
    Panel->>CS: Execute(CreateLinkCommand)
    CS->>Template: AddExecConnection(from, to)
    Template-->>CS: Success
    CS->>Panel: Update canvas
    Panel-->>User: Link appears

    User->>Panel: Ctrl+Z
    Panel->>CS: Undo()
    CS->>Template: RemoveExecConnection(from, to)
    Template-->>CS: Success
    CS->>Panel: Refresh canvas
    Panel-->>User: Link disappears
```

---

## Runtime Execution Flow

```mermaid
stateDiagram-v2
    [*] --> EntryPoint: Frame Start

    EntryPoint --> Branch: Follow "Out" exec pin
    Branch --> ResolveDataPins: Before node execution
    ResolveDataPins --> EvaluateCondition: Read DataPinCache
    
    EvaluateCondition --> TruePath: Condition = true
    EvaluateCondition --> FalsePath: Condition = false
    
    TruePath --> AtomicTask: Follow "True" exec pin
    FalsePath --> SetBBValue: Follow "False" exec pin
    
    AtomicTask --> CheckTaskState: IAtomicTask::Execute()
    CheckTaskState --> Running: Task not complete
    CheckTaskState --> Success: Task finished
    
    Running --> [*]: Keep CurrentNodeID, wait next frame
    Success --> GetBBValue: Follow "Completed" exec pin
    
    GetBBValue --> ReadBlackboard: Resolve "BBKey"
    ReadBlackboard --> StoreInCache: DataPinCache[output] = value
    StoreInCache --> SubGraph: Follow "Out" exec pin
    
    SubGraph --> CheckCycle: SubGraphCallStack::Contains?
    CheckCycle --> Error: Cycle detected
    CheckCycle --> LoadChild: No cycle
    
    LoadChild --> RecursiveExecute: VSGraphExecutor::ExecuteFrame(child)
    RecursiveExecute --> CopyOutput: Child completed
    CopyOutput --> FollowExecOutput: Success/Failure pin
    
    FollowExecOutput --> [*]: Frame End
    SetBBValue --> [*]: Frame End
    Error --> [*]: Frame End (error state)
```

---

## SubGraph Call Stack

```mermaid
flowchart TB
    subgraph Frame1["Frame 1: Parent Graph (guard_ai.ats)"]
        A1[CurrentNodeID = 1<br>EntryPoint] --> A2[CurrentNodeID = 2<br>SubGraph: patrol.ats]
    end
    
    A2 --> B1{Cycle Detection}
    B1 -->|Contains guard_ai?| B2[❌ ERROR: Cycle]
    B1 -->|No| B3[✅ Continue]
    
    B3 --> C1{Depth Check}
    C1 -->|Depth >= 4?| C2[❌ ERROR: Max depth]
    C1 -->|Depth < 4| C3[✅ Continue]
    
    C3 --> D1[Push CallStack]
    D1 --> D2["PathStack = [guard_ai.ats, patrol.ats]<br>Depth = 1"]
    
    D2 --> E1[Load child graph]
    
    subgraph Frame2["Frame 2: Child Graph (patrol.ats)"]
        E1 --> E2[CurrentNodeID = 0<br>EntryPoint]
        E2 --> E3[CurrentNodeID = 1<br>Branch]
        E3 --> E4[CurrentNodeID = 2<br>AtomicTask: Pathfind]
    end
    
    E4 --> F1[Child completes]
    F1 --> F2[Pop CallStack]
    F2 --> F3["PathStack = [guard_ai.ats]<br>Depth = 0"]
    
    F3 --> G1[Copy output data]
    G1 --> G2[Follow Success/Failure exec pin]
    
    G2 --> H1[Return to parent]
    
    subgraph Frame3["Frame 3: Parent Graph (continued)"]
        H1 --> H2[CurrentNodeID = 3<br>Next node after SubGraph]
    end
    
    style B2 fill:#FF6B6B
    style C2 fill:#FF6B6B
    style D2 fill:#90EE90
    style F3 fill:#90EE90
```

---

## Data Flow Architecture

```mermaid
flowchart LR
    subgraph Nodes["Node Graph"]
        N1[GetBBValue<br>Key: EnemyHealth]
        N2[Branch<br>Condition input]
        N3[AtomicTask<br>Attack]
    end
    
    subgraph LocalBB["LocalBlackboard (Entity)"]
        LB1[EnemyHealth: 75.0f]
        LB2[IsAggro: true]
    end
    
    subgraph GlobalBB["GlobalBlackboard (World)"]
        GB1[GameTime: 123.45f]
        GB2[QuestState: Active]
    end
    
    subgraph DataCache["DataPinCache (Runtime)"]
        DC1["Node 1 outputs:<br>Value: 75.0f"]
        DC2["Node 2 inputs:<br>Condition: 75.0f"]
    end
    
    LB1 -->|Read| N1
    N1 -->|Store output| DC1
    DC1 -->|Resolve input| DC2
    DC2 -->|Evaluate| N2
    N2 -->|Follow True| N3
    
    N3 -->|Write| LB2
    GB1 -.->|Global read| N1
    N3 -.->|Global write| GB2
    
    style LB1 fill:#FFD700
    style GB1 fill:#87CEEB
    style DC1 fill:#90EE90
```

---

## Debug System State Machine

```mermaid
stateDiagram-v2
    [*] --> NotDebugging: Editor starts

    NotDebugging --> Running: User: Start Debugging (F5)
    
    Running --> Paused: Breakpoint hit
    Running --> NotDebugging: User: Stop Debugging
    
    Paused --> Running: User: Continue (F5)
    Paused --> StepOver: User: Step Over (F10)
    Paused --> StepInto: User: Step Into (F11)
    Paused --> StepOut: User: Step Out (Shift+F11)
    Paused --> NotDebugging: User: Stop Debugging
    
    StepOver --> ExecuteNode: Execute current node
    ExecuteNode --> Paused: Pause at next node
    
    StepInto --> CheckSubGraph: Is current node SubGraph?
    CheckSubGraph --> EnterChild: Yes, enter child graph
    CheckSubGraph --> ExecuteNode: No, treat as StepOver
    EnterChild --> Paused: Pause at child EntryPoint
    
    StepOut --> ExecuteUntilReturn: Execute until SubGraph returns
    ExecuteUntilReturn --> Paused: Pause in parent after return
    
    Running --> CheckBreakpoint: Before each node
    CheckBreakpoint --> Paused: Breakpoint enabled
    CheckBreakpoint --> Running: No breakpoint
    
    note right of Paused
        DebugPanel shows:
        - Call stack
        - Blackboard snapshot
        - Current node highlight
    end note
    
    note right of Running
        VSGraphExecutor continues
        normal frame execution
    end note
```

---

## Command Stack Operations

```mermaid
flowchart TD
    subgraph Initial["Initial State"]
        CS1["CommandStack<br>CurrentIndex = 0<br>Commands = []"]
    end
    
    Initial --> A1[User: Create Node]
    A1 --> A2["Execute(CreateNodeCommand)"]
    A2 --> A3["Commands = [Create Node]<br>CurrentIndex = 1"]
    
    A3 --> B1[User: Create Link]
    B1 --> B2["Execute(CreateLinkCommand)"]
    B2 --> B3["Commands = [Create Node, Create Link]<br>CurrentIndex = 2"]
    
    B3 --> C1[User: Ctrl+Z]
    C1 --> C2["Undo()"]
    C2 --> C3["Commands[1].Undo() - Remove link<br>CurrentIndex = 1"]
    
    C3 --> D1[User: Ctrl+Z]
    D1 --> D2["Undo()"]
    D2 --> D3["Commands[0].Undo() - Remove node<br>CurrentIndex = 0"]
    
    D3 --> E1[User: Ctrl+Y]
    E1 --> E2["Redo()"]
    E2 --> E3["Commands[0].Execute() - Restore node<br>CurrentIndex = 1"]
    
    E3 --> F1[User: Create New Node]
    F1 --> F2["Execute(CreateNodeCommand)"]
    F2 --> F3["Discard Commands[1]<br>Commands = [Create Node, Create New Node]<br>CurrentIndex = 2"]
    
    style A3 fill:#90EE90
    style B3 fill:#90EE90
    style C3 fill:#FFD700
    style D3 fill:#FFA500
    style E3 fill:#90EE90
    style F3 fill:#87CEEB
```

---

## Validation Pipeline

```mermaid
flowchart TD
    A[User: Modify Graph] --> B[Auto-validate timer<br>1 second delay]
    B --> C[ValidationPanel::ValidateActiveGraph]
    
    C --> D1[Check: Missing EntryPoint]
    C --> D2[Check: Unreachable Nodes]
    C --> D3[Check: Dangling Connections]
    C --> D4[Check: Type Mismatches]
    C --> D5[Check: Circular ExecFlow]
    C --> D6[Check: Circular SubGraphs]
    C --> D7[Check: Missing Properties]
    C --> D8[Check: Invalid BB References]
    
    D1 --> E{Has Critical Errors?}
    D2 --> E
    D3 --> E
    D4 --> E
    D5 --> E
    D6 --> E
    D7 --> E
    D8 --> E
    
    E -->|Yes| F1[🔴 Display Critical Errors]
    E -->|No| F2{Has Errors?}
    
    F2 -->|Yes| G1[⚠️ Display Errors]
    F2 -->|No| G2{Has Warnings?}
    
    G2 -->|Yes| H1[⚠️ Display Warnings]
    G2 -->|No| H2[✅ Validation Passed]
    
    F1 --> I[User clicks error]
    G1 --> I
    H1 --> I
    
    I --> J[ValidationPanel::OnErrorClicked]
    J --> K[VisualScriptEditorPanel::FocusNode]
    K --> L[Highlight node in red]
    
    style F1 fill:#FF6B6B
    style G1 fill:#FFA500
    style H1 fill:#FFD700
    style H2 fill:#90EE90
```

---

## Profiler Data Flow

```mermaid
sequenceDiagram
    participant User
    participant Menu as Profiler Menu
    participant PP as PerformanceProfiler
    participant Exec as VSGraphExecutor
    participant Panel as ProfilerPanel

    User->>Menu: View → Profiler → Begin Profiling
    Menu->>PP: BeginProfiling()
    PP-->>Menu: Profiling enabled

    loop Game Loop (60 FPS)
        Exec->>PP: BeginFrame()
        
        loop For each node
            Exec->>PP: BeginNodeExecution(nodeID, nodeName)
            Note over PP: Start high-res timer
            
            Exec->>Exec: Execute node logic
            
            Exec->>PP: EndNodeExecution(nodeID)
            Note over PP: Calculate elapsed time<br>Update metrics
        end
        
        Exec->>PP: EndFrame()
        Note over PP: Commit FrameProfile to history
    end

    User->>Menu: View → Profiler Panel
    Menu->>Panel: Render()
    Panel->>PP: GetFrameHistory()
    PP-->>Panel: Last 60 frames
    Panel->>Panel: RenderFrameTimeline()
    Panel->>Panel: RenderHotspotTable()
    Panel-->>User: Display metrics

    User->>Panel: Export CSV
    Panel->>PP: SaveToFile("results.csv")
    PP-->>Panel: Success
    Panel-->>User: File saved
```

---

## Blackboard Scoping Hierarchy

```mermaid
graph TB
    subgraph World["World (Singleton)"]
        GB[GlobalBlackboard<br>---<br>GameTime: 123.45f<br>QuestState: Active<br>PlayerHealth: 100]
    end
    
    subgraph Entity1["Entity[42] - Guard NPC"]
        LB1[LocalBlackboard<br>---<br>EnemyHealth: 75.0f<br>PatrolIndex: 2<br>IsAggro: true]
        
        subgraph Task1["TaskRunnerComponent"]
            TR1[CurrentNodeID: 5<br>State: Running]
            
            subgraph DataCache1["DataPinCache"]
                DC1[Node 3 outputs:<br>Value: 75.0f]
            end
        end
    end
    
    subgraph Entity2["Entity[99] - Player"]
        LB2[LocalBlackboard<br>---<br>AmmoCount: 30<br>CurrentWeapon: Rifle<br>IsReloading: false]
        
        subgraph Task2["TaskRunnerComponent"]
            TR2[CurrentNodeID: 2<br>State: Success]
            
            subgraph DataCache2["DataPinCache"]
                DC2[Node 1 outputs:<br>Value: 30]
            end
        end
    end
    
    World --> Entity1
    World --> Entity2
    
    Entity1 --> LB1
    Entity1 --> Task1
    Task1 --> TR1
    Task1 --> DataCache1
    
    Entity2 --> LB2
    Entity2 --> Task2
    Task2 --> TR2
    Task2 --> DataCache2
    
    LB1 -.->|Read: local:EnemyHealth| TR1
    GB -.->|Read: global:GameTime| TR1
    DC1 -.->|Propagate data| TR1
    
    LB2 -.->|Read: local:AmmoCount| TR2
    GB -.->|Write: global:QuestState| TR2
    
    style GB fill:#87CEEB
    style LB1 fill:#FFD700
    style LB2 fill:#FFD700
    style DC1 fill:#90EE90
    style DC2 fill:#90EE90
```

---

## Template System Workflow

```mermaid
flowchart TD
    A[User: File → Save as Template] --> B[TemplateDialog::Show]
    B --> C[Input: Name, Description, Category]
    C --> D[TemplateManager::CreateTemplateFromBlueprint]
    
    D --> E[Generate UUID]
    E --> F[Serialize current graph to JSON]
    F --> G[Create BlueprintTemplate object]
    G --> H[Save to Blueprints/Templates/uuid.template]
    
    H --> I[TemplateManager::RefreshTemplates]
    I --> J[TemplateLibraryPanel updates]
    
    J --> K[User: Browse Template Library]
    K --> L{Select template?}
    
    L -->|Yes| M[TemplateLibraryPanel::OnTemplateClick]
    L -->|No| K
    
    M --> N[Preview: Thumbnail, Description, Author]
    N --> O[User: Apply Template]
    
    O --> P[TemplateManager::ApplyTemplateToBlueprint]
    P --> Q[Merge template JSON with current graph]
    Q --> R[TaskGraphLoader::ParseSchemaV4]
    R --> S[VisualScriptEditorPanel::LoadTemplate]
    S --> T[Display merged graph]
    
    style E fill:#90EE90
    style H fill:#87CEEB
    style T fill:#FFD700
```

---

## Multi-Tab SubGraph Navigation

```mermaid
stateDiagram-v2
    [*] --> RootTab: Editor opens graph

    state RootTab {
        [*] --> ShowRootNodes: Display root graph nodes
        ShowRootNodes --> ShowRootNodes: Edit nodes
    }

    RootTab --> SubGraphTab1: Double-click SubGraph node 1

    state SubGraphTab1 {
        [*] --> LoadSubGraph1: Load subgraph UUID
        LoadSubGraph1 --> ShowSubNodes1: Display subgraph nodes
        ShowSubNodes1 --> ShowSubNodes1: Edit nodes
    }

    SubGraphTab1 --> SubGraphTab2: Double-click SubGraph node 2

    state SubGraphTab2 {
        [*] --> LoadSubGraph2: Load subgraph UUID
        LoadSubGraph2 --> ShowSubNodes2: Display subgraph nodes
        ShowSubNodes2 --> ShowSubNodes2: Edit nodes
    }

    SubGraphTab2 --> SubGraphTab1: Click tab 1
    SubGraphTab1 --> RootTab: Click root tab
    RootTab --> SubGraphTab1: Click tab 1

    SubGraphTab1 --> RootTab: Close tab (×)
    SubGraphTab2 --> SubGraphTab1: Close tab (×)

    note right of RootTab
        Root tab cannot be closed
        Always visible
    end note

    note right of SubGraphTab1
        Tab label: Subgraph name
        Dirty indicator: *
    end note
```

---

## Type System & Data Pin Validation

```mermaid
graph TD
    subgraph PinTypes["Supported Data Types"]
        T1[Int]
        T2[Float]
        T3[Bool]
        T4[String]
        T5[Vector3]
        T6[EntityID]
    end
    
    subgraph Node1["GetBBValue Node"]
        N1O[Output: Value - Float]
    end
    
    subgraph Node2["Branch Node"]
        N2I[Input: Condition - Float]
    end
    
    subgraph Node3["SetBBValue Node"]
        N3I[Input: Value - Float]
    end
    
    subgraph Node4["MathOp Node"]
        N4I1[Input: A - Int]
        N4I2[Input: B - Float]
    end
    
    N1O -->|✅ Float → Float| N2I
    N2I -.->|✅ Float → Float| N3I
    
    N1O -.->|❌ Float → Int| N4I1
    T2 -.->|❌ Float → Int| N4I1
    
    style N1O fill:#90EE90
    style N2I fill:#90EE90
    style N3I fill:#90EE90
    style N4I1 fill:#FF6B6B
```

---

## Summary

These interactive diagrams visualize the complete Olympe Blueprint Editor v4 architecture:

1. **Class Hierarchy** — Backend/Frontend separation, plugin system
2. **Asset Loading** — Multi-version migration (v2/v3/v4)
3. **Graph Creation** — Command pattern, undo/redo
4. **Runtime Execution** — Node-by-node execution flow
5. **SubGraphs** — Call stack, cycle detection, depth limiting
6. **Data Flow** — DataPinCache, Blackboard scoping
7. **Debug System** — State machine, breakpoints, step controls
8. **Command Stack** — Undo/redo with branching history
9. **Validation** — Real-time error detection and navigation
10. **Profiler** — Per-node metrics collection and visualization

All diagrams are **Mermaid-compatible** and can be rendered in:
- GitHub README
- VS Code (with Mermaid extension)
- Markdown viewers (Typora, MarkText, etc.)
- Online editors (mermaid.live, draw.io)

---

**Related Documentation**:
- [Blueprint Editor User Guide v4](Blueprint_Editor_User_Guide_v4.md) — Basic workflows
- [Advanced Systems Documentation](Blueprint_Editor_Advanced_Systems.md) — Technical details
- [ATS Visual Script Complete Documentation](../Documentation/Olympe_ATS_VisualScript_Complete_Doc.md) — Node reference
