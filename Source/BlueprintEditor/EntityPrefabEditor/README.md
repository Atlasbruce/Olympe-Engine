# Entity Prefab Editor Module

## Overview

The Entity Prefab Editor Module extends the Olympe Engine Blueprint Editor v4 with visual editing capabilities for entity prefab files. This module follows the Plugin Architecture pattern and integrates seamlessly with existing Blueprint Editor infrastructure.

## Phase 1 - Initial Implementation (Read-Only Viewer)

**Status**: Under Development  
**Target**: MVP with visual prefab loading and display  
**Duration**: 8-10 days

### Phase 1 Goals

- ✅ Load entity prefab JSON files from `Gamedata/EntityPrefab/`
- ✅ Display prefabs as visual node graphs (entity center + component nodes)
- ✅ Support multi-file tab editing
- ✅ Implement zoom/pan on canvas
- ✅ Read-only visualization (editing in Phase 2+)

## Components

### Core Classes

#### 1. **PrefabLoader** (PrefabLoader.h/cpp)
Handles file I/O and JSON parsing for entity prefabs.

```cpp
// Load prefab from JSON file
EntityPrefab prefab = PrefabLoader::LoadFromFile("Gamedata/EntityPrefab/beacon.json");

// Access component schemas
const ComponentSchema* schema = PrefabLoader::GetComponentSchema("Transform");
```

**Responsibilities**:
- Load/save prefab JSON files
- Parse JSON into EntityPrefab data structures
- Manage component schema cache
- Validate prefabs against schemas

**Dependencies**: nlohmann/json, FileIO system

#### 2. **EntityPrefabGraphDocument** (EntityPrefabGraphDocument.h/cpp)
Extends GraphDocument to represent prefabs as node graphs.

```cpp
auto doc = std::make_shared<EntityPrefabGraphDocument>();
doc->LoadFromPrefab(prefab);
doc->AutoLayoutComponents(300.0f);  // Circle layout with 300px radius
```

**Responsibilities**:
- Convert prefab JSON to node graph representation
- Manage component nodes and their positions
- Handle selection and validation
- Support undo/redo via CommandStack

**Dependencies**: GraphDocument (NodeGraphCore), PrefabLoader

#### 3. **ComponentNodeData** (ComponentNodeData.h)
Represents a single component node visually.

**Properties**:
- Component type and name
- Position and size on canvas
- State (normal, hover, selected, error, warning, disabled)
- Input/output pins for connections
- Custom properties

#### 4. **PrefabCanvas** (PrefabCanvas.h/cpp)
ImGui wrapper for prefab visualization using ImNodes.

```cpp
auto canvas = std::make_shared<PrefabCanvas>(doc);
canvas->Render(width, height);
canvas->Pan(offset);
canvas->Zoom(delta, centerPoint);
```

**Responsibilities**:
- Render component nodes on canvas
- Handle mouse/keyboard input
- Manage zoom/pan view state
- Convert between screen and world coordinates
- Selection management

**Dependencies**: ImGui, ImNodes, EntityPrefabGraphDocument

#### 5. **ComponentNodeRenderer** (ComponentNodeRenderer.h/cpp)
Rendering logic for component nodes.

**Responsibilities**:
- Draw node shapes, text, pins
- Apply node styles and colors
- Handle visual state (hover, selected, error)
- Icon management

#### 6. **ParameterSchemaRegistry** (ParameterSchemaRegistry.h)
Singleton for managing component parameter schemas.

```cpp
auto& registry = ParameterSchemaRegistry::Get();
registry.LoadSchemasFromDirectory("Assets/Schemas/");
auto schema = registry.GetSchema("Transform");
```

**Responsibilities**:
- Cache component schemas
- Provide schema queries and validation
- Manage component categories
- Search and filtering

#### 7. **ComponentLibrary** (ParameterSchemaRegistry.h)
High-level component queries and creation.

```cpp
auto& library = ComponentLibrary::Get();
auto components = library.GetAvailableComponents();
auto component = library.CreateComponent("Transform", "root");
```

#### 8. **PropertyInspectorPrefab** (PropertyInspectorPrefab.h/cpp)
Property editing UI panel for selected components.

**Responsibilities**:
- Display selected component properties
- Render property editors by type (int, float, bool, Vector3, enum)
- Handle property validation
- Trigger callbacks on changes
- Read-only mode for Phase 1

#### 9. **EntityPrefabEditorPlugin** (EntityPrefabEditorPlugin.h/cpp)
Main plugin class extending BlueprintEditorPlugin.

**Responsibilities**:
- Plugin lifecycle (init, shutdown)
- Tab routing and creation
- Main editor UI layout
- Coordinate all sub-components

## File Structure

```
Source/BlueprintEditor/EntityPrefabEditor/
├── PrefabLoader.h/cpp                        // File I/O & schema management
├── EntityPrefabGraphDocument.h/cpp           // Graph document model
├── ComponentNodeData.h                       // Visual node representation
├── PrefabCanvas.h/cpp                        // Canvas rendering
├── ComponentNodeRenderer.h/cpp               // Node rendering logic
├── ParameterSchemaRegistry.h/cpp             // Schema caching
├── PropertyInspectorPrefab.h/cpp             // Property editor UI
├── EntityPrefabEditorPlugin.h/cpp            // Main plugin class
└── README.md                                  // This file
```

## Integration Points

### 1. Asset Browser
**File**: `Source/BlueprintEditor/AssetBrowser.cpp`

Add scanning of `Gamedata/EntityPrefab/` directory:
```cpp
void AssetBrowser::ScanPrefabDirectory(const std::string& path)
{
    // Scan Gamedata/EntityPrefab/ for .json files
    // Add to asset tree UI
}
```

### 2. Tab Manager
**File**: `Source/BlueprintEditor/TabManager.cpp`

Handle prefab tab creation on double-click:
```cpp
void TabManager::OnPrefabDoubleClicked(const std::string& filePath)
{
    auto tab = CreateTab(EditorTabType::PREFAB_EDITOR, filePath);
    // Route to EntityPrefabEditorPlugin::RenderEditor()
}
```

### 3. Blueprint Editor Backend
**File**: `Source/BlueprintEditor/blueprinteditor.h`

Register plugin in initialization:
```cpp
auto prefabPlugin = std::make_unique<EntityPrefabEditorPlugin>();
RegisterPlugin(std::move(prefabPlugin));
```

## Data Structures

### EntityPrefab (PrefabLoader.h)
```cpp
struct EntityPrefab {
    int schemaVersion = 4;
    std::string name;
    std::string author;
    std::string created;
    std::string modified;
    std::vector<ComponentData> components;
    std::map<std::string, std::string> metadata;
};
```

### ComponentData (PrefabLoader.h)
```cpp
struct ComponentData {
    std::string type;              // e.g., "Transform", "Mesh", "Physics"
    std::string name;              // e.g., "root", "collider"
    std::map<std::string, std::string> properties;
    bool enabled = true;
};
```

### ComponentSchema (PrefabLoader.h)
```cpp
struct ComponentSchema {
    std::string componentType;
    std::string category;
    std::string description;
    std::vector<ParameterDefinition> parameters;
};
```

## Usage Examples

### Example 1: Load and Display a Prefab

```cpp
// In EntityPrefabEditorPlugin::RenderEditor()

// 1. Load prefab from file
EntityPrefab prefab = PrefabLoader::LoadFromFile(filePath);

// 2. Create graph document
auto doc = std::make_shared<EntityPrefabGraphDocument>();
doc->LoadFromPrefab(prefab);
doc->AutoLayoutComponents(300.0f);

// 3. Create canvas
auto canvas = std::make_shared<PrefabCanvas>(doc);

// 4. Render
canvas->Render(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
```

### Example 2: Handle Node Selection

```cpp
// In PrefabCanvas::HandleMouseInput()

NodeGraph::NodeId clickedNode = GetNodeAtPosition(mousePos);
if (clickedNode != NodeGraph::InvalidNodeId) {
    SelectNode(clickedNode);

    // Update property inspector
    auto nodeData = document->GetComponentData(clickedNode);
    propertyInspector.SetSelectedNode(nodeData);
}
```

### Example 3: Query Component Schemas

```cpp
auto& library = ComponentLibrary::Get();

// List all available components
auto components = library.GetAvailableComponents();

// Get specific component schema
auto schema = library.GetComponentSchema("Transform");

// Validate component
bool valid = library.ValidateComponent(componentData);
```

## Phase Roadmap

### Phase 1: Basic Visualization (8-10 days) ✅ In Progress
- Load and display prefabs read-only
- Multi-tab support
- Zoom/pan canvas
- Property inspector (read-only)

### Phase 2: Component Library (7-8 days)
- Interactive component library UI
- Component search and filtering
- Drag-drop from library

### Phase 3: Basic Editing (8-10 days)
- Add/remove components
- Edit component properties
- Live validation

### Phase 4: Connections (5-7 days)
- Create connections between components
- Visual connection feedback

### Phase 5: Save & Undo (7-8 days)
- Save prefab changes
- Undo/Redo support
- Diff and merge

### Phase 6: Advanced Features (5-7 days)
- Auto-layout algorithms
- Prefab hierarchy
- Template system

## Dependencies

### Required Libraries
- **nlohmann/json** - JSON parsing (header-only, in Source/third_party/)
- **ImGui** - UI rendering (existing)
- **ImNodes** - Graph visualization (existing)
- **glm** - Math library (existing)

### Internal Dependencies
- **GraphDocument** (NodeGraphCore) - Base class
- **BlueprintEditorPlugin** - Plugin interface
- **FileIO system** - File operations
- **ValidationSystem** - Schema validation

### No External Dependencies Added ✅

## Build Integration

### CMake Configuration
```cmake
add_library(EntityPrefabEditor
    Source/BlueprintEditor/EntityPrefabEditor/PrefabLoader.cpp
    Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.cpp
    Source/BlueprintEditor/EntityPrefabEditor/PrefabCanvas.cpp
    Source/BlueprintEditor/EntityPrefabEditor/ComponentNodeRenderer.cpp
    Source/BlueprintEditor/EntityPrefabEditor/ParameterSchemaRegistry.cpp
    Source/BlueprintEditor/EntityPrefabEditor/PropertyInspectorPrefab.cpp
    Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabEditorPlugin.cpp
)

target_link_libraries(EntityPrefabEditor
    PUBLIC BlueprintEditor NodeGraphCore
    PRIVATE nlohmann_json
)
```

## Testing Strategy

### Phase 1 Tests

#### Unit Tests
1. **PrefabLoader**: JSON parsing, schema validation
2. **EntityPrefabGraphDocument**: Node creation, layout calculation
3. **ComponentNodeData**: Property management, state transitions
4. **ParameterSchemaRegistry**: Schema caching, queries

#### Integration Tests
1. **Load & Display**: Load prefab → Create graph → Render canvas
2. **Multi-Tab**: Open multiple prefabs in different tabs
3. **Selection**: Select nodes → Update property inspector

#### Manual Tests
1. Open beacon.json in Asset Browser
2. Display as visual node graph
3. Zoom/pan on canvas
4. Open multiple prefabs
5. Select components and view properties
6. Check for memory leaks (20+ components)

### Performance Targets (Phase 1)
- Load time: < 100ms
- Render FPS: 60+ with 20 nodes
- Memory per prefab: < 100 MB
- No memory leaks

## Success Criteria (Phase 1)

- ✅ Load beacon.json without errors
- ✅ Display 5+ component nodes visually
- ✅ Support zoom/pan on canvas
- ✅ Open multiple prefabs in tabs
- ✅ 60 FPS with 20 components
- ✅ No memory leaks (validated with ASAN)
- ✅ Unit tests pass (> 70% coverage)
- ✅ Code reviewed and approved

## Development Guidelines

### Code Style
- Follow existing Olympe Engine C++ conventions
- Use camelCase for methods/variables
- Use UPPER_CASE for constants
- Add doxygen comments for public APIs

### Documentation
- Document all public methods
- Add usage examples in README
- Update this file with changes
- Keep architecture decisions documented

### Testing
- Unit test coverage > 70%
- Write tests before/alongside code
- Test error cases and edge conditions
- Use existing test framework

### Code Review
- All PRs require review before merge
- Review against architecture plan
- Verify no memory leaks
- Check performance targets

## Architecture Decision Log

### Decision 1: Plugin Architecture
**Why**: Allows extending Blueprint Editor without modifying core code  
**Alternative**: Direct integration (higher risk)  
**Status**: ✅ Approved

### Decision 2: Reuse GraphDocument
**Why**: 80% infrastructure already exists, proven pattern  
**Alternative**: Custom graph model (higher development effort)  
**Status**: ✅ Approved

### Decision 3: Command Pattern for Undo/Redo
**Why**: Already implemented in CommandStack, proven pattern  
**Alternative**: Custom undo system (duplicate effort)  
**Status**: ✅ Approved

### Decision 4: Read-Only MVP for Phase 1
**Why**: Reduces scope to 8-10 days, gets feedback early  
**Alternative**: Full editing in Phase 1 (30+ days)  
**Status**: ✅ Approved

## Future Enhancements

- [ ] Prefab hierarchy/nesting
- [ ] Template system
- [ ] Advanced layout algorithms
- [ ] Diff/merge capabilities
- [ ] Version control integration
- [ ] Collaboration features
- [ ] Performance optimization for 100+ components
- [ ] Custom component plugin system

## Contact & Support

For questions about this module:
- See: Documentation/ENTITY_PREFAB_EDITOR_INTEGRATION_PLAN.md
- See: Documentation/ENTITY_PREFAB_EDITOR_PHASE1_IMPLEMENTATION.md
- See: Documentation/ENTITY_PREFAB_EDITOR_CODE_STRUCTURE.md

---

**Created**: 2026-04-02  
**Version**: Phase 1 - Initial Implementation  
**Status**: ✅ Ready to Implement
