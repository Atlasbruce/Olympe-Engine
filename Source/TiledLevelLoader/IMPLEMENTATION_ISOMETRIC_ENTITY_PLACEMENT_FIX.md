# Isometric Entity Placement - Final Working Solution

## Status: ? SOLVED

This document describes the correct and verified solution for converting Tiled TMJ isometric object coordinates to Olympe Engine world coordinates.

## The Problem

Tiled stores object positions in TMJ files using a special isometric coordinate system that differs from standard pixel coordinates. Direct use of these coordinates resulted in entities being misaligned with tiles.

## The Solution

### Key Discovery

**Tiled stores isometric object positions where BOTH X and Y are measured in `tileHeight` pixel units along the isometric axes.**

This is different from orthogonal maps where X and Y are direct pixel coordinates.

### Conversion Formula

```cpp
// Step 1: Convert TMJ pixel coords to tile coords
// CRITICAL: Both X and Y are divided by tileHeight (Tiled's convention)
tileX = tmjPixelX / tileHeight
tileY = tmjPixelY / tileHeight

// Step 2: Apply standard isometric projection
worldX = (tileX - tileY) * (tileWidth / 2)
worldY = (tileX + tileY) * (tileHeight / 2)
```

### Verified Example

**Map Configuration:**
- Map size: 184x128 tiles
- Tile size: 58x27 pixels (halfWidth=29, halfHeight=13.5)

**player_1 Conversion:**
```
TMJ coordinates: (1818.4, 1064.26)
tileX = 1818.4 / 27 = 67.35
tileY = 1064.26 / 27 = 39.42
worldX = (67.35 - 39.42) * 29 = 810
worldY = (67.35 + 39.42) * 13.5 = 1441
Result: Entity renders at tile (67, 39) ?
```

## Implementation

### TransformObjectPosition (TiledToOlympe.cpp)

```cpp
Vector TiledToOlympe::TransformObjectPosition(float x, float y, 
    float layerOffsetX, float layerOffsetY, uint32_t gid)
{
    if (config_.mapOrientation == "isometric")
    {
        const float tileWidth = static_cast<float>(config_.tileWidth);
        const float tileHeight = static_cast<float>(config_.tileHeight);
        const float halfWidth = tileWidth * 0.5f;
        const float halfHeight = tileHeight * 0.5f;
        
        // Convert TMJ pixel coords to tile coords (both use tileHeight!)
        const float tileX = (tileHeight != 0.0f) ? (x / tileHeight) : 0.0f;
        const float tileY = (tileHeight != 0.0f) ? (y / tileHeight) : 0.0f;
        
        // Standard isometric projection
        float worldX = (tileX - tileY) * halfWidth;
        float worldY = (tileX + tileY) * halfHeight;
        
        // Apply layer offsets (also in isometric pixel space)
        if (layerOffsetX != 0.0f || layerOffsetY != 0.0f) {
            float layerTileX = layerOffsetX / tileHeight;
            float layerTileY = layerOffsetY / tileHeight;
            worldX += (layerTileX - layerTileY) * halfWidth;
            worldY += (layerTileX + layerTileY) * halfHeight;
        }
        
        // Apply tileset offsets for tile objects (gid > 0)
        if (gid > 0) {
            const TiledTileset* tileset = FindTilesetForGid(gid);
            if (tileset) {
                worldX += static_cast<float>(tileset->tileoffsetX);
                worldY += static_cast<float>(tileset->tileoffsetY);
            }
        }

        return Vector(worldX, worldY, 0.0f);
    }

    // Orthogonal: direct pixel coordinates with layer offset
    return Vector(x + layerOffsetX, y + layerOffsetY, 0.0f);
}
```

## Why Both Divided by tileHeight?

In Tiled's isometric view, the X and Y axes run diagonally. Movement along either axis covers the same diagonal distance on screen. Tiled normalizes this by using `tileHeight` as the unit for BOTH axes, making the coordinate system uniform along both isometric directions.

## No Origin Offset Needed

The `originX` calculation (mapHeight * halfTileWidth) that Tiled uses is for **screen display only**. In our engine, both tiles and objects use the same world coordinate system where tile (0,0) is at world (0,0). The camera handles screen positioning.

## Files Modified

- `Source/TiledLevelLoader/src/TiledToOlympe.cpp` - `TransformObjectPosition()` function
- `Source/TiledLevelLoader/src/IsometricProjection.cpp` - Documentation and utilities
- `Source/TiledLevelLoader/include/IsometricProjection.h` - Header documentation
- `Source/TiledLevelLoader/include/TiledToOlympe.h` - Header documentation

## Debug Logging

The implementation includes debug logging:
```
[TransformObjectPosition] ISO: TMJ(1818.4, 1064.26) -> tile(67.35, 39.42) -> world(810, 1441)
```

## Summary

| Step | Operation | Formula |
|------|-----------|---------|
| 1 | TMJ ? Tile | `tileX = tmjX / tileHeight`, `tileY = tmjY / tileHeight` |
| 2 | Tile ? World | `worldX = (tileX - tileY) * halfWidth`, `worldY = (tileX + tileY) * halfHeight` |
| 3 | Offsets | Add layer offsets (converted to isometric) and tileset offsets |

**Result: Entities align perfectly with tiles as placed in Tiled Editor.**
