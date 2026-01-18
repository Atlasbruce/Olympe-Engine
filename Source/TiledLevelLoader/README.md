# TiledLevelLoader Module

Complete Tiled MapEditor (.tmj) file loader for Olympe Engine.

## Features

- **Full Tiled Format Support**
  - Orthogonal and isometric map orientations
  - Finite and infinite (chunked) maps
  - All layer types: tile layers, object groups, image layers, groups
  - External tilesets (.tsx, .tsj) with caching
  - Embedded tilesets
  
- **Data Encoding**
  - CSV format
  - Base64 encoding with gzip/zlib compression
  - Tile flip flags (horizontal, vertical, diagonal)

- **Map Features**
  - Parallax scrolling layers (parallaxx, parallaxy)
  - Repeating textures (repeatx, repeaty)
  - Layer offsets, opacity, tint colors
  - Custom properties (string, int, float, bool, color, file)

- **Object Conversion**
  - Objects → Olympe entities with prefab mapping
  - Rectangles, ellipses, points
  - Polygons → sectors
  - Polylines → patrol paths
  - Custom properties → entity overrides

- **Isometric Support**
  - World ↔ isometric coordinate transformations
  - Tile-to-screen and screen-to-tile conversions

## Usage

### Loading a Tiled Map

```cpp
#include "TiledLevelLoader.h"

using namespace Olympe::Tiled;

TiledLevelLoader loader;
TiledMap map;

if (loader.LoadFromFile("maps/level1.tmj", map)) {
    SYSTEM_LOG << "Map loaded: " << map.width << "x" << map.height << std::endl;
    SYSTEM_LOG << "Tile size: " << map.tilewidth << "x" << map.tileheight << std::endl;
    
    // Process layers
    for (const auto& layer : map.layers) {
        SYSTEM_LOG << "Layer: " << layer->name << std::endl;
    }
} else {
    SYSTEM_LOG << "Error: " << loader.GetLastError() << std::endl;
}
```

### Converting to Olympe Format

```cpp
#include "TiledToOlympe.h"
#include "OlympeTilemapEditor/include/LevelManager.h"

using namespace Olympe::Tiled;

// Configure conversion
ConversionConfig config;
config.flipY = true;
config.resourceBasePath = "Resources";
config.typeToPrefabMap["Player"] = "Blueprints/Player.json";
config.typeToPrefabMap["Enemy"] = "Blueprints/Enemy.json";
config.collisionLayerPatterns = {"collision", "walls"};

// Convert
TiledToOlympe converter;
converter.SetConfig(config);

// Optionally load prefab mappings from JSON file
// This will merge with or replace the manual mappings above
if (!converter.LoadPrefabMapping("Config/tiled_prefab_mapping.json")) {
    SYSTEM_LOG << "Warning: Could not load prefab mapping file" << std::endl;
}

Olympe::Editor::LevelDefinition level;
if (converter.Convert(map, level)) {
    SYSTEM_LOG << "Converted: " << level.entities.size() << " entities" << std::endl;
    
    // Get parallax layers
    const auto& parallax = converter.GetParallaxLayers();
    SYSTEM_LOG << "Parallax layers: " << parallax.GetLayerCount() << std::endl;
}
```

### Loading Prefab Mappings from JSON

Instead of manually configuring the type-to-prefab map in code, you can load mappings from a JSON file:

```cpp
TiledToOlympe converter;

// Load mappings from JSON file
if (converter.LoadPrefabMapping("Config/tiled_prefab_mapping.json")) {
    SYSTEM_LOG << "Prefab mappings loaded successfully" << std::endl;
} else {
    SYSTEM_LOG << "Failed to load mappings: " << converter.GetLastError() << std::endl;
}

// The JSON file format:
// {
//   "schema_version": 1,
//   "mapping": {
//     "player": "Blueprints/EntityPrefab/player.json",
//     "enemy": "Blueprints/EntityPrefab/enemy.json",
//     "collectible": "Blueprints/EntityPrefab/item.json"
//   }
// }
```


### Isometric Projection

```cpp
#include "IsometricProjection.h"

using namespace Olympe::Tiled;

int tileWidth = 64;
int tileHeight = 32;

// World to screen
Vec2 screenPos = IsometricProjection::WorldToIso(5.0f, 3.0f, tileWidth, tileHeight);

// Screen to world
Vec2 worldPos = IsometricProjection::IsoToWorld(screenPos.x, screenPos.y, tileWidth, tileHeight);

// Screen to tile
int tileX, tileY;
IsometricProjection::ScreenToTile(mouseX, mouseY, tileWidth, tileHeight, tileX, tileY);
```

### Working with Tile GIDs

Tiled uses Global Tile IDs (GIDs) which include both the tile ID and flip flags. Use the utility functions in TiledStructures.h to extract this information:

```cpp
#include "TiledStructures.h"

using namespace Olympe::Tiled;

// Given a tile GID from the map data
uint32_t gid = layer.data[index];

// Extract the actual tile ID (removing flip flags)
uint32_t tileId = GetTileId(gid);

// Check flip flags
bool flippedH = IsFlippedHorizontally(gid);
bool flippedV = IsFlippedVertically(gid);
bool flippedD = IsFlippedDiagonally(gid);

// Use these when rendering or processing tiles
if (tileId > 0) {  // 0 = empty tile
    // Render the tile with appropriate flipping
    RenderTile(tileId, flippedH, flippedV, flippedD);
}
```

The flip flag constants are also available if you need to manipulate GIDs:
- `FLIPPED_HORIZONTALLY_FLAG` - 0x80000000
- `FLIPPED_VERTICALLY_FLAG` - 0x40000000
- `FLIPPED_DIAGONALLY_FLAG` - 0x20000000
- `TILE_ID_MASK` - 0x1FFFFFFF

## Architecture

### Core Components

- **TiledStructures.h**: Data structures matching Tiled JSON format
- **TiledLevelLoader**: Main parser for .tmj files
- **TiledDecoder**: Base64 and compression utilities
- **TilesetParser**: Parser for .tsx/.tsj external tilesets
- **TilesetCache**: Singleton cache for loaded tilesets
- **TiledToOlympe**: Converter to Olympe LevelDefinition format
- **IsometricProjection**: Isometric coordinate math
- **ParallaxLayerManager**: Parallax scrolling system
- **TiledJsonHelper**: JSON parsing utilities

### Dependencies

- **nlohmann/json**: JSON parsing (from `Source/third_party/nlohmann/json.hpp`)
- **miniz**: Compression (single-header library)
- **tinyxml2**: XML parsing for .tsx files
- **SDL3**: Window/rendering (transitive)
- **Olympe system/logging**: SYSTEM_LOG macro

## File Format Support

### Supported
- ✅ Orthogonal maps
- ✅ Isometric maps
- ✅ Finite maps
- ✅ Infinite maps with chunks
- ✅ CSV encoding
- ✅ Base64 encoding
- ✅ Gzip compression
- ✅ Zlib compression
- ✅ Tile layers
- ✅ Object layers (all object types)
- ✅ Image layers
- ✅ Group layers
- ✅ External tilesets (.tsx, .tsj)
- ✅ Embedded tilesets
- ✅ Custom properties
- ✅ Parallax scrolling
- ✅ Tile flip flags

### Not Implemented
- ❌ Staggered maps
- ❌ Hexagonal maps
- ❌ Tile animations
- ❌ Wang sets
- ❌ Templates

## Integration

The module is integrated into the Olympe Engine build system via CMake:

1. Added to root `CMakeLists.txt`:
   ```cmake
   add_subdirectory(Source/TiledLevelLoader)
   ```

2. Linked to OlympeCore:
   ```cmake
   target_link_libraries(OlympeCore PUBLIC TiledLevelLoader)
   ```

## Examples

See the `Examples/TiledLoader/` directory (to be created) for complete examples:
- Loading orthogonal maps
- Loading isometric maps
- Converting to Olympe format
- Handling parallax layers
- Custom property mapping

## Notes

- Uses C++14 standard (no std::optional, std::filesystem)
- All errors logged via SYSTEM_LOG macro
- Thread-safe tileset caching
- Handles relative and absolute file paths
- Supports Windows and Unix path separators

## License

Part of Olympe Engine. See main LICENSE file.
