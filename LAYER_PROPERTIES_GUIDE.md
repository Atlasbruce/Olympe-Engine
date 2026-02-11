# Layer-Based Navigation System - Property Configuration Guide

## Overview

The Olympe Engine now supports custom Tiled layer properties to control navigation and collision map generation. This allows for fine-grained control over which tiles are walkable, which are obstacles, and how borders are handled.

## Layer Properties

Add these **optional** boolean properties to tile layers in Tiled MapEditor:

| Property | Type | Default | Behavior |
|----------|------|---------|----------|
| `isTilesetWalkable` | `bool` | N/A | If `true`: non-empty tiles (GID ≠ 0) are **navigable**<br>If `false`: non-empty tiles are **collisions/obstacles** |
| `useTilesetBorder` | `bool` | N/A | If `true`: empty tiles (GID = 0) **adjacent** to non-empty tiles are **border collisions**<br>If `false`: empty tiles adjacent to non-empty tiles are **navigable** |

## Key Rules

1. **Layers without properties** → **Graphic-only layers**, skipped for navigation/collision processing
2. **Adjacent definition** (universal for iso/ortho/hexa): A tile is adjacent if it touches in any of the **8 directions** (N, NE, E, SE, S, SW, W, NW)
3. **Cumulative processing**: Layers **accumulate** navigation data (don't overwrite) unless explicitly marking as blocked
4. **Rendering**: Overlays render **on top** of all graphics (fixed Z-order)

## How to Add Properties in Tiled

### Step 1: Open Your Map in Tiled

Open your `.tmj` or `.tmx` file in Tiled MapEditor.

### Step 2: Select a Tile Layer

Click on the tile layer you want to configure (e.g., "layer_ground").

### Step 3: Add Custom Properties

1. In the **Properties** panel (usually on the left), scroll to the **Custom Properties** section
2. Click the **+** button to add a new property
3. Enter the property details:
   - **Name**: `isTilesetWalkable` or `useTilesetBorder`
   - **Type**: `bool`
   - **Value**: `true` or `false`

### Step 4: Configure Each Layer

Repeat for each layer according to your map design.

## Example Configurations

### Example 1: hexa-quest.tmj

For the hexagonal test map, configure layers as follows:

#### Layer: "layer_ground"
- **Purpose**: Main walkable ground tiles
- **Properties**:
  - `isTilesetWalkable` = `true`
  - `useTilesetBorder` = `true`
- **Result**: 
  - Non-empty tiles (the hexagonal ground) are navigable
  - Empty tiles adjacent to ground tiles become border obstacles
  - Prevents walking off the edge of the map

#### Layer: "layer on ground"
- **Purpose**: Obstacles placed on top of ground (rocks, trees, etc.)
- **Properties**:
  - `isTilesetWalkable` = `false`
  - (no `useTilesetBorder` needed)
- **Result**:
  - Non-empty tiles (obstacles) block navigation
  - Overrides walkable tiles from "layer_ground"

#### Layer: "layer_leveldesign" (object layer)
- **Purpose**: Object spawn points (players, items, etc.)
- **Properties**: None needed (object layers are not processed for navigation)

### Example 2: isometric_quest_solo.tmj

For the isometric test map:

#### Layer: "layer_0" (ground tiles)
- **Properties**:
  - `isTilesetWalkable` = `true`
  - `useTilesetBorder` = `false` (optional: only if you don't want borders)
- **Result**: All ground tiles are navigable

#### Layer: "layer_collision" (walls/obstacles)
- **Properties**:
  - `isTilesetWalkable` = `false`
- **Result**: Walls and obstacles block navigation

#### Graphic-only layers (e.g., decorations)
- **Properties**: None
- **Result**: Skipped entirely for navigation processing

### Example 3: Multi-Layer Dungeon

#### Layer: "floor"
- `isTilesetWalkable` = `true`
- `useTilesetBorder` = `true`

#### Layer: "walls"
- `isTilesetWalkable` = `false`

#### Layer: "water"
- `isTilesetWalkable` = `false`
- (Alternative: use traversal cost in future versions)

#### Layer: "decorations"
- No properties (graphic-only)

## Expected Console Output

When loading a map with layer properties, you should see logs like:

```
+==========================================================+
| COLLISION & NAVIGATION MAP GENERATION                    |
+==========================================================+
  [1/2] Processing tile layers for navigation...
    Layer 'layer_ground' navigation properties:
      - isTilesetWalkable: true
      - useTilesetBorder: true
      Processing layer 'layer_ground' for navigation...
        -> Non-empty tiles: 295 navigable, 0 blocked
        -> Border tiles: 158 marked as blocked
    Layer 'layer on ground' navigation properties:
      - isTilesetWalkable: false
      - useTilesetBorder: false
      Processing layer 'layer on ground' for navigation...
        -> Non-empty tiles: 0 navigable, 5 blocked
    Skipping graphic-only layer: layer_decorations
    -> Summary: 2 layers processed, 1 skipped (graphic only)
    -> Total navigable tiles: 295
    -> Total blocked tiles: 163 (obstacles: 5, borders: 158, objects: 0)

[GridSystem] RenderNavigationOverlay call #0
  Scanned 110 tiles, found 42 navigable tiles  ✅
```

## Visual Results

### Navigation Overlay (Green)
- **Color**: `rgba(50, 200, 100, 150)` (semi-transparent green)
- **Shows**: Tiles that are navigable (can pathfind through)
- **Toggle**: Press the appropriate key to enable/disable (check `GridSettings_data::showNavigationOverlay`)

### Collision Overlay (Purple/Magenta)
- **Color**: `rgba(150, 50, 200, 150)` (semi-transparent purple)
- **Shows**: Tiles that are blocked (obstacles, walls, borders)
- **Toggle**: Press the appropriate key to enable/disable (check `GridSettings_data::showCollisionOverlay`)

### Overlay Rendering Order
Overlays now render **last** (after all tiles and entities), ensuring they are always visible on top.

## Processing Logic

### Cumulative Tile Processing

```cpp
// Phase 1: Mark walkable tiles (does NOT overwrite blocked tiles)
if (props.isTilesetWalkable && !tileProps.isBlocked) {
    tileProps.isNavigable = true;
    tileProps.isBlocked = false;
}

// Phase 2: Mark obstacles (OVERRIDES previous navigation settings)
if (!props.isTilesetWalkable) {
    tileProps.isBlocked = true;
    tileProps.isNavigable = false;
}

// Phase 3: Mark borders (8-directional)
if (props.useTilesetBorder && tileIsEmpty && hasNonEmptyNeighbor) {
    tileProps.isBlocked = true;
    tileProps.isNavigable = false;
}
```

### Border Detection (8-Directional)

Empty tiles are marked as borders if they have **at least one** non-empty neighbor in these directions:

```
[NW] [N] [NE]
[W]  [X]  [E]
[SW] [S] [SE]
```

This works universally for:
- **Orthogonal** maps (4 main + 4 diagonal neighbors)
- **Isometric** maps (diamond grid neighbors)
- **Hexagonal** maps (6 main + 2 edge neighbors)

## Migration Guide

### Existing Maps (No Properties)

If your existing maps don't have layer properties:
- **All layers will be skipped** (treated as graphic-only)
- **No navigation data will be generated**
- **Solution**: Add properties to at least one layer

### Legacy Collision Detection

The engine still supports legacy collision detection via:
- **Layer name patterns**: Layers named "collision" or "walls" (case-insensitive)
- **Object layers**: Object groups marked as collision layers
- **Compatibility**: These continue to work alongside the new property system

## Troubleshooting

### Problem: No overlays visible

**Check**:
1. `GridSettings_data::showNavigationOverlay` is `true`
2. `GridSettings_data::showCollisionOverlay` is `true`
3. At least one layer has navigation properties
4. Overlays have non-zero alpha in `GridSettings_data::navigationColors` and `collisionColors`

### Problem: All tiles show as blocked

**Check**:
1. `isTilesetWalkable` is set to `true` on ground layers
2. `useTilesetBorder` is not set on obstacle layers (only ground layers)
3. Layer processing order (ground layers should be processed before obstacle layers)

### Problem: No navigation data generated

**Check**:
1. At least one layer has `isTilesetWalkable` or `useTilesetBorder` property
2. Console logs show "Processing layer X for navigation"
3. Layer data is not empty (GID values > 0)

## Technical Details

### Files Modified

- `TiledStructures.h` - Added `LayerProperties` struct and `ParseLayerProperties()` function
- `World.cpp` - Rewrote `GenerateCollisionAndNavigationMaps()` with property-based processing
- `ECS_Systems.cpp` - Fixed overlay rendering Z-order
- `ECS_Components.h` - Increased overlay alpha for better visibility

### C++14 Compatibility

All code is C++14 compatible (no C++17 features used).

### Performance

- Border detection is O(8 * tiles) per layer = linear complexity
- Cumulative processing is O(layers * tiles) = linear in total data
- No performance regression expected for typical map sizes

## See Also

- `TILED_QUICK_START.md` - General Tiled MapEditor integration guide
- `NAVIGATION_IMPLEMENTATION_SUMMARY.md` - Technical implementation details
- `OVERLAY_RENDERING_FIX.md` - Z-order rendering fix details
