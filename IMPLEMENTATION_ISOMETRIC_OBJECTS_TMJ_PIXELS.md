# Isometric Objects: TMJ Pixel Coordinates (Option A)

## Problem Statement
Previous implementations applied complex coordinate transformations to isometric objects:
- TileToScreen/IsoToWorld conversions
- Inverse isometric projection
- Division by tileWidth/tileHeight to get tile coordinates
- Bounds-aware origin calculations

However, TMJ isometric objects are **already in pixel coordinates** and should not be converted to tile coordinates.

## Root Cause
The implementation treated isometric objects similarly to how tiles are rendered, applying isometric projection and inverse projection to convert between coordinate systems. This was incorrect because:

1. **Tile rendering** works with tile coordinates (worldX, worldY) and projects them to screen space
2. **TMJ objects** are placed in pixel coordinates directly in Tiled's isometric view
3. Objects don't need projection/inverse-projection - they're already in the right coordinate system

## Solution Implemented: Option A

### Changes to `TransformObjectPosition()` in `TiledToOlympe.cpp`

**Before** (lines 720-812, ~93 lines):
- Complex bounds-aware isometric origin calculation
- TileToScreen projections on map corners
- Inverse isometric projection formulas
- Conversion to world tile coordinates

**After** (lines 720-750, ~30 lines):
- Simple addition of offsets
- No coordinate conversion
- No isometric projection
- Objects remain in TMJ pixel coordinates

### New Implementation

```cpp
if (config_.mapOrientation == "isometric")
{
    // ISOMETRIC MODE - OPTION A: Keep objects in TMJ pixel coordinates
    // TMJ isometric objects are already in pixel coordinates and should not be converted.
    // We only apply layer offsets and tileset offsets (for tile objects with gid > 0).
    
    SYSTEM_LOG << "  → ISOMETRIC: Objects remain in TMJ pixel coordinates (no tile coordinate conversion)\n";
    
    // 1) Determine tileset offsets for tile objects (gid > 0)
    int tileOffsetX = 0;
    int tileOffsetY = 0;
    
    if (gid > 0) {
        const TiledTileset* tileset = FindTilesetForGid(gid);
        if (tileset) {
            tileOffsetX = tileset->tileoffsetX;
            tileOffsetY = tileset->tileoffsetY;
            SYSTEM_LOG << "  → Found tileset for gid " << gid 
                      << ": tileOffsetX=" << tileOffsetX
                      << ", tileOffsetY=" << tileOffsetY << "\n";
        }
    }
    
    // 2) Calculate final position: original TMJ pixels + layer offsets + tileset offsets
    float finalX = x + layerOffsetX + tileOffsetX;
    float finalY = y + layerOffsetY + tileOffsetY;
    
    SYSTEM_LOG << "  → Final position in TMJ pixels: (" << finalX << ", " << finalY << ")\n";
    
    return Vector(finalX, finalY, 0.0f);
}
```

## Key Changes

### 1. Removed Complex Calculations
**Removed**:
- `minTileX_`, `maxTileX_`, `minTileY_`, `maxTileY_` bounds calculations
- `IsometricProjection::TileToScreen()` calls on map corners
- `isoOriginX` and `isoOriginY` calculations
- `halfW` and `halfH` calculations
- Inverse isometric projection formulas
- Bottom-center anchor adjustment (`-halfW`)

**Kept**:
- Tileset offset lookup for tile objects (gid > 0)
- Layer offsets
- Clear logging

### 2. Simplified Formula

**Formula**: `finalPosition = TMJ_position + layerOffset + tilesetOffset`

Where:
- `TMJ_position` = (x, y) from Tiled's object placement
- `layerOffset` = (layerOffsetX, layerOffsetY) from layer properties
- `tilesetOffset` = (tileOffsetX, tileOffsetY) from tileset (only if gid > 0)

### 3. Clear Logging

Added explicit log message indicating objects remain in TMJ pixel coordinates:
```
"→ ISOMETRIC: Objects remain in TMJ pixel coordinates (no tile coordinate conversion)"
```

## Coordinate System Clarification

### TMJ Format
In Tiled's isometric view, when you place an object at position (x, y):
- These are **pixel coordinates** in the isometric screen space
- Tiled has already done the layout in isometric view
- No further conversion is needed

### Engine Requirements
The engine expects object positions to match where Tiled placed them:
- Use TMJ pixel coordinates directly
- Apply layer offsets to account for layer positioning
- Apply tileset offsets for tile objects (gid > 0)
- **Do NOT** convert to tile coordinates or apply isometric projection

## Orthogonal Compatibility

The orthogonal code path (lines 752-760) remains **unchanged**:
```cpp
// ORTHOGONAL / HEXAGONAL / STAGGERED MODES:
float finalX = x + layerOffsetX;
float finalY = y + layerOffsetY;
return Vector(finalX, finalY, 0.0f);
```

This maintains backward compatibility with all non-isometric map types.

## Benefits

1. **Simpler Code**: Reduced from ~90 lines to ~30 lines
2. **Clearer Intent**: Code explicitly states objects remain in TMJ pixels
3. **No Unnecessary Conversions**: Removed conversions that introduced errors
4. **Better Logging**: Clear indication of coordinate system being used
5. **Correct Behavior**: Objects positioned exactly where Tiled placed them

## Comparison: Before vs After

### Before (Option B - Inverse Projection)
```
TMJ pixels → subtract origin → apply adjustments → inverse isometric projection → tile coords
(x, y) → (screenX, screenY) → apply tileOffsets → (worldX, worldY) tiles
```

### After (Option A - Keep Pixels)
```
TMJ pixels → add layer offsets → add tileset offsets → final pixels
(x, y) → (x + layerOffsetX + tileOffsetX, y + layerOffsetY + tileOffsetY)
```

## Tile Rendering Unchanged

**Important**: This change affects **only objects/entities**. Tile rendering remains unchanged and continues to use isometric projection for rendering tiles correctly.

## Files Modified

1. **Source/TiledLevelLoader/src/TiledToOlympe.cpp**
   - Modified `TransformObjectPosition()` function (lines 720-750)
   - Simplified isometric branch to keep pixel coordinates
   - Added clear logging indicating TMJ pixel coordinate preservation
   - Removed ~60 lines of complex coordinate conversion code

## Testing Recommendations

1. **Isometric Maps**:
   - Load isometric TMJ/TMX files with objects
   - Verify objects appear where placed in Tiled
   - Check objects with gid (tile objects) respect tileset offsets
   - Verify objects without gid (shapes, polygons) are positioned correctly

2. **Multi-Layer Maps**:
   - Test with layer offsets (parallax, etc.)
   - Verify offsets are applied correctly to object positions

3. **Orthogonal Maps**:
   - Verify no regression in orthogonal map object placement
   - Confirm behavior remains unchanged

## Related Documentation

This fix supersedes the approaches described in:
- `IMPLEMENTATION_ISOMETRIC_ENTITY_FIX.md` (chunk origin, render order approach)
- `IMPLEMENTATION_ISOMETRIC_ENTITY_PLACEMENT_FIX.md` (map origin subtraction approach)
- `IMPLEMENTATION_BOUNDS_AWARE_ISOMETRIC_FIX.md` (bounds-aware origin approach)
- `PR_ISOMETRIC_OBJECT_PLACEMENT_FIX.md` (inverse projection approach)

All of these previous approaches attempted to convert TMJ pixel coordinates through various transformations. This new approach recognizes that TMJ coordinates are already correct and should be preserved.
