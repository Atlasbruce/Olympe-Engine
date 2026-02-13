---
id: modules
title: "Engine Modules"
sidebar_label: "Modules"
---

# Engine Modules

Olympe Engine is organized into several major modules, each with specific responsibilities. This modular architecture makes the engine maintainable, testable, and extensible.

## Core Engine

### GameEngine

**File**: `Source/GameEngine.h`, `Source/GameEngine.cpp`

The `GameEngine` class is the top-level singleton that orchestrates the entire engine. It:

- Manages the main game loop
- Owns and coordinates major subsystems (InputsManager, VideoGame, DataManager)
- Provides access to the main SDL renderer
- Tracks frame delta time for smooth updates

```cpp
GameEngine& engine = GameEngine::GetInstance();
engine.Initialize();
engine.Process(); // Called every frame
```

### World

**File**: `Source/World.h`, `Source/World.cpp`

The `World` class implements the ECS architecture and manages the game world state:

- **Entity Management**: Creates, destroys, and tracks all entities
- **Component Storage**: Manages component pools for all component types
- **System Registry**: Registers and updates all ECS systems
- **Level Loading**: Loads levels from Tiled TMX files via `TiledLevelLoader`
- **Prefab System**: Instantiates entities from JSON blueprints
- **Tileset Management**: Handles tile-based rendering

Key responsibilities:
- Entity lifecycle management
- Component registration and access
- System execution ordering
- Level state management

```cpp
World& world = World::GetInstance();
EntityID entity = world.CreateEntity();
world.AddComponent<Position_data>(entity, position);
```

### VideoGame

**File**: `Source/VideoGame.h`, `Source/VideoGame.cpp`

Manages high-level game state and player management:

- Player addition/removal
- Game state transitions (play, pause, quit)
- Event handling coordination
- Game-specific logic coordination

## ECS Systems

All systems inherit from `ECS_System` and process entities with specific component signatures.

### Rendering System

**Files**: `Source/ECS_Systems.cpp`, `Source/Rendering/IsometricRenderer.cpp`

Handles all entity rendering:

- Sprite rendering with transform
- Multi-layer rendering with z-ordering
- Camera-relative rendering
- Isometric projection support
- Tile layer rendering

Key functions:
- `RenderEntitiesForCamera()`: Renders all visible entities for a camera
- `RenderMultiLayerForCamera()`: Handles layered rendering with depth sorting

### AI System

**Files**: `Source/ECS_Systems_AI.cpp`, `Source/ECS_Systems_AI.h`

Processes AI entities using behavior trees:

- Evaluates behavior tree nodes
- Updates AI blackboard data
- Handles AI state transitions
- Processes patrol routes and waypoints

Requires components:
- `AIBlackboard_data`
- `AIBehaviorTree_data`
- `Position_data`

### Physics System

Handles physics simulation and collision:

- Velocity integration
- Collision detection using bounding boxes
- Collision response
- Trigger volume detection

Requires components:
- `Position_data`
- `Velocity_data`
- `BoundingBox_data`

### Camera System

**Files**: `Source/ECS_Systems_Camera.cpp`, `Source/ECS_Systems_Camera.h`

Manages camera behavior and viewport:

- Camera following (follow player entities)
- Camera bounds and constraints
- Viewport transformation
- Multi-camera support for split-screen

Requires components:
- `CameraFollow_data`
- `CameraTransform_data`

### Input System

**Files**: `Source/ECS_Systems.cpp`, `Source/InputsManager.cpp`

Processes player input and updates entity state:

- **InputEventConsumeSystem**: Consumes input events and updates ECS components
- **InputSystem**: Processes entities with `Controller_data` components
- **GameEventConsumeSystem**: Handles gameplay events (pause, quit, etc.)

Integrates with:
- Keyboard, mouse, and gamepad input
- Player binding system
- Input profiles and action mapping

## Prefab Factory

**File**: `Source/PrefabFactory.cpp`, `Source/prefabfactory.h`

Creates entities from JSON blueprint definitions:

- Parses JSON blueprint files
- Instantiates entities with specified components
- Handles component overrides for level instances
- Validates blueprint schemas

Used for data-driven entity creation without hardcoding in C++.

```cpp
EntityID guard = PrefabFactory::CreateEntityFromBlueprint(
    "Blueprints/EntityPrefab/guard.json"
);
```

### PrefabScanner

**File**: `Source/PrefabScanner.cpp`, `Source/PrefabScanner.h`

Scans directories for blueprint files and maintains a registry:

- Discovers available blueprints
- Validates blueprint files
- Provides blueprint metadata to editors
- Supports hot-reloading of blueprints

## Blueprint Editor

**Files**: `Source/BlueprintEditor/*.cpp`, `OlympeBlueprintEditor/`

Visual editor for creating and modifying entity blueprints:

- ImGui-based interface
- Component editing
- Blueprint validation
- Real-time preview
- Dependency scanning (for behavior trees)

Standalone application: `OlympeBlueprintEditor.exe`

## Tiled Level Loader

**Files**: `Source/TiledLevelLoader/*`

Loads and parses Tiled TMX map files:

- Parses Tiled TMX XML format
- Loads tilesets and tile layers
- Extracts entity instances from object layers
- Handles layer properties (z-order, visibility)
- Converts Tiled coordinates to engine coordinates

Integration:
- Called by `World::LoadLevel()`
- Creates tile chunks for rendering
- Instantiates entity prefabs from Tiled objects

## Input System

**Files**: `Source/InputsManager.cpp`, `Source/InputDevice.cpp`, `Source/InputConfigLoader.cpp`

Multi-device input management:

### Device Management
- Keyboard input via `KeyboardManager`
- Gamepad input via `JoystickManager`
- Mouse input via `MouseManager`
- Hot-plug support (add/remove controllers at runtime)

### Input Mapping
- JSON-based input profiles (`Config/olympe-config.json`)
- Action mapping (jump, attack, move, etc.)
- Player binding to devices
- Context-aware input (gameplay vs UI)

### Integration
- Publishes input events to `EventQueue`
- Systems consume events via `InputEventConsumeSystem`
- Updates `Controller_data` components on entities

## Navigation System

**Files**: `Source/AI/*.cpp`, `Source/system/NavigationGrid.h`

Provides pathfinding and navigation:

- Grid-based pathfinding (A* algorithm)
- Navigation mesh support
- Dynamic obstacle avoidance
- Waypoint and patrol route systems

Used by:
- AI behavior trees for movement
- Player character for click-to-move
- Projectiles for homing behavior

### Pathfinding Integration
- `PathfindingSystem`: Processes pathfinding requests
- `MoveIntent_data`: Component for movement intentions
- `NavigationGrid`: Grid representation for pathfinding

## Behavior Trees

**Files**: `Source/AI/BehaviorTree.cpp`, `Source/AI/BehaviorTree.h`

Data-driven AI decision-making:

- Node types: Selector, Sequence, Condition, Action
- JSON-based tree definitions
- Built-in conditions: target detection, range checks, health
- Built-in actions: move, attack, patrol, wander
- Blackboard for AI memory

Common behaviors:
- **Wander**: Random exploration
- **Patrol**: Follow waypoint routes
- **Chase**: Pursue target
- **Flee**: Escape from threat

See [Behavior Trees Overview](../behavior-trees/overview.md) for details.

## Data Manager

**File**: `Source/DataManager.cpp`, `Source/DataManager.h`

Centralized data and resource management:

- Texture loading and caching
- Asset lifetime management
- Resource path resolution
- Shared resource access

```cpp
DataManager& dm = DataManager::GetInstance();
SDL_Texture* texture = dm.LoadTexture("Resources/sprite.png");
```

## Module Dependencies

```
GameEngine
  ├── InputsManager
  │     ├── KeyboardManager
  │     ├── JoystickManager
  │     └── MouseManager
  ├── VideoGame
  │     └── World
  │           ├── ECS Systems
  │           │     ├── InputSystem
  │           │     ├── AISystem
  │           │     ├── PhysicsSystem
  │           │     ├── CameraSystem
  │           │     └── RenderingSystem
  │           ├── PrefabFactory
  │           │     └── PrefabScanner
  │           ├── TiledLevelLoader
  │           └── TilesetManager
  ├── DataManager
  └── OptionsManager
```

## Build Configuration

From `CMakeLists.txt`:

- **C++ Standard**: C++14
- **Main Targets**:
  - `OlympeEngine`: Runtime game executable
  - `OlympeBlueprintEditor`: Standalone blueprint editor
  - `OlympeCore`: Shared library for editor functionality
  - `TiledLevelLoader`: Static library for Tiled integration

Build options:
```cmake
option(BUILD_RUNTIME_ENGINE "Build runtime engine executable" ON)
option(BUILD_BLUEPRINT_EDITOR_STANDALONE "Build Blueprint Editor Standalone executable" ON)
```

## Next Steps

- Dive deeper into the [ECS Architecture](./ecs-overview.md)
- Learn about [Behavior Trees](../behavior-trees/overview.md) for AI
- Explore [Input System Configuration](../../user-guide/input-system/configuration.md)
- Read [Creating Blueprints](../../user-guide/blueprints/overview.md)
