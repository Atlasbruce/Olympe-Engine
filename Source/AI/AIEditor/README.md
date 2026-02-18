# AIEditorGUI - Complete AI Editor Interface

## Overview

AIEditorGUI is the main UI layer for the AI Editor, integrating NodeGraphCore and AIGraphPlugin_BT to provide a complete WYSIWYG editor for Behavior Trees and HFSM.

**Phase**: 1.3  
**Status**: Implementation Complete  
**Dependencies**: NodeGraphCore (Phase 1.1), AIGraphPlugin_BT (Phase 1.2)

## Architecture

### Components

1. **AIEditorGUI** - Main GUI class with 3-panel layout
2. **AIEditorPanels** - Specialized panels (Blackboard, Senses, Runtime Debug)
3. **AIEditorMenus** - Menus and actions
4. **AIEditorNodeRenderer** - ImNodes rendering with BT styling

### Layout

```
+----------------+----------------------------------+-----------------+
| Asset Browser  |         Node Graph               |   Inspector     |
|                |                                  |                 |
| - BT Files     |  [Tabs: Graph1 | Graph2]        | - Node Props    |
| - HFSM Files   |                                  | - Blackboard    |
| - Search       |  [Node Graph Canvas]             | - Senses        |
|                |                                  | - Runtime       |
+----------------+----------------------------------+-----------------+
```

## Features

### Core Features
- **3-Panel Layout**: Asset Browser (left), Node Graph (center), Inspector (right)
- **Multi-Graph Tabs**: Edit multiple graphs simultaneously
- **ImNodes Integration**: Professional node editor with zoom, pan, and grid
- **Command Pattern**: Full undo/redo support via command stack
- **BTNodePalette**: Drag-and-drop node creation from palette

### Menus
- **File**: New BT, New HFSM, Open, Save, Save As, Close
- **Edit**: Undo, Redo, Cut, Copy, Paste, Delete, Select All
- **View**: Toggle panels (Node Palette, Blackboard, Senses, Runtime Debug), Reset Layout
- **Help**: About

### AI-Specific Panels
- **Blackboard Inspector**: Visual editor for blackboard variables
- **Senses Debug**: AI perception debugging (vision, hearing)
- **Runtime Debug**: Live execution visualization with entity list

### Validation
- Real-time graph validation using BTGraphValidator
- Error/warning messages displayed in UI
- Prevents compilation of invalid graphs

## Usage

### Initialize Editor
```cpp
#include "AI/AIEditor/AIEditorGUI.h"

AIEditorGUI editor;
editor.Initialize();
```

### Render Loop
```cpp
while (running) {
    float deltaTime = CalculateDeltaTime();
    editor.Update(deltaTime);
    editor.Render();
}
```

### Shutdown
```cpp
editor.Shutdown();
```

### Creating a New Graph
```cpp
// Via menu action
editor.MenuAction_NewBT();

// Or directly via NodeGraphManager
NodeGraphManager& mgr = NodeGraphManager::Get();
GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
mgr.SetActiveGraph(id);
```

### Adding Nodes
```cpp
GraphDocument* doc = mgr.GetActiveGraph();
NodeId nodeId = doc->CreateNode("BT_Selector", Vector2(100, 100));
```

### Undo/Redo
```cpp
// Execute command via stack
auto cmd = std::make_unique<CreateNodeCommand>(doc, "BT_Action", Vector2(0, 0));
editor.GetCommandStack().ExecuteCommand(std::move(cmd));

// Undo
if (editor.GetCommandStack().CanUndo()) {
    editor.GetCommandStack().Undo();
}

// Redo
if (editor.GetCommandStack().CanRedo()) {
    editor.GetCommandStack().Redo();
}
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New Behavior Tree |
| Ctrl+Shift+N | New HFSM |
| Ctrl+O | Open File |
| Ctrl+S | Save |
| Ctrl+Shift+S | Save As |
| Ctrl+W | Close Tab |
| Ctrl+Z | Undo |
| Ctrl+Y | Redo |
| Ctrl+X | Cut |
| Ctrl+C | Copy |
| Ctrl+V | Paste |
| Delete | Delete Selected |
| Ctrl+A | Select All |
| F2 | Toggle AI Editor |
| F5 | Validate Graph |
| F6 | Compile to Runtime |

## Integration

### Dependencies
- **NodeGraphCore** - Graph document management and CRUD operations
- **AIGraphPlugin_BT** - BT node types, validation, and compilation
- **ImGui** - UI framework for panels and controls
- **ImNodes** - Node editor widgets for graph rendering

### Node Graph Integration
```cpp
// NodeGraphManager provides graph lifecycle
NodeGraphManager& mgr = NodeGraphManager::Get();

// Create graph
GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");

// Get active graph
GraphDocument* doc = mgr.GetActiveGraph();

// CRUD operations
NodeId nodeId = doc->CreateNode("BT_Selector", Vector2(0, 0));
doc->DeleteNode(nodeId);
doc->UpdateNodePosition(nodeId, Vector2(100, 100));
```

### BTNodeRegistry Integration
```cpp
// Get node type information
BTNodeRegistry& registry = BTNodeRegistry::Get();
const BTNodeTypeInfo* info = registry.GetNodeTypeInfo("BT_Selector");

// Query by category
auto composites = registry.GetNodeTypesByCategory(BTNodeCategory::Composite);
```

### Validation Integration
```cpp
// Validate graph
auto messages = BTGraphValidator::ValidateGraph(doc);

// Check for errors
for (const auto& msg : messages) {
    if (msg.severity == BTValidationSeverity::Error) {
        SYSTEM_LOG << "ERROR: " << msg.message << std::endl;
    }
}
```

## Testing

Run `TestAIEditorGUI.cpp` for integration tests:

### Test Suite
- **Test 1**: Initialize AIEditorGUI
- **Test 2**: Create New BT Graph
- **Test 3**: Load Existing BT
- **Test 4**: Node Creation via Palette
- **Test 5**: Undo/Redo
- **Test 6**: Validate Graph
- **Test 7**: Save/Load Roundtrip
- **Test 8**: Multi-Graph Tabs
- **Test 9**: Blackboard Panel
- **Test 10**: Node Palette Integration

### Running Tests
```bash
# Build tests
g++ -std=c++14 TestAIEditorGUI.cpp AIEditorGUI.cpp ...

# Run tests
./TestAIEditorGUI
```

Expected output:
```
========================================
AIEditorGUI Integration Tests (Phase 1.3)
========================================

[PASS] Test 1: Initialize AIEditorGUI
[PASS] Test 2: Create New BT Graph
...
[PASS] Test 10: Node Palette Integration

========================================
Results: 10 passed, 0 failed
========================================
```

## File Structure

```
Source/AI/AIEditor/
├── AIEditorGUI.h              # Main GUI class header
├── AIEditorGUI.cpp            # Main GUI implementation
├── AIEditorPanels.h           # Specialized panels header
├── AIEditorPanels.cpp         # Panels implementation
├── AIEditorMenus.h            # Menu handlers header
├── AIEditorMenus.cpp          # Menu implementation
├── AIEditorNodeRenderer.h     # Node rendering header
├── AIEditorNodeRenderer.cpp   # Node rendering implementation
├── TestAIEditorGUI.cpp        # Integration tests
└── README.md                  # This file
```

## Design Principles

### C++14 Compliance
- NO C++17/20 features (no std::optional, std::string_view, if constexpr, etc.)
- Traditional iterators: `for (auto it = container.begin(); it != container.end(); ++it)`
- Explicit type constructors

### JSON Handling
- Use JsonHelper functions exclusively:
  - `JsonHelper::GetInt(json, "key", default)`
  - `JsonHelper::GetString(json, "key", default)`
  - `JsonHelper::GetBool(json, "key", default)`
- NO direct nlohmann::json operator[] or .get<T>()

### Logging
- Use `SYSTEM_LOG` for all logging
- Format: `SYSTEM_LOG << "[AIEditorGUI] Message" << std::endl;`
- NO std::cout in production code (tests are exception)

### Namespace
All code in `namespace Olympe::AI`:
```cpp
namespace Olympe {
namespace AI {
    // Code here
} // namespace AI
} // namespace Olympe
```

### Memory Management
- Use `std::unique_ptr` for owned resources
- Use raw pointers for borrowed references
- No manual new/delete in application code

## ImNodes Best Practices

### Context Management
```cpp
// Create once in Initialize()
m_imnodesContext = ImNodes::CreateContext();
ImNodes::SetCurrentContext(static_cast<ImNodesContext*>(m_imnodesContext));

// Destroy in Shutdown()
ImNodes::DestroyContext(static_cast<ImNodesContext*>(m_imnodesContext));
```

### Node Rendering
```cpp
ImNodes::BeginNodeEditor();

// Render node
ImNodes::BeginNode(nodeId);
ImNodes::BeginNodeTitleBar();
ImGui::Text("Node Title");
ImNodes::EndNodeTitleBar();

ImNodes::BeginInputAttribute(inputPinId);
ImGui::Text("In");
ImNodes::EndInputAttribute();

ImNodes::BeginOutputAttribute(outputPinId);
ImGui::Text("Out");
ImNodes::EndOutputAttribute();

ImNodes::EndNode();

// Render links
ImNodes::Link(linkId, startPinId, endPinId);

ImNodes::EndNodeEditor();
```

### Pin ID Convention
- Input pin: `nodeId * 1000`
- Output pin: `nodeId * 1000 + 1`
- Child N: `nodeId * 1000 + 10 + N`

### Interaction Handling
```cpp
// Check link creation
int startPin, endPin;
if (ImNodes::IsLinkCreated(&startPin, &endPin)) {
    // Create link
}

// Check node selection
int numSelected = ImNodes::NumSelectedNodes();
if (numSelected > 0) {
    std::vector<int> selected(numSelected);
    ImNodes::GetSelectedNodes(selected.data());
}

// Check hover
int hoveredNode = -1;
if (ImNodes::IsNodeHovered(&hoveredNode)) {
    // Show tooltip
}
```

## Future Enhancements

### Phase 1.4 (Planned)
- File browser dialog for Open/Save
- Clipboard support (Cut/Copy/Paste)
- Node templates for common patterns
- Grid snap options
- Minimap for large graphs

### Phase 1.5 (Planned)
- Runtime debugging with breakpoints
- Variable watch window
- Performance profiler
- Hot reload for BT changes

### Phase 2.0 (Planned)
- Visual scripting integration
- AI behavior recording/playback
- Automated testing framework
- Multi-user collaboration

## Troubleshooting

### ImNodes Context Errors
**Problem**: Crash or assertion failure in ImNodes  
**Solution**: Ensure `ImNodes::SetCurrentContext()` is called before any ImNodes operations

### Node Not Rendering
**Problem**: Node doesn't appear in graph  
**Solution**: 
- Check node type is registered in BTNodeRegistry
- Verify node position is within visible area
- Check ImNodes::SetNodeGridSpacePos() is called

### Validation Errors Not Showing
**Problem**: Invalid graph doesn't show errors  
**Solution**: Call `BTGraphValidator::ValidateGraph()` and check message severity

### Undo/Redo Not Working
**Problem**: Undo/Redo has no effect  
**Solution**: Ensure all operations use CommandStack.ExecuteCommand(), not direct GraphDocument methods

## Contributing

When contributing to AIEditorGUI:
1. Follow C++14 coding standards
2. Use JsonHelper for all JSON access
3. Use SYSTEM_LOG for logging
4. Add tests for new features
5. Update this README for new functionality
6. Document all public APIs with Doxygen comments

## License

Copyright (c) 2026 Olympe Engine  
All rights reserved.

## Changelog

### v1.0 (2026-02-18) - Phase 1.3
- Initial implementation of AIEditorGUI
- 3-panel layout with Asset Browser, Node Graph, Inspector
- ImNodes integration for node rendering
- BTNodePalette integration for drag-and-drop
- Command pattern for undo/redo
- Menu system with keyboard shortcuts
- Specialized AI panels (Blackboard, Senses, Runtime Debug)
- 10 integration tests
- Complete documentation

## Contact

For questions or issues related to AIEditorGUI:
- Open an issue on GitHub
- Contact the Olympe Engine development team
- Refer to NodeGraphCore and AIGraphPlugin_BT documentation

---

**Last Updated**: 2026-02-18  
**Phase**: 1.3  
**Status**: Complete
