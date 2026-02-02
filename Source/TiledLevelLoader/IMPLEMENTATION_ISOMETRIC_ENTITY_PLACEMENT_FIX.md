# Isometric Entity Placement - Minimal Stable Version

## Approach

This document describes the minimal and stable approach for isometric object placement in the Olympe Engine.

**Core Principle**: Isometric objects remain in TMJ pixel coordinates and are NOT rebased. The origin handling is managed on the tile rendering side.

## Implementation

### TransformObjectPosition (Isometric Mode)

The simplified transformation for isometric objects applies only the necessary offsets without any rebasing:

```cpp
// 1) Log raw TMJ coordinates
SYSTEM_LOG << "[TRANSFORM] Raw TMJ (" << x << ", " << y << ")\n";

// 2) Determine tileset offsets for tile objects (gid > 0)
int tileOffsetX = 0;
int tileOffsetY = 0;
if (gid > 0) {
    const TiledTileset* tileset = FindTilesetForGid(gid);
    if (tileset) {
        tileOffsetX = tileset->tileoffsetX;
        tileOffsetY = tileset->tileoffsetY;
    }
}

// 3) Log offsets applied
SYSTEM_LOG << "Offsets applied (layer/tile): (" << layerOffsetX << "/" << tileOffsetX 
          << ", " << layerOffsetY << "/" << tileOffsetY << ")\n";

// 4) Apply layer offsets and tileset offsets to TMJ coordinates
float posX = x + layerOffsetX + tileOffsetX;
float posY = y + layerOffsetY + tileOffsetY;

// 5) Log final position
SYSTEM_LOG << "Final position (" << posX << ", " << posY << ")\n";

return Vector(posX, posY, 0.0f);
```

### Key Changes

1. **NO rebase calculation**: Removed all isometric origin calculations (4-corner projections, CalculateTMJOrigin)
2. **NO coordinate transformations**: No TileToScreen, IsoToWorld, or WorldToIso conversions
3. **Simple offset application**: Only apply layer offsets and tileset offsets
4. **Clean logging**: Three clear log lines showing the transformation process

### Orthogonal Mode

Orthogonal mode remains unchanged and continues to use raw TMJ coordinates with layer offsets only.

## Coordinate System

With this approach:
- **Tiles**: Use their own coordinate system with isometric projection applied during rendering
- **Objects**: Remain in TMJ pixel coordinates without rebasing
- **Origin handling**: Managed entirely on the tile rendering side, not in object placement

This simplification creates a stable, minimal version that avoids coordinate system mismatches.

## Logging

The simplified implementation produces three clean log lines for each object transformation in isometric mode:

```
[TRANSFORM] Raw TMJ (x, y)
Offsets applied (layer/tile): (layerOffsetX/tileOffsetX, layerOffsetY/tileOffsetY)
Final position (posX, posY)
```

This minimal logging shows:
1. The raw coordinates from the TMJ file
2. The offsets being applied (layer and tileset)
3. The final calculated position

## Files Modified

- `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - Simplified `TransformObjectPosition()` function
- `Source/TiledLevelLoader/IMPLEMENTATION_ISOMETRIC_ENTITY_PLACEMENT_FIX.md` - This documentation

## Related Files (Unchanged)

- `Source/TiledLevelLoader/src/IsometricProjection.cpp` - Isometric projection utilities (not used for object placement)
- `Source/TiledLevelLoader/include/IsometricProjection.h` - Header for isometric utilities
- Tile rendering code - Remains unchanged, handles origin on tile side

## Technical Notes

- This approach provides a minimal, stable baseline for isometric object placement
- No conditional logic based on map type or coordinate ranges
- The coordinate transformation is straightforward and predictable
- Tile rendering is not affected and continues to use its own origin handling
