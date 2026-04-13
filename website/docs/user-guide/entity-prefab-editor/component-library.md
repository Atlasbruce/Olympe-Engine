---
id: component-library
title: Component Library
sidebar_label: Component Library
sidebar_position: 2
---

# Component Library

The Component Library defines all available component types that can be added to entity prefabs. Component definitions are stored in `Gamedata/PrefabEntities/ComponentsParameters.json`.

## Standard Components

| Component | Category | Description |
|-----------|----------|-------------|
| Transform | Core | Position, rotation, scale |
| Identity | Core | Entity name and tags |
| Movement | Core | Velocity and movement |
| Sprite | Graphics | 2D sprite rendering |
| Collision | Physics | Collision shape and layers |
| Health | Gameplay | Hit points management |
| AIBlackboard | AI | Shared AI state variables |
| BehaviorTree | AI | BT graph assignment |
| VisualSprite | Graphics | Advanced sprite with animations |
| AnimationController | Graphics | Animation state machine |

## Parameter Types

| Type | JSON key | Example |
|------|----------|---------|
| Bool | `"bool"` | `true` |
| Int | `"int"` | `42` |
| Float | `"float"` | `1.5` |
| String | `"string"` | `"player"` |
| Vector2 | `"vector2"` | `[1.0, 2.0]` |
| Vector3 | `"vector3"` | `[1.0, 2.0, 0.0]` |
| Color | `"color"` | `[255, 0, 0, 255]` |
| EntityRef | `"entityRef"` | `"target_entity"` |
| Array | `"array"` | `[...]` |

## Adding Custom Components

To register a new component type, add an entry to `ComponentsParameters.json`:

```json
{
  "components": [
    {
      "name": "MyComponent",
      "category": "Gameplay",
      "description": "Custom game logic component",
      "parameters": [
        { "name": "speed", "type": "float", "default": 1.0 },
        { "name": "tag",   "type": "string", "default": "" }
      ]
    }
  ]
}
```

## Related

- [Entity Prefab Overview](entity-prefab-overview)
- [Editor Guide](editor-guide)
