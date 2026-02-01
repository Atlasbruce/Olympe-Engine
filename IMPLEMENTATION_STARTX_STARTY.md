# Implementation: startx/starty Offset Support

## Overview

This implementation adds proper support for `startx` and `starty` properties from Tiled Map JSON (TMJ) files. These properties define tile coordinate offsets for layers, especially important for infinite maps where tile layers can start at arbitrary positions in the world coordinate system.

## Problem Statement

Previously, the Olympe Engine's Tiled integration did not parse or apply the `startx` and `starty` offset properties defined in TMJ files. This could lead to discrepancies between tile and entity positions seen in the Tiled editor versus the Olympe Engine.

### What are startx and starty?

In Tiled's TMJ format:
- `startx` and `starty` are integer properties of **tile layers** (not object layers)
- They define the starting tile position offset for the layer
- For example, if `startx=-80` and `starty=-16`:
  - Tile at index [0,0] in the layer's data array represents the tile at world position (-80, -16)
  - Not at world position (0, 0)
- These are particularly important for infinite maps where layers can span arbitrary coordinate ranges

## Changes Made

### 1. TiledStructures.h

**Added fields to TiledLayer struct:**
```cpp
int startx;  // Starting X tile position offset (for infinite maps)
int starty;  // Starting Y tile position offset (for infinite maps)
```

**Updated constructor:**
```cpp
TiledLayer()
    : id(0), type(LayerType::TileLayer), visible(true),
      opacity(1.0f), offsetx(0.0f), offsety(0.0f),
      parallaxx(1.0f), parallaxy(1.0f), tintcolor(0xFFFFFFFF),
      width(0), height(0), startx(0), starty(0),  // <-- Added initialization
      repeatx(false), repeaty(false) {}
```

### 2. TiledLevelLoader.cpp

**Modified ParseTileLayer to parse startx/starty:**
```cpp
bool TiledLevelLoader::ParseTileLayer(const json& j, TiledLayer& layer)
{
    layer.width = GetInt(j, "width");
    layer.height = GetInt(j, "height");
    layer.startx = GetInt(j, "startx", 0);  // <-- Added
    layer.starty = GetInt(j, "starty", 0);  // <-- Added
    layer.encoding = GetString(j, "encoding", "csv");
    layer.compression = GetString(j, "compression", "");
    // ... rest of function
}
```

### 3. TiledToOlympe.cpp

**Added debug logging in ProcessVisualLayers:**
```cpp
// Debug: Log startx/starty offsets
if (layer->startx != 0 || layer->starty != 0) {
    SYSTEM_LOG << "  -> [DEBUG] Tile Layer '" << layer->name 
               << "' has startx=" << layer->startx 
               << ", starty=" << layer->starty << "\n";
}
```

This logging helps track which layers have offsets during TMJ parsing, making it easier to debug position discrepancies.

### 4. IsometricProjection.h

**Extended function signatures:**
```cpp
// Added optional startX and startY parameters (default 0)
static Vector WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight, 
                        int startX = 0, int startY = 0);

static Vector IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                        int startX = 0, int startY = 0);
```

### 5. IsometricProjection.cpp

**Updated WorldToIso implementation:**
```cpp
Vector IsometricProjection::WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight,
                                      int startX, int startY)
{
    // Apply startx/starty offsets to world coordinates
    float adjustedX = worldX + startX;
    float adjustedY = worldY + startY;
    
    // Standard isometric projection (diamond orientation)
    Vector result;
    result.x = (adjustedX - adjustedY) * ((float)tileWidth * 0.5f);
    result.y = (adjustedX + adjustedY) * ((float)tileHeight * 0.5f);
    return result;
}
```

**Updated IsoToWorld implementation:**
```cpp
Vector IsometricProjection::IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                      int startX, int startY)
{
    // Inverse isometric projection
    Vector result;
    float halfWidth = tileWidth * 0.5f;
    float halfHeight = tileHeight * 0.5f;
    
    result.x = (isoX / halfWidth + isoY / halfHeight) * 0.5f;
    result.y = (isoY / halfHeight - isoX / halfWidth) * 0.5f;
    
    // Apply startx/starty offsets (inverse)
    result.x -= startX;
    result.y -= startY;
    
    return result;
}
```

### 6. example_isometric.cpp

**Added documentation example:**
```cpp
// Example: Working with layer offsets (for infinite maps)
// If a tile layer has startx=-80, starty=-16, tile[0][0] in that layer
// represents tile at world position (-80, -16), not (0, 0)
int startX = -80;  // Example layer offset
int startY = -16;  // Example layer offset
int layerTileX = 0;  // First tile in layer data
int layerTileY = 0;

Vec2 screenPosWithOffset = IsometricProjection::WorldToIso(
    layerTileX, layerTileY, map.tilewidth, map.tileheight, startX, startY);
// ...
```

## How It Works

### For Tile Rendering

When rendering tiles from a layer with `startx` and `starty` offsets:

```cpp
// Get the layer's offsets
int startX = layer.startx;
int startY = layer.starty;

// For each tile in the layer's data
for (int layerY = 0; layerY < layer.height; layerY++) {
    for (int layerX = 0; layerX < layer.width; layerX++) {
        // Convert layer tile coordinates to isometric screen position
        Vector screenPos = IsometricProjection::WorldToIso(
            layerX, layerY, tileWidth, tileHeight, startX, startY);
        
        // Render tile at screenPos
        // ...
    }
}
```

### For Objects

Objects in Tiled are placed using pixel coordinates in the global map coordinate system. They are **not** affected by tile layer startx/starty offsets because:

1. Objects are in object layers, not tile layers
2. Object x,y coordinates are already in the correct global coordinate system
3. The `TransformObjectPosition` function handles objects independently

Therefore, `TransformObjectPosition` uses the default offset values (0, 0) when calling `WorldToIso`, which is correct.

## Testing

### Unit Test

A standalone test (`test_startx_starty.cpp`) verifies the mathematical correctness:

```bash
g++ -std=c++14 test_startx_starty.cpp -o test_startx_starty
./test_startx_starty
```

All tests pass with zero round-trip error, confirming:
- The offset transformations are mathematically correct
- WorldToIso and IsoToWorld are proper inverses
- Both positive and negative offsets work correctly

### Example TMJ Files

The implementation has been designed to work with real-world TMJ files such as:
- `Gamedata/Levels/isometric_quest.tmj` (has startx=-80, starty=-16)
- `Gamedata/Levels/isometric_quest_solo.tmj`

These files contain:
- Tile layers with startx/starty offsets
- Objects like "Beacon Far West" and "Beacon Far South" that should align correctly with the tiles

## Backward Compatibility

The changes are fully backward compatible:

1. **Default parameters**: The new `startX` and `startY` parameters default to 0, so existing code continues to work without modification
2. **Parsing**: TMJ files without startx/starty properties will have these values default to 0 during parsing
3. **Optional logging**: Debug logging only outputs when offsets are non-zero

## Usage Example

```cpp
// Load a map
TiledLevelLoader loader;
TiledMap map;
loader.LoadFromFile("map.tmj", map);

// Iterate through layers
for (const auto& layer : map.layers) {
    if (layer->type == LayerType::TileLayer) {
        // Check for offsets
        if (layer->startx != 0 || layer->starty != 0) {
            std::cout << "Layer '" << layer->name 
                     << "' has offsets: (" << layer->startx 
                     << ", " << layer->starty << ")" << std::endl;
        }
        
        // Render tiles with offsets
        for (int y = 0; y < layer->height; y++) {
            for (int x = 0; x < layer->width; x++) {
                Vector screenPos = IsometricProjection::WorldToIso(
                    x, y, map.tilewidth, map.tileheight, 
                    layer->startx, layer->starty);
                // Render at screenPos...
            }
        }
    }
}
```

## Files Modified

1. `Source/TiledLevelLoader/include/TiledStructures.h` - Added startx/starty fields
2. `Source/TiledLevelLoader/src/TiledLevelLoader.cpp` - Added parsing
3. `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - Added debug logging
4. `Source/TiledLevelLoader/include/IsometricProjection.h` - Extended signatures
5. `Source/TiledLevelLoader/src/IsometricProjection.cpp` - Applied offset logic
6. `Examples/TiledLoader/example_isometric.cpp` - Added usage example

## Files Added

1. `test_startx_starty.cpp` - Standalone unit test

## Next Steps

To complete the integration:

1. **Rendering System**: Update the tile rendering code to pass layer offsets when calling `WorldToIso`
2. **Camera System**: Ensure camera calculations account for layer offsets if needed
3. **Editor Integration**: If using a level editor, ensure it displays layers with correct offsets
4. **Additional Testing**: Test with various infinite maps with different offset ranges

## References

- [Tiled Map Format Documentation](https://doc.mapeditor.org/en/stable/reference/json-map-format/)
- Tiled TMJ format specifies startx/starty as optional integer properties on tile layers
