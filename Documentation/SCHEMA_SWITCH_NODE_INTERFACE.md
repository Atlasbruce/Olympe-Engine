# Schema: Switch Node Interface & Properties Panel

## 1. SWITCH NODE ON CANVAS (BEFORE & AFTER)

### BEFORE (Current - Broken)
```
┌─────────────────────────────────┐
│ 🔴 Switch Node "Pick Action"    │  ← Node title (orange)
├─────────────────────────────────┤
│                                 │
│ In ●                         ● [+]  ← [+] button to add pins (unclear purpose)
│                                 │
│                          ● Case_0 Out
│                          ● Case_1 Out   ← Dynamic pins (from DynamicExecOutputPins)
│                          ● Case_2 Out
│                                 │
│                              ● [-]  ← [-] button to remove (dangerous, no confirmation)
│                                 │
└─────────────────────────────────┘

PROBLEMS:
❌ No pin labels (user doesn't know what Case_0, Case_1 mean)
❌ No indication of match values ("0", "10", "20")
❌ No custom labels (user wanted "Idle", "Attack", "Patrol")
❌ [+][-] buttons incomplete - users unsure what they do
❌ After modal edit: pins disappear (DynamicExecOutputPins not synced)
❌ After save/load: pins disappear (regeneration missing)
```

### AFTER (Unified - Fixed)
```
┌────────────────────────────────────────────┐
│ 🟦 Switch Node "Pick Action"               │  ← Node title (blue)
├────────────────────────────────────────────┤
│ switchVariable: "local:action"             │  ← Shows what variable controls switch
│                                            │
│ In ●                                       │
│                                            │
│         ● Case_0 [Default(0)]       Out   │  ← Base case with label shown
│         ● Case_1 [Idle(1)]          Out   │  ← Custom label + match value
│         ● Case_2 [Attack(10)]       Out   │  ← Semantic data visible
│         ● Case_3 [Patrol(20)]       Out   │
│                                            │
│                          ℹ️ Hover for details
│                                            │
└────────────────────────────────────────────┘

BENEFITS:
✅ Clear pin labels showing match values
✅ Custom user labels readable
✅ Visual indicator of semantic data
✅ No orphaned pins after edits
✅ Consistent after save/load/undo
✅ Single source of truth (switchCases)
```

---

## 2. PROPERTIES PANEL - SWITCH NODE SECTION (NEW)

### Layout: Top to Bottom

```
╔════════════════════════════════════════════════════════════╗
║                     NODE PROPERTIES                       ║
╠════════════════════════════════════════════════════════════╣
║                                                            ║
║  ┌──────────────────────────────────────────────────────┐ ║
║  │ 🔵 Switch Node "Pick Action"                    [X] │ ║  ← Blue header (matches Branch)
║  └──────────────────────────────────────────────────────┘ ║
║                                                            ║
║  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ║
║                                                            ║
║  Switch On:                                                ║
║  ┌──────────────────────────────────────────────────────┐ ║
║  │ local:action              [▼ Blackboard Variable]   │ ║  ← Dropdown to select variable
║  └──────────────────────────────────────────────────────┘ ║     (only Int/String vars shown)
║                                                            ║
║  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ║
║                                                            ║
║  Cases: 4                                                  ║
║                                                            ║
║  ┌─────────────────────────────────────────────────────┐  ║
║  │ ┌─ Edit Switch Cases ──────────────────────────┐  │  ║
║  │ │                                              │  │  ║
║  │ │  Opens modal to add/remove/rename cases     │  │  ║  ← PRIMARY INTERFACE
║  │ │  Shows case values, custom labels, UI       │  │  ║     (replaces confusing [+][-])
║  │ │                                              │  │  ║
║  │ └──────────────────────────────────────────────┘  │  ║
║  └─────────────────────────────────────────────────────┘  ║
║                                                            ║
║  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ║
║                                                            ║
║  ☐ Breakpoint (F9)                                         ║  ← Debug option (like Branch)
║                                                            ║
║  ⚠️ Validation Issues:                                     ║  ← Verification panel
║     • No switch variable assigned                          ║
║     • 2 cases have same match value (10)                   ║
║                                                            ║
║  ┌ Click error to navigate ┐                              ║
║  └─────────────────────────┘                              ║
║                                                            ║
╚════════════════════════════════════════════════════════════╝
```

---

## 3. SWITCH CASE EDITOR MODAL (DETAILED)

### Layout & Components

```
╔════════════════════════════════════════════════════════════════════════╗
║                      SWITCH CASE EDITOR                               ║
╠════════════════════════════════════════════════════════════════════════╣
║                                                                        ║
║  Edit Switch Cases                                                     ║
║  ─────────────────────────────────────────────────────────────────── ║
║                                                                        ║
║  ┌──────────────────────────────────────────────────────────────────┐ ║
║  │                                                                  │ ║
║  │  # │  Match Value  │  Custom Label        │  [^] [v] [X]       │ ║
║  │  ─────────────────────────────────────────────────────────────  │ ║
║  │  0 │ 0             │ Default              │  ─   [v] ─         │ ║  Row 0: Can't move up (first)
║  │    │               │                      │      (move down)    │ ║
║  │  ─────────────────────────────────────────────────────────────  │ ║
║  │  1 │ 1             │ Idle                 │  [^] [v] [X]       │ ║  Row 1: Can move/delete
║  │    │               │                      │                     │ ║
║  │  ─────────────────────────────────────────────────────────────  │ ║
║  │  2 │ 10            │ Attack               │  [^] [v] [X]       │ ║  Row 2: Can move/delete
║  │    │               │                      │                     │ ║
║  │  ─────────────────────────────────────────────────────────────  │ ║
║  │  3 │ 20            │ Patrol               │  [^] [v] [X]       │ ║  Row 3: Can move/delete
║  │    │               │                      │                     │ ║
║  │  ─────────────────────────────────────────────────────────────  │ ║
║  │                                                                  │ ║
║  │  [Scroll area - shows only first 5 rows, can scroll down]       │ ║
║  │                                                                  │ ║
║  └──────────────────────────────────────────────────────────────────┘ ║
║                                                                        ║
║  ┌─────────────────┐                                                  ║
║  │ + Add Case      │  ← Button to add new case                       ║
║  └─────────────────┘                                                  ║
║                                                                        ║
║  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ║
║                                                                        ║
║                                     ┌──────────┐  ┌────────┐         ║
║                                     │  Apply   │  │ Cancel │         ║
║                                     └──────────┘  └────────┘         ║
║                                                                        ║
╚════════════════════════════════════════════════════════════════════════╝

KEY FEATURES:
✅ Scrollable table (supports many cases)
✅ Three columns: Match Value | Custom Label | Actions
✅ Row 0 (Case_0) cannot move up (base case)
✅ [^] [v] buttons to reorder (except Row 0 up)
✅ [X] buttons to delete (except Row 0)
✅ + Add Case button to create new case
✅ Apply/Cancel at bottom
```

### What Happens on Actions

```
ACTION: User clicks "Add Case" button
├─ Modal creates new SwitchCaseDefinition:
│  ├─ value = "NewCase"
│  ├─ pinName = "Case_4"  (auto-generated)
│  └─ customLabel = ""
├─ Adds to m_editingCases
├─ New row appears in table
└─ User can edit value & label

ACTION: User clicks [^] on Row 2 (Move Up)
├─ Swaps Row 2 with Row 1 in m_editingCases
├─ Swaps value buffers
├─ Swaps label buffers
├─ Table refreshes
└─ User sees reordered rows

ACTION: User clicks [X] on Row 3 (Delete)
├─ Removes Row 3 from m_editingCases
├─ Removes from buffers
├─ Table refreshes
└─ User sees one fewer row

ACTION: User clicks "Apply"
├─ Modal marks IsConfirmed() = true
├─ Caller reads GetSwitchCases() → m_editingCases
├─ Caller updates:
│  ├─ template.Nodes[i].switchCases = newCases
│  ├─ template.Nodes[i].DynamicExecOutputPins = Extract pin names (FIX!)
│  └─ mark m_dirty = true (for save)
├─ Modal closes
└─ Canvas refreshes (pins now visible!)

ACTION: User clicks "Cancel"
├─ Modal closes without applying
├─ m_editingCases discarded (copy was made on Open)
├─ Graph unchanged
└─ User returns to canvas
```

---

## 4. DATA FLOW AFTER UNIFIED FIX

### User Creates New Switch Case (Via Modal)

```
BEFORE STATE:
┌─ TaskNodeDefinition ─────────────────────┐
│ switchCases = [                          │
│   {value:"0", pinName:"Case_0", ...}     │
│   {value:"1", pinName:"Case_1", ...}     │
│ ]                                        │
│ DynamicExecOutputPins = ["Case_1"]  ❌  │  (stale!)
└──────────────────────────────────────────┘

USER ACTION: 
1. Click "Edit Switch Cases" button
2. Modal opens with copy of switchCases
3. Click "+ Add Case"
4. Enter value "10", label "Attack"
5. Click "Apply"

FIXED CODE EXECUTES:
┌─────────────────────────────────────────┐
│ newCases = modal.GetSwitchCases()        │
│ // newCases now has 3 items              │
│                                          │
│ def.switchCases = newCases               │ ← Update 1
│ def.DynamicExecOutputPins.clear()        │
│ for each case in newCases[1..end]:       │
│   add case.pinName to DynamicExecOutputPins
│ // DynamicExecOutputPins = ["Case_1", "Case_2"]
│                                          │ ← Update 2
│ m_dirty = true                           │ ← Update 3
│ m_switchCaseModal->Close()               │ ← Update 4
└─────────────────────────────────────────┘

AFTER STATE:
┌─ TaskNodeDefinition ──────────────────────────┐
│ switchCases = [                               │
│   {value:"0", pinName:"Case_0", ...}          │
│   {value:"1", pinName:"Case_1", ...}          │
│   {value:"10", pinName:"Case_2", label:"Attack"} ← NEW
│ ]                                             │
│ DynamicExecOutputPins = ["Case_1", "Case_2"] │ ✅ SYNCED!
└──────────────────────────────────────────────┘

CANVAS UPDATES:
Old pins: Case_0, Case_1
New pins: Case_0, Case_1, Case_2 [Attack(10)] ← Shows custom label!
```

---

## 5. SAVE/LOAD CYCLE (AFTER FIX)

### Serialization Flow

```
SAVE TO FILE:
┌─────────────────────────────────────┐
│ template.Nodes[2] (Switch node)     │
├─────────────────────────────────────┤
│ switchCases = [                     │
│   {val:"0", pin:"Case_0", label:""} │
│   {val:"1", pin:"Case_1", label:"Idle"}
│   {val:"10", pin:"Case_2", label:"Attack"}
│ ]                                   │
│ DynamicExecOutputPins = [...] ❌   │  (NOT saved - derived!)
└─────────────────────────────────────┘
         ↓
    JSON File
   (switchCases only)

LOAD FROM FILE:
         ↓
  JSON File (switchCases only)
         ↓
┌─ TaskGraphLoader::ParseSchemaV4() ─┐
│ node.switchCases = loaded from JSON │
│ node.DynamicExecOutputPins = []     │  (empty on load)
│                                     │
│ ← NEW FIX: Regenerate pins ←        │
│ for each case in switchCases[1..end]:
│   add case.pinName to DynamicExecOutputPins
│                                     │
│ Result:                             │
│ DynamicExecOutputPins = ["Case_1", "Case_2"]
└─────────────────────────────────────┘

AFTER LOAD:
Canvas shows: Case_0, Case_1 [Idle], Case_2 [Attack(10)]
✅ ALL PINS VISIBLE (not lost!)
✅ LABELS PRESERVED
✅ MATCH VALUES PRESERVED
```

---

## 6. UNDO/REDO SCENARIO

### Command Stack Integration

```
STATE 1: Canvas loaded
switchCases = [{0}, {1}]
DynamicExecOutputPins = ["Case_1"]

USER ACTION: Edit Switch Cases → Add Case_2 [Attack(10)]
       ↓
STATE 2: After Apply
switchCases = [{0}, {1}, {10}]
DynamicExecOutputPins = ["Case_1", "Case_2"]
Canvas shows: Case_0, Case_1, Case_2 [Attack]

UNDO PRESSED (Ctrl+Z):
       ↓
STATE 1 RESTORED:
switchCases = [{0}, {1}]
DynamicExecOutputPins = ["Case_1"]
Canvas shows: Case_0, Case_1
✅ Both systems reverted together!

REDO PRESSED (Ctrl+Y):
       ↓
STATE 2 RESTORED:
switchCases = [{0}, {1}, {10}]
DynamicExecOutputPins = ["Case_1", "Case_2"]
Canvas shows: Case_0, Case_1, Case_2 [Attack]
✅ Consistent restoration!
```

---

## 7. BUTTON STATES & BEHAVIORS

### Properties Panel Buttons

```
┌─ SCENARIO: No switch variable selected ─┐
│                                         │
│ Switch On: (empty)    [⚠️ Warning]     │
│                                         │
│ "Edit Switch Cases"  ← ENABLED but    │  (User can edit cases
│                                        │   but validation warns)
│ ⚠️ Missing switch variable             │
│                                         │
└─────────────────────────────────────────┘

┌─ SCENARIO: Valid configuration ────────┐
│                                         │
│ Switch On: local:action    [✓ Valid]   │
│                                         │
│ "Edit Switch Cases"  ← ENABLED          │  (Normal state)
│                                         │
│ ✓ Configuration OK                      │
│                                         │
└─────────────────────────────────────────┘

┌─ SCENARIO: Modal open ─────────────────┐
│                                         │
│ [Modal overlays properties panel]       │
│                                         │
│ User edits: adds/removes/reorders cases │
│ Click Apply → Modal closes              │
│ Click Cancel → Modal closes (no changes)│
│                                         │
└─────────────────────────────────────────┘
```

---

## 8. CANVAS RENDERING: BEFORE vs AFTER

### Pin Name Display

```
BEFORE (Broken):
┌──────────────────────┐
│ Switch "Pick Action" │
├──────────────────────┤
│ In ●                 │
│     ● Case_0 Out     │  ← No indication what "0" is
│     ● Case_1 Out     │  ← No custom label visible
│     ● Case_2 Out     │
└──────────────────────┘

Pins don't show: values (0, 1, 10), labels (Idle, Attack, etc.)

---

AFTER (Fixed):
┌─────────────────────────────────────┐
│ Switch "Pick Action"                │
├─────────────────────────────────────┤
│ In ●                                │
│     ● Case_0 [Default(0)]     Out   │  ← Shows: label + value
│     ● Case_1 [Idle(1)]        Out   │  ← Semantic data visible
│     ● Case_2 [Attack(10)]     Out   │  ← User immediately knows purpose
└─────────────────────────────────────┘

Hover on pin → Shows full tooltip:
  "Case_1: Match value '1', label 'Idle'"
```

---

## 9. IMPLEMENTATION CHECKPOINTS

### After Step 1: RenderSwitchNodeProperties Added
```
✅ Properties panel shows blue header
✅ Shows "Switch On:" field
✅ Shows "Cases: N" count
✅ "Edit Switch Cases" button visible (but modal not integrated yet)
❌ Button click does nothing
```

### After Step 2: Modal Integration
```
✅ Click "Edit Switch Cases" → Modal opens
✅ Modal shows current cases
✅ User can add/remove/reorder
✅ Click "Apply" → Modal closes
✅ Click "Cancel" → Modal closes without changes
✅ switchCases updated (template.Nodes[i].switchCases = newCases)
✅ m_dirty = true (will save next)
❌ Canvas pins not updated yet (reload needed)
```

### After Step 3: Serialization Fix
```
✅ Load graph → switchCases loaded
✅ DynamicExecOutputPins regenerated from switchCases
✅ Canvas pins visible immediately
✅ Labels/values preserved
❌ [+][-] buttons still bypass modal
```

### After Step 4: Canvas Callbacks Fixed
```
✅ [+] button on Switch → Opens modal (safer)
✅ [-] button on Switch → Opens modal (confirmation)
✅ [+] button on VSSequence → Direct add (unchanged)
✅ [+] button on Switch after fix → Modal workflow
❌ Minor edge cases
```

### After All 7 Steps Complete ✅
```
✅ Create Switch node → Shows base case
✅ Edit via modal → All cases visible
✅ Save/load → Cases preserved
✅ Undo/redo → Consistent
✅ [+][-] buttons work safely
✅ No data loss
✅ Labels/values always synced
✅ Runtime execution correct
```

---

## 10. ERROR SCENARIOS & HANDLING

### Edge Case: Duplicate Match Values

```
Modal shows warning:
⚠️ Duplicate values detected:
   - Case_1 and Case_3 both have value "10"
   - Only first match will be used at runtime

User can:
✓ Fix by editing one of the values
✓ Continue (accepted risk)
✗ Cannot save with validation error (Phase 21-B verification)
```

### Edge Case: Empty Match Value

```
Modal prevents:
If user tries to leave "Match Value" empty:
  "Match value cannot be empty"

User must either:
✓ Enter a valid value
✓ Delete the row [X]
```

### Edge Case: Very Long Custom Label

```
Pin display truncates:
Pin name: "Case_5 [This is a very long label...]"
                        ↑ Ellipsis on overflow

Hover shows full: "This is a very long label that describes the action"
```

---

END SCHEMA DOCUMENT
