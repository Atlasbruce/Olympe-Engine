/*
 * IsometricProjection.cpp - Isometric coordinate transformations
 */

#include "../include/IsometricProjection.h"
#include <cmath>

namespace Olympe {
namespace Tiled {

    Vector IsometricProjection::WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight,
                                          int startX, int startY)
    {
        // Apply startx/starty offsets to world coordinates
        float offsetWorldX = worldX + startX;
        float offsetWorldY = worldY + startY;
        
        // Standard isometric projection (diamond orientation)
        // Screen X = (worldX - worldY) * (tileWidth / 2)
        // Screen Y = (worldX + worldY) * (tileHeight / 2)
        
        Vector result;
        result.x = (offsetWorldX - offsetWorldY) * ((float)tileWidth * 0.5f);
        result.y = (offsetWorldX + offsetWorldY) * ((float)tileHeight * 0.5f);
        return result;
    }

    Vector IsometricProjection::IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                          int startX, int startY)
    {
        // Inverse isometric projection
        // worldX = (isoX / (tileWidth/2) + isoY / (tileHeight/2)) / 2
        // worldY = (isoY / (tileHeight/2) - isoX / (tileWidth/2)) / 2
        
        Vector result;
        float halfWidth = tileWidth * 0.5f;
        float halfHeight = tileHeight * 0.5f;
        
        result.x = (isoX / halfWidth + isoY / halfHeight) * 0.5f;
        result.y = (isoY / halfHeight - isoX / halfWidth) * 0.5f;
        
        // Apply startx/starty offsets (inverse)
        result.x -= startX;
        result.y -= startY;
        
        return result;
    }

    void IsometricProjection::ScreenToTile(float screenX, float screenY, int tileWidth, int tileHeight,
                                           int& outTileX, int& outTileY)
    {
        Vector world = IsoToWorld(screenX, screenY, tileWidth, tileHeight);
        outTileX = static_cast<int>(std::floor(world.x));
        outTileY = static_cast<int>(std::floor(world.y));
    }

    Vector IsometricProjection::TileToScreen(int tileX, int tileY, int tileWidth, int tileHeight)
    {
        return WorldToIso(static_cast<float>(tileX), static_cast<float>(tileY), 
                         tileWidth, tileHeight);
    }

} // namespace Tiled
} // namespace Olympe
