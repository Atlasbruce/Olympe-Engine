---
id: object-properties
title: Object Properties
sidebar_label: Object Properties
sidebar_position: 4
---

# Object Properties in Tiled

Objects in Tiled are used to define interactive elements, spawn points, collision areas, and game logic triggers. Olympe Engine uses custom properties on objects to configure entity behavior.

## Object Types

### Spawn Points

Define where entities appear in the game world.

#### Player Spawn
```json
{
  "type": "PlayerSpawn",
  "properties": {
    "spawnId": "player_start",
    "facing": "right"
  }
}
```

#### NPC Spawn
```json
{
  "type": "NPCSpawn",
  "properties": {
    "prefabPath": "Blueprints/NPCs/Villager.json",
    "behaviorTree": "Blueprints/BehaviorTrees/Wander.json",
    "wanderRadius": 100
  }
}
```

#### Enemy Spawn
```json
{
  "type": "EnemySpawn",
  "properties": {
    "prefabPath": "Blueprints/Enemies/Goblin.json",
    "behaviorTree": "Blueprints/BehaviorTrees/Patrol.json",
    "aggressive": true,
    "detectionRadius": 150
  }
}
```

## Common Object Properties

### Identity Properties
- **name** (string): Unique identifier for the object
- **tag** (string): Grouping tag for similar objects
- **entityType** (string): Entity classification (Player, NPC, Enemy, Item, etc.)

### Transform Properties
- **x**, **y** (float): Position in world coordinates
- **width**, **height** (float): Object dimensions
- **rotation** (float): Rotation in degrees

### Prefab Properties
- **prefabPath** (string): Path to prefab JSON file
  ```
  Example: "Blueprints/Items/HealthPotion.json"
  ```

### AI Properties
- **behaviorTree** (string): Path to behavior tree JSON
- **wanderRadius** (float): Maximum distance for wander behavior
- **patrolPoints** (string): Comma-separated patrol waypoint names
- **aggressive** (bool): Whether entity attacks on sight

### Physics Properties
- **static** (bool): Whether object is immovable
- **solid** (bool): Whether object blocks movement
- **sensor** (bool): Trigger without blocking (useful for zones)

## Object Property Examples

### Item Spawn

Create an item that can be picked up:

```cpp
// In Tiled object layer:
{
  "type": "ItemSpawn",
  "name": "health_potion_01",
  "properties": {
    "prefabPath": "Blueprints/Items/HealthPotion.json",
    "respawnable": true,
    "respawnTime": 30.0
  }
}
```

### Trigger Zone

Create an area that triggers events:

```cpp
// In Tiled object layer:
{
  "type": "TriggerZone",
  "name": "dungeon_entrance",
  "properties": {
    "sensor": true,
    "onEnter": "LoadDungeon",
    "targetMap": "Maps/Dungeon01.tmj"
  }
}
```

### Waypoint

Define navigation waypoints:

```cpp
// In Tiled object layer:
{
  "type": "Waypoint",
  "name": "patrol_point_1",
  "properties": {
    "waypointGroup": "main_patrol",
    "waitTime": 2.0
  }
}
```

## Loading Objects in Code

Objects are automatically loaded from Tiled maps:

```cpp
// TiledMapLoader processes objects during map loading
void World::LoadLevel(const std::string& levelPath) {
    LevelDefinition levelDef = TiledToOlympe::LoadLevel(levelPath);
    
    // Process all objects in object layers
    for (const auto& layer : levelDef.objectLayers) {
        for (const auto& object : layer.objects) {
            CreateEntityFromObject(object);
        }
    }
}

// Custom object processing
void CreateEntityFromObject(const TiledObject& object) {
    if (object.type == "NPCSpawn") {
        EntityID npc = PrefabFactory::CreateFromFile(
            object.GetProperty("prefabPath")
        );
        
        // Set position from object coordinates
        Position_data pos;
        pos.position = Vector(object.x, object.y, 0);
        World::Get().AddComponent<Position_data>(npc, pos);
        
        // Add AI behavior
        std::string btPath = object.GetProperty("behaviorTree");
        BehaviorTree::LoadForEntity(npc, btPath);
    }
}
```

## Object Property Types

Tiled supports various property types that Olympe Engine can read:

| Type | Tiled Type | C++ Type | Usage |
|------|-----------|----------|-------|
| **String** | string | std::string | Paths, names, identifiers |
| **Integer** | int | int | Counts, IDs, enum values |
| **Float** | float | float | Positions, radii, timers |
| **Boolean** | bool | bool | Flags, toggles |
| **Color** | color | SDL_Color | Tint colors, debug colors |
| **File** | file | std::string | File paths (relative to project) |
| **Object Reference** | object | int (object ID) | References to other objects |

## Advanced Object Patterns

### Linked Objects

Create relationships between objects:

```cpp
// Object A
{
  "name": "lever_01",
  "type": "Switch",
  "properties": {
    "targetObject": "gate_01"  // Reference by name
  }
}

// Object B
{
  "name": "gate_01",
  "type": "Gate",
  "properties": {
    "locked": true
  }
}
```

### Prefab Overrides

Override prefab properties per-instance:

```cpp
{
  "type": "NPCSpawn",
  "properties": {
    "prefabPath": "Blueprints/NPCs/Merchant.json",
    // Override specific component values
    "Name": "Bob the Blacksmith",
    "Health.maxHealth": 150,
    "Sprite.tint": "#FF0000"
  }
}
```

## Best Practices

### Naming Conventions
- Use `snake_case` for object names
- Prefix with type: `npc_villager_01`, `item_sword_rare`
- Include sequence numbers for multiples

### Layer Organization
- Put different object types on separate layers
- Name layers clearly: "Spawns_NPCs", "Spawns_Items", "Triggers"
- Use layer visibility to manage complexity

### Property Documentation
- Add comments in Tiled's object properties
- Document custom properties in your prefabs
- Keep a property reference document

### Performance Considerations
- Use object pooling for frequently spawned objects
- Mark static objects as `static: true`
- Limit the number of objects with AI behavior

## Debugging Objects

Enable object debug rendering:

```cpp
// In GameEngine initialization
World::Get().SetDebugRenderObjects(true);

// Objects will render with:
// - Bounding boxes
// - Names
// - Type labels
// - Property icons
```

## Property Validation

Olympe Engine validates object properties at load time:

```cpp
// Example validation in code
bool ValidateObjectProperties(const TiledObject& obj) {
    if (obj.type == "NPCSpawn") {
        if (!obj.HasProperty("prefabPath")) {
            LogError("NPCSpawn missing required 'prefabPath' property");
            return false;
        }
        
        // Validate path exists
        std::string path = obj.GetProperty("prefabPath");
        if (!FileExists(path)) {
            LogError("Prefab not found: " + path);
            return false;
        }
    }
    return true;
}
```

## See Also

- [Layer Properties](./layer-properties.md) - Custom properties for layers
- [Creating Prefabs](../blueprints/creating-prefabs.md) - Define reusable entity templates
- [Behavior Trees](../../technical-reference/behavior-trees/behavior-trees-overview.md) - AI system integration
- [Tiled Quick Start](./tiled-quick-start.md) - Getting started with Tiled
