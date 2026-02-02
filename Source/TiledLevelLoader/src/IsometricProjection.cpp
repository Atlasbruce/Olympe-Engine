/*
 * IsometricProjection.cpp - Isometric coordinate transformations
 */

#include "../include/IsometricProjection.h"
#include <cmath>
#include <algorithm>

namespace Olympe {
namespace Tiled {

    Vector IsometricProjection::WorldToIso(float worldX, float worldY, int tileWidth, int tileHeight,
                                          int startX, int startY, float offsetX, float offsetY,
                                          float globalOffsetX, float globalOffsetY)
    {
        // Apply startx/starty offsets to world coordinates
        float offsetWorldX = worldX + startX;
        float offsetWorldY = worldY + startY;
        
        // Standard isometric projection (diamond orientation)
        // Screen X = (worldX - worldY) * (tileWidth / 2)
        // Screen Y = (worldX + worldY) * (tileHeight / 2)
        
        Vector result;
        result.x = ((offsetWorldX - offsetWorldY) * ((float)tileWidth * 0.5f)) + offsetX + globalOffsetX;
        result.y = ((offsetWorldX + offsetWorldY) * ((float)tileHeight * 0.5f)) + offsetY + globalOffsetY;
        return result;
    }

    Vector IsometricProjection::IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                          int startX, int startY, float offsetX, float offsetY,
                                          float globalOffsetX, float globalOffsetY)
    {
        // Inverse isometric projection
        // worldX = (isoX / (tileWidth/2) + isoY / (tileHeight/2)) / 2
        // worldY = (isoY / (tileHeight/2) - isoX / (tileWidth/2)) / 2
        
        Vector result;
        float halfWidth = tileWidth * 0.5f;
        float halfHeight = tileHeight * 0.5f;
        
        // Apply global offsets first (inverse) and pixel offsets
        float adjIsoX = isoX - offsetX - globalOffsetX;
        float adjIsoY = isoY - offsetY - globalOffsetY;
        
        result.x = (adjIsoX / halfWidth + adjIsoY / halfHeight) * 0.5f;
        result.y = (adjIsoY / halfHeight - adjIsoX / halfWidth) * 0.5f;
        
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

    void IsometricProjection::CalculateTMJOrigin(int minTileX, int minTileY, int maxTileX, int maxTileY,
                                                  int tileWidth, int tileHeight,
                                                  float& outOriginX, float& outOriginY)
    {
        // Calculate TMJ origin by projecting all 4 map corners and finding the minimum
        // This aligns Tiled's TMJ coordinate system with the standard isometric projection
        // 
        // Tiled's coordinate system places the north corner of tile (0,0) at
        // position (mapHeight * tileWidth / 2, 0) in TMJ pixels. By calculating
        // the bounding box of all corners, we find where Tiled's (0,0) is in our
        // coordinate system, then negate it to define the origin offset.
        
        float halfTileWidth = tileWidth / 2.0f;
        float halfTileHeight = tileHeight / 2.0f;
        
        // Project 4 corners using standard isometric projection
        // North corner: (minTileX, minTileY) - top of the diamond
        float northX = (minTileX - minTileY) * halfTileWidth;
        float northY = (minTileX + minTileY) * halfTileHeight;
        
        // East corner: (maxTileX, minTileY) - right of the diamond
        float eastX = (maxTileX - minTileY) * halfTileWidth;
        float eastY = (maxTileX + minTileY) * halfTileHeight;
        
        // West corner: (minTileX, maxTileY) - left of the diamond
        float westX = (minTileX - maxTileY) * halfTileWidth;
        float westY = (minTileX + maxTileY) * halfTileHeight;
        
        // South corner: (maxTileX, maxTileY) - bottom of the diamond
        float southX = (maxTileX - maxTileY) * halfTileWidth;
        float southY = (maxTileX + maxTileY) * halfTileHeight;
        
        // Find min X and Y (top-left of bounding box)
        float minX = std::min(std::min(northX, eastX), std::min(westX, southX));
        float minY = std::min(std::min(northY, eastY), std::min(westY, southY));
        
        // Define TMJ origin as negative of the minimum corner position
        // This ensures that when we subtract the origin, we move entities in the correct direction
        outOriginX = -minX;
        outOriginY = -minY;
    }

} // namespace Tiled
} // namespace Olympe
