/*
 * IsometricProjection.h - Isometric coordinate transformation utilities
 * 
 * Provides conversion between world space (Cartesian) and
 * isometric screen space coordinates for isometric maps.
 */

#pragma once

namespace Olympe {
namespace Tiled {

    struct Vec2
    {
        float x;
        float y;

        Vec2() : x(0.0f), y(0.0f) {}
        Vec2(float x_, float y_) : x(x_), y(y_) {}
    };

    class IsometricProjection
    {
    public:
        // Convert world (tile) coordinates to isometric screen coordinates
        // worldX, worldY: tile coordinates (e.g., tile 0,0 = world 0,0)
        // tileWidth, tileHeight: dimensions of a single isometric tile in pixels
        // Returns screen pixel position
        static Vec2 WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight);

        // Convert isometric screen coordinates to world (tile) coordinates
        // isoX, isoY: screen pixel coordinates
        // tileWidth, tileHeight: dimensions of a single isometric tile in pixels
        // Returns world tile coordinates (may be fractional)
        static Vec2 IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight);

        // Get tile at screen position
        static void ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight,
                                 int& outTileX, int& outTileY);

        // Get screen position of tile corner
        static Vec2 TileToScreen(int tileX, int tileY, int tileWidth, int tileHeight);
    };

} // namespace Tiled
} // namespace Olympe
