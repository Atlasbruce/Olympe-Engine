---
id: pathfinding
title: Pathfinding System
sidebar_label: Pathfinding
sidebar_position: 1
---

# Pathfinding System

**Version:** 2.0  
**Last Updated:** 2025-02-13

## Overview

Olympe Engine features a robust A* pathfinding system that operates on tile-based navigation meshes. The system supports multiple projection types (orthogonal, isometric, hexagonal), multi-layer navigation, and variable traversal costs.

Key features:
- **A* algorithm**: Optimal pathfinding with heuristic optimization
- **Multi-grid support**: Orthogonal, isometric, and hexagonal grids
- **Multi-layer navigation**: Ground, sky, underground navigation layers
- **Traversal costs**: Variable movement costs per tile (mud, water, etc.)
- **Dynamic obstacles**: Runtime updates to navigation mesh
- **ECS integration**: Seamless integration with entity movement systems

## Architecture

### Core Components

```
NavigationMap (Singleton)
    ↓
CollisionMap (Singleton)
    ↓
TileProperties (Per-tile data)
    ↓
Pathfinding Algorithms (A*)
```

### Class Structure

```cpp
// Source/CollisionMap.h
class NavigationMap
{
public:
    static NavigationMap& Get();
    
    // Pathfinding
    bool FindPath(int startX, int startY, int goalX, int goalY,
                 std::vector<Vector>& outPath, 
                 CollisionLayer layer = CollisionLayer::Ground,
                 int maxIterations = 10000);
    
    // Utilities
    bool GetRandomNavigablePoint(float centerX, float centerY, float radius,
                                 int maxAttempts, float& outX, float& outY,
                                 CollisionLayer layer = CollisionLayer::Ground) const;
    
    void SetNavigable(int x, int y, bool isNavigable, float cost = 1.0f);
    bool IsNavigable(int x, int y) const;
    float GetTraversalCost(int x, int y) const;
};
```

---

## A* Algorithm Implementation

### Algorithm Overview

A* (A-star) is a best-first search algorithm that finds the shortest path between two points. It uses:
- **g(n)**: Actual cost from start to node n
- **h(n)**: Heuristic estimated cost from node n to goal
- **f(n) = g(n) + h(n)**: Total estimated cost through node n

### Data Structures

```cpp
struct PathNode
{
    int x, y;
    float gCost;  // Cost from start
    float hCost;  // Heuristic to goal
    float fCost() const { return gCost + hCost; }
    PathNode* parent;
    
    PathNode() : x(0), y(0), gCost(0), hCost(0), parent(nullptr) {}
    PathNode(int _x, int _y) : x(_x), y(_y), gCost(0), hCost(0), parent(nullptr) {}
};
```

### Priority Queue

Uses STL priority queue (min-heap) for efficient node selection:

```cpp
struct NodeCompare
{
    bool operator()(const PathNode* a, const PathNode* b) const
    {
        return a->fCost() > b->fCost(); // Min-heap (lower fCost = higher priority)
    }
};

std::priority_queue<PathNode*, std::vector<PathNode*>, NodeCompare> openSet;
```

### Core Algorithm

```cpp
bool NavigationMap::FindPath(int startX, int startY, int goalX, int goalY,
                             std::vector<Vector>& outPath, CollisionLayer layer, int maxIterations)
{
    outPath.clear();
    
    // 1. Validate positions
    if (!IsValidGridPosition(startX, startY) || !IsValidGridPosition(goalX, goalY))
        return false;
    
    if (!IsNavigable(startX, startY, layer) || !IsNavigable(goalX, goalY, layer))
        return false;
    
    // 2. Early exit if start == goal
    if (startX == goalX && startY == goalY)
    {
        float worldX, worldY;
        GridToWorld(startX, startY, worldX, worldY);
        outPath.push_back(Vector(worldX, worldY, 0.0f));
        return true;
    }
    
    // 3. Initialize data structures
    std::priority_queue<PathNode*, std::vector<PathNode*>, NodeCompare> openSet;
    std::unordered_set<int> closedSet;
    std::unordered_map<int, PathNode*> allNodes;
    
    // 4. Create start node
    PathNode* startNode = new PathNode(startX, startY);
    startNode->gCost = 0.0f;
    startNode->hCost = Heuristic(startX, startY, goalX, goalY);
    startNode->parent = nullptr;
    
    openSet.push(startNode);
    allNodes[encodePos(startX, startY)] = startNode;
    
    PathNode* goalNode = nullptr;
    int iterations = 0;
    
    // 5. Main loop
    while (!openSet.empty() && iterations < maxIterations)
    {
        ++iterations;
        
        // Get node with lowest fCost
        PathNode* current = openSet.top();
        openSet.pop();
        
        int currentKey = encodePos(current->x, current->y);
        
        // Skip if already processed
        if (closedSet.find(currentKey) != closedSet.end())
            continue;
        
        closedSet.insert(currentKey);
        
        // Check if goal reached
        if (current->x == goalX && current->y == goalY)
        {
            goalNode = current;
            break;
        }
        
        // Explore neighbors
        std::vector<std::pair<int, int>> neighbors;
        GetNeighbors(current->x, current->y, neighbors);
        
        for (const std::pair<int, int>& neighbor : neighbors)
        {
            int nx = neighbor.first;
            int ny = neighbor.second;
            int neighborKey = encodePos(nx, ny);
            
            // Skip if invalid or already processed
            if (!IsValidGridPosition(nx, ny) || !IsNavigable(nx, ny, layer))
                continue;
            
            if (closedSet.find(neighborKey) != closedSet.end())
                continue;
            
            // Calculate tentative gCost
            float moveCost = GetTraversalCost(nx, ny, layer);
            float tentativeG = current->gCost + moveCost;
            
            // Check if we found a better path
            PathNode* neighborNode = nullptr;
            if (allNodes.find(neighborKey) != allNodes.end())
            {
                neighborNode = allNodes[neighborKey];
                if (tentativeG >= neighborNode->gCost)
                    continue; // Not a better path
            }
            else
            {
                neighborNode = new PathNode(nx, ny);
                allNodes[neighborKey] = neighborNode;
            }
            
            // Update node
            neighborNode->gCost = tentativeG;
            neighborNode->hCost = Heuristic(nx, ny, goalX, goalY);
            neighborNode->parent = current;
            
            openSet.push(neighborNode);
        }
    }
    
    // 6. Reconstruct path
    bool pathFound = false;
    if (goalNode != nullptr)
    {
        std::vector<PathNode*> pathNodes;
        PathNode* current = goalNode;
        while (current != nullptr)
        {
            pathNodes.push_back(current);
            current = current->parent;
        }
        
        // Reverse to get start->goal order
        std::reverse(pathNodes.begin(), pathNodes.end());
        
        // Convert to world coordinates
        for (PathNode* node : pathNodes)
        {
            float worldX, worldY;
            GridToWorld(node->x, node->y, worldX, worldY);
            outPath.push_back(Vector(worldX, worldY, 0.0f));
        }
        
        pathFound = true;
    }
    
    // 7. Cleanup
    for (std::pair<const int, PathNode*>& entry : allNodes)
    {
        delete entry.second;
    }
    
    return pathFound;
}
```

---

## Heuristic Functions

The heuristic function `h(n)` estimates the cost from a node to the goal. Different heuristics are used for different grid types.

### Orthogonal Grid: Manhattan Distance

4-connected grid (up, down, left, right):

```cpp
case GridProjectionType::Ortho:
    // Manhattan distance
    return static_cast<float>(std::abs(x2 - x1) + std::abs(y2 - y1));
```

**Formula:** `h(n) = |x2 - x1| + |y2 - y1|`

**Properties:**
- Admissible (never overestimates)
- Optimal for 4-connected grids
- No diagonal movement allowed

### Isometric Grid: Chebyshev Distance

4-connected diamond grid:

```cpp
case GridProjectionType::Iso:
    // Chebyshev distance (max of absolute differences)
    return static_cast<float>(std::max(std::abs(x2 - x1), std::abs(y2 - y1)));
```

**Formula:** `h(n) = max(|x2 - x1|, |y2 - y1|)`

**Properties:**
- Admissible for isometric grids
- Accounts for diamond-shaped movement
- Optimal for isometric pathfinding

### Hexagonal Grid: Axial Distance

6-connected hexagonal grid:

```cpp
case GridProjectionType::HexAxial:
    // Axial distance (hexagonal grid)
    int dx = x2 - x1;
    int dy = y2 - y1;
    return static_cast<float>((std::abs(dx) + std::abs(dx + dy) + std::abs(dy)) / 2);
```

**Formula:** `h(n) = (|dx| + |dx + dy| + |dy|) / 2`

**Properties:**
- Admissible for hexagonal grids
- Accounts for 6-way connectivity
- Based on cube coordinate system

---

## Neighbor Generation

Different grid types have different neighbor connectivity patterns.

### Orthogonal Grid (4-connected)

```cpp
case GridProjectionType::Ortho:
    outNeighbors.push_back(std::make_pair(x, y - 1)); // Up
    outNeighbors.push_back(std::make_pair(x, y + 1)); // Down
    outNeighbors.push_back(std::make_pair(x - 1, y)); // Left
    outNeighbors.push_back(std::make_pair(x + 1, y)); // Right
    break;
```

```
    [N]
[W] [·] [E]
    [S]
```

### Isometric Grid (4-connected)

```cpp
case GridProjectionType::Iso:
    outNeighbors.push_back(std::make_pair(x - 1, y)); // NW
    outNeighbors.push_back(std::make_pair(x + 1, y)); // SE
    outNeighbors.push_back(std::make_pair(x, y - 1)); // NE
    outNeighbors.push_back(std::make_pair(x, y + 1)); // SW
    break;
```

```
    [NE]
[NW] [·] [SE]
    [SW]
```

### Hexagonal Grid (6-connected)

```cpp
case GridProjectionType::HexAxial:
    outNeighbors.push_back(std::make_pair(x + 1, y));     // E
    outNeighbors.push_back(std::make_pair(x + 1, y - 1)); // NE
    outNeighbors.push_back(std::make_pair(x, y - 1));     // NW
    outNeighbors.push_back(std::make_pair(x - 1, y));     // W
    outNeighbors.push_back(std::make_pair(x - 1, y + 1)); // SW
    outNeighbors.push_back(std::make_pair(x, y + 1));     // SE
    break;
```

```
   [NW] [NE]
[W] [·] [E]
   [SW] [SE]
```

---

## Traversal Costs

Tiles can have variable traversal costs to model different terrain types.

### Tile Properties

```cpp
struct TileProperties
{
    bool isBlocked = false;       // Hard collision (impassable)
    bool isNavigable = true;      // Can pathfind through
    float traversalCost = 1.0f;   // Cost for pathfinding
    TerrainType terrain = TerrainType::Ground;
    // ...
};
```

### Terrain Costs

| Terrain | Cost | Description |
|---------|------|-------------|
| Ground | 1.0 | Normal ground |
| Grass | 1.0 | Open grass |
| Sand | 1.2 | Slightly slower |
| Mud | 1.5 | Slows movement |
| Water | 2.0 | Very slow (wading) |
| Ice | 0.8 | Fast but slippery |
| Snow | 1.3 | Slows movement |
| Rock | 1.1 | Slightly rough |

### Setting Traversal Costs

```cpp
// Set tile as navigable with custom cost
NavigationMap::Get().SetNavigable(gridX, gridY, true, 1.5f); // Mud tile

// Or set via TileProperties
TileProperties props;
props.isNavigable = true;
props.traversalCost = 2.0f; // Water tile
CollisionMap::Get().SetTileProperties(gridX, gridY, props);
```

### Cost in Pathfinding

During pathfinding, the traversal cost is added to g(n):

```cpp
float moveCost = GetTraversalCost(nx, ny, layer);
float tentativeG = current->gCost + moveCost;
```

**Example:**
- Moving across 3 grass tiles (cost 1.0 each) = total cost 3.0
- Moving across 3 mud tiles (cost 1.5 each) = total cost 4.5
- A* will prefer routes with lower total cost

---

## Multi-Layer Navigation

The navigation system supports multiple independent navigation layers.

### Layer Types

```cpp
enum class CollisionLayer : uint8_t
{
    Ground = 0,       // Standard ground navigation
    Sky = 1,          // Aerial navigation (flying units)
    Underground = 2,  // Underground/tunnels
    Volume = 3,       // 3D volumes (stacked isometric)
    Custom1 = 4,
    Custom2 = 5,
    Custom3 = 6,
    Custom4 = 7,
    MaxLayers = 8
};
```

### Using Layers

```cpp
// Pathfind on ground layer
std::vector<Vector> groundPath;
NavigationMap::Get().FindPath(startX, startY, goalX, goalY, groundPath, CollisionLayer::Ground);

// Pathfind on sky layer (for flying units)
std::vector<Vector> skyPath;
NavigationMap::Get().FindPath(startX, startY, goalX, goalY, skyPath, CollisionLayer::Sky);
```

### Layer Configuration

```cpp
// Initialize with multiple layers
CollisionMap::Get().Initialize(width, height, GridProjectionType::Ortho, 
                              tileWidth, tileHeight, 3); // 3 layers

// Set active layer
NavigationMap::Get().SetActiveLayer(CollisionLayer::Sky);

// Set navigation properties per layer
NavigationMap::Get().SetNavigable(x, y, true, 1.0f); // Active layer
```

### Use Cases

- **Ground layer**: Walking units
- **Sky layer**: Flying units (ignore ground obstacles)
- **Underground layer**: Burrowing units
- **Custom layers**: Game-specific navigation (e.g., water-only for ships)

---

## Random Navigable Point Selection

Useful for wander behaviors, spawn points, and procedural placement.

### API

```cpp
bool GetRandomNavigablePoint(float centerX, float centerY, float radius,
                             int maxAttempts, float& outX, float& outY,
                             CollisionLayer layer = CollisionLayer::Ground) const;
```

### Implementation

```cpp
bool NavigationMap::GetRandomNavigablePoint(float centerX, float centerY, float radius,
                                            int maxAttempts, float& outX, float& outY,
                                            CollisionLayer layer) const
{
    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        // 1. Generate random angle (0 to 2π)
        float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * 3.14159265f;
        
        // 2. Generate random distance (0 to radius)
        // Use sqrt for uniform distribution
        float randomRadius = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * radius;
        
        // 3. Calculate world position
        float worldX = centerX + randomRadius * std::cos(angle);
        float worldY = centerY + randomRadius * std::sin(angle);
        
        // 4. Convert to grid coordinates
        int gridX, gridY;
        WorldToGrid(worldX, worldY, gridX, gridY);
        
        // 5. Check if navigable
        if (IsValidGridPosition(gridX, gridY) && IsNavigable(gridX, gridY, layer))
        {
            outX = worldX;
            outY = worldY;
            return true;
        }
    }
    
    return false; // Failed after maxAttempts
}
```

### Usage Example

```cpp
// Find random navigable point within 200 units
float destX, destY;
bool found = NavigationMap::Get().GetRandomNavigablePoint(
    entity.position.x, entity.position.y,
    200.0f,  // radius
    10,      // maxAttempts
    destX, destY
);

if (found)
{
    // Set as movement goal
    blackboard.wanderDestination = Vector(destX, destY);
    blackboard.hasWanderDestination = true;
}
```

### Why sqrt() for Uniform Distribution?

Without `sqrt()`, random points cluster toward center:

```cpp
// WRONG: Clusters toward center
float randomRadius = (rand() / RAND_MAX) * radius;

// CORRECT: Uniform distribution
float randomRadius = std::sqrt(rand() / RAND_MAX) * radius;
```

This is because the area of a ring increases with radius, so we need to bias selection toward outer rings.

---

## Performance Considerations

### Iteration Limit

Default: 10,000 iterations to prevent infinite loops on large maps.

```cpp
bool FindPath(/* ... */, int maxIterations = 10000);
```

For very large maps, increase the limit:

```cpp
NavigationMap::Get().FindPath(startX, startY, goalX, goalY, path, 
                             CollisionLayer::Ground, 50000);
```

### Path Caching

Cache paths that haven't changed:

```cpp
struct NavigationAgent_data
{
    std::vector<Vector> currentPath;
    Vector cachedGoal;
    bool pathDirty = true;
};

// Only recalculate if goal changed
if (agent.pathDirty || agent.cachedGoal != newGoal)
{
    NavigationMap::Get().FindPath(/* ... */, agent.currentPath);
    agent.cachedGoal = newGoal;
    agent.pathDirty = false;
}
```

### Hierarchical Pathfinding

For very large maps, use hierarchical pathfinding:

1. **High-level graph**: Connect navigation regions
2. **Low-level A***: Pathfind within regions

```cpp
// 1. Find high-level path between regions
std::vector<int> regionPath = FindRegionPath(startRegion, goalRegion);

// 2. Pathfind within each region
for (int i = 0; i < regionPath.size() - 1; ++i)
{
    Vector entry = GetRegionEntry(regionPath[i], regionPath[i+1]);
    Vector exit = GetRegionExit(regionPath[i], regionPath[i+1]);
    
    std::vector<Vector> subPath;
    NavigationMap::Get().FindPath(entry, exit, subPath);
    fullPath.insert(fullPath.end(), subPath.begin(), subPath.end());
}
```

### Memory Usage

A* allocates nodes dynamically. For high-frequency pathfinding, use object pooling:

```cpp
class PathNodePool
{
public:
    PathNode* Allocate()
    {
        if (m_freeNodes.empty())
        {
            return new PathNode();
        }
        PathNode* node = m_freeNodes.back();
        m_freeNodes.pop_back();
        return node;
    }
    
    void Free(PathNode* node)
    {
        node->parent = nullptr;
        m_freeNodes.push_back(node);
    }
    
private:
    std::vector<PathNode*> m_freeNodes;
};
```

---

## Dynamic Obstacles

Update navigation mesh at runtime for destructible walls, buildable bridges, etc.

### Example: Destructible Wall

```cpp
void DestroyWall(int tileX, int tileY)
{
    // Make tile navigable
    NavigationMap::Get().SetNavigable(tileX, tileY, true, 1.0f);
    
    // Invalidate cached paths that might use this tile
    InvalidatePathsNearTile(tileX, tileY);
}

void InvalidatePathsNearTile(int tileX, int tileY)
{
    // Mark all entities with paths near this tile as needing re-path
    auto entities = World::Get().GetAllEntitiesWithComponents<NavigationAgent_data, Position_data>();
    
    for (EntityID entity : entities)
    {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        
        // Check if path goes through changed tile
        for (const Vector& waypoint : agent.currentPath)
        {
            int wx, wy;
            NavigationMap::Get().WorldToGrid(waypoint.x, waypoint.y, wx, wy);
            
            if (std::abs(wx - tileX) <= 1 && std::abs(wy - tileY) <= 1)
            {
                agent.pathDirty = true;
                break;
            }
        }
    }
}
```

### Example: Buildable Bridge

```cpp
void BuildBridge(int tileX, int tileY)
{
    // Change tile from water (cost 2.0) to bridge (cost 1.0)
    NavigationMap::Get().SetNavigable(tileX, tileY, true, 1.0f);
    
    // Update visual representation
    SetTileSprite(tileX, tileY, SPRITE_BRIDGE);
    
    // Invalidate paths
    InvalidatePathsNearTile(tileX, tileY);
}
```

---

## ECS Integration

### Components

```cpp
// Source/ECS_Components.h
struct MoveIntent_data
{
    Vector targetPosition;
    float desiredSpeed = 1.0f;
    bool hasIntent = false;
    bool usePathfinding = false;  // Enable A* pathfinding
    bool avoidObstacles = true;
    float arrivalThreshold = 5.0f;
};

struct NavigationAgent_data
{
    std::vector<Vector> currentPath;
    int currentWaypointIndex = 0;
    bool hasPath = false;
};
```

### Requesting Pathfinding

```cpp
// Set move intent with pathfinding
MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
intent.targetPosition = targetPos;
intent.desiredSpeed = 1.0f;
intent.hasIntent = true;
intent.usePathfinding = true;  // Enable pathfinding
```

### Navigation System

```cpp
void NavigationSystem_Update()
{
    auto entities = World::Get().GetAllEntitiesWithComponents<
        NavigationAgent_data, MoveIntent_data, Position_data>();
    
    for (EntityID entity : entities)
    {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        
        // If pathfinding requested and path is invalid
        if (intent.usePathfinding && (!agent.hasPath || agent.pathDirty))
        {
            // Convert positions to grid
            int startX, startY, goalX, goalY;
            NavigationMap::Get().WorldToGrid(pos.position.x, pos.position.y, startX, startY);
            NavigationMap::Get().WorldToGrid(intent.targetPosition.x, intent.targetPosition.y, goalX, goalY);
            
            // Find path
            bool found = NavigationMap::Get().FindPath(startX, startY, goalX, goalY, agent.currentPath);
            
            if (found)
            {
                agent.hasPath = true;
                agent.currentWaypointIndex = 0;
                agent.pathDirty = false;
            }
            else
            {
                // Path not found - disable intent
                intent.hasIntent = false;
            }
        }
        
        // Follow path
        if (agent.hasPath && agent.currentWaypointIndex < agent.currentPath.size())
        {
            Vector currentWaypoint = agent.currentPath[agent.currentWaypointIndex];
            float dist = (pos.position - currentWaypoint).Magnitude();
            
            // Reached waypoint?
            if (dist < 5.0f)
            {
                agent.currentWaypointIndex++;
                
                // Reached final waypoint?
                if (agent.currentWaypointIndex >= agent.currentPath.size())
                {
                    agent.hasPath = false;
                    intent.hasIntent = false;
                }
            }
            else
            {
                // Move toward waypoint
                Vector direction = (currentWaypoint - pos.position).Normalized();
                pos.position += direction * intent.desiredSpeed * GameEngine::fDt;
            }
        }
    }
}
```

---

## Debugging and Visualization

### Debug Path Rendering

```cpp
void RenderDebugPath(const std::vector<Vector>& path)
{
    if (path.empty()) return;
    
    for (size_t i = 0; i < path.size() - 1; ++i)
    {
        const Vector& start = path[i];
        const Vector& end = path[i + 1];
        
        // Draw line between waypoints
        DrawLine(start.x, start.y, end.x, end.y, Color::Green);
        
        // Draw waypoint circles
        DrawCircle(start.x, start.y, 3.0f, Color::Yellow);
    }
    
    // Draw final waypoint
    if (!path.empty())
    {
        const Vector& final = path.back();
        DrawCircle(final.x, final.y, 5.0f, Color::Red);
    }
}
```

### Navigation Mesh Visualization

```cpp
void RenderNavigationMesh()
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
                
                // Color by cost
                Color color = Color::Green;
                if (cost > 1.5f)
                    color = Color::Red;    // High cost
                else if (cost > 1.0f)
                    color = Color::Yellow; // Medium cost
                
                DrawTile(worldX, worldY, color, 0.3f); // Semi-transparent
            }
            else
            {
                // Non-navigable (blocked)
                DrawTile(worldX, worldY, Color::Black, 0.5f);
            }
        }
    }
}
```

---

## Best Practices

### 1. Always Validate Positions

```cpp
// GOOD: Check if positions are valid
if (!NavigationMap::Get().IsValidGridPosition(startX, startY))
{
    SYSTEM_LOG << "ERROR: Invalid start position\n";
    return false;
}

// BAD: Assume positions are valid
int gridX = worldX / tileWidth; // May be out of bounds!
```

### 2. Check Navigability

```cpp
// GOOD: Check if start and goal are navigable
if (!NavigationMap::Get().IsNavigable(startX, startY) ||
    !NavigationMap::Get().IsNavigable(goalX, goalY))
{
    return false; // Can't pathfind
}

// BAD: Pathfind to blocked tile
NavigationMap::Get().FindPath(startX, startY, blockedX, blockedY, path); // Fails!
```

### 3. Set Appropriate Iteration Limits

```cpp
// GOOD: Adjust limit based on map size
int mapSize = width * height;
int iterations = std::min(mapSize, 50000);
NavigationMap::Get().FindPath(/* ... */, iterations);

// BAD: Always use default (may timeout on large maps)
NavigationMap::Get().FindPath(/* ... */); // Default 10k may be too small
```

### 4. Cache Paths When Possible

```cpp
// GOOD: Only recalculate when necessary
if (agent.pathDirty || newGoal != agent.cachedGoal)
{
    NavigationMap::Get().FindPath(/* ... */);
    agent.pathDirty = false;
}

// BAD: Recalculate every frame
NavigationMap::Get().FindPath(/* ... */); // Very expensive!
```

### 5. Use Appropriate Layer

```cpp
// GOOD: Flying unit uses Sky layer
NavigationMap::Get().FindPath(/* ... */, CollisionLayer::Sky);

// BAD: Flying unit uses Ground layer (blocked by walls)
NavigationMap::Get().FindPath(/* ... */, CollisionLayer::Ground);
```

---

## See Also

- [Navigation Mesh](./navigation-mesh.md) - Navigation mesh generation from Tiled maps
- [Behavior Tree Nodes](../behavior-trees/nodes.md) - AI pathfinding integration
- [Collision System](../architecture/collision.md) - Collision detection
- [Tiled Map Format](../../user-guide/tiled-editor/map-format.md) - Map data structure
