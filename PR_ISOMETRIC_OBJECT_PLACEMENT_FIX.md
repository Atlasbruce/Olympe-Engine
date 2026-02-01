# Isometric Object Placement Fix - Implementation Summary

## Overview
This PR implements proper isometric object placement with correct TMX alignment and tileset offsets, ensuring objects are correctly positioned in isometric maps while maintaining backward compatibility with orthogonal, hexagonal, and staggered map orientations.

## Problem Statement
Previously, isometric objects were kept in raw TMJ screen coordinates with an origin offset applied during tile rendering. This approach had several issues:
1. Objects were not properly aligned according to TMX standards (bottom-center anchor for tile objects)
2. Tileset-specific tile offsets were not applied to object placement
3. Objects and tiles were in different coordinate spaces, causing alignment issues

## Solution
The fix transforms isometric objects from TMJ screen coordinates to world isometric coordinates using the inverse isometric projection, applying proper TMX alignment rules and tileset offsets in the process.

## Changes Made

### 1. TiledToOlympe.h
**Added:**
- `tilesets_` member variable to store tileset reference during conversion
- `FindTilesetForGid()` helper method to look up tileset information for a given gid
- Updated `TransformObjectPosition()` signature to accept `gid` parameter (default 0)

### 2. TiledToOlympe.cpp

#### FindTilesetForGid() Implementation
```cpp
const TiledTileset* TiledToOlympe::FindTilesetForGid(uint32_t gid) const
```
- Strips flip flags from gid to get actual tile ID
- Searches through ordered tilesets to find the tileset that owns the gid
- Returns tileset pointer or nullptr if not found

#### TransformObjectPosition() Implementation
For isometric mode, the function now:

**1. Determines tileset info for object gid (if gid > 0)**
- Looks up tileset using `FindTilesetForGid()`
- Extracts `tileWidth`, `tileHeight`, `tileoffsetX`, `tileoffsetY` from tileset
- Falls back to map tile dimensions if no gid or tileset not found

**2. Applies TMX alignment rules:**
- Objects with gid in isometric are anchored bottom-center
- Adjusts X coordinate by `-(tileWidth/2)` for bottom-center anchor
- Applies tileset `tileoffsetX` and `tileoffsetY` to object position
- Also applies layer offsets (`layerOffsetX`, `layerOffsetY`)

**3. Converts TMJ screen coords to world iso coordinates:**
Uses inverse isometric projection formulas:
```
screenX = x + layerOffsetX + tileOffsetX - halfW  (bottom-center anchor)
screenY = y + layerOffsetY + tileOffsetY

worldX = (screenX/halfW + screenY/halfH) / 2
worldY = (screenY/halfH - screenX/halfW) / 2
```

**4. Adds comprehensive logging:**
- Logs gid, tileWidth/Height, tileoffset values
- Logs screenX/screenY after alignment
- Logs final worldX/worldY coordinates

#### Updated Call Sites
Updated all `TransformObjectPosition()` calls to pass `obj.gid`:
- `ConvertSectorObject()`
- `ConvertPolygonCollision()`
- `ConvertPatrolPath()`
- `ParseEntityDescriptor()`

#### Convert() Method
- Stores `tilesets` reference at start of conversion: `tilesets_ = &tiledMap.tilesets;`

### 3. ECS_Systems.cpp

#### RenderTileImmediate() Changes
**Removed isometric origin offset application:**
- Previously: tiles had origin offset applied to align with TMJ object coordinates
- Now: tiles use standard isometric projection without origin offset
- Reason: Objects are now converted to world iso coordinates, so tiles and objects share the same coordinate system

```cpp
// OLD (removed):
float originOffsetX = World::Get().GetIsometricOriginX();
float originOffsetY = World::Get().GetIsometricOriginY();
worldPos = Vector(isoX + originOffsetX, isoY + originOffsetY, 0.0f);

// NEW:
worldPos = Vector(isoX, isoY, 0.0f);
```

## Coordinate System Details

### Before (Previous Implementation)
- **Objects:** Raw TMJ screen coordinates + layer offsets
- **Tiles:** World iso coordinates + origin offset
- **Issue:** Objects and tiles in different coordinate spaces

### After (This Fix)
- **Objects:** World iso coordinates (converted from TMJ via inverse projection)
- **Tiles:** World iso coordinates (standard isometric projection)
- **Result:** Objects and tiles in same coordinate space, proper alignment

### Isometric Projection Formulas
**Forward (tile -> screen):**
```
screenX = (worldX - worldY) * (tileWidth / 2)
screenY = (worldX + worldY) * (tileHeight / 2)
```

**Inverse (screen -> tile):**
```
worldX = (screenX / halfW + screenY / halfH) / 2
worldY = (screenY / halfH - screenX / halfW) / 2
```

## Backward Compatibility

### Non-Isometric Modes (Orthogonal, Hexagonal, Staggered)
- **Unchanged:** `TransformObjectPosition()` returns `x + layerOffsetX, y + layerOffsetY` for non-isometric modes
- **Backward Compatible:** All existing behavior preserved for orthogonal, hex, and staggered maps
- **Tile Rendering:** No changes to orthogonal or hexagonal tile rendering paths

## Testing Recommendations

### Isometric Maps
1. Create/load isometric map with tile objects (gid > 0)
2. Verify objects align correctly at bottom-center of tiles
3. Test with tilesets that have tile offsets (tileoffsetX/Y)
4. Verify objects without gid still work correctly
5. Check multi-layer maps with layer offsets

### Non-Isometric Maps
1. Load orthogonal maps and verify object placement unchanged
2. Load hexagonal maps and verify object placement unchanged
3. Verify staggered maps (if supported) work correctly

### Logging Output
The implementation includes detailed logging that shows:
- Object gid and TMJ coordinates
- Tileset info (tileWidth, tileHeight, tileOffsets)
- Screen coordinates after alignment
- Final world iso coordinates

Enable `SYSTEM_LOG` to see transformation details.

## Benefits
1. **Correct TMX Compliance:** Objects follow TMX standard bottom-center anchor
2. **Tileset Offset Support:** Properly applies per-tileset tile offsets
3. **Unified Coordinate System:** Tiles and objects in same world space
4. **Better Debugging:** Comprehensive logging for troubleshooting
5. **Backward Compatible:** No changes to non-isometric modes
6. **Maintainable:** Clear, documented code with proper separation of concerns

## Files Changed
- `Source/TiledLevelLoader/include/TiledToOlympe.h`
- `Source/TiledLevelLoader/src/TiledToOlympe.cpp`
- `Source/ECS_Systems.cpp`

## Related Documentation
- TMX Map Format: https://doc.mapeditor.org/en/stable/reference/tmx-map-format/
- Isometric Projection: https://en.wikipedia.org/wiki/Isometric_projection
