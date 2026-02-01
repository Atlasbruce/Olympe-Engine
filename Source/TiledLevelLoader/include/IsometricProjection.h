/*
 * IsometricProjection.h - Isometric coordinate transformation utilities
 * 
 * Provides conversion between world space (Cartesian) and
 * isometric screen space coordinates for isometric maps.
 */

#pragma once
#include "../../vector.h"  // Use engine's Vector class

namespace Olympe {
namespace Tiled {

    class IsometricProjection
    {
    public:
        // Convert world (tile) coordinates to isometric screen coordinates
        // worldX, worldY: tile coordinates (e.g., tile 0,0 = world 0,0)
        // tileWidth, tileHeight: dimensions of a single isometric tile in pixels
        // startX, startY: tile offset from layer startx/starty properties (default 0)
        // Returns screen pixel position
        static Vector WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight, 
                                int startX = 0, int startY = 0);

        // Convert isometric screen coordinates to world (tile) coordinates
        // isoX, isoY: screen pixel coordinates
        // tileWidth, tileHeight: dimensions of a single isometric tile in pixels
        // startX, startY: tile offset from layer startx/starty properties (default 0)
        // Returns world tile coordinates (may be fractional)
        static Vector IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                int startX = 0, int startY = 0);

        // Get tile at screen position
        static void ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight,
                                 int& outTileX, int& outTileY);

        // Get screen position of tile corner
        static Vector TileToScreen(int tileX, int tileY, int tileWidth, int tileHeight);
    };

} // namespace Tiled
} // namespace Olympe
