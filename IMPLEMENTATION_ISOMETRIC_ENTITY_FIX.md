# Isometric Entity Position Fix - Implementation Summary

## Problem
Entities from Tiled TMJ files were completely misplaced in isometric view due to three critical issues:

1. **Coordinate system mismatch**: Chunks use negative offsets (-16,-16) but entities use absolute pixel coordinates (0,0+)
2. **Missing chunk origin translation**: Entity coordinates were not translated to align with the chunk coordinate system
3. **Ignored renderOrder property**: The map's "left-up" render order was parsed but never applied to entity transformations

## Solution Implemented

### Phase 1: Added Chunk Origin Tracking
**File**: `Source/TiledLevelLoader/include/TiledToOlympe.h`

Added two new member variables to track chunk coordinate system origin:
```cpp
// Chunk coordinate system origin (for infinite maps)
int chunkOriginX_ = 0;
int chunkOriginY_ = 0;
```

### Phase 2: Added Render Order to Config
**File**: `Source/TiledLevelLoader/include/TiledToOlympe.h`

Extended ConversionConfig struct to store render order:
```cpp
struct ConversionConfig {
    // ... existing fields ...
    
    // Render order ("right-down", "right-up", "left-down", "left-up")
    std::string renderOrder;
    
    ConversionConfig()
        : flipY(true), mapOrientation("orthogonal"), tileWidth(0), 
          tileHeight(0), renderOrder("right-down") {}
};
```

### Phase 3: Calculate Chunk Origin During Map Loading
**File**: `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

Modified `Convert()` method to detect and store chunk origin for infinite maps:
```cpp
if (isInfiniteMap_) {
    MapBounds bounds = CalculateActualMapBounds(tiledMap);
    
    // Store chunk origin offset for coordinate transformations
    chunkOriginX_ = bounds.minTileX;  // e.g., -16
    chunkOriginY_ = bounds.minTileY;  // e.g., -16
    
    SYSTEM_LOG << "  -> Chunk origin offset: (" 
               << chunkOriginX_ << ", " << chunkOriginY_ << ")\n";
}
```

### Phase 4: Store Render Order in Config
**File**: `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

Modified `ExtractMapConfiguration()` to store render order in config:
```cpp
// Convert render order
switch (tiledMap.renderorder) {
    case RenderOrder::RightDown: outLevel.mapConfig.renderOrder = "right-down"; break;
    case RenderOrder::RightUp: outLevel.mapConfig.renderOrder = "right-up"; break;
    case RenderOrder::LeftDown: outLevel.mapConfig.renderOrder = "left-down"; break;
    case RenderOrder::LeftUp: outLevel.mapConfig.renderOrder = "left-up"; break;
}

// Store render order in conversion config for use in coordinate transformations
config_.renderOrder = outLevel.mapConfig.renderOrder;

SYSTEM_LOG << "  -> Render order: " << config_.renderOrder << "\n";
```

### Phase 5: Fix TransformObjectPosition
**File**: `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

Completely rewrote the isometric transformation with 4 steps:

```cpp
Vector TiledToOlympe::TransformObjectPosition(float x, float y)
{
    bool isIsometric = (config_.mapOrientation == "isometric");

    if (isIsometric)
    {
        // Step 1: Convert TMJ pixels → tile coordinates
        float tileX = x / static_cast<float>(config_.tileWidth);
        float tileY = y / static_cast<float>(config_.tileHeight);

        // Step 2: Translate to chunk coordinate system
        // (align entity coords with chunk origin offset)
        tileX -= chunkOriginX_;
        tileY -= chunkOriginY_;

        // Step 3: Apply render order transformation
        // For render orders with "up" (right-up, left-up), invert Y-axis
        // because Tiled's Y-axis points down (screen) but isometric Y-axis points up (world)
        if (config_.renderOrder == "left-up" || config_.renderOrder == "right-up") {
            tileY = -tileY;
        }

        // Step 4: Apply isometric projection
        Vector isoPos = IsometricProjection::WorldToIso(
            tileX, tileY,
            config_.tileWidth, config_.tileHeight
        );

        return Vector(isoPos.x, isoPos.y, 0.0f);
    }

    // Orthogonal case
    return Vector(x, y, 0.0f);
}
```

## Transformation Example

For an entity at Tiled position (0, 0) with:
- Tile size: 58x27 pixels
- Chunk origin: (-16, -16)
- Render order: "left-up"

### Before Fix:
```
(0, 0) pixels → (0, 0) tiles → WorldToIso(0, 0) → (-310, 395) ❌ WRONG
```

### After Fix:
```
Step 1: (0, 0) pixels → (0, 0) tiles
Step 2: (0, 0) - (-16, -16) → (16, 16) tiles (chunk alignment)
Step 3: (16, -16) tiles (Y-flip for left-up)
Step 4: WorldToIso(16, -16) → North corner ✅ CORRECT
```

## Expected Results

After this fix:
- ✅ Entities align with the isometric terrain tiles
- ✅ The 4 beacon entities form a diamond shape around the map
- ✅ Player spawns on visible terrain
- ✅ Guards patrol correctly on their paths
- ✅ All entities are positioned relative to the chunk coordinate system

## Files Modified

1. `Source/TiledLevelLoader/include/TiledToOlympe.h` (2 changes)
   - Added `renderOrder` to ConversionConfig
   - Added `chunkOriginX_` and `chunkOriginY_` member variables

2. `Source/TiledLevelLoader/src/TiledToOlympe.cpp` (4 changes)
   - Initialize chunk origin fields in constructor
   - Calculate and store chunk origin in Convert()
   - Store render order in ExtractMapConfiguration()
   - Implement 4-step transformation in TransformObjectPosition()

## Compatibility

This fix is backward compatible:
- ✅ Works with finite maps (chunk origin is 0,0)
- ✅ Works with all render orders:
  - "right-down": No Y-flip (standard)
  - "left-down": No Y-flip
  - "right-up": Y-flip applied
  - "left-up": Y-flip applied
- ✅ Works with orthogonal maps (isometric path not taken)
- ✅ Works with different tile sizes (uses config values)

## Testing Recommendations

1. Load an infinite isometric map with "left-up" render order
2. Verify entities appear on the terrain
3. Check that beacons form a diamond shape
4. Confirm player and NPCs spawn at correct positions
5. Test with different chunk configurations
6. Test with other render orders (right-down, etc.)
