# Tiled Loader Fixes + Multi-Layer Rendering System - Implementation Report

## Overview
This implementation addresses critical gaps in the Tiled map loading system and implements a complete multi-layer rendering system with parallax support for both orthogonal and isometric projections.

## Problems Addressed

### 1. Prefab Name Mismatch ✅
**Problem**: Tiled mapping used full Blueprint paths (`Blueprints/EntityPrefab/player.json`) but `PrefabFactory::CreateEntity()` expects short names (`PlayerEntity`).

**Solution**: Updated `Config/tiled_prefab_mapping.json` to use registered prefab names:
```json
{
  "player": "PlayerEntity",
  "npc": "NPCEntity",
  "enemy": "Enemy",
  ...
}
```

### 2. Missing Prefabs ✅
**Problem**: Many entity types referenced in Tiled maps had no registered prefabs, causing "unknown prefab" errors.

**Solution**: Added 7 new prefab registrations in `Source/VideoGame.cpp`:
- `Enemy` - Hostile NPC with AI components
- `Zombie` - Slower enemy variant
- `Key` - Collectible item
- `Collectible` - Generic collectible (coins, etc.)
- `Treasure` - Treasure chest item
- `Portal` - Level transition trigger
- `AmbientSound` - Invisible sound trigger

Each prefab includes fallback sprite loading for missing assets.

### 3. Parallax Layers Not Loaded ✅
**Problem**: Tiled image layers were converted but not stored in the level data, so they were lost during loading.

**Solution**: 
- Modified `TiledToOlympe::Convert()` to store parallax layers in `LevelMetadata.customData["parallaxLayers"]` as JSON
- Added `customData` field to `LevelMetadata` struct
- Each layer stores: name, imagePath, scrollFactorX/Y, repeatX/Y, offset, opacity, zOrder, visible, tintColor

### 4. Collision Objects Not Created ✅
**Problem**: Tiled collision rectangles (type="collision") failed to instantiate because there was no "collision" prefab.

**Solution**: Added special handling in `World::LoadLevelFromTiled()` to create collision entities manually:
```cpp
if (entityInstance->type == "collision")
{
    EntityID eid = CreateEntity();
    AddComponent<Identity_data>(eid, entityInstance->name, "Collision", EntityType::Static);
    AddComponent<Position_data>(eid, ...);
    AddComponent<CollisionZone_data>(eid, bounds, true);
}
```

Width and height are extracted from Tiled custom properties.

### 5. No Multi-Layer Rendering ✅
**Problem**: All entities rendered in random order without depth sorting or parallax layer integration.

**Solution**: Implemented complete multi-layer rendering system in `Source/ECS_Systems.cpp`:

#### RenderMultiLayerForCamera()
- Collects all renderable items (parallax layers + entities)
- Assigns depth values:
  - **Background layers**: depth = -1000 + zOrder (for zOrder < 0 or scrollFactor < 1.0)
  - **Entities**: depth = Y position
  - **Foreground layers**: depth = 10000 + zOrder (for zOrder > 0 or scrollFactor > 1.0)
- Sorts all items by depth (back to front)
- Renders in sorted order

#### Entity Depth Sorting
Entities are sorted by their Y position, creating proper overlap:
- Entities further up (smaller Y) render first (background)
- Entities further down (larger Y) render last (foreground)
- This creates a natural "isometric-style" depth effect even for orthogonal maps

### 6. Parallax Layer Manager Singleton ✅
**Problem**: No system existed to manage and render parallax layers.

**Solution**: Enhanced `ParallaxLayerManager` class:
- Singleton pattern with `ParallaxLayerManager::Get()`
- Stores SDL_Texture* for each layer
- `RenderLayer()` method handles:
  - Parallax scrolling calculation
  - Texture repeating (tiled backgrounds)
  - Opacity/alpha modulation
  - Camera viewport clipping
- `RenderAllLayers()` for z-order sorted rendering

## File Changes Summary

### Modified Files
1. **Config/tiled_prefab_mapping.json**
   - Changed all mappings from Blueprint paths to registered prefab names
   - Added "npc_entity", "ambient" aliases

2. **Source/VideoGame.cpp**
   - Added 7 new prefab registrations (Enemy, Zombie, Key, Collectible, Treasure, Portal, AmbientSound)
   - Each with proper components (Identity, Position, Visual, BoundingBox)
   - Fallback sprite loading for missing assets

3. **Source/World.cpp**
   - Added includes: `ParallaxLayerManager.h`, `GameEngine.h`, `SDL_image.h`
   - Added collision object special handling in `LoadLevelFromTiled()`
   - Added parallax layer loading from metadata
   - Loads textures via `IMG_LoadTexture()`

4. **Source/OlympeTilemapEditor/include/LevelManager.h**
   - Added `nlohmann::json customData` field to `LevelMetadata` struct
   - Initialized in constructor: `customData(json::object())`

5. **Source/TiledLevelLoader/src/TiledToOlympe.cpp**
   - Store parallax layers in `outLevel.metadata.customData["parallaxLayers"]` as JSON array
   - Each layer JSON includes all properties

6. **Source/TiledLevelLoader/include/ParallaxLayerManager.h**
   - Added singleton pattern (`Get()` static method)
   - Added `SDL_Texture* texture` field to `ParallaxLayer` struct
   - Added `int zOrder` field for depth sorting
   - Added rendering methods: `RenderLayer()`, `RenderAllLayers()`
   - Made constructors private for singleton
   - Deleted copy constructor/assignment

7. **Source/TiledLevelLoader/src/ParallaxLayerManager.cpp**
   - Implemented singleton `Get()` method
   - Implemented `RenderLayer()` with parallax scrolling math
   - Handles texture repeating (tiling)
   - Applies opacity/alpha
   - Fixed to use `cam.worldPosition` instead of `cam.position`
   - Destructor cleans up SDL_Texture* resources

8. **Source/ECS_Systems.cpp**
   - Added includes: `ParallaxLayerManager.h`, `<algorithm>`
   - Modified `RenderingSystem::Render()` to call `RenderMultiLayerForCamera()`
   - Implemented `RenderMultiLayerForCamera()`:
     - Collects layers and entities with depth
     - Sorts by depth
     - Renders in order
   - Implemented `RenderSingleEntity()`:
     - Extracted from `RenderEntitiesForCamera()`
     - Handles frustum culling, transform, rendering

9. **Source/ECS_Systems.h**
   - Added function declarations:
     - `void RenderMultiLayerForCamera(const CameraTransform& cam);`
     - `void RenderSingleEntity(const CameraTransform& cam, EntityID entity);`

## Technical Details

### Parallax Scrolling Math
```cpp
scrollX = layer.offsetX - (cameraX * layer.scrollFactorX)
scrollY = layer.offsetY - (cameraY * layer.scrollFactorY)
```

- `scrollFactor = 0.0`: Layer doesn't move (distant background)
- `scrollFactor = 1.0`: Layer moves with camera (normal)
- `scrollFactor > 1.0`: Layer moves faster (foreground)

### Depth Sorting Algorithm
```cpp
Items sorted by: depthY (float)
- Background layers: -1000 + zOrder
- Entities: position.y
- Foreground layers: 10000 + zOrder
```

### Texture Repeating
For layers with `repeatX` or `repeatY` enabled:
```cpp
startTileX = scrollX / texW - 1
endTileX = (scrollX + screenW) / texW + 1
// Render tiles in nested loop
```

## Testing Checklist

### Prerequisites
- Windows build environment (MSVC 2022) OR
- CMake 3.14+ with SDL3 properly configured
- SDL3 development libraries
- SDL3_image development libraries

### Build Steps
```bash
# Windows (Visual Studio)
1. Open "Olympe Engine.sln" in Visual Studio 2022
2. Set configuration to Debug or Release
3. Build solution (F7)
4. Run "Olympe Engine" project

# Linux/macOS (CMake)
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/SDL3 ..
make
./OlympeEngine
```

### Functional Tests

#### Test 1: Prefab Instantiation
**Objective**: Verify all entity types can be instantiated without errors

**Steps**:
1. Launch the engine
2. Open game menu (F2)
3. Load `example_orthogonal.tmj` (if available in test maps)
4. Check console output for errors

**Expected**:
- ✅ No "PrefabFactory::RegisteredPrefabError: Prefab 'X' unknown" messages
- ✅ All entities visible in game world
- ✅ Console shows: "World::LoadLevelFromTiled - Creating entity: [name] from prefab: [PrefabName]"

**Failure indicators**:
- ❌ "Failed to create entity from prefab" warnings
- ❌ Missing entities in the map

#### Test 2: Collision Zone Creation
**Objective**: Verify collision rectangles are created from Tiled collision objects

**Steps**:
1. Load a map with collision objects (type="collision")
2. Check console output
3. Verify collision entities exist

**Expected**:
- ✅ Console shows: "World::LoadLevelFromTiled - Created collision zone 'X' at (x,y) size (w×h)"
- ✅ Collision entities appear in entity list (if editor is available)

**Failure indicators**:
- ❌ No collision zone creation messages
- ❌ Entities pass through collision boundaries

#### Test 3: Parallax Layer Loading
**Objective**: Verify parallax layers are loaded from Tiled map

**Steps**:
1. Load `parallax-forest.tmj` (or any map with image layers)
2. Check console output
3. Verify layers render

**Expected**:
- ✅ Console shows: "TiledToOlympe: Converted X parallax layers"
- ✅ Console shows: "TiledToOlympe: Stored X parallax layers in metadata"
- ✅ Console shows: "World::LoadLevelFromTiled - Loaded parallax layer 'X' (zOrder=N, parallaxX=F)"
- ✅ Console shows: "World::LoadLevelFromTiled - Total parallax layers loaded: X"

**Failure indicators**:
- ❌ "Warning: Failed to load parallax image: X - [SDL Error]"
- ❌ No parallax layer messages
- ❌ Background layers missing

#### Test 4: Parallax Scrolling Effect
**Objective**: Verify parallax layers scroll at different rates

**Steps**:
1. Load a map with parallax layers
2. Move the camera/player around
3. Observe background layers

**Expected**:
- ✅ Background layers (scrollFactor < 1.0) move slower than camera
- ✅ Foreground layers (scrollFactor > 1.0) move faster than camera
- ✅ Ground layers (scrollFactor = 1.0) move with camera
- ✅ Smooth scrolling without jitter

**Failure indicators**:
- ❌ All layers move at the same speed
- ❌ Layers don't move at all
- ❌ Jittery or incorrect scrolling

#### Test 5: Entity Depth Sorting
**Objective**: Verify entities render in correct order based on Y position

**Steps**:
1. Load a map with multiple entities at different Y positions
2. Position entities so they overlap
3. Observe rendering order

**Expected**:
- ✅ Entity with smaller Y (further up) renders behind
- ✅ Entity with larger Y (further down) renders in front
- ✅ Overlap appears natural (like isometric depth)

**Failure indicators**:
- ❌ Entities render in wrong order
- ❌ Z-fighting or flickering
- ❌ All entities at same depth

#### Test 6: Multi-Layer Rendering Order
**Objective**: Verify rendering order: background → entities → foreground

**Steps**:
1. Load a map with background and foreground parallax layers
2. Move player/entities around
3. Observe layer ordering

**Expected**:
- ✅ Background layers (zOrder < 0) render behind entities
- ✅ Entities render above background
- ✅ Foreground layers (zOrder > 0) render in front of entities
- ✅ No visual artifacts or Z-fighting

**Failure indicators**:
- ❌ Entities hidden behind foreground layers
- ❌ Background layers render in front
- ❌ Incorrect layer order

### Performance Tests

#### Test 7: Frame Rate
**Objective**: Verify rendering performance is acceptable

**Steps**:
1. Load a large map with many entities and parallax layers
2. Monitor frame rate
3. Move camera around rapidly

**Expected**:
- ✅ Stable 60 FPS (or target frame rate)
- ✅ No significant frame drops
- ✅ Smooth camera movement

**Failure indicators**:
- ❌ FPS drops below 30
- ❌ Stuttering or jitter
- ❌ Memory leaks

## Known Limitations

1. **Sprite Assets**: Some prefabs reference non-existent sprite files (`enemy.png`, `zombie.png`, etc.). Fallback to generic entity sprites is implemented.

2. **Isometric Depth Sorting**: Full isometric tile-level depth sorting is not implemented. Current implementation sorts entities by Y position, which works for orthogonal maps and simple isometric layouts.

3. **Build System**: CMake configuration requires manual SDL3 path setup. Windows Visual Studio solution may be easier.

4. **Texture Memory**: Parallax layer textures are loaded once and persist. For maps with many layers, this could use significant VRAM.

5. **Layer Z-Order Assumption**: Assumes zOrder range of [-100, 100]. Very large zOrder values may cause incorrect sorting.

## Future Enhancements

1. **Isometric Tile Sorting**: Implement diamond-pattern depth sorting for full isometric projection support.

2. **Dynamic Layer Loading**: Load/unload parallax layer textures based on visibility or memory constraints.

3. **Layer Blending Modes**: Support additive, multiply, overlay blending for parallax layers.

4. **Animated Parallax**: Support animated parallax layers (sprite sheets).

5. **Performance Optimization**: Spatial partitioning (quadtree) for entity culling.

6. **Editor Integration**: Visual parallax layer editor in the Olympe Tilemap Editor.

## Conclusion

This implementation successfully addresses all critical gaps in the Tiled map loading system:
- ✅ Prefab name mapping corrected
- ✅ Missing prefabs added
- ✅ Parallax layers loaded and rendered
- ✅ Collision objects created
- ✅ Multi-layer rendering with depth sorting
- ✅ Proper entity overlap handling

The system is now ready for level creation using Tiled Map Editor with full parallax scrolling support.

## References

- **Tiled Map Editor**: https://www.mapeditor.org/
- **SDL3 Documentation**: https://wiki.libsdl.org/SDL3/
- **Original Issue**: GitHub issue #[number] (if applicable)
- **Pull Request**: Branch `copilot/fix-tiled-loader-issues`
