---
id: project-structure
title: Project Structure
sidebar_label: Project Structure
---

# Project Structure

This guide explains the directory structure and organization of the Olympe Engine project to help you navigate the codebase effectively.

## Root Directory Overview

```
Olympe-Engine/
├── Source/                 # Core engine source code
├── Blueprints/            # Blueprint definitions (behavior trees, logic graphs)
├── Resources/             # Assets (sprites, icons, music)
├── Gamedata/              # Game-specific data files
├── Config/                # Configuration files
├── Documentation/         # Technical documentation
├── Templates/             # Project templates
├── Examples/              # Example projects and demos
├── OlympeBlueprintEditor/ # Blueprint editor application source
├── OlympeTilemapEditor/   # Tilemap editor application source
├── build/                 # Build output directory (generated)
├── CMakeLists.txt         # CMake build configuration
└── olympe.ini             # Engine runtime configuration
```

## Main Directories

### Source/

The `Source/` directory contains all core engine C++ source code, organized by functionality:

#### Core Engine Systems
- **AI/** - Artificial Intelligence system
  - `BehaviorTree.cpp/h` - Behavior tree implementation
  - `BehaviorTreeDependencyScanner.cpp/h` - Dependency analysis for behavior trees
  - `AIEvents.h` - AI event definitions

- **system/** - Low-level system management
  - Camera management (`CameraManager`, `CameraEventHandler`)
  - Input handling (`KeyboardManager`, `MouseManager`, `JoystickManager`)
  - Event system (`EventManager`, `EventQueue`)
  - Viewport management (`ViewportManager`)
  - Game menus (`GameMenu`, `SystemMenu`)
  - Logging utilities (`log_sink.h`)

- **Rendering/** - Graphics rendering systems
  - `IsometricRenderer.cpp/h` - Isometric view rendering

#### Editor Components
- **BlueprintEditor/** - Blueprint visual editor implementation
- **BlueprintEditorStandalone/** - Standalone blueprint editor entry point
  - `BlueprintEditorStandaloneMain.cpp` - Main entry for standalone editor

- **OlympeTilemapEditor/** - Tilemap editor for level design

#### Level Loading
- **TiledLevelLoader/** - Integration with Tiled map editor
  - Loads `.tmx` files and converts them to engine format

#### Game Framework
- **ECS (Entity Component System)**
  - `ECS_Entity.h` - Entity definitions
  - `ECS_Components.h` - Core component definitions
  - `ECS_Components_AI.h` - AI-specific components
  - `ECS_Components_Camera.h` - Camera components
  - `ECS_Systems.cpp/h` - Core system implementations
  - `ECS_Systems_AI.cpp/h` - AI system implementations
  - `ECS_Systems_Camera.cpp/h` - Camera system implementations
  - `ECS_Systems_Rendering_Camera.cpp/h` - Camera rendering systems
  - `ECS_Register.h` - Component registration

- **Game Management**
  - `GameEngine.cpp/h` - Main engine class
  - `GameState.cpp/h` - Game state management
  - `World.cpp/h` - World/scene management
  - `VideoGame.cpp/h` - Game application layer
  - `Level.h` - Level definitions

- **Data Management**
  - `DataManager.cpp/h` - Asset and data loading
  - `PrefabScanner.cpp/h` - Scans and indexes prefab definitions
  - `PrefabFactory.cpp` - Factory for instantiating prefabs
  - `ComponentDefinition.cpp/h` - Component metadata and definitions

- **Input System**
  - `InputsManager.cpp/h` - Input handling and mapping
  - `InputConfigLoader.cpp/h` - Loads input configurations
  - `InputDevice.cpp/h` - Input device abstraction
  - `InputsInspectorPanel.cpp` - Debug panel for input inspection

- **Graphics**
  - `Sprite.cpp/h` - Sprite rendering
  - `drawing.cpp/h` - Low-level drawing primitives
  - `RenderContext.cpp/h` - Rendering context management

- **Utilities**
  - `engine_utils.cpp/h` - General engine utilities
  - `vector.cpp/h` - Vector math
  - `ParameterResolver.cpp/h` - Parameter resolution system
  - `ParameterSchema.cpp/h` - Parameter schema definitions
  - `Serialization.h` - Serialization utilities
  - `json_helper.h` - JSON utilities
  - `Singleton.h` - Singleton pattern implementation

- **Quest System**
  - `Quest.h` - Quest definitions
  - `QuestManager.h` - Quest state management
  - `Task.h` - Quest task definitions
  - `Objective.h` - Quest objective tracking

- **Third-party Libraries**
  - **third_party/imgui/** - Dear ImGui UI library (for editor interface)
  - **third_party/imnodes/** - ImNodes node editor (for blueprint graphs)
  - **third_party/nlohmann/** - JSON for Modern C++ library

#### Main Entry Points
- `OlympeEngine.cpp` - Main entry point for the runtime engine

### Blueprints/

Contains blueprint definitions in JSON format. Blueprints define:
- Behavior trees for AI
- Entity logic graphs
- Reusable game logic patterns

These files are loaded at runtime and edited with the Blueprint Editor.

### Resources/

Game assets and resources:
- **Sprites/** - Image assets for rendering
- **Icons/** - Application icons and UI icons
- Audio files (`system_music.mp3`)
- Logo images (`olympe_logo*.png`, `olympe_logo*.jpg`)
- Application icon (`olympe.ico`)

### Gamedata/

Game-specific data files such as:
- Level definitions
- Entity data
- Game configuration
- Saved game states

### Config/

Configuration files for the engine:
- **Inputs.json** - Input mapping configuration
- **olympe-config.json** - Main engine configuration
- **tiled_prefab_mapping.json** - Mappings between Tiled objects and engine prefabs

### Documentation/

Technical documentation, implementation notes, and guides:
- Implementation summaries
- API documentation
- Design documents
- Development guides

### Templates/

Project templates for creating new games or projects with Olympe Engine.

### Examples/

Example projects demonstrating engine features. For instance:
- `ForestDemo` - Sample forest environment demonstration

## Build Structure

The project uses **CMake** as its build system (minimum version 3.14).

### Build Targets

The CMake configuration defines several build targets:

1. **OlympeCore** (Static Library)
   - Shared core functionality
   - Blueprint editor components
   - ImGui and ImNodes integration
   - Used by both runtime and editor executables

2. **OlympeEngine** (Executable)
   - The main runtime engine executable
   - Built when `BUILD_RUNTIME_ENGINE=ON` (default)
   - Compiled with `OLYMPE_RUNTIME_MODE` definition

3. **OlympeBlueprintEditor** (Executable)
   - Standalone blueprint editor
   - Built when `BUILD_BLUEPRINT_EDITOR_STANDALONE=ON` (default)
   - Compiled with `OLYMPE_BLUEPRINT_EDITOR_STANDALONE_MODE` definition

### Build Output

Build artifacts are generated in the `build/` directory:
```
build/
├── OlympeEngine.exe          # Runtime engine (or OlympeEngine on Linux)
├── OlympeBlueprintEditor.exe # Blueprint editor
├── SDL3.dll                  # SDL3 library (Windows)
└── Blueprints/              # Copied blueprint files
```

Debug builds generate executables with `_d` suffix (e.g., `Olympe Engine_d.exe`).

## Configuration Files

### olympe.ini

Runtime configuration file for the engine. Contains settings such as:
- Window dimensions and display settings
- Graphics options
- Audio settings
- Input preferences

This file is read at engine startup and can be modified to customize runtime behavior.

### CMakeLists.txt

The main build configuration file. Key features:
- C++14 standard requirement
- SDL3 dependency management
- Conditional build targets
- Automatic resource copying (Blueprints, DLLs)
- Installation rules

## Component Organization

Olympe Engine uses an **Entity Component System (ECS)** architecture:

1. **Components** are defined in `ECS_Components*.h` headers
2. **Component definitions** and metadata are managed by `ComponentDefinition`
3. **Systems** process components and are defined in `ECS_Systems*.cpp` files
4. **Entities** are lightweight IDs that aggregate components

The component system supports:
- **Prefabs**: Reusable entity templates scanned from JSON files
- **Dynamic loading**: Components can be added/removed at runtime
- **Serialization**: Entities can be saved and loaded
- **Blueprint integration**: Visual scripting for component behavior

### Component Registration

Components are registered at engine initialization:
1. `ComponentDefinition` loads component schemas
2. `PrefabScanner` indexes available prefabs
3. `PrefabFactory` instantiates entities from prefab definitions
4. Systems are registered with the ECS to process components each frame

## Dependencies

The project depends on:
- **SDL3** - Cross-platform multimedia library (windowing, rendering, input)
- **Dear ImGui** - Immediate mode GUI (used in editors)
- **ImNodes** - Node editor for ImGui (used for blueprint graphs)
- **nlohmann/json** - JSON parsing and serialization

## Development Workflow

Typical development workflow:
1. **Source code** changes go in `Source/`
2. **Blueprint logic** is edited using the Blueprint Editor
3. **Levels** are created in Tiled and saved to `Gamedata/`
4. **Assets** are placed in `Resources/`
5. **Configuration** is modified in `Config/` files
6. **Build** using CMake to generate executables
7. **Test** using the runtime engine executable

## Next Steps

- [Building the Engine](building.md) - Learn how to compile the project
- [Creating Your First Game](first-game.md) - Start developing with Olympe
- [Blueprint System](../core-concepts/blueprints.md) - Learn about visual scripting
- [Entity Component System](../core-concepts/ecs.md) - Understand the ECS architecture
