# Isometric Entity Placement Fix

## Problem Statement

When loading isometric maps from Tiled (TMJ format), entities were appearing with incorrect positions, offset from their intended locations relative to the tile map. This was caused by a coordinate system mismatch:

- **Tiles**: Rendered using isometric projection with a centered origin calculated from map bounds (via `TileToScreen` of the 4 corners)
- **Entities**: Positioned using raw TMJ pixel coordinates in the top-left reference frame without rebasing

This resulted in entities appearing concentrated far from the tiles, making the map unusable.

## Root Cause

In TMJ files, both tiles and entities (objects) use pixel coordinates in the top-left reference frame of the map's bounding box. However:

1. The tile rendering system calculates an **isometric origin** from the map bounds:
   - `isoOriginX = (minTileX - minTileY) * (tileWidth / 2)`
   - `isoOriginY = (minTileX + minTileY) * (tileHeight / 2)`

2. Tiles are then rendered relative to this isometric origin, effectively centering the coordinate system.

3. Entities were **not** rebased to this isometric origin, remaining in the original TMJ top-left coordinate system.

## Solution

The fix modifies the `TransformObjectPosition` function in `TiledToOlympe.cpp` to:

1. **Keep TMJ pixel coordinates**: Do not convert to tile coordinates or apply `IsoToWorld` transformations
2. **Apply layer offsets**: Add `offsetx`/`offsety` from layer properties
3. **Apply tileset offsets**: For tile objects (gid > 0), add `tileoffset` from tileset definition
4. **Calculate isometric origin**: Use the same formula as tile rendering:
   ```cpp
   float isoOriginX = (minTileX_ - minTileY_) * (config_.tileWidth / 2.0f);
   float isoOriginY = (minTileX_ + minTileY_) * (config_.tileHeight / 2.0f);
   ```
5. **Rebase to isometric origin**: Subtract the isometric origin from the final position:
   ```cpp
   float finalX = posX - isoOriginX;
   float finalY = posY - isoOriginY;
   ```

## Implementation Details

### Modified Function

`TiledToOlympe::TransformObjectPosition()` in `Source/TiledLevelLoader/src/TiledToOlympe.cpp`

### Key Changes

1. Added calculation of isometric origin using map bounds (`minTileX_`, `minTileY_`, `maxTileX_`, `maxTileY_`)
2. Subtracted isometric origin from entity positions in isometric mode
3. Added explicit logging: `"ISOMETRIC: rebasing TMJ pixels by iso origin (x, y)"`
4. Preserved orthogonal mode behavior (no changes to non-isometric code paths)

### Map Bounds

The map bounds are calculated in `TiledToOlympe::Convert()`:

- **Infinite maps**: Bounds are computed by scanning all tile chunks
- **Finite maps**: Bounds are `[0, width-1]` x `[0, height-1]`

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
  → ISOMETRIC: rebasing TMJ pixels by iso origin (isoX, isoY)
  → Final position after rebase: (finalX, finalY)
```

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
