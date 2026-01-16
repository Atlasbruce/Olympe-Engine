# Tiled Object Types Reference

## Overview

This document provides a complete reference for all object types supported by Olympe Engine's Tiled integration. Each object type maps to a specific entity prefab and supports various custom properties.

## Object Type Mapping Table

| Object Type | Prefab Path | Color | Description |
|-------------|-------------|-------|-------------|
| `player` | `Blueprints/EntityPrefab/player_entity.json` | Green (#00FF00) | Player spawn point |
| `npc` | `Blueprints/EntityPrefab/npc_entity.json` | Blue (#0000FF) | Non-hostile NPC character |
| `guard` | `Blueprints/EntityPrefab/guard_npc.json` | Light Blue (#0080FF) | Patrolling guard NPC |
| `enemy` | `Blueprints/EntityPrefab/enemy.json` | Red (#FF0000) | Generic hostile enemy |
| `zombie` | `Blueprints/EntityPrefab/zombie.json` | Dark Red (#800000) | Zombie enemy type |
| `key` | `Blueprints/EntityPrefab/key.json` | Yellow (#FFFF00) | Key item for doors |
| `treasure` | `Blueprints/EntityPrefab/treasure.json` | Gold (#FFD700) | Treasure chest |
| `collectible` | `Blueprints/EntityPrefab/item.json` | Orange (#FFA500) | Generic collectible item |
| `trigger` | `Blueprints/EntityPrefab/trigger.json` | Cyan (#00FFFF) | Invisible trigger zone |
| `exit` | `Blueprints/EntityPrefab/portal.json` | Green (#00FF00) | Level exit portal |
| `door` | `Blueprints/EntityPrefab/portal.json` | Brown (#804000) | Door (potentially locked) |
| `portal` | `Blueprints/EntityPrefab/portal.json` | Magenta (#FF00FF) | Teleporter portal |
| `waypoint` | `Blueprints/EntityPrefab/waypoint.json` | Cyan (#00FFFF) | Generic waypoint marker |
| `collision` | *(auto-generated)* | Red (#FF0000) | Collision zone (not spawned as entity) |
| `way` | *(auto-generated)* | Magenta (#FF00FF) | Patrol path (not spawned as entity) |
| `sector` | *(auto-generated)* | Yellow (#FFFF00) | Level sector boundary (not spawned as entity) |

## Object Type Details

### Player

**Type:** `player`  
**Prefab:** `Blueprints/EntityPrefab/player_entity.json`  
**Geometry:** Point

**Description:**  
Defines where the player spawns when the level loads. Only one player spawn point should exist per map.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `health` | int | 100 | Starting health |
| `speed` | float | 100.0 | Movement speed (pixels/second) |
| `facing` | string | "right" | Initial facing direction |

**Example in Tiled:**
```
Object: Point
  ├─ Name: "player_spawn"
  ├─ Type: "player"
  ├─ X: 320, Y: 240
  └─ Properties:
      ├─ health (int) = 100
      └─ speed (float) = 120.0
```

**Usage Notes:**
- Use exactly one player spawn per level
- Position determines initial camera position
- Player entity is created by `PrefabFactory::CreateEntity("player")`

---

### NPC

**Type:** `npc`  
**Prefab:** `Blueprints/EntityPrefab/npc_entity.json`  
**Geometry:** Point

**Description:**  
Non-hostile character that can display dialogue, sell items, or provide quests.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `friendlyName` | string | "" | Display name shown to player |
| `dialogueFile` | file | "" | Path to dialogue JSON |
| `speed` | float | 50.0 | Movement speed |
| `interactRadius` | float | 32.0 | Distance player can interact from |

**Example:**
```
Object: Point
  ├─ Name: "village_elder"
  ├─ Type: "npc"
  ├─ X: 400, Y: 300
  └─ Properties:
      ├─ friendlyName (string) = "Elder Aldric"
      └─ dialogueFile (file) = "Dialogue/elder_greeting.json"
```

**Usage Notes:**
- NPCs don't attack player
- Can have idle animations
- Can be stationary or patrol (without `patrolPathName`, stays in place)

---

### Guard

**Type:** `guard`  
**Prefab:** `Blueprints/EntityPrefab/guard_npc.json`  
**Geometry:** Point

**Description:**  
Patrolling NPC that follows a defined path. Can be neutral or become hostile.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `patrolPathName` | string | **(Required)** | Name of polyline "way" object |
| `speed` | float | 60.0 | Movement speed |
| `alertRadius` | float | 100.0 | Distance to detect player |
| `hostile` | bool | false | Attacks player on detection |

**Example:**
```
Object: Point
  ├─ Name: "castle_guard_1"
  ├─ Type: "guard"
  ├─ X: 500, Y: 200
  └─ Properties:
      ├─ patrolPathName (string) = "patrol_courtyard"
      ├─ speed (float) = 80.0
      └─ alertRadius (float) = 120.0
```

**Related:**  
Must have a corresponding `way` polyline with matching name:
```
Object: Polyline
  ├─ Name: "patrol_courtyard"
  ├─ Type: "way"
  └─ Points: [(500,200), (600,200), (600,300), (500,300)]
```

**Usage Notes:**
- Guard walks along polyline path in loop
- Path must have at least 2 points
- Guard starts at object position, then moves to first path point
- If `hostile: true`, attacks player within `alertRadius`

---

### Enemy

**Type:** `enemy`  
**Prefab:** `Blueprints/EntityPrefab/enemy.json`  
**Geometry:** Point

**Description:**  
Generic hostile entity that chases and attacks the player.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `health` | int | 50 | Maximum health |
| `damage` | int | 10 | Damage dealt per hit |
| `speed` | float | 80.0 | Movement speed |
| `aggroRadius` | float | 150.0 | Distance to start chasing player |
| `attackRange` | float | 32.0 | Distance to attack |

**Example:**
```
Object: Point
  ├─ Name: "goblin_warrior"
  ├─ Type: "enemy"
  ├─ X: 800, Y: 400
  └─ Properties:
      ├─ health (int) = 75
      ├─ damage (int) = 15
      └─ speed (float) = 90.0
```

**Usage Notes:**
- Idle until player enters `aggroRadius`
- Chases player when aggro'd
- Attacks when within `attackRange`
- Dies when health reaches 0

---

### Zombie

**Type:** `zombie`  
**Prefab:** `Blueprints/EntityPrefab/zombie.json`  
**Geometry:** Point

**Description:**  
Slow-moving undead enemy with high health but low speed.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `health` | int | 100 | Maximum health |
| `damage` | int | 20 | Damage dealt per hit |
| `speed` | float | 50.0 | Movement speed (slower than normal enemy) |
| `aggroRadius` | float | 200.0 | Detection range (larger than normal) |

**Example:**
```
Object: Point
  ├─ Name: "zombie_1"
  ├─ Type: "zombie"
  ├─ X: 300, Y: 500
  └─ Properties:
      └─ speed (float) = 40.0
```

**Usage Notes:**
- Slower than generic enemies
- Higher health pool
- Always hostile
- Cannot patrol (no patrol path support)

---

### Key

**Type:** `key`  
**Prefab:** `Blueprints/EntityPrefab/key.json`  
**Geometry:** Point

**Description:**  
Collectible key item used to unlock doors.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `keyId` | string | **(Required)** | Unique identifier for this key |
| `consumable` | bool | true | Destroyed when used |

**Example:**
```
Object: Point
  ├─ Name: "red_key"
  ├─ Type: "key"
  ├─ X: 150, Y: 150
  └─ Properties:
      └─ keyId (string) = "dungeon_red"
```

**Related:**  
Used with `door` objects:
```
Object: Point (door)
  └─ Properties:
      └─ requiresKey (string) = "dungeon_red"
```

**Usage Notes:**
- Player collects key on touch
- Stored in player inventory
- Used automatically when interacting with matching door
- Can be reusable if `consumable: false`

---

### Treasure

**Type:** `treasure`  
**Prefab:** `Blueprints/EntityPrefab/treasure.json`  
**Geometry:** Point

**Description:**  
Treasure chest that gives gold/items when opened.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `value` | int | 100 | Gold amount given |
| `itemId` | string | "" | Item ID (if giving item instead of gold) |
| `locked` | bool | false | Requires key to open |
| `requiresKey` | string | "" | Key ID required if locked |

**Example:**
```
Object: Point
  ├─ Name: "chest_room1"
  ├─ Type: "treasure"
  ├─ X: 700, Y: 300
  └─ Properties:
      ├─ value (int) = 250
      ├─ locked (bool) = true
      └─ requiresKey (string) = "dungeon_red"
```

**Usage Notes:**
- Player interacts to open
- If locked, requires matching key
- Destroyed after opening (one-time use)
- Can contain gold OR item, not both

---

### Collectible

**Type:** `collectible`  
**Prefab:** `Blueprints/EntityPrefab/item.json`  
**Geometry:** Point

**Description:**  
Generic collectible item (potions, scrolls, etc).

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `itemId` | string | **(Required)** | Item type identifier |
| `quantity` | int | 1 | Stack size |
| `autoPickup` | bool | true | Collected on touch vs requires interact |

**Example:**
```
Object: Point
  ├─ Name: "health_potion"
  ├─ Type: "collectible"
  ├─ X: 200, Y: 400
  └─ Properties:
      ├─ itemId (string) = "potion_health"
      └─ quantity (int) = 1
```

**Usage Notes:**
- Flexible item type (define meaning in game logic)
- Auto-collected by default
- Added to player inventory

---

### Exit

**Type:** `exit`  
**Prefab:** `Blueprints/EntityPrefab/portal.json`  
**Geometry:** Point or Rectangle

**Description:**  
Level exit that loads a new map when player enters.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `targetLevel` | string | **(Required)** | Path to .tmj file to load |
| `targetX` | float | 0.0 | Spawn X in target level (optional) |
| `targetY` | float | 0.0 | Spawn Y in target level (optional) |

**Example:**
```
Object: Rectangle
  ├─ Name: "level_exit"
  ├─ Type: "exit"
  ├─ X: 900, Y: 400
  ├─ Width: 64, Height: 64
  └─ Properties:
      ├─ targetLevel (string) = "Resources/Maps/level2.tmj"
      ├─ targetX (float) = 100.0
      └─ targetY (float) = 100.0
```

**Usage Notes:**
- Use rectangle for visible portal area
- Use point for invisible exit triggers
- Level loads when player overlaps
- If `targetX`/`targetY` omitted, uses target map's player spawn

---

### Door

**Type:** `door`  
**Prefab:** `Blueprints/EntityPrefab/portal.json`  
**Geometry:** Point or Rectangle

**Description:**  
Door to another map, optionally requiring a key.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `targetLevel` | string | **(Required)** | Path to .tmj file |
| `requiresKey` | string | "" | Key ID required (if locked) |
| `targetX` | float | 0.0 | Spawn X in target level |
| `targetY` | float | 0.0 | Spawn Y in target level |

**Example:**
```
Object: Rectangle
  ├─ Name: "dungeon_door"
  ├─ Type: "door"
  ├─ X: 600, Y: 200
  ├─ Width: 32, Height: 64
  └─ Properties:
      ├─ targetLevel (string) = "Resources/Maps/dungeon.tmj"
      └─ requiresKey (string) = "dungeon_red"
```

**Usage Notes:**
- If `requiresKey` set, player must have key in inventory
- Displays message if player lacks key
- Same prefab as `exit` and `portal`

---

### Portal

**Type:** `portal`  
**Prefab:** `Blueprints/EntityPrefab/portal.json`  
**Geometry:** Point or Rectangle

**Description:**  
Magical portal teleporter between maps or within same map.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `targetLevel` | string | **(Required)** | Path to .tmj file (or same map) |
| `targetX` | float | **(Required)** | Destination X coordinate |
| `targetY` | float | **(Required)** | Destination Y coordinate |
| `bidirectional` | bool | false | Create return portal at destination |

**Example:**
```
Object: Point
  ├─ Name: "portal_to_tower"
  ├─ Type: "portal"
  ├─ X: 400, Y: 600
  └─ Properties:
      ├─ targetLevel (string) = "Resources/Maps/wizard_tower.tmj"
      ├─ targetX (float) = 200.0
      ├─ targetY (float) = 200.0
      └─ bidirectional (bool) = true
```

**Usage Notes:**
- Can teleport within same level (use same .tmj path)
- Instant teleport when player touches
- If `bidirectional: true`, system creates return portal (advanced feature)

---

### Waypoint

**Type:** `waypoint`  
**Prefab:** `Blueprints/EntityPrefab/waypoint.json`  
**Geometry:** Point

**Description:**  
Generic marker for scripting, fast travel, or objectives.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `waypointId` | string | "" | Unique identifier |
| `visible` | bool | false | Rendered in-game or invisible |
| `unlocked` | bool | true | Accessible to player |

**Example:**
```
Object: Point
  ├─ Name: "town_square_waypoint"
  ├─ Type: "waypoint"
  ├─ X: 500, Y: 500
  └─ Properties:
      ├─ waypointId (string) = "town_square"
      ├─ visible (bool) = true
      └─ unlocked (bool) = false
```

**Usage Notes:**
- Flexible purpose (fast travel, objectives, scripting)
- Game logic decides behavior
- Not interactive by default

---

### Collision (Special)

**Type:** `collision`  
**Prefab:** *(None - auto-handled)*  
**Geometry:** Rectangle, Polygon, or Polyline

**Description:**  
Defines impassable areas (walls, obstacles). Automatically converted to collision data, not spawned as entities.

**Properties:**  
None required. All properties ignored.

**Example:**
```
Object: Rectangle
  ├─ Type: "collision"
  ├─ X: 100, Y: 100
  ├─ Width: 64, Height: 128
```

**Usage Notes:**
- Use dedicated "Collision" object layer
- System creates `CollisionZone_data` components automatically
- Supports rectangles (axis-aligned), polygons (arbitrary shapes), polylines (paths)
- Not rendered in-game (editor-only visualization)

---

### Way (Special)

**Type:** `way`  
**Prefab:** *(None - auto-handled)*  
**Geometry:** Polyline

**Description:**  
Defines a patrol path for guards/NPCs. Converted to AI waypoint data, not spawned as entity.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `name` | string | **(Required)** | Unique path identifier (guards reference this) |
| `looped` | bool | true | Return to start after reaching end |
| `bidirectional` | bool | false | Patrol forward then backward |

**Example:**
```
Object: Polyline
  ├─ Name: "patrol_castle_walls"
  ├─ Type: "way"
  ├─ Points: [(200,100), (400,100), (400,300), (200,300)]
  └─ Properties:
      └─ looped (bool) = true
```

**Usage Notes:**
- Must have `name` property (guards use `patrolPathName` to reference)
- At least 2 points required
- Points are world coordinates (pixels)
- Converted to `AIBlackboard_data.patrolPoints` in guard entities

---

### Sector (Special)

**Type:** `sector`  
**Prefab:** *(None - auto-handled)*  
**Geometry:** Polygon or Rectangle

**Description:**  
Defines a level region for streaming, LOD, or gameplay zones. Stored as metadata, not spawned as entity.

**Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `biome` | string | "" | Region type (forest, desert, cave) |
| `music` | file | "" | Audio track for this sector |
| `ambientColor` | color | "" | Lighting tint (#RRGGBB) |
| `spawnRate` | float | 1.0 | Enemy spawn multiplier |
| `fogDensity` | float | 0.0 | Fog intensity (0.0-1.0) |

**Example:**
```
Object: Polygon
  ├─ Name: "dark_forest"
  ├─ Type: "sector"
  ├─ Points: [(0,0), (500,0), (600,400), (100,500)]
  └─ Properties:
      ├─ biome (string) = "forest_dark"
      ├─ music (file) = "Audio/creepy_forest.ogg"
      ├─ ambientColor (color) = "#404040"
      └─ spawnRate (float) = 2.5
```

**Usage Notes:**
- Used for LOD, streaming, gameplay logic
- See [TILED_SECTORS.md](TILED_SECTORS.md) for details
- Flexible custom properties (define any metadata you need)
- Polygon for organic shapes, rectangle for grid-based

---

## Custom Property Best Practices

### Naming Conventions

- Use camelCase: `patrolPathName`, `friendlyName`, `itemId`
- Be descriptive: `alertRadius` instead of `radius`
- Prefix for grouping: `spawn_type`, `spawn_count`

### Type Selection

- Use **int** for counts, IDs (discrete values)
- Use **float** for physics, distances, timers (continuous values)
- Use **string** for identifiers, file references
- Use **bool** for flags
- Use **file** for asset paths (enables file browser in Tiled)
- Use **color** for RGB values (enables color picker)

### Default Values

Set sensible defaults in `objecttypes.xml` to reduce repetitive property setting.

### Documentation

Add comments in your `tiled_prefab_mapping.json`:
```json
{
  "mapping": {
    "guard": "Blueprints/EntityPrefab/guard_npc.json"
  },
  "notes": {
    "guard": "Requires patrolPathName property to reference a 'way' polyline"
  }
}
```

## Adding New Object Types

### Step 1: Create Prefab

Create entity prefab JSON in `Blueprints/EntityPrefab/`:

```json
{
  "name": "my_entity",
  "components": {
    "Position_data": { "x": 0, "y": 0 },
    "Sprite_data": { "texturePath": "Resources/Sprites/my_entity.png" }
  }
}
```

### Step 2: Add to Mapping

Edit `Config/tiled_prefab_mapping.json`:

```json
{
  "mapping": {
    "my_entity": "Blueprints/EntityPrefab/my_entity.json"
  }
}
```

### Step 3: Add to objecttypes.xml

Edit `Templates/Tiled/objecttypes.xml`:

```xml
<objecttype name="my_entity" color="#00FF00">
  <property name="prefabPath" type="string" default="Blueprints/EntityPrefab/my_entity.json"/>
  <property name="customProp" type="int" default="42"/>
</objecttype>
```

### Step 4: Use in Tiled

1. Import updated `objecttypes.xml`
2. Create object
3. Set type to "my_entity"
4. Customize properties as needed

## Prefab Fallback Behavior

If object type is not in mapping:

1. Check if object has `prefabPath` custom property → use that
2. Else, use `defaultPrefab` from `ConversionConfig`
3. Else, log warning and skip entity

**Example:**
```
Object: Point
  ├─ Type: "unknown_type"  (not in mapping)
  └─ Properties:
      └─ prefabPath (string) = "Blueprints/EntityPrefab/custom.json"
```
System will use `custom.json` instead of failing.

## See Also

- [TILED_INTEGRATION.md](TILED_INTEGRATION.md) - Integration overview and API
- [TILED_WORKFLOW.md](TILED_WORKFLOW.md) - Using object types in practice
- [Config/tiled_prefab_mapping.json](../Config/tiled_prefab_mapping.json) - Current mapping configuration
- [Templates/Tiled/objecttypes.xml](../Templates/Tiled/objecttypes.xml) - Object type definitions for Tiled
