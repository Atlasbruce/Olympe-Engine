# Isometric Entity Placement Fix - Implementation Summary

## Problem Statement

Entities placed in Tiled's isometric maps were not aligning with the rendered tiles. The issue was that TMJ object coordinates were kept in pixel coordinates without rebasing on the isometric origin used for tile rendering, causing visual offset/scale issues.

## Root Cause Analysis

### Coordinate Systems

1. **Tile Renderer Coordinate System** (in `Source/World.cpp:GetIsometricOriginX/Y()`):
   - Uses an isometric origin calculated from map bounds:
     ```cpp
     isoOriginX = (minTileX - minTileY) * (tileWidth / 2.0f);
     isoOriginY = (minTileX + minTileY) * (tileHeight / 2.0f);
     ```
   - This origin ensures tiles render correctly relative to the map bounds
   - Tiles are positioned relative to this isometric origin

2. **Tiled TMJ Coordinate System**:
   - Objects are placed using pixel coordinates
   - For isometric maps, Tiled uses a coordinate system where:
     - (0, 0) is at the top-left of the bounding box
     - Objects are positioned relative to this top-left corner
   - This reference frame differs from the tile renderer's isometric origin

### The Mismatch

- **Before Fix**: TMJ coordinates were used directly without rebasing to isometric origin
- **Problem**: TMJ coordinates are relative to top-left bounding box, while tiles use isometric origin
- **Result**: Entities appeared offset from their intended positions relative to tiles

## Solution Implemented

### Changes to `TransformObjectPosition()` in `TiledToOlympe.cpp`

The fix rebases TMJ object coordinates by the isometric origin to align with the tile renderer:

1. **Calculate Isometric Origin from Map Bounds**:
   ```cpp
   // Formula matches World::GetIsometricOriginX/Y()
   float isoOriginX = (minTileX_ - minTileY_) * (config_.tileWidth / 2.0f);
   float isoOriginY = (minTileX_ + minTileY_) * (config_.tileHeight / 2.0f);
   ```

2. **Rebase TMJ Coordinates**:
   ```cpp
   // Subtract isometric origin to align reference frames
   float finalX = x + layerOffsetX + tileOffsetX - isoOriginX;
   float finalY = y + layerOffsetY + tileOffsetY - isoOriginY;
   ```

3. **Apply Layer and Tileset Offsets**:
   - Layer offsets (offsetx/offsety) are applied
   - Tileset offsets (tileoffset) for gid-based objects are applied
   - No conversion to tile coordinates is performed

4. **Return Rebased Coordinates**:
   - The returned coordinates are in the same reference frame as the tile renderer
   - Aligns TMJ's top-left bounding box origin with tile renderer's isometric origin at (0,0)

### Key Design Decisions

1. **Bounds-Aware Isometric Origin**: The isometric origin is calculated from the actual map bounds (minTileX, minTileY, maxTileX, maxTileY) rather than map dimensions. This ensures correct alignment for infinite maps and maps with non-zero tile coordinate origins.

2. **No Tile Coordinate Conversion**: Objects remain in pixel coordinates (TMJ format). Only the reference frame is adjusted by subtracting the isometric origin. No conversion to/from tile coordinates is performed.

3. **Consistent with Tile Renderer**: Uses the exact same formula as `World::GetIsometricOriginX/Y()` to ensure perfect alignment between tiles and entities.

4. **Orthogonal Handling Unchanged**: The orthogonal map handling remains exactly as it was, preserving backward compatibility.

5. **Comprehensive Logging**: Added explicit log message:
   ```
   "ISOMETRIC: rebasing TMJ pixels by iso origin (x, y)"
   ```
   This aids debugging and validates the transformation is being applied.

## Mathematical Verification

For an isometric map with bounds minTileX=0, minTileY=0, maxTileX=9, maxTileY=9, tileWidth=58, tileHeight=27:

### Isometric Origin Calculation
- **isoOriginX**: `(0 - 0) * (58 / 2) = 0`
- **isoOriginY**: `(0 + 0) * (27 / 2) = 0`

For a map with offset bounds minTileX=-5, minTileY=-3, maxTileX=4, maxTileY=6:
- **isoOriginX**: `(-5 - (-3)) * (58 / 2) = -2 * 29 = -58`
- **isoOriginY**: `(-5 + (-3)) * (27 / 2) = -8 * 13.5 = -108`

### Example: Entity at TMJ position (2528, 815)
With bounds-aware origin:
- **Before rebasing**: (2528, 815) in TMJ pixels
- **Isometric origin**: calculated from actual map bounds
- **After rebasing**: (2528 - isoOriginX, 815 - isoOriginY)
- **Result**: Aligned with tile renderer coordinate system

The formula ensures that TMJ's top-left bounding box origin aligns with the tile renderer's isometric origin, regardless of the actual tile coordinate bounds.


## Testing Recommendations

1. **Isometric Maps with Standard Bounds**:
   - Load isometric TMJ/TMX files with objects where tiles start at (0, 0)
   - Verify objects appear aligned with their corresponding tiles
   - Check objects with gid (tile objects) respect tileset offsets
   - Verify objects without gid (shapes, polygons) are positioned correctly

2. **Infinite Maps with Non-Zero Bounds**:
   - Test with maps where minTileX/minTileY are negative or non-zero
   - Verify isometric origin calculation correctly handles bounds
   - Confirm entities align properly even with offset coordinate systems

3. **Multi-Layer Maps**:
   - Test with layer offsets (parallax, etc.)
   - Verify offsets are applied correctly to object positions
   - Ensure layering doesn't affect alignment

4. **Orthogonal Maps**:
   - Verify no regression in orthogonal map object placement
   - Confirm behavior remains unchanged from previous implementation

## Files Modified

1. **`Source/TiledLevelLoader/src/TiledToOlympe.cpp`**
   - Updated `TransformObjectPosition()` method (lines 720-760)
   - Added isometric origin calculation from map bounds
   - Added rebasing of TMJ coordinates by subtracting isometric origin
   - Added explicit logging: "ISOMETRIC: rebasing TMJ pixels by iso origin (x,y)"
   - Kept orthogonal handling unchanged

2. **`IMPLEMENTATION_ISOMETRIC_ENTITY_PLACEMENT_FIX.md`** (this file)
   - Updated to reflect bounds-aware isometric origin approach
   - Added mathematical verification with map bounds examples
   - Clarified alignment of TMJ bounding box with iso origin

## Compatibility

✅ **Backward Compatible**:
- Works with all map sizes (infinite and finite)
- Works with all tile dimensions
- Orthogonal maps unchanged
- Handles maps with non-zero minimum tile coordinates
- All renderorder variants supported

✅ **Aligns With**:
- Tile renderer coordinate system (`Source/World.cpp::GetIsometricOriginX/Y()`)
- IsometricProjection utilities
- Existing entity rendering pipeline
- Map bounds metadata (minTileX, minTileY, maxTileX, maxTileY)

## Expected Impact

After this fix:
- ✅ Entities placed in Tiled will appear at the correct positions aligned with tiles
- ✅ Objects align precisely with rendered isometric tiles
- ✅ No more visual offset/scale issues
- ✅ Isometric origin properly accounted for using map bounds
- ✅ Works correctly with infinite maps and non-zero tile coordinates
- ✅ Explicit logging aids debugging and validation

## Implementation Details

### Formula
```cpp
finalX = rawX + layerOffsetX + tileOffsetX - isoOriginX
finalY = rawY + layerOffsetY + tileOffsetY - isoOriginY
```

Where:
- `rawX, rawY`: Original TMJ pixel coordinates
- `layerOffsetX, layerOffsetY`: Layer offset properties from Tiled
- `tileOffsetX, tileOffsetY`: Tileset tile offset (for gid-based objects only)
- `isoOriginX, isoOriginY`: Calculated from map bounds using tile renderer formula

### Log Output
When processing objects in isometric mode, look for:
```
[TRANSFORM] Mode: isometric, Raw TMJ: (x, y), Layer offsets: (ox, oy), GID: gid
  → ISOMETRIC: rebasing TMJ pixels by iso origin (isoX, isoY)
  → Final position (rebased): (finalX, finalY)
```

## Usage Notes

When placing entities in Tiled for isometric maps:
1. Place objects at the desired pixel position in Tiled's isometric view
2. The engine will automatically rebase coordinates to align with tile renderer
3. Check logs with `[TRANSFORM]` and `ISOMETRIC` prefixes to verify transformations
4. Entities will appear aligned with tiles regardless of map bounds

## References

- Tile renderer isometric origin: `Source/World.cpp` lines 1879-1922 (`GetIsometricOriginX()` and `GetIsometricOriginY()`)
- IsometricProjection utilities: `Source/TiledLevelLoader/src/IsometricProjection.cpp`
- Isometric renderer: `Source/Rendering/IsometricRenderer.cpp`
- Map bounds metadata: stored in `LevelDefinition::metadata.customData`
