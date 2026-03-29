---
id: prefab-properties
title: Prefab Properties
sidebar_label: Prefab Properties
sidebar_position: 3
---

# Prefab Properties

Prefab properties allow you to customize component values when creating entities from blueprints. This system supports both default values in prefab files and per-instance overrides from Tiled maps.

## Property System Overview

The prefab property system works in layers:

1. **Component Defaults** - Defined in C++ component structs
2. **Prefab Values** - Specified in blueprint JSON files
3. **Instance Overrides** - Set per-entity in Tiled or code

```cpp
// Layer 1: Component Default
struct Health_data {
    float maxHealth = 100.0f;  // Default value
    float currentHealth = 100.0f;
};

// Layer 2: Prefab JSON
{
  "components": {
    "Health": {
      "maxHealth": 150.0,  // Override default
      "currentHealth": 150.0
    }
  }
}

// Layer 3: Instance Override (in Tiled object)
{
  "prefabPath": "Blueprints/Enemies/Orc.json",
  "Health.maxHealth": 200.0  // Override prefab value
}
```

## Component Property Format

Properties in prefab JSON files follow this structure:

```json
{
  "name": "PrefabName",
  "components": {
    "ComponentName": {
      "propertyName": value,
      "nestedProperty": {
        "subProperty": value
      }
    }
  }
}
```

## Common Component Properties

### Identity Component

```json
{
  "Identity": {
    "name": "EntityName",
    "tag": "Enemy",
    "entityType": "Enemy"
  }
}
```

**Properties:**
- `name` (string) - Entity identifier
- `tag` (string) - Grouping tag
- `entityType` (string) - Entity type classification

### Position Component

```json
{
  "Position": {
    "x": 100.0,
    "y": 200.0,
    "z": 0.0
  }
}
```

**Properties:**
- `x`, `y`, `z` (float) - World coordinates

### Sprite Component

```json
{
  "Sprite": {
    "texturePath": "Resources/Textures/Characters/Knight.png",
    "width": 32,
    "height": 48,
    "sourceX": 0,
    "sourceY": 0,
    "scale": 1.0,
    "flipHorizontal": false,
    "flipVertical": false,
    "tint": "#FFFFFF",
    "opacity": 1.0
  }
}
```

**Properties:**
- `texturePath` (string) - Path to texture file
- `width`, `height` (int) - Sprite dimensions
- `sourceX`, `sourceY` (int) - Source rectangle position
- `scale` (float) - Uniform scale multiplier
- `flipHorizontal`, `flipVertical` (bool) - Flip sprite
- `tint` (string) - Color tint (hex: "#RRGGBB")
- `opacity` (float) - Alpha transparency (0.0 to 1.0)

### Health Component

```json
{
  "Health": {
    "maxHealth": 100.0,
    "currentHealth": 100.0,
    "invulnerable": false,
    "regeneration": 0.0
  }
}
```

**Properties:**
- `maxHealth` (float) - Maximum health points
- `currentHealth` (float) - Starting health
- `invulnerable` (bool) - Immune to damage
- `regeneration` (float) - Health per second

### Movement Component

```json
{
  "Movement": {
    "velocity": {
      "x": 0.0,
      "y": 0.0
    },
    "maxSpeed": 150.0,
    "acceleration": 500.0,
    "friction": 800.0
  }
}
```

**Properties:**
- `velocity` (Vector) - Current velocity
- `maxSpeed` (float) - Maximum movement speed
- `acceleration` (float) - Acceleration rate
- `friction` (float) - Deceleration rate

### Collision Component

```json
{
  "Collision": {
    "solid": true,
    "bounds": {
      "x": 0,
      "y": 0,
      "width": 32,
      "height": 48
    },
    "layer": "Default",
    "mask": ["Default", "Player"]
  }
}
```

**Properties:**
- `solid` (bool) - Whether entity blocks movement
- `bounds` (Rect) - Collision rectangle
- `layer` (string) - Collision layer name
- `mask` (array) - Layers this entity collides with

## AI Component Properties

### AI Blackboard

```json
{
  "AIBlackboard": {
    "wanderRadius": 100.0,
    "detectionRadius": 150.0,
    "aggressiveRange": 200.0,
    "patrolSpeed": 80.0,
    "chaseSpeed": 150.0
  }
}
```

**Properties:**
- `wanderRadius` (float) - Wander behavior radius
- `detectionRadius` (float) - Player detection range
- `aggressiveRange` (float) - Attack range
- `patrolSpeed`, `chaseSpeed` (float) - Movement speeds

### Behavior Tree

```json
{
  "BehaviorTree": {
    "treePath": "Blueprints/BehaviorTrees/GuardAI.json",
    "enabled": true,
    "updateFrequency": 0.1
  }
}
```

**Properties:**
- `treePath` (string) - Path to behavior tree JSON
- `enabled` (bool) - Whether AI is active
- `updateFrequency` (float) - Seconds between updates

## Property Override System

### In Tiled Maps

Override properties using dot notation in object properties:

```json
{
  "type": "NPCSpawn",
  "prefabPath": "Blueprints/NPCs/Guard.json",
  
  // Component property overrides
  "Identity.name": "Royal Guard",
  "Health.maxHealth": 200.0,
  "Sprite.scale": 1.2,
  "AIBlackboard.detectionRadius": 250.0
}
```

### In Code

Override properties when instantiating:

```cpp
// Load prefab
EntityID guard = PrefabFactory::CreateFromFile(
    "Blueprints/NPCs/Guard.json"
);

// Override components
Health_data health;
health.maxHealth = 200.0f;
health.currentHealth = 200.0f;
World::Get().SetComponent<Health_data>(guard, health);

// Or modify existing component
auto* sprite = World::Get().GetComponent<Sprite_data>(guard);
if (sprite) {
    sprite->scale = 1.2f;
}
```

## Property Types

### Primitive Types

```json
{
  "intValue": 42,
  "floatValue": 3.14,
  "boolValue": true,
  "stringValue": "text"
}
```

### Vector/Position Types

```json
{
  "position": {
    "x": 100.0,
    "y": 200.0,
    "z": 0.0
  }
}
```

### Color Types

```json
{
  "color": "#FF0000",      // Hex color
  "colorRGBA": {           // RGBA components
    "r": 255,
    "g": 0,
    "b": 0,
    "a": 255
  }
}
```

### Array Types

```json
{
  "tags": ["Enemy", "Humanoid", "Armored"],
  "patrolPoints": ["waypoint_1", "waypoint_2", "waypoint_3"]
}
```

### Nested Objects

```json
{
  "Animation": {
    "states": {
      "idle": {
        "frames": [0, 1, 2, 3],
        "frameDuration": 0.15
      },
      "walk": {
        "frames": [4, 5, 6, 7],
        "frameDuration": 0.1
      }
    }
  }
}
```

## Advanced Property Patterns

### Conditional Properties

Use conditions to set properties based on game state:

```json
{
  "Health": {
    "maxHealth": 100.0,
    "difficulty": {
      "easy": {
        "maxHealth": 75.0
      },
      "hard": {
        "maxHealth": 150.0
      }
    }
  }
}
```

### Property Templates

Create reusable property sets:

```json
// Template: WeakEnemy.json
{
  "Health": {
    "maxHealth": 50.0
  },
  "Movement": {
    "maxSpeed": 80.0
  },
  "AIBlackboard": {
    "detectionRadius": 100.0
  }
}

// Use in prefab
{
  "name": "Goblin",
  "template": "Templates/WeakEnemy.json",
  "components": {
    "Identity": {
      "name": "Goblin"
    }
  }
}
```

### Property Expressions

Some properties support expressions:

```json
{
  "Health": {
    "maxHealth": 100.0,
    "currentHealth": "maxHealth * 0.5"  // Start at half health
  },
  "Sprite": {
    "scale": "1.0 + (level * 0.1)"  // Scale based on level
  }
}
```

## Property Validation

Olympe Engine validates properties at load time:

```cpp
// Example validation errors:

// Missing required property
Error: Prefab 'Guard.json' missing required property 'Identity.name'

// Invalid property type
Error: Property 'Health.maxHealth' expects float, got string

// Invalid property value
Error: Property 'Sprite.opacity' must be between 0.0 and 1.0

// Unknown property
Warning: Unknown property 'Health.unknownField' in prefab 'Enemy.json'
```

## Property Defaults

Components define default values:

```cpp
// In ECS_Components.h
struct Movement_data {
    Vector velocity = Vector(0, 0, 0);
    float maxSpeed = 100.0f;      // Default max speed
    float acceleration = 400.0f;  // Default acceleration
    float friction = 600.0f;      // Default friction
};
```

If a property is not specified in the prefab, the default is used.

## Component Registration

Components must be registered to support properties:

```cpp
// In ECS_Components_Registration.cpp
AUTO_REGISTER_COMPONENT(Identity_data);
AUTO_REGISTER_COMPONENT(Position_data);
AUTO_REGISTER_COMPONENT(Sprite_data);
AUTO_REGISTER_COMPONENT(Health_data);
AUTO_REGISTER_COMPONENT(Movement_data);
// ... etc
```

This enables the prefab system to instantiate components dynamically.

## Debugging Properties

### Enable Property Logging

```cpp
// In prefabfactory.h
#define DEBUG_PREFAB_PROPERTIES 1

// Output example:
[PrefabFactory] Loading: Blueprints/Enemies/Orc.json
[PrefabFactory]   Component: Identity
[PrefabFactory]     name: "Orc Warrior"
[PrefabFactory]     entityType: "Enemy"
[PrefabFactory]   Component: Health
[PrefabFactory]     maxHealth: 150.0
[PrefabFactory]     currentHealth: 150.0
```

### Inspect Properties at Runtime

```cpp
// Get component and inspect
auto* health = World::Get().GetComponent<Health_data>(entity);
if (health) {
    std::cout << "Max Health: " << health->maxHealth << std::endl;
    std::cout << "Current Health: " << health->currentHealth << std::endl;
}
```

## Best Practices

### Property Organization
- Group related properties together
- Use consistent naming conventions
- Document custom properties

### Default Values
- Set sensible defaults in component structs
- Only override in prefab when necessary
- Use overrides for specialization

### Performance
- Avoid complex property expressions
- Cache property lookups
- Use property templates for common patterns

### Validation
- Validate properties at load time
- Provide clear error messages
- Use type-safe property access

## Example: Complete Enemy Prefab

```json
{
  "name": "OrcWarrior",
  "description": "Strong melee enemy with patrol behavior",
  
  "components": {
    "Identity": {
      "name": "Orc Warrior",
      "tag": "Enemy",
      "entityType": "Enemy"
    },
    
    "Position": {
      "x": 0.0,
      "y": 0.0,
      "z": 0.0
    },
    
    "Sprite": {
      "texturePath": "Resources/Textures/Enemies/Orc.png",
      "width": 48,
      "height": 64,
      "scale": 1.0,
      "tint": "#FFFFFF"
    },
    
    "Health": {
      "maxHealth": 150.0,
      "currentHealth": 150.0,
      "regeneration": 0.5
    },
    
    "Movement": {
      "maxSpeed": 120.0,
      "acceleration": 400.0,
      "friction": 600.0
    },
    
    "Collision": {
      "solid": true,
      "bounds": {
        "x": 8,
        "y": 16,
        "width": 32,
        "height": 48
      },
      "layer": "Enemy",
      "mask": ["Player", "Enemy", "World"]
    },
    
    "AIBlackboard": {
      "wanderRadius": 100.0,
      "detectionRadius": 150.0,
      "aggressiveRange": 200.0,
      "patrolSpeed": 60.0,
      "chaseSpeed": 120.0
    },
    
    "BehaviorTree": {
      "treePath": "Blueprints/BehaviorTrees/PatrolAndAttack.json",
      "enabled": true,
      "updateFrequency": 0.1
    }
  }
}
```

## See Also

- [Creating Prefabs](./creating-prefabs.md) - How to create blueprint files
- [Component Overrides](./component-overrides.md) - Advanced override system
- [ECS Components](../../technical-reference/architecture/ecs-components.md) - Component reference
- [Object Properties](../tiled-editor/object-properties.md) - Tiled integration
