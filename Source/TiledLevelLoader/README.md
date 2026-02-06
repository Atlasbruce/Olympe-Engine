# TiledLevelLoader Module

Complete Tiled MapEditor file loader for Olympe Engine with support for both JSON (.tmj) and XML (.tmx) formats.

## Features

- **Full Tiled Format Support**
  - **TMJ (JSON)** and **TMX (XML)** map formats
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

- **GID Resolution System**
  - Automatic lastgid calculation for all tilesets
  - Fast GID → tileset lookup
  - GID → local tile ID conversion
  - GID → atlas coordinates calculation
  - Built-in caching for performance

## Usage

### Loading a Tiled Map (TMJ or TMX)

```cpp
#include "TiledLevelLoader.h"

using namespace Olympe::Tiled;

TiledLevelLoader loader;
TiledMap map;

// Auto-detects format by extension (.tmx, .tmj, .json)
if (loader.LoadFromFile("maps/level1.tmx", map)) {
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

### Using the GID Resolver

```cpp
#include "TiledStructures.h"

using namespace Olympe::Tiled;

// Get a GID from a tile layer
uint32_t gid = layer->data[index];

// Resolve GID to get all information
ResolvedGid resolved = ResolveGid(map, gid);

if (resolved.IsValid()) {
    SYSTEM_LOG << "Tileset: " << resolved.tileset->name << std::endl;
    SYSTEM_LOG << "Local ID: " << resolved.localId << std::endl;
    SYSTEM_LOG << "Atlas coords: (" << resolved.tileX << ", " << resolved.tileY << ")" << std::endl;
    SYSTEM_LOG << "Flip H: " << resolved.flipH << ", V: " << resolved.flipV << ", D: " << resolved.flipD << std::endl;
    
    // Access tileset properties
    SYSTEM_LOG << "Tile offset: (" << resolved.tileset->tileoffsetX 
              << ", " << resolved.tileset->tileoffsetY << ")" << std::endl;
}

// Or use map methods directly
const TiledTileset* tileset = map.FindTilesetForGid(gid);
if (tileset) {
    int localId = tileset->GetLocalId(gid);
    int tileX, tileY;
    tileset->GetTileCoords(gid, tileX, tileY);
}
```

### Validating Image Resources

```cpp
#include "TiledStructures.h"

// Get all image paths that need to be loaded
std::vector<std::string> imagePaths = GetAllImagePaths(map);

SYSTEM_LOG << "Map requires " << imagePaths.size() << " images:" << std::endl;
for (const auto& path : imagePaths) {
    SYSTEM_LOG << "  - " << path << std::endl;
    // Ensure image is loaded in your texture manager
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

Olympe::Editor::LevelDefinition level;
if (converter.Convert(map, level)) {
    SYSTEM_LOG << "Converted: " << level.entities.size() << " entities" << std::endl;
    
    // Get parallax layers
    const auto& parallax = converter.GetParallaxLayers();
    SYSTEM_LOG << "Parallax layers: " << parallax.GetLayerCount() << std::endl;
}
```

### Isometric Coordinate Conversion

**Important:** Tiled stores isometric object positions where BOTH X and Y are measured in `tileHeight` pixel units.

```cpp
#include "IsometricProjection.h"

using namespace Olympe::Tiled;

// TMJ to World Coordinate Conversion (for objects)
// =================================================
// Formula: 
//   tileX = tmjPixelX / tileHeight
//   tileY = tmjPixelY / tileHeight
//   worldX = (tileX - tileY) * (tileWidth / 2)
//   worldY = (tileX + tileY) * (tileHeight / 2)

int tileWidth = 58;
int tileHeight = 27;

// Example: player_1 at TMJ (1818.4, 1064.26)
float tmjX = 1818.4f;
float tmjY = 1064.26f;

// Step 1: Convert to tile coords (both divided by tileHeight!)
float tileX = tmjX / tileHeight;  // = 67.35
float tileY = tmjY / tileHeight;  // = 39.42

// Step 2: Apply isometric projection
float worldX = (tileX - tileY) * (tileWidth * 0.5f);  // = 810
float worldY = (tileX + tileY) * (tileHeight * 0.5f); // = 1441

// Result: Entity renders at tile (67, 39) ✓

// Utility functions (for screen ↔ world conversion)
Vec2 screenPos = IsometricProjection::WorldToIso(worldX, worldY, tileWidth, tileHeight);
Vec2 worldPos = IsometricProjection::IsoToWorld(screenPos.x, screenPos.y, tileWidth, tileHeight);

// Screen to tile
int outTileX, outTileY;
IsometricProjection::ScreenToTile(mouseX, mouseY, tileWidth, tileHeight, outTileX, outTileY);
```

See `IMPLEMENTATION_ISOMETRIC_ENTITY_PLACEMENT_FIX.md` for detailed documentation.

## GID Resolution System

The module provides a comprehensive GID (Global Tile ID) resolution system that automatically:

1. **Calculates lastgid** for each tileset based on:
   - Explicit `tilecount` from Tiled
   - OR calculated from image dimensions: `(imagewidth / tilewidth) * (imageheight / tileheight)`
   - Accounts for margin and spacing

2. **Resolves GIDs** to tileset information:
   - Finds the correct tileset for any GID
   - Calculates local tile ID within the tileset
   - Computes atlas coordinates (X, Y) for rendering
   - Extracts flip flags (horizontal, vertical, diagonal)

3. **Provides helper functions**:
   - `map.FindTilesetForGid(gid)` - Find tileset containing a GID
   - `tileset.ContainsGid(gid)` - Check if GID belongs to tileset
   - `tileset.GetLocalId(gid)` - Convert GID to local tile ID
   - `tileset.GetTileCoords(gid, x, y)` - Get atlas coordinates
   - `ResolveGid(map, gid)` - One-stop resolution with all info

### Performance Notes

- GID resolution is O(n) where n is the number of tilesets (typically small)
- lastgid values are cached after loading
- No dynamic allocations during resolution
- Suitable for hot-path rendering code

## Architecture

### Core Components

- **TiledStructures.h**: Data structures matching Tiled JSON/XML format with GID resolution
- **TiledLevelLoader**: Main parser for .tmx/.tmj files with auto-detection
- **TiledDecoder**: Base64 and compression utilities
- **TilesetParser**: Parser for .tsx/.tsj external tilesets
- **TilesetCache**: Singleton cache for loaded tilesets
- **TiledToOlympe**: Converter to Olympe LevelDefinition format
- **IsometricProjection**: Isometric coordinate math
- **ParallaxLayerManager**: Parallax scrolling system
- **TiledJsonHelper**: JSON parsing utilities

### Robustness & Validation

See [VALIDATION_AND_ROBUSTNESS.md](VALIDATION_AND_ROBUSTNESS.md) for details on:
- Data integrity validation
- Enhanced error reporting
- Tileoffset parsing and application
- Corruption detection
- Error recovery strategies

### Dependencies

- **nlohmann/json**: JSON parsing (from `Source/third_party/nlohmann/json.hpp`)
- **miniz**: Compression (single-header library)
- **tinyxml2**: XML parsing for .tmx/.tsx files
- **SDL3**: Window/rendering (transitive)
- **Olympe system/logging**: SYSTEM_LOG macro

## File Format Support

### Map Formats
- ✅ **TMX (XML)** - Full support with auto-detection
- ✅ **TMJ (JSON)** - Full support with auto-detection
- ✅ Orthogonal maps
- ✅ Isometric maps
- ✅ Finite maps
- ✅ Infinite maps with chunks

### Tileset Formats
- ✅ **TSX (XML)** - External tilesets
- ✅ **TSJ (JSON)** - External tilesets
- ✅ Embedded tilesets (both TMX and TMJ)
- ✅ Image-based tilesets (single atlas)
- ✅ Collection tilesets (individual images)

### Data Encodings
- ✅ CSV encoding
- ✅ Base64 encoding
- ✅ Gzip compression
- ✅ Zlib compression
- ✅ Uncompressed XML tiles
- ✅ Tile layers
- ✅ Object layers (all object types: rectangle, ellipse, point, polygon, polyline, text)
- ✅ Image layers
- ✅ Group layers (recursive)
- ✅ Custom properties (string, int, float, bool, color, file)
- ✅ Parallax scrolling
- ✅ Tile flip flags (horizontal, vertical, diagonal)
- ✅ Tileoffset support (for alignment)
- ✅ lastgid calculation and GID resolution

### Not Implemented
- ❌ Staggered maps
- ❌ Hexagonal maps
- ❌ Tile animations
- ❌ Wang sets
- ❌ Templates

## Image Loading

The module parses and tracks all image paths from:
- Tileset images (main atlas images)
- Collection tileset individual images
- Image layer backgrounds

**Important**: The runtime must ensure these images are loaded before rendering. Use the `GetAllImagePaths(map)` helper to enumerate required images for preloading.

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
