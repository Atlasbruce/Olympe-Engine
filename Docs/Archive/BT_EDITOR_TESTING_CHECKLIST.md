# BT Editor Phase 2 - Testing Checklist

## Pre-Test Setup

- [ ] Build project (Windows Visual Studio or CMake)
- [ ] Launch Olympe Engine
- [ ] Load a level with AI entities
- [ ] Press F10 to open BT Debugger
- [ ] Select an entity with a behavior tree

---

## Basic Editor Mode

### Toggle Editor Mode
- [ ] **Test**: Check "Editor Mode" checkbox
- [ ] **Expected**: Toolbar appears with buttons: Add Node, Save, Undo, Redo
- [ ] **Expected**: Status shows "[Unmodified]" in gray
- [ ] **Expected**: Console shows: `[BTEditor] Entered editor mode, editing tree: [TreeName]`

### Exit Editor Mode
- [ ] **Test**: Uncheck "Editor Mode" checkbox
- [ ] **Expected**: Toolbar disappears
- [ ] **Expected**: Tree returns to debugger view
- [ ] **Expected**: Execution highlighting works (if entity is active)

---

## Node Creation

### Right-Click Context Menu
- [ ] **Test**: Right-click in empty graph area
- [ ] **Expected**: Popup menu appears with 6 options:
  - Selector
  - Sequence
  - Condition
  - Action
  - Inverter
  - Repeater

### Create Each Node Type
- [ ] **Test**: Create a Selector node
  - [ ] Node appears at cursor position
  - [ ] Node has blue title bar (color: `#6496FF`)
  - [ ] Node shows icon: `?`
  - [ ] Node has "Out" pin (bottom/right)
  - [ ] Console: `[BTEditor] Created node: New Selector (ID: 1000)`

- [ ] **Test**: Create a Sequence node
  - [ ] Node has green title bar (color: `#64FF96`)
  - [ ] Node shows icon: `->`
  - [ ] Node has "Out" pin

- [ ] **Test**: Create a Condition node
  - [ ] Node has orange title bar (color: `#FFC864`)
  - [ ] Node shows icon: `◆`
  - [ ] Node has "In" pin (top/left) only
  - [ ] No "Out" pin

- [ ] **Test**: Create an Action node
  - [ ] Node has rose title bar (color: `#FF6496`)
  - [ ] Node shows icon: `►`
  - [ ] Node has "In" pin only
  - [ ] No "Out" pin

- [ ] **Test**: Create an Inverter node
  - [ ] Node has purple title bar (color: `#C864FF`)
  - [ ] Node shows icon: `!`
  - [ ] Node has both "In" and "Out" pins

- [ ] **Test**: Create a Repeater node
  - [ ] Node has light blue title bar (color: `#9696FF`)
  - [ ] Node shows icon: `↻`
  - [ ] Node has both "In" and "Out" pins

### Verify Node IDs
- [ ] **Test**: Create 3 nodes sequentially
- [ ] **Expected**: IDs increment: 1000, 1001, 1002
- [ ] **Expected**: Status changes to "[Modified]" in yellow/orange

---

## Node Selection

### Single Selection
- [ ] **Test**: Click on a node
- [ ] **Expected**: Node shows selection outline
- [ ] **Expected**: Toolbar shows "Selected: 1"

### Multiple Selection
- [ ] **Test**: Hold Ctrl and click 3 different nodes
- [ ] **Expected**: All 3 nodes show selection outline
- [ ] **Expected**: Toolbar shows "Selected: 3"

### Deselection
- [ ] **Test**: Click on empty space
- [ ] **Expected**: All nodes deselect
- [ ] **Expected**: Toolbar shows "Selected: 0"

---

## Connection Creation

### Valid Connection (Composite → Leaf)
- [ ] **Test**: Create Sequence and Action nodes
- [ ] **Test**: Drag from Sequence "Out" to Action "In"
- [ ] **Expected**: Connection appears as a line/curve
- [ ] **Expected**: Console: `[BTEditor] Connection created: [parent] -> [child]`
- [ ] **Expected**: Status shows "[Modified]"

### Valid Connection (Decorator → Leaf)
- [ ] **Test**: Create Inverter and Condition nodes
- [ ] **Test**: Drag from Inverter "Out" to Condition "In"
- [ ] **Expected**: Connection created successfully

### Invalid Connection (Leaf → Any)
- [ ] **Test**: Try to drag from Action "In" pin
- [ ] **Expected**: Cannot drag (Actions have no "Out" pin)

### Invalid Connection (Self)
- [ ] **Test**: Try to connect a node to itself
- [ ] **Expected**: Console: `[BTEditor] Invalid connection: [id] -> [id]`
- [ ] **Expected**: Connection not created

### Invalid Connection (Duplicate)
- [ ] **Test**: Create connection A→B
- [ ] **Test**: Try to create same connection A→B again
- [ ] **Expected**: Console: `[BTEditor] Invalid connection`
- [ ] **Expected**: Second connection blocked

### Invalid Connection (Cycle)
- [ ] **Test**: Create chain A→B→C
- [ ] **Test**: Try to create connection C→A
- [ ] **Expected**: Console: `[BTEditor] Invalid connection`
- [ ] **Expected**: Cycle prevented

### Decorator Single Child Limit
- [ ] **Test**: Connect Inverter to Action1
- [ ] **Test**: Try to connect same Inverter to Action2
- [ ] **Expected**: Console: `[BTEditor] Invalid connection`
- [ ] **Expected**: Second child blocked

---

## Connection Deletion

### Delete via Selection
- [ ] **Test**: Create a connection
- [ ] **Test**: Click on the connection line to select it
- [ ] **Test**: Press Delete key
- [ ] **Expected**: Connection disappears
- [ ] **Expected**: Console: `[BTEditor] Connection deleted: [parent] -> [child]`

### Delete via Right-Click (if supported by ImNodes)
- [ ] **Test**: Right-click on connection
- [ ] **Expected**: Context menu or deletion

---

## Node Deletion

### Delete Single Node
- [ ] **Test**: Select a leaf node (Action or Condition)
- [ ] **Test**: Press Delete key
- [ ] **Expected**: Node disappears
- [ ] **Expected**: Console: `[BTEditor] Deleted node ID: [id]`

### Delete Connected Node
- [ ] **Test**: Create A→B connection
- [ ] **Test**: Select and delete node B
- [ ] **Expected**: Node B deleted
- [ ] **Expected**: Connection A→B removed

### Delete Multiple Nodes
- [ ] **Test**: Select 3 nodes (Ctrl+click)
- [ ] **Test**: Press Delete key
- [ ] **Expected**: All 3 nodes disappear
- [ ] **Expected**: Console shows 3 deletion messages

---

## Node Duplication

### Duplicate Single Node
- [ ] **Test**: Select a Sequence node
- [ ] **Test**: Press Ctrl+D
- [ ] **Expected**: New node appears: "New Sequence (Copy)"
- [ ] **Expected**: New node has unique ID
- [ ] **Expected**: Console: `[BTEditor] Duplicated node: New Sequence (Copy) (ID: [new_id])`

### Duplicate Multiple Nodes
- [ ] **Test**: Select 2 nodes
- [ ] **Test**: Press Ctrl+D
- [ ] **Expected**: 2 new nodes created with " (Copy)" suffix
- [ ] **Expected**: New nodes are selected

### Duplicate Connected Nodes
- [ ] **Test**: Create A→B connection
- [ ] **Test**: Select A and duplicate
- [ ] **Expected**: Duplicate A' created
- [ ] **Expected**: Connection is NOT duplicated (expected behavior)

---

## Undo/Redo

### Undo Node Creation
- [ ] **Test**: Create a new Action node
- [ ] **Test**: Press Ctrl+Z
- [ ] **Expected**: Node disappears
- [ ] **Expected**: Console: `[BTEditor] Undo performed`
- [ ] **Expected**: Redo button enabled

### Redo Node Creation
- [ ] **Test**: After undo, press Ctrl+Y
- [ ] **Expected**: Node reappears
- [ ] **Expected**: Console: `[BTEditor] Redo performed`

### Undo Node Deletion
- [ ] **Test**: Delete a node
- [ ] **Test**: Press Ctrl+Z
- [ ] **Expected**: Node reappears

### Undo Connection Creation
- [ ] **Test**: Create connection A→B
- [ ] **Test**: Press Ctrl+Z
- [ ] **Expected**: Connection disappears

### Undo Connection Deletion
- [ ] **Test**: Delete connection A→B
- [ ] **Test**: Press Ctrl+Z
- [ ] **Expected**: Connection reappears

### Undo Stack Limit
- [ ] **Test**: Perform 101 operations
- [ ] **Test**: Try to undo all
- [ ] **Expected**: Can only undo last 100 operations
- [ ] **Expected**: Oldest action is lost

### Redo Stack Clear
- [ ] **Test**: Create node, undo, create different node
- [ ] **Expected**: Redo button becomes disabled (redo stack cleared)

---

## Save Functionality

### Save Modified Tree
- [ ] **Test**: Create 2 nodes and 1 connection
- [ ] **Test**: Click "Save Tree" button
- [ ] **Expected**: Console: `[BTEditor] Tree saved to: Blueprints/AI/[TreeName]_edited.json`
- [ ] **Expected**: Status changes to "[Unmodified]"
- [ ] **Expected**: File exists in `Blueprints/AI/` directory

### Verify Saved JSON
- [ ] **Test**: Open saved JSON file
- [ ] **Expected**: File contains:
  - `"schema_version": 2`
  - `"type": "BehaviorTree"`
  - `"name": "[TreeName]"`
  - `"metadata"` section with author, timestamps, tags
  - `"data"` section with `rootNodeId` and `nodes` array
  - All created nodes present with correct types
  - All connections present in `childIds` or `decoratorChildId`

### Save Without Modifications
- [ ] **Test**: Click "Save Tree" without making changes
- [ ] **Expected**: Console: `[BTEditor] No changes to save`

---

## Validation Rules

### Rule: Prevent Self-Connection
- [ ] **Test**: Try A→A
- [ ] **Expected**: Blocked

### Rule: Prevent Cycles
- [ ] **Test**: Create A→B→C, try C→A
- [ ] **Expected**: Blocked

### Rule: Prevent Duplicate Connections
- [ ] **Test**: Create A→B, try A→B again
- [ ] **Expected**: Blocked

### Rule: Decorator Single Child
- [ ] **Test**: Inverter→A, try Inverter→B
- [ ] **Expected**: Blocked

### Rule: Leaf Nodes Have No Children
- [ ] **Test**: Try to drag from Action "Out" pin
- [ ] **Expected**: No "Out" pin exists, cannot drag

---

## Camera and Navigation

### Existing Camera Controls (Should Still Work)
- [ ] **Test**: Mouse wheel zoom in/out
- [ ] **Test**: Press F to fit graph to view
- [ ] **Test**: Press C to center view
- [ ] **Test**: Press 0 to reset zoom
- [ ] **Test**: Press M to toggle minimap
- [ ] **Test**: Pan by dragging empty space

### Editor Mode Doesn't Break Camera
- [ ] **Test**: Enter editor mode and use all camera controls
- [ ] **Expected**: All camera controls work normally

---

## Entity Switching

### Switch Entity While Editing
- [ ] **Test**: Select Entity A, enter editor mode, create a node
- [ ] **Test**: Select Entity B (without exiting editor mode)
- [ ] **Expected**: Unsaved changes warning? (current: auto-load new tree)
- [ ] **Expected**: New tree loaded for Entity B
- [ ] **Expected**: Editor state reset (undo/redo stacks cleared)

### Modified Tree Warning (Future Enhancement)
- [ ] **Test**: Modify tree, switch entity
- [ ] **Expected**: Warning dialog about unsaved changes (not yet implemented)

---

## Console Output Verification

### Expected Messages
- [ ] Node creation: `[BTEditor] Created node: [name] (ID: [id])`
- [ ] Node deletion: `[BTEditor] Deleted node ID: [id]`
- [ ] Node duplication: `[BTEditor] Duplicated node: [name] (ID: [id])`
- [ ] Connection created: `[BTEditor] Connection created: [parent] -> [child]`
- [ ] Connection deleted: `[BTEditor] Connection deleted: [parent] -> [child]`
- [ ] Invalid connection: `[BTEditor] Invalid connection: [parent] -> [child]`
- [ ] Save success: `[BTEditor] Tree saved to: [filename]`
- [ ] Undo: `[BTEditor] Undo performed`
- [ ] Redo: `[BTEditor] Redo performed`

---

## Edge Cases

### Empty Tree
- [ ] **Test**: Delete all nodes
- [ ] **Expected**: Empty graph
- [ ] **Expected**: Can still create new nodes

### Large Tree
- [ ] **Test**: Create 20+ nodes
- [ ] **Expected**: Layout still works
- [ ] **Expected**: No performance issues

### Rapid Operations
- [ ] **Test**: Rapidly create/delete/undo/redo (stress test)
- [ ] **Expected**: No crashes
- [ ] **Expected**: All operations complete correctly

### Invalid Tree State
- [ ] **Test**: Manually edit JSON to create invalid tree
- [ ] **Test**: Load in editor
- [ ] **Expected**: Graceful handling or validation errors

---

## Integration with Debugger

### Switch Modes Mid-Execution
- [ ] **Test**: Entity is running AI, enter editor mode
- [ ] **Expected**: Execution highlighting stops
- [ ] **Expected**: Tree is frozen for editing

### Return to Debugger Mode
- [ ] **Test**: Exit editor mode while entity is still active
- [ ] **Expected**: Execution highlighting resumes
- [ ] **Expected**: Can see current node pulsing yellow

---

## Known Limitations (Document, Don't Fix)

- [ ] Node positions not saved to JSON (all spawn at origin)
- [ ] No node property editing (parameters, conditions, actions)
- [ ] No copy/paste between trees
- [ ] No drag to reposition nodes
- [ ] No visual indication of validation errors (only console)
- [ ] Save filename is auto-generated (no custom name)
- [ ] No confirmation dialog for destructive operations

---

## Test Results Summary

**Date**: __________  
**Tester**: __________  
**Build**: __________  

**Tests Passed**: ___ / ___  
**Tests Failed**: ___ / ___  
**Blockers Found**: ___ 

**Critical Issues**:
- 

**Minor Issues**:
- 

**Suggested Improvements**:
- 

---

## Sign-Off

- [ ] All critical features working
- [ ] No crashes or major bugs
- [ ] Console output is correct
- [ ] Saved JSON files are valid
- [ ] Documentation is accurate

**Approved by**: __________  
**Date**: __________
