# Behavior Tree Graph Editor - Technical Architecture

## Overview

The Behavior Tree (BT) Graph Editor is a comprehensive visual editor for creating and modifying AI behavior trees in the Olympe Engine. It is integrated into the `BehaviorTreeDebugWindow` class with an editor mode toggle, allowing seamless switching between runtime debugging and tree editing.

The editor renders in a **separate SDL3 window** with its own ImGui context, following the same standalone window pattern as the Animation Editor. This provides a dedicated workspace for AI development while keeping the main game window uncluttered.

**Hotkey**: Press **F8** (formerly F10) to toggle the BT Debugger/Editor window.

## Core Architecture

### Main Components

#### 1. BehaviorTreeDebugWindow
**Location**: `Source/AI/BehaviorTreeDebugWindow.h/cpp`

The main class that serves dual purposes:
- **Debug Mode**: Runtime visualization of AI execution with entity list, node highlighting, blackboard inspection, and execution logs
- **Editor Mode**: Full graph editing capabilities with node creation, connection management, validation, and save/load

Key features:
```cpp
class BehaviorTreeDebugWindow {
    bool m_editorMode;              // Toggle between debug and editor modes
    bool m_isVisible;               // Window visibility state
    BTCommandStack m_commandStack;  // Undo/redo management
    
    // Editor state
    uint32_t m_selectedNodeId;      // Currently selected node
    bool m_isDraggingPin;           // Pin connection dragging
    uint32_t m_dragSourceNodeId;    // Source node for connection
    bool m_showNewBTDialog;         // New BT creation dialog
    bool m_showValidationPanel;     // Validation results panel
    
    // Configuration
    BTConfig m_config;              // Loaded from BT_config.json
};
```

**Standalone Window Management**:
- `CreateSeparateWindow()` - Creates SDL3 window with dedicated ImGui context
- `DestroySeparateWindow()` - Proper cleanup of window and renderer
- `ProcessEvent(SDL_Event*)` - Handles input events for the separate window
- `RenderInSeparateWindow()` - Renders UI in separate window context

#### 2. BTGraphLayoutEngine
**Location**: `Source/AI/BTGraphLayoutEngine.h/cpp`

Automatic graph layout engine using the **Sugiyama algorithm** with 5 phases:
1. **Layering**: Assign nodes to hierarchical layers via BFS
2. **Initial Ordering**: Order nodes within each layer
3. **Crossing Reduction**: Minimize edge crossings using barycenter heuristic
4. **Buchheim-Walker Layout**: Optimal parent-centering in abstract space
5. **Force-Directed Collision Resolution**: Iterative overlap elimination

**Layout Directions**:
- `BTLayoutDirection::TopToBottom` - Traditional vertical layout
- `BTLayoutDirection::LeftToRight` - Horizontal layout (default)

**Configuration**:
```cpp
struct BTNodeLayout {
    uint32_t nodeId;        // Node identifier
    Vector position;        // Final world position (x, y)
    int layer;              // Hierarchical layer (0 = root)
    int orderInLayer;       // Order within layer
    float width;            // Visual width (200px default)
    float height;           // Visual height (100px default)
};
```

Spacing is configurable via `BT_config.json`:
- `horizontalSpacing`: Distance between nodes horizontally (280px default)
- `verticalSpacing`: Distance between layers (120px default)
- `gridSize`: Grid snap size (16px default)
- `gridSnappingEnabled`: Enable/disable grid snapping

#### 3. BTEditorCommand / BTCommandStack
**Location**: `Source/AI/BTEditorCommand.h/cpp`

Command pattern implementation for **undo/redo** functionality. Maximum 100 commands in stack.

**Command Types**:
1. **AddNodeCommand** - Create new node at position
2. **DeleteNodeCommand** - Remove node and its connections
3. **MoveNodeCommand** - Change node position
4. **ConnectNodesCommand** - Create parent-child connection
5. **DisconnectNodesCommand** - Remove connection
6. **EditParameterCommand** - Modify node parameters (type, label, etc.)

**Usage**:
```cpp
BTCommandStack m_commandStack;

// Execute command (adds to undo stack)
m_commandStack.Execute(std::make_unique<AddNodeCommand>(tree, nodeType, position));

// Undo/Redo
m_commandStack.Undo();   // Ctrl+Z
m_commandStack.Redo();   // Ctrl+Y

// Check availability
bool canUndo = m_commandStack.CanUndo();
std::string undoDesc = m_commandStack.GetUndoDescription();
```

**Keyboard Shortcuts**:
- `Ctrl+Z`: Undo last action
- `Ctrl+Y`: Redo last undone action
- `Ctrl+S`: Save current tree
- `Ctrl+Shift+S`: Save as new file
- `Delete`: Delete selected node
- `Ctrl+D`: Duplicate selected node

#### 4. BTConfig
**Location**: `Source/AI/BehaviorTreeDebugWindow.h`

Configuration structure loaded from `Config/BT_config.json`:

```json
{
  "layout": {
    "defaultHorizontal": true,
    "gridSize": 16.0,
    "gridSnappingEnabled": true,
    "horizontalSpacing": 280.0,
    "verticalSpacing": 120.0
  },
  "rendering": {
    "pinRadius": 6.0,
    "pinOutlineThickness": 2.0,
    "bezierTangent": 80.0,
    "connectionThickness": 2.0
  },
  "nodeColors": {
    "Selector": {
      "Running": {"r": 255, "g": 200, "b": 100, "a": 255},
      "Success": {"r": 100, "g": 255, "b": 100, "a": 255},
      "Failure": {"r": 255, "g": 100, "b": 100, "a": 255}
    }
    // ... colors for each node type and status
  }
}
```

## Node System

### Node Types
From `BehaviorTree.h` - `BTNodeType` enum:

**Composite Nodes** (multiple children):
- `Selector` (`?`) - OR node, succeeds if any child succeeds
- `Sequence` (`→`) - AND node, succeeds if all children succeed
- `Parallel` (`||`) - Runs multiple children simultaneously

**Decorator Nodes** (single child modifier):
- `Inverter` (`!`) - Inverts child result
- `Repeater` (`↻`) - Repeats child N times
- `UntilSuccess` - Repeats until child succeeds
- `UntilFailure` - Repeats until child fails
- `Cooldown` - Time-based execution limiting

**Leaf Nodes** (no children):
- `Action` (`►`) - Performs action (move, attack, patrol, etc.)
- `Condition` (`◆`) - Checks condition (target visible, health check, etc.)

### Node Statuses
From `BehaviorTree.h` - `BTStatus` enum:

- `Running` - Node is executing (yellow/orange)
- `Success` - Node completed successfully (green)
- `Failure` - Node failed (red)
- `Invalid` - Node is invalid/not configured (gray)

Node colors are determined by `GetNodeColorByStatus(nodeType, status)` using config values.

### Node Structure
```cpp
struct BTNode {
    uint32_t id;                    // Unique identifier
    BTNodeType type;                // Node type
    std::string name;               // Display name
    std::vector<uint32_t> childIds; // Child node IDs
    
    // Node-specific data
    std::string actionType;         // For Action nodes
    std::string conditionType;      // For Condition nodes
    int repeatCount;                // For Repeater nodes
    float cooldownTime;             // For Cooldown nodes
    
    Vector editorPosition;          // Position in editor
};
```

## Editor Features

### 1. Node Palette
**Method**: `RenderNodePalette()`

Displays all available node types for creation. Click a type, then click in the graph to place.

Categories:
- **Composites**: Selector, Sequence, Parallel
- **Decorators**: Inverter, Repeater, UntilSuccess, UntilFailure, Cooldown
- **Leaves**: Action, Condition

### 2. Pin-Based Connections
**Methods**: `RenderNodePins()`, `RenderBezierConnection()`

Visual connection system:
- **Input Pins**: Left side of nodes (or top in TopToBottom layout)
- **Output Pins**: Right side of nodes (or bottom in TopToBottom layout)
- **Pin Colors**:
  - Green: Valid connection target
  - Red: Invalid connection (would create cycle or violate rules)
  - Gray: Default state

**Connection Process**:
1. Click and drag from output pin (right side)
2. Drag to input pin of target node (left side)
3. Visual feedback shows connection validity
4. Release to create connection (executes `ConnectNodesCommand`)

**Connection Validation**: `ValidateConnection(parentId, childId)`, `IsConnectionValid()`

Rules:
- No cycles allowed (detected via DFS)
- Decorators must have exactly 1 child
- Composites must have at least 1 child
- No duplicate parent-child connections

### 3. Real-Time Validation Panel
**Method**: `RenderValidationPanel()`

Displays validation messages from `BehaviorTreeAsset::ValidateTreeFull()`:

**Message Types**:
- **Error** (red): Critical issues that prevent tree execution
- **Warning** (yellow): Non-critical issues that may affect behavior
- **Info** (blue): Informational messages

**Validation Rules**:
- Exactly one root node (no parent)
- No cycles in the graph
- No orphaned nodes (disconnected from root)
- Decorators have exactly 1 child
- Composites have at least 1 child
- Action nodes have valid `actionType`
- Condition nodes have valid `conditionType`

Structure:
```cpp
struct BTValidationMessage {
    enum Severity { Error, Warning, Info };
    Severity severity;
    uint32_t nodeId;      // 0 for tree-wide issues
    std::string message;
};
```

### 4. Node Properties Editor
**Method**: `RenderNodeProperties()`

Editable properties for selected node:
- **Name**: Display label
- **Type**: Node type (read-only, set at creation)
- **Action Type**: Dropdown for Action nodes (Move, Attack, Patrol, Idle, etc.)
- **Condition Type**: Dropdown for Condition nodes (CanSeeTarget, HealthBelow, etc.)
- **Repeat Count**: Integer for Repeater nodes
- **Cooldown Time**: Float for Cooldown nodes

Changes are recorded via `EditParameterCommand` for undo/redo.

### 5. Save/Load System
**Methods**: `Save()`, `SaveAs()`, `SerializeTreeToJson()`

**JSON Format**: Behavior Tree V2 Schema
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
      "name": "MainSequence",
      "children": [2, 3],
      "editorPosition": {"x": 0, "y": 0}
    },
    {
      "id": 2,
      "type": "Condition",
      "name": "CheckHealth",
      "conditionType": "HealthBelow",
      "children": [],
      "editorPosition": {"x": 200, "y": 100}
    }
    // ...
  ]
}
```

**File Operations** (via `RenderFileMenu()`):
- **New**: Create new tree from template
- **Open**: Load existing `.json` file
- **Save** (Ctrl+S): Overwrite current file
- **Save As** (Ctrl+Shift+S): Save to new file
- **Close**: Close editor window

### 6. New BT from Templates
**Method**: `RenderNewBTDialog()`, `CreateFromTemplate()`

**4 Built-in Templates**:
1. **Empty** - Single root Selector node
2. **Basic AI** - Simple patrol + chase + attack structure
3. **Patrol** - Waypoint patrol with idle behavior
4. **Combat** - Combat decision tree with health checks

Template structure creates nodes with proper connections and placeholder parameters.

### 7. Grid Snapping
**Method**: `SnapToGrid(position)`

Nodes snap to grid for alignment:
- Configurable grid size (`gridSize` in config, default 16px)
- Can be toggled on/off (`gridSnappingEnabled`)
- Applies to node creation and dragging

### 8. Camera Controls

**Zoom**:
- Mouse wheel: Zoom in/out (range: 0.3 - 3.0x)
- Double-click: Reset zoom to 1.0

**Pan**:
- Middle mouse drag: Pan viewport
- Arrow keys: Pan viewport

**Fit View**:
- Button in toolbar: Centers and scales to fit entire tree

**Minimap**:
- Top-right corner overlay
- Shows full tree with viewport indicator
- Click to jump to location

### 9. Entity List (Debug Mode)
**Method**: `RenderEntityListPanel()`

When not in editor mode, shows entities with behavior trees:
- **Filtering**: By name, tree ID, or status
- **Sorting**: By name, tree ID, status, or last update time
- **Selection**: Click to view entity's tree
- **Status Indicators**: Color-coded by last execution status

### 10. Blackboard Inspection
**Method**: `RenderBlackboardSection()`

In debug mode, displays entity's blackboard variables:
- Key-value pairs
- Data types (int, float, bool, string, EntityID, Vector)
- Real-time values during AI execution

### 11. Execution Log
**Method**: `RenderExecutionLog()`

Shows recent node executions with:
- Timestamp (time ago)
- Node name and ID
- Execution result (Running, Success, Failure)
- Entity that executed
- Color-coded by status

## Standalone Window Pattern

The BT Editor uses the same standalone window pattern as the Animation Editor:

### Window Lifecycle
1. **Creation** (`CreateSeparateWindow()`):
   - Create SDL_Window (1600x900, resizable)
   - Create SDL_Renderer (accelerated, VSync)
   - Create dedicated ImGuiContext
   - Initialize ImGui for SDL3 + SDL_Renderer backend

2. **Rendering** (`RenderInSeparateWindow()`):
   - Switch to BT window's ImGui context
   - Poll SDL events for this window
   - Begin ImGui frame
   - Render UI (full-screen window with menu bar)
   - End frame and present renderer

3. **Destruction** (`DestroySeparateWindow()`):
   - Shutdown ImGui backend
   - Destroy ImGui context
   - Destroy SDL_Renderer
   - Destroy SDL_Window

### Event Handling
`ProcessEvent(SDL_Event*)` handles:
- Window close button (SDL_EVENT_WINDOW_CLOSE_REQUESTED)
- Keyboard shortcuts (Ctrl+Z, Ctrl+Y, Delete, etc.)
- Mouse events (clicks, drags, wheel)
- Resize events

Events are routed to BT Debugger **after** Animation Editor in `OlympeEngine.cpp` main loop.

## Integration with BehaviorTreeAsset

The editor modifies `BehaviorTreeAsset` objects (defined in `BehaviorTree.h`):

**CRUD Operations**:
```cpp
class BehaviorTreeAsset {
    // Create
    uint32_t AddNode(BTNodeType type, const std::string& name);
    
    // Read
    BTNode* GetNode(uint32_t nodeId);
    const std::vector<BTNode>& GetAllNodes() const;
    
    // Update
    void SetNodeName(uint32_t nodeId, const std::string& name);
    void SetNodePosition(uint32_t nodeId, const Vector& pos);
    
    // Delete
    bool RemoveNode(uint32_t nodeId);
    
    // Connections
    bool ConnectNodes(uint32_t parentId, uint32_t childId);
    bool DisconnectNodes(uint32_t parentId, uint32_t childId);
    
    // Validation
    std::vector<BTValidationMessage> ValidateTreeFull() const;
};
```

## Performance Considerations

- **Layout Computation**: Only runs when tree structure changes or layout toggle
- **Rendering**: Uses ImGui draw lists for efficient GPU rendering
- **Command Stack**: Limited to 100 commands to prevent memory growth
- **Validation**: Only runs when explicitly requested or before save
- **Event Processing**: Separate window doesn't block main game loop

## File Locations

**Core Editor**:
- `Source/AI/BehaviorTreeDebugWindow.h/cpp` - Main editor class
- `Source/AI/BTGraphLayoutEngine.h/cpp` - Layout algorithm
- `Source/AI/BTEditorCommand.h/cpp` - Command pattern for undo/redo
- `Source/AI/BehaviorTree.h/cpp` - Tree data structures and CRUD operations

**Configuration**:
- `Config/BT_config.json` - Editor configuration (colors, spacing, etc.)

**Integration**:
- `Source/OlympeEngine.cpp` - Main loop event routing (F8 hotkey)

## Related Documentation

- [BT Editor User Guide](../../BT_EDITOR_USER_GUIDE.md) - User-facing documentation
- [BT Debugger User Guide](../user-guide/ai/bt-debugger.md) - Runtime debugging features
- [Creating BT Actions Guide](04_Behavior_Trees/Creating_BT_Actions_Guide.md) - Implementing custom actions
- [Behavior Trees Quick Reference](04_Behavior_Trees/_QuickRef_BehaviorTrees.md) - API reference
