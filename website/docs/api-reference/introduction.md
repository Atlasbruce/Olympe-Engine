---
id: api-introduction
title: "API Reference Introduction"
sidebar_label: "Introduction"
---

# API Reference Introduction

Welcome to the Olympe Engine API Reference. This section provides detailed documentation of the engine's classes, functions, and data structures.

## Full API Documentation

Complete API documentation is generated using **Doxygen** from source code comments. The generated documentation includes:

- Class hierarchies and inheritance diagrams
- Function signatures and parameter descriptions
- Member variable documentation
- Code examples and usage notes
- Cross-references between related classes

### Generating API Docs

To generate the full API documentation:

```bash
# Install Doxygen (if not already installed)
# Windows: Download from https://www.doxygen.nl/download.html
# Linux: sudo apt install doxygen
# macOS: brew install doxygen

# Generate documentation
cd /path/to/Olympe-Engine
doxygen Doxyfile
```

Generated documentation will be available in `Docs/html/index.html`.

## Key Classes and Headers

This section provides an overview of the most important classes and headers you'll encounter when working with Olympe Engine.

### Core Engine

#### GameEngine
**File**: `Source/GameEngine.h`

Top-level singleton that orchestrates the entire engine.

```cpp
GameEngine& engine = GameEngine::GetInstance();
engine.Initialize();
engine.Process(); // Call every frame
```

**Key Members:**
- `InputsManager& inputsmanager`: Input system access
- `VideoGame& videogame`: Game state management
- `DataManager& datamanager`: Resource management
- `static float fDt`: Delta time between frames
- `SDL_Renderer* renderer`: Main rendering context

#### World
**File**: `Source/World.h`

Manages the ECS architecture and game world state.

```cpp
World& world = World::GetInstance();
EntityID entity = world.CreateEntity();
world.AddComponent<Position_data>(entity, position);
world.GetComponent<Position_data>(entity)->position.x += 10.0f;
```

**Key Methods:**
- `CreateEntity()`: Create new entity
- `DestroyEntity(EntityID)`: Destroy entity
- `AddComponent<T>(EntityID, T)`: Add component to entity
- `GetComponent<T>(EntityID)`: Get component from entity
- `HasComponent<T>(EntityID)`: Check if entity has component
- `RemoveComponent<T>(EntityID)`: Remove component from entity
- `LoadLevel(string)`: Load Tiled level
- `RegisterSystem<T>()`: Register ECS system

### ECS Architecture

#### ECS_Entity
**File**: `Source/ECS_Entity.h`

Defines entity identifiers and component signatures.

```cpp
using EntityID = uint32_t;
using ComponentSignature = std::bitset<MAX_COMPONENTS>;
```

#### ECS_Components
**File**: `Source/ECS_Components.h`

All component data structures. Key components:

- **Identity_data**: Name, tag, entity type
- **Position_data**: 2D/3D position
- **Velocity_data**: Movement velocity
- **BoundingBox_data**: Collision bounds
- **VisualSprite_data**: Sprite rendering
- **Controller_data**: Player input
- **AIBlackboard_data**: AI state
- **AIBehaviorTree_data**: Behavior tree
- **CameraTransform_data**: Camera state
- **CameraFollow_data**: Camera following behavior

#### ECS_Systems
**File**: `Source/ECS_Systems.h`

Base class for all systems and system declarations.

```cpp
class ECS_System {
    ComponentSignature requiredSignature;
    std::set<EntityID> m_entities;
    virtual void Process() {}
    virtual void Render() {}
};
```

**Key Systems:**
- `InputSystem`: Process player input
- `AISystem`: Process behavior trees
- `PathfindingSystem`: Navigation and pathfinding
- `PhysicsSystem`: Collision detection
- `CameraSystem`: Camera updates
- `RenderingSystem`: Entity rendering

### Input System

#### InputsManager
**File**: `Source/InputsManager.h`

Manages all input devices and player bindings.

```cpp
InputsManager& input = InputsManager::GetInstance();
input.InitializeInputSystem("Config/olympe-config.json");
int joystickCount = input.GetConnectedJoysticksCount();
```

**Key Methods:**
- `InitializeInputSystem(string)`: Load input configuration
- `HandleEvent(SDL_Event*)`: Process SDL events
- `GetDeviceManager()`: Access device management
- `GetContextManager()`: Access input contexts

#### InputDevice
**File**: `Source/InputDevice.h`

Device management and profile assignment.

```cpp
InputDeviceManager& deviceMgr = InputDeviceManager::GetInstance();
DeviceID keyboard = deviceMgr.GetKeyboardDevice();
```

### Behavior Trees

#### BehaviorTree
**File**: `Source/AI/BehaviorTree.h`

Behavior tree definitions and evaluation.

**Key Structures:**
- `BTNode`: Individual tree node
- `BTNodeType`: Node type enum (Selector, Sequence, Action, Condition)
- `BTStatus`: Execution status (Running, Success, Failure)
- `BTConditionType`: Built-in conditions
- `BTActionType`: Built-in actions

```cpp
struct AIBehaviorTree_data {
    std::string behaviorTreePath;
    BTNode rootNode;
    BTStatus currentStatus;
};
```

### Prefab and Blueprint System

#### PrefabFactory
**File**: `Source/PrefabFactory.cpp`, `Source/prefabfactory.h`

Creates entities from JSON blueprints.

```cpp
EntityID npc = PrefabFactory::CreateEntityFromBlueprint(
    "Blueprints/EntityPrefab/guard.json"
);
```

#### PrefabScanner
**File**: `Source/PrefabScanner.h`

Scans and registers available blueprints.

```cpp
PrefabScanner::ScanDirectory("Blueprints/EntityPrefab/");
std::vector<std::string> prefabs = PrefabScanner::GetAvailablePrefabs();
```

### Level Loading

#### TiledLevelLoader
**Files**: `Source/TiledLevelLoader/*`

Loads Tiled TMX map files.

**Key Classes:**
- `TiledMap`: Parsed map data
- `TileLayer`: Tile layer information
- `ObjectGroup`: Entity instances
- `Tileset`: Tileset metadata

```cpp
World::GetInstance().LoadLevel("Levels/forest_level.tmx");
```

### Data Management

#### DataManager
**File**: `Source/DataManager.h`

Resource loading and caching.

```cpp
DataManager& data = DataManager::GetInstance();
SDL_Texture* texture = data.LoadTexture("Resources/sprite.png");
```

### Rendering

#### IsometricRenderer
**File**: `Source/Rendering/IsometricRenderer.cpp`

Isometric projection rendering.

```cpp
void RenderEntitiesForCamera(const CameraTransform& cam);
void RenderMultiLayerForCamera(const CameraTransform& cam);
```

#### RenderContext
**File**: `Source/RenderContext.h`

Rendering state and utilities.

### Utilities

#### Vector
**File**: `Source/vector.h`

2D/3D vector math.

```cpp
struct Vector {
    float x, y, z;
    Vector operator+(const Vector& other);
    float Length() const;
    Vector Normalized() const;
};
```

#### Sprite
**File**: `Source/Sprite.h`

Sprite loading and rendering utilities.

#### EventQueue
**File**: `Source/system/EventQueue.h`

Event system for inter-system communication.

```cpp
EventQueue::Publish("Input", "ButtonPressed", {{"action", "jump"}, {"playerID", 0}});
EventQueue::Subscribe("Input", [](const Event& e) { /* handle */ });
```

## Component Reference

### Common Component Properties

Most components follow similar patterns:

```cpp
// Position component
struct Position_data {
    Vector position;
};

// Velocity component
struct Velocity_data {
    Vector velocity;
    float maxSpeed;
};

// Sprite component
struct VisualSprite_data {
    std::string spritePath;
    SDL_Rect srcRect;
    Vector hotSpot;
    bool isVisible;
};
```

See [ECS Overview](../technical-reference/architecture/ecs-overview.md) for complete component documentation.

## System Reference

### System Processing Order

Systems are processed in this order each frame:

1. **InputEventConsumeSystem**: Consume input events
2. **GameEventConsumeSystem**: Handle game state events
3. **AISystem**: Update behavior trees
4. **PathfindingSystem**: Process navigation requests
5. **InputSystem**: Process player controller input
6. **PhysicsSystem**: Update physics simulation
7. **CameraSystem**: Update camera transforms
8. **RenderingSystem**: Render all entities

## Header Dependency Graph

```
GameEngine.h
  ├── World.h
  │     ├── ECS_Entity.h
  │     ├── ECS_Components.h
  │     ├── ECS_Systems.h
  │     ├── ECS_Register.h
  │     ├── PrefabFactory.h
  │     └── TiledLevelLoader (subdirectory)
  ├── InputsManager.h
  │     ├── InputDevice.h
  │     ├── InputConfigLoader.h
  │     └── system/KeyboardManager.h
  ├── DataManager.h
  └── VideoGame.h
```

## Common Patterns

### Creating and Configuring Entities

```cpp
// Create entity
EntityID entity = World::Get().CreateEntity();

// Add identity
Identity_data identity;
identity.name = "Player";
identity.entityType = EntityType::Player;
World::Get().AddComponent<Identity_data>(entity, identity);

// Add position
Position_data position;
position.position = Vector(100.0f, 200.0f, 0.0f);
World::Get().AddComponent<Position_data>(entity, position);

// Add sprite
VisualSprite_data sprite;
sprite.spritePath = "Resources/player.png";
sprite.srcRect = {0, 0, 32, 32};
sprite.hotSpot = Vector(16.0f, 16.0f);
World::Get().AddComponent<VisualSprite_data>(entity, sprite);
```

### Accessing Components in Systems

```cpp
class MySystem : public ECS_System {
    virtual void Process() override {
        for (EntityID entity : m_entities) {
            Position_data* pos = World::Get().GetComponent<Position_data>(entity);
            Velocity_data* vel = World::Get().GetComponent<Velocity_data>(entity);
            
            if (pos && vel) {
                pos->position = pos->position + vel->velocity * GameEngine::fDt;
            }
        }
    }
};
```

### Loading Resources

```cpp
DataManager& dm = DataManager::Get();
SDL_Texture* texture = dm.LoadTexture("Resources/sprite.png");
// Texture is cached; subsequent calls return cached texture
```

## C++ Standard and Dependencies

- **C++ Standard**: C++14
- **Main Dependencies**:
  - SDL3 (rendering, input, windowing)
  - nlohmann/json (JSON parsing)
  - ImGui (editor UI)
  - tinyxml2 (Tiled TMX parsing)

## Next Steps

- Explore [ECS Architecture](../technical-reference/architecture/ecs-overview.md) for in-depth component system details
- Read [Engine Modules](../technical-reference/architecture/modules.md) to understand module organization
- Study [Behavior Trees](../technical-reference/behavior-trees/overview.md) for AI programming
- Check [Blueprint System](../user-guide/blueprints/overview.md) for data-driven entity creation

## Contributing API Documentation

When contributing code, please:

1. Add Doxygen comments to all public classes and functions
2. Use `@brief`, `@param`, `@return` tags
3. Include usage examples where helpful
4. Document preconditions and postconditions
5. Note thread-safety considerations

Example:

```cpp
/**
 * @brief Creates a new entity in the world
 * @return EntityID of the newly created entity
 * 
 * Example:
 * @code
 * EntityID player = World::Get().CreateEntity();
 * @endcode
 */
EntityID CreateEntity();
```

See [Code Style Guide](../contributing/code-style.md) for full documentation standards.
