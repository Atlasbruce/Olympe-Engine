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
        // ==========================================================================
        // TMJ ISOMETRIC ORIGIN CALCULATION
        // ==========================================================================
        //
        // In Tiled's isometric coordinate system, the origin is at the TOP (north)
        // corner of the diamond. The map extends:
        // - Right-down along the X axis (increasing tileX)
        // - Left-down along the Y axis (increasing tileY)
        //
        // The isometric origin X offset ensures tile (0,0) appears at the correct
        // screen position. It equals: mapHeight * (tileWidth / 2)
        //
        // This places the northwest edge of the map at screen X = 0.
        //
        // VERIFICATION (184x128 map, 58x27 tiles):
        // - player_1 TMJ: (1818.63, 1064.03)
        // - Expected tile: (67, 39)
        // - tileX = 1818.63 / 27 = 67.36 ?
        // - tileY = 1064.03 / 27 = 39.41 ?
        // - originX = 128 * 29 = 3712
        // - screenX = (67.36 - 39.41) * 29 + 3712 = 4522.55 ?
        // - screenY = (67.36 + 39.41) * 13.5 = 1441.40 ?
        // - Matches mouse position (4523, 1443) in Tiled! ?
        //
        // ==========================================================================
        
        (void)minTileX;
        (void)minTileY;
        (void)maxTileX;
        
        int mapHeightTiles = maxTileY - minTileY + 1;
        float halfTileWidth = tileWidth * 0.5f;
        
        // Origin X = map height in tiles * half tile width
        // Origin Y = 0 (top of diamond)
        outOriginX = mapHeightTiles * halfTileWidth;
        outOriginY = 0.0f;
    }

} // namespace Tiled
} // namespace Olympe
