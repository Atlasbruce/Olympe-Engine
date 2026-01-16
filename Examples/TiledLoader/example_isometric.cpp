/*
 * example_isometric.cpp - Example of working with isometric maps
 * 
 * This example demonstrates:
 * 1. Loading an isometric map
 * 2. Converting coordinates between world and isometric space
 * 3. Mouse picking in isometric view
 */

#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/IsometricProjection.h"
#include "system/system_utils.h"

using namespace Olympe::Tiled;

int main()
{
    Logging::InitLogger();
    
    SYSTEM_LOG << "=== Isometric Map Example ===" << std::endl;
    
    // Load isometric map
    TiledLevelLoader loader;
    TiledMap map;
    
    if (!loader.LoadFromFile("Resources/Maps/isometric_level.tmj", map)) {
        SYSTEM_LOG << "ERROR: " << loader.GetLastError() << std::endl;
        return 1;
    }
    
    if (map.orientation != MapOrientation::Isometric) {
        SYSTEM_LOG << "WARNING: Map is not isometric!" << std::endl;
    }
    
    SYSTEM_LOG << "Loaded isometric map: " << map.width << "x" << map.height << std::endl;
    SYSTEM_LOG << "Tile size: " << map.tilewidth << "x" << map.tileheight << std::endl;
    
    // ========================================================================
    // Coordinate Transformations
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Coordinate Transformations ---" << std::endl;
    
    // Example: Convert tile coordinates to screen position
    int tileX = 5;
    int tileY = 3;
    
    Vec2 screenPos = IsometricProjection::TileToScreen(tileX, tileY, map.tilewidth, map.tileheight);
    SYSTEM_LOG << "Tile (" << tileX << ", " << tileY << ") -> Screen (" 
               << screenPos.x << ", " << screenPos.y << ")" << std::endl;
    
    // Convert back to verify
    Vec2 worldPos = IsometricProjection::IsoToWorld(screenPos.x, screenPos.y, 
                                                     map.tilewidth, map.tileheight);
    SYSTEM_LOG << "Screen -> World (" << worldPos.x << ", " << worldPos.y << ")" << std::endl;
    
    // ========================================================================
    // Mouse Picking
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Mouse Picking ---" << std::endl;
    
    // Simulate mouse position
    float mouseX = 256.0f;
    float mouseY = 128.0f;
    
    int pickedTileX, pickedTileY;
    IsometricProjection::ScreenToTile(mouseX, mouseY, map.tilewidth, map.tileheight,
                                      pickedTileX, pickedTileY);
    
    SYSTEM_LOG << "Mouse at (" << mouseX << ", " << mouseY << ") -> Tile ("
               << pickedTileX << ", " << pickedTileY << ")" << std::endl;
    
    // ========================================================================
    // Grid Rendering Example
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Isometric Grid Positions ---" << std::endl;
    SYSTEM_LOG << "First 5x5 tiles:" << std::endl;
    
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 5; ++x) {
            Vec2 pos = IsometricProjection::TileToScreen(x, y, map.tilewidth, map.tileheight);
            SYSTEM_LOG << "  [" << x << "," << y << "] = (" << pos.x << ", " << pos.y << ")" << std::endl;
        }
    }
    
    SYSTEM_LOG << "\n=== Example Complete ===" << std::endl;
    
    return 0;
}
