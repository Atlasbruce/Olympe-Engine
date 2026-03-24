# Pathfinding Integration Guide

## Table of Contents
- [Introduction](#introduction)
- [When to Use Pathfinding](#when-to-use-pathfinding)
- [Requesting a Path](#requesting-a-path)
- [Handling Path Results](#handling-path-results)
- [Following a Path](#following-a-path)
- [Repathing Strategies](#repathing-strategies)
- [Multi-Layer Pathfinding](#multi-layer-pathfinding)
- [Performance Tips](#performance-tips)
- [Complete Integration Examples](#complete-integration-examples)

---

## Introduction

This guide provides step-by-step instructions for integrating pathfinding into your game using the Olympe Engine's navigation system. The system uses A* pathfinding on a multi-layer grid, with support for dynamic obstacles, varied terrain costs, and multiple navigation layers.

### **Key Components**

1. **CollisionMap**: Stores tile properties (collision, navigability, terrain type)
2. **NavigationMap**: Provides A* pathfinding using CollisionMap data
3. **NavigationAgent_data**: ECS component for entity pathfinding state
4. **NavigationSystem**: ECS system that handles pathfinding and path following

---

## When to Use Pathfinding

### **Use Pathfinding For:**

✅ **AI Navigation**
- Enemies finding player
- NPCs moving between locations
- Guards patrolling waypoints
- Allies following player

✅ **Click-to-Move**
- RTS unit commands
- Point-and-click adventures
- Top-down action games

✅ **Complex Environments**
- Maps with walls and obstacles
- Multi-floor buildings
- Mazes and dungeons

✅ **Dynamic Obstacles**
- Destructible walls
- Doors that open/close
- Moving platforms

### **Avoid Pathfinding For:**

❌ **Direct Line-of-Sight**
- Projectiles (use raycasts)
- Straight-line movement
- Simple chase behavior (when no obstacles)

❌ **Very Short Distances**
- Adjacent tile movement
- Sub-tile precision

❌ **Real-Time Requirements**
- Frame-by-frame steering
- High-frequency updates (> 30 Hz)

---

## Requesting a Path

### **Basic Path Request**

```cpp
// Step 1: Get NavigationMap instance
NavigationMap& navMap = NavigationMap::Get();

// Step 2: Convert world positions to grid coordinates
Vector startWorld(100.0f, 150.0f, 0.0f);
Vector goalWorld(800.0f, 600.0f, 0.0f);

int startX, startY, goalX, goalY;
navMap.WorldToGrid(startWorld.x, startWorld.y, startX, startY);
navMap.WorldToGrid(goalWorld.x, goalWorld.y, goalX, goalY);

// Step 3: Request path
std::vector<Vector> path;
bool success = navMap.FindPath(
    startX, startY,  // Start grid coordinates
    goalX, goalY,    // Goal grid coordinates
    path,            // Output path (world coordinates)
    CollisionLayer::Ground, // Layer to pathfind on
    10000            // Max iterations (performance limit)
);

// Step 4: Handle result
if (success) {
    // Path found - use waypoints
} else {
    // No path - handle failure
}
```

### **Path Request Parameters**

| Parameter | Type | Description | Typical Value |
|-----------|------|-------------|---------------|
| `startX`, `startY` | `int` | Start grid position | Entity's current tile |
| `goalX`, `goalY` | `int` | Goal grid position | Target tile |
| `path` | `std::vector<Vector>&` | Output waypoints (world space) | Empty vector |
| `layer` | `CollisionLayer` | Navigation layer | `CollisionLayer::Ground` |
| `maxIterations` | `int` | A* iteration limit | `5000` - `10000` |

### **World-to-Grid Conversion**

Always convert world coordinates to grid coordinates before calling `FindPath()`:

```cpp
// Correct: Convert to grid first
int startX, startY;
navMap.WorldToGrid(entityPos.x, entityPos.y, startX, startY);

// FindPath() expects grid coordinates
bool success = navMap.FindPath(startX, startY, goalX, goalY, path);
```

### **Validation Before Pathfinding**

```cpp
// Validate start and goal positions
if (!navMap.IsValidGridPosition(startX, startY)) {
    // Start position out of bounds
    return false;
}

if (!navMap.IsValidGridPosition(goalX, goalY)) {
    // Goal position out of bounds
    return false;
}

if (!navMap.IsNavigable(startX, startY)) {
    // Start position blocked
    return false;
}

if (!navMap.IsNavigable(goalX, goalY)) {
    // Goal position blocked (can't reach)
    return false;
}

// Safe to pathfind
bool success = navMap.FindPath(startX, startY, goalX, goalY, path);
```

---

## Handling Path Results

### **Success: Path Found**

When `FindPath()` returns `true`, the `path` vector contains world-space waypoints:

```cpp
std::vector<Vector> path;
bool success = navMap.FindPath(startX, startY, goalX, goalY, path);

if (success) {
    std::cout << "Path found with " << path.size() << " waypoints\n";
    
    // Path is in world coordinates, ready to use
    for (size_t i = 0; i < path.size(); ++i) {
        std::cout << "Waypoint " << i << ": (" 
                  << path[i].x << ", " << path[i].y << ")\n";
    }
    
    // Store path in agent component
    NavigationAgent_data& agent = ...;
    agent.currentPath = path;
    agent.currentWaypointIndex = 0;
    agent.hasPath = true;
}
```

### **Failure: No Path**

When `FindPath()` returns `false`, no valid path exists:

```cpp
if (!success) {
    // Possible reasons:
    // 1. Goal is unreachable (blocked or isolated)
    // 2. Start/goal out of bounds
    // 3. Max iterations exceeded (partial path not returned)
    
    // Handle failure gracefully
    HandleNoPath(entity);
}
```

### **Failure Handling Strategies**

#### **Strategy 1: Stop and Wait**
```cpp
void HandleNoPath(EntityID entity) {
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    agent.hasPath = false;
    agent.currentPath.clear();
    
    // Entity stops moving and idles
}
```

#### **Strategy 2: Find Nearest Reachable Position**
```cpp
void HandleNoPath(EntityID entity, const Vector& unreachableGoal) {
    // Search for nearest navigable tile to goal
    int goalX, goalY;
    NavigationMap::Get().WorldToGrid(unreachableGoal.x, unreachableGoal.y, goalX, goalY);
    
    // Spiral search outward
    for (int radius = 1; radius < 20; ++radius) {
        for (int dx = -radius; dx <= radius; ++dx) {
            for (int dy = -radius; dy <= radius; ++dy) {
                int x = goalX + dx;
                int y = goalY + dy;
                
                if (NavigationMap::Get().IsNavigable(x, y)) {
                    // Found reachable alternative
                    std::vector<Vector> path;
                    bool success = NavigationMap::Get().FindPath(..., x, y, path);
                    if (success) {
                        // Use alternative path
                        return;
                    }
                }
            }
        }
    }
    
    // Still no path - give up
    HandleNoPath(entity);
}
```

#### **Strategy 3: Use Fallback Behavior**
```cpp
void HandleNoPath(EntityID entity) {
    AIState_data& aiState = World::Get().GetComponent<AIState_data>(entity);
    
    // Switch to alternative AI behavior
    if (aiState.currentState == "Chase") {
        aiState.currentState = "Patrol"; // Fall back to patrol
    }
}
```

### **Partial Paths (Iteration Limit Exceeded)**

If `maxIterations` is reached before finding goal, `FindPath()` returns `false`:

```cpp
// Long-distance pathfinding
std::vector<Vector> path;
bool success = navMap.FindPath(startX, startY, goalX, goalY, path, 
                               CollisionLayer::Ground, 
                               2000); // Low iteration limit

if (!success) {
    // Possible partial path scenario
    // Consider hierarchical pathfinding or waypoints
}
```

**Solution: Hierarchical Pathfinding**
```cpp
// Break long path into multiple shorter segments
Vector start = entityPos;
Vector goal = targetPos;
Vector midpoint = (start + goal) * 0.5f;

// Path to midpoint first
std::vector<Vector> pathToMid;
bool success1 = navMap.FindPath(..., midpoint, pathToMid, ..., 2000);

// Then path from midpoint to goal
std::vector<Vector> pathToGoal;
bool success2 = navMap.FindPath(midpoint, ..., goal, pathToGoal, ..., 2000);

if (success1 && success2) {
    // Combine paths
    std::vector<Vector> fullPath = pathToMid;
    fullPath.insert(fullPath.end(), pathToGoal.begin(), pathToGoal.end());
}
```

---

## Following a Path

### **NavigationAgent_data Component**

The `NavigationAgent_data` component stores pathfinding state:

```cpp
struct NavigationAgent_data
{
    float agentRadius = 16.0f;         // Collision radius
    float maxSpeed = 100.0f;           // Max movement speed (pixels/sec)
    float arrivalThreshold = 5.0f;     // Distance to consider "arrived"
    
    uint8_t layerMask = 0x01;          // Layer bits (Ground = 0x01)
    
    std::vector<Vector> currentPath;   // Cached path (world coordinates)
    int currentWaypointIndex = 0;      // Current waypoint
    Vector targetPosition = Vector(0, 0, 0);
    bool hasPath = false;
    bool needsRepath = false;
    
    float steeringWeight = 1.0f;
    bool avoidObstacles = true;
};
```

### **Basic Path Following**

```cpp
void FollowPath(EntityID entity, float deltaTime)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    // Check if path is complete
    if (agent.currentWaypointIndex >= static_cast<int>(agent.currentPath.size())) {
        agent.hasPath = false;
        agent.currentPath.clear();
        return; // Path finished
    }
    
    // Get current waypoint
    const Vector& waypoint = agent.currentPath[agent.currentWaypointIndex];
    
    // Calculate direction and distance
    Vector direction = waypoint - position.position;
    float distance = direction.Length();
    
    // Check if arrived at waypoint
    if (distance < agent.arrivalThreshold) {
        ++agent.currentWaypointIndex; // Move to next waypoint
        return;
    }
    
    // Move towards waypoint
    direction.Normalize();
    float moveSpeed = agent.maxSpeed * deltaTime;
    
    if (moveSpeed > distance) {
        // Don't overshoot
        position.position = waypoint;
        ++agent.currentWaypointIndex;
    } else {
        position.position.x += direction.x * moveSpeed;
        position.position.y += direction.y * moveSpeed;
    }
}
```

### **Path Following in NavigationSystem**

The `NavigationSystem` ECS system handles path following automatically:

```cpp
// NavigationSystem::Process() (simplified)
void NavigationSystem::Process()
{
    for (EntityID entity : m_entities) {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        
        if (agent.needsRepath) {
            // Recalculate path
            RequestPath(entity, agent.targetPosition);
            agent.needsRepath = false;
        }
        
        if (agent.hasPath) {
            // Follow current path
            FollowPath(entity, deltaTime);
        }
    }
}
```

### **Integration with Movement System**

If you have a separate `Movement_data` component, integrate with path following:

```cpp
void FollowPath(EntityID entity, float deltaTime)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Movement_data& movement = World::Get().GetComponent<Movement_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    if (!agent.hasPath || agent.currentWaypointIndex >= agent.currentPath.size()) {
        // No path - stop movement
        movement.velocity = Vector(0, 0, 0);
        return;
    }
    
    const Vector& waypoint = agent.currentPath[agent.currentWaypointIndex];
    Vector direction = waypoint - position.position;
    float distance = direction.Length();
    
    if (distance < agent.arrivalThreshold) {
        ++agent.currentWaypointIndex;
        return;
    }
    
    // Update Movement_data component
    direction.Normalize();
    movement.direction = direction;
    movement.velocity = direction * agent.maxSpeed;
    
    // Movement system will apply velocity to position
}
```

---

## Repathing Strategies

### **When to Repath**

1. **Dynamic Obstacle Detected**: Wall destroyed, door opened/closed
2. **Stuck Detection**: Agent not making progress
3. **Target Moved**: Goal position changed significantly
4. **Periodic Repath**: Repath every N seconds for long paths
5. **Layer Change**: Agent switches navigation layer (ground ↔ sky)

### **Strategy 1: Event-Driven Repathing**

```cpp
// When a wall is destroyed
void OnWallDestroyed(int gridX, int gridY)
{
    // Update collision map
    CollisionMap::Get().UpdateTileState(gridX, gridY, [](TileProperties& tile) {
        tile.isBlocked = false;
        tile.isNavigable = true;
    });
    
    // Mark all agents as needing repath
    for (EntityID entity : World::Get().GetEntitiesWithComponent<NavigationAgent_data>()) {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        agent.needsRepath = true;
    }
}
```

### **Strategy 2: Stuck Detection**

```cpp
void DetectStuck(EntityID entity, float deltaTime)
{
    static std::unordered_map<EntityID, Vector> lastPositions;
    static std::unordered_map<EntityID, float> stuckTimers;
    
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    if (!agent.hasPath) {
        stuckTimers[entity] = 0.0f;
        return;
    }
    
    // Check if position changed significantly
    Vector lastPos = lastPositions[entity];
    float distanceMoved = (position.position - lastPos).Length();
    
    if (distanceMoved < 1.0f) { // Barely moved
        stuckTimers[entity] += deltaTime;
        
        if (stuckTimers[entity] > 2.0f) { // Stuck for 2 seconds
            // Repath
            agent.needsRepath = true;
            stuckTimers[entity] = 0.0f;
        }
    } else {
        stuckTimers[entity] = 0.0f; // Making progress
    }
    
    lastPositions[entity] = position.position;
}
```

### **Strategy 3: Periodic Repathing**

```cpp
void PeriodicRepath(EntityID entity, float deltaTime)
{
    static std::unordered_map<EntityID, float> repathTimers;
    
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    if (!agent.hasPath) {
        repathTimers[entity] = 0.0f;
        return;
    }
    
    repathTimers[entity] += deltaTime;
    
    if (repathTimers[entity] > 5.0f) { // Repath every 5 seconds
        agent.needsRepath = true;
        repathTimers[entity] = 0.0f;
    }
}
```

### **Strategy 4: Target Movement Threshold**

```cpp
void CheckTargetMoved(EntityID entity)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    if (!agent.hasPath) return;
    
    // Get current target position (e.g., from player)
    Vector currentTargetPos = GetPlayerPosition();
    
    // Check if target moved significantly
    float distanceMoved = (currentTargetPos - agent.targetPosition).Length();
    
    if (distanceMoved > 100.0f) { // Target moved > 100 pixels
        agent.targetPosition = currentTargetPos;
        agent.needsRepath = true;
    }
}
```

### **Strategy 5: Path Validation**

```cpp
bool ValidatePath(const std::vector<Vector>& path)
{
    NavigationMap& navMap = NavigationMap::Get();
    
    // Check each waypoint is still navigable
    for (const Vector& waypoint : path) {
        int gridX, gridY;
        navMap.WorldToGrid(waypoint.x, waypoint.y, gridX, gridY);
        
        if (!navMap.IsNavigable(gridX, gridY)) {
            // Path invalid - need to repath
            return false;
        }
    }
    
    return true; // Path still valid
}

void CheckPathValidity(EntityID entity)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    if (!agent.hasPath) return;
    
    if (!ValidatePath(agent.currentPath)) {
        agent.needsRepath = true;
    }
}
```

---

## Multi-Layer Pathfinding

### **Layer Mask Concept**

The `NavigationAgent_data::layerMask` field is a bitfield specifying which layers the agent can navigate:

```cpp
// Layer mask bits
// Bit 0 = Ground (0x01)
// Bit 1 = Sky (0x02)
// Bit 2 = Underground (0x04)
// Bit 3 = Volume (0x08)
// Bits 4-7 = Custom1-4

// Examples:
agent.layerMask = 0x01;        // Ground only
agent.layerMask = 0x02;        // Sky only
agent.layerMask = 0x01 | 0x02; // Ground OR Sky
agent.layerMask = 0x04;        // Underground only
```

### **Example: Ground Unit**

```cpp
// Create ground unit (can't fly or go underground)
EntityID groundUnit = World::Get().CreateEntity();
NavigationAgent_data& agent = World::Get().AddComponent<NavigationAgent_data>(groundUnit);

agent.layerMask = 0x01; // Ground layer only (bit 0)
agent.maxSpeed = 100.0f;
agent.arrivalThreshold = 5.0f;

// Pathfinding will use Ground layer
// Walls on ground layer will block this unit
```

### **Example: Flying Unit**

```cpp
// Create flying unit (ignores ground obstacles)
EntityID flyingUnit = World::Get().CreateEntity();
NavigationAgent_data& agent = World::Get().AddComponent<NavigationAgent_data>(flyingUnit);

agent.layerMask = 0x02; // Sky layer only (bit 1)
agent.maxSpeed = 150.0f;
agent.arrivalThreshold = 5.0f;

// Pathfinding will use Sky layer
// Ground walls won't block, but sky obstacles will
```

### **Example: Amphibious Unit**

```cpp
// Create unit that can walk or swim (ground + water layer)
EntityID amphibiousUnit = World::Get().CreateEntity();
NavigationAgent_data& agent = World::Get().AddComponent<NavigationAgent_data>(amphibiousUnit);

agent.layerMask = 0x01 | 0x10; // Ground + Custom1 (water)
agent.maxSpeed = 80.0f;

// Pathfinding will prefer layer with lower cost
// Can cross both land and water
```

### **Example: Ghost (Phase Through Walls)**

```cpp
// Create ghost that can phase through walls
EntityID ghost = World::Get().CreateEntity();
NavigationAgent_data& agent = World::Get().AddComponent<NavigationAgent_data>(ghost);

agent.layerMask = 0x10; // Custom1 (ghost layer)
agent.maxSpeed = 120.0f;

// Set up ghost layer with no collisions
for (int y = 0; y < mapHeight; ++y) {
    for (int x = 0; x < mapWidth; ++x) {
        TileProperties ghostTile;
        ghostTile.isBlocked = false;
        ghostTile.isNavigable = true;
        ghostTile.traversalCost = 1.0f;
        CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Custom1, ghostTile);
    }
}

// Ghost can move through walls that block other units
```

### **Multi-Layer Pathfinding Request**

```cpp
void RequestPathOnLayer(EntityID entity, const Vector& targetPos, CollisionLayer layer)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    NavigationMap& navMap = NavigationMap::Get();
    
    // Convert positions
    int startX, startY, goalX, goalY;
    navMap.WorldToGrid(position.position.x, position.position.y, startX, startY);
    navMap.WorldToGrid(targetPos.x, targetPos.y, goalX, goalY);
    
    // Pathfind on specific layer
    std::vector<Vector> path;
    bool success = navMap.FindPath(startX, startY, goalX, goalY, path, layer);
    
    if (success) {
        agent.currentPath = path;
        agent.currentWaypointIndex = 0;
        agent.hasPath = true;
        agent.targetPosition = targetPos;
    }
}
```

### **Dynamic Layer Switching**

```cpp
// Unit can switch between ground and underground
void SwitchToUnderground(EntityID entity)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    // Change layer mask
    agent.layerMask = 0x04; // Underground (bit 2)
    
    // Trigger repath on new layer
    agent.needsRepath = true;
}

void SwitchToGround(EntityID entity)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    
    agent.layerMask = 0x01; // Ground (bit 0)
    agent.needsRepath = true;
}
```

---

## Performance Tips

### **1. Limit Pathfinding Frequency**

```cpp
// Don't pathfind every frame!
static float pathfindTimer = 0.0f;
pathfindTimer += deltaTime;

if (pathfindTimer > 0.5f) { // Pathfind twice per second max
    RequestPath(entity, targetPos);
    pathfindTimer = 0.0f;
}
```

### **2. Use Iteration Limits**

```cpp
// Adjust maxIterations based on distance
Vector start = entityPos;
Vector goal = targetPos;
float distance = (goal - start).Length();

int maxIterations;
if (distance < 200.0f) {
    maxIterations = 1000; // Short distance
} else if (distance < 1000.0f) {
    maxIterations = 5000; // Medium distance
} else {
    maxIterations = 10000; // Long distance
}

bool success = navMap.FindPath(..., path, layer, maxIterations);
```

### **3. Cache Paths**

```cpp
// Store path in NavigationAgent_data
// Reuse until invalidated by dynamic obstacles or repath flag

NavigationAgent_data& agent = ...;
if (agent.hasPath && !agent.needsRepath) {
    // Use cached path - don't recalculate
    FollowPath(entity, deltaTime);
} else {
    // Calculate new path
    RequestPath(entity, targetPos);
}
```

### **4. Spatial Partitioning for Repathing**

```cpp
// Only repath agents near dynamic obstacle changes
void OnWallDestroyed(int gridX, int gridY)
{
    // Convert to world position
    float worldX, worldY;
    CollisionMap::Get().GridToWorld(gridX, gridY, worldX, worldY);
    Vector obstaclePos(worldX, worldY, 0.0f);
    
    // Only repath agents within 500 pixels
    for (EntityID entity : GetAllNavigationAgents()) {
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        float distance = (pos.position - obstaclePos).Length();
        
        if (distance < 500.0f) {
            NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
            agent.needsRepath = true;
        }
    }
}
```

### **5. Hierarchical Pathfinding**

```cpp
// For very long distances, use waypoints
std::vector<Vector> waypoints = {
    Vector(100, 100, 0),
    Vector(500, 300, 0),
    Vector(900, 600, 0),
    Vector(1500, 1200, 0)
};

// Pathfind to nearest waypoint, then to next, etc.
void PathfindHierarchical(EntityID entity, const std::vector<Vector>& waypoints)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    // Find nearest waypoint
    Vector nearestWaypoint = FindNearestWaypoint(position.position, waypoints);
    
    // Pathfind to nearest waypoint (short path)
    RequestPath(entity, nearestWaypoint);
}
```

### **6. Amortize Pathfinding Over Frames**

```cpp
// Pathfind for one agent per frame (round-robin)
static int agentIndex = 0;
std::vector<EntityID> agents = GetAllNavigationAgents();

if (!agents.empty()) {
    EntityID entity = agents[agentIndex % agents.size()];
    
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    if (agent.needsRepath) {
        RequestPath(entity, agent.targetPosition);
        agent.needsRepath = false;
    }
    
    ++agentIndex;
}
```

### **7. Use Lower Resolution for Long Distances**

```cpp
// For distant pathfinding, use a coarser grid
// (Requires separate low-res collision map)

// Example: 2x downscale
// Original: 100x100 map with 32x32 tiles
// Low-res: 50x50 map with 64x64 tiles

CollisionMap lowResMap;
lowResMap.Initialize(50, 50, GridProjectionType::Ortho, 64.0f, 64.0f, 1);

// Populate low-res map (merge 2x2 tiles)
for (int y = 0; y < 50; ++y) {
    for (int x = 0; x < 50; ++x) {
        bool blocked = false;
        for (int dy = 0; dy < 2; ++dy) {
            for (int dx = 0; dx < 2; ++dx) {
                if (highResMap.HasCollision(x*2 + dx, y*2 + dy)) {
                    blocked = true;
                }
            }
        }
        lowResMap.SetCollision(x, y, blocked);
    }
}

// Pathfind on low-res map for distant targets
// Refine path on high-res map when closer
```

### **8. Profile Pathfinding**

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

std::vector<Vector> path;
bool success = navMap.FindPath(startX, startY, goalX, goalY, path);

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

std::cout << "Pathfinding took " << duration.count() << " microseconds\n";

// Adjust maxIterations based on profiling results
```

---

## Complete Integration Examples

### **Example 1: Enemy Chase AI**

```cpp
// EnemyChaseSystem.cpp
void EnemyChaseSystem::Process()
{
    // Get player position
    Vector playerPos = GetPlayerPosition();
    
    for (EntityID enemy : m_entities) {
        AIState_data& aiState = World::Get().GetComponent<AIState_data>(enemy);
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(enemy);
        Position_data& position = World::Get().GetComponent<Position_data>(enemy);
        
        if (aiState.currentState != "Chase") continue;
        
        // Check if need to repath
        float distanceToTarget = (playerPos - agent.targetPosition).Length();
        if (distanceToTarget > 100.0f || !agent.hasPath) {
            // Player moved or no path - request new path
            agent.targetPosition = playerPos;
            RequestPath(enemy, playerPos);
        }
        
        // Follow current path
        if (agent.hasPath) {
            FollowPath(enemy, deltaTime);
        }
    }
}

void EnemyChaseSystem::RequestPath(EntityID entity, const Vector& targetPos)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    NavigationMap& navMap = NavigationMap::Get();
    
    int startX, startY, goalX, goalY;
    navMap.WorldToGrid(position.position.x, position.position.y, startX, startY);
    navMap.WorldToGrid(targetPos.x, targetPos.y, goalX, goalY);
    
    std::vector<Vector> path;
    bool success = navMap.FindPath(
        startX, startY, goalX, goalY, 
        path, 
        CollisionLayer::Ground, 
        5000
    );
    
    if (success) {
        agent.currentPath = path;
        agent.currentWaypointIndex = 0;
        agent.hasPath = true;
        agent.targetPosition = targetPos;
    } else {
        // Can't reach player - switch to patrol
        AIState_data& aiState = World::Get().GetComponent<AIState_data>(entity);
        aiState.currentState = "Patrol";
    }
}
```

### **Example 2: Click-to-Move RTS**

```cpp
// RTSControlSystem.cpp
void RTSControlSystem::OnMouseClick(float mouseX, float mouseY)
{
    // Convert screen to world coordinates
    Vector worldPos = ScreenToWorld(mouseX, mouseY);
    
    // Get selected units
    std::vector<EntityID> selectedUnits = GetSelectedUnits();
    
    for (EntityID unit : selectedUnits) {
        CommandMoveToPosition(unit, worldPos);
    }
}

void RTSControlSystem::CommandMoveToPosition(EntityID unit, const Vector& targetPos)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(unit);
    Position_data& position = World::Get().GetComponent<Position_data>(unit);
    
    NavigationMap& navMap = NavigationMap::Get();
    
    // Validate target is navigable
    int goalX, goalY;
    navMap.WorldToGrid(targetPos.x, targetPos.y, goalX, goalY);
    
    if (!navMap.IsNavigable(goalX, goalY)) {
        // Find nearest navigable tile
        goalX = FindNearestNavigableTile(goalX, goalY).x;
        goalY = FindNearestNavigableTile(goalX, goalY).y;
    }
    
    // Request path
    int startX, startY;
    navMap.WorldToGrid(position.position.x, position.position.y, startX, startY);
    
    std::vector<Vector> path;
    bool success = navMap.FindPath(startX, startY, goalX, goalY, path);
    
    if (success) {
        agent.currentPath = path;
        agent.currentWaypointIndex = 0;
        agent.hasPath = true;
        agent.targetPosition = targetPos;
        
        // Visual feedback (e.g., waypoint markers)
        ShowPathVisual(path);
    } else {
        // Show "can't move there" feedback
        ShowErrorFeedback("Cannot reach that location");
    }
}
```

### **Example 3: Patrol Path**

```cpp
// PatrolSystem.cpp
void PatrolSystem::Process()
{
    for (EntityID entity : m_entities) {
        AIState_data& aiState = World::Get().GetComponent<AIState_data>(entity);
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
        AIBlackboard_data& blackboard = World::Get().GetComponent<AIBlackboard_data>(entity);
        PatrolPath_data& patrolPath = World::Get().GetComponent<PatrolPath_data>(entity);
        
        if (aiState.currentState != "Patrol") continue;
        
        // Check if arrived at current waypoint
        if (!agent.hasPath) {
            // Move to next patrol waypoint
            ++patrolPath.currentIndex;
            
            if (patrolPath.currentIndex >= patrolPath.waypoints.size()) {
                if (patrolPath.looping) {
                    patrolPath.currentIndex = 0; // Loop back
                } else {
                    patrolPath.currentIndex = patrolPath.waypoints.size() - 1;
                    aiState.currentState = "Idle"; // Patrol complete
                    continue;
                }
            }
            
            // Path to next waypoint
            Vector nextWaypoint = patrolPath.waypoints[patrolPath.currentIndex];
            RequestPath(entity, nextWaypoint);
        }
        
        // Follow current path
        if (agent.hasPath) {
            FollowPath(entity, deltaTime);
        }
    }
}
```

### **Example 4: Multi-Layer Flying Enemy**

```cpp
// FlyingEnemySystem.cpp
EntityID CreateFlyingEnemy(const Vector& spawnPos)
{
    EntityID enemy = World::Get().CreateEntity();
    
    // Add components
    Position_data& position = World::Get().AddComponent<Position_data>(enemy);
    position.position = spawnPos;
    
    NavigationAgent_data& agent = World::Get().AddComponent<NavigationAgent_data>(enemy);
    agent.layerMask = 0x02; // Sky layer (bit 1)
    agent.maxSpeed = 150.0f;
    agent.arrivalThreshold = 10.0f;
    
    AIState_data& aiState = World::Get().AddComponent<AIState_data>(enemy);
    aiState.currentState = "FlyToPlayer";
    
    return enemy;
}

void FlyingEnemySystem::Process()
{
    Vector playerPos = GetPlayerPosition();
    
    for (EntityID enemy : m_entities) {
        NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(enemy);
        
        // Repath if player moved
        float distToTarget = (playerPos - agent.targetPosition).Length();
        if (distToTarget > 150.0f || !agent.hasPath) {
            RequestPathOnSkyLayer(enemy, playerPos);
        }
        
        if (agent.hasPath) {
            FollowPath(enemy, deltaTime);
        }
    }
}

void FlyingEnemySystem::RequestPathOnSkyLayer(EntityID entity, const Vector& target)
{
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    Position_data& position = World::Get().GetComponent<Position_data>(entity);
    
    NavigationMap& navMap = NavigationMap::Get();
    
    int startX, startY, goalX, goalY;
    navMap.WorldToGrid(position.position.x, position.position.y, startX, startY);
    navMap.WorldToGrid(target.x, target.y, goalX, goalY);
    
    std::vector<Vector> path;
    bool success = navMap.FindPath(
        startX, startY, goalX, goalY,
        path,
        CollisionLayer::Sky, // Use Sky layer
        5000
    );
    
    if (success) {
        agent.currentPath = path;
        agent.currentWaypointIndex = 0;
        agent.hasPath = true;
        agent.targetPosition = target;
    }
}
```

### **Example 5: Dynamic Obstacle Repathing**

```cpp
// DoorInteractionSystem.cpp
void DoorInteractionSystem::ToggleDoor(EntityID doorEntity)
{
    Position_data& doorPos = World::Get().GetComponent<Position_data>(doorEntity);
    
    // Get door grid position
    int doorX, doorY;
    CollisionMap::Get().WorldToGrid(doorPos.position.x, doorPos.position.y, doorX, doorY);
    
    // Toggle door state
    bool doorOpen = false;
    CollisionMap::Get().UpdateTileState(doorX, doorY, [&doorOpen](TileProperties& tile) {
        doorOpen = !tile.isNavigable;
        tile.isBlocked = !doorOpen;
        tile.isNavigable = doorOpen;
        tile.customFlags = doorOpen ? 0x01 : 0x00;
    });
    
    // Trigger repath for nearby agents
    RepathNearbyAgents(doorPos.position, 300.0f);
}

void DoorInteractionSystem::RepathNearbyAgents(const Vector& center, float radius)
{
    for (EntityID entity : GetAllNavigationAgents()) {
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        float distance = (pos.position - center).Length();
        
        if (distance < radius) {
            NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
            agent.needsRepath = true;
        }
    }
}
```

---

## Troubleshooting

### **Path Not Found**

**Symptoms**: `FindPath()` returns `false`

**Possible Causes:**
1. Goal is blocked or unreachable
2. Start position is invalid
3. Incorrect layer specified
4. maxIterations too low for distance

**Solutions:**
```cpp
// Debug validation
if (!navMap.IsValidGridPosition(goalX, goalY)) {
    std::cout << "Goal out of bounds\n";
}
if (!navMap.IsNavigable(goalX, goalY)) {
    std::cout << "Goal is blocked\n";
}

// Find alternative goal
Vector alternativeGoal = FindNearestNavigableTile(goalX, goalY);
```

### **Agent Gets Stuck**

**Symptoms**: Agent stops moving or jitters

**Possible Causes:**
1. Path invalidated by dynamic obstacle
2. Waypoint unreachable
3. `arrivalThreshold` too small

**Solutions:**
```cpp
// Increase arrival threshold
agent.arrivalThreshold = 10.0f; // Larger threshold

// Implement stuck detection
DetectStuck(entity, deltaTime);
```

### **Poor Performance**

**Symptoms**: Frame drops during pathfinding

**Solutions:**
```cpp
// Reduce maxIterations
bool success = navMap.FindPath(..., path, layer, 2000); // Lower limit

// Amortize over frames
PathfindOneAgentPerFrame();

// Use hierarchical pathfinding for long distances
```

---

## Summary

This guide covered:

✅ When to use pathfinding  
✅ Requesting paths with `NavigationMap::FindPath()`  
✅ Handling success, failure, and partial paths  
✅ Following paths with `NavigationAgent_data`  
✅ Repathing strategies for dynamic obstacles  
✅ Multi-layer pathfinding for flying/underground units  
✅ Performance optimization techniques  
✅ Complete integration examples  

For more details, see:
- [Navigation System Reference](Navigation_System_Reference.md)
- [Collision Types Reference](Collision_Types_Reference.md)
- [Adding ECS Components Guide](Adding_ECS_Components_Guide.md)
