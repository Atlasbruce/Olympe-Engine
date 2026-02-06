/*
 * IsometricProjection.h - Isometric coordinate transformation utilities
 * 
 * TILED ISOMETRIC COORDINATE SYSTEM:
 * ----------------------------------
 * Tiled stores object positions in TMJ files using isometric pixel coordinates
 * where BOTH X and Y are measured in tileHeight units along isometric axes.
 * 
 * TMJ to World conversion:
 *   tileX = tmjPixelX / tileHeight
 *   tileY = tmjPixelY / tileHeight
 *   worldX = (tileX - tileY) * (tileWidth / 2)
 *   worldY = (tileX + tileY) * (tileHeight / 2)
 * 
 * See IsometricProjection.cpp for detailed documentation and examples.
 */

#pragma once
#include "../../vector.h"  // Use engine's Vector class

namespace Olympe {
namespace Tiled {

    class IsometricProjection
    {
    public:
        // Convert tile coordinates to screen coordinates (isometric projection)
        static Vector WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight, 
                                int startX = 0, int startY = 0, float offsetX = 0.0f, float offsetY = 0.0f,
                                float globalOffsetX = 0.0f, float globalOffsetY = 0.0f);

        // Convert screen coordinates to tile coordinates (inverse isometric)
        static Vector IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                int startX = 0, int startY = 0, float offsetX = 0.0f, float offsetY = 0.0f,
                                float globalOffsetX = 0.0f, float globalOffsetY = 0.0f);

        // Get tile at screen position
        static void ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight,
                                 int& outTileX, int& outTileY);

        // Get screen position of tile corner
        static Vector TileToScreen(int tileX, int tileY, int tileWidth, int tileHeight);
        
        // Calculate Tiled's screen origin (for reference, not used in world coords)
        static void CalculateTMJOrigin(int minTileX, int minTileY, int maxTileX, int maxTileY,
                                       int tileWidth, int tileHeight,
                                       float& outOriginX, float& outOriginY);
    };

} // namespace Tiled
} // namespace Olympe
