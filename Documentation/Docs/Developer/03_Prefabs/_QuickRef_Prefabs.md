# Prefabs Quick Reference

**What:** Reusable entity templates defined in JSON  
**Where:** `Gamedata/EntityPrefab/*.json`  
**Purpose:** Create entities with pre-configured components

---

## Basic Prefab Structure

```json
{
  "schema_version": 2,
  "type": "Player",
  "blueprintType": "EntityPrefab",
  "name": "Player Character",
  "data": {
    "prefabName": "Player",
    "components": [
      {
        "type": "Identity_data",
        "properties": {
          "name": "Player_{id}",
          "tag": "Player",
          "entityType": "Player"
        }
      },
      {
        "type": "Position_data",
        "properties": {
          "position": { "x": 0, "y": 0, "z": 0 }
        }
      }
    ]
  }
}
```

---

## Common Component Examples

### Transform Components
```json
{
  "type": "Position_data",
  "properties": {
    "position": { "x": 100, "y": 200, "z": 0 }
  }
},
{
  "type": "Movement_data",
  "properties": {
    "speed": 200.0,
    "acceleration": 500.0
  }
}
```

### Visual Components
```json
{
  "type": "VisualSprite_data",
  "properties": {
    "spritePath": "./Resources/Sprites/player.png",
    "width": 32,
    "height": 32,
    "layer": 0
  }
}
```

### Physics Components
```json
{
  "type": "PhysicsBody_data",
  "properties": {
    "mass": 1.0,
    "friction": 0.8,
    "useGravity": true
  }
},
{
  "type": "BoundingBox_data",
  "properties": {
    "width": 32,
    "height": 32,
    "offsetX": 0,
    "offsetY": 0
  }
}
```

### AI Components
```json
{
  "type": "AIBlackboard_data",
  "properties": {}
},
{
  "type": "AISenses_data",
  "properties": {
    "visionRadius": 300.0,
    "visionAngle": 120.0,
    "hearingRadius": 500.0,
    "perceptionHz": 5.0,
    "thinkHz": 10.0
  }
}
```

---

## Prefab Loading

### Synonym Registry
`EntityPrefabSynonymsRegister.json` maps type names to prefab files:

```json
{
  "canonicalTypes": {
    "Player": {
      "prefabFile": "player.json",
      "synonyms": ["player", "PLAYER", "PlayerEntity"]
    },
    "Zombie": {
      "prefabFile": "zombie.json",
      "synonyms": ["zombie", "ZOMBIE", "enemy"]
    }
  }
}
```

**Case-insensitive matching:** `"Player"`, `"player"`, and `"PLAYER"` all load `player.json`

### Loading from Code
```cpp
// Load by entity type
EntityID entity = PrefabFactory::CreateEntityFromType("Player", world);

// Load from specific file
EntityID entity = PrefabFactory::LoadPrefabFromFile("Gamedata/EntityPrefab/player.json", world);
```

---

## Scoped Overrides

**Problem:** Multiple components may have same property names (e.g., `width`)

**Solution:** Use component-scoped overrides in Tiled map objects:

### In Tiled Map (TMJ)
Use dot notation for component-scoped properties:
```
Transform.width = 64          # Sets Transform component's width
VisualSprite.width = 32       # Sets VisualSprite component's width
health = 100                  # Legacy flat property (applies to any component with "health")
```

### In Level Loading
Scoped overrides take priority over flat properties:

```cpp
// Priority 1: Component-scoped (from Tiled dot notation)
overrides["Transform"]["width"] = 64

// Priority 2: Flat properties (legacy fallback)
properties["width"] = 32
```

---

## Common Prefab Patterns

### 1. Player Entity
```json
{
  "prefabName": "Player",
  "components": [
    "Identity_data",
    "Position_data",
    "Movement_data",
    "VisualSprite_data",
    "PhysicsBody_data",
    "BoundingBox_data",
    "Health_data",
    "PlayerBinding_data",
    "Controller_data"
  ]
}
```

### 2. Enemy NPC
```json
{
  "prefabName": "Zombie",
  "components": [
    "Identity_data",
    "Position_data",
    "Movement_data",
    "VisualSprite_data",
    "Health_data",
    "AIBlackboard_data",
    "AISenses_data",
    "BehaviorTreeRuntime_data",
    "NavigationAgent_data"
  ]
}
```

### 3. Static Item
```json
{
  "prefabName": "Key",
  "components": [
    "Identity_data",
    "Position_data",
    "VisualSprite_data",
    "BoundingBox_data",
    "Collectable_data"
  ]
}
```

### 4. Trigger Zone
```json
{
  "prefabName": "Trigger",
  "components": [
    "Identity_data",
    "Position_data",
    "BoundingBox_data",
    "TriggerZone_data"
  ]
}
```

---

## Parameter Types

Supported types in `properties`:

| Type | Example | Description |
|------|---------|-------------|
| **Float** | `"speed": 100.0` | Decimal numbers |
| **Int** | `"health": 100` | Integers |
| **Bool** | `"enabled": true` | true/false |
| **String** | `"name": "Player"` | Text |
| **Vector2** | `"position": {"x": 0, "y": 0}` | 2D coordinates |
| **Vector3** | `"position": {"x": 0, "y": 0, "z": 0}` | 3D coordinates |
| **Color** | `"tint": "#FF0000"` | Hex color |
| **Array** | `"points": [1, 2, 3]` | Lists |

---

## Quick Tips

✅ **Always set `schema_version: 2`** for latest format  
✅ **Use synonym registry** for case-insensitive entity type matching  
✅ **Component order doesn't matter** (loaded in dependency order)  
✅ **Missing properties use defaults** from `ParameterSchemas.json`  
✅ **Use scoped overrides** to avoid property name conflicts  
✅ **Test prefabs** by spawning in-game or via editor  

❌ **Don't hardcode entity IDs** (use dynamic references)  
❌ **Don't duplicate components** (each component type once per entity)  
❌ **Don't forget required properties** (check schema for required fields)  

---

## Related Documentation

- **Detailed Guide:** `Creating_Prefabs_Guide.md`
- **Component List:** `01_ECS_Components/_QuickRef_Components.md`
- **Level Loading:** `06_Level_Loading/_QuickRef_Level_Format.md`
- **Scoped Overrides:** `/COMPONENT_SCOPED_OVERRIDES.md` (project root)
