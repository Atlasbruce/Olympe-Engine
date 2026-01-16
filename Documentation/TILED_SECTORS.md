# Tiled Sector System

## What Are Sectors?

Sectors are **logical divisions** of your game world that define boundaries, zones, or regions. They're used for:

- **Level of Detail (LOD)**: Load/unload assets based on player position
- **Streaming**: Load map chunks dynamically as player explores
- **AI Boundaries**: Define where NPCs can patrol or spawn
- **Gameplay Zones**: Trigger events when player enters/exits areas
- **Rendering Optimization**: Cull entities outside current sector
- **Audio Zones**: Change music/ambience per sector
- **Physics Culling**: Disable distant physics calculations

In Tiled, sectors are created using **polygon** or **rectangle objects** with type `"sector"`.

## Sector Types

### Polygon Sectors

Complex, irregular boundaries defined by vertices.

**Use Cases:**
- Natural terrain boundaries (forests, lakes, cliffs)
- Organic level zones
- Irregular gameplay areas

**Example in Tiled:**
```
Object Layer: "Sectors"
  └─ Polygon Object
      ├─ Name: "forest_sector_01"
      ├─ Type: "sector"
      └─ Points: [(0,0), (100,0), (150,50), (100,100), (0,100)]
```

**Advantages:**
- Precise boundary control
- Natural-looking zones
- Memory efficient for complex shapes

**Disadvantages:**
- More complex collision detection
- Harder to edit/visualize

### Rectangle Sectors

Simple axis-aligned rectangular boundaries.

**Use Cases:**
- Grid-based level divisions
- Building interiors
- Uniform chunk streaming

**Example in Tiled:**
```
Object Layer: "Sectors"
  └─ Rectangle Object
      ├─ Name: "dungeon_room_01"
      ├─ Type: "sector"
      ├─ X: 0, Y: 0
      ├─ Width: 320, Height: 240
```

**Advantages:**
- Fast AABB collision detection
- Easy to edit and visualize
- Simple chunk alignment

**Disadvantages:**
- Less flexible than polygons
- May not fit organic shapes

## Automatic Sector Calculation (Infinite Maps)

For **infinite maps**, Olympe Engine automatically generates sectors based on chunk boundaries.

### Algorithm

```cpp
void CalculateSectorsFromChunks(const TiledMap& map, LevelDefinition& level) {
    if (!map.infinite) return;
    
    for (const auto& layer : map.layers) {
        if (layer->type != LayerType::TileLayer) continue;
        
        for (const auto& chunk : layer->chunks) {
            Sector sector;
            sector.name = "auto_chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.y);
            sector.bounds = {
                chunk.x * map.tilewidth,
                chunk.y * map.tileheight,
                (chunk.x + chunk.width) * map.tilewidth,
                (chunk.y + chunk.height) * map.tileheight
            };
            level.sectors.push_back(sector);
        }
    }
}
```

### How It Works

1. **Detect Infinite Map**: Check `map.infinite == true`
2. **Iterate Chunks**: Loop through all tile layer chunks
3. **Create Sector per Chunk**: Each chunk becomes a sector
4. **Calculate Bounds**: Chunk position/size → world coordinates
5. **Auto-Name**: Format: `"auto_chunk_X_Y"` (e.g., "auto_chunk_0_0", "auto_chunk_16_0")

### Example

**Tiled Map:**
```json
{
  "infinite": true,
  "chunkwidth": 16,
  "chunkheight": 16,
  "tilewidth": 32,
  "tileheight": 32
}
```

**Generated Sectors:**
- Chunk (0, 0) → Sector bounds: (0, 0, 512, 512)
- Chunk (16, 0) → Sector bounds: (512, 0, 1024, 512)
- Chunk (0, 16) → Sector bounds: (0, 512, 512, 1024)

**Streaming Logic:**
```cpp
// Load only sectors near player
for (const auto& sector : level.sectors) {
    float distance = DistanceToPlayer(sector.bounds);
    if (distance < STREAM_RADIUS) {
        LoadSector(sector);
    } else {
        UnloadSector(sector);
    }
}
```

## Sector Metadata Storage

Sectors are stored in `LevelDefinition` with their boundaries and custom properties.

### Data Structure

```cpp
struct Sector {
    std::string name;
    std::vector<Vec2> polygon;  // For polygon sectors
    AABB bounds;                // Bounding box (for rect or polygon)
    nlohmann::json properties;  // Custom properties from Tiled
};

struct LevelDefinition {
    std::vector<Sector> sectors;
    // ... other level data
};
```

### Custom Properties

Add custom properties to sector objects in Tiled to store metadata:

**Example Properties:**
- `biome` (string): "forest", "desert", "cave"
- `difficulty` (int): 1-10
- `music` (file): Path to music track for this sector
- `ambientColor` (color): #RRGGBB for lighting
- `spawnRate` (float): Enemy spawn multiplier
- `fogDensity` (float): 0.0-1.0

**Accessing in Code:**
```cpp
for (const auto& sector : level.sectors) {
    if (sector.properties.contains("biome")) {
        std::string biome = sector.properties["biome"];
        SYSTEM_LOG << "Sector " << sector.name << " is biome: " << biome << "\n";
    }
}
```

## Creating Sectors in Tiled

### Step-by-Step Guide

1. **Create Object Layer**:
   - Layer → New Layer → Object Layer
   - Name: "Sectors" or "Zones"

2. **Draw Sector (Rectangle)**:
   - Select Rectangle tool (R)
   - Click and drag to define area
   - Set object type to "sector"
   - Set name property (e.g., "town_square")

3. **Draw Sector (Polygon)**:
   - Select Polygon tool (P)
   - Click to place vertices
   - Double-click to close polygon
   - Set object type to "sector"
   - Set name property

4. **Add Custom Properties**:
   - Select sector object
   - Properties panel → Custom Properties
   - Add properties as needed

5. **Visualize Sectors**:
   - Set sector layer color (e.g., yellow, semi-transparent)
   - Toggle layer visibility to see overlap with tiles

### Best Practices

✅ **Do:**
- Name sectors descriptively ("forest_north", "dungeon_entrance")
- Keep sector boundaries aligned with visual landmarks
- Use rectangles for grid-based maps
- Use polygons for organic shapes
- Add custom properties for metadata
- Document sector purposes in map comments

❌ **Don't:**
- Overlap sectors (causes ambiguity)
- Create too many tiny sectors (overhead)
- Use generic names ("sector1", "sector2")
- Forget to set object type to "sector"

## Level of Detail (LOD) Concepts

Sectors enable LOD by controlling asset resolution based on distance.

### LOD Levels

| Level | Distance | Assets Loaded |
|-------|----------|---------------|
| LOD 0 | 0-512px | Full detail (high-res textures, all entities) |
| LOD 1 | 512-1024px | Medium detail (mid-res textures, major entities) |
| LOD 2 | 1024-2048px | Low detail (low-res textures, visible entities only) |
| LOD 3 | 2048+px | Minimal (culled, placeholder sprites) |

### Implementation Example

```cpp
void UpdateSectorLOD(Sector& sector, Vec2 playerPos) {
    float distance = Distance(playerPos, sector.bounds.center);
    
    if (distance < 512.0f) {
        sector.lodLevel = 0;
        LoadHighDetailAssets(sector);
    } else if (distance < 1024.0f) {
        sector.lodLevel = 1;
        LoadMediumDetailAssets(sector);
    } else if (distance < 2048.0f) {
        sector.lodLevel = 2;
        LoadLowDetailAssets(sector);
    } else {
        sector.lodLevel = 3;
        UnloadAssets(sector);
    }
}
```

## Streaming Concepts

Sectors enable dynamic loading/unloading of map data at runtime.

### Streaming Workflow

1. **Track Player Position**: Update each frame
2. **Calculate Sector Distances**: Distance from player to each sector center
3. **Prioritize Sectors**: Sort by distance
4. **Load Near Sectors**: Sectors within `STREAM_IN_RADIUS`
5. **Unload Far Sectors**: Sectors beyond `STREAM_OUT_RADIUS`
6. **Budget Management**: Limit loads per frame to avoid hitches

### Streaming States

```cpp
enum class SectorState {
    Unloaded,   // Not in memory
    Loading,    // Async load in progress
    Loaded,     // In memory, active
    Unloading   // Async unload in progress
};
```

### Hysteresis

Use different radii for load/unload to prevent thrashing:

```cpp
const float STREAM_IN_RADIUS = 1024.0f;   // Load at this distance
const float STREAM_OUT_RADIUS = 1536.0f;  // Unload at this distance (farther)
```

**Why?** If player moves along boundary with same radius, sectors would constantly load/unload.

## Point-in-Sector Queries

Determine which sector(s) contain a point (e.g., player position).

### Rectangle Sector

```cpp
bool IsPointInRectSector(Vec2 point, const Sector& sector) {
    return point.x >= sector.bounds.minX &&
           point.x <= sector.bounds.maxX &&
           point.y >= sector.bounds.minY &&
           point.y <= sector.bounds.maxY;
}
```

### Polygon Sector (Ray Casting)

```cpp
bool IsPointInPolygonSector(Vec2 point, const Sector& sector) {
    int crossings = 0;
    int n = sector.polygon.size();
    
    for (int i = 0; i < n; ++i) {
        Vec2 v1 = sector.polygon[i];
        Vec2 v2 = sector.polygon[(i + 1) % n];
        
        if (((v1.y > point.y) != (v2.y > point.y)) &&
            (point.x < (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x)) {
            crossings++;
        }
    }
    
    return (crossings % 2) == 1;
}
```

### Query Usage

```cpp
Sector* GetPlayerSector(Vec2 playerPos, const LevelDefinition& level) {
    for (auto& sector : level.sectors) {
        if (sector.polygon.empty()) {
            // Rectangle sector
            if (IsPointInRectSector(playerPos, sector)) {
                return &sector;
            }
        } else {
            // Polygon sector
            if (IsPointInPolygonSector(playerPos, sector)) {
                return &sector;
            }
        }
    }
    return nullptr;
}
```

## Sector-Based Optimizations

### Entity Culling

```cpp
void CullEntitiesBySector(const Sector& currentSector) {
    for (EntityID entity : allEntities) {
        Vec2 entityPos = GetPosition(entity);
        if (!IsPointInSector(entityPos, currentSector)) {
            SetEntityVisible(entity, false);
        } else {
            SetEntityVisible(entity, true);
        }
    }
}
```

### Physics Culling

```cpp
void UpdatePhysics(const Sector& currentSector, float deltaTime) {
    for (EntityID entity : physicsEntities) {
        Vec2 entityPos = GetPosition(entity);
        if (IsPointInSector(entityPos, currentSector) || IsInAdjacentSector(entityPos)) {
            UpdatePhysicsForEntity(entity, deltaTime);
        }
        // Else: skip physics update for distant entity
    }
}
```

### AI Boundaries

```cpp
void ConstrainNPCMovement(EntityID npc, const Sector& allowedSector) {
    Vec2 targetPos = GetTargetPosition(npc);
    
    if (!IsPointInSector(targetPos, allowedSector)) {
        // Clamp to sector boundary
        targetPos = ClampToSectorBounds(targetPos, allowedSector);
        SetTargetPosition(npc, targetPos);
    }
}
```

## Example: Forest with 3 Sectors

### Tiled Setup

**Map:** forest_level.tmj (2000x2000px, 32x32 tiles)

**Object Layer: "Sectors"**
1. **Polygon Object**
   - Name: "forest_entrance"
   - Type: sector
   - Polygon: Covers northwest area (irregular shape)
   - Properties: `biome: "forest_light"`, `music: "forest_calm.ogg"`

2. **Rectangle Object**
   - Name: "forest_deep"
   - Type: sector
   - Rectangle: (800, 800, 800, 800)
   - Properties: `biome: "forest_dark"`, `music: "forest_tense.ogg"`, `spawnRate: 2.0`

3. **Polygon Object**
   - Name: "forest_lake"
   - Type: sector
   - Polygon: Covers southeast lake (irregular)
   - Properties: `biome: "forest_water"`, `music: "forest_ambient.ogg"`, `fogDensity: 0.3`

### Runtime Behavior

```cpp
// Player enters "forest_deep"
Sector* sector = GetPlayerSector(playerPos, level);
if (sector && sector->name == "forest_deep") {
    PlayMusic(sector->properties["music"]);
    SetEnemySpawnRate(sector->properties["spawnRate"]);
    SYSTEM_LOG << "Entered sector: " << sector->name << "\n";
}
```

## Future Features (Planned)

- **Sector Portals**: Define connections between non-adjacent sectors
- **Sector Transitions**: Smooth blending between sector properties (music fade, fog)
- **Hierarchical Sectors**: Nested sectors (country → region → town → building)
- **Dynamic Sectors**: Runtime creation/modification
- **Sector Events**: Trigger scripts on enter/exit
- **Sector Instances**: Multiple instances of same sector template

## Performance Considerations

### Memory

- **Rectangle Sector**: ~64 bytes (name, AABB, properties)
- **Polygon Sector**: ~64 bytes + (16 bytes × vertex count)

**Example:** 100 sectors × 100 bytes ≈ **10 KB**

### CPU

- **Point-in-Rectangle**: ~10 CPU instructions (very fast)
- **Point-in-Polygon**: ~50 CPU instructions per vertex (slower)

**Optimization:** Pre-calculate AABB for polygons, test AABB first before expensive polygon test.

### Recommendations

- **< 100 sectors**: No concerns
- **100-500 sectors**: Use spatial partitioning (quadtree)
- **> 500 sectors**: Consider hierarchical sectors or merge small sectors

## See Also

- [TILED_INTEGRATION.md](TILED_INTEGRATION.md) - General integration guide
- [TILED_WORKFLOW.md](TILED_WORKFLOW.md) - Creating sectors in Tiled
- [TILED_OBJECT_TYPES.md](TILED_OBJECT_TYPES.md) - Object type reference
