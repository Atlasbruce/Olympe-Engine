# Runtime Instantiation

Entity prefabs are loaded at runtime and instantiated as ECS entities.

## Loading Pipeline

```
JSON File
    ↓ PrefabLoader::Load()
EntityPrefabGraphDocument
    ↓ ComponentNode iteration
ECS World::Spawn()
    ↓ per-node:
      world.AddComponent<T>(entity, params)
Live Entity (EntityId)
```

## C++ API

```cpp
#include "BlueprintEditor/EntityPrefabEditor/PrefabLoader.h"

// Load prefab
auto doc = Olympe::PrefabLoader::Load("Gamedata/PrefabEntities/guard.json");

// Instantiate
EntityId guard = world.Spawn(*doc);
```

## Component Override at Spawn

```cpp
Olympe::SpawnOverrides overrides;
overrides.Set("Transform", "x", 500.f);
overrides.Set("Health",    "maxHp", 200);

EntityId e = world.Spawn(*doc, overrides);
```

## Tested Prefabs

| File | Nodes | Connections |
|------|-------|-------------|
| `guard.json` | 6 | 5 |
| `player.json` | 8 | 7 |

## Error Handling

- Missing component types log a warning and are skipped
- Invalid parameter types use default values
- Missing JSON files throw `PrefabLoadException`
