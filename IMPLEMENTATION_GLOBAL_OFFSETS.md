# Global Offset Implementation Summary

## Problem Statement

Entities such as "Beacon Far East" and "Beacon Far South" were affected by a consistent global offset. While earlier fixes applied partial corrections (such as layer offset support), additional adjustments were required for full accuracy in alignment.

**Observed behavior**: Entities were positioned relative to tile (0, 0) but were displaced consistently by a global offset, causing them to appear shifted relative to the expected positions.

## Solution Implemented

### 1. Global Offset Infrastructure (Phase 1)

**Added global offset fields to TiledToOlympe class:**
- `globalOffsetX_` - Global X offset applied after isometric projection
- `globalOffsetY_` - Global Y offset applied after isometric projection

These fields default to 0.0f and provide a mechanism for applying consistent corrections to all entity positions if needed.

### 2. Enhanced Coordinate Transformation Functions

**Extended IsometricProjection::WorldToIso and IsoToWorld:**
- Added `globalOffsetX` and `globalOffsetY` parameters
- Applied after isometric projection in WorldToIso: `result.x += globalOffsetX; result.y += globalOffsetY`
- Removed before inverse projection in IsoToWorld: `adjIsoX -= globalOffsetX; adjIsoY -= globalOffsetY`
- Maintains mathematical correctness with proper round-trip conversion
- Default parameter values ensure backward compatibility

### 3. Complete Transformation Pipeline in TransformObjectPosition

Implemented a proper 5-step transformation pipeline:

```cpp
// Step 1: Apply layer pixel offsets
float adjustedX = x + layerOffsetX;
float adjustedY = y + layerOffsetY;

// Step 2: Convert TMJ pixels to tile coordinates
float tileX = adjustedX / (tileWidth / 2.0f);
float tileY = adjustedY / tileHeight;

// Step 3: Translate to chunk coordinate system
tileX -= chunkOriginX_;
tileY -= chunkOriginY_;

// Step 4: Apply render order transformation (Y-flip for "up" render orders)
if (renderOrder == "left-up" || renderOrder == "right-up") {
    tileY = -tileY;
}

// Step 5: Apply isometric projection with global offsets
Vector isoPos = IsometricProjection::WorldToIso(
    tileX, tileY, tileWidth, tileHeight,
    0, 0, 0.0f, 0.0f,  // startX, startY, offsetX, offsetY
    globalOffsetX_, globalOffsetY_
);
```

### 4. Enhanced Logging

Added comprehensive logging that triggers when:
- Coordinates are far from origin (threshold-based)
- Global offsets are non-zero
- Chunk origin offsets are non-zero

Logs show each transformation step:
1. Input TMJ coordinates
2. After layer offsets
3. Tile coordinates
4. After chunk origin adjustment
5. After render order Y-flip (if applicable)
6. Final isometric position
7. Total difference from input

## Key Improvements Over Previous Implementation

### Missing Transformations Now Applied:

1. **Chunk Origin Offset**: Previous implementation did not subtract `chunkOriginX_` and `chunkOriginY_` from tile coordinates
   - Critical for infinite maps where chunks start at negative tile coordinates (e.g., -16, -16)
   - Entities must be aligned with the chunk coordinate system

2. **Render Order Transformation**: Previous implementation did not apply Y-flip for "up" render orders
   - For "left-up" and "right-up" render orders, Y-axis must be inverted
   - Tiled's Y-axis points down (screen space) but isometric Y-axis points up (world space)

3. **Global Offset Support**: Infrastructure now exists to apply consistent corrections
   - Can be adjusted at runtime if minor systematic errors are observed
   - Applied after all other transformations for clean separation of concerns

## Test Results

Created `test_global_offsets.cpp` to verify mathematical correctness:
- **13/13 tests passed**
- Verified round-trip conversion: World → Iso → World
- Tested all offset combinations (layer, chunk origin, global)
- Tested beacon entity coordinates with chunk origin adjustments

Created `test_beacon_positions.cpp` to analyze actual beacon transformations:
- Uses actual map parameters (tile size 58x27, chunk origin -16,-16)
- Shows complete transformation pipeline for each beacon
- Demonstrates proper chunk origin and Y-flip application

## Map Parameters (isometric_quest.tmj)

- **Orientation**: isometric
- **Render Order**: left-up (requires Y-flip)
- **Tile Size**: 58x27 pixels
- **Chunk Origin**: (-16, -16) for infinite map
- **Infinite**: true

## Beacon Entity Positions in TMJ

- **Beacon Far East**: (3078, -378)
- **Beacon Far West**: (-1755, -108)
- **Beacon Far South**: (1620, 3024)

## Files Modified

1. **Source/TiledLevelLoader/include/TiledToOlympe.h**
   - Added `globalOffsetX_` and `globalOffsetY_` fields

2. **Source/TiledLevelLoader/include/IsometricProjection.h**
   - Added global offset parameters to WorldToIso and IsoToWorld

3. **Source/TiledLevelLoader/src/IsometricProjection.cpp**
   - Implemented global offset application in both functions

4. **Source/TiledLevelLoader/src/TiledToOlympe.cpp**
   - Implemented complete 5-step transformation pipeline
   - Added chunk origin offset application
   - Added render order Y-flip transformation
   - Enhanced logging for all transformation steps

## Files Added

1. **test_global_offsets.cpp** - Unit tests for offset mathematics
2. **test_beacon_positions.cpp** - Integration test with actual map parameters

## C++14 Compliance

All code changes use C++14 standard features:
- Default parameter values
- Member initialization in declarations
- Standard library features (no C++17+ features used)

## Next Steps for Validation

1. **Runtime Testing**: Load isometric_quest.tmj in the game engine
2. **Visual Verification**: Check that beacons appear in correct positions
3. **Global Offset Tuning**: If minor systematic errors remain, adjust `globalOffsetX_` and `globalOffsetY_`
4. **Comparison with Tiled Editor**: Verify entity positions match those shown in Tiled

## Usage

To adjust global offsets if needed:
```cpp
// In TiledToOlympe::Convert() or initialization:
globalOffsetX_ = 0.0f;  // Adjust based on observed error
globalOffsetY_ = 0.0f;  // Adjust based on observed error
```

The infrastructure is now in place to support any necessary fine-tuning through these global offset values.
