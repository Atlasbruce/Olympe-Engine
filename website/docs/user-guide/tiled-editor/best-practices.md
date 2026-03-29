---
id: best-practices
title: Best Practices
sidebar_label: Best Practices
sidebar_position: 5
---

# Tiled Editor Best Practices

Follow these guidelines to create efficient, maintainable maps for Olympe Engine.

## Project Organization

### Directory Structure
```
Gamedata/
├── Maps/
│   ├── Overworld/
│   │   ├── Village.tmj
│   │   └── Forest.tmj
│   ├── Dungeons/
│   │   └── Cave01.tmj
│   └── Tilesets/
│       ├── Terrain.tsx
│       └── Objects.tsx
├── Blueprints/
│   ├── NPCs/
│   └── Items/
└── Resources/
    └── Textures/
        └── Tilesets/
```

### File Naming
- Use **PascalCase** for map files: `VillageCenter.tmj`, `ForestPath.tmj`
- Use descriptive names: `DungeonLevel1.tmj` not `Level1.tmj`
- Version significant changes: `Boss_Arena_v2.tmj`

## Map Design

### Map Size Guidelines

Choose appropriate map sizes:

- **Small maps** (32×32 to 64×64): Interior rooms, small encounters
- **Medium maps** (64×64 to 128×128): Villages, dungeons, arenas
- **Large maps** (128×128 to 256×256): Overworld areas, expansive dungeons
- **Huge maps** (256×256+): Entire overworld maps

```cpp
// Example: Creating a medium-sized village map
Map Size: 80×60 tiles
Tile Size: 32×32 pixels
Total Size: 2560×1920 pixels
```

### Performance Considerations

#### Tile Layers
- Limit tile layers to 10-15 maximum
- Merge static layers when possible
- Use fewer layers for large maps

#### Object Count
- Keep objects under 500 per map for best performance
- Use object pooling for similar entities
- Consider breaking large maps into zones

#### Tileset Optimization
- Use texture atlases (single image tilesets)
- Keep tilesets under 2048×2048 pixels
- Share tilesets across multiple maps

## Layer Organization

### Recommended Layer Structure

```
Layer Hierarchy:
├── Sky (Background layer, static)
├── Background_Parallax (-1 parallax, optional)
├── Background_Tiles (Ground layer, z=0)
├── Details_Lower (Objects layer, z=1)
├── Collision (Collision layer, invisible)
├── Navigation (Navigation mesh layer, invisible)
├── Objects (Object placement layer)
├── Details_Upper (Objects layer, z=3, foreground)
└── Foreground (z=5, overlays)
```

### Layer Naming Conventions

Use prefixes to indicate layer type:
- `Tiles_*` - Tile layers
- `Objects_*` - Object layers
- `Collision_*` - Collision data
- `Nav_*` - Navigation mesh
- `Trigger_*` - Trigger zones

Example names:
```
Tiles_Ground
Tiles_Decoration
Objects_NPCs
Objects_Items
Collision_Walls
Nav_Walkable
Trigger_Zones
```

## Custom Properties

### Layer Properties Best Practices

Always set these properties on tile layers:

```json
{
  "renderLayer": "Ground",     // Required: Z-order
  "isometric": false,          // Set true for isometric maps
  "parallaxX": 1.0,           // Camera parallax effect
  "parallaxY": 1.0,
  "depthSorting": false       // Enable for isometric layers
}
```

### Object Properties Best Practices

Include all required properties:

```json
{
  "name": "unique_identifier",
  "type": "NPCSpawn",
  "prefabPath": "Blueprints/NPCs/Guard.json",
  "behaviorTree": "Blueprints/BehaviorTrees/Guard.json"
}
```

### Property Naming
- Use **camelCase** for property names
- Be descriptive: `detectionRadius` not `radius`
- Use consistent naming across maps

## Collision Design

### Collision Tiles
- Use dedicated collision tileset
- Mark non-walkable tiles consistently
- Test all collision boundaries

```cpp
// Set collision property on tileset tiles
{
  "collision": true,    // Basic collision
  "solid": true,       // Blocks all movement
  "height": 1.0        // Collision height
}
```

### Collision Shapes
- Use simple shapes (rectangles) when possible
- Complex shapes hurt performance
- Group collision tiles for efficiency

## Navigation Mesh

### Navmesh Layer Setup
```json
{
  "name": "Nav_Walkable",
  "type": "tilelayer",
  "properties": {
    "navigation": true,
    "walkable": true
  }
}
```

### Navmesh Best Practices
- Cover all walkable areas
- Leave gaps for obstacles
- Test pathfinding with visualization
- Mark stairs/ramps with special tiles

## Tileset Management

### Creating Tilesets

1. **Use TSX files** (external tilesets)
   - Easier to maintain
   - Shared across maps
   - Better version control

2. **Set tile properties once**
   ```cpp
   // In tileset, not per-map instance
   Tile Properties:
   - collision: true
   - animated: true
   - sound: "stone"
   ```

3. **Organize by theme**
   - Terrain.tsx - ground, walls
   - Props.tsx - decorations
   - Characters.tsx - animated sprites

### Tileset Properties

Set useful properties:

```json
{
  "tilewidth": 32,
  "tileheight": 32,
  "spacing": 0,      // Pixels between tiles
  "margin": 0,       // Pixels around tileset
  "tileoffset": {    // Rendering offset
    "x": 0,
    "y": 0
  }
}
```

## Prefab Integration

### Spawning Prefabs

Use objects to spawn prefabs:

```cpp
// Object in Tiled
{
  "type": "PrefabSpawn",
  "properties": {
    "prefabPath": "Blueprints/Items/Chest.json",
    "containsItem": "Blueprints/Items/Key.json"
  }
}

// Loaded as entity in Olympe Engine
EntityID chest = PrefabFactory::CreateFromFile(
    "Blueprints/Items/Chest.json"
);
```

### Prefab Property Overrides

Override prefab components per-instance:

```json
{
  "prefabPath": "Blueprints/Enemies/Orc.json",
  "Health.maxHealth": 200,     // Override max health
  "Sprite.scale": 1.5,         // Make larger
  "Identity.name": "Orc Chief" // Custom name
}
```

## Testing and Iteration

### In-Editor Testing
1. Use Tiled's preview mode
2. Check layer visibility
3. Verify object placement
4. Test different zoom levels

### In-Game Testing
```cpp
// Enable debug rendering
World::Get().ToggleGrid(true);
World::Get().ToggleCollisionOverlay(true);
World::Get().ToggleNavigationOverlay(true);
```

### Performance Testing
- Monitor FPS with large maps
- Check memory usage
- Test with many entities spawned
- Profile rendering times

## Version Control

### What to Commit
✅ Commit these files:
- `.tmj` map files
- `.tsx` tileset files
- Custom property templates

❌ Don't commit:
- Autosave files (`*.tmj~`)
- Backup files (`*.bak`)
- Temporary exports

### Map Metadata

Include metadata in map properties:

```json
{
  "author": "YourName",
  "version": "1.2",
  "created": "2024-01-15",
  "description": "Village center with shops and NPCs"
}
```

## Common Pitfalls

### ❌ Don't Do This
- Using inline tilesets (embed images in maps)
- Hardcoding positions in code instead of using objects
- Creating giant single-layer maps
- Forgetting to set renderLayer property
- Using inconsistent tile sizes

### ✅ Do This
- Use external TSX tilesets
- Place all entities with objects
- Use multiple layers appropriately
- Always set layer properties
- Maintain consistent tile sizes

## Optimization Checklist

Before finalizing a map:

- [ ] Remove unused layers
- [ ] Merge static layers where possible
- [ ] Optimize tileset images (compress PNGs)
- [ ] Set appropriate render layers
- [ ] Test collision boundaries
- [ ] Verify navigation mesh coverage
- [ ] Check object property completeness
- [ ] Test performance with target entity count
- [ ] Enable depth sorting only where needed
- [ ] Document custom properties

## Advanced Techniques

### Parallax Backgrounds

Create depth with multiple background layers:

```json
// Far background
{
  "name": "Background_Far",
  "renderLayer": "Background_Far",
  "parallaxX": 0.2,
  "parallaxY": 0.2
}

// Near background
{
  "name": "Background_Near",
  "renderLayer": "Background_Near",
  "parallaxX": 0.5,
  "parallaxY": 0.5
}
```

### Isometric Maps

For isometric projection:

```json
{
  "orientation": "isometric",
  "properties": {
    "isometric": true,
    "depthSorting": true,
    "tileoffsetX": 0,
    "tileoffsetY": 16  // Half tile height
  }
}
```

### Animated Tiles

Set up tile animations in tileset:

```json
{
  "id": 42,
  "animation": [
    {"tileid": 42, "duration": 100},
    {"tileid": 43, "duration": 100},
    {"tileid": 44, "duration": 100},
    {"tileid": 45, "duration": 100}
  ]
}
```

## Resources

### Templates
- Example maps: `Examples/Maps/`
- Template tilesets: `Templates/Tilesets/`
- Prefab templates: `Templates/Blueprints/`

### Documentation
- [Tiled Quick Start](./tiled-quick-start.md)
- [Layer Properties Reference](./layer-properties.md)
- [Object Properties Reference](./object-properties.md)

### External Resources
- [Tiled Documentation](https://doc.mapeditor.org/)
- [OpenGameArt](https://opengameart.org/) - Free tilesets
- [itch.io](https://itch.io/game-assets/free) - Asset packs

## Troubleshooting

### Map Not Loading
- Check file path is correct
- Verify JSON syntax (use JSON validator)
- Ensure referenced tilesets exist
- Check console for error messages

### Collision Not Working
- Verify collision layer properties set
- Check tile collision properties in tileset
- Enable collision overlay for debugging

### Objects Not Spawning
- Check object type is recognized
- Verify prefab paths are correct
- Ensure all required properties are set
- Check console for loading errors

### Performance Issues
- Reduce number of layers
- Optimize tileset sizes
- Decrease object count
- Profile with debug tools
