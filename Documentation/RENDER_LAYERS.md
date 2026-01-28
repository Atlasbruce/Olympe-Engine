# Render Layer Management System

## Overview

The Olympe Engine now includes a centralized render layer (z-order/depth) management system to ensure correct entity sorting during rendering. This system automatically assigns appropriate depth values to entities based on their type, ensuring proper visual hierarchy.

## Architecture

### RenderLayer Enum

Predefined layers with specific depth ranges:

| Layer            | Z Value | Depth Range | Usage                              |
|------------------|---------|-------------|------------------------------------|
| Background_Far   | -2      | -20000      | Distant parallax backgrounds       |
| Background_Near  | -1      | -10000      | Near backgrounds                   |
| Ground           | 0       | 0           | Floor tiles, terrain               |
| Objects          | 1       | 10000       | Items, decorations, collectibles   |
| Characters       | 2       | 20000       | NPCs, players, enemies             |
| Flying           | 3       | 30000       | Flying enemies, projectiles        |
| Effects          | 4       | 40000       | Particles, VFX                     |
| UI_Near          | 5       | 50000       | UI elements, HUD                   |
| Foreground_Near  | 10      | 100000      | Close foreground elements          |
| Foreground_Far   | 20      | 200000      | Very close overlay                 |

### EntityType Enum

Categories for entities:

- **Player** - Player-controlled characters
- **NPC** - Non-player characters
- **Enemy** - Hostile entities
- **Item** - Static items
- **Collectible** - Pickable items
- **Effect** - Visual effects
- **Particle** - Particle systems
- **UIElement** - User interface elements
- **Background** - Background elements
- **Trigger** - Invisible trigger zones
- **Waypoint** - Navigation waypoints
- **Static** - Static decorative objects
- **Dynamic** - Dynamic objects (default fallback)

## Depth Calculation

The final depth used for rendering is calculated as:

```cpp
depth = position.z * 10000 + position.y
```

- `position.z` = Layer (set by LayerManager)
- `position.y` = Intra-layer sorting (isometric depth)

This ensures:
1. Entities on different layers are always sorted by layer
2. Entities on the same layer are sorted by Y position (isometric perspective)

## Usage

### Automatic Layer Assignment

When creating entities via PrefabFactory, layers are automatically assigned:

```cpp
// Layer is automatically assigned based on EntityType in prefab JSON
EntityID player = PrefabFactory::Get().CreateEntityFromPrefabName("PlayerEntity");
// → entityType = EntityType::Player
// → Layer auto-assigned: RenderLayer::Characters (z=2)
// → Final depth ≈ 20000 + player.y
```

### Explicit Layer Override

You can explicitly specify a layer if needed:

```cpp
// Force a boss to appear on the Flying layer
EntityID dragon = PrefabFactory::Get().CreateEntityFromPrefabName("DragonBoss", RenderLayer::Flying);
// → Layer forced: RenderLayer::Flying (z=3)
// → Final depth = 30000 + dragon.y
```

### Dynamic Layer Changes

Change an entity's layer at runtime:

```cpp
// Make player temporarily appear in the foreground
World::Get().SetEntityLayer(playerEntity, RenderLayer::Effects);

// Return to normal
World::Get().SetEntityLayer(playerEntity, RenderLayer::Characters);
```

### Query Current Layer

Check an entity's current layer:

```cpp
RenderLayer currentLayer = World::Get().GetEntityLayer(entity);
if (currentLayer == RenderLayer::Ground)
{
    // Entity is at ground level
}
```

## Prefab Configuration

### JSON Structure

In entity prefab JSON files, specify the entityType:

```json
{
  "type": "Identity_data",
  "properties": {
    "name": "Player",
    "tag": "Player",
    "entityType": "Player"  // ← This determines the render layer
  }
}
```

### Entity Type to Layer Mapping

The system automatically maps entity types to layers:

| EntityType   | Default RenderLayer |
|--------------|---------------------|
| Player       | Characters          |
| NPC          | Characters          |
| Enemy        | Characters          |
| Item         | Objects             |
| Collectible  | Objects             |
| Effect       | Effects             |
| Particle     | Effects             |
| UIElement    | UI_Near             |
| Background   | Background_Near     |
| Trigger      | Ground              |
| Waypoint     | Ground              |
| Static       | Ground              |
| Dynamic      | Ground              |

## API Reference

### World Class Methods

#### `GetDefaultLayerForType(EntityType type) const`

Returns the default render layer for a given entity type.

**Parameters:**
- `type` - The EntityType to query

**Returns:**
- RenderLayer enum value

**Example:**
```cpp
RenderLayer layer = World::Get().GetDefaultLayerForType(EntityType::Player);
// Returns RenderLayer::Characters
```

#### `SetEntityLayer(EntityID entity, RenderLayer layer)`

Assigns an entity to a specific render layer.

**Parameters:**
- `entity` - The entity to modify
- `layer` - The target render layer

**Effects:**
- Updates the entity's `Position_data.position.z` to match the layer value
- Logs the layer assignment

**Example:**
```cpp
World::Get().SetEntityLayer(playerID, RenderLayer::Flying);
```

#### `GetEntityLayer(EntityID entity) const`

Gets the current render layer of an entity.

**Parameters:**
- `entity` - The entity to query

**Returns:**
- RenderLayer enum value (defaults to Ground if no Position_data)

**Example:**
```cpp
RenderLayer layer = World::Get().GetEntityLayer(itemID);
```

#### `GetNextCustomLayerIndex()`

Returns the next available index for custom dynamic layers.

**Returns:**
- Integer value starting from 21 (Foreground_Far + 1)

**Example:**
```cpp
int customLayer = World::Get().GetNextCustomLayerIndex();
```

### PrefabFactory Methods

#### `CreateEntityFromPrefabName(const std::string& prefabName)`

Creates an entity with automatic layer assignment.

**Parameters:**
- `prefabName` - Name of the prefab to instantiate

**Returns:**
- EntityID of created entity, or INVALID_ENTITY_ID on failure

**Example:**
```cpp
EntityID player = PrefabFactory::Get().CreateEntityFromPrefabName("PlayerEntity");
```

#### `CreateEntityFromPrefabName(const std::string& prefabName, RenderLayer layer)`

Creates an entity with explicit layer override.

**Parameters:**
- `prefabName` - Name of the prefab to instantiate
- `layer` - Render layer to assign (overrides auto-assignment)

**Returns:**
- EntityID of created entity, or INVALID_ENTITY_ID on failure

**Example:**
```cpp
EntityID boss = PrefabFactory::Get().CreateEntityFromPrefabName("Boss", RenderLayer::Flying);
```

## Helper Functions

### `LayerToZ(RenderLayer layer)`

Converts a RenderLayer enum to a z-coordinate value.

```cpp
float z = LayerToZ(RenderLayer::Characters); // Returns 2.0f
```

### `ZToLayer(float z)`

Converts a z-coordinate to a RenderLayer enum (with rounding).

```cpp
RenderLayer layer = ZToLayer(2.3f); // Returns RenderLayer::Characters (rounds to 2)
```

## Best Practices

1. **Use EntityType in prefabs**: Always specify the correct entityType in your prefab JSON files for automatic layer assignment.

2. **Avoid manual z manipulation**: Let the layer system manage position.z values. Only use explicit layer changes when needed.

3. **Layer override sparingly**: Use the layer override only for special cases (e.g., cinematic effects, temporary visual changes).

4. **Check layer consistency**: If entities appear in wrong order, verify their EntityType in prefab files.

5. **Document custom layers**: If using GetNextCustomLayerIndex(), document what your custom layers represent.

## Troubleshooting

### Entities rendering in wrong order

1. Check the EntityType in the prefab JSON
2. Verify the layer assignment in console logs
3. Ensure position.y is set correctly for isometric depth

### Entity appears behind ground

- Likely assigned to Ground or Background layer
- Check EntityType - might be defaulting to "Dynamic" or "Static"
- Use explicit layer override if needed

### Layer changes not visible

- Ensure Position_data component exists on the entity
- Check console logs for layer assignment messages
- Verify the rendering system is using the depth calculation formula

## Implementation Notes

- The system is backward compatible - entities without EntityType will default to Ground layer
- Position.z values are managed by the layer system - don't modify them directly
- The string-to-EntityType conversion uses an efficient map-based lookup
- Layer override skips auto-assignment to avoid redundant processing
