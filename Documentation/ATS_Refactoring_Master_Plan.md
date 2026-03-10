# ATS Visual Scripting System - Complete Refactoring Master Plan

## Executive Summary
- Overview of the ATS refactoring initiative
- Goals: Migration from BehaviorTree to Visual Scripting, schema v4 flat format
- Current status: Phases 7-8 completed and merged

## Project Context
### Original System (Schema v2)
- Doubly-nested JSON structure (data.nodes[].data.nodes[])
- BehaviorTree-centric design
- Limited flexibility for visual scripting

### Migration Path
- Schema v2 → v3 (flat nodes with NextOnSuccess/NextOnFailure)
- Schema v3 → v4 (ATS Visual Scripting flat format)
- Full backward compatibility maintained

## Detailed Phase Breakdown

### Phase 1: Foundation & Schema Migration ✅
**Status**: COMPLETED

**Objectives**:
- Establish schema v4 flat format
- Create BTtoVSMigrator for legacy support

**Technical Details**:
- Implemented TaskGraphLoader with version detection
- Created migration pipeline for doubly-nested v2 format
- Added JsonHelper utilities for safe JSON parsing

**Design Decisions**:
- Single-level flat structure for nodes array
- Explicit execution connections (execConnections)
- Separate data connections (dataConnections)

**Files Modified**:
- Source/TaskSystem/TaskGraphLoader.cpp
- Source/TaskSystem/TaskGraphLoader.h
- Source/BlueprintEditor/BTtoVSMigrator.cpp
- Source/json_helper.h

### Phase 2: Core Node Types ✅
**Status**: COMPLETED

**Objectives**:
- Implement EntryPoint node
- Implement core control flow nodes

**Node Types Implemented**:
1. **EntryPoint**
   - Purpose: Single entry point for graph execution
   - Pins: Output execution pin "Start"
   
2. **Branch** (conditional execution)
   - Input: execution + boolean condition
   - Outputs: "True" and "False" execution paths
   
3. **Sequence**
   - Executes multiple nodes in order
   - Outputs: numbered execution pins (Out0, Out1, Out2...)

**Technical Implementation**:
- TaskNodeDefinition structure extended
- TaskNodeType enum with new node types
- ExecutionPin and DataPin structures

**Justification**:
- EntryPoint ensures deterministic graph entry
- Branch enables conditional AI behaviors
- Sequence allows ordered action execution

### Phase 3: Data Flow System ✅
**Status**: COMPLETED

**Objectives**:
- Implement data pins and connections
- Support typed data flow between nodes

**Features**:
- DataPin structure (name, type, direction)
- DataConnection structure (fromNode, fromPin, toNode, toPin)
- Type validation during connection

**Supported Data Types**:
- Boolean
- Integer
- Float
- String
- Vector2/Vector3
- Entity reference

**Technical Details**:
- DataConnections stored separately from ExecConnections
- Runtime type checking during graph instantiation
- Blackboard integration for persistent data

**Files Modified**:
- Source/TaskSystem/TaskGraphTemplate.h
- Source/TaskSystem/TaskGraphLoader.cpp (ParseSchemaV4)

### Phase 4: Blackboard Variables ✅
**Status**: COMPLETED

**Objectives**:
- Global variable system for task graphs
- Read/write access from any node

**Features Implemented**:
1. **GetBlackboardVariable** node
   - Reads variable from blackboard
   - Output data pin with variable value
   
2. **SetBlackboardVariable** node
   - Writes variable to blackboard
   - Input data pin for new value

**Blackboard Schema**:
```json
{
  "blackboard": [
    {
      "key": "variableName",
      "type": "Float",
      "value": 0.0,
      "scope": "local"
    }
  ]
}
```

**Scope Types**:
- local: Instance-specific (per entity)
- shared: Graph-wide (shared across instances)
- global: Engine-wide (shared across all graphs)

**Technical Implementation**:
- BlackboardVariable structure in TaskGraphTemplate
- Runtime blackboard instance per graph execution
- Scope resolution during variable access

### Phase 5: Math & Logic Nodes ✅
**Status**: COMPLETED

**Objectives**:
- Mathematical operations
- Logical operations
- Comparison operations

**Node Types**:
1. **MathOperation**
   - Operations: Add, Subtract, Multiply, Divide
   - Inputs: A (float), B (float)
   - Output: Result (float)

2. **CompareOperation**
   - Comparisons: Equal, NotEqual, Greater, Less, GreaterOrEqual, LessOrEqual
   - Inputs: A (float), B (float)
   - Output: Result (boolean)

3. **LogicOperation**
   - Operations: AND, OR, NOT
   - Inputs: A (bool), B (bool, optional for NOT)
   - Output: Result (boolean)

**Design Decisions**:
- Pure data nodes (no execution pins)
- Strongly typed inputs/outputs
- Support for constant values or data connections

**Use Cases**:
- Distance calculations for AI
- Health threshold checks
- Complex conditional logic

### Phase 6: Switch & ForEach Nodes ✅
**Status**: COMPLETED

**Objectives**:
- Multi-branch control flow
- Collection iteration

**Node Types**:
1. **Switch**
   - Input: integer or string key
   - Multiple output execution pins (Case0, Case1, ..., Default)
   - Routing based on key value

2. **ForEach**
   - Input: array/collection
   - Outputs: LoopBody (per iteration), Completed (after all)
   - Provides CurrentIndex and CurrentItem variables

**Technical Challenges**:
- Dynamic pin generation based on cases
- Loop state management for ForEach
- Break/Continue support (future enhancement)

**Example Use Case**:
```
ForEach(EnemiesInRange) → ApplyDamage → Completed → NextAction
```

### Phase 7: Final Integration & Advanced Features ✅
**Status**: COMPLETED & MERGED (PR #342)

**Objectives**:
- Complete TaskGraphLoader implementation
- Advanced node features
- Full migration support

**Key Features**:
1. **Schema Version Detection**
   - Automatic detection of v2/v3/v4 formats
   - Routing to appropriate parser

2. **Field Name Flexibility**
   - Support both new and legacy field names
   - "id"/"nodeID", "type"/"nodeType", "params"/"parameters"

3. **Connection Migration**
   - Legacy NextOnSuccess/NextOnFailure to execConnections
   - Automatic connection generation

**Code Improvements**:
- Comprehensive error reporting
- Validation at each parsing stage
- Detailed logging for debugging

**Testing**:
- Unit tests for all schema versions
- Integration tests with existing graphs
- Test cases in Tests/TaskSystem/TaskGraphLoaderTest.cpp

**Files Modified**:
- Source/TaskSystem/TaskGraphLoader.cpp (complete rewrite)
- Source/TaskSystem/TaskGraphLoader.h
- Source/TaskSystem/TaskGraphMigrator_v3_to_v4.cpp
- Tests/TaskSystem/TaskGraphLoaderTest.cpp

### Phase 8: Subgraph System (Nested Graphs) ✅
**Status**: COMPLETED & MERGED (PR #343)

**Objectives**:
- Support for reusable graph modules
- Graph nesting and composition

**Features Implemented**:
1. **SubgraphNode**
   - References external graph file
   - Exposes input/output parameters as pins
   - Execution flow: In → SubgraphExecution → Out

2. **Subgraph Parameters**
   - Input parameters: passed to subgraph blackboard
   - Output parameters: returned from subgraph
   - Type-safe parameter binding

3. **Subgraph Loader**
   - Lazy loading of subgraph templates
   - Caching for performance
   - Circular dependency detection

**JSON Schema**:
```json
{
  "id": 10,
  "type": "Subgraph",
  "label": "PatrolBehavior",
  "params": {
    "subgraphPath": "Blueprints/AI/patrol_subgraph.json",
    "inputParams": {
      "patrolRadius": "blackboard:patrolRange"
    },
    "outputParams": {
      "targetReached": "local:patrolComplete"
    }
  }
}
```

**Technical Implementation**:
- SubgraphDefinition structure
- SubgraphInstance runtime management
- Parameter mapping between parent and subgraph blackboards

**Use Cases**:
- Reusable AI behaviors (patrol, chase, flee)
- Complex state machines as subgraphs
- Modular graph design

**Files Modified**:
- Source/TaskSystem/TaskGraphTemplate.h (added Subgraph node type)
- Source/TaskSystem/TaskGraphLoader.cpp (subgraph parsing)
- Source/TaskSystem/TaskGraphRuntime.cpp (subgraph execution)

## Current Status & Issues

### ✅ Completed Work
- All 8 phases implemented
- PRs #342 and #343 merged to master
- Schema v4 fully operational
- Backward compatibility maintained

### ✅ Current Issue: RESOLVED (Phase 9 Complete)

**Issues Fixed** (Phase 9 — 2026-03-10):

1. **ImNodes Assertion Crash** — Fixed in `VisualScriptEditorPanel.cpp`
   - Nodes dropped via drag-and-drop now pre-register their position with
     `ImNodes::SetNodeEditorSpacePos()` to avoid the `node_idx != -1` assertion.
   - Position-sync loop skips nodes that haven't been rendered yet this frame
     using the `m_positionedNodes` tracking set.

2. **Inconsistent JSON Schema** — Fixed across all Blueprints/AI files
   - Added root-level `"type"` field (equal to `"graphType"`) to all 22 v4 files.
   - Migrated `patrol.json` and `guard.json` from legacy `nodeID`/`nodeType`/
     `localBlackboard`/`ExecConnections` fields to the standard v4 schema.
   - Normalized `connections` → `execConnections`/`dataConnections` in Examples
     and Templates directories.

3. **Missing Catalogs** — Fixed in `Blueprints/Catalogues/`
   - `ActionTypes.json` now contains 7 action definitions (Wait, MoveToLocation,
     Attack, Flee, ChangeState, PatrolPickPoint, SetMoveGoal).
   - `ConditionTypes.json` now contains 4 condition definitions (CheckBlackboardValue,
     TargetInRange, TargetVisible, HasTarget).

4. **Reference Test Graph** — Created `Gamedata/TaskGraph/Examples/reference_complete_v4.ats`
   - Exercises all 11 v4 node types: EntryPoint, VSSequence, Branch, AtomicTask,
     GetBBValue, SetBBValue, MathOp, DoOnce, While, Delay, SubGraph.

5. **Integration Test** — Added `OlympeATSReferenceV4Tests` (26th test target)
   - Validates loading, node count, all node types, blackboard variables,
     and one-frame execution.

6. **Validation Script** — Created `Tools/validate_ats_v4.py`
   - Reports 0 errors for all 22 v4 files in `Blueprints/AI/`.

## Next Steps

### Phase 10: Blueprint Editor Integration 📅
**Status**: PLANNED

**Objectives**:
- Visual graph editing
- Node palette
- Connection management

**Features**:
- Drag-and-drop node creation
- Visual connection drawing
- Real-time validation
- Schema v4 export

### Phase 11: Advanced Node Library 📅
**Status**: PLANNED

**Objectives**:
- Expand node library
- AI-specific nodes
- Game-specific utilities

**Planned Nodes**:
- WaitForSeconds (delay execution)
- PlayAnimation
- PlaySound
- SpawnEntity
- RaycastCheck
- GetDistance
- MoveToLocation
- LookAt

### Phase 12: Performance Optimization 📅
**Status**: PLANNED

**Objectives**:
- Graph execution optimization
- Memory management
- Profiling tools

**Areas**:
- Node execution caching
- Connection lookup optimization
- Blackboard access patterns
- Subgraph instantiation pooling

## Technical Architecture

### File Structure
```
Source/
├── TaskSystem/
│   ├── TaskGraphTemplate.h          # Graph structure definitions
│   ├── TaskGraphTemplate.cpp        
│   ├── TaskGraphLoader.h            # JSON loading & parsing
│   ├── TaskGraphLoader.cpp          
│   ├── TaskGraphRuntime.h           # Execution engine
│   ├── TaskGraphRuntime.cpp         
│   ├── TaskGraphMigrator_v3_to_v4.h # Schema migration
│   └── TaskGraphMigrator_v3_to_v4.cpp
├── BlueprintEditor/
│   └── BTtoVSMigrator.cpp           # Legacy v2 migration
└── json_helper.h                    # JSON utilities

Blueprints/
└── AI/
    ├── guardV2_ai.json              # Example v4 graph
    └── npc_wander_ai.json           # Example v2 graph (legacy)

Tests/
└── TaskSystem/
    └── TaskGraphLoaderTest.cpp      # Unit tests
```

### Key Data Structures

```cpp
// Node definition
struct TaskNodeDefinition {
    int NodeID;
    TaskNodeType Type;
    std::string Label;
    std::map<std::string, std::string> Parameters;
    std::vector<ExecutionPin> OutputExecPins;
    std::vector<DataPin> InputDataPins;
    std::vector<DataPin> OutputDataPins;
};

// Graph template
struct TaskGraphTemplate {
    std::string GraphType;  // "VisualScript"
    int RootNodeID;
    std::vector<TaskNodeDefinition> Nodes;
    std::vector<ExecutionConnection> ExecConnections;
    std::vector<DataConnection> DataConnections;
    std::vector<BlackboardVariable> Blackboard;
};

// Runtime instance
class TaskGraphRuntime {
    TaskGraphTemplate* Template;
    std::map<std::string, Variant> BlackboardValues;
    int CurrentNodeID;
    ExecutionState State;
};
```

### Schema v4 JSON Format

```json
{
  "schema_version": 4,
  "graphType": "VisualScript",
  "metadata": {
    "name": "GuardAI",
    "description": "Guard patrol and chase behavior",
    "author": "Olympe Engine",
    "version": "1.0"
  },
  "nodes": [
    {
      "id": 0,
      "type": "EntryPoint",
      "label": "Start",
      "position": {"x": 100, "y": 100}
    },
    {
      "id": 1,
      "type": "Branch",
      "label": "CheckPlayerDistance",
      "params": {
        "condition": "blackboard:playerInRange"
      },
      "position": {"x": 300, "y": 100}
    }
  ],
  "execConnections": [
    {
      "fromNode": 0,
      "fromPin": "Start",
      "toNode": 1,
      "toPin": "In"
    },
    {
      "fromNode": 1,
      "fromPin": "True",
      "toNode": 2,
      "toPin": "In"
    }
  ],
  "dataConnections": [
    {
      "fromNode": 10,
      "fromPin": "Distance",
      "toNode": 11,
      "toPin": "A"
    }
  ],
  "blackboard": [
    {
      "key": "playerInRange",
      "type": "Boolean",
      "value": false,
      "scope": "local"
    },
    {
      "key": "patrolRadius",
      "type": "Float",
      "value": 100.0,
      "scope": "local"
    }
  ]
}
```

## Design Principles

### 1. Backward Compatibility
- All legacy formats (v2, v3) must continue to work
- Migration is automatic and transparent
- No manual file conversion required

### 2. Type Safety
- Strong typing for data connections
- Compile-time validation where possible
- Runtime type checking for safety

### 3. Visual-First Design
- Graph structure optimized for visual representation
- Flat format (no nested nodes)
- Explicit connections (no implicit flow)

### 4. Performance
- Minimal runtime overhead
- Efficient lookup structures
- Lazy loading of subgraphs

### 5. Extensibility
- Easy to add new node types
- Plugin architecture for custom nodes
- Schema evolution support

## Known Limitations

1. **No Circular Subgraphs**: Circular dependencies are detected and rejected
2. **Single Entry Point**: Only one EntryPoint node per graph
3. **Static Pin Count**: Pins cannot be added/removed at runtime (except Switch)
4. **No Multi-Threading**: Graph execution is single-threaded

## References

### Related Documentation
- `Documentation/AI_System_Guide.md` - AI integration
- `website/docs/user-guide/blueprint-editor.md` - Visual editor
- `Source/TaskSystem/README.md` - Task system overview

### Pull Requests
- PR #342: Phase 7 - Final Integration & Advanced Features
- PR #343: Phase 8 - Subgraph System (Nested Graphs)

### Test Files
- `Tests/TaskSystem/TaskGraphLoaderTest.cpp` - Loader tests
- `Blueprints/AI/guardV2_ai.json` - v4 example
- `Blueprints/AI/npc_wander_ai.json` - v2 example

## Glossary

- **ATS**: Advanced Task System
- **BT**: Behavior Tree (legacy system)
- **VS**: Visual Scripting (new system)
- **Blackboard**: Global variable storage for graphs
- **Subgraph**: Reusable nested graph module
- **Execution Pin**: Connection point for control flow
- **Data Pin**: Connection point for data flow
- **Schema**: JSON format version for task graphs

---

**Document Version**: 1.0  
**Last Updated**: 2026-03-10 17:27:00  
**Status**: ✅ Fully Operational — Phase 9 COMPLETE