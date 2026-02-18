# NodeGraphCore Module

## Overview

NodeGraphCore is a generic, reusable node graph system for the Olympe Engine. It provides a unified foundation for all node-based editors including:
- Behavior Tree Editor
- HFSM (Hierarchical Finite State Machine) Editor  
- Animation Graph Editor
- Blueprint Editor

## Features

- **Generic Node Graph Data Model**: Unified structures (nodes, links, pins) that work across all editor types
- **CRUD Operations**: Complete Create, Read, Update, Delete operations for nodes and links
- **Multi-Graph Management**: Singleton manager supporting multiple open graphs with tab-like interface
- **Undo/Redo System**: Command pattern implementation for all operations
- **JSON v2 Schema**: Unified serialization format with automatic migration from legacy formats
- **Validation**: Graph structure validation and cycle detection
- **Layout Engine Interface**: Abstract interface for pluggable layout algorithms

## Architecture

### Core Components

#### 1. NodeGraphCore.h
Basic data structures:
- `GraphId`, `NodeId`, `PinId`, `LinkId`: Type-safe ID wrappers
- `Vector2`: 2D position
- `NodeData`: Node information (type, position, parameters, children)
- `PinData`: Pin information for connections
- `LinkData`: Link between two pins
- `EditorState`: Viewport state (zoom, scroll, selection)

#### 2. GraphDocument
Main document class for a single graph:
```cpp
GraphDocument doc;
doc.type = "AIGraph";
doc.graphKind = "BehaviorTree";

// Create nodes
NodeId id = doc.CreateNode("BT_Selector", Vector2(100, 100));

// Update position
doc.UpdateNodePosition(id, Vector2(150, 150));

// Validate
std::string error;
bool valid = doc.ValidateGraph(error);

// Serialize
json j = doc.ToJson();
```

#### 3. NodeGraphManager
Singleton for managing multiple graphs:
```cpp
NodeGraphManager& mgr = NodeGraphManager::Get();

// Create new graph
GraphId g1 = mgr.CreateGraph("AIGraph", "BehaviorTree");

// Load from file
GraphId g2 = mgr.LoadGraph("path/to/file.json");

// Set active
mgr.SetActiveGraph(g1);

// Save
mgr.SaveGraph(g1, "output.json");
```

#### 4. CommandSystem
Undo/redo support:
```cpp
CommandStack stack;

// Execute command
auto cmd = std::make_unique<CreateNodeCommand>(&doc, "BT_Action", Vector2(0, 0));
stack.ExecuteCommand(std::move(cmd));

// Undo/Redo
if (stack.CanUndo()) stack.Undo();
if (stack.CanRedo()) stack.Redo();
```

Available commands:
- `CreateNodeCommand`: Create a new node
- `DeleteNodeCommand`: Delete a node
- `ConnectPinsCommand`: Create a link
- `MoveNodeCommand`: Move a node

#### 5. GraphMigrator
Automatic JSON version migration:
```cpp
// Automatically detects version and migrates
json oldJson = LoadJsonFile("legacy.json");
GraphDocument doc = GraphMigrator::LoadWithMigration(oldJson);
```

Supported formats:
- **v0**: Legacy BehaviorTree format (no schema_version field)
- **v1**: Blueprint v1 format
- **v2**: Unified format (current)

#### 6. ILayoutEngine
Abstract interface for layout algorithms:
```cpp
class MyLayoutEngine : public ILayoutEngine {
public:
    std::vector<NodeLayout> ComputeLayout(
        const GraphDocument* graph,
        const LayoutParams& params) override
    {
        // Implement layout algorithm
    }
};
```

## JSON v2 Schema

```json
{
  "schemaVersion": 2,
  "type": "AIGraph",
  "graphKind": "BehaviorTree",
  "metadata": {
    "author": "Atlasbruce",
    "created": "2026-02-18T10:00:00Z",
    "tags": ["AI", "Combat"]
  },
  "editorState": {
    "zoom": 1.0,
    "scrollOffset": { "x": 0, "y": 0 },
    "selectedNodes": [],
    "layoutDirection": "TopToBottom"
  },
  "data": {
    "rootNodeId": 1,
    "nodes": [
      {
        "id": 1,
        "type": "BT_Selector",
        "name": "Root",
        "position": { "x": 200.0, "y": 100.0 },
        "children": [2, 3],
        "parameters": {}
      }
    ],
    "links": [
      {
        "id": 1,
        "fromPin": { "nodeId": 1, "pinId": "child0" },
        "toPin": { "nodeId": 2, "pinId": "parent" }
      }
    ]
  }
}
```

## Coding Standards

All code in this module follows strict C++14 compliance:

### ✅ Allowed
- `auto` type inference
- Range-based for loops
- Lambda expressions
- `enum class`
- `override`, `final`

### ❌ Forbidden
- `std::optional`, `std::variant`, `std::any` (C++17)
- Structured bindings: `auto& [k, v]` (C++17)
- `if constexpr` (C++17)
- `std::string_view` (C++17)

### JSON Usage
Always use JSON helpers from `json_helper.h`:
```cpp
// ✅ Correct
int value = JsonHelper::GetInt(json, "key", defaultValue);

// ❌ Wrong
int value = json["key"].get<int>();
```

### Namespace
All code must be in `namespace Olympe::NodeGraph`:
```cpp
namespace Olympe {
namespace NodeGraph {

// Implementation

} // namespace NodeGraph
} // namespace Olympe
```

### Logging
Use `SYSTEM_LOG` instead of `std::cout`:
```cpp
SYSTEM_LOG << "[NodeGraphCore] Message" << std::endl;
```

## Testing

Basic tests are provided in `TestNodeGraphCore.cpp`:
- Test 1: Create GraphDocument
- Test 2: CRUD Nodes
- Test 3: Connect Pins
- Test 4: Serialization
- Test 5: Multi-graph
- Test 6: Undo/Redo
- Test 7: HasCycles
- Test 8: ValidateGraph

## Integration

To use NodeGraphCore in your editor:

1. Include the headers:
```cpp
#include "NodeGraphCore/NodeGraphCore.h"
#include "NodeGraphCore/GraphDocument.h"
#include "NodeGraphCore/NodeGraphManager.h"
```

2. Create or load a graph:
```cpp
NodeGraphManager& mgr = NodeGraphManager::Get();
GraphId graphId = mgr.LoadGraph("my_graph.json");
GraphDocument* doc = mgr.GetGraph(graphId);
```

3. Perform operations with undo support:
```cpp
CommandStack commandStack;

auto cmd = std::make_unique<CreateNodeCommand>(
    doc, "MyNodeType", Vector2(x, y));
commandStack.ExecuteCommand(std::move(cmd));
```

4. Save when done:
```cpp
mgr.SaveGraph(graphId, "output.json");
```

## Future Extensions

The modular design allows for easy extension:
- Custom node types via `NodeData::type` field
- Custom validation rules
- Custom layout engines via `ILayoutEngine`
- Custom commands for specialized operations

## Performance

- CRUD operations: < 1ms
- JSON serialization (50 nodes): < 50ms
- Undo/Redo: < 1ms
- Cycle detection: O(V+E) where V=nodes, E=edges

## License

Part of Olympe Engine V2 - 2025-2026
