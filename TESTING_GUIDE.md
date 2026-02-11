# Testing Guide for Overlay Rendering Fix

## How to Test the Fix

### Prerequisites
1. Build the project with the latest changes
2. Load an isometric level (e.g., `isometric_quest_solo.tmj`)
3. Have the game running with a player entity spawned

### Test Steps

#### 1. Enable Grid Overlays
Press **TAB** key to toggle grid overlays on/off.

#### 2. Verify Navigation Overlay
- **Expected**: Green/translucent overlay on all navigable tiles
- **Before fix**: No tiles rendered (0 displayed despite 3177 loaded)
- **After fix**: All 3177 navigable tiles should be visible

#### 3. Verify Collision Overlay  
- **Expected**: Red/purple overlay on blocked tiles
- **Before fix**: No tiles rendered
- **After fix**: All blocked tiles should be visible with purple circles

#### 4. Check Debug Logs
Look for these log entries in `olympe.log` or console output:

**During Level Loading:**
```
[1/2] Processing tile layers for navigation...
  Processing navigation from layer: graphicmap_ground (184x128, data size: 23552)
    DEBUG: Set navigable at tile (0,0), tileId=XXX in layer 'graphicmap_ground'
    DEBUG: Set navigable at tile (1,0), tileId=XXX in layer 'graphicmap_ground'
    ...
    -> Marked 3177 navigable tiles in this layer
-> Processed tile layers: 3177 navigable tiles

DEBUG: Verifying tile data can be read back...
  -> Sample navigable tile at (0,0)
  -> Sample navigable tile at (1,0)
  -> Sample navigable tile at (2,0)
  -> In first 10x10 grid: XX navigable, XX blocked
```

**During Rendering (First 3 Frames):**
```
[GridSystem] RenderNavigationOverlay call #0
  World bounds: x=XXX, y=XXX, w=XXX, h=XXX
  Corner grid coords: TL(X,Y) TR(X,Y) BL(X,Y) BR(X,Y)
  Grid range: (minX,minY) to (maxX,maxY)
  Tiles to scan: XXXX
  DEBUG: Test tile (67,39): navigable=YES, blocked=NO
  DEBUG: Test tile world coords: (810.XXX,1441.XXX)
  DEBUG: Is in grid scan range? YES
  -> Navigable tile #1 at grid (X,Y)
     World coords: (XXX,XXX)
  -> Navigable tile #2 at grid (X,Y)
     World coords: (XXX,XXX)
  ...
  -> Scanned XXXX tiles, found XXXX navigable tiles
```

#### 5. Verify Specific Expectations

**Player Spawn Tile Test:**
- Tile (67, 39) should be:
  - Marked as navigable: `navigable=YES`
  - Not blocked: `blocked=NO`  
  - In the viewport scan range when player is visible
  - Rendered with navigation overlay

**Tile Count Match:**
- Tiles found during rendering should match loading count
- Loading: `3177 navigable tiles`
- Rendering: `found 3177 navigable tiles` (across all frames showing tiles)

**Corner Coordinates:**
- All 4 corners should have different grid coordinates for isometric maps
- Example for isometric:
  - TL and BR will have similar Y values (vertical extent)
  - TR and BL define the X extent (horizontal extent in diamond)

## Visual Verification

### Navigation Overlay Appearance
```
     Viewport (Screen Space)          Grid Overlay (Isometric)
    ┌───────────────────┐                   ╱╲
    │                   │                  ╱  ╲
    │   [Game View]     │       →        ╱ ○○ ╲
    │                   │               ╱ ○○○○ ╲
    │                   │              │ ○○○○○○ │  ← Navigable tiles
    └───────────────────┘               ╲ ○○○○ ╱
                                         ╲ ○○ ╱
                                          ╲╱
```

Each `○` represents a navigable tile rendered with the overlay color.

### Expected Behavior
- **Movement**: Overlay should move smoothly with camera
- **Zoom**: Overlay tiles should scale with zoom level
- **Coverage**: All walkable areas should have overlay (no gaps)
- **Performance**: No noticeable frame rate drop

## Troubleshooting

### If No Tiles Render:
1. Check `olympe.log` for initialization errors
2. Verify collision map dimensions: should be `184x128`
3. Confirm tiles are being loaded: check for "Marked 3177 navigable tiles"
4. Ensure overlay is enabled: check `showNavigationOverlay` flag

### If Partial Rendering:
1. Check corner coordinates in logs - all 4 should be different
2. Verify grid range includes expected tiles
3. Check if `tilesScanned` matches expected viewport coverage

### If Wrong Colors:
1. Check overlay color configuration in grid settings
2. Verify layer index matches expected collision layer
3. Navigation: typically green/cyan (configurable)
4. Collision: typically red/purple (configurable)

## Performance Benchmarks

**Before Fix:**
- Tiles scanned per frame: 0 (incorrect viewport bounds)
- Tiles rendered: 0
- Grid range: Very small or empty

**After Fix (Expected):**
- Tiles scanned per frame: ~500-2000 (depends on zoom and viewport size)
- Tiles rendered: Matches tiles found (all navigable in viewport)
- Grid range: Diamond-shaped area covering full viewport

**Performance Impact:**
- Additional overhead: ~2 extra WorldToGrid() calls per frame
- Negligible impact: <0.1ms on modern hardware
- Memory: No additional allocations (uses existing tile data)

## Success Criteria

✅ **All criteria must pass:**
1. Navigation overlay displays tiles (count > 0)
2. Navigable tile count matches loading (3177 tiles total)
3. Player spawn tile (67, 39) confirmed navigable and visible
4. No performance regression (FPS remains stable)
5. Overlay moves smoothly with camera
6. Debug logs show all 4 corners being checked
7. Grid range forms a diamond shape for isometric maps

## Additional Tests

### Camera Movement Test
1. Move camera around the map
2. Verify overlay updates correctly
3. Tiles should appear/disappear at viewport edges smoothly

### Zoom Test
1. Zoom in: Tiles should become larger
2. Zoom out: Tiles should become smaller
3. Verify no tiles disappear unexpectedly

### Layer Switch Test
1. If multiple collision layers exist, switch between them
2. Verify overlay updates to show correct layer's tiles
3. Check layer index in debug logs matches selected layer
