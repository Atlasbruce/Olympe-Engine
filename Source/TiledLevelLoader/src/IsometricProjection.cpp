/*
 * IsometricProjection.cpp - Isometric coordinate transformations for Olympe Engine
 * 
 * ============================================================================
 * TILED ISOMETRIC COORDINATE SYSTEM - COMPLETE REFERENCE
 * ============================================================================
 * 
 * CRITICAL DISCOVERY: Tiled stores object positions in TMJ files using a 
 * special isometric pixel coordinate system where BOTH X and Y are measured
 * in units of tileHeight pixels along the isometric axes.
 * 
 * TMJ TO WORLD CONVERSION (the correct formula):
 * -----------------------------------------------
 *   1. Convert TMJ pixel coords to tile coords:
 *      tileX = tmjPixelX / tileHeight   (BOTH divided by tileHeight!)
 *      tileY = tmjPixelY / tileHeight
 * 
 *   2. Apply standard isometric projection:
 *      worldX = (tileX - tileY) * (tileWidth / 2)
 *      worldY = (tileX + tileY) * (tileHeight / 2)
 * 
 * WHY BOTH DIVIDED BY tileHeight?
 * --------------------------------
 * In Tiled's isometric view, the X and Y axes run diagonally. Movement along
 * either axis covers the same diagonal distance on screen. Tiled normalizes
 * this by using tileHeight as the unit for BOTH axes, making the coordinate
 * system uniform along both isometric directions.
 * 
 * VERIFIED EXAMPLE (184x128 map, 58x27 tile size):
 * ------------------------------------------------
 *   player_1 in TMJ: (1818.4, 1064.26)
 *   tileX = 1818.4 / 27 = 67.35
 *   tileY = 1064.26 / 27 = 39.42
 *   worldX = (67.35 - 39.42) * 29 = 810
 *   worldY = (67.35 + 39.42) * 13.5 = 1441
 *   Result: Entity renders at tile (67, 39) as expected!
 * 
 * NO ORIGIN OFFSET NEEDED:
 * ------------------------
 * The originX calculation (mapHeight * halfTileWidth) is for Tiled's SCREEN
 * display only. In our engine, both tiles and objects use the same world
 * coordinate system where tile (0,0) is at world (0,0). The camera handles
 * screen positioning.
 * 
 * ============================================================================
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
        // worldX/Y here are tile coordinates, output is screen pixels
        Vector result;
        result.x = ((offsetWorldX - offsetWorldY) * ((float)tileWidth * 0.5f)) + offsetX + globalOffsetX;
        result.y = ((offsetWorldX + offsetWorldY) * ((float)tileHeight * 0.5f)) + offsetY + globalOffsetY;
        return result;
    }

    Vector IsometricProjection::IsoToWorld(float isoX, float isoY, int tileWidth, int tileHeight,
                                          int startX, int startY, float offsetX, float offsetY,
                                          float globalOffsetX, float globalOffsetY)
    {
        // Inverse isometric projection (screen pixels to tile coordinates)
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
        // Tiled's screen origin for display purposes (NOT used in world coords)
        // This is only for reference - our coordinate system doesn't need it
        (void)minTileX;
        (void)minTileY;
        (void)maxTileX;
        
        int mapHeightTiles = maxTileY - minTileY + 1;
        float halfTileWidth = tileWidth * 0.5f;
        
        outOriginX = mapHeightTiles * halfTileWidth;
        outOriginY = 0.0f;
    }

} // namespace Tiled
} // namespace Olympe
