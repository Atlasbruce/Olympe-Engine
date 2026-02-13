---
id: navigation-mesh
title: Navigation Mesh Generation
sidebar_label: Navigation Mesh
sidebar_position: 2
---

# Navigation Mesh Generation

**Version:** 2.0  
**Last Updated:** 2025-02-13

## Overview

Olympe Engine automatically generates navigation meshes from Tiled map files (.tmj/.tmx). The system analyzes tile layers, interprets collision properties, and constructs multi-layer navigation grids that power the A* pathfinding system.

This guide covers:
- **Automatic generation** from Tiled maps
- **Layer property configuration** in Tiled
- **Tile collision interpretation**
- **Multi-layer navigation mesh** setup
- **Dynamic updates** at runtime
- **Custom navigation zones**

## Architecture

### Pipeline Overview

```
Tiled Map (.tmj/.tmx)
        ↓
TiledLevelLoader
        ↓
TiledToOlympe Converter
        ↓
CollisionMap Initialization
        ↓
Navigation Mesh Generation
        ↓
A* Pathfinding Ready
```

### Key Components

```cpp
// Source/CollisionMap.h
class CollisionMap {
    void Initialize(int width, int height, GridProjectionType projection,
                   float tileWidth, float tileHeight, int numLayers);
    
    void SetTileProperties(int x, int y, const TileProperties& props);
};

class NavigationMap {
    void Initialize(int width, int height, GridProjectionType projection,
                   float tileWidth, float tileHeight, int numLayers);
    
    void SetNavigable(int x, int y, bool isNavigable, float cost);
};
```

---

## Tiled Layer Properties

Navigation mesh generation is controlled by custom properties on tile layers in Tiled.

### Layer Property: isTilesetWalkable

**Type:** Boolean  
**Default:** `false`

Controls whether non-empty tiles are walkable or obstacles.

| Value | Behavior |
|-------|----------|
| `true` | Non-empty tiles are **walkable** (floor, ground) |
| `false` | Non-empty tiles are **obstacles** (walls, barriers) |

#### Use Cases

**Walkable Layer (`isTilesetWalkable: true`):**
- Floor tiles
- Ground terrain
- Platforms
- Roads

**Obstacle Layer (`isTilesetWalkable: false`):**
- Walls
- Trees
- Rocks
- Buildings

### Layer Property: useTilesetBorder

**Type:** Boolean  
**Default:** `false`

Controls whether empty tiles adjacent to non-empty tiles become obstacles.

| Value | Behavior |
|-------|----------|
| `true` | Empty tiles next to non-empty tiles are **obstacles** |
| `false` | Empty tiles are unaffected |

#### Use Case: Wall Borders

Prevents entities from "hugging" walls by making border tiles impassable:

```
[W] [W] [W] [W]    ← Non-empty wall tiles (obstacles)
[B] [·] [·] [B]    ← Border tiles (obstacles if useTilesetBorder=true)
[B] [·] [·] [B]    ← Walkable area
[B] [B] [B] [B]
```

Where:
- `[W]` = Wall tile (non-empty)
- `[B]` = Border tile (empty, adjacent to wall)
- `[·]` = Walkable tile (empty, not adjacent to wall)

### Parsing in Code

```cpp
// Source/TiledLevelLoader/include/TiledStructures.h
struct LayerProperties
{
    bool hasNavigationProperties;
    bool isTilesetWalkable;
    bool useTilesetBorder;
};

inline LayerProperties ParseLayerProperties(
    const std::map<std::string, TiledProperty>& properties)
{
    LayerProperties props;
    
    auto walkableIt = properties.find("isTilesetWalkable");
    if (walkableIt != properties.end() && walkableIt->second.type == PropertyType::Bool)
    {
        props.hasNavigationProperties = true;
        props.isTilesetWalkable = walkableIt->second.boolValue;
    }
    
    auto borderIt = properties.find("useTilesetBorder");
    if (borderIt != properties.end() && borderIt->second.type == PropertyType::Bool)
    {
        props.hasNavigationProperties = true;
        props.useTilesetBorder = borderIt->second.boolValue;
    }
    
    return props;
}
```

---

## Setting Up Navigation in Tiled

### Step 1: Create Map

1. Open Tiled Map Editor
2. Create new map: **File → New → New Map**
3. Choose projection:
   - **Orthogonal** (standard grid)
   - **Isometric** (diamond grid)
   - **Hexagonal** (hex grid)
4. Set tile size (e.g., 32x32 pixels)
5. Set map size (e.g., 50x50 tiles)

### Step 2: Create Tile Layers

Create separate layers for different navigation purposes:

**Ground Layer:**
- Name: "Ground"
- Purpose: Floor tiles (walkable area)

**Walls Layer:**
- Name: "Walls"
- Purpose: Wall tiles (obstacles)

**Decoration Layer:**
- Name: "Decoration"
- Purpose: Visual details (no collision)

### Step 3: Configure Layer Properties

#### Ground Layer (Walkable)

1. Select "Ground" layer
2. **View → Object Types Editor** (if not open)
3. Add custom property: `isTilesetWalkable`
4. Set type: `bool`
5. Set value: `true` ✓

**Result:** Non-empty tiles in this layer are walkable.

#### Walls Layer (Obstacles)

1. Select "Walls" layer
2. Add custom property: `isTilesetWalkable`
3. Set type: `bool`
4. Set value: `false`

**Optional:** Add `useTilesetBorder: true` to create obstacle borders.

**Result:** Non-empty tiles in this layer block movement.

#### Decoration Layer (No Collision)

1. Select "Decoration" layer
2. **Don't add any navigation properties**

**Result:** Layer is ignored for navigation (visual only).

### Step 4: Paint Tiles

1. Select layer
2. Choose tileset
3. Use paint tools to place tiles
4. **Ground layer**: Paint floors, paths, terrain
5. **Walls layer**: Paint walls, obstacles, barriers

### Step 5: Export Map

1. **File → Export As...**
2. Choose format:
   - **JSON (.tmj)** - Recommended
   - **XML (.tmx)** - Also supported
3. Save to `Gamedata/Maps/` directory
4. Engine loads map automatically

---

## Navigation Mesh Generation Process

### Phase 1: Map Loading

```cpp
// Load Tiled map file
TiledMap tiledMap;
TiledLevelLoader loader;
bool loaded = loader.LoadFromFile("Gamedata/Maps/level1.tmj", tiledMap);
```

### Phase 2: Map Conversion

```cpp
// Convert Tiled structures to Olympe format
Olympe::Editor::LevelDefinition level;
TiledToOlympe converter;
bool converted = converter.Convert(tiledMap, level);
```

### Phase 3: Collision Map Initialization

```cpp
// Initialize collision map with map dimensions
int width = tiledMap.width;
int height = tiledMap.height;
float tileWidth = static_cast<float>(tiledMap.tilewidth);
float tileHeight = static_cast<float>(tiledMap.tileheight);

GridProjectionType projection = GridProjectionType::Ortho;
if (tiledMap.orientation == "isometric")
    projection = GridProjectionType::Iso;
else if (tiledMap.orientation == "hexagonal")
    projection = GridProjectionType::HexAxial;

int numLayers = 1; // Or parse from map properties

CollisionMap::Get().Initialize(width, height, projection, 
                              tileWidth, tileHeight, numLayers);
```

### Phase 4: Layer Processing

For each tile layer with navigation properties:

```cpp
for (const TiledLayer& layer : tiledMap.layers)
{
    if (layer.type != LayerType::TileLayer)
        continue;
    
    // Parse navigation properties
    LayerProperties layerProps = ParseLayerProperties(layer.properties);
    
    if (!layerProps.hasNavigationProperties)
        continue; // Skip layers without nav properties
    
    // Process layer tiles
    ProcessNavigationLayer(layer, layerProps);
}
```

### Phase 5: Tile Property Generation

```cpp
void ProcessNavigationLayer(const TiledLayer& layer, const LayerProperties& props)
{
    for (int y = 0; y < layer.height; ++y)
    {
        for (int x = 0; x < layer.width; ++x)
        {
            int tileIndex = y * layer.width + x;
            uint32_t gid = layer.data[tileIndex];
            
            // Strip flip flags
            uint32_t tileId = gid & TILE_ID_MASK;
            
            bool isEmpty = (tileId == 0);
            bool isNonEmpty = !isEmpty;
            
            // Determine if tile is navigable
            bool isNavigable = false;
            if (props.isTilesetWalkable)
            {
                // Walkable layer: non-empty tiles are navigable
                isNavigable = isNonEmpty;
            }
            else
            {
                // Obstacle layer: non-empty tiles are NOT navigable
                isNavigable = isEmpty;
            }
            
            // Handle border tiles
            if (props.useTilesetBorder && isEmpty)
            {
                // Check if adjacent to non-empty tile
                bool adjacentToTile = IsAdjacentToNonEmptyTile(layer, x, y);
                if (adjacentToTile)
                {
                    isNavigable = false; // Border is obstacle
                }
            }
            
            // Set tile properties
            TileProperties tileProps;
            tileProps.isNavigable = isNavigable;
            tileProps.isBlocked = !isNavigable;
            tileProps.traversalCost = 1.0f; // Default cost
            
            CollisionMap::Get().SetTileProperties(x, y, tileProps);
        }
    }
}
```

### Phase 6: Navigation Map Sync

```cpp
// Navigation map delegates to collision map
NavigationMap::Get().Initialize(width, height, projection, 
                               tileWidth, tileHeight, numLayers);
```

---

## Multi-Layer Navigation

### Layer Configuration in Tiled

Use map-level custom properties to specify layer count:

**Map Property:** `navigationLayers`  
**Type:** Integer  
**Value:** Number of layers (1-8)

### Example: 3-Layer Setup

**Ground Layer (Layer 0):**
- Walking units
- Standard ground movement

**Sky Layer (Layer 1):**
- Flying units
- Ignores ground obstacles

**Underground Layer (Layer 2):**
- Burrowing units
- Separate tunnel network

### Configuring Layers in Code

```cpp
// Read layer count from map properties
int numLayers = 1; // Default
auto layerProp = tiledMap.properties.find("navigationLayers");
if (layerProp != tiledMap.properties.end() && layerProp->second.type == PropertyType::Int)
{
    numLayers = layerProp->second.intValue;
    numLayers = std::clamp(numLayers, 1, 8); // Enforce limits
}

// Initialize with multiple layers
CollisionMap::Get().Initialize(width, height, projection, 
                              tileWidth, tileHeight, numLayers);
```

### Layer Assignment

**Option 1: Map Property (Tile Layer)**

Add property to tile layer: `collisionLayer: 1` (Sky layer)

**Option 2: Code Assignment**

```cpp
// Set properties for specific layer
CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Sky, tileProps);
```

---

## Terrain Types and Traversal Costs

### Defining Terrain Types in Tiled

Use custom tile properties to specify terrain type:

1. Select tileset in Tiled
2. Select specific tile
3. Add custom property: `terrainType`
4. Set type: `string`
5. Set value: `"Water"`, `"Mud"`, `"Sand"`, etc.

### Parsing Terrain Types

```cpp
enum class TerrainType : uint8_t
{
    Invalid = 0,
    Ground,
    Water,
    Grass,
    Sand,
    Rock,
    Ice,
    Lava,
    Mud,
    Snow,
    Custom = 255
};

TerrainType ParseTerrainType(const std::string& typeName)
{
    if (typeName == "Ground") return TerrainType::Ground;
    if (typeName == "Water") return TerrainType::Water;
    if (typeName == "Grass") return TerrainType::Grass;
    if (typeName == "Sand") return TerrainType::Sand;
    if (typeName == "Mud") return TerrainType::Mud;
    if (typeName == "Snow") return TerrainType::Snow;
    if (typeName == "Ice") return TerrainType::Ice;
    // ... etc
    return TerrainType::Ground;
}
```

### Assigning Traversal Costs

```cpp
float GetTraversalCostForTerrain(TerrainType terrain)
{
    switch (terrain)
    {
        case TerrainType::Ground: return 1.0f;
        case TerrainType::Grass: return 1.0f;
        case TerrainType::Sand: return 1.2f;
        case TerrainType::Mud: return 1.5f;
        case TerrainType::Water: return 2.0f;
        case TerrainType::Snow: return 1.3f;
        case TerrainType::Ice: return 0.8f;
        case TerrainType::Rock: return 1.1f;
        case TerrainType::Lava: return 999.0f; // Effectively impassable
        default: return 1.0f;
    }
}

// Apply to navigation mesh
tileProps.terrain = ParseTerrainType(terrainName);
tileProps.traversalCost = GetTraversalCostForTerrain(tileProps.terrain);
CollisionMap::Get().SetTileProperties(x, y, tileProps);
```

---

## Custom Navigation Zones

### Safe Zones

Mark safe zones using custom tile flags:

**In Tiled:**
1. Select tile
2. Add property: `isSafeZone: true`

**In Code:**
```cpp
// Parse custom flags
uint8_t customFlags = 0;
if (tile.properties.find("isSafeZone") != tile.properties.end())
{
    if (tile.properties["isSafeZone"].boolValue)
    {
        customFlags |= 0x01; // Bit 0 = safe zone
    }
}

tileProps.customFlags = customFlags;
CollisionMap::Get().SetTileProperties(x, y, tileProps);
```

**Usage:**
```cpp
// Check if entity is in safe zone
int gridX, gridY;
CollisionMap::Get().WorldToGrid(pos.x, pos.y, gridX, gridY);

const TileProperties& tile = CollisionMap::Get().GetTileProperties(gridX, gridY);
bool isSafe = (tile.customFlags & 0x01) != 0;
```

### Slow Zones

Mark areas that slow movement:

```cpp
// Parse slow zone multiplier
float slowMultiplier = 1.0f;
auto slowProp = tile.properties.find("slowMultiplier");
if (slowProp != tile.properties.end() && slowProp->second.type == PropertyType::Float)
{
    slowMultiplier = slowProp->second.floatValue;
}

tileProps.traversalCost *= slowMultiplier;
```

### One-Way Passages

Use custom flags for directional movement:

```cpp
// Custom flags:
// Bit 1 = one-way north
// Bit 2 = one-way east
// Bit 3 = one-way south
// Bit 4 = one-way west

// Check if movement direction is allowed
bool CanMoveFrom(int fromX, int fromY, int toX, int toY)
{
    const TileProperties& fromTile = CollisionMap::Get().GetTileProperties(fromX, fromY);
    
    // Determine direction
    int dx = toX - fromX;
    int dy = toY - fromY;
    
    // Check one-way flags
    if (dy < 0 && (fromTile.customFlags & 0x02)) return false; // North blocked
    if (dx > 0 && (fromTile.customFlags & 0x04)) return false; // East blocked
    if (dy > 0 && (fromTile.customFlags & 0x08)) return false; // South blocked
    if (dx < 0 && (fromTile.customFlags & 0x10)) return false; // West blocked
    
    return true;
}
```

---

## Dynamic Navigation Mesh Updates

### Destructible Obstacles

```cpp
void DestroyWall(int tileX, int tileY)
{
    // Make tile navigable
    TileProperties props = CollisionMap::Get().GetTileProperties(tileX, tileY);
    props.isNavigable = true;
    props.isBlocked = false;
    CollisionMap::Get().SetTileProperties(tileX, tileY, props);
    
    // Update visual representation
    UpdateTileSprite(tileX, tileY, SPRITE_RUBBLE);
    
    // Invalidate nearby paths
    InvalidatePathsNearTile(tileX, tileY);
}
```

### Buildable Bridges

```cpp
void BuildBridge(int tileX, int tileY)
{
    // Change from water (cost 2.0) to bridge (cost 1.0)
    TileProperties props = CollisionMap::Get().GetTileProperties(tileX, tileY);
    props.traversalCost = 1.0f;
    props.terrain = TerrainType::Ground;
    CollisionMap::Get().SetTileProperties(tileX, tileY, props);
    
    // Update sprite
    UpdateTileSprite(tileX, tileY, SPRITE_BRIDGE);
    
    // Invalidate paths
    InvalidatePathsNearTile(tileX, tileY);
}
```

### Opening/Closing Doors

```cpp
void ToggleDoor(int tileX, int tileY, bool open)
{
    TileProperties props = CollisionMap::Get().GetTileProperties(tileX, tileY);
    props.isNavigable = open;
    props.isBlocked = !open;
    CollisionMap::Get().SetTileProperties(tileX, tileY, props);
    
    UpdateTileSprite(tileX, tileY, open ? SPRITE_DOOR_OPEN : SPRITE_DOOR_CLOSED);
    InvalidatePathsNearTile(tileX, tileY);
}
```

### Invalidating Cached Paths

```cpp
void InvalidatePathsNearTile(int tileX, int tileY, int radius = 5)
{
    auto entities = World::Get().GetAllEntitiesWithComponents<NavigationAgent_data>();
    
    for (EntityID entity : entities)
    {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        
        // Check if path goes near changed tile
        for (const Vector& waypoint : agent.currentPath)
        {
            int wx, wy;
            NavigationMap::Get().WorldToGrid(waypoint.x, waypoint.y, wx, wy);
            
            int dist = std::abs(wx - tileX) + std::abs(wy - tileY);
            if (dist <= radius)
            {
                agent.pathDirty = true;
                break;
            }
        }
    }
}
```

---

## Grid Projection Types

### Orthogonal Grid

**Properties:**
- 4-connected neighbors
- Tile coordinates = grid coordinates
- Simple world-to-grid conversion

**World-to-Grid:**
```cpp
outGridX = floor(worldX / tileWidth);
outGridY = floor(worldY / tileHeight);
```

**Grid-to-World:**
```cpp
outWorldX = (gridX + 0.5) * tileWidth;  // Center of tile
outWorldY = (gridY + 0.5) * tileHeight;
```

### Isometric Grid

**Properties:**
- 4-connected diamond neighbors
- Visual diamond projection
- More complex coordinate conversion

**World-to-Grid:**
```cpp
float isoX = worldX / (tileWidth * 0.5f);
float isoY = worldY / (tileHeight * 0.5f);
outGridX = floor((isoX + isoY) * 0.5f);
outGridY = floor((isoY - isoX) * 0.5f);
```

**Grid-to-World:**
```cpp
outWorldX = (gridX - gridY) * (tileWidth * 0.5f);
outWorldY = (gridX + gridY) * (tileHeight * 0.5f);
```

### Hexagonal Grid

**Properties:**
- 6-connected neighbors
- Axial coordinates
- Cube coordinate conversion for rounding

**World-to-Grid:**
```cpp
float q = (worldX * sqrt(3) / 3 - worldY / 3) / tileWidth;
float r = (worldY * 2 / 3) / tileHeight;

// Cube coordinate rounding
// ... (complex hex rounding logic)

outGridX = rx;  // q coordinate
outGridY = rz;  // r coordinate
```

**Grid-to-World:**
```cpp
float q = static_cast<float>(gridX);
float r = static_cast<float>(gridY);
outWorldX = tileWidth * (sqrt(3) * q + sqrt(3) / 2 * r);
outWorldY = tileHeight * (3.0f / 2.0f * r);
```

---

## Debugging Navigation Mesh

### Visual Overlay

```cpp
void RenderNavigationMeshOverlay()
{
    int width = NavigationMap::Get().GetWidth();
    int height = NavigationMap::Get().GetHeight();
    
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float worldX, worldY;
            NavigationMap::Get().GridToWorld(x, y, worldX, worldY);
            
            if (NavigationMap::Get().IsNavigable(x, y))
            {
                float cost = NavigationMap::Get().GetTraversalCost(x, y);
                
                // Color by traversal cost
                SDL_Color color;
                if (cost <= 1.0f)
                    color = {0, 255, 0, 100};     // Green: normal
                else if (cost <= 1.5f)
                    color = {255, 255, 0, 100};   // Yellow: slow
                else
                    color = {255, 0, 0, 100};     // Red: very slow
                
                DrawTileOverlay(worldX, worldY, color);
            }
            else
            {
                // Red: blocked
                DrawTileOverlay(worldX, worldY, {255, 0, 0, 150});
            }
        }
    }
}
```

### Console Commands

```cpp
// Print navigation mesh info
void DebugPrintNavigationMesh()
{
    int width = NavigationMap::Get().GetWidth();
    int height = NavigationMap::Get().GetHeight();
    
    int navigable = 0;
    int blocked = 0;
    
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (NavigationMap::Get().IsNavigable(x, y))
                navigable++;
            else
                blocked++;
        }
    }
    
    std::cout << "Navigation Mesh Statistics:\n";
    std::cout << "  Total tiles: " << (width * height) << "\n";
    std::cout << "  Navigable: " << navigable << " (" 
              << (100.0f * navigable / (width * height)) << "%)\n";
    std::cout << "  Blocked: " << blocked << " (" 
              << (100.0f * blocked / (width * height)) << "%)\n";
}
```

### Export to Image

```cpp
void ExportNavigationMeshToImage(const std::string& filename)
{
    int width = NavigationMap::Get().GetWidth();
    int height = NavigationMap::Get().GetHeight();
    
    // Create image buffer
    std::vector<uint8_t> pixels(width * height * 3);
    
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = (y * width + x) * 3;
            
            if (NavigationMap::Get().IsNavigable(x, y))
            {
                float cost = NavigationMap::Get().GetTraversalCost(x, y);
                
                // Green channel = navigable
                // Red channel = cost
                pixels[idx + 0] = static_cast<uint8_t>(cost * 100);  // R
                pixels[idx + 1] = 255;                                // G
                pixels[idx + 2] = 0;                                  // B
            }
            else
            {
                // Black = blocked
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
            }
        }
    }
    
    // Save as PNG/BMP (using image library)
    SaveImage(filename, pixels.data(), width, height);
}
```

---

## Complete Example

### Tiled Map Configuration

**Map Properties:**
- `navigationLayers: 1`
- `projection: "orthogonal"`

**Ground Layer:**
- Name: "Ground"
- Property: `isTilesetWalkable: true`

**Walls Layer:**
- Name: "Walls"
- Property: `isTilesetWalkable: false`
- Property: `useTilesetBorder: true`

**Water Layer:**
- Name: "Water"
- Property: `isTilesetWalkable: true`
- Tile property: `terrainType: "Water"`

### Loading Code

```cpp
// Load map
TiledMap tiledMap;
TiledLevelLoader::Get().LoadFromFile("Gamedata/Maps/dungeon.tmj", tiledMap);

// Convert to Olympe format
Olympe::Editor::LevelDefinition level;
TiledToOlympe::Get().Convert(tiledMap, level);

// Initialize collision/navigation maps
CollisionMap::Get().Initialize(
    tiledMap.width, tiledMap.height,
    GridProjectionType::Ortho,
    static_cast<float>(tiledMap.tilewidth),
    static_cast<float>(tiledMap.tileheight),
    1 // Single layer
);

NavigationMap::Get().Initialize(
    tiledMap.width, tiledMap.height,
    GridProjectionType::Ortho,
    static_cast<float>(tiledMap.tilewidth),
    static_cast<float>(tiledMap.tileheight),
    1
);

// Process layers
for (const TiledLayer& layer : tiledMap.layers)
{
    if (layer.type == LayerType::TileLayer)
    {
        LayerProperties props = ParseLayerProperties(layer.properties);
        if (props.hasNavigationProperties)
        {
            ProcessNavigationLayer(layer, props);
        }
    }
}

// Navigation mesh ready!
std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(startX, startY, goalX, goalY, path);
```

---

## Best Practices

### 1. Consistent Layer Naming

Use consistent layer names across all maps:
- "Ground" - Walkable floor
- "Walls" - Obstacles
- "Water" - Special terrain
- "Decoration" - Visual only

### 2. Set Properties on All Nav Layers

Always set `isTilesetWalkable` on layers affecting navigation:

```
Ground:     isTilesetWalkable = true
Walls:      isTilesetWalkable = false
Water:      isTilesetWalkable = true (but high cost)
Decoration: (no property = ignored)
```

### 3. Use Border Property Wisely

Enable `useTilesetBorder: true` only on wall layers:

```
Ground: useTilesetBorder = false
Walls:  useTilesetBorder = true  ← Prevents wall-hugging
```

### 4. Test Navigation in Editor

Create debug mode to visualize navigation mesh:
- Green = navigable
- Red = blocked
- Yellow = high cost

### 5. Optimize Large Maps

For maps > 100x100 tiles:
- Use sectorization
- Cache paths
- Limit pathfinding iterations

---

## Troubleshooting

### Problem: All Tiles Blocked

**Cause:** Layer missing `isTilesetWalkable` property  
**Solution:** Add property to layer in Tiled

### Problem: Entities Stuck on Walls

**Cause:** Border tiles not configured  
**Solution:** Enable `useTilesetBorder: true` on wall layer

### Problem: Paths Ignore Terrain Costs

**Cause:** Terrain types not parsed  
**Solution:** Add `terrainType` property to tiles in tileset

### Problem: Navigation Mesh Empty

**Cause:** No layers have navigation properties  
**Solution:** Ensure at least one layer has `isTilesetWalkable`

### Problem: Wrong Projection

**Cause:** Mismatch between Tiled orientation and engine projection  
**Solution:** Verify `projection` matches Tiled map orientation

---

## Performance Considerations

### Generation Time

- **Small maps (< 50x50)**: < 1ms
- **Medium maps (50x50 to 100x100)**: 1-10ms
- **Large maps (> 100x100)**: 10-100ms

### Memory Usage

Each tile: ~64 bytes  
100x100 map = ~640 KB per layer

### Optimization Tips

1. **Pre-calculate world coordinates** during initialization
2. **Use layer properties** to skip visual-only layers
3. **Batch tile updates** for dynamic changes
4. **Cache frequently-used paths**

---

## See Also

- [Pathfinding System](./pathfinding.md) - A* algorithm and usage
- [Behavior Tree Navigation](../behavior-trees/nodes.md) - AI integration
- [Tiled Map Format](../../user-guide/tiled-editor/map-format.md) - Map structure
- [Collision System](../architecture/collision.md) - Collision detection
