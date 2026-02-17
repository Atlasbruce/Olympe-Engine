# Behavior Tree Editor - User Guide

> **📖 Latest Documentation**: For the most up-to-date version of this guide, visit the [official Olympe Engine documentation site](https://atlasbruce.github.io/Olympe-Engine/editors/bt-graph-editor).

## Overview

The Behavior Tree (BT) Editor is an interactive visual editor integrated into the BT Debugger window. It allows you to create, modify, and test behavior trees directly in the engine without manually editing JSON files.

## Getting Started

### Opening the Editor

1. Launch the Olympe Engine
2. Load a level with AI entities that use behavior trees
3. Press **F10** to open the BT Debugger window
4. Select an entity from the entity list (left panel)
5. Check the **"Editor Mode"** checkbox to enable editing

### Editor Interface

The editor interface consists of:

- **Entity List** (left): List of entities with behavior trees
- **Node Graph** (center): Visual graph of the behavior tree
- **Inspector** (right): Runtime information and blackboard values
- **Toolbar**: Quick access to editor operations
- **Status Indicator**: Shows if the tree has been modified

## Node Types

The editor supports 6 types of nodes:

### Composite Nodes
- **Selector** (`?`): OR node - succeeds if any child succeeds
- **Sequence** (`→`): AND node - succeeds if all children succeed

### Decorator Nodes
- **Inverter** (`!`): Inverts the result of its child
- **Repeater** (`↻`): Repeats its child N times

### Leaf Nodes
- **Condition** (`◆`): Checks a condition (e.g., target visible, health below threshold)
- **Action** (`►`): Performs an action (e.g., move, attack, patrol)

## Creating Nodes

There are two ways to create nodes:

### Method 1: Right-Click Menu
1. Right-click anywhere in the node graph area
2. Select the desired node type from the popup menu
3. The node will be created at the cursor position

### Method 2: Toolbar Button
1. Click the **"Add Node"** button in the toolbar
2. Select the desired node type from the popup menu
3. The node will be created at the last cursor position

## Connecting Nodes

### Creating Connections
1. Click and drag from an **output pin** (labeled "Out") of a parent node
2. Release on an **input pin** (labeled "In") of a child node
3. If the connection is valid, it will be created
4. If invalid, a message will appear in the console

### Connection Rules
- Only **Composite** (Selector, Sequence) and **Decorator** (Inverter, Repeater) nodes can have children
- **Decorator** nodes can only have **one child**
- **Leaf** nodes (Action, Condition) cannot have children
- **No cycles** are allowed (parent cannot be a descendant of child)
- **No duplicate** connections between the same nodes

### Deleting Connections
1. Select a connection (it will highlight)
2. Press **Delete** or right-click and select delete
3. The connection will be removed from the tree

## Selecting Nodes

### Single Selection
- Click on a node to select it
- Selected nodes show a highlight outline

### Multiple Selection
- Hold **Shift** or **Ctrl** and click multiple nodes
- Or drag a selection box around multiple nodes
- The toolbar shows the count of selected nodes

## Editing Operations

### Deleting Nodes
1. Select one or more nodes
2. Press the **Delete** key
3. The nodes and all connections to/from them will be removed

**Keyboard Shortcut**: `Delete`

### Duplicating Nodes
1. Select one or more nodes
2. Press **Ctrl+D** or use the toolbar
3. Duplicate nodes will be created with " (Copy)" appended to their names

**Keyboard Shortcut**: `Ctrl+D`

### Undo/Redo
- **Undo**: Press **Ctrl+Z** or click the Undo button
- **Redo**: Press **Ctrl+Y** or **Ctrl+Shift+Z**, or click the Redo button
- The editor maintains a history of up to **100 actions**

**Keyboard Shortcuts**: 
- Undo: `Ctrl+Z`
- Redo: `Ctrl+Y` or `Ctrl+Shift+Z`

## Saving Changes

### Manual Save
1. Click the **"Save Tree"** button in the toolbar
2. The tree will be saved to `Blueprints/AI/[TreeName]_edited.json`
3. The modified indicator will change to "[Unmodified]"

### File Format
- Saved files use the **v2 schema format**
- They include metadata (author, timestamp, tags)
- They are compatible with the standard BT loader

## Validation

The editor automatically validates all operations:

### Connection Validation
✅ **Valid Connections**:
- Parent is a Composite or Decorator node
- Child exists in the tree
- No cycles would be created
- No duplicate connections

❌ **Invalid Connections** (blocked):
- Self-connections (node to itself)
- Cycles (would make parent a descendant of child)
- Duplicate connections
- Decorator nodes already have a child
- Leaf nodes as parents

### Console Output
- All editor actions are logged to the console
- Invalid operations show error messages
- Use console output to debug validation issues

## Camera Controls

### Navigation
- **Pan**: Click and drag in empty space
- **Zoom**: Mouse wheel
- **Keyboard Zoom In**: `+` or `=` key
- **Keyboard Zoom Out**: `-` key

### Quick Actions
- **Fit to View**: Press `F` - Fits entire tree in viewport
- **Center View**: Press `C` - Centers tree in viewport
- **Reset Zoom**: Press `0` - Resets zoom to 100%
- **Toggle Minimap**: Press `M` - Shows/hides minimap overlay

## Tips and Best Practices

### Organizing Trees
1. **Start with the root**: Create your root node first (usually a Selector or Sequence)
2. **Build top-down**: Create parents before children
3. **Group related behaviors**: Use Sequences for action chains
4. **Add fallbacks**: Use Selectors for alternative behaviors

### Node Naming
- Give nodes descriptive names
- Use consistent naming conventions
- Example: "Check Target Visible" instead of "Condition 1"

### Testing Changes
1. Make small incremental changes
2. Test after each major modification
3. Use the debugger view (disable editor mode) to see execution flow
4. Watch the blackboard values in the inspector

### Avoiding Common Mistakes
❌ Don't create cycles
❌ Don't leave nodes disconnected (orphaned)
❌ Don't forget to save your changes
❌ Don't mix editor and debugger modes without testing

✅ Use undo/redo frequently
✅ Save intermediate versions
✅ Test with different entity states
✅ Validate connections before complex changes

## Workflow Example

Here's a typical workflow for creating a simple patrol behavior:

1. **Enable Editor Mode**
   - Check "Editor Mode" checkbox

2. **Create Root Node**
   - Right-click → Select "Sequence"
   - Rename to "Patrol Behavior"

3. **Add Condition**
   - Right-click → Select "Condition"
   - Rename to "Has Patrol Points"

4. **Add Action**
   - Right-click → Select "Action"
   - Rename to "Pick Next Patrol Point"

5. **Add Movement**
   - Right-click → Select "Action"
   - Rename to "Move to Patrol Point"

6. **Connect Nodes**
   - Drag from Sequence output to Condition input
   - Drag from Sequence output to First Action input
   - Drag from Sequence output to Second Action input

7. **Save Tree**
   - Click "Save Tree" button
   - File saved to `Blueprints/AI/Patrol_Behavior_edited.json`

8. **Test**
   - Disable "Editor Mode"
   - Watch entity execute the behavior
   - Check execution log in inspector

## Keyboard Reference

| Action | Keyboard Shortcut |
|--------|------------------|
| Delete selected nodes | `Delete` |
| Duplicate selected nodes | `Ctrl+D` |
| Undo | `Ctrl+Z` |
| Redo | `Ctrl+Y` or `Ctrl+Shift+Z` |
| Fit to view | `F` |
| Center view | `C` |
| Reset zoom | `0` |
| Toggle minimap | `M` |
| Zoom in | `+` or `=` |
| Zoom out | `-` |

## Mouse Reference

| Action | Mouse Operation |
|--------|----------------|
| Select node | Left-click node |
| Multi-select | Ctrl/Shift + Left-click |
| Create connection | Drag from output pin to input pin |
| Delete connection | Select link, press Delete |
| Pan view | Click and drag empty space |
| Zoom | Mouse wheel |
| Context menu | Right-click in graph area |

## Troubleshooting

### Problem: Cannot create connection
**Solution**: Check console for validation error. Ensure:
- Parent node can have children (Composite or Decorator)
- Decorator doesn't already have a child
- Connection wouldn't create a cycle

### Problem: Node disappeared after deletion
**Solution**: This is normal behavior. Use Undo (Ctrl+Z) if deleted by mistake.

### Problem: Tree modified but not saved
**Solution**: Click "Save Tree" button before closing. Modified indicator shows "[Modified]" in yellow.

### Problem: Undo/Redo not working
**Solution**: 
- Check if undo stack is empty (button is disabled)
- Action history is limited to 100 operations
- Only editing operations are undoable (not camera moves)

### Problem: Editor mode checkbox doesn't work
**Solution**: Ensure an entity is selected and has a valid behavior tree loaded.

## Advanced Features

### Undo/Redo System
- Stores up to 100 actions
- Supports both node and connection operations
- Clears redo stack when new action is performed
- Each action stores complete state for restoration

### Link Tracking
- Editor maintains internal map of link IDs
- Enables proper connection deletion
- Updated each frame during rendering
- Cleared when switching trees

### Cycle Detection
- Uses Depth-First Search (DFS) algorithm
- Checks all descendants of potential child
- Prevents creating invalid tree structures
- O(N) complexity where N is number of nodes

## Future Enhancements

Planned features for future versions:
- [ ] Node property editing (parameters, conditions, actions)
- [ ] Visual node positioning saved to JSON
- [ ] Drag nodes to reposition
- [ ] Copy/paste nodes between trees
- [ ] Tree templates and snippets
- [ ] Visual feedback for validation errors
- [ ] Hot-reload edited trees in runtime
- [ ] Multi-tree editing
- [ ] Search and filter nodes
- [ ] Node comments and annotations

## Contact & Support

For questions, bugs, or feature requests:
- Check console output for detailed error messages
- Review validation rules in this guide
- Test with simple trees first
- Use undo frequently during experimentation

## Version History

**v1.0** (Phase 2) - Initial Release
- Basic node creation (6 types)
- Connection management
- Undo/Redo (100 actions)
- JSON save functionality
- Cycle detection
- Connection validation
- Multi-select support
- Keyboard shortcuts
