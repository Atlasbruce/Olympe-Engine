# Collision Types Reference

## Table of Contents
- [Overview](#overview)
- [TerrainType Enum](#terraintype-enum)
- [CollisionLayer Enum](#collisionlayer-enum)
- [Custom Flags](#custom-flags)
- [Dynamic States](#dynamic-states)
- [State Transition API](#state-transition-api)
- [Gameplay Integration Examples](#gameplay-integration-examples)

---

## Overview

The Olympe Engine collision system provides rich tile metadata through the `TileProperties` structure. This document details the available terrain types, collision layers, custom flags, and dynamic state transitions that enable complex gameplay mechanics.

### **TileProperties Quick Reference**

```cpp
struct TileProperties
{
    bool isBlocked = false;           // Hard collision
    bool isNavigable = true;          // Pathfinding allowed
    float traversalCost = 1.0f;       // Movement cost multiplier
    TerrainType terrain = TerrainType::Ground;
    uint8_t customFlags = 0;          // 8 bits for gameplay flags
    CollisionLayer layer = CollisionLayer::Ground;
    
    // Dynamic states
    bool isDynamic = false;
    std::string onDestroyedState;
    std::string onBuiltState;
    std::string metadata;             // JSON metadata
};
```

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

### **Terrain Type Details**

---

### **Invalid (0)**

**Description**: Uninitialized or error state  
**Use Case**: Debugging, error detection  
**Recommended Settings**:
- `isBlocked`: `true`
- `isNavigable`: `false`
- `traversalCost`: `N/A`

**Example**:
```cpp
TileProperties invalid;
invalid.terrain = TerrainType::Invalid;
invalid.isBlocked = true;
invalid.isNavigable = false;
```

---

### **Ground (1)**

**Description**: Standard walkable surface  
**Use Case**: Floors, paths, roads, bridges  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `1.0f` (normal)

**Visual Examples**: Stone floor, dirt path, wooden bridge, tile floor

**Example**:
```cpp
TileProperties ground;
ground.terrain = TerrainType::Ground;
ground.isBlocked = false;
ground.isNavigable = true;
ground.traversalCost = 1.0f;
```

**Gameplay Scenarios**:
- Standard dungeon floors
- Town roads and pathways
- Building interiors
- Stone platforms

---

### **Water (2)**

**Description**: Liquid terrain, typically impassable  
**Use Case**: Rivers, lakes, oceans, moats  
**Recommended Settings**:
- `isBlocked`: `false` (can enter but slow)
- `isNavigable`: `false` (can't walk) OR `true` (can swim)
- `traversalCost`: `5.0f` (very slow if swimming)

**Visual Examples**: River, lake, ocean, puddle

**Example (Non-Swimmable)**:
```cpp
TileProperties water;
water.terrain = TerrainType::Water;
water.isBlocked = false;      // Can enter (fall in)
water.isNavigable = false;    // Can't pathfind through
water.traversalCost = 999.0f; // Effectively infinite cost
```

**Example (Swimmable)**:
```cpp
TileProperties water;
water.terrain = TerrainType::Water;
water.isBlocked = false;
water.isNavigable = true;    // Can swim
water.traversalCost = 5.0f;  // 5x slower than ground
```

**Gameplay Scenarios**:
- Water hazards (drowning)
- Swimming mechanics
- Boat navigation layer
- Bridge building over water
- Fishing spots

---

### **Grass (3)**

**Description**: Natural vegetation  
**Use Case**: Fields, meadows, lawns  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `1.0f` - `1.2f` (normal or slightly slow)

**Visual Examples**: Grass field, meadow, garden, lawn

**Example**:
```cpp
TileProperties grass;
grass.terrain = TerrainType::Grass;
grass.isBlocked = false;
grass.isNavigable = true;
grass.traversalCost = 1.1f; // Slightly slower
```

**Gameplay Scenarios**:
- Open fields for combat
- Stealth mechanics (grass hides player)
- Sound dampening (quieter footsteps)
- Fire spread (grass burns)

---

### **Sand (4)**

**Description**: Loose granular terrain  
**Use Case**: Deserts, beaches, dunes  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `1.5f` (moderately slow)

**Visual Examples**: Desert sand, beach sand, sand dunes

**Example**:
```cpp
TileProperties sand;
sand.terrain = TerrainType::Sand;
sand.isBlocked = false;
sand.isNavigable = true;
sand.traversalCost = 1.5f; // 50% slower
```

**Gameplay Scenarios**:
- Desert environments
- Beach areas
- Quicksand traps (higher cost or non-navigable)
- Footprint trails
- Sandstorms (visibility reduction)

---

### **Rock (5)**

**Description**: Hard stone terrain  
**Use Case**: Mountains, cliffs, rubble, rocky ground  
**Recommended Settings**:
- `isBlocked`: `true` (solid rock wall) OR `false` (rocky ground)
- `isNavigable`: `false` (wall) OR `true` (difficult terrain)
- `traversalCost`: `2.0f` (if navigable)

**Visual Examples**: Mountain rock, cliff face, boulder, rubble

**Example (Wall)**:
```cpp
TileProperties rockWall;
rockWall.terrain = TerrainType::Rock;
rockWall.isBlocked = true;
rockWall.isNavigable = false;
rockWall.isDynamic = true; // Can be destroyed
rockWall.onDestroyedState = "Rubble";
```

**Example (Rocky Ground)**:
```cpp
TileProperties rockyGround;
rockyGround.terrain = TerrainType::Rock;
rockyGround.isBlocked = false;
rockyGround.isNavigable = true;
rockyGround.traversalCost = 2.0f; // Difficult terrain
```

**Gameplay Scenarios**:
- Destructible walls (becomes rubble)
- Difficult terrain (mountains)
- Mining mechanics
- Climbing mechanics
- Rockslides/avalanches

---

### **Ice (6)**

**Description**: Frozen slippery surface  
**Use Case**: Frozen lakes, glaciers, ice paths  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `0.5f` - `0.8f` (faster due to sliding)

**Visual Examples**: Frozen lake, glacier, ice path, icicles

**Example**:
```cpp
TileProperties ice;
ice.terrain = TerrainType::Ice;
ice.isBlocked = false;
ice.isNavigable = true;
ice.traversalCost = 0.8f; // Slightly faster
ice.customFlags = 0x01;   // Slippery flag
```

**Gameplay Scenarios**:
- Ice sliding puzzles
- Reduced traction (momentum-based movement)
- Melting mechanics (ice → water)
- Ice skating speed boost
- Cracking ice hazards

---

### **Lava (7)**

**Description**: Molten rock, extreme hazard  
**Use Case**: Volcanoes, fire pits, lava flows  
**Recommended Settings**:
- `isBlocked`: `false` (can enter but damage)
- `isNavigable`: `false` (can't path through)
- `traversalCost`: `999.0f` (effectively infinite)

**Visual Examples**: Lava flow, fire pit, volcano, molten rock

**Example**:
```cpp
TileProperties lava;
lava.terrain = TerrainType::Lava;
lava.isBlocked = false;     // Can enter (fatal)
lava.isNavigable = false;   // Can't pathfind
lava.traversalCost = 999.0f;
lava.customFlags = 0x02;    // Damage flag
```

**Gameplay Scenarios**:
- Environmental hazards (instant death)
- Special items (lava immunity)
- Lava bridges (temporary safe paths)
- Volcanic eruptions
- Heat damage zones

---

### **Mud (8)**

**Description**: Soft, sticky terrain  
**Use Case**: Swamps, bogs, muddy ground  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `2.0f` - `3.0f` (very slow)

**Visual Examples**: Swamp mud, bog, muddy path, quicksand

**Example**:
```cpp
TileProperties mud;
mud.terrain = TerrainType::Mud;
mud.isBlocked = false;
mud.isNavigable = true;
mud.traversalCost = 2.5f; // 2.5x slower
```

**Gameplay Scenarios**:
- Swamp environments
- Slowing effects (boots stuck in mud)
- Quicksand traps (high cost)
- Muddy footprints (tracking)
- Rain effects (ground becomes muddy)

---

### **Snow (9)**

**Description**: Snow-covered terrain  
**Use Case**: Winter environments, tundra, mountains  
**Recommended Settings**:
- `isBlocked`: `false`
- `isNavigable`: `true`
- `traversalCost`: `1.5f` (moderately slow)

**Visual Examples**: Snow field, tundra, snowy path, avalanche

**Example**:
```cpp
TileProperties snow;
snow.terrain = TerrainType::Snow;
snow.isBlocked = false;
snow.isNavigable = true;
snow.traversalCost = 1.5f; // Trudging through snow
```

**Gameplay Scenarios**:
- Winter environments
- Snowshoe mechanics (reduce cost)
- Footprint trails
- Avalanche hazards
- Blizzard effects (visibility)
- Snow melting (snow → water/mud)

---

### **Custom (255)**

**Description**: Game-specific terrain type  
**Use Case**: Teleporters, conveyors, portals, special zones  
**Recommended Settings**: Varies by implementation

**Visual Examples**: Teleporter pad, conveyor belt, portal, magic circle

**Example (Teleporter)**:
```cpp
TileProperties teleporter;
teleporter.terrain = TerrainType::Custom;
teleporter.isBlocked = false;
teleporter.isNavigable = true;
teleporter.traversalCost = 1.0f;
teleporter.customFlags = 0x08; // Teleporter flag
teleporter.metadata = R"({"destination": "level2_spawn"})";
```

**Example (Conveyor Belt)**:
```cpp
TileProperties conveyor;
conveyor.terrain = TerrainType::Custom;
conveyor.isBlocked = false;
conveyor.isNavigable = true;
conveyor.traversalCost = 0.5f; // Faster when moving with belt
conveyor.customFlags = 0x10; // Conveyor flag
conveyor.metadata = R"({"direction": "east", "speed": 2.0})";
```

**Gameplay Scenarios**:
- Teleportation pads
- One-way doors
- Moving platforms
- Pressure plates
- Magic circles
- Damage zones
- Speed boost zones

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

### **Layer Descriptions and Use Cases**

---

### **Ground (0)**

**Description**: Standard walking layer for ground-based units  
**Bit Mask**: `0x01`

**Typical Use Cases**:
- Player movement
- Ground enemies (zombies, soldiers, animals)
- NPCs walking
- Vehicles (cars, tanks)

**Blocked By**:
- Walls
- Buildings
- Closed doors
- Large obstacles

**Setup Example**:
```cpp
// Wall on ground layer
TileProperties wall;
wall.isBlocked = true;
wall.isNavigable = false;
wall.layer = CollisionLayer::Ground;
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Ground, wall);

// Open path on ground layer
TileProperties path;
path.isBlocked = false;
path.isNavigable = true;
path.traversalCost = 1.0f;
path.layer = CollisionLayer::Ground;
CollisionMap::Get().SetTileProperties(11, 10, CollisionLayer::Ground, path);
```

**Agent Setup**:
```cpp
NavigationAgent_data& agent = ...;
agent.layerMask = 0x01; // Ground layer only
```

---

### **Sky (1)**

**Description**: Aerial layer for flying units  
**Bit Mask**: `0x02`

**Typical Use Cases**:
- Flying enemies (dragons, helicopters, drones)
- Airborne vehicles
- Levitating characters
- Birds, bats

**Blocked By**:
- Ceilings
- Aerial obstacles (hanging chains, chandeliers)
- Force fields
- Sky barriers

**NOT Blocked By**:
- Ground walls
- Ground obstacles
- Closed doors (at ground level)

**Setup Example**:
```cpp
// Ground wall blocks ground layer
TileProperties groundWall;
groundWall.isBlocked = true;
groundWall.isNavigable = false;
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Ground, groundWall);

// Sky layer is open (flying over wall)
TileProperties skyOpen;
skyOpen.isBlocked = false;
skyOpen.isNavigable = true;
skyOpen.traversalCost = 0.9f; // Slightly faster in air
CollisionMap::Get().SetTileProperties(10, 10, CollisionLayer::Sky, skyOpen);

// Ceiling blocks sky layer
TileProperties ceiling;
ceiling.isBlocked = true;
ceiling.isNavigable = false;
CollisionMap::Get().SetTileProperties(15, 15, CollisionLayer::Sky, ceiling);
```

**Agent Setup**:
```cpp
NavigationAgent_data& flyingEnemy = ...;
flyingEnemy.layerMask = 0x02; // Sky layer only
flyingEnemy.maxSpeed = 150.0f; // Faster than ground units
```

---

### **Underground (2)**

**Description**: Subterranean layer for tunnels and sewers  
**Bit Mask**: `0x04`

**Typical Use Cases**:
- Underground tunnels
- Sewer systems
- Burrowing enemies (moles, worms)
- Subway trains
- Secret passages

**Blocked By**:
- Bedrock
- Underground walls
- Collapsed tunnels
- Underground water

**NOT Blocked By**:
- Ground-level walls (above the tunnel)
- Buildings (if tunnel goes under)

**Setup Example**:
```cpp
// Ground layer: building (blocked)
TileProperties building;
building.isBlocked = true;
building.isNavigable = false;
CollisionMap::Get().SetTileProperties(20, 20, CollisionLayer::Ground, building);

// Underground layer: tunnel (open)
TileProperties tunnel;
tunnel.isBlocked = false;
tunnel.isNavigable = true;
tunnel.traversalCost = 1.2f; // Slightly slower (dark, cramped)
CollisionMap::Get().SetTileProperties(20, 20, CollisionLayer::Underground, tunnel);
```

**Agent Setup**:
```cpp
NavigationAgent_data& mole = ...;
mole.layerMask = 0x04; // Underground layer only
```

**Multi-Layer Agent (Can Switch)**:
```cpp
// Agent that can burrow underground
NavigationAgent_data& burrowingUnit = ...;
burrowingUnit.layerMask = 0x01; // Start on ground

// When burrowing
void Burrow(EntityID entity) {
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    agent.layerMask = 0x04; // Switch to underground
    agent.needsRepath = true;
}

// When surfacing
void Surface(EntityID entity) {
    NavigationAgent_data& agent = World::Get().GetComponent<NavigationAgent_data>(entity);
    agent.layerMask = 0x01; // Switch to ground
    agent.needsRepath = true;
}
```

---

### **Volume (3)**

**Description**: 3D volume layer for stacked isometric maps  
**Bit Mask**: `0x08`

**Typical Use Cases**:
- Multi-floor buildings in isometric view
- Elevated platforms
- Bridges over roads
- Vertical separation of movement

**Example**: In an isometric city builder:
- Ground layer: street level
- Volume layer: elevated highway or second floor of building

**Setup Example**:
```cpp
// Ground layer: road at street level
TileProperties road;
road.isBlocked = false;
road.isNavigable = true;
CollisionMap::Get().SetTileProperties(30, 30, CollisionLayer::Ground, road);

// Volume layer: elevated highway above road
TileProperties highway;
highway.isBlocked = false;
highway.isNavigable = true;
CollisionMap::Get().SetTileProperties(30, 30, CollisionLayer::Volume, highway);
```

**Agent Setup**:
```cpp
// Car on street level
NavigationAgent_data& streetCar = ...;
streetCar.layerMask = 0x01; // Ground

// Car on elevated highway
NavigationAgent_data& highwayCar = ...;
highwayCar.layerMask = 0x08; // Volume
```

---

### **Custom1-4 (4-7)**

**Description**: Game-specific layers for custom mechanics  
**Bit Masks**: `0x10`, `0x20`, `0x40`, `0x80`

**Typical Use Cases**:

#### **Custom1: Water Layer (Ships)**
```cpp
// Water tiles navigable for ships only
TileProperties water;
water.isBlocked = false;
water.isNavigable = true;
water.terrain = TerrainType::Water;
CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Custom1, water);

// Ship agent
NavigationAgent_data& ship = ...;
ship.layerMask = 0x10; // Custom1 (water)
```

#### **Custom2: Ghost Phase (Spectral Entities)**
```cpp
// Ghosts can pass through all walls
for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
        TileProperties ghostLayer;
        ghostLayer.isBlocked = false;
        ghostLayer.isNavigable = true;
        CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Custom2, ghostLayer);
    }
}

// Ghost agent
NavigationAgent_data& ghost = ...;
ghost.layerMask = 0x20; // Custom2 (ghost phase)
```

#### **Custom3: Teleporter Network**
```cpp
// Only accessible via teleportation (not walking)
TileProperties teleportNode;
teleportNode.isBlocked = false;
teleportNode.isNavigable = true;
teleportNode.traversalCost = 0.1f; // Instant travel
CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Custom3, teleportNode);
```

#### **Custom4: Climbing Layer**
```cpp
// Climbable surfaces (walls, ladders)
TileProperties climbable;
climbable.isBlocked = false;
climbable.isNavigable = true;
climbable.traversalCost = 1.5f; // Slower climbing
CollisionMap::Get().SetTileProperties(x, y, CollisionLayer::Custom4, climbable);

// Climber agent
NavigationAgent_data& climber = ...;
climber.layerMask = 0x01 | 0x80; // Ground + Custom4 (can walk and climb)
```

---

## Custom Flags

The `TileProperties::customFlags` field is an 8-bit bitfield for game-specific logic.

### **Bit Manipulation Basics**

```cpp
// Define flags
enum TileFlags : uint8_t
{
    FLAG_DOOR_OPEN     = 0x01, // Bit 0
    FLAG_TRAP_ARMED    = 0x02, // Bit 1
    FLAG_SECRET_DOOR   = 0x04, // Bit 2
    FLAG_TELEPORTER    = 0x08, // Bit 3
    FLAG_PRESSURE_PLATE = 0x10, // Bit 4
    FLAG_DESTRUCTIBLE  = 0x20, // Bit 5
    FLAG_BUILDABLE     = 0x40, // Bit 6
    FLAG_LOCKED        = 0x80  // Bit 7
};

// Set flag (turn on)
tile.customFlags |= FLAG_DOOR_OPEN;

// Clear flag (turn off)
tile.customFlags &= ~FLAG_DOOR_OPEN;

// Toggle flag
tile.customFlags ^= FLAG_DOOR_OPEN;

// Check flag
if (tile.customFlags & FLAG_TRAP_ARMED) {
    // Trap is armed
}

// Set multiple flags
tile.customFlags = FLAG_SECRET_DOOR | FLAG_LOCKED;

// Check multiple flags (all set)
if ((tile.customFlags & (FLAG_DOOR_OPEN | FLAG_LOCKED)) == (FLAG_DOOR_OPEN | FLAG_LOCKED)) {
    // Both door open AND locked (contradiction?)
}

// Check any flag set
if (tile.customFlags & (FLAG_TRAP_ARMED | FLAG_PRESSURE_PLATE)) {
    // Either trap armed OR pressure plate
}
```

### **Common Flag Patterns**

#### **Door States**
```cpp
enum DoorFlags : uint8_t
{
    DOOR_OPEN = 0x01,
    DOOR_LOCKED = 0x02,
    DOOR_SECRET = 0x04,
    DOOR_ONE_WAY = 0x08
};

// Closed, unlocked door
tile.customFlags = 0x00;

// Open door
tile.customFlags = DOOR_OPEN;

// Locked door
tile.customFlags = DOOR_LOCKED;

// Secret door (hidden, locked)
tile.customFlags = DOOR_SECRET | DOOR_LOCKED;
```

#### **Trap States**
```cpp
enum TrapFlags : uint8_t
{
    TRAP_ARMED = 0x01,
    TRAP_TRIGGERED = 0x02,
    TRAP_VISIBLE = 0x04,
    TRAP_DISARMED = 0x08
};

// Armed, hidden trap
tile.customFlags = TRAP_ARMED;

// Triggered trap (visible)
tile.customFlags = TRAP_TRIGGERED | TRAP_VISIBLE;

// Disarmed trap
tile.customFlags = TRAP_DISARMED | TRAP_VISIBLE;
```

#### **Interactive Tiles**
```cpp
enum InteractiveFlags : uint8_t
{
    INTERACT_DOOR = 0x01,
    INTERACT_CHEST = 0x02,
    INTERACT_LEVER = 0x04,
    INTERACT_NPC = 0x08
};

// Tile with multiple interactions
tile.customFlags = INTERACT_DOOR | INTERACT_LEVER;
```

### **Custom Flag Examples**

#### **Example 1: Door with Multiple States**
```cpp
void SetupDoor(int x, int y)
{
    TileProperties door;
    door.terrain = TerrainType::Ground;
    door.isBlocked = true;      // Closed
    door.isNavigable = false;
    door.isDynamic = true;
    door.customFlags = 0x00;    // Closed, unlocked
    
    CollisionMap::Get().SetTileProperties(x, y, door);
}

void OpenDoor(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.isBlocked = false;
        tile.isNavigable = true;
        tile.customFlags |= 0x01; // Set "door open" flag
    });
}

void CloseDoor(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.isBlocked = true;
        tile.isNavigable = false;
        tile.customFlags &= ~0x01; // Clear "door open" flag
    });
}

void LockDoor(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.customFlags |= 0x02; // Set "locked" flag
    });
}

bool IsDoorLocked(int x, int y)
{
    const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
    return (tile.customFlags & 0x02) != 0;
}
```

#### **Example 2: Trap System**
```cpp
void ArmTrap(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.customFlags = 0x01; // Trap armed
    });
}

void TriggerTrap(int x, int y)
{
    const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
    
    if (tile.customFlags & 0x01) { // Trap armed
        // Trigger trap effect
        SpawnProjectile(x, y);
        
        // Mark as triggered
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.customFlags &= ~0x01; // Disarm
            tile.customFlags |= 0x02;  // Mark triggered
        });
    }
}
```

#### **Example 3: Buildable Tiles**
```cpp
void MarkBuildable(int x, int y)
{
    TileProperties tile;
    tile.isBlocked = false;
    tile.isNavigable = true;
    tile.customFlags = 0x40; // Buildable flag
    
    CollisionMap::Get().SetTileProperties(x, y, tile);
}

bool CanBuildHere(int x, int y)
{
    const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
    return (tile.customFlags & 0x40) != 0 && tile.isNavigable;
}

void PlaceBuilding(int x, int y)
{
    if (!CanBuildHere(x, y)) return;
    
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.isBlocked = true;
        tile.isNavigable = false;
        tile.customFlags &= ~0x40; // Clear buildable flag
    });
}
```

---

## Dynamic States

Dynamic states enable tiles to transition between different configurations at runtime.

### **Key Fields**

```cpp
bool isDynamic = false;           // Can this tile change state?
std::string onDestroyedState;     // State name after destruction
std::string onBuiltState;         // State name after construction
std::string metadata;             // JSON metadata
```

### **Common Dynamic State Patterns**

#### **1. Destructible Wall → Rubble**

**Initial State (Wall)**:
```cpp
TileProperties wall;
wall.terrain = TerrainType::Rock;
wall.isBlocked = true;
wall.isNavigable = false;
wall.isDynamic = true;
wall.onDestroyedState = "Rubble";
wall.metadata = R"({"health": 100})";

CollisionMap::Get().SetTileProperties(x, y, wall);
```

**After Destruction (Rubble)**:
```cpp
void DestroyWall(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.terrain = TerrainType::Rock;
        tile.isBlocked = false;
        tile.isNavigable = true;
        tile.traversalCost = 1.5f; // Rubble is harder to walk through
        tile.onDestroyedState = ""; // Already destroyed
    });
    
    // Trigger repath for nearby agents
    RepathNearbyAgents(x, y);
}
```

#### **2. Water → Bridge**

**Initial State (Water)**:
```cpp
TileProperties water;
water.terrain = TerrainType::Water;
water.isBlocked = false;
water.isNavigable = false; // Can't walk on water
water.isDynamic = true;
water.onBuiltState = "Bridge";

CollisionMap::Get().SetTileProperties(x, y, water);
```

**After Bridge Built**:
```cpp
void BuildBridge(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.terrain = TerrainType::Ground;
        tile.isBlocked = false;
        tile.isNavigable = true;
        tile.traversalCost = 1.0f;
        tile.onBuiltState = ""; // Already built
        tile.customFlags = 0x01; // Mark as bridge
    });
    
    RepathNearbyAgents(x, y);
}
```

#### **3. Open ↔ Closed Door**

**Initial State (Closed)**:
```cpp
TileProperties closedDoor;
closedDoor.terrain = TerrainType::Ground;
closedDoor.isBlocked = true;
closedDoor.isNavigable = false;
closedDoor.isDynamic = true;
closedDoor.customFlags = 0x00; // Closed

CollisionMap::Get().SetTileProperties(x, y, closedDoor);
```

**Toggle Door**:
```cpp
void ToggleDoor(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        bool isOpen = (tile.customFlags & 0x01) != 0;
        isOpen = !isOpen;
        
        tile.isBlocked = !isOpen;
        tile.isNavigable = isOpen;
        tile.customFlags = isOpen ? 0x01 : 0x00;
    });
    
    RepathNearbyAgents(x, y);
}
```

#### **4. Lava Solidifies → Rock**

**Initial State (Lava)**:
```cpp
TileProperties lava;
lava.terrain = TerrainType::Lava;
lava.isBlocked = false;
lava.isNavigable = false;
lava.isDynamic = true;
lava.onDestroyedState = "Rock"; // "Destruction" = cooling
lava.metadata = R"({"temperature": 1200})";

CollisionMap::Get().SetTileProperties(x, y, lava);
```

**After Cooling (Rock)**:
```cpp
void CoolLava(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.terrain = TerrainType::Rock;
        tile.isBlocked = false;
        tile.isNavigable = true;
        tile.traversalCost = 2.0f; // Rocky terrain
        tile.onDestroyedState = "";
    });
    
    RepathNearbyAgents(x, y);
}
```

#### **5. Ice Melts → Water**

**Initial State (Ice)**:
```cpp
TileProperties ice;
ice.terrain = TerrainType::Ice;
ice.isBlocked = false;
ice.isNavigable = true;
ice.traversalCost = 0.8f;
ice.isDynamic = true;
ice.onDestroyedState = "Water";

CollisionMap::Get().SetTileProperties(x, y, ice);
```

**After Melting (Water)**:
```cpp
void MeltIce(int x, int y)
{
    CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
        tile.terrain = TerrainType::Water;
        tile.isBlocked = false;
        tile.isNavigable = false; // Can't walk on water
        tile.traversalCost = 999.0f;
        tile.onDestroyedState = "";
    });
    
    RepathNearbyAgents(x, y);
}
```

---

## State Transition API

The `UpdateTileState()` method provides a lambda-based API for atomic state transitions.

### **Signature**

```cpp
typedef std::function<void(TileProperties&)> TileUpdateFunc;
void UpdateTileState(int x, int y, TileUpdateFunc updateFunc);
void UpdateTileState(int x, int y, CollisionLayer layer, TileUpdateFunc updateFunc);
```

### **Lambda Patterns**

#### **Pattern 1: Simple State Change**
```cpp
CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    tile.isBlocked = false;
    tile.isNavigable = true;
});
```

#### **Pattern 2: Conditional State Change**
```cpp
CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    if (tile.isBlocked) {
        // Only change if currently blocked
        tile.isBlocked = false;
        tile.isNavigable = true;
    }
});
```

#### **Pattern 3: Toggle State**
```cpp
CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    tile.isBlocked = !tile.isBlocked;
    tile.isNavigable = !tile.isNavigable;
});
```

#### **Pattern 4: Capture External Variables**
```cpp
bool shouldOpen = true;
CollisionMap::Get().UpdateTileState(x, y, [shouldOpen](TileProperties& tile) {
    tile.isBlocked = !shouldOpen;
    tile.isNavigable = shouldOpen;
});
```

#### **Pattern 5: Complex Logic**
```cpp
CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    // Multi-step transformation
    if (tile.terrain == TerrainType::Water) {
        // Freeze water
        tile.terrain = TerrainType::Ice;
        tile.isNavigable = true;
        tile.traversalCost = 0.8f;
    } else if (tile.terrain == TerrainType::Ice) {
        // Melt ice
        tile.terrain = TerrainType::Water;
        tile.isNavigable = false;
        tile.traversalCost = 999.0f;
    }
});
```

#### **Pattern 6: Metadata Update**
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
    // Parse existing metadata
    json meta = tile.metadata.empty() ? json::object() : json::parse(tile.metadata);
    
    // Update metadata
    meta["health"] = meta.value("health", 100) - 25;
    
    if (meta["health"] <= 0) {
        // Destroy tile
        tile.isBlocked = false;
        tile.isNavigable = true;
    }
    
    // Save metadata
    tile.metadata = meta.dump();
});
```

---

## Gameplay Integration Examples

### **Example 1: Dungeon Door System**

```cpp
class DoorSystem
{
public:
    void SetupDoor(int x, int y, bool locked, bool secret)
    {
        TileProperties door;
        door.terrain = TerrainType::Ground;
        door.isBlocked = true;
        door.isNavigable = false;
        door.isDynamic = true;
        door.customFlags = 0x00;
        
        if (locked) door.customFlags |= 0x02;
        if (secret) door.customFlags |= 0x04;
        
        CollisionMap::Get().SetTileProperties(x, y, door);
    }
    
    bool TryOpenDoor(int x, int y, bool hasKey)
    {
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        
        // Check if locked
        if (tile.customFlags & 0x02) {
            if (!hasKey) {
                ShowMessage("Door is locked");
                return false;
            }
        }
        
        // Open door
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.isBlocked = false;
            tile.isNavigable = true;
            tile.customFlags |= 0x01; // Mark open
        });
        
        RepathNearbyAgents(x, y);
        PlaySound("door_open");
        return true;
    }
    
    void CloseDoor(int x, int y)
    {
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.isBlocked = true;
            tile.isNavigable = false;
            tile.customFlags &= ~0x01; // Clear open flag
        });
        
        RepathNearbyAgents(x, y);
        PlaySound("door_close");
    }
};
```

### **Example 2: Destructible Environment**

```cpp
class DestructibleSystem
{
public:
    void DamageWall(int x, int y, float damage)
    {
        CollisionMap::Get().UpdateTileState(x, y, [damage](TileProperties& tile) {
            // Parse metadata
            json meta = tile.metadata.empty() ? json::object() : json::parse(tile.metadata);
            float health = meta.value("health", 100.0f);
            
            health -= damage;
            
            if (health <= 0.0f) {
                // Destroy wall
                tile.isBlocked = false;
                tile.isNavigable = true;
                tile.traversalCost = 1.5f; // Rubble
                tile.terrain = TerrainType::Rock;
            } else {
                // Update health
                meta["health"] = health;
                tile.metadata = meta.dump();
            }
        });
        
        // Check if destroyed
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        if (!tile.isBlocked) {
            // Wall destroyed
            SpawnDebris(x, y);
            RepathNearbyAgents(x, y);
            PlaySound("wall_collapse");
        }
    }
};
```

### **Example 3: Bridge Building**

```cpp
class BuildingSystem
{
public:
    bool CanBuildBridge(int x, int y)
    {
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        
        // Can only build on water
        if (tile.terrain != TerrainType::Water) return false;
        
        // Check if already built
        if (tile.customFlags & 0x01) return false; // Bridge flag
        
        return true;
    }
    
    void BuildBridge(int x, int y)
    {
        if (!CanBuildBridge(x, y)) return;
        
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.terrain = TerrainType::Ground;
            tile.isBlocked = false;
            tile.isNavigable = true;
            tile.traversalCost = 1.0f;
            tile.customFlags = 0x01; // Mark as bridge
        });
        
        RepathNearbyAgents(x, y);
        PlaySound("build_bridge");
    }
    
    void DestroyBridge(int x, int y)
    {
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.terrain = TerrainType::Water;
            tile.isBlocked = false;
            tile.isNavigable = false;
            tile.traversalCost = 999.0f;
            tile.customFlags = 0x00; // Clear bridge flag
        });
        
        RepathNearbyAgents(x, y);
        PlaySound("bridge_collapse");
    }
};
```

### **Example 4: Ice/Fire Environment**

```cpp
class ElementalSystem
{
public:
    void FreezeWater(int x, int y)
    {
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        
        if (tile.terrain != TerrainType::Water) return;
        
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.terrain = TerrainType::Ice;
            tile.isNavigable = true;
            tile.traversalCost = 0.8f; // Slippery/faster
        });
        
        RepathNearbyAgents(x, y);
    }
    
    void MeltIce(int x, int y)
    {
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        
        if (tile.terrain != TerrainType::Ice) return;
        
        CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
            tile.terrain = TerrainType::Water;
            tile.isNavigable = false;
            tile.traversalCost = 999.0f;
        });
        
        RepathNearbyAgents(x, y);
    }
    
    void IgniteTile(int x, int y)
    {
        const TileProperties& tile = CollisionMap::Get().GetTileProperties(x, y);
        
        if (tile.terrain == TerrainType::Grass || tile.terrain == TerrainType::Wood) {
            CollisionMap::Get().UpdateTileState(x, y, [](TileProperties& tile) {
                tile.terrain = TerrainType::Lava; // Fire!
                tile.isNavigable = false;
                tile.customFlags |= 0x02; // Damage flag
            });
            
            // Spread fire to adjacent tiles
            SpreadFire(x, y);
        }
    }
};
```

---

## Summary

This reference covered:

✅ TerrainType enum with detailed use cases  
✅ CollisionLayer enum with multi-layer scenarios  
✅ Custom flags with bit manipulation examples  
✅ Dynamic states for destructible/buildable tiles  
✅ State transition API with lambda patterns  
✅ Complete gameplay integration examples  

For more details, see:
- [Navigation System Reference](Navigation_System_Reference.md)
- [Pathfinding Integration Guide](Pathfinding_Integration_Guide.md)
- [Adding ECS Components Guide](Adding_ECS_Components_Guide.md)
