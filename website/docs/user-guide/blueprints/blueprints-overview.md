---
id: blueprints-overview
title: "Blueprint System Overview"
sidebar_label: "Overview"
---

# Blueprint System Overview

Blueprints are JSON-based entity definitions that allow you to create and configure game entities without writing code. They enable data-driven game development and make it easy to iterate on entity design.

## What are Blueprints?

A blueprint is a JSON file that describes an entity template:

- **Components**: Which components the entity has
- **Properties**: Initial values for component data
- **Metadata**: Name, description, and schema version

When you instantiate a blueprint, the engine creates a new entity with all specified components and values.

## Why Use Blueprints?

### Advantages Over Hardcoded Entities

1. **No Compilation Required**: Modify entities without recompiling the engine
2. **Designer-Friendly**: Non-programmers can create and modify entities
3. **Iteration Speed**: Rapid prototyping and testing
4. **Version Control**: Easy to diff and merge entity changes
5. **Reusability**: Define once, instantiate many times
6. **Tool Integration**: Works with the Blueprint Editor and Tiled level editor

### When to Use Code vs Blueprints

**Use Blueprints for:**
- Standard game entities (enemies, items, NPCs)
- Level-specific objects
- Prototyping new entity types
- Entities that designers need to modify

**Use Code for:**
- Dynamic entity generation with complex logic
- Procedurally generated entities
- Entities requiring runtime-only data
- System-level entities (cameras, managers)

## Basic Blueprint Structure

### Simple Example

```json
{
    "schema_version": 1,
    "type": "EntityBlueprint",
    "name": "SimpleCharacter",
    "description": "A basic character entity with position, bounding box, and sprite",
    "components": [
        {
            "type": "Position",
            "properties": {
                "position": {
                    "x": 100.0,
                    "y": 200.0,
                    "z": 0.0
                }
            }
        },
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
        },
        {
            "type": "VisualSprite",
            "properties": {
                "spritePath": "Resources/entity_male.png",
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
    ]
}
```

### Blueprint Schema

Every blueprint must include:

- **`schema_version`**: Blueprint format version (currently `1`)
- **`type`**: Must be `"EntityBlueprint"`
- **`name`**: Unique identifier for the blueprint
- **`description`**: Human-readable description
- **`components`**: Array of component definitions

Each component has:
- **`type`**: Component type name (e.g., `"Position"`, `"VisualSprite"`)
- **`properties`**: Object containing component-specific data

## Component Types

Common component types available in blueprints:

### Identity
```json
{
    "type": "Identity",
    "properties": {
        "name": "Guard",
        "tag": "Enemy",
        "entityType": "NPC",
        "isPersistent": false
    }
}
```

### Position
```json
{
    "type": "Position",
    "properties": {
        "position": {"x": 0.0, "y": 0.0, "z": 0.0}
    }
}
```

### VisualSprite
```json
{
    "type": "VisualSprite",
    "properties": {
        "spritePath": "Resources/sprites/character.png",
        "srcRect": {"x": 0, "y": 0, "w": 32, "h": 32},
        "hotSpot": {"x": 16, "y": 16},
        "isVisible": true
    }
}
```

### BoundingBox
```json
{
    "type": "BoundingBox",
    "properties": {
        "boundingBox": {"x": -16, "y": -16, "w": 32, "h": 32},
        "isSolid": true,
        "isTrigger": false
    }
}
```

### Velocity
```json
{
    "type": "Velocity",
    "properties": {
        "velocity": {"x": 0.0, "y": 0.0, "z": 0.0},
        "maxSpeed": 100.0
    }
}
```

### AIBlackboard
```json
{
    "type": "AIBlackboard",
    "properties": {
        "targetEntity": 0,
        "moveGoal": {"x": 0.0, "y": 0.0, "z": 0.0},
        "patrolPoints": [],
        "currentPatrolIndex": 0,
        "detectionRadius": 200.0,
        "attackRadius": 50.0
    }
}
```

### AIBehaviorTree
```json
{
    "type": "AIBehaviorTree",
    "properties": {
        "behaviorTreePath": "Blueprints/AI/patrol_behavior.json"
    }
}
```

## Using Blueprints

### Loading in Code

```cpp
#include "PrefabFactory.h"

// Create entity from blueprint
EntityID guard = PrefabFactory::CreateEntityFromBlueprint(
    "Blueprints/EntityPrefab/guard.json"
);

// Blueprint is instantiated with all components
// You can still modify components after creation
Position_data* pos = World::GetInstance().GetComponent<Position_data>(guard);
pos->position.x = 500.0f;
```

### Loading from Tiled

Blueprints integrate with the Tiled map editor:

1. In Tiled, create an object in an object layer
2. Set the object's custom property `prefab` to the blueprint path
3. When loading the level, the engine instantiates the blueprint at the object's position

Example Tiled object properties:
```
prefab: "Blueprints/EntityPrefab/guard.json"
```

The Tiled Level Loader automatically:
- Instantiates the blueprint
- Overrides the position from Tiled object coordinates
- Applies any component overrides specified in Tiled

See [Component Overrides](./component-overrides.md) for advanced Tiled integration.

## Blueprint Organization

### Directory Structure

```
Blueprints/
├── EntityPrefab/           # Entity blueprints
│   ├── player.json
│   ├── guard.json
│   ├── treasure_chest.json
│   └── ...
├── AI/                     # Behavior tree definitions
│   ├── patrol_behavior.json
│   ├── wander_behavior.json
│   └── chase_behavior.json
├── Templates/              # Reusable templates
└── README.md
```

### Naming Conventions

- Use lowercase with underscores: `treasure_chest.json`
- Descriptive names: `forest_guard.json`, not `enemy1.json`
- Group related entities: `goblin_warrior.json`, `goblin_archer.json`

## Blueprint Editor

Olympe Engine includes a standalone Blueprint Editor (`OlympeBlueprintEditor.exe`) for visual blueprint editing:

### Features
- Visual component editor
- Property validation
- Real-time preview
- Dependency scanning
- Schema validation

### Usage
1. Launch `OlympeBlueprintEditor.exe`
2. Open an existing blueprint or create a new one
3. Add/remove/edit components
4. Save changes
5. Test in the engine

## Advanced Topics

### Component Overrides

When instantiating blueprints from Tiled, you can override specific component properties. See [Component Overrides](./component-overrides.md) for details.

### Behavior Tree Dependencies

AI entities with `AIBehaviorTree` components reference external behavior tree JSON files. The Blueprint Editor scans these dependencies. See [Behavior Tree Dependency Loading](../../technical-reference/behavior-trees/behavior-tree-dependency-loading.md).

### Blueprint Validation

The `PrefabFactory` validates blueprints on load:

- Schema version compatibility
- Component type existence
- Property type matching
- Required property presence

Invalid blueprints are logged and skipped.

### Hot Reloading (Future)

Currently, blueprint changes require restarting the engine. Future versions will support hot-reloading for rapid iteration.

## Best Practices

1. **Start Simple**: Begin with minimal components, add complexity as needed
2. **Use Descriptive Names**: Clear blueprint names help team communication
3. **Document Complex Blueprints**: Add detailed descriptions for non-obvious designs
4. **Version Control**: Commit blueprints to your repository
5. **Test Early**: Instantiate and test blueprints frequently during development
6. **Reuse Components**: Prefer adding existing components over creating new ones
7. **Keep Related Data Together**: If data is always used together, consider a single component

## Related Documentation

- [Creating Prefabs](./creating-prefabs.md) - Step-by-step guide to creating entity prefabs
- [Component Overrides](./component-overrides.md) - Override blueprint properties in Tiled
- [ECS Overview](../../technical-reference/architecture/ecs-overview.md) - Understanding components and entities
- [Behavior Trees](../../technical-reference/behavior-trees/behavior-trees-overview.md) - AI programming with behavior trees

## Examples

The `Blueprints/` directory includes example blueprints:

- **`example_entity_simple.json`**: Minimal entity example
- **`example_entity_complete.json`**: Entity with all common components
- **`EntityPrefab/`**: Production-ready entity blueprints

Study these examples to understand blueprint structure and best practices.
