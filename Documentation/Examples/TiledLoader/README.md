# TiledLoader Examples

This directory contains example code demonstrating the use of the TiledLevelLoader module.

## Examples

### 1. example_load_tiled_map.cpp
Complete example showing:
- Loading a .tmj file
- Inspecting map structure (layers, tilesets, objects)
- Configuring conversion settings
- Converting to Olympe LevelDefinition format
- Handling parallax layers
- Custom prefab mapping for objects
- Saving converted level

**Usage:**
```cpp
g++ -std=c++14 example_load_tiled_map.cpp -I../../Source -L../../build -lTiledLevelLoader -lOlympeCore
./a.out
```

### 2. example_isometric.cpp
Demonstrates isometric map support:
- Loading isometric maps
- TMJ object coordinate → world coordinate conversion
- Mouse picking in isometric view
- Calculating screen positions for tiles

**Key Formula:**
Tiled stores isometric object positions where BOTH X and Y are measured in `tileHeight` units:
```cpp
// TMJ pixel coords → tile coords (both divided by tileHeight!)
float tileX = tmjX / tileHeight;
float tileY = tmjY / tileHeight;

// Standard isometric projection
float worldX = (tileX - tileY) * (tileWidth / 2);
float worldY = (tileX + tileY) * (tileHeight / 2);
```

See [Documentation/TILED_ISOMETRIC.md](../../Documentation/TILED_ISOMETRIC.md) for detailed explanation.

**Usage:**
```cpp
g++ -std=c++14 example_isometric.cpp -I../../Source -L../../build -lTiledLevelLoader
./a.out
```

## Building Examples

To build all examples with CMake, add to your CMakeLists.txt:

```cmake
# TiledLoader Examples (optional)
option(BUILD_TILED_EXAMPLES "Build TiledLoader examples" OFF)

if(BUILD_TILED_EXAMPLES)
    add_executable(example_load_tiled_map Examples/TiledLoader/example_load_tiled_map.cpp)
    target_link_libraries(example_load_tiled_map PRIVATE TiledLevelLoader OlympeCore)
    
    add_executable(example_isometric Examples/TiledLoader/example_isometric.cpp)
    target_link_libraries(example_isometric PRIVATE TiledLevelLoader)
endif()
```

Then build with:
```bash
cmake -DBUILD_TILED_EXAMPLES=ON ..
make
```

## Sample Tiled Map

For testing, create a simple map in Tiled:

1. Open Tiled Map Editor
2. Create new map:
   - Orthogonal or Isometric
   - Size: 20x15 tiles
   - Tile size: 32x32 (or 64x32 for isometric)
3. Add tileset from image
4. Add layers:
   - "Ground" (tile layer)
   - "Walls" (tile layer for collision)
   - "Objects" (object layer)
   - "Background" (image layer with parallax)
5. Add objects with types: "Player", "Enemy", "Coin"
6. Save as .tmj format
7. Place in `Resources/Maps/level1.tmj`

## Custom Properties

You can add custom properties to objects in Tiled:

- **String properties**: Become string values in entity overrides
- **Int/Float properties**: Become numeric values
- **Bool properties**: Become boolean values
- **Color properties**: Become color string values
- **File properties**: Become path strings

Example in Tiled:
```
Object "Player"
  Type: Player
  Properties:
    health (int): 100
    speed (float): 5.5
    canFly (bool): false
    team (string): "heroes"
```

Converts to Olympe entity with overrides:
```json
{
  "id": "entity_1",
  "name": "Player",
  "prefabPath": "Blueprints/Player.json",
  "overrides": {
    "health": 100,
    "speed": 5.5,
    "canFly": false,
    "team": "heroes"
  }
}
```

## Prefab Mapping

Configure how Tiled object types map to Olympe prefabs:

```cpp
ConversionConfig config;
config.typeToPrefabMap["Player"] = "Blueprints/Player.json";
config.typeToPrefabMap["Enemy"] = "Blueprints/Enemy.json";
config.typeToPrefabMap["Coin"] = "Blueprints/Collectibles/Coin.json";
config.defaultPrefab = "Blueprints/DefaultEntity.json";
```

Objects without a type or with unmapped types use the default prefab.

## Layer Patterns

Configure which layers are treated as collision or sectors:

```cpp
config.collisionLayerPatterns = {"collision", "walls", "solid"};
config.sectorLayerPatterns = {"sector", "zone", "trigger"};
```

- **Collision layers**: Non-zero tiles become collision in collisionMap
- **Sector layers**: Polygon objects become sector entities
- **Patrol paths**: Polyline objects become patrol path entities

## Parallax Layers

Image layers in Tiled automatically become parallax layers:

```cpp
// In Tiled, set layer properties:
// - Parallax X: 0.5 (slower scrolling)
// - Parallax Y: 0.5
// - Repeat X: true
// - Repeat Y: false

const auto& parallax = converter.GetParallaxLayers();
for (size_t i = 0; i < parallax.GetLayerCount(); ++i) {
    const auto* layer = parallax.GetLayer(i);
    // Render layer with parallax scrolling
    float renderX, renderY;
    parallax.CalculateRenderPosition(*layer, cameraX, cameraY, renderX, renderY);
}
```

## Troubleshooting

**Problem**: Map fails to load
- Check file path is correct
- Verify .tmj format (not .tmx)
- Check external tilesets (.tsx/.tsj) are accessible

**Problem**: Objects not converting
- Verify object types are set in Tiled
- Check prefab mapping configuration
- Ensure prefab files exist

**Problem**: Collision not working
- Check collision layer names match patterns
- Verify layer is visible in Tiled
- Check tile IDs are non-zero

**Problem**: Parallax not working
- Verify image layer has image assigned
- Check parallax factors are set
- Ensure image path is accessible

## See Also

- [TiledLevelLoader README](../../Source/TiledLevelLoader/README.md)
- [Tiled Documentation](https://doc.mapeditor.org/)
- [Olympe Engine Documentation](../../Documentation/)
