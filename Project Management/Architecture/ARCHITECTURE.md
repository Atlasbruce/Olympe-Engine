# Olympe Engine — Architecture

**Last Updated:** 2026-03-16 22:49:08 UTC  
**Author:** @Atlasbruce

---

## Overview

This document describes the high-level architecture of the Olympe Engine Blueprint Editor,
with particular focus on the Phase 24 Condition Preset System Refactor.

---

## Condition Preset System (Phase 24 Refactor)

### Overview

The Condition Preset System decouples condition logic from node implementation,
allowing reusable, globally-managed condition definitions shared across the entire project.

**Key principle:** Nodes no longer store conditions directly. They reference global presets by ID.

---

### Data Model Architecture

#### 1. Global Condition Preset Registry

- **Location:** `./Blueprints/Presets/condition_presets.json`
- **Scope:** Project-level (all blueprints share same presets)
- **Loaded:** At Blueprint editor startup
- **Persistence:** Automatic save on any change

#### 2. Condition Preset Structure

```json
{
  "id": "preset_001",
  "name": "Condition #1",
  "left": {"mode": "Variable|Const|Pin", "value": "..."},
  "operator": "==|!=|<|<=|>|>=",
  "right": {"mode": "Variable|Const|Pin", "value": "..."}
}
```

**Operand modes:**

| Mode | Description |
|---|---|
| `Variable` | References a Blackboard variable by key |
| `Const` | Inline constant float value |
| `Pin` | Dynamic data input pin (value provided at runtime via wire) |

#### 3. Dynamic Data Pin System

Each `Pin` operand generates an independent data input pin on the owning node:

- **Pin ID:** Global unique UUID (e.g., `pin_inst_abc123`)
- **Label:** `In #ConditionIndex(L|R): [condition_preview]`
  - `L` = Left operand
  - `R` = Right operand
- **Data Type:** Float (independent per pin)
- **Mapping:** Pin ID → Runtime data value

Example:

```
Condition: [Pin:1] <= [Pin:2]
├─ Pin 1: "In #3L: [Pin:1] <= [Pin:2]" receives float (Pin:1 value)
└─ Pin 2: "In #3R: [Pin:1] <= [Pin:2]" receives float (Pin:2 value)
```

---

### Node Architecture Update

#### Before (Phase 23 and earlier)

```
NodeBranch
├─ Embedded Condition[] (stored in node)
├─ Complex editing UI in node properties
└─ No reusability (duplicated across nodes)
```

#### After (Phase 24+)

```
NodeBranch
├─ name: string
├─ conditions: NodeConditionRef[] (references only)
│  └─ presetID, logicalOp, leftPinID, rightPinID
├─ dynamicPins: DynamicDataPin[] (auto-generated)
└─ breakpoint: bool

ConditionPresetRegistry (Global)
├─ presets: map<id, ConditionPreset>
├─ Load() / Save()
└─ CRUD API
```

**Why references instead of embedded conditions?**

- **Reusability:** The same condition preset can be referenced by multiple nodes
- **Consistency:** Editing one preset updates all nodes referencing it
- **Separation of concerns:** Condition logic is decoupled from node data
- **Reduced duplication:** No more copy-pasted condition definitions across the graph

---

### NodeConditionRef

```
NodeConditionRef
├─ presetID: string       (references ConditionPresetRegistry entry)
├─ logicalOp: AND|OR      (how this condition combines with the previous)
├─ leftPinID: string      (UUID — populated only when left operand mode == Pin)
└─ rightPinID: string     (UUID — populated only when right operand mode == Pin)
```

---

### DynamicDataPin

```
DynamicDataPin
├─ pinID: string          (globally unique UUID)
├─ label: string          (e.g. "In #3L: [Pin:1] <= [Pin:2]")
├─ dataType: Float
└─ runtimeValue: float    (resolved at evaluation time)
```

---

### Serialization Format (Phase 24+)

#### Node Branch JSON

```json
{
  "nodeType": "Branch",
  "nodeID": 42,
  "name": "My Branch",
  "breakpoint": false,
  "conditions": [
    {
      "presetID": "preset_001",
      "logicalOp": "AND",
      "leftPinID": "pin_inst_abc123",
      "rightPinID": ""
    }
  ],
  "dynamicPins": [
    {
      "pinID": "pin_inst_abc123",
      "label": "In #1L: [Pin] <= 5.0",
      "dataType": "Float"
    }
  ]
}
```

#### condition_presets.json

```json
{
  "version": 1,
  "presets": [
    {
      "id": "preset_001",
      "name": "Health Below Threshold",
      "left": {"mode": "Variable", "value": "bb_health"},
      "operator": "<",
      "right": {"mode": "Const", "value": "25.0"}
    },
    {
      "id": "preset_002",
      "name": "Speed Check",
      "left": {"mode": "Variable", "value": "bb_speed"},
      "operator": ">=",
      "right": {"mode": "Pin", "value": ""}
    }
  ]
}
```

---

### Breaking Changes vs Phase 23

| Area | Phase 23 | Phase 24 |
|---|---|---|
| Condition storage | Embedded in `NodeBranch.conditions[]` | Global registry + node refs |
| Node JSON keys | `conditions[].type`, `conditions[].value` | `conditions[].presetID`, `conditions[].logicalOp` |
| Pin management | Static pins defined at node creation | Dynamic pins generated from condition refs |
| Editing workflow | Edit conditions inline in node properties | Edit global presets in dedicated panel; assign to nodes |
| Reusability | None (per-node duplication) | Full reuse across all nodes and blueprints |
| Migration path | N/A | Manual migration required for existing blueprints (future task) |

---

### Module Implications

| Module | Impact |
|---|---|
| `VisualScriptEditorPanel` | New panel for Global Preset Manager; updated node properties rendering |
| `VisualScriptNodeRenderer` | Updated `NodeBranch` rendering with dynamic pins |
| `UndoRedoStack` | New commands: `AddPresetCommand`, `EditPresetCommand`, `DeletePresetCommand`, `AssignPresetToNodeCommand` |
| `TaskGraphLoader` | Updated deserialization to handle Phase 24 node JSON format |
| `VSGraphVerifier` | New validation rules for unresolved preset references and dangling pin IDs |

---

### Validation Rules (Phase 24, Planned)

| ID | Severity | Rule |
|---|---|---|
| E030 | ERROR | NodeBranch references unknown preset ID |
| E031 | ERROR | NodeConditionRef has unresolved leftPinID |
| E032 | ERROR | NodeConditionRef has unresolved rightPinID |
| W020 | WARNING | ConditionPreset defined but not referenced by any node |
| W021 | WARNING | DynamicDataPin with no incoming connection |

---

## Blueprint Editor — Existing Architecture

### Module Overview

| Module | Purpose |
|---|---|
| `VisualScriptEditorPanel` | Main editor panel: canvas, toolbar, properties, blackboard |
| `VisualScriptNodeRenderer` | Per-node ImNodes rendering logic |
| `VSGraphVerifier` | Stateless global graph validation (22+ rules) |
| `VSConnectionValidator` | Per-connection validation (self-loop, duplicate, cycle) |
| `UndoRedoStack` | Command stack (all editor mutations are undoable) |
| `TaskGraphLoader` | JSON serialization/deserialization |
| `BlackboardVariablePresetRegistry` | Global singleton for Blackboard variable presets (Phase 23-B) |
| `BBVariableRegistry` | Per-graph Blackboard variable store |

### Undo/Redo Architecture

All mutations in the Blueprint Editor go through `UndoRedoStack`. Commands implement:

```
IEditorCommand
├─ Execute()
├─ Undo()
└─ Redo()
```

**Existing commands:**

| Command | Action |
|---|---|
| `AddNodeCommand` | Add a node to the graph |
| `DeleteNodeCommand` | Remove a node |
| `MoveNodeCommand` | Move a node on the canvas |
| `AddConnectionCommand` | Add an exec connection |
| `AddDataConnectionCommand` | Add a data connection |
| `DeleteLinkCommand` | Remove a connection |
| `EditNodePropertyCommand` | Edit a node property value |
| `AddExecPinCommand` | Add a dynamic exec output pin |
| `RemoveExecPinCommand` | Remove a dynamic exec output pin |

### Save / Load Architecture

- **Serialization format:** JSON (version 4+)
- **Node positions:** Stored in ImNodes **grid space** (not editor/screen space)
- **Blackboard:** Serialized in `SaveAndWrite()` / loaded in `ParseBlackboardV4()`
- **Viewport:** Reset before save, restored after save to avoid position corruption

---

**Last Updated:** 2026-03-16 22:49:08 UTC
