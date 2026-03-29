---
id: bt-editor-architecture
title: BT Editor Architecture
sidebar_label: Editor Architecture
---

# Behavior Tree Graph Editor - Architecture

## Overview

The BT Graph Editor is integrated into `BehaviorTreeDebugWindow` with dual debug/editor modes. It renders in a standalone SDL3 window with its own ImGui context.

**Hotkey**: F8

For complete technical details, see [BT_Graph_Editor_Architecture.md](https://github.com/Atlasbruce/Olympe-Engine/blob/main/Docs/Developer/BT_Graph_Editor_Architecture.md) in the repository.

## Core Components

### 1. BehaviorTreeDebugWindow
**Location**: `Source/AI/BehaviorTreeDebugWindow.h/cpp`

Main class with dual functionality:
- **Debug Mode**: Runtime visualization, entity list, blackboard, execution log
- **Editor Mode**: Full editing with node creation, validation, save/load

Key members:
```cpp
bool m_editorMode;              // Toggle debug/editor
BTCommandStack m_commandStack;  // Undo/redo management
uint32_t m_selectedNodeId;      // Current selection
bool m_isDraggingPin;           // Connection dragging
BTConfig m_config;              // From BT_config.json
```

### 2. BTGraphLayoutEngine
**Location**: `Source/AI/BTGraphLayoutEngine.h/cpp`

Automatic graph layout using Sugiyama algorithm (5 phases):
1. Layering (BFS)
2. Initial ordering
3. Crossing reduction
4. Buchheim-Walker layout
5. Force-directed collision resolution

**Layout Directions**:
- `TopToBottom` - Vertical
- `LeftToRight` - Horizontal (default)

### 3. BTEditorCommand / BTCommandStack
**Location**: `Source/AI/BTEditorCommand.h/cpp`

Command pattern for undo/redo (max 100 commands):
- `AddNodeCommand`
- `DeleteNodeCommand`
- `MoveNodeCommand`
- `ConnectNodesCommand`
- `DisconnectNodesCommand`
- `EditParameterCommand`

Keyboard shortcuts: Ctrl+Z (undo), Ctrl+Y (redo)

### 4. BTConfig
Configuration from `Config/BT_config.json`:
- Layout settings (spacing, grid size, snap)
- Rendering settings (pin radius, bezier tangent)
- Node colors by type and status

## Node System

### Node Types
From `BTNodeType` enum:

**Composites**:
- Selector, Sequence, Parallel

**Decorators**:
- Inverter, Repeater, UntilSuccess, UntilFailure, Cooldown

**Leaves**:
- Action, Condition

### Node Statuses
From `BTStatus` enum:
- Running (yellow/orange)
- Success (green)
- Failure (red)
- Invalid (gray)

## Editor Features

### Pin-Based Connections
- Input pins (left) and output pins (right)
- Drag from output to input to connect
- Visual feedback (green = valid, red = invalid)
- Bezier curves for visual clarity

### Validation System
Real-time validation with `BTValidationMessage`:
- **Error**: Critical issues (red)
- **Warning**: Non-critical (yellow)
- **Info**: Suggestions (blue)

Rules:
- Exactly one root node
- No cycles
- Decorators: exactly 1 child
- Composites: at least 1 child

### Node Properties Editor
Edit selected node:
- Name, action type, condition type
- Repeat count (Repeater)
- Cooldown time (Cooldown)

### Save/Load System
**JSON V2 Format**:
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
      "children": [2, 3],
      "editorPosition": {"x": 0, "y": 0}
    }
  ]
}
```

### Templates
4 built-in templates:
1. Empty - Single root
2. Basic AI - Patrol + chase + attack
3. Patrol - Waypoint patrol
4. Combat - Combat decision tree

## Standalone Window Pattern

Same pattern as Animation Editor:

**Lifecycle**:
1. `CreateSeparateWindow()` - SDL_Window, SDL_Renderer, ImGuiContext
2. `RenderInSeparateWindow()` - Render UI in separate context
3. `DestroySeparateWindow()` - Cleanup

**Event Handling**:
`ProcessEvent(SDL_Event*)` for window close, keyboard shortcuts, mouse events

## Integration

### BehaviorTreeAsset
CRUD operations:
```cpp
uint32_t AddNode(BTNodeType type, const std::string& name);
bool RemoveNode(uint32_t nodeId);
bool ConnectNodes(uint32_t parentId, uint32_t childId);
bool DisconnectNodes(uint32_t parentId, uint32_t childId);
std::vector<BTValidationMessage> ValidateTreeFull() const;
```

## File Locations

- `Source/AI/BehaviorTreeDebugWindow.h/cpp` - Main editor
- `Source/AI/BTGraphLayoutEngine.h/cpp` - Layout engine
- `Source/AI/BTEditorCommand.h/cpp` - Command pattern
- `Source/AI/BehaviorTree.h/cpp` - Data structures
- `Config/BT_config.json` - Configuration
- `Source/OlympeEngine.cpp` - Main loop (F8 hotkey)

## Related Documentation

- [BT Graph Editor User Guide](/editors/bt-graph-editor) - User documentation
- [Creating BT Actions](/technical-reference/behavior-trees/creating-actions) - Custom actions
- [Behavior Trees Overview](/technical-reference/behavior-trees/overview) - System architecture

## See Also

- Complete architecture document in repo: `Docs/Developer/BT_Graph_Editor_Architecture.md`
- Source code: `Source/AI/`
