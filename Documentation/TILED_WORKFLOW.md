# Tiled to Olympe Engine Workflow

## Complete Workflow Overview

This guide walks you through the entire process of creating a game level in Tiled and loading it in Olympe Engine.

```
[Design Level] → [Create in Tiled] → [Export .tmj] → [Load in Olympe] → [Test & Iterate]
```

**Time estimate:** 30-60 minutes for first level, 10-20 minutes for subsequent levels

## Prerequisites

### Software Required

1. **Tiled MapEditor** (1.9+)
   - Download: https://www.mapeditor.org/
   - Free and open-source

2. **Olympe Engine** (built from source)
   - Repository: `/home/runner/work/Olympe-Engine/Olympe-Engine`

### Assets Required

1. **Tileset Images** (.png)
   - Place in `Resources/Tilesets/`
   - Example: `ground_tiles.png`, `wall_tiles.png`

2. **Entity Prefabs** (.json)
   - Located in `Blueprints/EntityPrefab/`
   - Examples: `player_entity.json`, `npc_entity.json`, `enemy.json`

3. **Object Types Template** (optional but recommended)
   - Use `Templates/Tiled/objecttypes.xml`
   - Import into Tiled for pre-configured object types

## Step-by-Step Workflow

### Phase 1: Initial Setup in Tiled

#### 1.1 Create New Map

**File → New → New Map...**

**Settings:**
- **Orientation:** Orthogonal (or Isometric if desired)
- **Tile layer format:** CSV or Base64 (uncompressed)
- **Tile render order:** Right Down
- **Map size:** Fixed (e.g., 20×15 tiles) or Infinite
- **Tile size:** 32×32 pixels (or match your tileset)

**Example Configuration:**
```
Orientation: Orthogonal
Width: 20 tiles
Height: 15 tiles
Tile Width: 32px
Tile Height: 32px
Infinite: No
```

**Save as:** `Resources/Maps/my_level.tmj`

#### 1.2 Import Object Types

**View → Object Types Editor**

- Click Import (folder icon)
- Select `Templates/Tiled/objecttypes.xml`
- Click Apply

You now have pre-configured object types with proper colors and default properties.

#### 1.3 Add Tilesets

**Map → Add External Tileset...**

- Navigate to `Resources/Tilesets/`
- Select tileset (.tsx or .tsj file)
- Click Open

**Or create embedded tileset:**

1. **Map → New Tileset...**
2. Set name (e.g., "ground")
3. Browse to tileset image
4. Set tile width/height (must match map settings)
5. Click Save As... → save as .tsj in `Resources/Tilesets/`

**Best Practice:** Use external tilesets for reusability across maps.

### Phase 2: Build the Level

#### 2.1 Create Tile Layers

**Layer → New Layer → Tile Layer**

**Common Layer Structure:**
1. **Ground** - Base terrain (grass, dirt, floor)
2. **Ground Details** - Decoration (flowers, cracks)
3. **Walls** - Impassable obstacles
4. **Roof** - Overhead decorations (optional, for parallax)

**For Each Layer:**
1. Select layer
2. Choose tileset from Tilesets panel
3. Use Stamp Brush (B) to paint tiles
4. Use Fill Tool (F) for large areas
5. Use Eraser (E) to remove tiles

**Tips:**
- Keep layers organized (use meaningful names)
- Use layer opacity to see underlying layers while editing
- Lock layers you're not editing to prevent accidental changes

#### 2.2 Add Collision Layer

**Option A: Dedicated Tile Layer**

1. Create new tile layer named "Collision"
2. Paint collision tiles (use distinct tileset or same tiles)
3. Set layer custom property: `collision` (bool) = true

**Option B: Object Layer with Collision Objects (Recommended)**

1. Create object layer named "Collision"
2. Use Rectangle tool (R) to draw collision zones
3. Set each object's **Type** to `"collision"`
4. No other properties needed

**Example:**
```
Layer: "Collision" (object layer)
  └─ Rectangle Object
      ├─ Type: "collision"
      ├─ X: 64, Y: 64
      ├─ Width: 128, Height: 32
```

Olympe automatically creates `CollisionZone_data` components from these objects.

### Phase 3: Place Objects and Entities

#### 3.1 Create Object Layer

**Layer → New Layer → Object Layer**

Name: "Objects" or "Entities"

#### 3.2 Place Player Spawn

1. Select **Point tool** (I) or Insert Point (Insert key)
2. Click where player should spawn
3. Select the point object
4. **Properties Panel:**
   - Name: `"player_spawn"`
   - Type: `"player"`
5. Optional: Add custom property `health` (int) = 100

#### 3.3 Place NPCs

1. Select **Point tool**
2. Click where NPC should spawn
3. **Properties:**
   - Name: `"village_elder"`
   - Type: `"npc"`
   - Custom Property: `friendlyName` (string) = "Elder Thomas"
   - Custom Property: `dialogueFile` (file) = "Dialogue/elder_intro.json"

#### 3.4 Place Enemies

1. Select **Point tool**
2. Click where enemy should spawn
3. **Properties:**
   - Name: `"goblin_1"`
   - Type: `"enemy"`
   - Custom Property: `health` (int) = 50
   - Custom Property: `damage` (int) = 10

#### 3.5 Create Patrol Path (for Guards)

1. Select **Polyline tool** (L)
2. Click to place waypoints (at least 2)
3. Double-click to finish
4. **Properties:**
   - Name: `"patrol_path_1"` (important!)
   - Type: `"way"`
5. Place guard NPC with property: `patrolPathName` (string) = `"patrol_path_1"`

**Example:**
```
Layer: "Objects"
  ├─ Polyline Object
  │   ├─ Name: "patrol_path_1"
  │   ├─ Type: "way"
  │   └─ Points: [(100,100), (200,100), (200,200), (100,200)]
  └─ Point Object
      ├─ Name: "guard_1"
      ├─ Type: "guard"
      └─ Property: patrolPathName = "patrol_path_1"
```

The guard will patrol along the polyline path automatically.

#### 3.6 Add Collectibles

**Keys:**
```
Type: "key"
Property: keyId (string) = "dungeon_key_red"
```

**Treasures:**
```
Type: "treasure"
Property: value (int) = 100
```

**Items:**
```
Type: "collectible"
Property: itemId (string) = "health_potion"
```

### Phase 4: Advanced Features

#### 4.1 Add Parallax Layers (Image Layers)

**For Background:**

1. **Layer → New Layer → Image Layer**
2. Name: "Background Mountains"
3. **Properties Panel → Custom Properties:**
   - `parallaxx` (float) = 0.3
   - `parallaxy` (float) = 0.3
   - `repeatx` (bool) = true (optional, for repeating backgrounds)
   - `repeaty` (bool) = false
4. Set image source (browse to .png file)

**Parallax Factors:**
- `< 1.0` = Moves slower than camera (background)
- `= 1.0` = Moves with camera (foreground)
- `> 1.0` = Moves faster than camera (extreme foreground)

**Example Multi-Layer Parallax:**
```
Layer: "Sky" (image)
  ├─ parallaxx: 0.1, parallaxy: 0.1
Layer: "Mountains" (image)
  ├─ parallaxx: 0.3, parallaxy: 0.3
Layer: "Hills" (image)
  ├─ parallaxx: 0.6, parallaxy: 0.6
Layer: "Ground" (tile)
  └─ (no parallax, moves 1:1 with camera)
```

#### 4.2 Define Sectors

**For Streaming/LOD:**

1. Create object layer "Sectors"
2. Use **Polygon tool** (P) or **Rectangle tool** (R)
3. Draw sector boundaries
4. **Properties:**
   - Name: `"forest_sector_01"`
   - Type: `"sector"`
   - Custom Properties:
     - `biome` (string) = "forest"
     - `music` (file) = "Audio/forest_theme.ogg"
     - `ambientColor` (color) = #80C080

See [TILED_SECTORS.md](TILED_SECTORS.md) for details.

#### 4.3 Add Portals/Exits

**Doorway to Next Level:**
```
Type: "portal"
Properties:
  - targetLevel (string) = "Resources/Maps/level2.tmj"
  - targetX (float) = 100.0
  - targetY (float) = 100.0
```

**Locked Door:**
```
Type: "door"
Properties:
  - requiresKey (string) = "dungeon_key_red"
  - targetLevel (string) = "Resources/Maps/dungeon_inner.tmj"
```

### Phase 5: Map Properties and Metadata

#### 5.1 Set Map Custom Properties

**Map → Map Properties → Custom Properties**

**Useful Properties:**
- `ambientLight` (color) = #808080 (for dark dungeons)
- `gravity` (float) = 9.81 (if physics needed)
- `weather` (string) = "rain" (for weather system)
- `timeOfDay` (string) = "night"

#### 5.2 Set Background Color

**Map → Map Properties**

- **Background Color:** Choose color (displays when no tiles present)

### Phase 6: Testing and Export

#### 6.1 Validate Map

**Before exporting, check:**
- ✅ Player spawn point exists (type: "player")
- ✅ All objects have correct types set
- ✅ Polyline patrol paths have names
- ✅ Guards reference correct patrol path names
- ✅ All custom properties are correct data types
- ✅ Collision objects cover walls/obstacles
- ✅ Tileset images are in correct paths
- ✅ External tilesets are saved

#### 6.2 Save Map

**File → Save As...**

- Location: `Resources/Maps/my_level.tmj`
- Format: JSON (.tmj) - **Required**

**Important:** Olympe only supports .tmj (JSON) format, not .tmx (XML).

#### 6.3 Test in Olympe

1. **Run Olympe Engine**
2. **Press F3** (opens Tiled Loader menu)
3. **Navigate** to your map file
4. **Click "Load"**
5. **Test gameplay:**
   - Player spawns correctly
   - Movement works
   - Collision stops player at walls
   - NPCs appear at correct positions
   - Patrol paths work
   - Collectibles are interactable

#### 6.4 Iterate

**Common Adjustments:**
- Move spawn points
- Adjust collision zones
- Add/remove enemies
- Tweak patrol paths
- Change parallax factors
- Refine tile placement

**Workflow:**
1. Edit in Tiled (Ctrl+S to save)
2. Switch to Olympe
3. Press F3 → Reload map
4. Test changes
5. Repeat

No engine restart needed!

## Object Types Reference

Complete list of supported object types in Olympe:

| Type | Purpose | Required Properties | Optional Properties |
|------|---------|---------------------|---------------------|
| `player` | Player spawn point | None | `health`, `speed` |
| `npc` | Non-hostile NPC | None | `friendlyName`, `dialogueFile` |
| `guard` | Patrolling NPC | `patrolPathName` | `speed`, `alertRadius` |
| `enemy` | Hostile entity | None | `health`, `damage`, `speed` |
| `zombie` | Zombie enemy | None | `health`, `speed` |
| `collision` | Collision zone | None | None (auto-handled) |
| `way` | Patrol path | `name` | None |
| `sector` | Level sector | None | `biome`, `music`, etc. |
| `key` | Key item | `keyId` | None |
| `treasure` | Treasure chest | None | `value` |
| `collectible` | Generic item | `itemId` | None |
| `exit` | Level exit | `targetLevel` | None |
| `door` | Locked door | `targetLevel` | `requiresKey` |
| `portal` | Teleporter | `targetLevel`, `targetX`, `targetY` | None |
| `waypoint` | Generic marker | None | Any custom props |

See [TILED_OBJECT_TYPES.md](TILED_OBJECT_TYPES.md) for detailed reference.

## Custom Properties Guide

### Property Types in Tiled

| Tiled Type | C++ Type | Example Value |
|------------|----------|---------------|
| string | std::string | `"hello"` |
| int | int | `42` |
| float | float | `3.14` |
| bool | bool | `true` |
| color | std::string | `"#FF0000"` |
| file | std::string | `"path/to/file.txt"` |

### Adding Custom Properties to Objects

1. Select object
2. **Properties Panel → Custom Properties**
3. Click **+** button
4. Choose property type
5. Enter name and value

### Property Override System

Custom properties in Tiled **override** default prefab values.

**Example:**

**Prefab:** `enemy.json`
```json
{
  "components": {
    "Health_data": { "maxHealth": 100, "currentHealth": 100 }
  }
}
```

**Tiled Object:**
```
Type: "enemy"
Property: health (int) = 50
```

**Result:** Enemy spawns with 50 HP instead of 100.

## Tileset Setup Best Practices

### External Tilesets (.tsx/.tsj)

**Advantages:**
- Reusable across maps
- Centralized tile properties
- Easier version control

**How to Create:**
1. **File → New → New Tileset...**
2. Set name (e.g., "dungeon_tiles")
3. Browse to image
4. Set tile dimensions
5. **File → Save As...** → `Resources/Tilesets/dungeon_tiles.tsj`

**How to Use:**
1. Open map
2. **Map → Add External Tileset...**
3. Select .tsj file

### Tileset Organization

**Recommended Structure:**
```
Resources/Tilesets/
  ├── ground/
  │   ├── grass.png
  │   ├── grass.tsj
  │   ├── dirt.png
  │   └── dirt.tsj
  ├── walls/
  │   ├── stone.png
  │   └── stone.tsj
  └── decorations/
      ├── flowers.png
      └── flowers.tsj
```

### Tile Animations (Advanced)

1. Select tile in tileset
2. **View → Tile Animation Editor**
3. Add frames
4. Set duration per frame
5. Olympe loads animated tiles automatically

## Common Pitfalls and Solutions

### ❌ Map doesn't load

**Causes:**
- Wrong file format (.tmx instead of .tmj)
- Invalid JSON syntax
- Missing external tilesets
- Incorrect file path

**Solutions:**
- Save as .tmj (JSON)
- Validate JSON with online tool
- Ensure tilesets exist in correct paths
- Use relative paths from executable

### ❌ Entities don't spawn

**Causes:**
- Object type not in `tiled_prefab_mapping.json`
- Prefab file doesn't exist
- Object has no type set
- Object layer is invisible

**Solutions:**
- Add mapping in `Config/tiled_prefab_mapping.json`
- Verify prefab exists in `Blueprints/EntityPrefab/`
- Set object type in Tiled
- Make layer visible in Tiled

### ❌ Patrol paths don't work

**Causes:**
- Polyline has no name
- Guard's `patrolPathName` doesn't match polyline name
- Polyline has only 1 point
- Wrong object type (not "way")

**Solutions:**
- Name polyline (e.g., "patrol_path_1")
- Set guard property: `patrolPathName` = exact polyline name
- Add at least 2 points to polyline
- Set polyline type to "way"

### ❌ Tiles render incorrectly

**Causes:**
- Wrong tile size in map settings
- Tileset image path incorrect
- Flipped tiles not supported (they are!)
- Z-order issues

**Solutions:**
- Match map tile size to tileset tile size
- Use relative paths for tileset images
- Flip flags work automatically
- Check layer order (bottom = background, top = foreground)

### ❌ Collision doesn't work

**Causes:**
- Object type not set to "collision"
- Collision objects on wrong layer
- Physics system not initialized
- Entity missing collider component

**Solutions:**
- Set type to "collision" for all collision objects
- Create dedicated "Collision" object layer
- Verify World has physics enabled
- Add `CollisionZone_data` component to entity prefabs

## Performance Tips

### For Large Maps

1. **Use Infinite Maps**: Enable chunking for streaming
2. **Limit Layer Count**: Max 10-15 layers
3. **Optimize Tilesets**: Use texture atlases
4. **Use Sectors**: Define LOD regions
5. **Reduce Object Count**: Combine similar objects

### For Many Entities

1. **Use Object Pooling**: Reuse entity instances
2. **Cull Off-Screen**: Don't render distant entities
3. **Simplify AI**: Reduce update frequency for distant NPCs
4. **Batch Rendering**: Group entities by sprite

## Keyboard Shortcuts in Tiled

| Key | Action |
|-----|--------|
| `B` | Stamp Brush (paint tiles) |
| `F` | Fill Tool (bucket fill) |
| `E` | Eraser |
| `R` | Rectangle Object |
| `P` | Polygon Object |
| `L` | Polyline Object |
| `I` | Insert Point |
| `S` | Select Tool |
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `Ctrl+D` | Duplicate Layer/Object |
| `Ctrl+S` | Save |
| `Space+Drag` | Pan View |

## Next Steps

Once your workflow is comfortable:

1. **Create Map Templates**: Save frequently used maps as templates
2. **Build Tileset Library**: Organize reusable tilesets
3. **Script Level Generation**: Use Tiled's scripting API for procedural content
4. **Integrate with CI/CD**: Auto-validate maps on commit
5. **Explore Advanced Features**: Terrain brushes, wang tiles, automapping

## See Also

- [TILED_INTEGRATION.md](TILED_INTEGRATION.md) - API reference
- [TILED_ISOMETRIC.md](TILED_ISOMETRIC.md) - Isometric workflow
- [TILED_SECTORS.md](TILED_SECTORS.md) - Sector creation guide
- [TILED_OBJECT_TYPES.md](TILED_OBJECT_TYPES.md) - Complete object reference
- [Templates/Tiled/README.md](../Templates/Tiled/README.md) - Example maps and templates
