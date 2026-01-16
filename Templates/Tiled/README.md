# Tiled Templates for Olympe Engine

This directory contains ready-to-use templates for creating game levels with Tiled MapEditor.

## Files

### objecttypes.xml

Pre-configured object types for Olympe Engine with proper colors, default properties, and types.

**How to Import:**

1. Open Tiled MapEditor
2. **View → Object Types Editor**
3. Click the **Import** button (folder icon)
4. Navigate to this file and select it
5. Click **Apply** or **OK**

**What You Get:**

All Olympe object types ready to use:
- `player` (green) - Player spawn point
- `npc` (blue) - Non-hostile NPCs
- `guard` (light blue) - Patrolling guards
- `enemy` (red) - Hostile enemies
- `zombie` (dark red) - Zombie enemies
- `key` (yellow) - Collectible keys
- `treasure` (gold) - Treasure chests
- `collectible` (orange) - Generic items
- `exit` (green) - Level exits
- `door` (brown) - Doors (lockable)
- `portal` (magenta) - Teleporters
- `waypoint` (cyan) - Waypoints
- `collision` (red) - Collision zones
- `way` (magenta) - Patrol paths
- `sector` (yellow) - Level sectors

**Benefits:**
- Color-coded for easy visualization
- Default properties pre-filled
- Type-safe (correct data types)
- Consistent across team

### example_orthogonal.tmj

Example orthogonal (top-down) map showing basic level structure.

**Features:**
- 20×15 tiles (32×32 pixel tiles)
- Ground layer with border
- Collision walls (rectangles)
- Player spawn point
- NPC, enemy, and treasure examples

**How to Use:**

1. Open in Tiled: **File → Open** → select this file
2. Add your own tilesets: **Map → Add External Tileset...**
3. Edit layers, add objects
4. **File → Save As...** with new name
5. Load in Olympe with **F3** menu

**Learn From:**
- Layer structure (Ground, Collision, Objects)
- Object placement and properties
- Collision zone setup

### example_isometric.tmj

Example isometric map (58×27 tiles, 2:1 aspect ratio).

**Features:**
- 10×10 tiles (58×27 pixel tiles)
- Isometric orientation
- Diamond rendering order
- Player and NPC examples

**How to Use:**

Same as orthogonal example, but note:
- Tiles render in diamond pattern
- Coordinate conversion handled automatically
- See [Documentation/TILED_ISOMETRIC.md](../../Documentation/TILED_ISOMETRIC.md) for details

## Quick Start

### 1. Import Object Types

```
Tiled → View → Object Types Editor → Import → objecttypes.xml
```

### 2. Create New Map

**Option A: From Scratch**

```
File → New → New Map...
Settings:
  - Orientation: Orthogonal (or Isometric)
  - Map size: 20×15 (or as needed)
  - Tile size: 32×32 (or match your tileset)
Save as: Resources/Maps/my_level.tmj
```

**Option B: From Example**

```
File → Open → example_orthogonal.tmj
File → Save As... → Resources/Maps/my_level.tmj
```

### 3. Add Tilesets

```
Map → Add External Tileset...
Select: Resources/Tilesets/your_tileset.tsj
```

Or create embedded:

```
Map → New Tileset...
Browse to image, set tile size
Save As... → Resources/Tilesets/your_tileset.tsj
```

### 4. Build Your Level

**Layers:**
1. Ground - Base terrain
2. Walls - Obstacles
3. Collision - Collision zones (object layer)
4. Objects - Entities, NPCs, items

**Tools:**
- `B` - Stamp Brush (paint tiles)
- `F` - Fill Tool
- `R` - Rectangle Object
- `P` - Polygon Object
- `L` - Polyline Object
- `I` - Insert Point

### 5. Place Objects

1. Select Object Layer ("Objects")
2. Press `I` (Insert Point) or use toolbar
3. Click to place
4. Set object type (player, npc, enemy, etc.)
5. Customize properties in Properties panel

### 6. Test in Olympe

1. Save map (Ctrl+S)
2. Run Olympe Engine
3. Press **F3** to open Tiled Loader
4. Navigate to your .tmj file
5. Click **Load**

## Object Type Usage

### Player Spawn

```
Type: player
Geometry: Point
Properties:
  - health (int) = 100
  - speed (float) = 100.0
```

Place exactly **one** per level.

### NPC

```
Type: npc
Geometry: Point
Properties:
  - friendlyName (string) = "Villager"
  - dialogueFile (file) = "Dialogue/villager.json"
```

For friendly characters.

### Guard (Patrolling)

```
Type: guard
Geometry: Point
Properties:
  - patrolPathName (string) = "patrol_castle"
  - speed (float) = 60.0
  - alertRadius (float) = 100.0
```

**Requires a patrol path:**

```
Type: way
Geometry: Polyline
Name: "patrol_castle"
Points: [(100,100), (200,100), (200,200), (100,200)]
```

Guard will follow the polyline path in a loop.

### Enemy

```
Type: enemy
Geometry: Point
Properties:
  - health (int) = 50
  - damage (int) = 10
  - speed (float) = 80.0
```

Chases player within `aggroRadius`.

### Collision Zones

```
Type: collision
Geometry: Rectangle (or Polygon)
```

**No properties needed.** Automatically becomes impassable.

**Tips:**
- Use rectangles for walls
- Use polygons for irregular shapes
- Place on dedicated "Collision" object layer

### Patrol Paths

```
Type: way
Geometry: Polyline
Name: "my_path_name"
Properties:
  - looped (bool) = true
```

**Steps:**
1. Press `L` (Polyline tool)
2. Click waypoints (at least 2)
3. Double-click to finish
4. Set Name property (e.g., "patrol_courtyard")
5. Set Type to "way"

Guards reference this by name via `patrolPathName` property.

## Parallax Layers

Add image layers with custom properties for parallax scrolling.

**Example:**

1. **Layer → New Layer → Image Layer**
2. Name: "Background Mountains"
3. Set image source
4. **Add custom properties:**
   - `parallaxx` (float) = 0.3
   - `parallaxy` (float) = 0.3
   - `repeatx` (bool) = true (optional)

**Parallax Factors:**
- `< 1.0` = Background (moves slower than camera)
- `= 1.0` = Normal (moves with camera)
- `> 1.0` = Foreground (moves faster than camera)

## Sectors (LOD/Streaming)

Define level regions with polygon or rectangle objects.

**Example:**

1. Create object layer "Sectors"
2. Press `P` (Polygon tool) or `R` (Rectangle tool)
3. Draw boundary
4. Set Type to "sector"
5. Set Name (e.g., "forest_entrance")
6. **Add custom properties:**
   - `biome` (string) = "forest"
   - `music` (file) = "Audio/forest.ogg"
   - `ambientColor` (color) = #80C080

See [Documentation/TILED_SECTORS.md](../../Documentation/TILED_SECTORS.md) for more.

## Common Pitfalls

### ❌ Objects don't spawn

**Cause:** Object type not set or incorrect

**Fix:** Select object, set Type dropdown in Properties panel

### ❌ Patrol path doesn't work

**Cause:** Polyline has no name, or guard's `patrolPathName` doesn't match

**Fix:**
1. Select polyline, set Name property
2. Select guard, set `patrolPathName` to exact polyline name

### ❌ Collision doesn't work

**Cause:** Object type not set to "collision"

**Fix:** Select collision objects, set Type to "collision"

### ❌ Map won't load in Olympe

**Cause:** Wrong file format (.tmx instead of .tmj)

**Fix:** Save as JSON format (.tmj)

### ❌ Tiles are black

**Cause:** Tileset image path incorrect

**Fix:** Use relative paths from .tmj file, or absolute paths from project root

## Custom Properties

You can add any custom properties to objects beyond the defaults.

**How:**

1. Select object
2. Properties panel → Custom Properties
3. Click **+** button
4. Choose type (string, int, float, bool, color, file)
5. Enter name and value

**Example:**

Add `questId` to an NPC:
```
Property: questId (string) = "quest_village_help"
```

Your game code can read this via the entity's property overrides.

## File Organization

**Recommended Structure:**

```
Resources/
├── Maps/
│   ├── level_1.tmj
│   ├── level_2.tmj
│   └── dungeon.tmj
├── Tilesets/
│   ├── ground.png
│   ├── ground.tsj
│   ├── walls.png
│   └── walls.tsj
└── Sprites/
    └── characters/
```

**Tips:**
- Keep .tmj and .tsj files together with assets
- Use external tilesets (.tsj) for reusability
- Version control .tmj files (JSON is git-friendly)
- Use relative paths for portability

## Advanced Techniques

### Tile Animations

1. Select tile in Tileset Editor
2. **View → Tile Animation Editor**
3. Add frames and set durations
4. Olympe loads animated tiles automatically

### Group Layers

Organize related layers:

1. **Layer → New Layer → Group Layer**
2. Drag layers into group
3. Toggle visibility of entire group

Useful for:
- Background layers (parallax group)
- Foreground overlays
- Debug layers (collision visualization)

### Object Templates

Save frequently used objects as templates:

1. Right-click object
2. **Save As Template...**
3. Save as .tx (Tiled template)
4. Reuse: **Insert Template** from toolbar

### Automapping

Advanced: Use Tiled's automapping rules for procedural decoration.

See: https://doc.mapeditor.org/en/stable/manual/automapping/

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `B` | Stamp Brush |
| `F` | Fill Tool |
| `E` | Eraser |
| `R` | Rectangle Object |
| `P` | Polygon Object |
| `L` | Polyline Object |
| `I` | Insert Point |
| `S` | Select Tool |
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `Ctrl+D` | Duplicate |
| `Space+Drag` | Pan View |

## Resources

**Olympe Documentation:**
- [TILED_INTEGRATION.md](../../Documentation/TILED_INTEGRATION.md) - Integration overview
- [TILED_WORKFLOW.md](../../Documentation/TILED_WORKFLOW.md) - Complete workflow guide
- [TILED_OBJECT_TYPES.md](../../Documentation/TILED_OBJECT_TYPES.md) - Object type reference
- [TILED_ISOMETRIC.md](../../Documentation/TILED_ISOMETRIC.md) - Isometric maps guide
- [TILED_SECTORS.md](../../Documentation/TILED_SECTORS.md) - Sectors and LOD

**Tiled Resources:**
- Official Manual: https://doc.mapeditor.org/
- Tutorials: https://doc.mapeditor.org/en/stable/manual/introduction/
- Forum: https://discourse.mapeditor.org/

**Olympe Config:**
- `Config/tiled_prefab_mapping.json` - Object type → prefab mapping
- `Blueprints/EntityPrefab/` - Entity prefab definitions

## Getting Help

**Issues with templates?**
- Check console logs in Olympe (look for "TiledLevelLoader" messages)
- Verify .tmj file is valid JSON (use online JSON validator)
- Ensure `objecttypes.xml` is imported in Tiled

**Map won't load?**
- Check file paths are correct
- Verify external tilesets exist
- Test with example maps first

**Need more object types?**
1. Create prefab in `Blueprints/EntityPrefab/`
2. Add mapping in `Config/tiled_prefab_mapping.json`
3. Add to `objecttypes.xml`
4. Re-import in Tiled

## Next Steps

1. **Practice**: Create a simple test level with the examples
2. **Customize**: Modify object types for your game
3. **Expand**: Add custom properties and prefabs
4. **Iterate**: Use F3 menu for rapid testing
5. **Share**: Export templates for your team

Happy mapping! 🗺️
