# Tiled MapEditor Integration Guide

## Overview

Olympe Engine provides full integration with [Tiled MapEditor](https://www.mapeditor.org/), allowing you to create levels using Tiled's powerful visual editor and load them directly into your game. The integration supports both orthogonal and isometric maps, with features like parallax layers, object spawning, collision zones, patrol paths, and sector definitions.

## Architecture

### TiledLevelLoader Module

The Tiled integration is implemented in the `Source/TiledLevelLoader/` module with three main components:

```
TiledLevelLoader/
├── include/
│   ├── TiledStructures.h    # Data structures mirroring Tiled JSON format
│   ├── TiledLevelLoader.h   # JSON parser for .tmj/.tsj files
│   ├── TiledToOlympe.h      # Converter to Olympe LevelDefinition
│   └── TiledJsonHelper.h    # JSON utilities (nlohmann::json wrapper)
└── src/
    ├── TiledLevelLoader.cpp # Parser implementation
    └── TiledToOlympe.cpp    # Conversion logic
```

**Component Responsibilities:**

1. **TiledLevelLoader**: Parses Tiled .tmj (JSON map) and .tsj (JSON tileset) files into `TiledMap` structures
2. **TiledToOlympe**: Converts `TiledMap` to Olympe's `LevelDefinition` format, handling entity spawning, collision generation, and metadata extraction
3. **TiledStructures**: Defines C++ structures matching Tiled's JSON schema (maps, layers, objects, tilesets, properties)

### Supported Features

- ✅ **Map Orientations**: Orthogonal, Isometric
- ✅ **Map Types**: Finite, Infinite (chunked)
- ✅ **Layer Types**: Tile layers, Object layers, Image layers, Group layers
- ✅ **Object Types**: Rectangle, Ellipse, Point, Polygon, Polyline
- ✅ **Tilesets**: Embedded, External (.tsx/.tsj), Image-based, Collection
- ✅ **Tile Flipping**: Horizontal, Vertical, Diagonal
- ✅ **Custom Properties**: All Tiled property types (string, int, float, bool, color, file)
- ✅ **Parallax Scrolling**: Via layer custom properties
- ✅ **Prefab Mapping**: Object types → Entity prefabs

## API Reference

### World::LoadLevelFromTiled()

Primary function to load a Tiled map into the game world.

```cpp
bool World::LoadLevelFromTiled(const std::string& tiledMapPath);
```

**Parameters:**
- `tiledMapPath`: Path to .tmj file (e.g., "Resources/Maps/level1.tmj")

**Returns:** `true` on success, `false` on failure

**Example:**
```cpp
World& world = World::Get();
if (!world.LoadLevelFromTiled("Resources/Maps/forest_level.tmj")) {
    SYSTEM_LOG << "Failed to load level\n";
}
```

**What it does:**
1. Loads and parses the .tmj file using `TiledLevelLoader`
2. Converts to `LevelDefinition` using `TiledToOlympe`
3. Loads prefab mappings from `Config/tiled_prefab_mapping.json`
4. Unloads current level
5. Spawns entities from object layers
6. Applies custom property overrides to entities
7. Sets up collision zones
8. Configures parallax layers

### TiledLevelLoader Class

Low-level parser for Tiled files.

```cpp
namespace Olympe::Tiled {
    class TiledLevelLoader {
    public:
        bool LoadFromFile(const std::string& filepath, TiledMap& outMap);
        const std::string& GetLastError() const;
    };
}
```

**Usage:**
```cpp
Olympe::Tiled::TiledLevelLoader loader;
Olympe::Tiled::TiledMap map;
if (!loader.LoadFromFile("map.tmj", map)) {
    std::cout << "Error: " << loader.GetLastError() << "\n";
}
```

### TiledToOlympe Class

Converter from Tiled format to Olympe format.

```cpp
namespace Olympe::Tiled {
    class TiledToOlympe {
    public:
        void SetConfig(const ConversionConfig& config);
        bool Convert(const TiledMap& tiledMap, 
                    Olympe::Editor::LevelDefinition& outLevel);
        const std::string& GetLastError() const;
        const ParallaxLayerManager& GetParallaxLayers() const;
    };
}
```

**ConversionConfig:**
```cpp
struct ConversionConfig {
    std::map<std::string, std::string> typeToPrefabMap; // Object type → prefab path
    std::string defaultPrefab;                           // Fallback prefab
    std::vector<std::string> collisionLayerPatterns;     // e.g., "collision", "walls"
    std::vector<std::string> sectorLayerPatterns;        // e.g., "sectors", "zones"
    bool flipY;                                          // Coordinate conversion
    std::string resourceBasePath;                        // For resolving relative paths
};
```

## Using the F3 Menu to Load Levels

The engine provides an in-game debug menu for quick level loading during development.

**To open the Tiled Level Loader menu:**

1. Run your game/engine
2. Press **F3** (toggles menu visibility)
3. The "Tiled Level Loader" window appears

**Menu Features:**

- **File Browser**: Navigate to .tmj files in your project
- **Load Button**: Load selected map into current world
- **Recent Files**: Quick access to recently loaded maps
- **Error Display**: Shows parsing/conversion errors
- **Map Info**: Displays dimensions, orientation, layer count

**Keyboard Shortcuts:**
- `F3` - Toggle Tiled Loader menu
- `ESC` - Close menu

**Tips:**
- Menu persists across sessions (last directory remembered)
- Loads `Config/tiled_prefab_mapping.json` automatically
- Replaces current level (saves are lost unless explicitly saved)
- Works in both debug and release builds

## Prefab Mapping Configuration

Edit `Config/tiled_prefab_mapping.json` to define how Tiled object types map to entity prefabs:

```json
{
  "schema_version": 1,
  "mapping": {
    "player": "Blueprints/EntityPrefab/player_entity.json",
    "npc": "Blueprints/EntityPrefab/npc_entity.json",
    "guard": "Blueprints/EntityPrefab/guard_npc.json",
    "enemy": "Blueprints/EntityPrefab/enemy.json",
    "zombie": "Blueprints/EntityPrefab/zombie.json",
    "key": "Blueprints/EntityPrefab/key.json",
    "treasure": "Blueprints/EntityPrefab/treasure.json",
    "portal": "Blueprints/EntityPrefab/portal.json"
  }
}
```

**How it works:**
1. Object in Tiled has `type` property (e.g., "player")
2. Loader looks up type in mapping → finds prefab path
3. Spawns entity using `PrefabFactory::CreateEntity()`
4. Applies position from Tiled object coordinates
5. Applies custom property overrides from Tiled

**Special Object Types:**
- `collision` - Not in mapping; creates `CollisionZone_data` component automatically
- `way` - Not in mapping; creates patrol path (polyline → AI patrol points)
- `sector` - Not in mapping; stores polygon metadata in level definition

## Troubleshooting

### Map fails to load

**Symptom:** `LoadLevelFromTiled()` returns false, no entities spawn

**Solutions:**
1. Check file path is correct (relative to executable)
2. Verify .tmj file is valid JSON (open in text editor)
3. Check console for error messages (look for "TiledLevelLoader" logs)
4. Ensure external tilesets (.tsx/.tsj) exist in expected locations
5. Verify `Config/tiled_prefab_mapping.json` exists

### Entities spawn at wrong positions

**Symptom:** Objects appear offset or flipped

**Solutions:**
1. Check map orientation (orthogonal vs isometric)
2. For isometric, see `Documentation/TILED_ISOMETRIC.md` for coordinate conversion
3. Verify `flipY` setting in `ConversionConfig` (default: true for Tiled's top-left origin)
4. Check object anchor point in Tiled (should be bottom-left for most entities)

### Prefab not found

**Symptom:** "Failed to create entity from prefab" in console

**Solutions:**
1. Check object type in Tiled matches key in `tiled_prefab_mapping.json`
2. Verify prefab file exists at specified path
3. Check prefab JSON is valid (test with Blueprint Editor)
4. Ensure `PrefabFactory` has loaded prefab directory

### Collision not working

**Symptom:** Player walks through walls

**Solutions:**
1. Ensure collision objects have `type` set to "collision" in Tiled
2. Verify collision layer is visible in Tiled
3. Check collision objects overlap tiles correctly
4. Add `CollisionZone_data` component to collision blueprint
5. Verify physics system is active in World

### Tileset images not found

**Symptom:** Black tiles or missing graphics

**Solutions:**
1. Check tileset image paths are relative to .tmj file
2. Use external tilesets (.tsx) with consistent paths
3. Verify image files exist in specified locations
4. Check `resourceBasePath` in `ConversionConfig`
5. Use forward slashes (/) in paths, not backslashes

### Parallax layers not scrolling

**Symptom:** Image layers don't parallax

**Solutions:**
1. Add custom properties to image layer in Tiled:
   - `parallaxx` (float) - horizontal parallax factor
   - `parallaxy` (float) - vertical parallax factor
2. Verify layer is of type "imagelayer"
3. Check `ParallaxLayerManager` is active in rendering pipeline
4. Ensure layer is visible in Tiled

### Patrol paths not working

**Symptom:** NPCs don't follow paths

**Solutions:**
1. Verify polyline object has `type` set to "way"
2. Add `name` property to polyline (guards use `patrolPathName` to reference)
3. Check polyline has at least 2 points
4. Ensure NPC has `AIBlackboard_data` component
5. Set guard's `patrolPathName` property to match polyline's `name`

### Infinite map chunks missing

**Symptom:** Parts of infinite map don't render

**Solutions:**
1. Check chunk coordinates in .tmj file
2. Verify chunk data arrays are not empty
3. Ensure chunk dimensions match map's `chunkwidth`/`chunkheight`
4. Check for JSON parsing errors in console
5. Test with finite map first to isolate issue

## File Format Reference

**Supported Tiled File Formats:**
- `.tmj` - Tiled Map (JSON format) - **Primary format**
- `.tsj` - Tiled Tileset (JSON format) - External tilesets
- `.tsx` - Tiled Tileset (XML format) - Legacy external tilesets (limited support)

**Version Compatibility:**
- Tiled 1.9+ (JSON format)
- Schema version 1.10+

**Map Format Requirements:**
- Tile layer encoding: `data` array (no Base64/compression)
- Object layers: Standard JSON objects
- Custom properties: All standard Tiled types supported
- Coordinates: Top-left origin (converted automatically)

## Best Practices

1. **Use External Tilesets**: Easier to reuse across maps
2. **Name Layers Clearly**: Use conventions like "Ground", "Walls", "Collision", "Objects"
3. **Group Related Objects**: Use Tiled's layer groups for organization
4. **Test Incrementally**: Load maps frequently during editing to catch issues early
5. **Version Control .tmj Files**: JSON is git-friendly
6. **Keep Prefab Mapping Updated**: Add new object types as you create them
7. **Use Object Types**: Define Tiled object types in `objecttypes.xml` (see Templates)
8. **Document Custom Properties**: Add comments in prefab JSONs about expected properties

## See Also

- [TILED_ISOMETRIC.md](TILED_ISOMETRIC.md) - Isometric projection and rendering
- [TILED_SECTORS.md](TILED_SECTORS.md) - Sector system for LOD and streaming
- [TILED_WORKFLOW.md](TILED_WORKFLOW.md) - Complete workflow from Tiled to Olympe
- [TILED_OBJECT_TYPES.md](TILED_OBJECT_TYPES.md) - Reference for all object types
- [Templates/Tiled/README.md](../Templates/Tiled/README.md) - Ready-to-use Tiled templates
