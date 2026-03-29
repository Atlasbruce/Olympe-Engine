# Navigation System Reference

## Table of Contents
- [Architecture Overview](#architecture-overview)
- [CollisionMap API](#collisionmap-api)
- [NavigationMap API](#navigationmap-api)
- [TileProperties Structure](#tileproperties-structure)
- [TerrainType Enum](#terraintype-enum)
- [CollisionLayer Enum](#collisionlayer-enum)
- [GridProjectionType Enum](#gridprojectiontype-enum)
- [Coordinate Systems](#coordinate-systems)
- [Complete Code Examples](#complete-code-examples)

---

## Architecture Overview

The Olympe Engine navigation system provides a multi-layer collision and pathfinding infrastructure designed for 2D games with complex navigation requirements. The system consists of two main singleton components:

### **CollisionMap**
- Stores rich tile properties (collision, terrain type, traversal cost, custom flags)
- Supports up to 8 independent collision layers
- Handles dynamic state transitions (destructible walls, buildable bridges, openable doors)
- Provides world-to-grid coordinate conversion
- Supports orthogonal, isometric, and hexagonal grid projections

### **NavigationMap**
- Implements A* pathfinding algorithm
- Delegates to CollisionMap for tile properties
- Supports multi-layer pathfinding
- Handles iteration limits for performance control
- Returns world-space paths ready for agent following

### **Key Design Principles**
1. **Singleton Pattern**: Both CollisionMap and NavigationMap are singletons accessed via `Get()`
2. **Multi-Layer Support**: 8 independent layers (Ground, Sky, Underground, Volume, Custom1-4)
3. **Rich Tile Data**: Each tile stores collision, navigability, traversal cost, terrain type, and custom flags
4. **Dynamic States**: Tiles can transition between states (wall → rubble, water → bridge)
5. **Grid Projection Agnostic**: Supports orthogonal, isometric, and hexagonal grids

---

## CollisionMap API

The `CollisionMap` singleton manages collision and tile properties across multiple layers.

### **Initialization**

```cpp
void Initialize(int width, int height, GridProjectionType projection, 
               float tileWidth, float tileHeight, int numLayers = 1);
```

**Parameters:**
- `width`: Map width in tiles
- `height`: Map height in tiles
- `projection`: Grid projection type (Ortho, Iso, HexAxial)
- `tileWidth`: Tile width in world units (pixels)
- `tileHeight`: Tile height in world units (pixels)
- `numLayers`: Number of collision layers (1-8, default: 1)

**Example:**
```cpp
// Initialize 100x100 orthogonal map with 32x32 tiles, 2 layers
CollisionMap::Get().Initialize(100, 100, GridProjectionType::Ortho, 32.0f, 32.0f, 2);
```

---

### **Layer Management**

```cpp
void SetActiveLayer(CollisionLayer layer);
CollisionLayer GetActiveLayer() const;
int GetNumLayers() const;
```

**Description:**
- `SetActiveLayer()`: Sets the active layer for subsequent operations
- `GetActiveLayer()`: Returns the currently active layer
- `GetNumLayers()`: Returns the total number of initialized layers

**Example:**
```cpp
// Switch to sky layer
CollisionMap::Get().SetActiveLayer(CollisionLayer::Sky);

// Check current layer
CollisionLayer current = CollisionMap::Get().GetActiveLayer();
```

---

### **Tile Properties Access (Current Layer)**

```cpp
void SetTileProperties(int x, int y, const TileProperties& props);
const TileProperties& GetTileProperties(int x, int y) const;
```

**Description:**
- Operates on the currently active layer set via `SetActiveLayer()`
- Returns `s_emptyTile` for invalid coordinates

**Example:**
```cpp
// Set tile properties on active layer
TileProperties props;
props.isBlocked = true;
props.terrain = TerrainType::Rock;
props.traversalCost = 2.0f;
CollisionMap::Get().SetTileProperties(10, 15, props);

// Get tile properties
const TileProperties& tile = CollisionMap::Get().GetTileProperties(10, 15);
if (tile.isBlocked) {
    // Handle collision
}
```

---

### **Tile Properties Access (Specific Layer)**

```cpp
void SetTileProperties(int x, int y, CollisionLayer layer, const TileProperties& props);
const TileProperties& GetTileProperties(int x, int y, CollisionLayer layer) const;
```

**Description:**
- Directly access properties on a specific layer without changing active layer
- More efficient when working with multiple layers simultaneously

**Example:**
```cpp
// Set ground layer to grass (walkable)
TileProperties ground;
ground.isBlocked = false;
ground.terrain = TerrainType::Grass;
ground.traversalCost = 1.0f;
CollisionMap::Get().SetTileProperties(5, 5, CollisionLayer::Ground, ground);

// Set sky layer to blocked (no flying)
TileProperties sky;
sky.isBlocked = true;
CollisionMap::Get().SetTileProperties(5, 5, CollisionLayer::Sky, sky);

// Read from both layers
auto groundTile = CollisionMap::Get().GetTileProperties(5, 5, CollisionLayer::Ground);
auto skyTile = CollisionMap::Get().GetTileProperties(5, 5, CollisionLayer::Sky);
```

---

### **Quick Collision Checks**

```cpp
void SetCollision(int x, int y, bool hasCollision);
bool HasCollision(int x, int y) const;
bool HasCollision(int x, int y, CollisionLayer layer) const;
```

**Description:**
- Backward-compatible API for simple collision checks
- `SetCollision()` updates the `isBlocked` field of TileProperties
- More efficient than full property access when only collision matters

**Example:**
```cpp
// Simple collision set
CollisionMap::Get().SetCollision(10, 10, true);

// Check collision on active layer
if (CollisionMap::Get().HasCollision(10, 10)) {
    // Blocked!
}

// Check collision on specific layer
if (CollisionMap::Get().HasCollision(10, 10, CollisionLayer::Underground)) {
    // Underground passage blocked
}
```

---

### **Dynamic State Transitions**

```cpp
typedef std::function<void(TileProperties&)> TileUpdateFunc;
void UpdateTileState(int x, int y, TileUpdateFunc updateFunc);
void UpdateTileState(int x, int y, CollisionLayer layer, TileUpdateFunc updateFunc);
```

**Description:**
- Powerful lambda-based API for dynamic tile state changes
- Enables destructible walls, buildable bridges, openable doors
- Thread-safe via lambda callback pattern

**Example: Destructible Wall**
```cpp
// Destroy a wall (wall → rubble)
CollisionMap::Get().UpdateTileState(10, 10, [](TileProperties& tile) {
    tile.isBlocked = false;        // No longer blocks
    tile.isNavigable = true;       // Now navigable
    tile.terrain = TerrainType::Rock; // Changed to rubble
    tile.onDestroyedState = "Rubble";
});
```

**Example: Build Bridge**
```cpp
// Build a bridge over water
CollisionMap::Get().UpdateTileState(5, 5, CollisionLayer::Ground, [](TileProperties& tile) {
    tile.isBlocked = false;
    tile.isNavigable = true;
    tile.traversalCost = 1.0f; // Normal cost
    tile.terrain = TerrainType::Ground;
    tile.onBuiltState = "Bridge";
});
```

**Example: Open Door**
```cpp
// Toggle door state
bool isDoorOpen = false;
CollisionMap::Get().UpdateTileState(15, 20, [&isDoorOpen](TileProperties& tile) {
    isDoorOpen = !isDoorOpen;
    tile.isBlocked = !isDoorOpen;
    tile.isNavigable = isDoorOpen;
    tile.customFlags = isDoorOpen ? 0x01 : 0x00; // Custom flag: bit 0 = door open
});
```

---

### **Coordinate Conversion**

```cpp
void WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const;
void GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const;
```

**Description:**
- Converts between world-space (pixels) and grid-space (tiles)
- Handles orthogonal, isometric, and hexagonal projections automatically
- Essential for entity-to-tile and tile-to-entity conversions

**Example:**
```cpp
// Convert entity position to grid coordinates
Vector entityPos(256.0f, 192.0f, 0.0f);
int gridX, gridY;
CollisionMap::Get().WorldToGrid(entityPos.x, entityPos.y, gridX, gridY);
// gridX = 8, gridY = 6 (for 32x32 tiles)

// Convert grid coordinates back to world
float worldX, worldY;
CollisionMap::Get().GridToWorld(gridX, gridY, worldX, worldY);
// worldX = 256.0f, worldY = 192.0f
```

---

### **Validation**

```cpp
bool IsValidGridPosition(int x, int y) const;
bool IsValidGridPosition(int x, int y, CollisionLayer layer) const;
```

**Description:**
- Checks if grid coordinates are within map bounds
- Layer-specific version validates both position and layer

**Example:**
```cpp
// Check if position is valid
if (CollisionMap::Get().IsValidGridPosition(10, 15)) {
    // Safe to access
}

// Check if position and layer are valid
if (CollisionMap::Get().IsValidGridPosition(10, 15, CollisionLayer::Sky)) {
    // Safe to access sky layer at this position
}
```

---

### **Accessors**

```cpp
int GetWidth() const;
int GetHeight() const;
GridProjectionType GetProjection() const;
float GetTileWidth() const;
float GetTileHeight() const;
const std::vector<std::vector<TileProperties>>& GetLayer(CollisionLayer layer) const;
```

**Description:**
- Query map dimensions and properties
- `GetLayer()` provides direct access to raw tile grid (for visualization/debugging)

**Example:**
```cpp
int mapWidth = CollisionMap::Get().GetWidth();
int mapHeight = CollisionMap::Get().GetHeight();
float tileSize = CollisionMap::Get().GetTileWidth();

// Access raw grid for visualization
const auto& groundLayer = CollisionMap::Get().GetLayer(CollisionLayer::Ground);
for (int y = 0; y < mapHeight; ++y) {
    for (int x = 0; x < mapWidth; ++x) {
        const TileProperties& tile = groundLayer[y][x];
        // Render tile based on properties
    }
}
```

---

### **Clear**

```cpp
void Clear();
```

**Description:**
- Clears all layers and resets map to empty state
- Call before loading a new level

**Example:**
```cpp
CollisionMap::Get().Clear();
```

---

## NavigationMap API

The `NavigationMap` singleton provides A* pathfinding using CollisionMap data.

### **Initialization**

```cpp
void Initialize(int width, int height, GridProjectionType projection,
               float tileWidth, float tileHeight, int numLayers = 1);
```

**Parameters:** Same as CollisionMap::Initialize()

**Example:**
```cpp
// Initialize navigation map (should match CollisionMap dimensions)
NavigationMap::Get().Initialize(100, 100, GridProjectionType::Ortho, 32.0f, 32.0f, 2);
```

---

### **Layer Management**

```cpp
void SetActiveLayer(CollisionLayer layer);
CollisionLayer GetActiveLayer() const;
```

**Description:**
- Sets the active layer for pathfinding operations
- Delegates to CollisionMap for actual layer data

**Example:**
```cpp
// Pathfind on ground layer
NavigationMap::Get().SetActiveLayer(CollisionLayer::Ground);
```

---

### **Navigability Queries**

```cpp
void SetNavigable(int x, int y, bool isNavigable, float cost = 1.0f);
bool IsNavigable(int x, int y) const;
float GetTraversalCost(int x, int y) const;
bool IsNavigable(int x, int y, CollisionLayer layer) const;
float GetTraversalCost(int x, int y, CollisionLayer layer) const;
```

**Description:**
- `SetNavigable()`: Updates tile navigability and cost (delegates to CollisionMap)
- `IsNavigable()`: Checks if tile can be pathfound through
- `GetTraversalCost()`: Returns movement cost multiplier (1.0 = normal, >1.0 = slow)

**Example:**
```cpp
// Check if tile is walkable
if (NavigationMap::Get().IsNavigable(10, 10)) {
    float cost = NavigationMap::Get().GetTraversalCost(10, 10);
    // cost = 1.0 (normal), 2.0 (mud), 0.5 (road), etc.
}

// Check navigability on specific layer
if (NavigationMap::Get().IsNavigable(10, 10, CollisionLayer::Underground)) {
    // Underground tunnel accessible
}
```

---

### **A* Pathfinding**

```cpp
bool FindPath(int startX, int startY, int goalX, int goalY, 
             std::vector<Vector>& outPath, CollisionLayer layer = CollisionLayer::Ground,
             int maxIterations = 10000);
```

**Parameters:**
- `startX`, `startY`: Start grid coordinates
- `goalX`, `goalY`: Goal grid coordinates
- `outPath`: Output vector of world-space waypoints (cleared before use)
- `layer`: Collision layer to pathfind on (default: Ground)
- `maxIterations`: Maximum A* iterations (prevents infinite loops)

**Returns:**
- `true` if path found, `false` otherwise
- `outPath` contains world-space positions if successful

**Example:**
```cpp
std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(
    5, 5,   // Start grid position
    50, 50, // Goal grid position
    path,   // Output path
    CollisionLayer::Ground, // Layer
    5000    // Max iterations
);

if (found) {
    // Path contains world-space waypoints
    for (const Vector& waypoint : path) {
        // Follow path
    }
}
```

---

### **Coordinate Conversion**

```cpp
void WorldToGrid(float worldX, float worldY, int& outGridX, int& outGridY) const;
void GridToWorld(int gridX, int gridY, float& outWorldX, float& outWorldY) const;
```

**Description:**
- Delegates to CollisionMap for consistency
- Use before calling FindPath() to convert entity positions

**Example:**
```cpp
// Convert entity positions to grid for pathfinding
Vector startWorld(100.0f, 150.0f, 0.0f);
Vector goalWorld(800.0f, 600.0f, 0.0f);

int startX, startY, goalX, goalY;
NavigationMap::Get().WorldToGrid(startWorld.x, startWorld.y, startX, startY);
NavigationMap::Get().WorldToGrid(goalWorld.x, goalWorld.y, goalX, goalY);

std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(startX, startY, goalX, goalY, path);
```

---

### **Validation**

```cpp
bool IsValidGridPosition(int x, int y) const;
```

**Description:**
- Checks if grid position is within bounds
- Delegates to CollisionMap

---

### **Accessors**

```cpp
int GetWidth() const;
int GetHeight() const;
GridProjectionType GetProjection() const;
```

---

### **Clear**

```cpp
void Clear();
```

**Description:**
- Clears navigation data
- Call when unloading level

---

## TileProperties Structure

Rich per-tile data structure stored in CollisionMap.

```cpp
struct TileProperties
{
    bool isBlocked = false;           // Hard collision (impassable wall)
    bool isNavigable = true;          // Can pathfind through
    float traversalCost = 1.0f;       // Cost for pathfinding (1.0 = normal, >1.0 = slow)
    TerrainType terrain = TerrainType::Ground;
    uint8_t customFlags = 0;          // 8 bits for custom gameplay flags
    
    // Multi-layer support
    CollisionLayer layer = CollisionLayer::Ground;
    
    // Dynamic state support
    bool isDynamic = false;           // Can this tile change state?
    std::string onDestroyedState;     // State name after destruction
    std::string onBuiltState;         // State name after construction
    std::string metadata;             // JSON metadata for custom logic
};
```

### **Field Descriptions**

| Field | Type | Description | Example Values |
|-------|------|-------------|----------------|
| `isBlocked` | `bool` | Hard collision | `true` (wall), `false` (floor) |
| `isNavigable` | `bool` | Can pathfind through | `true` (walkable), `false` (pit) |
| `traversalCost` | `float` | Movement cost multiplier | `1.0` (normal), `2.0` (mud), `0.5` (road) |
| `terrain` | `TerrainType` | Terrain classification | `Ground`, `Water`, `Grass`, `Rock` |
| `customFlags` | `uint8_t` | 8 custom bits | `0x01` (door open), `0x02` (trap armed) |
| `layer` | `CollisionLayer` | Collision layer | `Ground`, `Sky`, `Underground` |
| `isDynamic` | `bool` | Can change state | `true` (door), `false` (wall) |
| `onDestroyedState` | `string` | Post-destruction state | `"Rubble"`, `"Ashes"` |
| `onBuiltState` | `string` | Post-construction state | `"Bridge"`, `"Wall"` |
| `metadata` | `string` | Custom JSON data | `"{\"health\":100}"` |

---

## TerrainType Enum

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
```

### **Use Cases by Terrain Type**

| Terrain | Typical Cost | Use Case | Example |
|---------|--------------|----------|---------|
| `Ground` | 1.0 | Normal walkable floor | Stone path, dirt road |
| `Water` | 5.0+ | Slow movement or swimming | River, lake, ocean |
| `Grass` | 1.0 | Normal terrain | Field, meadow |
| `Sand` | 1.5 | Slightly slowed | Desert, beach |
| `Rock` | 2.0+ | Difficult terrain | Mountain, rubble |
| `Ice` | 0.8 | Fast but slippery | Frozen lake, glacier |
| `Lava` | 10.0+ | Extremely dangerous | Volcano, fire pit |
| `Mud` | 2.0 | Slow movement | Swamp, bog |
| `Snow` | 1.5 | Slowed movement | Tundra, snowfield |
| `Custom` | Variable | Game-specific | Teleporter, conveyor |

---

## CollisionLayer Enum

```cpp
enum class CollisionLayer : uint8_t
{
    Ground = 0,       // Layer 0: Standard ground collision
    Sky = 1,          // Layer 1: Aerial navigation/flying
    Underground = 2,  // Layer 2: Underground/tunnels
    Volume = 3,       // Layer 3: 3D volumes (for stacked isometric)
    Custom1 = 4,
    Custom2 = 5,
    Custom3 = 6,
    Custom4 = 7,
    MaxLayers = 8
};
```

### **Use Cases by Layer**

| Layer | Use Case | Example Scenario |
|-------|----------|------------------|
| `Ground` | Standard walking | Player movement, ground enemies |
| `Sky` | Flying units | Flying enemies, air vehicles |
| `Underground` | Tunnels/sewers | Underground passage, subway |
| `Volume` | Vertical stacking | Multi-floor buildings in isometric |
| `Custom1-4` | Game-specific | Water layer, ghost phase, teleport network |

### **Multi-Layer Example: Flying Enemy**

```cpp
// Ground layer: blocked by walls
CollisionMap::Get().SetActiveLayer(CollisionLayer::Ground);
CollisionMap::Get().SetCollision(10, 10, true); // Wall

// Sky layer: open air (no collision)
CollisionMap::Get().SetActiveLayer(CollisionLayer::Sky);
CollisionMap::Get().SetCollision(10, 10, false); // Open

// Flying enemy can pass over wall
NavigationAgent_data& flyingEnemy = ...;
flyingEnemy.layerMask = 0x02; // Bit 1 = Sky layer
// Enemy will pathfind on Sky layer, ignoring ground walls
```

---

## GridProjectionType Enum

```cpp
enum class GridProjectionType
{
    Ortho = 0,     // Orthogonal (square grid)
    Iso = 1,       // Isometric (diamond grid)
    HexAxial = 2   // Hexagonal (axial coordinates)
};
```

### **Projection Characteristics**

| Projection | Neighbors | Conversion Complexity | Use Case |
|------------|-----------|----------------------|----------|
| `Ortho` | 4 (or 8) | Simple | Top-down games, roguelikes |
| `Iso` | 4 | Medium | Isometric strategy, SimCity-style |
| `HexAxial` | 6 | High | Hex-based strategy, Civilization-style |

---

## Coordinate Systems

### **Orthogonal (Ortho)**

```cpp
// World-to-Grid
gridX = floor(worldX / tileWidth)
gridY = floor(worldY / tileHeight)

// Grid-to-World (center of tile)
worldX = (gridX + 0.5) * tileWidth
worldY = (gridY + 0.5) * tileHeight
```

**Example:**
```
tileWidth = 32, tileHeight = 32
worldPos = (256, 192)
gridPos = (8, 6)
```

---

### **Isometric (Iso)**

```cpp
// World-to-Grid
tileX = worldX / (tileWidth / 2) + worldY / (tileHeight / 2)
tileY = worldY / (tileHeight / 2) - worldX / (tileWidth / 2)
gridX = floor(tileX)
gridY = floor(tileY)

// Grid-to-World (center of tile)
worldX = (gridX - gridY) * (tileWidth / 2)
worldY = (gridX + gridY) * (tileHeight / 2)
```

**Example:**
```
tileWidth = 64, tileHeight = 32
worldPos = (640, 480)
gridPos = (25, 5)
```

---

### **Hexagonal (HexAxial)**

```cpp
// Pointy-top hexagon conversion
// (Complex axial coordinate math - handled internally)
```

---

## Complete Code Examples

### **Example 1: Basic Map Setup**

```cpp
// Initialize collision and navigation maps
CollisionMap& collisionMap = CollisionMap::Get();
NavigationMap& navMap = NavigationMap::Get();

int mapWidth = 100;
int mapHeight = 100;
float tileSize = 32.0f;

collisionMap.Initialize(mapWidth, mapHeight, GridProjectionType::Ortho, tileSize, tileSize, 1);
navMap.Initialize(mapWidth, mapHeight, GridProjectionType::Ortho, tileSize, tileSize, 1);

// Set up some walls
for (int x = 0; x < 10; ++x) {
    collisionMap.SetCollision(x, 5, true); // Horizontal wall
}

// Set up some slow terrain
TileProperties mudTile;
mudTile.isBlocked = false;
mudTile.isNavigable = true;
mudTile.terrain = TerrainType::Mud;
mudTile.traversalCost = 2.0f; // 2x slower

for (int y = 10; y < 15; ++y) {
    collisionMap.SetTileProperties(20, y, mudTile);
}
```

---

### **Example 2: Multi-Layer Setup**

```cpp
// Initialize with 3 layers
CollisionMap::Get().Initialize(50, 50, GridProjectionType::Ortho, 32.0f, 32.0f, 3);
NavigationMap::Get().Initialize(50, 50, GridProjectionType::Ortho, 32.0f, 32.0f, 3);

// Ground layer: walls block ground units
TileProperties groundWall;
groundWall.isBlocked = true;
groundWall.isNavigable = false;
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Ground, groundWall);

// Sky layer: open air for flying units
TileProperties skyOpen;
skyOpen.isBlocked = false;
skyOpen.isNavigable = true;
skyOpen.traversalCost = 0.8f; // Slightly faster
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Sky, skyOpen);

// Underground layer: tunnel blocked
TileProperties undergroundRock;
undergroundRock.isBlocked = true;
undergroundRock.isNavigable = false;
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Underground, undergroundRock);
```

---

### **Example 3: Basic Pathfinding**

```cpp
// Entity at world position (100, 150) wants to reach (800, 600)
Vector startWorld(100.0f, 150.0f, 0.0f);
Vector goalWorld(800.0f, 600.0f, 0.0f);

// Convert to grid coordinates
int startX, startY, goalX, goalY;
NavigationMap::Get().WorldToGrid(startWorld.x, startWorld.y, startX, startY);
NavigationMap::Get().WorldToGrid(goalWorld.x, goalWorld.y, goalX, goalY);

// Find path
std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(startX, startY, goalX, goalY, path);

if (found) {
    // Follow path waypoints
    for (const Vector& waypoint : path) {
        // Move entity towards waypoint
        std::cout << "Waypoint: (" << waypoint.x << ", " << waypoint.y << ")\n";
    }
} else {
    // No path found - handle failure
    std::cout << "No path to target!\n";
}
```

---

### **Example 4: Layer-Specific Pathfinding**

```cpp
// Flying enemy pathfinds on Sky layer
Vector startPos(200.0f, 200.0f, 0.0f);
Vector goalPos(600.0f, 400.0f, 0.0f);

int startX, startY, goalX, goalY;
NavigationMap::Get().WorldToGrid(startPos.x, startPos.y, startX, startY);
NavigationMap::Get().WorldToGrid(goalPos.x, goalPos.y, goalX, goalY);

std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(
    startX, startY,
    goalX, goalY,
    path,
    CollisionLayer::Sky, // Fly over ground obstacles
    5000
);

if (found) {
    // Flying enemy can ignore ground walls
}
```

---

### **Example 5: Custom Flags (Bit Manipulation)**

```cpp
// Define custom flags
enum CustomFlags : uint8_t
{
    FLAG_DOOR_OPEN = 0x01,   // Bit 0
    FLAG_TRAP_ARMED = 0x02,  // Bit 1
    FLAG_SECRET_DOOR = 0x04, // Bit 2
    FLAG_TELEPORTER = 0x08,  // Bit 3
    // ... up to 8 flags
};

// Set multiple flags
TileProperties tile;
tile.customFlags = FLAG_DOOR_OPEN | FLAG_SECRET_DOOR; // Bits 0 and 2 set

CollisionMap::Get().SetTileProperties(15, 20, tile);

// Check flags
const TileProperties& checkTile = CollisionMap::Get().GetTileProperties(15, 20);
if (checkTile.customFlags & FLAG_DOOR_OPEN) {
    // Door is open
}
if (checkTile.customFlags & FLAG_TRAP_ARMED) {
    // Trap is armed
}

// Toggle flag
CollisionMap::Get().UpdateTileState(15, 20, [](TileProperties& t) {
    t.customFlags ^= FLAG_DOOR_OPEN; // Toggle bit 0
});
```

---

### **Example 6: Destructible Wall**

```cpp
// Wall starts as solid rock
TileProperties wall;
wall.isBlocked = true;
wall.isNavigable = false;
wall.terrain = TerrainType::Rock;
wall.isDynamic = true;
wall.onDestroyedState = "Rubble";
CollisionMap::Get().SetTileProperties(25, 30, wall);

// ... later, wall is destroyed by explosion ...

// Update to rubble (passable but slow)
CollisionMap::Get().UpdateTileState(25, 30, [](TileProperties& tile) {
    tile.isBlocked = false;
    tile.isNavigable = true;
    tile.traversalCost = 1.5f; // Rubble is harder to walk through
    tile.terrain = TerrainType::Rock;
    tile.onDestroyedState = ""; // Already destroyed
});

// Existing paths are now invalid - agents should repath
```

---

### **Example 7: Buildable Bridge**

```cpp
// Water tile (not navigable)
TileProperties water;
water.isBlocked = false;
water.isNavigable = false; // Can't walk on water
water.terrain = TerrainType::Water;
water.isDynamic = true;
water.onBuiltState = "Bridge";
CollisionMap::Get().SetTileProperties(10, 15, water);

// ... player builds bridge ...

// Update to bridge (navigable)
CollisionMap::Get().UpdateTileState(10, 15, [](TileProperties& tile) {
    tile.isNavigable = true;
    tile.traversalCost = 1.0f;
    tile.terrain = TerrainType::Ground;
    tile.onBuiltState = ""; // Already built
    tile.customFlags = 0x01; // Mark as bridge
});

// Now entities can pathfind across the water
```

---

### **Example 8: Openable Door with State**

```cpp
// Door entity with position
Vector doorPos(480.0f, 320.0f, 0.0f);
int doorX, doorY;
CollisionMap::Get().WorldToGrid(doorPos.x, doorPos.y, doorX, doorY);

// Initial state: door closed
TileProperties closedDoor;
closedDoor.isBlocked = true;
closedDoor.isNavigable = false;
closedDoor.isDynamic = true;
closedDoor.customFlags = 0x00; // Bit 0 = door open
CollisionMap::Get().SetTileProperties(doorX, doorY, closedDoor);

// Toggle door function
auto ToggleDoor = [doorX, doorY]() {
    CollisionMap::Get().UpdateTileState(doorX, doorY, [](TileProperties& tile) {
        bool isOpen = (tile.customFlags & 0x01) != 0;
        isOpen = !isOpen;
        
        tile.isBlocked = !isOpen;
        tile.isNavigable = isOpen;
        tile.customFlags = isOpen ? 0x01 : 0x00;
    });
};

// Player interacts with door
ToggleDoor(); // Opens
ToggleDoor(); // Closes
```

---

### **Example 9: Terrain-Based Movement Cost**

```cpp
// Set up varied terrain
struct TerrainSetup {
    TerrainType type;
    float cost;
    int startX, startY, endX, endY;
};

TerrainSetup terrains[] = {
    { TerrainType::Ground, 1.0f, 0, 0, 20, 20 },   // Normal ground
    { TerrainType::Mud, 2.0f, 20, 0, 40, 20 },     // Slow mud
    { TerrainType::Sand, 1.5f, 40, 0, 60, 20 },    // Sandy desert
    { TerrainType::Ice, 0.8f, 60, 0, 80, 20 },     // Slippery ice (faster)
    { TerrainType::Grass, 1.0f, 0, 20, 20, 40 },   // Normal grass
};

for (const auto& setup : terrains) {
    TileProperties tile;
    tile.isBlocked = false;
    tile.isNavigable = true;
    tile.terrain = setup.type;
    tile.traversalCost = setup.cost;
    
    for (int y = setup.startY; y < setup.endY; ++y) {
        for (int x = setup.startX; x < setup.endX; ++x) {
            CollisionMap::Get().SetTileProperties(x, y, tile);
        }
    }
}

// Pathfinding will automatically prefer ice (faster) over mud (slower)
```

---

### **Example 10: Isometric Map Setup**

```cpp
// Initialize isometric map
CollisionMap::Get().Initialize(
    58, 27, // Map dimensions in tiles
    GridProjectionType::Iso,
    184.0f, 128.0f, // Tile width/height
    1
);
NavigationMap::Get().Initialize(58, 27, GridProjectionType::Iso, 184.0f, 128.0f, 1);

// Convert isometric world position to grid
Vector isoWorldPos(810.0f, 1441.0f, 0.0f);
int gridX, gridY;
CollisionMap::Get().WorldToGrid(isoWorldPos.x, isoWorldPos.y, gridX, gridY);
// Grid position calculated using isometric formulas

// Set collision on isometric tile
CollisionMap::Get().SetCollision(gridX, gridY, true);
```

---

### **Example 11: Performance-Constrained Pathfinding**

```cpp
// Long-distance pathfinding with iteration limit
Vector start(100.0f, 100.0f, 0.0f);
Vector goal(3000.0f, 3000.0f, 0.0f);

int startX, startY, goalX, goalY;
NavigationMap::Get().WorldToGrid(start.x, start.y, startX, startY);
NavigationMap::Get().WorldToGrid(goal.x, goal.y, goalX, goalY);

std::vector<Vector> path;
bool found = NavigationMap::Get().FindPath(
    startX, startY,
    goalX, goalY,
    path,
    CollisionLayer::Ground,
    2000 // Limit iterations to prevent frame drops
);

if (!found) {
    // Partial path may be available - use hierarchical pathfinding
    // or subdivide into multiple shorter paths
}
```

---

### **Example 12: Layer Mask for Multi-Layer Navigation**

```cpp
// Agent that can navigate both ground and underground
NavigationAgent_data& agent = ...;

// Layer mask: bits 0 and 2 (Ground + Underground)
agent.layerMask = 0x01 | 0x04; // 0b00000101

// When agent requests path, system checks first active layer
// Pathfinding will prefer Ground layer if available, fall back to Underground
```

---

## Best Practices

### **1. Initialize Once**
```cpp
// During level load
CollisionMap::Get().Initialize(...);
NavigationMap::Get().Initialize(...);
// Both should match dimensions and projection
```

### **2. Clear on Level Unload**
```cpp
// During level unload
CollisionMap::Get().Clear();
NavigationMap::Get().Clear();
```

### **3. Validate Coordinates**
```cpp
if (CollisionMap::Get().IsValidGridPosition(x, y)) {
    // Safe to access
}
```

### **4. Use Layer-Specific Access for Performance**
```cpp
// Efficient: direct layer access
auto tile = CollisionMap::Get().GetTileProperties(x, y, CollisionLayer::Ground);

// Less efficient: set active layer first
CollisionMap::Get().SetActiveLayer(CollisionLayer::Ground);
auto tile = CollisionMap::Get().GetTileProperties(x, y);
```

### **5. Limit Pathfinding Iterations**
```cpp
// For real-time games, limit iterations to maintain framerate
bool found = NavigationMap::Get().FindPath(..., path, layer, 5000);
```

### **6. Repath on Dynamic Changes**
```cpp
// When destroying a wall or opening a door
CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    tile.isBlocked = false;
    tile.isNavigable = true;
});

// Mark all agents as needing repath
for (auto& agent : agents) {
    agent.needsRepath = true;
}
```

### **7. Use Custom Flags for Game Logic**
```cpp
// Define flags in a central enum
enum GameFlags : uint8_t {
    FLAG_DOOR = 0x01,
    FLAG_TRAP = 0x02,
    FLAG_TELEPORTER = 0x04,
    // ...
};

// Check flags efficiently
if (tile.customFlags & FLAG_TRAP) {
    // Trigger trap
}
```

---

## Performance Considerations

1. **Pathfinding is O(N log N)**: Use `maxIterations` to cap computation time
2. **Cache paths**: Avoid re-pathfinding every frame
3. **Layer-specific access**: Faster than switching active layer repeatedly
4. **World-to-Grid is fast**: No performance penalty for frequent conversions
5. **UpdateTileState is thread-safe**: Lambda ensures atomic updates

---

## See Also

- [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md)
- [Collision Types Reference](Collision_Types_Reference.md)
- [Adding ECS Components Guide](Adding_ECS_Components_Guide.md)
