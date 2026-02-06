# Multi-Layer Collision & Navigation System Documentation

This directory contains comprehensive documentation for the Olympe Engine's multi-layer collision and navigation system with A* pathfinding.

## 📚 Documentation Index

### 🗺️ [Navigation System Reference](Navigation_System_Reference.md)
Complete API reference for CollisionMap and NavigationMap singletons.

**Contents:**
- Architecture overview
- CollisionMap API (initialization, layers, tile properties, coordinates)
- NavigationMap API (A* pathfinding, navigability)
- Data structures (TileProperties, TerrainType, CollisionLayer)
- Coordinate conversions (world ↔ grid)
- 12+ complete code examples

**Use when:** You need API reference, method signatures, or quick lookup.

---

### 🎯 [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md)
Step-by-step tutorial for integrating A* pathfinding into your game.

**Contents:**
- When to use pathfinding (decision tree)
- Requesting paths (NavigationMap::FindPath)
- Following paths (NavigationAgent_data)
- Repathing strategies (5 patterns)
- Multi-layer pathfinding
- Performance optimization (8 techniques)
- 5 complete integration examples (enemy AI, RTS, patrols, flying units, dynamic obstacles)

**Use when:** You're adding pathfinding to entities or implementing AI navigation.

---

### 🧱 [Collision Types Reference](Collision_Types_Reference.md)
Detailed reference for terrain types, layers, and dynamic states.

**Contents:**
- TerrainType values (Ground, Water, Grass, Sand, Rock, Ice, Lava, Mud, Snow, Custom)
- CollisionLayer values (8 layers with use cases)
- Custom flags (8-bit manipulation)
- Dynamic states (destructible walls, bridges, doors)
- State transition API (UpdateTileState)
- 4 complete gameplay systems (dungeon, destruction, building, elements)

**Use when:** You're designing level mechanics, tile behaviors, or multi-layer systems.

---

### 🔧 [Adding ECS Components Guide](Adding_ECS_Components_Guide.md)
Complete workflow for adding new ECS components to the engine.

**Contents:**
- 7-step integration process (with flowchart)
- C++ struct definition (ECS_Components.h)
- JSON schema serialization (ParameterSchemas.json)
- Prefab updates
- Tiled map parsing
- ECS system creation
- System registration
- Testing and debugging
- 8 common pitfalls with solutions
- 2 complete examples (Shield system, Quest system)

**Use when:** You're extending the engine with new components or systems.

---

## 🚀 Quick Start

### 1. Initialize Collision & Navigation Maps

The maps are automatically initialized when loading a Tiled level via `World::LoadLevelFromTiled()`. The system reads map dimensions, projection type, and generates collision/navigation data.

```cpp
// Maps are initialized in World::GenerateCollisionAndNavigationMaps()
// Called automatically during level loading
```

### 2. Access Maps

```cpp
// Get singleton instances
CollisionMap& collMap = CollisionMap::Get();
NavigationMap& navMap = NavigationMap::Get();

// Check collision
bool blocked = collMap.HasCollision(gridX, gridY);

// Request pathfinding
std::vector<Vector> path;
bool found = navMap.FindPath(startX, startY, goalX, goalY, path);
```

### 3. Use NavigationAgent Component

Add `NavigationAgent_data` to your entity prefab:

```json
{
  "type": "NavigationAgent_data",
  "properties": {
    "agentRadius": 16.0,
    "maxSpeed": 100.0,
    "layerMask": 1
  }
}
```

The `NavigationSystem` automatically processes entities with this component.

---

## 🔑 Key Features

### ✅ Multi-Layer Support
8 independent collision layers (Ground, Sky, Underground, Volume, + 4 custom)

### ✅ 3 Projection Types
Orthogonal, Isometric, Hexagonal (pointy-top)

### ✅ A* Pathfinding
Optimized A* with projection-specific heuristics (Manhattan, Chebyshev, Axial)

### ✅ Rich Tile Data
Per-tile properties: collision, navigation, terrain type, traversal cost, custom flags, dynamic states

### ✅ Dynamic States
Tile state transitions via lambdas (destructible walls, buildable bridges, openable doors)

### ✅ Visual Debug Overlays
Per-layer color-coded overlays for collision and navigation (configurable via GridSettings_data)

### ✅ ECS Integration
Lightweight `NavigationAgent_data` component with automatic `NavigationSystem` processing

### ✅ Performance Optimized
Frustum culling for overlays, iteration limits for A*, early exit, min-heap priority queue

### ✅ Future-Ready
Sectorization architecture for dynamic loading/unloading

---

## 📖 Learning Path

### Beginner
1. Read [Navigation System Reference](Navigation_System_Reference.md) - Architecture overview
2. Try basic examples in the reference guide
3. Read [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md) - Sections 1-3

### Intermediate
1. Complete [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md)
2. Implement one of the 5 integration patterns
3. Read [Collision Types Reference](Collision_Types_Reference.md) - Multi-layer systems

### Advanced
1. Implement dynamic states (Collision Types Reference)
2. Optimize pathfinding for your game (Pathfinding Guide - Section 6)
3. Create custom ECS components ([Adding ECS Components Guide](Adding_ECS_Components_Guide.md))

---

## 💡 Common Use Cases

### Enemy Chase AI
See [Pathfinding Integration Guide - Example 1](Pathfinding_Integration_Guide.md#example-1-enemy-chase-ai)

### Click-to-Move (RTS)
See [Pathfinding Integration Guide - Example 2](Pathfinding_Integration_Guide.md#example-2-click-to-move-rts)

### Patrol Paths
See [Pathfinding Integration Guide - Example 3](Pathfinding_Integration_Guide.md#example-3-patrol-paths)

### Flying Enemies (Multi-Layer)
See [Pathfinding Integration Guide - Example 4](Pathfinding_Integration_Guide.md#example-4-flying-enemies)

### Destructible Walls
See [Collision Types Reference - Gameplay System 2](Collision_Types_Reference.md#gameplay-system-2-destructible-environment)

### Buildable Bridges
See [Collision Types Reference - Gameplay System 3](Collision_Types_Reference.md#gameplay-system-3-bridge-building)

---

## 🛠️ Troubleshooting

### Path Not Found
- Check if start/goal are valid grid positions
- Verify start/goal tiles are navigable (not blocked)
- Increase `maxIterations` parameter if map is large
- Check layer mask matches tile's collision layer

### Overlay Not Rendering
- Ensure `GridSettings_data.enabled = true` (TAB key)
- Set `GridSettings_data.showCollisionOverlay = true` (C key)
- Set `GridSettings_data.showNavigationOverlay = true` (N key)
- Verify active layer index is valid (0-7)

### Agent Not Moving
- Check `NavigationAgent_data.hasPath == true`
- Verify `NavigationAgent_data.currentPath` not empty
- Ensure `NavigationSystem` is registered in `World::Initialize_ECS_Systems()`
- Check entity has both `Position_data` and `NavigationAgent_data`

---

## 📞 Support

For issues, feature requests, or questions:
1. Check the relevant documentation guide
2. Review code examples
3. Check [Adding ECS Components Guide](Adding_ECS_Components_Guide.md) - Common Pitfalls section
4. Open an issue on GitHub

---

## 🎯 Next Steps

1. Explore the documentation guides
2. Try the code examples
3. Integrate pathfinding into your game
4. Extend the system with custom terrain types or collision layers
5. Contribute improvements back to the engine

**Happy pathfinding! 🚀**
