---
id: ecs-overview
title: "Entity Component System Overview"
sidebar_label: "ECS Overview"
---

# Entity Component System Overview

Olympe Engine uses the **Entity Component System (ECS)** architecture pattern to organize game logic. ECS provides a flexible, data-oriented approach to game development that separates data (Components) from behavior (Systems) and entities (IDs).

## What is ECS?

ECS is an architectural pattern that prioritizes composition over inheritance. Instead of creating complex class hierarchies, you build entities by composing them from reusable components and processing them with systems.

### Why Olympe Uses ECS

- **Performance**: Data-oriented design enables better cache coherency and parallelization
- **Flexibility**: Entities can be modified at runtime by adding/removing components
- **Reusability**: Components and Systems are decoupled and highly reusable
- **Maintainability**: Clear separation of data and logic makes code easier to maintain
- **Scalability**: Easy to add new component types and systems without modifying existing code

## Core Concepts

### Entities

Entities are simple unique identifiers (`EntityID`) that represent game objects. They have no data or behavior themselves—they're just IDs that link components together.

```cpp
// EntityID is just a type alias for an unsigned integer
using EntityID = uint32_t;

// Creating an entity returns its ID
EntityID player = World::GetInstance().CreateEntity();
```

### Components

Components are plain data structures that define what an entity *is*. They contain no logic, only data. Olympe Engine defines its components in `ECS_Components.h`.

**Common Components:**

- **Identity_data**: Name, tag, entity type
- **Position_data**: 2D/3D position in world space
- **BoundingBox_data**: Collision bounds
- **VisualSprite_data**: Sprite rendering information
- **Velocity_data**: Movement velocity
- **Controller_data**: Player input mappings
- **AIBlackboard_data**: AI decision-making data
- **AIBehaviorTree_data**: Behavior tree state

#### Example Components

```cpp
// Identity component
struct Identity_data {
    std::string name = "Entity";
    std::string tag = "Untagged";
    EntityType entityType = EntityType::None;
    bool isPersistent = false;
};

// Position component
struct Position_data {
    Vector position;
};

// Sprite component
struct VisualSprite_data {
    std::string spritePath;
    SDL_Rect srcRect;
    Vector hotSpot;
    bool isVisible = true;
};
```

### Systems

Systems contain the game logic. They process entities that have specific component combinations. Systems are defined in `ECS_Systems.h` and `ECS_Systems.cpp`.

**Core Systems:**

- **InputSystem**: Processes player input
- **InputEventConsumeSystem**: Updates ECS from input events
- **GameEventConsumeSystem**: Handles game state events
- **AISystem**: Processes AI behavior trees
- **PathfindingSystem**: Handles navigation and pathfinding
- **RenderingSystem**: Renders entities with sprites
- **CameraSystem**: Manages camera transformations
- **PhysicsSystem**: Handles collision detection and response

## Creating Entities and Adding Components

### Manual Entity Creation

```cpp
// Create an entity
World& world = World::GetInstance();
EntityID player = world.CreateEntity();

// Add components
Identity_data identity;
identity.name = "Player";
identity.entityType = EntityType::Player;
world.AddComponent<Identity_data>(player, identity);

Position_data position;
position.position = Vector(100.0f, 200.0f, 0.0f);
world.AddComponent<Position_data>(player, position);

VisualSprite_data sprite;
sprite.spritePath = "Resources/player.png";
sprite.srcRect = {0, 0, 32, 32};
sprite.hotSpot = Vector(16.0f, 16.0f);
world.AddComponent<VisualSprite_data>(player, sprite);
```

### Blueprint-Based Creation

Most entities are created from JSON blueprints using the `PrefabFactory`:

```cpp
// Load and instantiate from blueprint
EntityID npc = PrefabFactory::CreateEntityFromBlueprint("Blueprints/EntityPrefab/guard.json");
```

See the [Blueprint System Overview](../../user-guide/blueprints/overview.md) for more details.

## Component Registration

Components must be registered with the World's component pools before use. This happens in `World::Initialize()`:

```cpp
void World::Initialize() {
    // Register all component types
    RegisterComponent<Identity_data>();
    RegisterComponent<Position_data>();
    RegisterComponent<VisualSprite_data>();
    RegisterComponent<BoundingBox_data>();
    RegisterComponent<Velocity_data>();
    RegisterComponent<Controller_data>();
    RegisterComponent<AIBlackboard_data>();
    RegisterComponent<AIBehaviorTree_data>();
    // ... more components
}
```

## System Processing

Systems are updated every frame in the game loop. Each system processes only the entities that match its required component signature:

```cpp
// In GameEngine::Process()
void GameEngine::Process() {
    World& world = World::GetInstance();
    
    // Update all systems
    world.GetSystem<InputEventConsumeSystem>()->Process();
    world.GetSystem<GameEventConsumeSystem>()->Process();
    world.GetSystem<AISystem>()->Process();
    world.GetSystem<PathfindingSystem>()->Process();
    world.GetSystem<InputSystem>()->Process();
    
    // ... more systems
}
```

### System Signatures

Each system defines which components it requires:

```cpp
class InputSystem : public ECS_System {
public:
    InputSystem() {
        // Only process entities with Controller_data
        requiredSignature.set(World::GetComponentTypeID<Controller_data>());
    }
    
    virtual void Process() override {
        // Process all entities in m_entities set
        for (EntityID entity : m_entities) {
            Controller_data* controller = World::GetInstance().GetComponent<Controller_data>(entity);
            // Process input logic...
        }
    }
};
```

## Accessing Components

```cpp
// Get a component from an entity
Position_data* pos = world.GetComponent<Position_data>(player);
if (pos) {
    pos->position.x += 10.0f;
}

// Check if entity has a component
if (world.HasComponent<Velocity_data>(entity)) {
    // Entity is moving
}

// Remove a component
world.RemoveComponent<Velocity_data>(entity);
```

## Key Files

- **`Source/ECS_Entity.h`**: Entity ID and signature definitions
- **`Source/ECS_Components.h`**: All component data structures
- **`Source/ECS_Systems.h`**: System base class and all system declarations
- **`Source/ECS_Systems.cpp`**: Core system implementations
- **`Source/ECS_Register.h`**: Component pool and registration logic
- **`Source/World.h`**: World class that manages the ECS

## Best Practices

1. **Keep components data-only**: No methods, just data structures
2. **Systems should be stateless**: All state should be in components
3. **Use composition**: Build entities from small, focused components
4. **Batch processing**: Systems process multiple entities per frame for efficiency
5. **Avoid entity references**: Use EntityID instead of pointers

## Next Steps

- Learn about [Engine Modules](./modules.md) and how systems interact
- Explore [Blueprint System](../../user-guide/blueprints/overview.md) for data-driven entity creation
- Read about [Behavior Trees](../behavior-trees/overview.md) for AI programming
