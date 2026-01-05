# Olympe Blueprint System

## Overview

The Olympe Blueprint system provides a JSON-based entity definition format that allows game designers to create and edit entities without modifying C++ code. This document describes the blueprint schema, component properties, and development roadmap.

## Entity Blueprint Structure

An entity blueprint is a JSON file with the following structure:

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "EntityName",
    "description": "Entity description",
    "components": [
        {
            "type": "ComponentType",
            "properties": {
                // Component-specific properties
            }
        }
    ]
}
```

## Component Types and Properties

### Core Components

#### Position
Defines the entity's position in 2D/3D space.

```json
{
    "type": "Position",
    "properties": {
        "position": {
            "x": 100.0,
            "y": 200.0,
            "z": 0.0
        }
    }
}
```

#### BoundingBox
Defines the collision rectangle for the entity.

```json
{
    "type": "BoundingBox",
    "properties": {
        "boundingBox": {
            "x": 0.0,
            "y": 0.0,
            "w": 32.0,
            "h": 32.0
        }
    }
}
```

#### VisualSprite
Defines the visual representation of the entity.

```json
{
    "type": "VisualSprite",
    "properties": {
        "spritePath": "Resources/sprite.png",
        "srcRect": {
            "x": 0.0,
            "y": 0.0,
            "w": 32.0,
            "h": 32.0
        },
        "hotSpot": {
            "x": 16.0,
            "y": 16.0
        }
    }
}
```

### Physics Components

#### Movement
Defines movement direction and velocity.

```json
{
    "type": "Movement",
    "properties": {
        "direction": {"x": 1.0, "y": 0.0},
        "velocity": {"x": 0.0, "y": 0.0}
    }
}
```

#### PhysicsBody
Defines physical properties like mass and speed.

```json
{
    "type": "PhysicsBody",
    "properties": {
        "mass": 1.0,
        "speed": 50.0
    }
}
```

### Gameplay Components

#### Health
Tracks entity health points.

```json
{
    "type": "Health",
    "properties": {
        "currentHealth": 100,
        "maxHealth": 100
    }
}
```

#### AIBehavior
Defines AI behavior type.

```json
{
    "type": "AIBehavior",
    "properties": {
        "behaviorType": "patrol"
    }
}
```

Supported behavior types:
- `idle`: Entity stands still
- `patrol`: Entity follows a patrol path
- `chase`: Entity chases a target
- `flee`: Entity runs away from threats
- `wander`: Entity moves randomly

#### TriggerZone
Defines a detection/trigger zone around the entity.

```json
{
    "type": "TriggerZone",
    "properties": {
        "radius": 50.0,
        "triggered": false
    }
}
```

#### Inventory
Manages entity's item collection.

```json
{
    "type": "Inventory",
    "properties": {
        "items": ["item_id_1", "item_id_2"]
    }
}
```

### Visual/Audio Components

#### Animation
Controls sprite animation.

```json
{
    "type": "Animation",
    "properties": {
        "animationID": "walk_cycle",
        "currentFrame": 0,
        "frameDuration": 0.1,
        "elapsedTime": 0.0
    }
}
```

#### AudioSource
Plays sound effects.

```json
{
    "type": "AudioSource",
    "properties": {
        "soundEffectID": "footsteps",
        "volume": 0.8
    }
}
```

#### FX (Visual Effects)
Displays visual effects like particles or explosions.

```json
{
    "type": "FX",
    "properties": {
        "effectType": "explosion",
        "duration": 1.0,
        "elapsedTime": 0.0
    }
}
```

### Input Components

#### Controller
Handles gamepad/controller input.

```json
{
    "type": "Controller",
    "properties": {
        "controllerIndex": -1,
        "isConnected": false
    }
}
```

#### PlayerController
Tracks player input states.

```json
{
    "type": "PlayerController",
    "properties": {
        "isJumping": false,
        "isShooting": false,
        "isRunning": false,
        "isInteracting": false
    }
}
```

### Other Components

#### Camera
Camera settings for entities that have camera control.

```json
{
    "type": "Camera",
    "properties": {
        "zoomLevel": 1.0,
        "rotation": 0.0,
        "targetEntity": 0
    }
}
```

#### NPC
NPC-specific data.

```json
{
    "type": "NPC",
    "properties": {
        "npcType": "vendor"
    }
}
```

## Example Blueprints

See the following example files:
- `example_entity_simple.json` - Basic entity with position, bounding box, and sprite
- `example_entity_complete.json` - Complete entity demonstrating all major components

## Usage in Code

### Creating a Blueprint Programmatically

```cpp
#include "EntityBlueprint.h"
using namespace Olympe::Blueprint;

// Create a new entity blueprint
EntityBlueprint entity("MyEntity");
entity.description = "A custom entity";

// Add components
entity.AddComponent("Position", CreatePositionComponent(100, 200).properties);
entity.AddComponent("BoundingBox", CreateBoundingBoxComponent(0, 0, 32, 32).properties);
entity.AddComponent("VisualSprite", 
    CreateVisualSpriteComponent("sprite.png", 0, 0, 32, 32).properties);

// Save to file
entity.SaveToFile("Blueprints/my_entity.json");
```

### Loading a Blueprint

```cpp
// Load from file
EntityBlueprint entity = EntityBlueprint::LoadFromFile("Blueprints/my_entity.json");

// Access components
if (auto* pos = entity.GetComponent("Position"))
{
    float x = pos->properties["position"]["x"];
    float y = pos->properties["position"]["y"];
}

// Modify properties
entity.GetComponent("Position")->properties["position"]["x"] = 500.0f;

// Save changes
entity.SaveToFile("Blueprints/my_entity_modified.json");
```

## Files

- `OlympeBlueprintEditor/include/EntityBlueprint.h` - Blueprint class definitions
- `OlympeBlueprintEditor/src/EntityBlueprint.cpp` - Blueprint implementation
- `OlympeBlueprintEditor/src/blueprint_test.cpp` - Test program demonstrating blueprint usage
- `Blueprints/*.json` - Example blueprint files

## Next Steps

See `DEVELOPMENT_PLAN.md` for the complete development roadmap and sprint planning.
