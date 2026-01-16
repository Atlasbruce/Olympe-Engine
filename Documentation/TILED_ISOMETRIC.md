# Tiled Isometric Maps - Technical Guide

## Isometric Projection Overview

Isometric projection creates a pseudo-3D appearance by rendering tiles in a diamond pattern. Olympe Engine's Tiled integration fully supports isometric maps with proper coordinate transformations, diamond rendering, and tile flipping.

### Coordinate Systems

**Tiled Coordinates (Grid Space):**
- Origin: (0, 0) at top-left tile
- X axis: Increases right
- Y axis: Increases down
- Units: Tiles

**World Coordinates (Cartesian):**
- Origin: Configurable (typically center or bottom-left)
- X axis: Increases right (horizontal)
- Y axis: Increases down (Olympe convention) or up (mathematical)
- Units: Pixels

**Screen Coordinates (Isometric Space):**
- Origin: Top-left of screen
- Diamond-shaped tile arrangement
- Tiles overlap visually
- Units: Pixels

### Isometric Tile Aspect Ratio

Olympe Engine uses **2:1 isometric tiles** (industry standard):
- Tile width: 58 pixels
- Tile height: 27 pixels  
- Ratio: 58:27 ≈ 2.148:1 (close to 2:1)

This creates a 30° angle from horizontal, producing a natural isometric look.

**Common Isometric Tile Sizes:**
- 64x32 (exactly 2:1) - Common in indie games
- 128x64 (exactly 2:1) - Higher resolution
- 58x27 (2.148:1) - Olympe Engine default
- 116x54 (2.148:1) - Double Olympe resolution

## World-to-Screen Conversion

Convert grid coordinates to isometric screen position.

### Formula

```cpp
screenX = (gridX - gridY) * (tileWidth / 2)
screenY = (gridX + gridY) * (tileHeight / 2)
```

**For 58x27 tiles:**
```cpp
screenX = (gridX - gridY) * 29
screenY = (gridX + gridY) * 13.5
```

### Example

Given tile at grid position (4, 2):

```
screenX = (4 - 2) * 29 = 2 * 29 = 58
screenY = (4 + 2) * 13.5 = 6 * 13.5 = 81
```

The tile renders at screen position (58, 81).

### Code Implementation

```cpp
struct Vec2 {
    float x, y;
};

Vec2 GridToScreen(int gridX, int gridY, int tileWidth, int tileHeight) {
    return {
        (gridX - gridY) * (tileWidth / 2.0f),
        (gridX + gridY) * (tileHeight / 2.0f)
    };
}

// Example: 58x27 tiles
Vec2 screenPos = GridToScreen(4, 2, 58, 27);
// screenPos = (58.0, 81.0)
```

## Screen-to-World Conversion

Convert screen coordinates to grid position (e.g., mouse picking).

### Formula

```cpp
gridX = (screenX / (tileWidth / 2) + screenY / (tileHeight / 2)) / 2
gridY = (screenY / (tileHeight / 2) - screenX / (tileWidth / 2)) / 2
```

**For 58x27 tiles:**
```cpp
gridX = (screenX / 29.0 + screenY / 13.5) / 2
gridY = (screenY / 13.5 - screenX / 29.0) / 2
```

### Example

Given screen position (116, 108):

```
gridX = (116 / 29.0 + 108 / 13.5) / 2 = (4.0 + 8.0) / 2 = 6.0
gridY = (108 / 13.5 - 116 / 29.0) / 2 = (8.0 - 4.0) / 2 = 2.0
```

The click is on tile (6, 2).

### Code Implementation

```cpp
struct GridPos {
    int x, y;
};

GridPos ScreenToGrid(float screenX, float screenY, int tileWidth, int tileHeight) {
    float halfWidth = tileWidth / 2.0f;
    float halfHeight = tileHeight / 2.0f;
    
    float gridXFloat = (screenX / halfWidth + screenY / halfHeight) / 2.0f;
    float gridYFloat = (screenY / halfHeight - screenX / halfWidth) / 2.0f;
    
    return {
        static_cast<int>(std::floor(gridXFloat)),
        static_cast<int>(std::floor(gridYFloat))
    };
}

// Example: Mouse at (232, 162)
GridPos tile = ScreenToGrid(232, 162, 58, 27);
// tile = (8, 4)
```

## Isometric Rendering Pipeline

### Render Order (Diamond Pattern)

Isometric tiles must be rendered in **back-to-front** order to ensure proper visual layering.

**Standard Render Order:**
1. Start at top tile (gridY = 0, gridX = 0)
2. Iterate diagonally: increase gridY, then gridX
3. Render row by row: top-left to bottom-right

**Pseudo-code:**
```cpp
for (int y = 0; y < mapHeight; ++y) {
    for (int x = 0; x < mapWidth; ++x) {
        RenderTile(x, y);
    }
}
```

**Why this order?**  
Because isometric Y coordinates increase down-right, tiles at higher Y values are visually "in front" of tiles at lower Y values.

### Diamond-Shaped Culling

Only render tiles visible in viewport to optimize performance.

```cpp
// Calculate visible tile range
int minGridX = (viewportMinX / (tileWidth / 2) + viewportMinY / (tileHeight / 2)) / 2;
int maxGridX = (viewportMaxX / (tileWidth / 2) + viewportMaxY / (tileHeight / 2)) / 2;
int minGridY = (viewportMinY / (tileHeight / 2) - viewportMaxX / (tileWidth / 2)) / 2;
int maxGridY = (viewportMaxY / (tileHeight / 2) - viewportMinX / (tileWidth / 2)) / 2;

// Clamp to map bounds
minGridX = std::max(0, minGridX - 1);
maxGridX = std::min(mapWidth - 1, maxGridX + 1);
minGridY = std::max(0, minGridY - 1);
maxGridY = std::min(mapHeight - 1, maxGridY + 1);

// Render only visible tiles
for (int y = minGridY; y <= maxGridY; ++y) {
    for (int x = minGridX; x <= maxGridX; ++x) {
        RenderTile(x, y);
    }
}
```

### Depth Sorting Objects

Entities and objects on isometric maps need depth sorting based on their Y position.

**Sorting Rule:**
```cpp
// Sort entities by Y position (higher Y = draw later = appears in front)
std::sort(entities.begin(), entities.end(), [](const Entity& a, const Entity& b) {
    return a.position.y < b.position.y;
});
```

**Multi-tile objects:**  
Use the **bottom-most tile** Y position as the sort key.

## Tile Flipping Handling

Tiled supports flipping tiles horizontally, vertically, and diagonally. These flags are encoded in the tile ID.

### Flip Flag Encoding

```cpp
const uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
const uint32_t FLIPPED_VERTICALLY_FLAG   = 0x40000000;
const uint32_t FLIPPED_DIAGONALLY_FLAG   = 0x20000000;
const uint32_t TILE_ID_MASK              = 0x1FFFFFFF;

uint32_t tileID = tileData[index];

bool flippedH = (tileID & FLIPPED_HORIZONTALLY_FLAG) != 0;
bool flippedV = (tileID & FLIPPED_VERTICALLY_FLAG) != 0;
bool flippedD = (tileID & FLIPPED_DIAGONALLY_FLAG) != 0;

uint32_t actualTileID = tileID & TILE_ID_MASK;
```

### Rendering Flipped Tiles

Apply transformations when rendering:

```cpp
SDL_RendererFlip flip = SDL_FLIP_NONE;
if (flippedH) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
if (flippedV) flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);

double rotation = 0.0;
if (flippedD) {
    // Diagonal flip = 90° rotation + horizontal flip
    rotation = 90.0;
    flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
}

SDL_RenderCopyEx(renderer, texture, &srcRect, &dstRect, rotation, nullptr, flip);
```

### Flip Combinations

| Horizontal | Vertical | Diagonal | Result |
|------------|----------|----------|--------|
| No | No | No | Original |
| Yes | No | No | Mirrored horizontally |
| No | Yes | No | Mirrored vertically |
| Yes | Yes | No | Rotated 180° |
| No | No | Yes | Rotated 90° CW + H-flip |
| Yes | No | Yes | Rotated 90° CW |
| No | Yes | Yes | Rotated 270° CW |
| Yes | Yes | Yes | Rotated 90° CCW |

## Isometric Map Example (58x27 tiles)

### Map Configuration

```json
{
  "compressionlevel": -1,
  "height": 10,
  "width": 10,
  "infinite": false,
  "orientation": "isometric",
  "renderorder": "right-down",
  "tiledversion": "1.11.0",
  "tileheight": 27,
  "tilewidth": 58,
  "type": "map",
  "version": "1.10"
}
```

### Tile Positions Visualization

```
Grid:           Screen (approx):
(0,0)           (0, 0)
(1,0)           (29, 13.5)
(0,1)           (-29, 13.5)
(1,1)           (0, 27)
(2,0)           (58, 27)
(0,2)           (-58, 27)
```

**Diamond Pattern:**
```
        (0,0)
       /    \
    (0,1)  (1,0)
     / \    / \
 (0,2) (1,1) (2,0)
   \   / \   /
    (1,2) (2,1)
```

## Isometric Collision Detection

### AABB in Isometric Space

Convert tile bounds to world AABB for collision:

```cpp
struct AABB {
    float minX, minY, maxX, maxY;
};

AABB GetTileBounds(int gridX, int gridY, int tileWidth, int tileHeight) {
    Vec2 center = GridToScreen(gridX, gridY, tileWidth, tileHeight);
    float halfW = tileWidth / 2.0f;
    float halfH = tileHeight / 2.0f;
    
    return {
        center.x - halfW,
        center.y - halfH,
        center.x + halfW,
        center.y + halfH
    };
}
```

### Diamond-Shaped Collision

For pixel-perfect isometric collision, use diamond mask:

```cpp
bool IsInDiamond(float x, float y, Vec2 center, int tileWidth, int tileHeight) {
    float dx = std::abs(x - center.x);
    float dy = std::abs(y - center.y);
    
    float halfW = tileWidth / 2.0f;
    float halfH = tileHeight / 2.0f;
    
    // Diamond equation: dx/halfW + dy/halfH <= 1
    return (dx / halfW + dy / halfH) <= 1.0f;
}
```

## Parallax Layers in Isometric

Parallax scrolling works similarly to orthogonal maps, but scrolling feels more natural with adjusted factors.

**Recommended Parallax Factors:**
- Background mountains: `parallaxx: 0.3, parallaxy: 0.3`
- Mid-ground clouds: `parallaxx: 0.6, parallaxy: 0.6`
- Foreground overlay: `parallaxx: 1.2, parallaxy: 1.2`

**Note:** Keep parallax factors equal for X and Y to maintain isometric perspective.

## Performance Optimization

### Chunk Rendering

For large isometric maps, use Tiled's infinite map format with chunks:

```cpp
// Render only chunks in viewport
for (const auto& chunk : map.chunks) {
    if (IsChunkVisible(chunk, viewport)) {
        RenderChunk(chunk);
    }
}
```

### Tile Batching

Batch tiles using same tileset into single draw call:

```cpp
// Group tiles by texture
std::map<SDL_Texture*, std::vector<TileInstance>> tileBatches;

for (const auto& tile : visibleTiles) {
    SDL_Texture* texture = GetTileTexture(tile.tileID);
    tileBatches[texture].push_back(tile);
}

// Render each batch
for (const auto& [texture, tiles] : tileBatches) {
    RenderTileBatch(texture, tiles);
}
```

## Common Pitfalls

### Wrong Render Order
❌ **Don't:** Render left-to-right, top-to-bottom  
✅ **Do:** Render in diamond order (back-to-front)

### Incorrect Coordinate Conversion
❌ **Don't:** Use orthogonal formulas for isometric  
✅ **Do:** Use proper isometric projection formulas

### Forgetting Flip Flags
❌ **Don't:** Render tile ID directly  
✅ **Do:** Extract actual ID and apply flip transformations

### Ignoring Z-Order
❌ **Don't:** Render objects in arbitrary order  
✅ **Do:** Sort by Y position (or explicit Z-order property)

### Pixel-Perfect Collision on Diamond
❌ **Don't:** Use rectangular AABB for clickable tiles  
✅ **Do:** Use diamond-shaped hit detection for precision

## See Also

- [TILED_INTEGRATION.md](TILED_INTEGRATION.md) - General integration guide
- [TILED_WORKFLOW.md](TILED_WORKFLOW.md) - Creating isometric maps in Tiled
- [Tiled Manual - Isometric Maps](https://doc.mapeditor.org/en/stable/manual/editing-tilesets/#isometric-tiles)
