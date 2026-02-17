---
id: bt-graph-editor
title: BT Graph Editor
sidebar_label: BT Graph Editor
---

# Behavior Tree Graph Editor

## Overview

The Behavior Tree (BT) Graph Editor is an interactive visual editor for creating and modifying AI behavior trees. It's integrated into the BT Debugger window with an editor mode toggle, allowing you to design AI behaviors without manually editing JSON files.

**Hotkey**: Press **F8** to open the BT Debugger/Editor window.

## Quick Start

### Opening the Editor

1. Launch the Olympe Engine
2. Load a level with AI entities
3. Press **F8** to open the BT Debugger window
4. Enable **"Editor Mode"** checkbox to start editing

### Interface Overview

- **Entity List** (left): Browse entities with behavior trees
- **Node Graph** (center): Visual tree editing canvas
- **Inspector** (right): Node properties and validation
- **Toolbar**: File operations and editing tools
- **Status Bar**: Current file and modification status

## Node Types

### Composite Nodes (Multiple Children)
- **Selector** (`?`) - OR logic: succeeds if any child succeeds
- **Sequence** (`→`) - AND logic: succeeds if all children succeed
- **Parallel** (`||`) - Runs multiple children simultaneously

### Decorator Nodes (Single Child Modifier)
- **Inverter** (`!`) - Flips child result (success ↔ failure)
- **Repeater** (`↻`) - Repeats child N times
- **UntilSuccess** - Repeats until child succeeds
- **UntilFailure** - Repeats until child fails
- **Cooldown** - Time-based execution limiting

### Leaf Nodes (No Children)
- **Action** (`►`) - Performs action (move, attack, patrol)
- **Condition** (`◆`) - Checks condition (health, distance, etc.)

## Creating Nodes

### From Node Palette
1. Click desired node type in the palette
2. Click in the graph to place the node

### Right-Click Menu
1. Right-click in empty graph area
2. Select node type from context menu
3. Node appears at cursor position

## Connecting Nodes

### Pin-Based Connection System

Each node has:
- **Input Pin** (left side): Receives connection from parent
- **Output Pin** (right side): Connects to children

### Creating Connections
1. Click and drag from **output pin** (parent node)
2. Drag to **input pin** (child node)
3. Visual feedback shows validity:
   - **Green**: Valid connection
   - **Red**: Invalid (would create cycle)
4. Release mouse to create connection

### Connection Rules
- No cycles allowed (no node can be its own ancestor)
- Decorators: Exactly 1 child required
- Composites: At least 1 child required
- No duplicate connections

## Editor Features

### File Operations

**File Menu**:
- **New** (Ctrl+N): Create from template (Empty, Basic AI, Patrol, Combat)
- **Open** (Ctrl+O): Load existing behavior tree
- **Save** (Ctrl+S): Save to current file
- **Save As** (Ctrl+Shift+S): Save to new file

### Edit Operations

**Toolbar/Edit Menu**:
- **Undo** (Ctrl+Z): Undo last action (max 100 steps)
- **Redo** (Ctrl+Y): Redo last undone action
- **Delete** (Delete): Remove selected node
- **Duplicate** (Ctrl+D): Copy selected node

### Node Properties Editor

Select a node to edit properties:
- **Name**: Display label
- **Action Type** (Action nodes): Move, Attack, Patrol, Idle, etc.
- **Condition Type** (Condition nodes): CanSeeTarget, HealthBelow, etc.
- **Repeat Count** (Repeater): Number of repetitions
- **Cooldown Time** (Cooldown): Time in seconds

### Real-Time Validation

The validation panel shows issues:
- **Errors** (red): Critical problems preventing execution
- **Warnings** (yellow): Potential issues
- **Info** (blue): Suggestions

Common validations:
- Exactly one root node (no parent)
- No cycles in graph
- Decorators have exactly 1 child
- Composites have at least 1 child
- Actions have valid action type
- Conditions have valid condition type

### Camera Controls

**Zoom**:
- Mouse wheel: Zoom in/out (0.3x - 3.0x)
- Double-click: Reset zoom

**Pan**:
- Middle mouse drag: Pan viewport
- Arrow keys: Pan viewport

**Fit View**: Button in toolbar centers and scales entire tree

**Minimap**: Top-right overlay shows full tree with viewport indicator

### Grid Snapping

Nodes snap to grid for clean alignment:
- Grid size: 16px (configurable in `BT_config.json`)
- Toggle: Enable/disable in View menu

## Keyboard Shortcuts

### File
- `Ctrl+N` - New tree
- `Ctrl+O` - Open
- `Ctrl+S` - Save
- `Ctrl+Shift+S` - Save As
- `F8` - Toggle window

### Edit
- `Ctrl+Z` - Undo
- `Ctrl+Y` - Redo
- `Delete` - Delete selected
- `Ctrl+D` - Duplicate selected

### View
- `Mouse Wheel` - Zoom
- `Middle Mouse + Drag` - Pan
- `Arrow Keys` - Pan
- `F` - Fit view

## Debugging Mode

Disable "Editor Mode" to switch to debug mode:
- View runtime execution of trees
- See active nodes highlighted
- Inspect blackboard variables
- View execution log
- Filter/sort entities

## Best Practices

### Tree Organization
- **Single Root**: Always start with one root node (Selector or Sequence)
- **Depth Limit**: Keep trees shallow (3-5 levels deep)
- **Naming**: Use descriptive node names ("Check Health", "Attack Target")

### Structure Patterns
- **Selector for Fallbacks**: Try high-priority actions first
- **Sequence for Steps**: Chain required actions
- **Decorators for Conditions**: Wrap sequences with condition checks

### Testing
- Save frequently (Ctrl+S)
- Validate before testing (check validation panel)
- Test with multiple entities
- Use debug mode to watch execution

## File Format

The editor saves in **Behavior Tree V2** JSON format:

```json
{
  "version": 2,
  "treeId": 123,
  "name": "EnemyAI",
  "rootNodeId": 1,
  "nodes": [
    {
      "id": 1,
      "type": "Sequence",
      "name": "Main",
      "children": [2, 3],
      "editorPosition": {"x": 0, "y": 0}
    },
    {
      "id": 2,
      "type": "Condition",
      "name": "Check Health",
      "conditionType": "HealthBelow",
      "children": [],
      "editorPosition": {"x": 200, "y": 100}
    }
  ]
}
```

Files are saved to `Gamedata/BehaviorTrees/` by default.

## Troubleshooting

### Tree Won't Save
- Check file permissions on target directory
- Verify tree passes validation (no errors)
- Try Save As to different location

### Connections Won't Create
- Check for cycles (parent cannot be descendant of child)
- Verify node types support children
- Check validation panel for specific error

### Undo Not Working
- Undo stack has 100-action limit
- Check if you're at oldest action
- Try Redo if you undid by accident

## Related Documentation

- [BT Graph Editor Architecture](/technical-reference/behavior-trees/bt-editor-architecture) - Technical details
- [Creating BT Actions](/technical-reference/behavior-trees/creating-actions) - Implementing custom actions
- [BT Debugger](/user-guide/ai/bt-debugger) - Runtime debugging features
- [Behavior Trees Overview](/technical-reference/behavior-trees/overview) - AI system architecture

## Video Tutorials

Coming soon: Video walkthroughs of common workflows.

## Support

- [GitHub Issues](https://github.com/Atlasbruce/Olympe-Engine/issues)
- [Documentation Site](https://atlasbruce.github.io/Olympe-Engine/)
