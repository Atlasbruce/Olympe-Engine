# Phase 24 — Branch/While Condition Node UI/UX Specification

**Created:** 2026-03-18 UTC  
**Status:** 📋 Phase 1 — Documentation Baseline  
**Owner:** Copilot  

---

## 1. Mockup Reference

Target design for Branch/While node:

```
┌────────────────────────────────────────────────┐
│ Is Health Critical?                      [Name]│
│────────────────────────────────────────────────│
│ In                                Then    Else │
│ [arc left]          [condition logic]  [arcs]  │
│                                                 │
│ [mHealth] <= [2]        ← SECTION 3 (green)    │
│ And [mSpeed] <= [100.0]                        │
│ Or [mSpeed] == [Pin:1]  ← Yellow highlight     │
│                                                 │
│ Pin-in #1                 Pin-in #2            │
│ [Enabled on Node]     [Enabled on Node]        │
│                                                 │
│ [mSpeed] == [Pin:1]   ← condition avec ref     │
│ mSpeed     == Pin-in #1  [CSDT]                │
└────────────────────────────────────────────────┘
```

**Panel Properties (right-side inspector):**
```
┌────────────────────────────────────────────────┐
│ Is Health Critical?                      [Name]│
│────────────────────────────────────────────────│
│ In                                              │
│ Then                                            │
│ Else                                            │
│                                                 │
│ ▼ Edit Conditions  (collapsible, inline)       │
│   [mHealth] <= [2]           [And ▾] [X]       │
│   [mSpeed] <= [100.00]       [Or  ▾] [X]       │
│   [mSpeed] == [Pin ▾]        [—   ] [X]        │
│   [+ Add Condition]                             │
│                                                 │
│ ▼ Condition Preset                             │
│   Preset Name: [___________]                   │
│   [Save Preset] [Load Preset]                   │
│                                                 │
│ Breakpoint (F9) [ ]                            │
│ Graph Verification                             │
└────────────────────────────────────────────────┘
```

---

## 2. Panel Layout Specifications

### 2.1 Properties Panel — Section Structure

| Section | Label | Color | Requirement |
|---------|-------|-------|-------------|
| Title | `def.NodeName` | Blue `RGBA(0.2, 0.8, 1.0, 1.0)` | Always rendered; no duplicate |
| Exec Pins | In / Then / Else | Cyan `RGBA(0.4, 0.8, 1.0, 1.0)` | Shown as read-only text |
| Conditions | Condition list | Green `RGBA(0.2, 0.9, 0.2, 1.0)` | Inline editable rows |
| Condition Preset | Save/load preset | Default | Collapsible section |
| Breakpoint | F9 checkbox | Default | Always present |

### 2.2 Conditions List (Section 3 — Properties Panel)

Each condition row contains:
- **Left operand dropdown** — variable or constant selector
- **Operator dropdown** — `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Right operand dropdown** — variable, constant, or **Pin** (dynamic pin reference)
- **Logical operator dropdown** — `And` / `Or` (not shown for first row)
- **Delete button** — `[X]` removes the condition and its associated dynamic pin

Rendered **inline** inside the Properties panel (NOT as a modal/popup).

```cpp
// ✅ CORRECT — Inline rendering
if (ImGui::CollapsingHeader("Edit Conditions", ImGuiTreeNodeFlags_DefaultOpen)) {
    m_conditionsPanel->RenderUI();
}

// ❌ INCORRECT — Modal rendering (do not use)
if (ImGui::Button("Edit Conditions")) {
    ImGui::OpenPopup("EditConditionsModal");
}
```

### 2.3 Condition Preset Section

- Collapsible header: `"Condition Preset"`
- Text input for preset name
- `[Save Preset]` button — saves current conditions to registry
- `[Load Preset]` button — loads conditions from registry into node

---

## 3. Node Canvas Rendering Specifications

### 3.1 Visual Section Hierarchy on Canvas

| Section | Content | Color |
|---------|---------|-------|
| 1 — Title | `def.NodeName` (14pt) | Blue `#0066CC` |
| 2 — Exec Pins | `In`, `Then`, `Else` pin connectors | Cyan |
| 3 — Conditions | All condition expressions as text | Green `#00FF00` |
| 4 — Dynamic Pins | Dynamic pin slot labels | Yellow `#FFD700` |

### 3.2 Section 3 — Conditions Preview (Canvas)

Render each condition as a text line on the canvas node body:
```
[mHealth] <= [2]
And [mSpeed] <= [100.0]
Or [mSpeed] == [Pin:1]
```

Implementation:
```cpp
if (nodeType == TaskNodeType::Branch || nodeType == TaskNodeType::While) {
    if (!def.conditionRefs.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.2f, 1.0f)); // green
        for (const auto& ref : def.conditionRefs) {
            ImGui::Text("[%s] %s [%s]",
                GetOperandDisplayName(ref.left).c_str(),
                ref.operatorStr.c_str(),
                GetOperandDisplayName(ref.right).c_str());
        }
        ImGui::PopStyleColor();
    }
}
```

### 3.3 Section 4 — Dynamic Pin Slots (Canvas)

Each dynamic data pin renders as a named input slot on the node:
```
Pin-in #1   [●─────]
Pin-in #2   [●─────]
```

Color: Yellow `RGBA(1.0, 0.9, 0.2, 1.0)`

---

## 4. Data Structure Requirements

### 4.1 `NodeConditionRef` (required fields)

```cpp
struct NodeConditionRef {
    std::string id;           // UUID
    Operand     left;         // left operand
    std::string operatorStr;  // "==", "!=", "<", "<=", ">", ">="
    Operand     right;        // right operand
    std::string logicalOp;    // "And" | "Or" | "" (first condition)

    // Dynamic pin references (populated by DynamicDataPinManager)
    std::string leftPinID;    // UUID of dynamic pin for left operand (Pin mode)
    std::string rightPinID;   // UUID of dynamic pin for right operand (Pin mode)
};
```

### 4.2 `DynamicDataPin` (required fields)

```cpp
struct DynamicDataPin {
    std::string id;             // globally unique UUID
    std::string label;          // display label e.g. "Pin-in #1"
    int         conditionIndex; // which condition owns this pin
    OperandPosition position;   // Left or Right operand side
    bool        enabled;        // visible/connected on canvas
};
```

### 4.3 `ConditionRef → DynamicDataPin` Mapping

Each operand set to **Pin mode** in a `NodeConditionRef` must generate exactly one `DynamicDataPin`.  
The pin UUID is stored in `NodeConditionRef::leftPinID` or `rightPinID`.

---

## 5. Pin Generation Algorithm

```
For each condition at index i in conditionRefs:
    if condition.left.mode == Operand::Mode::Pin:
        pin = DynamicDataPin {
            id             = generateUUID(),
            label          = "Pin-in #" + nextPinNumber(),
            conditionIndex = i,
            position       = OperandPosition::Left,
            enabled        = true
        }
        conditionRefs[i].leftPinID = pin.id
        dynamicPins.push_back(pin)

    if condition.right.mode == Operand::Mode::Pin:
        pin = DynamicDataPin {
            id             = generateUUID(),
            label          = "Pin-in #" + nextPinNumber(),
            conditionIndex = i,
            position       = OperandPosition::Right,
            enabled        = true
        }
        conditionRefs[i].rightPinID = pin.id
        dynamicPins.push_back(pin)
```

Pin UUIDs must be **globally unique** and **stable** across reloads (reuse by conditionIndex + side when UUID already exists).

---

## 6. Acceptance Criteria

### 6.1 Properties Panel

- [ ] **P1** — Title renders once (no duplicate), blue color
- [ ] **P1** — Exec pins (In/Then/Else) visible as read-only
- [ ] **P1** — Condition list renders inline (not modal)
- [ ] **P1** — Each condition row: left dropdown + operator + right dropdown + logical op + delete button
- [ ] **P1** — `[+ Add Condition]` button adds a new empty condition row
- [ ] **P1** — Logical operator (`And`/`Or`) shown per row (not first row)
- [ ] **P1** — Delete button removes condition AND its associated dynamic pin(s)
- [ ] **P2** — Condition Preset section: save/load from registry
- [ ] **P2** — Breakpoint checkbox (F9) functional

### 6.2 Canvas Rendering

- [ ] **P1** — Section 3 (conditions) renders all condition expressions in green
- [ ] **P1** — Section 4 (dynamic pins) renders pin slot labels in yellow
- [ ] **P2** — Hover tooltip shows full condition details

### 6.3 Data Synchronization

- [ ] **P1** — Editing a condition updates `def.conditionRefs` in the node definition
- [ ] **P1** — Pin mode operand triggers DynamicDataPinManager to generate a pin
- [ ] **P1** — Panel reload triggers when `m_selectedNodeID` changes
- [ ] **P1** — `IsDirty()` flag correctly propagates condition changes to template

### 6.4 Quality Gates

- [ ] Zero compile errors (C++14, headless)
- [ ] Zero ImGui layout regressions on other node types
- [ ] Console: 0 errors, 0 unexpected warnings
- [ ] All existing Phase 24 headless tests still pass (37+)
- [ ] Render time < 1ms per frame for node/panel

---

## 7. Related Files

| File | Role |
|------|------|
| `Source/Editor/ConditionPreset/NodeConditionRef.h` | Condition data structure |
| `Source/Editor/ConditionPreset/DynamicDataPin.h` | Dynamic pin data structure |
| `Source/Editor/ConditionPreset/DynamicDataPinManager.h/.cpp` | Pin generation logic |
| `Source/Editor/Panels/NodeConditionsPanel.h/.cpp` | Properties panel condition UI |
| `Source/Editor/Nodes/NodeBranchRenderer.h/.cpp` | Canvas rendering |
| `Source/BlueprintEditor/VisualScriptEditorPanel.cpp` | Panel integration host |
| `Source/BlueprintEditor/VisualScriptNodeRenderer.cpp` | Canvas integration host |

---

*See also: [IMPLEMENTATION_STATUS.md](./IMPLEMENTATION_STATUS.md) | [QUALITY_STANDARDS.md](./QUALITY_STANDARDS.md)*
