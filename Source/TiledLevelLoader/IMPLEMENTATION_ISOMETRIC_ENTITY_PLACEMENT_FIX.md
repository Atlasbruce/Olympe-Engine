# Isometric Entity Placement Fix (v3 - Corrected Sign for TMJ Origin)

## Problem Statement

When loading isometric maps from Tiled (TMJ format), entities were appearing with incorrect positions, offset from their intended locations relative to the tile map. This was caused by a coordinate system mismatch and an incorrect sign in the origin calculation:

- **Tiles**: Rendered using standard isometric projection (worldToIso) starting from tile coordinates
- **Entities**: Positioned using TMJ pixel coordinates, which use Tiled's coordinate system where the north corner of tile (0,0) is offset in screen space
- **Previous Fix Issue (v1)**: The original formula `isoOriginX = (minTileX - minTileY) * (tileWidth / 2)` resulted in (0,0) for finite maps, causing no rebasing effect
- **Previous Fix Issue (v2)**: The 4-corner calculation produced correct minimum values but with wrong sign, e.g., `isoOriginX = -3683` resulted in `finalX = posX - (-3683) = posX + 3683`, moving entities away instead of toward their correct position

This resulted in entities appearing offset from their tile positions, especially noticeable in infinite maps with negative tile coordinates.

## Root Cause

In TMJ files, both tiles and entities (objects) use pixel coordinates, but in different coordinate systems:

1. **Tile rendering** uses standard isometric projection:
   - `screenX = (tileX - tileY) * (tileWidth / 2)`
   - `screenY = (tileX + tileY) * (tileHeight / 2)`
   - For tile (0,0): projects to screen (0, 0)

2. **Tiled's TMJ coordinate system** positions the north corner of tile (0,0) at:
   - `(mapHeight * tileWidth / 2, 0)` in TMJ pixel coordinates
   - This creates an offset between Tiled's coordinate system and the engine's rendering system

3. **Sign error in v2**: When calculating the 4 corners and finding the minimum:
   - For an infinite map with negative tiles, `minX` could be negative (e.g., -3683)
   - Using `isoOriginX = minX = -3683` directly, then `finalX = posX - isoOriginX = posX + 3683`
   - This moves entities in the WRONG direction (right instead of left)

## Solution (v3)

The corrected fix calculates the TMJ origin by projecting all 4 map corners, finding the minimum X/Y, and then **negating** these values:

1. **Keep TMJ pixel coordinates**: Do not convert to tile coordinates or apply `IsoToWorld` transformations
2. **Apply layer offsets**: Add `offsetx`/`offsety` from layer properties
3. **Apply tileset offsets**: For tile objects (gid > 0), add `tileoffset` from tileset definition
4. **Calculate TMJ origin from 4 corners with correct sign**:
   ```cpp
   // Project 4 corners using standard isometric projection
   float northX = (minTileX_ - minTileY_) * halfTileWidth;
   float eastX = (maxTileX_ - minTileY_) * halfTileWidth;
   float westX = (minTileX_ - maxTileY_) * halfTileWidth;
   float southX = (maxTileX_ - maxTileY_) * halfTileWidth;
   
   // Find min X/Y (top-left of bounding box)
   float minX = min(northX, eastX, westX, southX);
   float minY = min(northY, eastY, westY, southY);
   
   // Negate to define the origin offset
   float tmjOriginX = -minX;
   float tmjOriginY = -minY;
   ```
5. **Rebase to world coordinates**: Subtract the TMJ origin from entity positions:
   ```cpp
   float finalX = posX - tmjOriginX;
   float finalY = posY - tmjOriginY;
   ```

## Implementation Details

### Modified Functions

1. `IsometricProjection::CalculateTMJOrigin()` in `Source/TiledLevelLoader/src/IsometricProjection.cpp`
2. `TiledToOlympe::TransformObjectPosition()` in `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

### Key Changes (v3)

1. **Sign correction in CalculateTMJOrigin**:
   - Now returns `outOriginX = -minX` and `outOriginY = -minY`
   - This ensures that `finalX = posX - tmjOriginX` moves entities in the correct direction

2. **For an infinite map with negative tiles** (e.g., tiles -10 to 10):
   - West corner (-10, 10): projects to (-640, 0) assuming 64x32 tiles
   - minX = -640
   - tmjOriginX = -(-640) = 640
   - finalX = posX - 640 ← Correctly subtracts the offset
   - **Previously (v2)**: tmjOriginX = -640, finalX = posX + 640 ❌

3. **Enhanced logging** to show all 4 corner calculations explicitly:
   ```
   → ISOMETRIC: Calculating TMJ origin from 4 corners:
      North corner (minX,minY) -> (nx, ny)
      East corner  (maxX,minY) -> (ex, ey)
      West corner  (minX,maxY) -> (wx, wy)
      South corner (maxX,maxY) -> (sx, sy)
      TMJ Origin (negative of min): (originX, originY)
   ```

4. **Preserved orthogonal mode** behavior (no changes to non-isometric code paths)

### Coordinate System Formula

The key insight is that Tiled's TMJ coordinate system is offset from the standard isometric projection:

- **Standard isometric**: Tile (0,0) → screen (0, 0)
- **Tiled TMJ**: Tile (0,0) → screen (mapHeight * tileWidth / 2, 0)

By calculating the minimum corner projection and negating it, we effectively define the offset needed to align Tiled's coordinate system with our rendering system.

### Map Bounds

The map bounds are calculated in `TiledToOlympe::Convert()`:

- **Infinite maps**: Bounds are computed by scanning all tile chunks
  - `minTileX_`, `minTileY_`: Minimum tile coordinates found
  - `maxTileX_`, `maxTileY_`: Maximum tile coordinates found
- **Finite maps**: Bounds are set from map dimensions
  - `minTileX_ = 0`, `minTileY_ = 0`
  - `maxTileX_ = width - 1`, `maxTileY_ = height - 1`

These bounds are stored in member variables:
- `minTileX_`, `minTileY_`: Minimum tile coordinates
- `maxTileX_`, `maxTileY_`: Maximum tile coordinates

### Coordinate System Alignment

After the fix, both tiles and entities use the same coordinate system:

```
TMJ Pixel Coords → Apply Offsets → Rebase to Iso Origin → Final Position
```

This ensures that:
- An entity placed at a specific tile in Tiled editor appears at that same tile in the engine
- The relationship between entities and tiles matches the Tiled editor preview
- Orthogonal maps continue to work correctly (no rebasing applied)

## Verification

To verify the fix works correctly:

1. **Isometric maps**: Entities should appear at their Tiled editor positions relative to tiles
2. **Orthogonal maps**: No regression - entities should still appear correctly
3. **Infinite isometric maps**: Entity positions should account for negative tile coordinates correctly
4. **Layer offsets**: Entities on offset layers should respect those offsets
5. **Tile objects**: Tile-based entities (gid > 0) should respect tileset offsets

## Logging

The fix adds detailed logging for debugging:

```
[TRANSFORM] Mode: isometric, Raw TMJ: (x, y), Layer offsets: (ox, oy), GID: gid
  → ISOMETRIC: Processing TMJ pixel coordinates
  → Position after offsets: (x', y')
  → ISOMETRIC: Calculating TMJ origin from 4 corners:
     North corner (minX,minY) -> (nx, ny)
     East corner  (maxX,minY) -> (ex, ey)
     West corner  (minX,maxY) -> (wx, wy)
     South corner (maxX,maxY) -> (sx, sy)
     TMJ Origin (negative of min): (originX, originY)
  → Final position after rebase: (finalX, finalY)
```

**Key improvement**: The logs now explicitly show all 4 corner positions and that the origin is the "negative of min", making it clear that the sign correction is applied.

## Technical Notes

- The isometric origin formula matches `World::GetIsometricOriginX/Y()` used by tile rendering
- The fix does NOT modify tile rendering - only entity placement
- The same coordinate transformation is applied consistently to all entity types:
  - Regular entities
  - Collision objects
  - Sectors (polygon objects)
  - Patrol paths (polyline objects)
- Polygon/polyline vertex coordinates remain relative to the entity origin (not rebased individually)

## Related Files

- `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - Entity position transformation
- `Source/TiledLevelLoader/src/IsometricProjection.cpp` - TMJ origin calculation with sign correction
- `Source/TiledLevelLoader/include/IsometricProjection.h` - Isometric projection utilities
- `Examples/TiledLoader/example_isometric.cpp` - Isometric usage example
