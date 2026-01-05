# Blueprint Editor - Phases 2, 3 & 4 Implementation

## Overview

This document describes the implementation of the advanced Blueprint Editor features, including dynamic enum catalogs, visual node graph editing, and runtime entity inspection.

## Phase 2: Dynamic Enum Catalogues

### What Was Implemented

The enum catalog system provides dynamic type definitions for Actions, Conditions, and Decorators used in behavior trees and state machines.

**Backend Components:**
- `EnumCatalogManager`: Singleton manager that loads and manages type catalogs
- Three JSON catalog files in `Blueprints/Catalogues/`:
  - `ActionTypes.json`: 12 action types (MoveTo, Attack, Patrol, Wait, PlayAnimation, Flee, PickupItem, UseItem, Heal, SetVariable, SendMessage, Idle)
  - `ConditionTypes.json`: 12 condition types (HasTarget, IsLowHealth, IsTargetInRange, HasLineOfSight, IsAlive, HasItem, IsVariableTrue, IsInCombat, HeardNoise, IsPlayerNearby, CanAttack, HasReachedGoal)
  - `DecoratorTypes.json`: 12 decorator types (Inverter, Succeeder, Failer, Repeater, RepeatUntilFail, RepeatUntilSuccess, Cooldown, TimeLimit, RetryUntilSuccess, ForceSuccess, ForceFailure, AlwaysRunning)

**Features:**
- JSON-based type definitions with metadata (name, description, category, tooltip, parameters)
- Automatic loading on initialization
- Validation of catalog structure
- Hot-reload support (backend ready)
- Type lookup and validation API

### Usage Example

```cpp
// Load catalogs (done automatically on initialization)
EnumCatalogManager::Get().Initialize();

// Query action types
auto actions = EnumCatalogManager::Get().GetActionTypes();
for (const auto& actionId : actions) {
    const CatalogType* type = EnumCatalogManager::Get().FindActionType(actionId);
    std::cout << "Action: " << type->name << " - " << type->description << std::endl;
}

// Validate a type
bool valid = EnumCatalogManager::Get().IsValidConditionType("HasTarget");
```

## Phase 3: Node Graph Editor

### What Was Implemented

A complete visual node graph editor for Behavior Trees and Hierarchical Finite State Machines using ImNodes.

**Backend Components:**
- `NodeGraphManager`: Manages multiple node graphs simultaneously
- `NodeGraph`: Individual graph with nodes and connections
- `GraphNode`: Node data structure supporting various types (Sequence, Selector, Action, Condition, Decorator, State, Transition, Comment)
- `GraphLink`: Connection between nodes

**Frontend Components:**
- `NodeGraphPanel`: ImGui/ImNodes visual editor panel

**Features:**
- Multi-graph support with tabs (open multiple behavior trees/FSMs)
- Visual node editor with drag & drop
- Context menu for node creation (right-click)
- Automatic integration with enum catalogs (dynamic action/condition/decorator menus)
- Node connections via drag & drop
- Node positioning persistence
- Graph serialization (JSON)
- Support for both Behavior Trees and HFSM

### Usage Example

```cpp
// Create a new graph
int graphId = NodeGraphManager::Get().CreateGraph("Enemy AI", "BehaviorTree");

// Add nodes
NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
int rootId = graph->CreateNode(NodeType::BT_Selector, 100, 100, "Root");
int actionId = graph->CreateNode(NodeType::BT_Action, 100, 200, "Move To Player");

// Set action type (from catalog)
GraphNode* actionNode = graph->GetNode(actionId);
actionNode->actionType = "MoveTo";

// Link nodes
graph->LinkNodes(rootId, actionId);

// Save graph
NodeGraphManager::Get().SaveGraph(graphId, "Blueprints/AI/enemy_ai.json");
```

### UI Controls

- **Right-click on canvas**: Open node creation menu
- **Left-click & drag**: Select and move nodes
- **Drag from output to input**: Create connection
- **Tab bar**: Switch between multiple graphs
- **+ button**: Create new graph

## Phase 4: Dynamic Inspector & Runtime Entity List

### What Was Implemented

A complete runtime entity inspection system that syncs with the World ECS.

**Backend Components:**
- `EntityInspectorManager`: Tracks all runtime entities and provides inspection API
- `WorldBridge.cpp`: Decoupled bridge between World and editor
- World hooks in `World::CreateEntity()` and `World::DestroyEntity()`

**Frontend Components:**
- `EntitiesPanel`: Lists all runtime entities with filtering
- `InspectorPanel`: Adaptive inspector showing entity properties or node properties

**Features:**
- Automatic entity tracking (all entities created via World::CreateEntity())
- Real-time entity list updates
- Entity filtering by name
- Component listing for each entity
- Live property editing (Position, Velocity, etc.)
- Adaptive inspector (switches between node and entity inspection)
- Non-intrusive integration (no breaking changes to existing code)

### Usage Example

**Automatic Tracking:**
```cpp
// Creating an entity automatically notifies the editor
EntityID entity = World::Get().CreateEntity();
World::Get().AddComponent<Position_data>(entity);

// The entity immediately appears in the EntitiesPanel
// and can be selected for inspection
```

**Manual Inspection:**
```cpp
// Get all entities
auto entities = EntityInspectorManager::Get().GetAllEntities();

// Select an entity
EntityInspectorManager::Get().SetSelectedEntity(entityId);

// Get component properties
auto properties = EntityInspectorManager::Get().GetComponentProperties(entityId, "Position_data");

// Edit a property
EntityInspectorManager::Get().SetComponentProperty(entityId, "Position_data", "x", "123.45");
```

### UI Panels

**EntitiesPanel:**
- Shows count of all runtime entities
- Filter input for searching by name
- Clickable entity list
- Hover tooltip showing entity ID and components
- Selection synchronizes with inspector

**InspectorPanel:**
- Adaptive display based on selection:
  - Entity selected: Shows all components as collapsible headers
  - Node selected: Shows node properties (future enhancement)
  - Nothing selected: Shows help text
- Live property editing with ImGui controls:
  - Float properties: DragFloat
  - Int properties: DragInt
  - Bool properties: Checkbox
  - String properties: InputText
- Changes applied immediately to runtime entities

## Architecture

### Separation of Concerns

The implementation follows a strict backend/frontend separation:

**Backend (Business Logic):**
- `EnumCatalogManager`: Catalog management
- `NodeGraphManager`: Graph data and operations
- `EntityInspectorManager`: Entity tracking and inspection
- All managers are singletons with clean APIs

**Frontend (UI):**
- `NodeGraphPanel`: Visual graph editor
- `EntitiesPanel`: Entity list UI
- `InspectorPanel`: Property inspector UI
- All panels delegate data operations to backend managers

### Integration

The new features integrate seamlessly with existing systems:

1. **BlueprintEditor Backend**: Initializes all managers
2. **BlueprintEditorGUI**: Renders all panels
3. **World ECS**: Notifies editor of entity changes (optional compilation flag)

## Building

The project file has been updated with all new source files:
- EnumCatalogManager.cpp/h
- NodeGraphManager.cpp/h
- EntityInspectorManager.cpp/h
- WorldBridge.cpp
- NodeGraphPanel.cpp/h
- EntitiesPanel.cpp/h
- InspectorPanel.cpp/h

**Preprocessor Define:**
- `OLYMPE_BLUEPRINT_EDITOR_ENABLED` is added to all configurations
- This enables World notifications to the editor

## Testing Checklist

- [x] Catalog files load correctly
- [x] EnumCatalogManager initializes without errors
- [x] NodeGraphManager can create and manage graphs
- [x] EntityInspectorManager tracks entities
- [ ] World hooks notify editor on entity creation/destruction
- [ ] NodeGraphPanel renders and allows node creation
- [ ] EntitiesPanel displays all runtime entities
- [ ] InspectorPanel shows entity properties
- [ ] Property editing updates entity data in real-time
- [ ] Multi-graph tabs work correctly
- [ ] Context menu shows catalog types
- [ ] Graph serialization works (save/load)

## Future Enhancements

1. **Node Graph:**
   - Layout algorithms (auto-arrange)
   - Mini-map for large graphs
   - Node search/filter
   - Graph validation warnings

2. **Inspector:**
   - More component types support
   - Vector2/Vector3 specialized editors
   - Color picker for color properties
   - Reference pickers for entity references

3. **Entities Panel:**
   - Hierarchical view (parent-child relationships)
   - Component filtering
   - Bulk operations (delete multiple, copy, etc.)
   - Entity creation from editor

4. **Catalogs:**
   - UI for hot-reload button
   - Catalog editor (add/edit types without editing JSON)
   - Custom parameter types
   - Validation rules

## Files Added

**Catalogues:**
- `Blueprints/Catalogues/ActionTypes.json`
- `Blueprints/Catalogues/ConditionTypes.json`
- `Blueprints/Catalogues/DecoratorTypes.json`

**Backend:**
- `Source/BlueprintEditor/EnumCatalogManager.h/cpp`
- `Source/BlueprintEditor/NodeGraphManager.h/cpp`
- `Source/BlueprintEditor/EntityInspectorManager.h/cpp`
- `Source/BlueprintEditor/WorldBridge.cpp`

**Frontend:**
- `Source/BlueprintEditor/NodeGraphPanel.h/cpp`
- `Source/BlueprintEditor/EntitiesPanel.h/cpp`
- `Source/BlueprintEditor/InspectorPanel.h/cpp`

**Modified:**
- `Source/World.h` (added notification hooks)
- `Source/World.cpp` (added notification calls)
- `Source/BlueprintEditor/blueprinteditor.h` (no changes needed)
- `Source/BlueprintEditor/blueprinteditor.cpp` (added manager initialization)
- `Source/BlueprintEditor/BlueprintEditorGUI.h` (added panel members)
- `Source/BlueprintEditor/BlueprintEditorGUI.cpp` (added panel rendering)
- `Olympe Engine.vcxproj` (added new files, preprocessor define)

## Contact

For questions or issues with this implementation, please refer to the main BLUEPRINT_EDITOR_ARCHITECTURE.md document.
