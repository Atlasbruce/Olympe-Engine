---
id: quick-start
title: Quick Start Guide
sidebar_label: Quick Start
---

# Quick Start Guide

This guide will walk you through creating your first game with Olympe Engine. By the end, you'll have a simple scene with a player character that can move around a Tiled map.

## Prerequisites

Before starting, make sure you have:

- ✅ [Installed Olympe Engine](./installation.md)
- ✅ Built the project successfully
- ✅ [Tiled MapEditor](https://www.mapeditor.org/) installed (optional but recommended)

## Your First Entity

Let's create a simple player entity programmatically.

### Step 1: Create a New World

In your main game code, initialize the engine and create a world:

```cpp
#include "GameEngine.h"
#include "World.h"

int main(int argc, char* argv[]) {
    // Initialize the engine
    GameEngine engine;
    if (!engine.Initialize(1280, 720, "My First Olympe Game")) {
        return -1;
    }
    
    // Get the world instance
    World& world = World::Get();
    
    // Your game code here...
    
    return 0;
}
```

### Step 2: Create an Entity

Create a player entity with essential components:

```cpp
// Create an entity
EntityID player = world.CreateEntity();

// Add position component
Position_data pos;
pos.position.x = 400.0f;
pos.position.y = 300.0f;
pos.position.z = 0.0f;
world.AddComponent<Position_data>(player, pos);

// Add visual sprite
VisualSprite_data sprite;
sprite.spritePath = "Resources/player.png";
sprite.srcRect = {0, 0, 32, 32};
sprite.hotSpot = {16, 16};
world.AddComponent<VisualSprite_data>(player, sprite);

// Add bounding box for collision
BoundingBox_data bbox;
bbox.boundingBox = {0, 0, 32, 32};
world.AddComponent<BoundingBox_data>(player, bbox);

// Add physics body
PhysicsBody_data physics;
physics.mass = 1.0f;
physics.speed = 100.0f;
world.AddComponent<PhysicsBody_data>(player, physics);

// Add movement component
Movement_data movement;
world.AddComponent<Movement_data>(player, movement);

// Add player controller
PlayerController_data controller;
world.AddComponent<PlayerController_data>(player, controller);

// Add health
Health_data health;
health.currentHealth = 100;
health.maxHealth = 100;
world.AddComponent<Health_data>(player, health);
```

### Step 3: Add Input Control

Bind an input device to control the player:

```cpp
// Bind keyboard/mouse to player 1
InputsManager::Get().BindControllerToPlayer(player, -1, 0);
// -1 = keyboard/mouse device
// 0 = player index
```

### Step 4: Run the Game Loop

```cpp
// Main game loop
bool running = true;
SDL_Event event;

while (running) {
    // Handle events
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            running = false;
        }
        InputsManager::Get().HandleEvent(&event);
    }
    
    // Update world (processes all ECS systems)
    float deltaTime = 1.0f / 60.0f;  // 60 FPS
    world.Update(deltaTime);
    
    // Render
    engine.Render();
}

// Cleanup
engine.Shutdown();
```

## Using Blueprints (Easier Way!)

Instead of creating entities in code, use JSON blueprints:

### Step 1: Create a Blueprint

Create `Blueprints/EntityPrefab/player.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Player",
    "description": "Playable character",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 400.0, "y": 300.0, "z": 0.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/player.png",
                "srcRect": {"x": 0, "y": 0, "w": 32, "h": 32},
                "hotSpot": {"x": 16, "y": 16}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": 0, "y": 0, "w": 32, "h": 32}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 1.0,
                "speed": 100.0
            }
        },
        {
            "type": "Movement",
            "properties": {}
        },
        {
            "type": "PlayerController",
            "properties": {}
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 100,
                "maxHealth": 100
            }
        }
    ]
}
```

### Step 2: Load the Blueprint

```cpp
#include "PrefabFactory.h"

// Create player from blueprint
EntityID player = PrefabFactory::Get().CreateFromBlueprint(
    "Blueprints/EntityPrefab/player.json"
);

// That's it! All components are automatically added.
```

## Loading a Tiled Map

Now let's load a complete game level from a Tiled map.

### Step 1: Create a Map in Tiled

1. Open Tiled MapEditor
2. Create a new map:
   - **Orientation**: Orthogonal (or Isometric/Hexagonal)
   - **Tile size**: 32x32 pixels
   - **Map size**: 20x15 tiles
3. Add tilesets from your game assets
4. Create layers:
   - `layer_ground` - Ground tiles
   - `layer_objects` - Obstacles and decorations
   - `layer_entities` - Object layer for entity spawn points
5. Save as **TMJ** format: `my_level.tmj`

### Step 2: Configure Layer Properties

Select the `layer_ground` layer and add custom properties:

- **Property**: `isTilesetWalkable`
- **Type**: `bool`
- **Value**: `true`

Select the `layer_objects` layer and add:

- **Property**: `isTilesetWalkable`
- **Type**: `bool`
- **Value**: `false`

These properties tell the engine which tiles are walkable. See the [Tiled Editor Guide](../user-guide/tiled-editor/layer-properties.md) for details.

### Step 3: Add Entity Spawn Points

1. Create an object layer: `layer_entities`
2. Add a point or rectangle object
3. Set the object's **Type** field to `Player`
4. Add custom property: `prefab` = `player.json`

### Step 4: Load the Map

```cpp
// Load the level
bool success = world.LoadLevel("Gamedata/Levels/my_level.tmj");

if (success) {
    std::cout << "Level loaded successfully!" << std::endl;
    // The player entity will be automatically spawned
    // Collision and navigation maps are automatically generated
} else {
    std::cerr << "Failed to load level!" << std::endl;
}
```

## Adding AI Enemies

Let's add a simple wandering enemy.

### Step 1: Create Enemy Blueprint

Create `Blueprints/EntityPrefab/enemy.json`:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "Enemy",
    "description": "Wandering enemy NPC",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {"x": 600.0, "y": 400.0, "z": 0.0}
            }
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/enemy.png",
                "srcRect": {"x": 0, "y": 0, "w": 32, "h": 32},
                "hotSpot": {"x": 16, "y": 16}
            }
        },
        {
            "type": "BoundingBox",
            "properties": {
                "boundingBox": {"x": 0, "y": 0, "w": 32, "h": 32}
            }
        },
        {
            "type": "PhysicsBody",
            "properties": {
                "mass": 1.0,
                "speed": 50.0
            }
        },
        {
            "type": "Movement",
            "properties": {}
        },
        {
            "type": "AIBehavior",
            "properties": {
                "behaviorType": "wander"
            }
        },
        {
            "type": "Health",
            "properties": {
                "currentHealth": 50,
                "maxHealth": 50
            }
        },
        {
            "type": "NPC",
            "properties": {
                "npcType": "enemy"
            }
        }
    ]
}
```

### Step 2: Spawn the Enemy

```cpp
// Spawn an enemy
EntityID enemy = PrefabFactory::Get().CreateFromBlueprint(
    "Blueprints/EntityPrefab/enemy.json"
);

// Or add it to your Tiled map as a spawn point
```

The enemy will automatically use the wander behavior tree to move around randomly!

## Enable Debug Visualizations

See navigation and collision overlays:

```cpp
// Get grid settings (usually attached to a camera entity)
GridSettings_data* gridSettings = world.GetComponent<GridSettings_data>(cameraEntity);

if (gridSettings) {
    // Show navigation overlay (green)
    gridSettings->showNavigationOverlay = true;
    
    // Show collision overlay (purple)
    gridSettings->showCollisionOverlay = true;
    
    // Show grid
    gridSettings->showGrid = true;
}
```

Or toggle them at runtime with keyboard shortcuts (if configured in your input system).

## Complete Example

Here's a complete minimal example:

```cpp
#include "GameEngine.h"
#include "World.h"
#include "PrefabFactory.h"
#include "InputsManager.h"

int main(int argc, char* argv[]) {
    // Initialize engine
    GameEngine engine;
    if (!engine.Initialize(1280, 720, "My Olympe Game")) {
        return -1;
    }
    
    World& world = World::Get();
    
    // Load a Tiled map with entities
    world.LoadLevel("Gamedata/Levels/test_map.tmj");
    
    // Or manually create a player
    EntityID player = PrefabFactory::Get().CreateFromBlueprint(
        "Blueprints/EntityPrefab/player.json"
    );
    InputsManager::Get().BindControllerToPlayer(player, -1, 0);
    
    // Spawn some enemies
    for (int i = 0; i < 5; i++) {
        PrefabFactory::Get().CreateFromBlueprint(
            "Blueprints/EntityPrefab/enemy.json"
        );
    }
    
    // Main loop
    bool running = true;
    SDL_Event event;
    Uint64 lastTime = SDL_GetPerformanceCounter();
    
    while (running) {
        // Calculate delta time
        Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = (now - lastTime) / (float)SDL_GetPerformanceFrequency();
        lastTime = now;
        
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            InputsManager::Get().HandleEvent(&event);
        }
        
        // Update game state
        world.Update(deltaTime);
        
        // Render
        engine.Render();
    }
    
    engine.Shutdown();
    return 0;
}
```

## Next Steps

Congratulations! You've created your first Olympe Engine game. Now explore:

- 📖 [Project Structure](./project-structure.md) - Understand the codebase
- 🗺️ [Tiled Editor Guide](../user-guide/tiled-editor/introduction.md) - Master map creation
- 🎨 [Blueprint System](../user-guide/blueprints/overview.md) - Advanced entity design
- 🤖 [Behavior Trees](../technical-reference/behavior-trees/overview.md) - Create complex AI
- 🧭 [Navigation System](../technical-reference/navigation/navigation-system.md) - Pathfinding and navigation
- 🎮 [Input System](../user-guide/input-system/user-guide.md) - Configure controls

## Need Help?

- Check out the [examples](../examples/basic-game.md)
- Read the [FAQ](../faq.md)
- Ask in [GitHub Discussions](https://github.com/Atlasbruce/Olympe-Engine/discussions)
