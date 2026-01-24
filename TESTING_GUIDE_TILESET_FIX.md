# Testing Guide: Tileset Offset Fix

## Overview
This fix addresses the critical issue where external tileset files were not being parsed, causing tileoffset to always be 0.

## What Was Changed
1. `Source/World.cpp` - Added TilesetCache integration to parse external .tsx/.tsj files
2. `TILESET_OFFSET_FIX.md` - Implementation documentation

## How to Test

### Prerequisites
- Build the project successfully
- Have test levels with external tilesets (e.g., `isometric_quest.tmj`)

### Step 1: Compile the Project
```bash
# On Windows with Visual Studio
msbuild "Olympe Engine.sln" /p:Configuration=Debug

# Or with CMake
mkdir build && cd build
cmake .. -DSDL3_DIR=../SDL/cmake
cmake --build .
```

### Step 2: Run the Game Engine
```bash
# Run the engine
./OlympeEngine
# or on Windows
OlympeEngine.exe
```

### Step 3: Load a Level with External Tilesets
Load the `isometric_quest.tmj` level which uses external tilesets:
- Trees.tsx (offset: -100, 0)
- Tiles iso cube.tsx (offset: 0, 26)
- tiles-iso-1.tsx

### Step 4: Check Console Output
Look for these messages in the console:

✅ **Expected Output:**
```
  -> Loading external tileset: tiles-iso-1.tsx
  -> Extracted tileoffset: (0, 0)
  ✓ Loaded tileset texture: tiles-iso-1.png (gid: 1-108, offset: 0,0)
  
  -> Loading external tileset: Tiles iso cube.tsx
  -> Extracted tileoffset: (0, 26)
  ✓ Loaded tileset texture: tiles-iso-set3.png (gid: 109-126, offset: 0,26)
  
  -> Loading external tileset: Trees.tsx
  -> Extracted tileoffset: (-100, 0)
  ✓ Loaded collection tileset: Trees (79 tiles, offset: -100,0)
```

❌ **Bad Output (Old Behavior):**
```
  ✓ Loaded tileset texture: ... (offset: 0,0)  # Always shows 0,0
```

### Step 5: Visual Verification
Compare the visual rendering:

**Before Fix:**
- Trees appear shifted 100px to the RIGHT
- Horizontal misalignment between trees and ground tiles
- Isometric cube tiles not properly aligned vertically

**After Fix:**
- Trees properly aligned horizontally (100px to the LEFT)
- Perfect alignment between ground tiles and tree bases
- Isometric cubes properly aligned (26px DOWN)
- No visual artifacts or gaps

### Step 6: Test Different Map Types
Test with both orthogonal and isometric maps:
- `parallax-forest.tmj` (orthogonal)
- `isometric_quest.tmj` (isometric)

### Step 7: Test Backward Compatibility
Verify that maps with embedded tilesets (no "source" property) still work correctly.

## Expected Results

### Console Logs
✅ Each external tileset shows "Loading external tileset" message
✅ Each tileset shows "Extracted tileoffset" with correct values
✅ Collection tilesets show correct tile count and offset
✅ Image-based tilesets show correct gid range and offset

### Visual Results
✅ Trees render at correct horizontal position (-100 offset)
✅ Isometric cubes align properly (+26 vertical offset)
✅ Ground tiles perfectly aligned with tall sprite bases
✅ No gaps or overlaps in tile rendering
✅ Smooth scrolling without visual glitches

### Performance
✅ No noticeable performance degradation
✅ TilesetCache prevents redundant parsing
✅ Loading time similar to before (external files cached)

## Known Test Files

### Tileset Files to Check
1. **Gamedata/Levels/Trees.tsj**
   - Format: JSON
   - Expected offset: x=-100, y=0
   - Type: Collection tileset (79 individual tiles)

2. **Gamedata/Levels/Tiles iso cube.tsx**
   - Format: XML
   - Expected offset: x=0, y=26
   - Type: Image-based tileset (18 tiles)

3. **Gamedata/Levels/tiles-iso-1.tsx**
   - Format: XML
   - Expected offset: x=0, y=0
   - Type: Image-based tileset

### Map Files to Test
1. **Gamedata/Levels/isometric_quest.tmj**
   - Uses all three external tilesets above
   - Best file for comprehensive testing

2. **Gamedata/Levels/parallax-forest.tmj**
   - Uses parallax_forest.tsx
   - Tests orthogonal map with external tilesets

## Troubleshooting

### Issue: "Failed to load external tileset"
**Cause:** File not found or incorrect path
**Solution:** Check that tileset files exist in Gamedata/Levels/ directory

### Issue: Offset still shows (0, 0)
**Cause:** Tileset file doesn't have tileoffset property
**Solution:** Verify the .tsx/.tsj file contains `<tileoffset>` or `"tileoffset"` property

### Issue: Visual alignment still wrong
**Cause:** Renderer not applying offset correctly
**Solution:** Check ECS_Systems.cpp lines 534-538 and 574-578

### Issue: Build errors
**Cause:** Missing includes or wrong paths
**Solution:** Verify TiledLevelLoader is properly linked in CMakeLists.txt

## Success Criteria

✅ All external tilesets load successfully
✅ Console shows correct tileoffset values
✅ Trees render with -100 horizontal offset
✅ Isometric cubes render with +26 vertical offset
✅ No visual misalignment in isometric_quest.tmj
✅ Backward compatibility maintained
✅ No performance regression
✅ No new compiler warnings or errors

## Reporting Issues

If testing reveals issues, please report:
1. Console output (full log)
2. Screenshot showing visual problem
3. Map file being tested
4. Tileset file contents
5. Build configuration (Debug/Release)
6. Platform (Windows/Linux/Mac)
