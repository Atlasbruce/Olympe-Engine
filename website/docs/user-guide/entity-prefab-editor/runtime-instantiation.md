---
id: runtime-instantiation
title: Runtime Instantiation
sidebar_label: Runtime Instantiation
sidebar_position: 4
---

# Runtime Instantiation

Entity prefabs are loaded at runtime via `PrefabLoader` and instantiated as ECS entities.

## Loading a Prefab

```cpp
#include "BlueprintEditor/EntityPrefabEditor/PrefabLoader.h"

// Load from JSON file
auto prefab = Olympe::PrefabLoader::Load("Gamedata/PrefabEntities/enemy.json");

// Instantiate as ECS entity
EntityId entity = world.Spawn(prefab);
```

## Instantiation Pipeline

```mermaid
graph LR
    A[JSON File] --> B[PrefabLoader::Load]
    B --> C[EntityPrefabGraphDocument]
    C --> D[ComponentNode list]
    D --> E[ECS World::Spawn]
    E --> F[Live Entity]
```

## Component Overrides

You can override component parameters at spawn time:

```cpp
Olympe::PrefabOverrides overrides;
overrides.Set("Transform", "position", Vector3(100.f, 200.f, 0.f));
overrides.Set("Health", "maxHp", 150);

EntityId entity = world.Spawn(prefab, overrides);
```

## JSON Schema

```json
{
  "version": 4,
  "nodes": [
    {
      "nodeId": 1,
      "componentType": "Transform",
      "componentName": "Position",
      "position": [0, 0],
      "parameters": {
        "position": [0.0, 0.0, 0.0],
        "rotation": 0.0,
        "scale": [1.0, 1.0]
      }
    }
  ],
  "connections": [],
  "canvasState": {
    "zoom": 1.0,
    "offset": [0.0, 0.0]
  }
}
```

## Related

- [Component Library](component-library)
- [ECS Overview](../../technical-reference/architecture/ecs-overview)
