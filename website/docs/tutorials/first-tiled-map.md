---
id: first-tiled-map
title: Creating Your First Tiled Map
sidebar_label: First Tiled Map
sidebar_position: 1
---

# Creating Your First Tiled Map

Learn how to create a game level using Tiled Map Editor and load it into Olympe Engine. This tutorial covers everything from setting up your first map to placing objects and configuring layer properties.

## What You'll Learn

- Installing and configuring Tiled Map Editor
- Creating a new orthogonal and isometric map
- Setting up tilesets and drawing terrain
- Adding object layers and placing game entities
- Configuring layer properties for parallax and z-ordering
- Using component-scoped properties for entity customization
- Loading and testing your map in Olympe Engine

## Prerequisites

- Olympe Engine installed and running
- Basic understanding of tilemaps and 2D game development
- Tiled Map Editor (download from [mapeditor.org](https://www.mapeditor.org/))

---

## Part 1: Installing Tiled Map Editor

### Step 1: Download Tiled

1. Visit [https://www.mapeditor.org/](https://www.mapeditor.org/)
2. Download the latest stable version for your platform (Windows, macOS, or Linux)
3. Install Tiled following the platform-specific instructions
4. Launch Tiled to verify installation

### Step 2: Configure Tiled for Olympe

Tiled works with Olympe out of the box, but here are some recommended settings:

1. Open Tiled preferences (**Edit → Preferences**)
2. Under **General** tab:
   - Set **Object selection behavior** to "Single click"
   - Enable **Highlight current layer**
3. Under **Interface** tab:
   - Enable **Show tile animations**
   - Enable **Show tile collision shapes** (if using collision tiles)

---

## Part 2: Creating Your First Orthogonal Map

Orthogonal maps use a standard grid layout (like classic top-down RPGs).

### Step 1: Create New Map

1. Click **File → New → New Map...** (or press `Ctrl+N`)
2. Configure map settings:
   - **Orientation**: Orthogonal
   - **Tile layer format**: CSV (easier to read) or Base64 (compressed)
   - **Tile render order**: Right Down
   - **Map size**: 30 tiles wide × 20 tiles high
   - **Tile size**: 32 pixels × 32 pixels
   - **Infinite**: Unchecked (fixed size map)

3. Click **Save As** and choose the **TMJ format** (Tiled JSON)
   - Save to `Resources/Maps/my_first_level.tmj`
   - TMJ is text-based and version-control friendly

### Step 2: Add a Tileset

A tileset is a collection of tiles (images) used to paint your map.

1. Click **Map → New Tileset...** (or press `Ctrl+T`)
2. Choose **Based on Tileset Image**
3. Configure tileset:
   - **Name**: "terrain"
   - **Type**: Based on Tileset Image
   - **Image**: Browse to a tileset image (e.g., `Resources/Tilesets/terrain_tiles.png`)
   - **Tile width**: 32 pixels
   - **Tile height**: 32 pixels
   - **Margin**: 0 pixels (space around edge)
   - **Spacing**: 0 pixels (space between tiles)
4. Click **Save As** and save to `Resources/Tilesets/terrain.tsj`

**Screenshot Placeholder**: `[Screenshot: New Tileset dialog with settings]`

### Step 3: Create a Ground Layer

1. In the **Layers** panel (usually on the right), you'll see one default layer
2. Select it, then right-click and choose **Layer Properties**
3. Rename it to "Ground"
4. Set the following properties:
   - **Visible**: Checked
   - **Locked**: Unchecked
   - **Opacity**: 1.0

### Step 4: Paint Terrain

1. Select the "Ground" layer in the Layers panel
2. In the **Tilesets** panel (usually bottom-right), select your tileset
3. Select the **Stamp Brush** tool (hotkey: `B`)
4. Click tiles in the tileset to select them
   - Single click selects one tile
   - Click and drag to select a rectangle of tiles
5. Paint on the map by clicking or dragging
6. Use these tools for efficient painting:
   - **Fill tool** (`F`): Fill an area with selected tile
   - **Terrain brush** (`T`): Auto-tiling with terrain sets
   - **Eraser** (`E`): Remove tiles

**Screenshot Placeholder**: `[Screenshot: Painted ground layer with grass and dirt tiles]`

### Step 5: Add More Tile Layers

Create additional layers for depth:

1. Click **Layer → Add Tile Layer** (or `Ctrl+Shift+N`)
2. Name it "Decorations"
3. Add props like rocks, flowers, bushes
4. Create another layer named "Walls" for collision geometry
5. Paint walls, trees, buildings on this layer

**Layer Organization Tips:**
- Use descriptive names: "Ground", "Decorations", "Trees", "Walls"
- Order matters: layers render bottom-to-top in the list
- Lock layers you're not editing to avoid accidental changes

### Step 6: Configure Layer Z-Order

Olympe Engine uses z-order for depth sorting. Configure each layer's render order:

1. Right-click a layer → **Layer Properties**
2. Add a custom property:
   - **Name**: `zOrder`
   - **Type**: int
   - **Value**: Set based on depth (see table below)

| Layer Purpose | zOrder Value | Example |
|---------------|--------------|---------|
| Far background | -20000 to -10000 | Distant mountains |
| Near background | -10000 to 0 | Sky, clouds |
| Ground/terrain | 0 to 10000 | Floor tiles |
| Objects/decorations | 10000 to 20000 | Props, items |
| Characters | 20000 to 30000 | Player, NPCs |
| Flying objects | 30000 to 40000 | Birds, projectiles |
| Effects | 40000 to 50000 | Particles, VFX |
| UI elements | 50000+ | HUD, overlays |

**Example Configuration:**
- Ground layer: `zOrder = 0`
- Decorations layer: `zOrder = 10000`
- Walls layer: `zOrder = 15000`

---

## Part 3: Adding Objects and Entities

Object layers contain game entities like players, enemies, NPCs, items, and trigger zones.

### Step 1: Create an Object Layer

1. Click **Layer → Add Object Layer** (or `Ctrl+Shift+O`)
2. Name it "Entities"
3. This layer will contain your game objects

### Step 2: Place an Object

1. Select the "Entities" layer
2. Choose the **Insert Rectangle** tool (`R`) or **Insert Point** tool (`I`)
3. Click on the map to place an object
4. Select the object to view its properties in the **Properties** panel

**Screenshot Placeholder**: `[Screenshot: Object layer with placed rectangle objects]`

### Step 3: Configure Object Properties

Each object needs a **Type** field that tells Olympe which prefab to instantiate:

1. Select the object
2. In the **Properties** panel:
   - Set **Type** to the prefab name (e.g., "Player", "Enemy", "Chest")
   - The **Name** field is optional but useful for identification

**Screenshot Placeholder**: `[Screenshot: Object properties panel with Type field]`

### Step 4: Add Component-Scoped Custom Properties

Component-scoped properties allow you to override specific component values per instance:

1. Select the object
2. In the **Properties** panel, click the **+** button
3. Add custom properties using dot notation: `ComponentName.propertyName`

**Example: Enemy Configuration**

Add these custom properties to an enemy object:

| Property Name | Type | Value | Description |
|---------------|------|-------|-------------|
| `Transform.x` | float | 400.0 | X position override |
| `Transform.y` | float | 300.0 | Y position override |
| `Health.maxHealth` | int | 150 | Enemy has 150 HP |
| `Health.currentHealth` | int | 150 | Starts at full health |
| `Physics.speed` | float | 80.0 | Movement speed |
| `AIBlackboard.detectionRadius` | float | 200.0 | Detection range |
| `AIBlackboard.attackRadius` | float | 50.0 | Attack range |

**Screenshot Placeholder**: `[Screenshot: Custom properties with component prefixes]`

### Component Naming Reference

Common component names you can use:

- **Transform**: `x`, `y`, `z`, `rotation`, `scale`
- **Position**: `position.x`, `position.y`, `position.z`
- **BoundingBox**: `boundingBox.x`, `boundingBox.y`, `boundingBox.w`, `boundingBox.h`
- **Health**: `maxHealth`, `currentHealth`
- **Physics**: `mass`, `speed`, `friction`
- **PhysicsBody**: `speed`, `mass`
- **Movement**: `direction.x`, `direction.y`, `velocity.x`, `velocity.y`
- **VisualSprite**: `spritePath`, `srcRect.x`, `srcRect.y`, `srcRect.w`, `srcRect.h`
- **AIBlackboard**: `detectionRadius`, `attackRadius`, `patrolPoints`
- **AIBehaviorTree**: `behaviorTreePath`

### Step 5: Place Multiple Objects

Create a diverse level by adding:

1. **Player spawn point**:
   - Type: "Player"
   - Add property: `Identity.tag = "Player"`

2. **Enemies**:
   - Type: "Enemy"
   - Vary `Health.maxHealth` and `Physics.speed` for variety

3. **NPCs**:
   - Type: "NPC"
   - Add property: `AIBehaviorTree.behaviorTreePath = "Blueprints/AI/npc_wander.json"`

4. **Items and collectibles**:
   - Type: "HealthPotion", "Coin", "Key"

5. **Trigger zones**:
   - Type: "TriggerZone"
   - Add properties: `TriggerZone.radius = 100.0`

**Screenshot Placeholder**: `[Screenshot: Map with multiple objects placed]`

---

## Part 4: Creating an Isometric Map

Isometric maps provide a 3D-like perspective (like strategy games or classic RPGs).

### Step 1: Create Isometric Map

1. Click **File → New → New Map...**
2. Configure map settings:
   - **Orientation**: Isometric
   - **Map size**: 30 × 30 tiles
   - **Tile size**: 64 pixels × 32 pixels (typical isometric dimensions)
   - **Stagger axis**: Y
   - **Stagger index**: Odd

3. Save as `Resources/Maps/my_isometric_level.tmj`

### Step 2: Add Isometric Tileset

1. Create a new tileset with isometric tiles
2. **Important**: Isometric tilesets often need tile offsets
3. In the tileset properties:
   - **Tile offset X**: 0 (or adjust based on your tileset)
   - **Tile offset Y**: 26 (typical for isometric tiles with height)

### Step 3: Paint Isometric Terrain

1. Select tiles from your isometric tileset
2. Paint the ground plane first (flat tiles)
3. Add elevated tiles (walls, cliffs, buildings)
4. Use layers for height separation:
   - "Ground" (zOrder = 0)
   - "Elevated" (zOrder = 10000)
   - "Tall Objects" (zOrder = 20000)

**Screenshot Placeholder**: `[Screenshot: Isometric map with terrain and elevation]`

### Step 4: Isometric Object Placement

Object placement in isometric maps requires special consideration:

1. Create object layer as before
2. Place objects at the **base** of where they should appear
3. The engine will automatically sort based on Y position
4. For tall objects (trees, buildings), add property:
   - `VisualSprite.hotSpot.y` = tile height for proper anchoring

---

## Part 5: Advanced Layer Features

### Parallax Scrolling

Create depth with parallax layers:

1. Create an **Image Layer** for backgrounds:
   - **Layer → Add Image Layer**
   - Name it "Sky"
   - Set the image (e.g., `Resources/Backgrounds/sky.png`)

2. Add layer properties:
   - **Name**: `parallaxx`
   - **Type**: float
   - **Value**: 0.5 (moves at half camera speed)

   - **Name**: `parallaxy`
   - **Type**: float
   - **Value**: 0.5

**Parallax Values Guide:**
- `1.0` = moves with camera (no parallax)
- `0.5` = moves at half speed (appears distant)
- `0.0` = fixed in place (skybox)
- `2.0` = moves faster than camera (foreground effect)

### Layer Groups

Organize complex maps with layer groups:

1. **Layer → Add Group Layer**
2. Name it (e.g., "Environment")
3. Drag layers into the group to organize them
4. Collapse groups to reduce clutter

### Infinite Maps

For open-world or procedural levels:

1. Enable **Infinite** when creating the map
2. Tiles are stored in chunks
3. Olympe Engine supports infinite maps with chunk-based rendering

---

## Part 6: Loading Your Map in Olympe Engine

### Step 1: Verify File Location

Ensure your map file is in the correct directory:

```
YourGame/
├── Resources/
│   ├── Maps/
│   │   ├── my_first_level.tmj
│   │   └── my_isometric_level.tmj
│   └── Tilesets/
│       ├── terrain.tsj
│       └── terrain_tiles.png
```

### Step 2: Load Map in Code

Add this code to your game initialization:

```cpp
#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "World.h"

// In your initialization function:
void LoadLevel()
{
    using namespace Olympe::Tiled;
    
    // Create loader
    TiledLevelLoader loader;
    TiledMap tiledMap;
    
    // Load TMJ file
    std::string mapPath = "Resources/Maps/my_first_level.tmj";
    
    if (!loader.LoadFromFile(mapPath, tiledMap)) {
        SYSTEM_LOG << "ERROR: Failed to load map: " 
                   << loader.GetLastError() << std::endl;
        return;
    }
    
    SYSTEM_LOG << "Map loaded successfully!" << std::endl;
    SYSTEM_LOG << "  Size: " << tiledMap.width << "x" << tiledMap.height 
               << " tiles" << std::endl;
    SYSTEM_LOG << "  Layers: " << tiledMap.layers.size() << std::endl;
    
    // Convert to Olympe format
    TiledToOlympe converter;
    Olympe::Editor::LevelDefinition levelDef;
    
    if (!converter.Convert(tiledMap, levelDef)) {
        SYSTEM_LOG << "ERROR: Failed to convert map" << std::endl;
        return;
    }
    
    // Load into world
    World& world = World::GetInstance();
    world.LoadLevel(levelDef);
    
    SYSTEM_LOG << "Level loaded with " << levelDef.entities.size() 
               << " entities" << std::endl;
}
```

### Step 3: Verify Map Loading

Run your game and check the console for output:

```
Map loaded successfully!
  Size: 30x20 tiles
  Layers: 4
Level loaded with 15 entities
```

### Step 4: Test Object Properties

Verify component-scoped properties were applied:

```cpp
// Example: Check if enemy health was set correctly
EntityID enemy = world.FindEntityByTag("Enemy");
if (enemy != INVALID_ENTITY_ID) {
    auto* health = world.GetComponent<Health_data>(enemy);
    if (health) {
        SYSTEM_LOG << "Enemy health: " << health->currentHealth 
                   << "/" << health->maxHealth << std::endl;
        // Should print: "Enemy health: 150/150"
    }
}
```

---

## Part 7: Common Issues and Troubleshooting

### Map Not Loading

**Problem**: Map file not found

**Solution**:
- Verify file path is correct
- Check working directory of your executable
- Use absolute path for testing: `"C:/MyGame/Resources/Maps/my_first_level.tmj"`

### Tiles Not Rendering

**Problem**: Tiles appear as empty squares

**Solution**:
- Verify tileset image path is correct in TSJ file
- Check image file exists in `Resources/Tilesets/`
- Ensure tileset is saved as external TSJ/TSX file
- Verify image format is supported (PNG, JPG)

### Objects Not Spawning

**Problem**: Objects in Tiled don't appear in game

**Solution**:
- Check object **Type** field is set to a valid prefab name
- Verify prefab exists: `Blueprints/EntityPrefab/[PrefabName].json`
- Check console for "Prefab not found" errors
- Ensure object is on a visible, unlocked layer

### Component Properties Not Applying

**Problem**: Custom properties from Tiled are ignored

**Solution**:
- Use dot notation: `ComponentName.propertyName`
- Check spelling matches component name exactly (case-sensitive)
- Verify property type matches expected type (float vs int)
- Check console for property override warnings

### Z-Order Issues

**Problem**: Objects render in wrong order

**Solution**:
- Add `zOrder` property to tile layers
- For object layers, ensure entities have `Position_data` component
- Objects sort by Y position automatically in isometric maps
- Higher zOrder values render on top

### Isometric Tiles Offset Wrong

**Problem**: Isometric tiles don't align properly

**Solution**:
- Check tileset **tile offset** values in TSJ file
- Typical isometric offset: `tileoffsetY = 26` for 64×32 tiles
- Adjust in Tiled: Tileset properties → Tile Offset
- Save and reload map

---

## Best Practices

### Map Organization

1. **Use descriptive layer names**: "Ground_Grass", "Decorations_Trees", "Entities_Enemies"
2. **Group related layers**: Put all terrain layers in a "Terrain" group
3. **Lock layers** you're not editing to prevent accidents
4. **Use colors**: Right-click layer → **Properties** → set a tint color for visibility

### Object Naming

1. **Give unique names** to important objects (e.g., "BossEnemy", "ExitDoor")
2. **Use tags** for grouping (add custom property `tag = "Collectible"`)
3. **Document complex properties** in object name or custom `notes` property

### Performance Optimization

1. **Merge static layers**: Combine decorative layers that don't change
2. **Use object layers sparingly**: Too many objects can slow down loading
3. **Limit parallax layers**: Each adds rendering overhead
4. **Chunk large maps**: Use infinite maps with chunks for huge worlds

### Version Control

1. **Always use TMJ/TSJ format** (JSON) instead of TMX/TSX (XML)
2. **Commit tilesets separately**: TSJ files should be in version control
3. **Don't commit embedded tilesets**: Always use external tileset files
4. **Use relative paths**: Makes maps portable across team members

### Workflow Tips

1. **Test early and often**: Load map in engine every few changes
2. **Start simple**: Create basic layout first, add details later
3. **Reuse tilesets**: Create a library of reusable tilesets
4. **Document custom properties**: Keep a reference of your component schemas

---

## Next Steps

Now that you can create and load Tiled maps, explore these advanced topics:

### Intermediate:
- [**Object Properties Guide**](../user-guide/tiled-editor/object-properties.md) - Deep dive into custom properties
- [**Layer Properties**](../user-guide/tiled-editor/layer-properties.md) - Advanced layer configuration
- [**Creating Entity Prefabs**](./creating-entities.md) - Build the prefabs your maps reference

### Advanced:
- [**Behavior Trees for NPCs**](./behavior-tree-basics.md) - Add AI to your level entities
- [**Collision and Physics**](../user-guide/physics/collision-detection.md) - Set up collision layers
- [**Animation System**](../user-guide/animation/sprite-animation.md) - Animate your map objects
- [**Dynamic Lighting**](../user-guide/rendering/lighting.md) - Add lighting to your levels

### Resources:
- [**Tiled Best Practices**](../user-guide/tiled-editor/best-practices.md) - Professional map design tips
- [**Performance Optimization**](../technical-reference/optimization/level-loading.md) - Optimize large maps
- [**Tiled Documentation**](https://doc.mapeditor.org/) - Official Tiled reference

---

## Summary

In this tutorial, you learned:

✅ How to install and configure Tiled Map Editor  
✅ Creating orthogonal and isometric maps  
✅ Setting up tilesets and painting terrain  
✅ Adding object layers and placing entities  
✅ Configuring layer properties for z-ordering and parallax  
✅ Using component-scoped properties for entity customization  
✅ Loading and testing maps in Olympe Engine  
✅ Troubleshooting common issues  

You're now ready to create rich, complex levels for your Olympe Engine games!

---

## Additional Resources

- **Example Maps**: `Examples/TiledLoader/` - Sample map files
- **Example Code**: `Examples/TiledLoader/example_load_tiled_map.cpp`
- **Component Reference**: `website/docs/api-reference/components.md`
- **Tiled Website**: [https://www.mapeditor.org/](https://www.mapeditor.org/)
- **Olympe Discord**: Get help from the community

**Happy Mapping! 🗺️**
