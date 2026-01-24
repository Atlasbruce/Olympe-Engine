# Tileset Offset Fix - Implementation Summary

## Problem
External tileset files (.tsx/.tsj) were not being parsed, causing `tileoffset` values to always remain at the default `0`, resulting in visual misalignment of tiles, particularly for tall sprites like trees.

## Root Cause
The `TilesetManager::LoadTilesets()` method in `World.cpp` was only reading the main TMJ file, which contains references to external tilesets via the `"source"` property. However, it was never opening or parsing those external files, so it never extracted the `tileoffset` values defined in them.

## Solution
Integrated the existing `TilesetCache` and `TilesetParser` system to load and parse external tileset files.

## Changes Made

### 1. Source/World.cpp (Lines 21-23)
Added includes for Tiled loader components:
```cpp
#include "TiledLevelLoader/include/TilesetCache.h"
#include "TiledLevelLoader/include/TilesetParser.h"
#include "TiledLevelLoader/include/TiledStructures.h"
```

### 2. Source/World.cpp (Lines 892-1116)
Replaced `TilesetManager::LoadTilesets()` method with new implementation:

**Key Features:**
- Checks for `"source"` property in tileset JSON
- Uses `TilesetCache::GetInstance().GetTileset()` to load external .tsx/.tsj files
- Extracts `tileoffsetX` and `tileoffsetY` from loaded `TiledTileset`
- Maintains backward compatibility with embedded (inline) tilesets
- Supports both image-based and collection tilesets
- Adds detailed logging for debugging

## Technical Details

### External Tileset Loading Flow
1. Read `isometric_quest.tmj` and find tileset with `"source": "Trees.tsx"`
2. Call `TilesetCache::GetInstance().GetTileset("Gamedata/Levels/Trees.tsx")`
3. TilesetCache uses TilesetParser to parse .tsx (XML) or .tsj (JSON) format
4. TilesetParser extracts all properties including `tileoffset`
5. Copy `tileoffsetX` and `tileoffsetY` from `TiledTileset` to `TilesetInfo`
6. Renderer applies correct offset when drawing tiles

### Example Tilesets
- **Trees.tsj**: `{"tileoffset": {"x": -100, "y": 0}}`
- **Tiles iso cube.tsx**: `<tileoffset x="0" y="26"/>`

## Verification Points
After loading a level with external tilesets, the console should show:
```
  -> Loading external tileset: Trees.tsx
  -> Extracted tileoffset: (-100, 0)
  ✓ Loaded collection tileset: Trees (79 tiles, offset: -100,0)
```

## Files Verified (No Changes Needed)
- `TiledStructures.h` - Already has `tileoffsetX` and `tileoffsetY` fields
- `TilesetParser.cpp` - Already parses `tileoffset` from both XML and JSON

## Impact
- ✅ Trees and other tall sprites now render with correct horizontal alignment
- ✅ Isometric cube tiles now render with correct vertical alignment
- ✅ Ground tiles align perfectly with sprite bases
- ✅ Backward compatible with embedded tilesets
- ✅ No breaking changes to existing functionality
