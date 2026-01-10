# Blueprint Editor Architecture - Visual Guide

## 1. EditorContext: Capability-Driven Design

```
┌──────────────────────────────────────────────────────────────┐
│                      EditorContext                           │
│                       (Singleton)                            │
├──────────────────────────────────────────────────────────────┤
│                   EditorCapabilities                         │
│                                                              │
│  Runtime Mode          │  Standalone Mode                   │
│  ────────────          │  ───────────────                   │
│  isRuntime = true      │  isRuntime = false                 │
│  canCreate = false     │  canCreate = true                  │
│  canEdit = false       │  canEdit = true                    │
│  canDelete = false     │  canDelete = true                  │
│  canLink = false       │  canLink = true                    │
│  canSave = false       │  canSave = true                    │
│  showEntityContext=true│  showEntityContext=false           │
└──────────────────────────────────────────────────────────────┘
                            │
                            │ Query capabilities
                            ▼
        ┌───────────────────────────────────────┐
        │       NodeGraphPanel                  │
        │       (UI Components)                 │
        ├───────────────────────────────────────┤
        │  if (CanCreate()) {                  │
        │    Show "Create Node" menu           │
        │  }                                   │
        │                                      │
        │  if (CanDelete()) {                  │
        │    Enable Delete key                 │
        │    Show "Delete" menu item           │
        │  }                                   │
        │                                      │
        │  if (CanLink()) {                    │
        │    Enable link creation              │
        │  }                                   │
        └───────────────────────────────────────┘
```

## 2. Tab Persistence Architecture

```
Before Fix:
┌─────────────────────────────────────────┐
│  Graph Tab Bar                          │
│  ┌─────┬─────┬─────┐                   │
│  │ G1  │ G2  │ G3  │                   │  User clicks G3
│  └─────┴─────┴─────┘                   │       │
│         ▲                               │       │
│         │ Always re-selects first!     │       ▼
│         │                               │  Selection changes to G3
│    RenderGraphTabs()                    │       │
│    calls SetActiveGraph(G1)             │       │
│    EVERY frame!                         │       ▼
│                                         │  Next frame: forced back to G1
└─────────────────────────────────────────┘  (BUG!)

After Fix:
┌────────────────────────────────────────────────────┐
│  NodeGraphManager                                  │
│  ┌──────────────────────────────────────────────┐ │
│  │  m_Graphs = { 1→G1, 2→G2, 3→G3 }           │ │
│  │  m_GraphOrder = [1, 2, 3]  ← Insertion order│ │
│  │  m_ActiveGraphId = 3                        │ │
│  │  m_LastActiveGraphId = 3   ← Persistent!   │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
             │
             │ GetAllGraphIds() returns [1,2,3]
             ▼
┌─────────────────────────────────────────┐
│  RenderGraphTabs()                      │
│  ┌─────┬─────┬─────┐                   │
│  │ G1  │ G2  │ G3* │ ← Active tab      │
│  └─────┴─────┴─────┘                   │
│                                         │
│  Only sets SetSelected flag on G3      │
│  Only calls SetActiveGraph() when      │
│  user clicks a DIFFERENT tab           │
│                                         │
│  Result: Tab stays selected! ✓         │
└─────────────────────────────────────────┘
```

## 3. DnD Safety Flow

```
Before Fix:
┌──────────────────────────────────────────┐
│  User drags "MoveTo" node                │
│           ▼                              │
│  ImGui::AcceptDragDropPayload()         │
│           ▼                              │
│  std::string data = (char*)payload->Data│ ← NO VALIDATION!
│           ▼                              │
│  CreateNode(Action, data)               │ ← CRASH if null/corrupt!
└──────────────────────────────────────────┘

After Fix:
┌──────────────────────────────────────────────────────┐
│  User drags "MoveTo" node                            │
│           ▼                                          │
│  payload = AcceptDragDropPayload("NODE_TYPE")       │
│           ▼                                          │
│  ┌────────────────────────────┐                     │
│  │ Safety Validation Layer    │                     │
│  ├────────────────────────────┤                     │
│  │ 1. Check payload != null   │                     │
│  │ 2. Check Data != null      │                     │
│  │ 3. Check DataSize > 0      │                     │
│  │ 4. Bounds check (max 256B) │                     │
│  │ 5. Copy to local string    │                     │
│  │ 6. Ensure NUL-termination  │                     │
│  │ 7. Validate format         │                     │
│  │ 8. Check catalog validity  │                     │
│  └────────────────────────────┘                     │
│           ▼                                          │
│  if (IsValidActionType("MoveTo")) {                 │
│      CreateNode(Action, "MoveTo")  ← Safe! ✓        │
│  } else {                                           │
│      ShowErrorTooltip("Invalid type")               │
│  }                                                  │
└──────────────────────────────────────────────────────┘
```

## 4. JSON Validation Flow

```
┌────────────────────────────────────────────────────┐
│  Blueprint File (guard_patrol.json)                │
│  ┌──────────────────────────────────────────────┐ │
│  │ {                                            │ │
│  │   "rootNodeId": 1,                          │ │
│  │   "nodes": [...]                            │ │
│  │   // Missing: type, metadata, editorState   │ │
│  │ }                                            │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
                    │
                    │ Load
                    ▼
┌────────────────────────────────────────────────────┐
│  BlueprintValidator::DetectType()                  │
│  ┌──────────────────────────────────────────────┐ │
│  │ Heuristics:                                  │ │
│  │  - Has rootNodeId + nodes → BehaviorTree    │ │
│  │  - Has states + initialState → HFSM         │ │
│  │  - Has components → EntityPrefab            │ │
│  │  - Has elements → UIBlueprint               │ │
│  │  - Has worldSize/entities → Level           │ │
│  └──────────────────────────────────────────────┘ │
│              │                                     │
│              └─→ Detected: "BehaviorTree"         │
└────────────────────────────────────────────────────┘
                    │
                    ▼
┌────────────────────────────────────────────────────┐
│  BlueprintValidator::Normalize()                   │
│  ┌──────────────────────────────────────────────┐ │
│  │ Adds missing fields:                         │ │
│  │  + schema_version: 2                         │ │
│  │  + type: "BehaviorTree"                      │ │
│  │  + blueprintType: "BehaviorTree"             │ │
│  │  + metadata: {                               │ │
│  │      author: "Unknown",                      │ │
│  │      created: "",                            │ │
│  │      tags: []                                │ │
│  │    }                                         │ │
│  │  + editorState: {                            │ │
│  │      zoom: 1.0,                              │ │
│  │      scrollOffset: {x:0, y:0}                │ │
│  │    }                                         │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
                    │
                    ▼
┌────────────────────────────────────────────────────┐
│  BlueprintValidator::ValidateJSON()                │
│  ┌──────────────────────────────────────────────┐ │
│  │ BehaviorTree validation:                     │ │
│  │  ✓ Has data.nodes (array)                    │ │
│  │  ✓ Has data.rootNodeId (integer)             │ │
│  │                                              │ │
│  │ Result: VALID ✓                              │ │
│  └──────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────┘
                    │
                    │ All checks passed
                    ▼
┌────────────────────────────────────────────────────┐
│  Normalized & Validated Blueprint                  │
│  Ready to use in editor                            │
└────────────────────────────────────────────────────┘
```

## 5. Complete System Integration

```
                    ┌─────────────────────┐
                    │   User Action       │
                    │  (Gameplay/Editing) │
                    └──────────┬──────────┘
                               │
            ┌──────────────────┴──────────────────┐
            ▼                                     ▼
  ┌──────────────────┐              ┌──────────────────┐
  │ Initialize       │              │ Initialize       │
  │ RuntimeEditor()  │              │ StandaloneEditor()│
  └────────┬─────────┘              └────────┬─────────┘
           │                                  │
           ▼                                  ▼
  ┌──────────────────────────────────────────────────┐
  │           EditorContext                          │
  │  Sets capabilities for current mode              │
  └────────┬─────────────────────────────────────────┘
           │
           ▼
  ┌──────────────────────────────────────────────────┐
  │         BlueprintEditor Backend                  │
  │  - Manages graphs (NodeGraphManager)             │
  │  - Validates JSON (BlueprintValidator)           │
  │  - Tracks entities                               │
  │  - Handles assets                                │
  └────────┬─────────────────────────────────────────┘
           │
           ▼
  ┌──────────────────────────────────────────────────┐
  │         NodeGraphPanel (UI)                      │
  │  - Renders tabs (with persistence)               │
  │  - Handles DnD (with safety)                     │
  │  - Shows context menus (capability-gated)        │
  │  - Processes keyboard shortcuts (gated)          │
  │  - Renders graphs (entity-independent)           │
  └──────────────────────────────────────────────────┘
```

## Key Benefits Summary

### 1. EditorContext
- ✅ Clear separation: Runtime (read-only) vs Standalone (full CRUD)
- ✅ Single source of truth for capabilities
- ✅ Easy to add new modes or capabilities
- ✅ UI automatically adapts to mode

### 2. Tab Persistence
- ✅ Tabs maintain order across sessions
- ✅ Active tab doesn't jump around
- ✅ Smart neighbor selection on close
- ✅ Predictable user experience

### 3. DnD Safety
- ✅ No crashes from null/corrupt payloads
- ✅ Bounds checking prevents overflows
- ✅ Type validation before creation
- ✅ User feedback via tooltips

### 4. JSON Validation
- ✅ Automatic type detection
- ✅ Missing fields added transparently
- ✅ Structural validation catches errors
- ✅ Better error messages

### 5. Overall Architecture
- ✅ Minimal changes (~800 LOC total)
- ✅ C++14 compliant
- ✅ Backward compatible
- ✅ Well documented
- ✅ Extensible design
