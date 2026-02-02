# Isometric Entity Placement Fix (v2 - Corrected TMJ Origin Calculation)

## Problem Statement

When loading isometric maps from Tiled (TMJ format), entities were appearing with incorrect positions, offset from their intended locations relative to the tile map. This was caused by a coordinate system mismatch:

- **Tiles**: Rendered using standard isometric projection (worldToIso) starting from tile coordinates
- **Entities**: Positioned using TMJ pixel coordinates, which use Tiled's coordinate system where the north corner of tile (0,0) is offset in screen space
- **Previous Fix Issue**: The original formula `isoOriginX = (minTileX - minTileY) * (tileWidth / 2)` resulted in (0,0) for finite maps, causing no rebasing effect

This resulted in entities appearing offset from their tile positions, especially noticeable in finite maps.

## Root Cause

In TMJ files, both tiles and entities (objects) use pixel coordinates, but in different coordinate systems:

1. **Tile rendering** uses standard isometric projection:
   - `screenX = (tileX - tileY) * (tileWidth / 2)`
   - `screenY = (tileX + tileY) * (tileHeight / 2)`
   - For tile (0,0): projects to screen (0, 0)

2. **Tiled's TMJ coordinate system** positions the north corner of tile (0,0) at:
   - `(mapHeight * tileWidth / 2, 0)` in TMJ pixel coordinates
   - This creates an offset between Tiled's coordinate system and the engine's rendering system

3. **Previous formula limitation**: For finite maps where `minTileX = minTileY = 0`:
   - `isoOriginX = (0 - 0) * (tileWidth / 2) = 0`
   - This resulted in NO rebasing, leaving entities offset

## Solution (v2)

The corrected fix calculates the TMJ origin by projecting all 4 map corners and finding the minimum X/Y (top-left of bounding box):

1. **Keep TMJ pixel coordinates**: Do not convert to tile coordinates or apply `IsoToWorld` transformations
2. **Apply layer offsets**: Add `offsetx`/`offsety` from layer properties
3. **Apply tileset offsets**: For tile objects (gid > 0), add `tileoffset` from tileset definition
4. **Calculate TMJ origin from 4 corners**: Project all corners using `TileToScreen` and find min X/Y:
   ```cpp
   // Project 4 corners using standard isometric projection
   float northX = (minTileX_ - minTileY_) * halfTileWidth;
   float eastX = (maxTileX_ - minTileY_) * halfTileWidth;
   float westX = (minTileX_ - maxTileY_) * halfTileWidth;
   float southX = (maxTileX_ - maxTileY_) * halfTileWidth;
   
   // Find min X/Y (top-left of bounding box)
   float isoOriginX = min(northX, eastX, westX, southX);
   float isoOriginY = min(northY, eastY, westY, southY);
   ```
5. **Rebase to world coordinates**: Subtract the TMJ origin from entity positions:
   ```cpp
   float finalX = posX - isoOriginX;
   float finalY = posY - isoOriginY;
   ```

## Implementation Details

### Modified Function

`TiledToOlympe::TransformObjectPosition()` in `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

### Key Changes (v2)

1. **Calculates TMJ origin from 4 map corners** instead of using only min tile coordinates:
   - Projects north, east, west, and south corners using standard isometric formula
   - Finds minimum X and Y values as the TMJ origin (top-left of bounding box)
   - This works for both finite maps (0,0 to width-1,height-1) and infinite maps (arbitrary bounds)

2. **For a finite 10x10 map** (tiles 0-9 in each dimension):
   - West corner (0, 9): projects to (-288, 144)
   - North corner (0, 0): projects to (0, 0)
   - TMJ Origin: (-288, 0) ← This is the top-left of the bounding box
   - **Previously**: Origin was (0, 0) causing no rebasing effect
   - **Now**: Origin is correctly calculated, enabling proper alignment

3. **Enhanced logging** to show all 4 corner calculations and the resulting origin

4. **Preserved orthogonal mode** behavior (no changes to non-isometric code paths)

### Coordinate System Formula

The key insight is that Tiled's TMJ coordinate system is offset from the standard isometric projection:

- **Standard isometric**: Tile (0,0) → screen (0, 0)
- **Tiled TMJ**: Tile (0,0) → screen (mapHeight * tileWidth / 2, 0)

By calculating the minimum corner projection, we effectively find where Tiled's (0,0) TMJ coordinate is in our coordinate system, then subtract it to align everything.

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
3. **Infinite isometric maps**: Entity positions should account for negative tile coordinates
4. **Layer offsets**: Entities on offset layers should respect those offsets
5. **Tile objects**: Tile-based entities (gid > 0) should respect tileset offsets

## Logging

The fix adds detailed logging for debugging:

```
[TRANSFORM] Mode: isometric, Raw TMJ: (x, y), Layer offsets: (ox, oy), GID: gid
  → ISOMETRIC: Processing TMJ pixel coordinates
  → Position after offsets: (x', y')
  → ISOMETRIC: Calculated TMJ origin from 4 corners:
     North (minX,minY) -> (nx, ny)
     East  (maxX,minY) -> (ex, ey)
     West  (minX,maxY) -> (wx, wy)
     South (maxX,maxY) -> (sx, sy)
     TMJ Origin (min): (isoX, isoY)
  → Final position after rebase: (finalX, finalY)
```

**Key improvement**: The logs now show the actual calculated origin values, which will be non-zero for finite maps like `isometric_quest_solo`, confirming the rebasing is working.

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
- `Source/World.cpp` - Isometric origin calculation for tile rendering
- `Source/TiledLevelLoader/include/IsometricProjection.h` - Isometric projection utilities
- `Examples/TiledLoader/example_isometric.cpp` - Isometric usage example
