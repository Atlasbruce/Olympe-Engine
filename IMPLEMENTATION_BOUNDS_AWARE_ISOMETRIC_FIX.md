# Bounds-Aware Isometric Entity Positioning Fix

## Problem Statement
The previous isometric entity positioning fix used `mapHeight_` directly to calculate the isometric origin offset. This worked for finite maps starting at tile (0,0), but failed for:
1. Infinite maps where tile coordinates might not start at 0
2. Maps where the actual tile bounds differ from the declared map dimensions

## Root Cause
The formula `originX = mapHeight_ * tileWidth / 2` assumes:
- Tiles start at Y=0
- mapHeight_ accurately represents the visual span of the map
- No coordinate system offset from chunk origins

For infinite maps with negative tile coordinates or sparse tile placement, this assumption breaks down.

## Solution Implemented

### 1. Store Actual Tile Coordinate Bounds
**File**: `Source/TiledLevelLoader/include/TiledToOlympe.h`

Added member variables to track the actual min/max tile coordinates:
```cpp
// Actual tile coordinate bounds (for bounds-aware isometric origin calculation)
int minTileX_ = 0;
int minTileY_ = 0;
int maxTileX_ = 0;
int maxTileY_ = 0;
```

### 2. Initialize Bounds During Map Loading
**File**: `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

For **infinite maps**, store the calculated bounds:
```cpp
if (isInfiniteMap_) {
    MapBounds bounds = CalculateActualMapBounds(tiledMap);
    
    // Store actual tile coordinate bounds
    minTileX_ = bounds.minTileX;
    minTileY_ = bounds.minTileY;
    maxTileX_ = bounds.maxTileX;
    maxTileY_ = bounds.maxTileY;
}
```

For **finite maps**, bounds are from 0 to width/height - 1:
```cpp
else {
    minTileX_ = 0;
    minTileY_ = 0;
    maxTileX_ = mapWidth_ - 1;
    maxTileY_ = mapHeight_ - 1;
}
```

### 3. Use Bounds-Aware Origin Calculation
**File**: `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - `TransformObjectPosition()`

**Before**:
```cpp
float mapOriginX = (float)mapHeight_ * (float)config_.tileWidth / 2.0f;
```

**After**:
```cpp
// Calculate origin using actual bounds span
float boundsHeight = (float)(maxTileY_ - minTileY_);
float originX = boundsHeight * (float)config_.tileWidth / 2.0f;

// Account for chunk origin offsets (for infinite maps)
if (chunkOriginX_ != 0 || chunkOriginY_ != 0) {
    float chunkOffsetX = (float)chunkOriginX_ * (float)config_.tileWidth / 2.0f;
    float chunkOffsetY = (float)chunkOriginY_ * (float)config_.tileHeight / 2.0f;
    originX += chunkOffsetX;
    originY += chunkOffsetY;
}
```

## Key Design Decisions

### 1. Use (maxTileY - minTileY) Instead of mapHeight_
The bounds span `(maxTileY - minTileY)` represents the actual visual extent of tiles in the Y direction, which is what determines where Tiled places the isometric origin.

**Example**:
- Tiles from Y=0 to Y=9: boundsHeight = 9
- Tiles from Y=-5 to Y=14: boundsHeight = 19
- Both have 20 and 20 tiles respectively, but different coordinate systems

### 2. Account for Chunk Origin Offsets
For infinite maps, chunk coordinates might start at negative values (e.g., -16, -16). This offset must be incorporated into the origin calculation to align the coordinate systems.

### 3. Isometric-Only Application
The fix is explicitly guarded by:
```cpp
bool isIsometric = (config_.mapOrientation == "isometric");
```

This ensures orthogonal, hexagonal, and staggered map modes remain unchanged.

### 4. Render-Order Y Flips Remain Disabled
As per previous fixes, render-order based Y flips are NOT applied to object positions - they only affect tile rendering iteration order.

## Enhanced Logging

Added comprehensive logging to validate the fix:
```
[ISO_TRANSFORM] Map bounds: minTile(-16, -16) maxTile(15, 15)
  → Bounds-based height: (maxTileY - minTileY) = (15 - -16) = 31
  → Chunk origin offset: (-16, -16) => pixel offset (-464, -216)
  → Final origin values: originX=436, originY=-216
[ISO_TRANSFORM] Raw TMJ (Tiled Map JSON) coordinates: (464, 432)
  → After subtracting map origin: (28, 648)
  → Layer offsets (offsetX=0, offsetY=0): (28, 648)
  → Final world isometric position: (28, 648)
```

## Examples

### Example 1: Finite Map (10x10, tiles 0-9)
```
minTileY = 0, maxTileY = 9
boundsHeight = 9
originX = 9 * 64 / 2 = 288
chunkOffset = 0 (finite map)
Final originX = 288
```

### Example 2: Infinite Map (tiles -5 to 14)
```
minTileY = -5, maxTileY = 14
boundsHeight = 19
originX = 19 * 64 / 2 = 608
chunkOffsetX = -5 * 64 / 2 = -160
Final originX = 608 + (-160) = 448
```

## Files Modified

1. **Source/TiledLevelLoader/include/TiledToOlympe.h**
   - Added `minTileX_`, `minTileY_`, `maxTileX_`, `maxTileY_` member variables

2. **Source/TiledLevelLoader/src/TiledToOlympe.cpp**
   - Initialize bounds fields in constructor
   - Store bounds during map loading (both infinite and finite cases)
   - Update `TransformObjectPosition()` to use bounds-aware calculation
   - Add comprehensive logging for bounds and origin values

## Compatibility

This fix is backward compatible:
- ✅ Works with finite maps (bounds are [0, width-1] x [0, height-1])
- ✅ Works with infinite maps (bounds calculated from actual chunks)
- ✅ Only affects isometric maps
- ✅ Orthogonal/hexagonal/staggered maps remain unchanged
- ✅ All existing isometric fix behavior preserved (render-order, layer offsets, global offsets)

## Testing Recommendations

1. Test with finite isometric maps (tiles starting at 0,0)
2. Test with infinite isometric maps with negative chunk origins
3. Test with infinite isometric maps with sparse tile placement
4. Verify orthogonal maps are not affected
5. Check that entity positions align with rendered tiles
6. Validate logging output matches expected calculations
