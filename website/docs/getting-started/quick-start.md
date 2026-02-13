---
id: quick-start
title: Quick Start
sidebar_label: Quick Start
sidebar_position: 3
---

# Quick Start Guide

Get up and running with Olympe Engine in minutes!

## Your First Scene

Olympe Engine uses JSON blueprints to define entities. Create a simple player entity:

### 1. Create a Blueprint

Create `Blueprints/Player.json`:

```json
{
  "components": [
    {
      "type": "Identity_data",
      "name": "Player"
    },
    {
      "type": "Transform_data",
      "position": [0, 0],
      "rotation": 0,
      "scale": [1, 1]
    },
    {
      "type": "Sprite_data",
      "texturePath": "player.png"
    }
  ]
}
```

### 2. Load the Blueprint

```cpp
EntityID player = PrefabFactory::Get().CreateEntityFromPrefabName("Player");
```

### 3. Run Your Game

Build and run the engine to see your player entity!

## Next Steps

- Explore the [API Reference](../api-reference/index) for detailed documentation
- Learn about [ECS architecture](../api-reference/index#core-systems)
- Study existing blueprints in the `Blueprints/` directory

## Common Tasks

### Moving an Entity

```cpp
auto* transform = world.GetComponent<Transform_data>(player);
transform->position.x += 10.0f;
```

### Adding Components

```cpp
Health_data health;
health.currentHealth = 100.0f;
world.AddComponent(player, health);
```

## Resources

- [Full API Documentation](../api-reference/index)
- [Example Projects](https://github.com/Atlasbruce/Olympe-Engine/tree/master/Examples)
- [Tiled Integration Guide](https://github.com/Atlasbruce/Olympe-Engine/blob/master/TILED_QUICK_START.md)
