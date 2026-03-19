# PHASE 24 - Condition Preset System
## Final Design Specification (March 18, 2026)

**Document Version:** 1.0  
**Status:** READY FOR IMPLEMENTATION  
**Last Updated:** 2026-03-18  

---

## 📋 TABLE OF CONTENTS

1. [Executive Summary](#executive-summary)
2. [Architecture Overview](#architecture-overview)
3. [Panel A: Branch Node Properties](#panel-a-branch-node-properties)
4. [Panel B: Condition Preset Bank](#panel-b-condition-preset-bank)
5. [Panel C: Local Variables Reference](#panel-c-local-variables-reference)
6. [Branch Node Canvas Rendering](#branch-node-canvas-rendering)
7. [Dynamic Pin-In Naming & Display](#dynamic-pin-in-naming--display)
8. [Three Variable Types](#three-variable-types)
9. [User Workflows](#user-workflows)
10. [Technical Implementation Details](#technical-implementation-details)

---

## EXECUTIVE SUMMARY

The Condition Preset System provides a unified interface for managing reusable condition expressions in Visual Script graphs. Users can:

1. **Create condition presets** in a global bank (reusable across multiple Branch nodes)
2. **Assign presets to Branch nodes** via a properties panel
3. **Configure operands** (Variable/Const/Pin) with auto-generated dynamic pins
4. **Control preset changes** that cascade to all dependent Branch nodes

**Key Innovation:** Pin-mode operands auto-generate named data-input pins on the node, making condition logic visible and connectable in the graph canvas.

---

## ARCHITECTURE OVERVIEW

### Three Coexisting Panels

```
┌────────────────────────────────────────────────────────────┐
│  RIGHT SIDE PANEL (Resizable & Scrollable)                │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  PART A: BRANCH NODE PROPERTIES (When node selected)      │
│  ┌──────────────────────────────────────────────────────┐ │
│  │ Title: Branch node name                              │ │
│  │ Conditions: List of assigned presets                 │ │
│  │ [+ Add Condition]                                    │ │
│  │ [Breakpoint (F9)]                                    │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                            │
│  ▓▓▓ RESIZABLE SPLITTER ▓▓▓ (draggable divider)          │
│                                                            │
│  PART B: CONDITION PRESET BANK (Global & Editable)       │
│  ┌──────────────────────────────────────────────────────┐ │
│  │ [+ Add Condition Preset]                             │ │
│  │ [Expandable list of condition presets]               │ │
│  │ [When expanded: 3 editable field controls]           │ │
│  │ [Scrollable if many presets]                         │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                            │
│  ▓▓▓ RESIZABLE SPLITTER ▓▓▓ (draggable divider)          │
│                                                            │
│  PART C: LOCAL VARIABLES REFERENCE (Read-only)           │
│  ┌──────────────────────────────────────────────────────┐ │
│  │ [List of blackboard variables]                       │ │
│  │ [Scrollable]                                         │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                            │
└────────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────────┐
│  TOOLBAR BUTTON                                            │
├────────────────────────────────────────────────────────────┤
│ [Condition Presets] → Opens independent window with full │
│                       Condition Preset Bank UI            │
└────────────────────────────────────────────────────────────┘
```

### Coexistence of Panels

| Panel | Location | Type | Visibility |
|-------|----------|------|------------|
| **Part A: Node Properties** | Right panel, Top | Integrated | When Branch node selected |
| **Part B: Preset Bank (in Properties)** | Right panel, Middle | Integrated | Always visible (when Part A visible) |
| **Part C: Local Variables** | Right panel, Bottom | Integrated | Always visible |
| **Preset Bank (Independent)** | Separate window | Modal/Dockable | Via toolbar button |

Both versions of Preset Bank use the **same data model** and stay synchronized.

---

## PANEL A: BRANCH NODE PROPERTIES

### Purpose
Display and manage the conditions assigned to the currently selected Branch node.

### Layout

```
┌──────────────────────────────────────────┐
│ Branch                        Name       │  ← Header (blue background)
├──────────────────────────────────────────┤
│ (no conditions)                          │  ← If empty
│                                          │
│ Condition #1 [mHealth]<=[2]      [X]   │  ← Condition row #1
│ And  Condition #3 [mSpeed]>=[100] [X]  │  ← Condition row #2 with operator
│ Or   Condition #5 [mSpeed]==[Pin:1] [X]│  ← Condition row #3 with Pin
│                                          │
│ [+ Add Condition]                        │  ← Add button (full width)
│                                          │
│ ☑ Breakpoint (F9)                       │  ← Checkbox
└──────────────────────────────────────────┘
```

### Components

#### Header
- **Title:** Node name (e.g., "Branch", "Is Health Critical?")
- **Name field:** Display only (references node definition)
- **Background:** Blue color (matches node canvas title)

#### Conditions List
Each condition line contains:

| Component | When | Format |
|-----------|------|--------|
| **Operator Dropdown** | Row 2+ | "And" / "Or" dropdown |
| **Preset Dropdown** | All rows | Displays all available presets with live filter |
| **Delete Button** | All rows | "[X]" button to remove from node |

**Preset Dropdown Display Format:**
```
[Show preview of condition]
├─ Condition #1 [mHealth] <= [2]
├─ Condition #2 [AI_Mode] == [3]
├─ Condition #3 [mSpeed] >= [100.00]
├─ Condition #4 [Pin:1] != [Pin:2]
└─ Condition #5 [mSpeed] == [Pin:1]
```

**Features:**
- Live text filter (search by condition name or preview)
- Display preset ID and full syntax
- Show Pin notation when applicable

#### Add Condition Button
- **Label:** "[+ Add Condition]"
- **Width:** Full width of panel
- **Behavior:** Adds new empty row to conditions list + dropdown opens
- **Result:** New condition line with operator selector visible (default="And")

#### Breakpoint Checkbox
- **Label:** "Breakpoint (F9)"
- **State:** Reflects DebugController::HasBreakpoint()
- **Action:** Toggle sets breakpoint on node

### Data Binding

**Source:** `TaskNodeDefinition::conditionRefs` (vector of `NodeConditionRef`)

```cpp
struct NodeConditionRef {
    std::string presetID;      // Links to ConditionPreset::id
    LogicalOp logicalOp;       // And / Or (only used for row 2+)
};
```

**Synchronization:**
- Changes immediately update `eNode.def.conditionRefs`
- `OnDynamicPinsNeedRegeneration` callback fires
- DynamicDataPinManager regenerates pins
- Node canvas re-renders with new pins

---

## PANEL B: CONDITION PRESET BANK

### Purpose
Central repository for condition preset definitions. All Branch nodes reference presets from this bank. Changes cascade to all dependent nodes.

### Location & Availability
- **Integrated:** In NodeConditionsPanel (below Part A) - ALWAYS visible
- **Independent:** Via toolbar "Condition Presets" button - separate window

**Both sync automatically.**

### Layout

```
┌──────────────────────────────────────────────────────────┐
│ Condition Preset        [+ Add Condition Preset]         │  ← Header + Add button
├──────────────────────────────────────────────────────────┤
│                                                          │
│ ▶ Condition #1 [mHealth] <= [2]               [X][Dup] │  ← Collapsed preset
│                                                          │
│ ▼ Condition #3 [mSpeed] >= [100.00]           [X][Dup] │  ← Expanded preset
│ │ mSpeed         ▼  >=         ▼  Const.  ▼           │
│ │                              100.000 [-][+]          │
│ │                                                       │
│ ▼ Condition #4 [Pin:1] != [Pin:2]            [X][Dup] │  ← Expanded with Pin mode
│ │ Pin-in       ▼  !=         ▼  Pin-in  ▼             │
│ │ Pin-in #1           Pin-in #2                        │
│ │ [Enabled on Node]   [Enabled on Node]                │
│ │                                                       │
│ ▼ Condition #5 [mSpeed] == [Pin:1]           [X][Dup] │  ← Expanded mixed mode
│ │ mSpeed       ▼  ==         ▼  Pin-in  ▼             │
│ │                           Pin-in #1                  │
│ │                           [Enabled on Node]          │
│ │                                                       │
└──────────────────────────────────────────────────────────┘
```

### Components

#### Header
- **Label:** "Condition Preset"
- **Button:** "[+ Add Condition Preset]"
  - Creates new preset with defaults:
    - Name: "Condition #N" (auto-increment)
    - VAR1: Empty (default to first local variable)
    - Operator: "=="
    - VAR2: "0" (const)

#### Preset List Items (Collapsed)
```
▶ Condition #1 [mHealth] <= [2]               [X][Dup]
```

**Elements:**
- **Toggle:** `▶` (collapsed) / `▼` (expanded)
- **Preview:** Full condition syntax (auto-generated)
- **Delete:** `[X]` button
- **Duplicate:** `[Dup]` button (creates copy with ID+1)

**Click Behavior:**
- Toggle = expand/collapse
- Preview text = select preset

#### Preset List Items (Expanded)

```
▼ Condition #3 [mSpeed] >= [100.00]           [X][Dup]
│ mSpeed         ▼  >=         ▼  Const.  ▼
│                              100.000 [-][+]
│
```

**Three Editable Fields:**

##### Field 1: VAR1 Type & Value

```
mSpeed         ▼  (dropdown)
```

**Dropdown Options:**
```
━━━━━━━━━━━━━━━━━━━━━━━━
[ [Pin-in] ]       ← Pin mode
[ [Const] ]        ← Const mode
───────────────────
AI_Mode            ← Local Variables
Position
mHealth
EntitID
mSpeed
Acceleration
HasTarget
mVelocity
... (all blackboard variables)
```

**Display:**
- **If Local Variable:** Shows variable name (e.g., "mSpeed")
- **If Const:** Shows "[Const]"
- **If Pin:** Shows "[Pin-in]"

##### Field 2: Operator

```
>=         ▼  (dropdown)
```

**Options:**
- `==` (equal)
- `!=` (not equal)
- `<` (less than)
- `<=` (less or equal)
- `>` (greater than)
- `>=` (greater or equal)

##### Field 3: VAR2 Type & Value

**If VAR2 mode = "Const":**
```
Const.  ▼
100.000  (numeric field)
[-][+]   (increment/decrement buttons)
```

**If VAR2 mode = "Local Variable":**
```
mSpeed  ▼  (dropdown same as Field 1)
```

**If VAR2 mode = "Pin-in":**
```
Pin-in  ▼
Pin-in #1
[Enabled on Node]  (read-only label)
```

---

## PANEL C: LOCAL VARIABLES REFERENCE

### Purpose
Display available blackboard variables for reference when configuring condition operands.

### Features
- **Read-only list** of all blackboard entries
- **Scrollable** (if many variables)
- **Shows:** Variable name, type, current value
- **Updates:** When graph is loaded or variables change

### Display Format

```
┌──────────────────────────────────────────┐
│ Local Blackboard                         │
├──────────────────────────────────────────┤
│ Graph Verification                       │
│ Click 'Verify' in toolbar to run.        │
│                                          │
│ Local Blackboard                         │
│ ☑ Add key                                │
│                                          │
│ AI_Mode              Default: [default]  │
│ Position             Vector   G   x      │
│ mHealth              Int      G   x      │
│ EntitID              EntityID G   x      │
│ mSpeed               Float    G   x      │
│ Acceleration         Vector   G   x      │
│ HasTarget            Bool     G   x      │
│ mVelocity            Float    G   x      │
└──────────────────────────────────────────┘
```

---

## BRANCH NODE CANVAS RENDERING

### Final Node Appearance

```
┌─────────────────────────────────────────┐
│   Is Health Critical?            (BLUE) │  ← Section 1: Title
├─────────────────────────────────────────┤
│In                                Then   │  ← Section 2: Exec Pins
│                                  Else   │     (Connectable triangles)
├─────────────────────────────────────────┤
│ [mHealth] <= [2]                (GREEN) │  ← Section 3: Conditions Preview
│And [mSpeed] >= [100.00]                 │     (Read-only, green text)
│Or  [mSpeed] == [Pin:1]                  │
├─────────────────────────────────────────┤
│● In #1: [mSpeed] == [Pin :1]   (YELLOW)│  ← Section 4: Dynamic Data Pins
│  (Connectable circle)                   │     (Only if Pin-mode operands)
└─────────────────────────────────────────┘
```

### Section Details

#### Section 1: Title Bar (Blue)
- **Color:** ImNodes::BeginNodeTitleBar() styling
- **Text:** Node name (e.g., "Is Health Critical?")
- **Breakpoint indicator:** "[BP]" label if breakpoint set

#### Section 2: Exec Pins (Connectable)
- **Left pin:** "In" (exec input, triangle shape)
- **Right pins:** "Then", "Else" (exec outputs, triangle shapes)
- **All connectable** via ImNodes attribute system

#### Section 3: Conditions Preview (Read-only, Green)
- **Display format:** "[VAR1] OPERATOR [VAR2]"
- **Examples:**
  ```
  [mHealth] <= [2]
  And [mSpeed] >= [100.00]
  Or  [mSpeed] == [Pin :1]
  ```
- **Logical operators:** "And", "Or" displayed before line 2+
- **Color:** Green text (RGB: 0, 1.0, 0)
- **Not interactive:** Read-only display

#### Section 4: Dynamic Data Pins (Yellow, Connectable)
- **Only rendered** if any Pin-mode operands exist
- **One pin per Pin-mode operand** in the conditions
- **Display format:** "● In #N: [condition syntax with Pin highlighted]"
- **Color:** Yellow circle (connectable data-in pin)

---

## DYNAMIC PIN-IN NAMING & DISPLAY

### Pin Naming Convention

Each Pin-mode operand generates a unique dynamic pin with auto-incremented index.

### Examples

#### Scenario 1: Single Pin in VAR1
```
Condition: [mHealth] <= [Pin:1]
           ^^^^^^^^      ^^^^^^
           VAR1(Local)   VAR2(Pin)

Generated Pin: In #1: [mHealth] <= [Pin :1]
                                   (highlight)
```

**Highlighting:** Underline `[Pin :1]` in yellow

#### Scenario 2: Single Pin in VAR2 Only
```
Condition: [mSpeed] >= [Pin:1]
           ^^^^^^      ^^^^^^
           VAR1(Local) VAR2(Pin)

Generated Pin: In #1: [mSpeed] >= [Pin :1]
                                  (highlight)
```

**Highlighting:** Underline `[Pin :1]` in yellow

#### Scenario 3: Both Operands as Pin
```
Condition: [Pin:1] != [Pin:2]
           ^^^^^^    ^^^^^^
           VAR1(Pin) VAR2(Pin)

Generated Pins:
  In #1: [Pin :1] != [Pin :2]
         (highlight)
  
  In #2: [Pin :1] != [Pin :2]
                     (highlight)
```

**Highlighting:**
- Pin #1 line: Underline `[Pin :1]` only
- Pin #2 line: Underline `[Pin :2]` only

### Highlighting Implementation

**Option A (Selected): Underline + Yellow Color**
```
In #1: [mSpeed] == <u style="color:yellow">[Pin:1]</u>
```

**CSS/ImGui Implementation:**
- Use `ImGui::TextColored()` for the Pin reference text
- Apply color: `ImVec4(1.0f, 1.0f, 0.0f, 1.0f)` (yellow)
- Add underline style via separate text rendering

**Fallback (if unavailable):** Use different color or italic style

### Counter Logic

**Pin numbering increments globally** across all Pin-mode operands in a condition:

```
Condition #1: [Pin:1] == [Pin:2] AND [Pin:3] >= [mHealth]
              VAR1(Pin)  VAR2(Pin)    VAR1(Pin)  VAR2(Local)

Generated:
  In #1: [Pin:1] == [Pin:2] AND [Pin:3] >= [mHealth]  (highlight Pin:1)
  In #2: [Pin:1] == [Pin:2] AND [Pin:3] >= [mHealth]  (highlight Pin:2)
  In #3: [Pin:1] == [Pin:2] AND [Pin:3] >= [mHealth]  (highlight Pin:3)
```

---

## THREE VARIABLE TYPES

### Type 1: Local Variable

**Source:** Blackboard variables (left panel list)

**Definition:**
```cpp
enum class VariableType {
    Bool, Int, Float, Vector, EntityID, String, None
};

struct BlackboardEntry {
    std::string name;           // e.g., "mHealth"
    VariableType type;          // e.g., Int
    TaskValue defaultValue;     // Runtime value
};
```

**In Preset:** Displays variable name in dropdown
```
mSpeed      (dropdown shows all available)
```

**At Runtime:** Value fetched from Blackboard at execution time

**Example:**
```
Condition: [mHealth] <= [100]
If mHealth = 50 → True
If mHealth = 150 → False
```

### Type 2: Const

**Source:** Numeric literal value

**Definition:**
```cpp
struct Operand {
    OperandMode mode;           // Const, Variable, Pin
    TaskValue constValue;       // For Const mode: float/int/etc
};
```

**In Preset:** Numeric input field with [-]/[+] buttons
```
100.000  [-][+]
```

**At Runtime:** Literal value used directly

**Type Casting:** All consts interpreted as `float` for comparison

**Example:**
```
Condition: [mHealth] <= [100]
If mHealth = 50 → True
If mHealth = 150 → False
```

### Type 3: Pin-in (Data Input Pin)

**Source:** External node output connected via graph edge

**Definition:**
```cpp
struct DynamicDataPin {
    std::string id;             // "Pin:1", "Pin:2", etc
    int nodeID;                 // Which node this pin belongs to
    OperandPosition position;   // VAR1 / VAR2 in condition
    std::string displayLabel;   // "In #1: ..."
};
```

**In Preset:** Auto-labeled as Pin-in
```
Pin-in  (dropdown)
Pin-in #1
[Enabled on Node]  (read-only)
```

**Pin Naming Rules:**
- **First Pin-mode operand:** `Pin:1` (index in global Pin list)
- **Second Pin-mode operand:** `Pin:2`
- **Nth Pin-mode operand:** `Pin:N`

**At Runtime:** Value comes from connected upstream node

**Example:**
```
Condition: [mSpeed] == [Pin:1]

Runtime:
  If upstream connects value 100 → [Pin:1] = 100
  If [mSpeed] = 100 → True
  If [mSpeed] = 50 → False
```

**Pin Connection Labels:**

On the node canvas, each Pin-in shows:
```
● In #1: [mSpeed] == [Pin :1]
```

This label tells the user:
- Which condition uses this pin
- What role it plays (VAR1 vs VAR2)
- Full condition syntax for context

---

## USER WORKFLOWS

### Workflow 1: Create a Reusable Condition Preset

**Steps:**

1. **Create Preset:**
   - Click `[+ Add Condition Preset]` in bank
   - New row appears: "Condition #N [---] [---] [---]"

2. **Configure Operands:**
   - Click VAR1 dropdown → Select variable (e.g., "mHealth")
   - Click Operator → Select "<=
   - Click VAR2 dropdown → Select "Const"
   - Enter value: "100"

3. **Preview Updates:**
   - Live preview: "Condition #N [mHealth] <= [100]"

4. **Save:**
   - (Auto-saved to registry)
   - Available in all Branch nodes

### Workflow 2: Assign Preset to Branch Node

**Steps:**

1. **Select Branch Node:**
   - Click node in canvas
   - Part A (Node Properties) appears

2. **Add Condition:**
   - Click `[+ Add Condition]`
   - New row: "And [---] [dropdown]"

3. **Select Preset:**
   - Click dropdown
   - Search/select: "Condition #1 [mHealth] <= [100]"

4. **Confirm:**
   - Condition added to node
   - (If no Pin-mode operands: no dynamic pins)

5. **Apply:**
   - Click `[Apply]` button (if modal)
   - Node updates with condition

### Workflow 3: Use Pin-mode Operand (Advanced)

**Steps:**

1. **In Preset Bank:** Expand a condition
   - Click VAR2 dropdown → Select "[Pin-in]"
   - Display changes: Shows "Pin-in #1" with label "[Enabled on Node]"

2. **In Node Properties:**
   - Add this preset to a Branch node
   - Dynamic pin appears: "● In #1: [mHealth] == [Pin :1]"

3. **Connect Pin:**
   - In canvas, connect upstream node output → Dynamic pin "In #1"
   - Data flows from upstream → Pin value
   - Condition evaluated at runtime

4. **Result:**
   - Node shows connected pin with yellow color
   - Syntax clearly indicates which part uses Pin

### Workflow 4: Edit Preset (Cascading Change)

**Steps:**

1. **Select Preset in Bank:**
   - Expand: "Condition #1 [mHealth] <= [100]"

2. **Modify:**
   - Change VAR1: "mHealth" → "mFatigue"
   - Or change value: "100" → "50"

3. **Preview Updates:**
   - Live: "Condition #1 [mFatigue] <= [50]"

4. **Cascade:**
   - ALL nodes using "Condition #1" update immediately
   - Node canvas re-renders
   - Dynamic pins updated if needed

### Workflow 5: Delete Preset

**Steps:**

1. **In Preset Bank:**
   - Click `[X]` button next to preset

2. **Confirmation:**
   - Dialog: "Delete 'Condition #1'? Used by 3 nodes"

3. **Confirm:**
   - Preset deleted from registry
   - All references removed from nodes
   - Dynamic pins cleaned up

---

## TECHNICAL IMPLEMENTATION DETAILS

### Data Flow Architecture

```
ConditionPresetRegistry (Global)
├─ id → ConditionPreset
│  ├─ name
│  ├─ left (Operand)
│  ├─ op (ComparisonOp)
│  ├─ right (Operand)
│  └─ GetPreview() → "[ ] op [ ]"
│
└─ Save/Load from JSON
   └─ Blueprints/Presets/condition_presets.json

TaskNodeDefinition (Per Node)
├─ conditionRefs: vector<NodeConditionRef>
│  ├─ presetID → links to ConditionPreset
│  └─ logicalOp (And/Or)
│
└─ dynamicPins: vector<DynamicDataPin>
   ├─ id ("Pin:1", "Pin:2", ...)
   ├─ displayLabel ("In #1: ...")
   └─ Used by renderer

DynamicDataPinManager
├─ RegeneratePinsFromConditions()
│  ├─ Iterate conditions
│  ├─ Detect Pin-mode operands
│  └─ Create pins with proper IDs
│
└─ GetAllPins() → all dynamic pins for node

NodeConditionsPanel (UI)
├─ Render()
│  ├─ Part A: Node Properties
│  ├─ Part B: Preset Bank (integrated)
│  └─ Part C: Local Variables
│
└─ Callbacks → DynamicPinsNeedRegeneration

ConditionPresetLibraryPanel (UI - Independent)
├─ Render() (separate window)
└─ Data syncs with Part B
```

### Key Data Structures

#### Operand

```cpp
enum class OperandMode { Variable, Const, Pin };

struct Operand {
    OperandMode mode;
    std::string variableRef;    // For Variable mode
    float constValue;           // For Const mode
    int pinIndex;               // For Pin mode (0, 1, ...)
    
    static Operand CreateVariable(const std::string& varName);
    static Operand CreateConst(float value);
    static Operand CreatePin(int pinIdx);
};
```

#### DynamicDataPin

```cpp
struct DynamicDataPin {
    std::string id;             // "Pin:1", "Pin:2", ...
    int nodeID;                 // Owner node
    OperandPosition position;   // VAR1 or VAR2
    std::string displayLabel;   // "In #1: [mHealth] == [Pin :1]"
    
    // Helper methods
    std::string GetDisplayLabel() const;
    std::string GetShortLabel() const;   // "Pin:1"
};
```

#### NodeConditionRef

```cpp
enum class LogicalOp { And, Or };

struct NodeConditionRef {
    std::string presetID;       // Links to ConditionPreset::id
    LogicalOp logicalOp;        // And / Or (for row 2+)
};
```

### Pin ID Generation Algorithm

```
ALGORITHM: GeneratePinIDsForConditions
INPUT: vector<NodeConditionRef> conditions, Registry
OUTPUT: vector<DynamicDataPin> pins

counter = 1
FOR each condition IN conditions:
    preset = registry.GetPreset(condition.presetID)
    
    IF preset.left.mode == Pin:
        pin = new DynamicDataPin()
        pin.id = "Pin:" + counter
        pin.displayLabel = "In #" + counter + ": " + 
                          preset.GetPreview() + "(highlight Pin:X)"
        pins.add(pin)
        counter++
    
    IF preset.right.mode == Pin:
        pin = new DynamicDataPin()
        pin.id = "Pin:" + counter
        pin.displayLabel = "In #" + counter + ": " + 
                          preset.GetPreview() + "(highlight Pin:X)"
        pins.add(pin)
        counter++

RETURN pins
```

### Highlighting Algorithm

For each Pin-mode operand in condition preview:

1. **Parse condition syntax:** "[VAR1] OP [VAR2]"
2. **Identify Pin references:** "Pin:1", "Pin:2", etc
3. **For each Pin reference in line:**
   - Render text color YELLOW
   - Apply underline style
4. **Display:** "In #1: [mSpeed] == <u style='yellow'>[Pin:1]</u>"

**Implementation (ImGui):**

```cpp
// Render condition text with highlighting
ImGui::Text("In #1: [mSpeed] == ");
ImGui::SameLine();
ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[Pin:1]");  // Yellow

// Alternative: Use formatted string
std::string displayText = FormatWithPinHighlight(condition);
ImGui::TextUnformatted(displayText.c_str());
```

### Cascade Update Mechanism

**When preset is modified:**

1. ConditionPresetLibraryPanel detects edit
2. Registry.UpdatePreset(presetID, newData)
3. Registry emits event: OnPresetChanged(presetID)
4. All Branch nodes with this preset:
   - Update condition preview (if displayed)
   - Call DynamicDataPinManager.RegeneratePins()
   - Update dynamic pin list
   - NodeBranchRenderer re-renders on next frame

**Implementation:**

```cpp
// In ConditionPresetRegistry
std::function<void(const std::string&)> OnPresetChanged;

// In NodeBranchRenderer
m_registry.OnPresetChanged = [this](const std::string& presetID) {
    m_refreshPending = true;  // Trigger re-render
};
```

---

## IMPLEMENTATION CHECKLIST

### Phase 1: Data Structure Updates
- [ ] Update `DynamicDataPin` struct to store position (VAR1/VAR2)
- [ ] Update `NodeConditionRef` to include logical operator
- [ ] Verify `Operand` enum includes Pin mode
- [ ] Test Registry CRUD operations

### Phase 2: Panel Integration
- [ ] Merge `ConditionPresetLibraryPanel` into `NodeConditionsPanel`
- [ ] Implement resizable splitter between Part A and Part B
- [ ] Add vertical scrolling to each section
- [ ] Implement Part C (Local Variables reference)

### Phase 3: UI Controls
- [ ] Add expandable toggles to presets
- [ ] Implement 3-field editing (VAR1, Op, VAR2)
- [ ] Add dropdown filtering for presets
- [ ] Implement add/delete/duplicate preset buttons

### Phase 4: Pin Generation
- [ ] Implement GeneratePinIDsForConditions() algorithm
- [ ] Update DynamicDataPinManager.RegeneratePins()
- [ ] Test pin numbering across multiple conditions
- [ ] Verify pin labels generate correctly

### Phase 5: Rendering
- [ ] Update NodeBranchRenderer section 3 (conditions preview)
- [ ] Implement highlighting for Pin references
- [ ] Test yellow underline styling
- [ ] Add section 4 (dynamic pins) rendering

### Phase 6: Testing & Polish
- [ ] End-to-end workflow testing
- [ ] Cascade update verification
- [ ] Pin connection verification
- [ ] Save/load roundtrip test

---

## NOTES & FUTURE ENHANCEMENTS

### Current Limitations
- Pin numbering is local to each condition (no global Pin:1, Pin:2, ... across multiple conditions yet)
- No undo/redo for preset edits (future enhancement)
- No preset grouping/categories (future enhancement)

### Future Enhancements
1. **Preset Categories:** Group presets by type (e.g., "Health checks", "Status checks")
2. **Preset Versioning:** Track changes to presets over time
3. **Preset Export/Import:** Share presets between projects
4. **Advanced Operators:** Add bitwise/logical operators
5. **Array/Collection Support:** Iterate over collections in conditions
6. **Preset Validation:** Warn on unused or circular references

---

**END OF SPECIFICATION**

---

**Document Prepared By:** AI Programming Assistant  
**Date:** March 18, 2026  
**Revision:** 1.0  
**Status:** Ready for Implementation
