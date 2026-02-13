---
id: overlay-rendering
title: Overlay Rendering
sidebar_label: Overlay Rendering
---

# Overlay Rendering Fix for Isometric Grids

## Problem Description

Navigation and collision overlay rendering was not displaying tiles despite data being correctly loaded (3177 navigable tiles detected during loading but 0 rendered).

## Root Cause

**Isometric Grid Coordinate Bounds Calculation**

For isometric grids, the visible area in grid space forms a **diamond shape**, not a rectangle. The original implementation only checked two corners of the viewport (top-left and bottom-right) when converting from world coordinates to grid coordinates:

```cpp
// OLD CODE (INCORRECT for isometric)
collMap.WorldToGrid(bounds.x, bounds.y, minGridX, minGridY);
collMap.WorldToGrid(bounds.x + bounds.w, bounds.y + bounds.h, maxGridX, maxGridY);
```

This approach works for orthogonal grids but **fails for isometric grids** because:
- In orthogonal grids: rectangular viewport → rectangular grid range
- In isometric grids: rectangular viewport → diamond-shaped grid range

By only checking two diagonal corners, many tiles in the diamond were excluded from the min/max calculation.

## Solution

Check **all four corners** of the viewport and compute min/max across all of them:

```cpp
// NEW CODE (CORRECT for isometric)
// Check all four corners
int tlX, tlY, trX, trY, blX, blY, brX, brY;
collMap.WorldToGrid(bounds.x, bounds.y, tlX, tlY); // Top-left
collMap.WorldToGrid(bounds.x + bounds.w, bounds.y, trX, trY); // Top-right
collMap.WorldToGrid(bounds.x, bounds.y + bounds.h, blX, blY); // Bottom-left
collMap.WorldToGrid(bounds.x + bounds.w, bounds.y + bounds.h, brX, brY); // Bottom-right

// Find min/max across all corners
minGridX = std::min({tlX, trX, blX, brX});
minGridY = std::min({tlY, trY, blY, brY});
maxGridX = std::max({tlX, trX, blX, brX});
maxGridY = std::max({tlY, trY, blY, brY});
```

## Visual Explanation

### Orthogonal Grid (Original Code Works)
```
Viewport in World Space:     Grid Space (same shape):
┌─────────────┐              ┌─────────────┐
│             │              │ * * * * * * │
│             │      →       │ * * * * * * │
│             │              │ * * * * * * │
└─────────────┘              └─────────────┘
  TL        BR                 TL        BR
  (min)     (max)              (min)     (max)
```
Two corners are sufficient to determine min/max.

### Isometric Grid (Original Code FAILS)
```
Viewport in World Space:     Grid Space (diamond):
┌─────────────┐                  TL
│             │                 ╱  ╲
│             │      →      TR ╱    ╲ BL
│             │                ╲    ╱
└─────────────┘                 ╲  ╱
  TL        BR                    BR
```
With original code checking only TL and BR:
- We get the vertical extent (BR is min Y, TL is max Y) ✓
- **But miss horizontal extent!** TR and BL define the actual X bounds ✗

With fixed code checking all 4 corners:
- minX = min(TL.x, TR.x, BL.x, BR.x) ✓
- maxX = max(TL.x, TR.x, BL.x, BR.x) ✓
- minY = min(TL.y, TR.y, BL.y, BR.y) ✓
- maxY = max(TL.y, TR.y, BL.y, BR.y) ✓

## Files Changed

1. **Source/ECS_Systems.cpp**:
   - `GridSystem::RenderCollisionOverlay()` - Fixed grid bounds calculation
   - `GridSystem::RenderNavigationOverlay()` - Fixed grid bounds calculation
   - Added comprehensive debug logging to both functions

2. **Source/World.cpp**:
   - Added debug logging during tile loading
   - Added verification check after loading

## Testing

The fix includes comprehensive debug logging that outputs:
- World bounds of the viewport
- Grid coordinates of all 4 corners
- Computed min/max grid range
- Number of tiles scanned
- Number of navigable/blocked tiles found
- Sample tiles with their coordinates

Enable grid overlays with the TAB key to see the rendered tiles.

## Expected Behavior After Fix

- Navigation overlay should display all 3177 navigable tiles
- Collision overlay should display blocked tiles correctly
- Debug logs should show:
  - Grid range spanning the visible diamond area
  - Tiles being found and rendered during each frame
  - Player spawn tile (67, 39) confirmed as navigable

## Performance Impact

Minimal - the fix adds 2 extra `WorldToGrid()` calls per frame (checking TR and BL corners in addition to TL and BR). These are simple mathematical transformations with negligible overhead.
