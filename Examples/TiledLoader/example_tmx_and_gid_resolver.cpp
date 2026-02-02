/*
 * Example: Using TMX Support and GID Resolver
 * 
 * This example demonstrates:
 * 1. Loading TMX (XML) format maps
 * 2. Using the GID resolver to find tile information
 * 3. Validating image resources for preloading
 */

#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/TiledStructures.h"
#include <iostream>

using namespace Olympe::Tiled;

void PrintTileInfo(const TiledMap& map, uint32_t gid)
{
    // Use the comprehensive GID resolver
    ResolvedGid resolved = ResolveGid(map, gid);
    
    if (resolved.IsValid()) {
        std::cout << "  GID " << gid << " resolved to:\n";
        std::cout << "    Tileset: " << resolved.tileset->name << "\n";
        std::cout << "    Local ID: " << resolved.localId << "\n";
        std::cout << "    Atlas coords: (" << resolved.tileX << ", " << resolved.tileY << ")\n";
        std::cout << "    Flipped: H=" << resolved.flipH 
                  << " V=" << resolved.flipV 
                  << " D=" << resolved.flipD << "\n";
        std::cout << "    Tile offset: (" << resolved.tileset->tileoffsetX 
                  << ", " << resolved.tileset->tileoffsetY << ")\n";
    } else {
        std::cout << "  GID " << gid << " is invalid or empty\n";
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <map.tmx or map.tmj>\n";
        return 1;
    }
    
    std::string mapPath = argv[1];
    
    // ========================================================================
    // Step 1: Load map (auto-detects TMX or TMJ format)
    // ========================================================================
    std::cout << "Loading map: " << mapPath << "\n\n";
    
    TiledLevelLoader loader;
    TiledMap map;
    
    if (!loader.LoadFromFile(mapPath, map)) {
        std::cerr << "Failed to load map: " << loader.GetLastError() << "\n";
        return 1;
    }
    
    std::cout << "Map loaded successfully!\n";
    std::cout << "  Dimensions: " << map.width << "x" << map.height << " tiles\n";
    std::cout << "  Tile size: " << map.tilewidth << "x" << map.tileheight << " pixels\n";
    std::cout << "  Orientation: ";
    switch (map.orientation) {
        case MapOrientation::Orthogonal: std::cout << "Orthogonal\n"; break;
        case MapOrientation::Isometric: std::cout << "Isometric\n"; break;
        default: std::cout << "Other\n"; break;
    }
    std::cout << "\n";
    
    // ========================================================================
    // Step 2: Display tileset information (with lastgid)
    // ========================================================================
    std::cout << "Tilesets (" << map.tilesets.size() << "):\n";
    for (const auto& tileset : map.tilesets) {
        std::cout << "  - " << tileset.name << "\n";
        std::cout << "    GID range: " << tileset.firstgid 
                  << " to " << tileset.lastgid 
                  << " (" << tileset.tilecount << " tiles)\n";
        std::cout << "    Image: " << tileset.image << "\n";
        std::cout << "    Tile size: " << tileset.tilewidth << "x" << tileset.tileheight << "\n";
        if (tileset.tileoffsetX != 0 || tileset.tileoffsetY != 0) {
            std::cout << "    Tile offset: (" << tileset.tileoffsetX 
                      << ", " << tileset.tileoffsetY << ")\n";
        }
    }
    std::cout << "\n";
    
    // ========================================================================
    // Step 3: Demonstrate GID resolution
    // ========================================================================
    std::cout << "Analyzing tile layers:\n";
    for (const auto& layer : map.layers) {
        if (layer->type != LayerType::TileLayer) continue;
        
        std::cout << "\nLayer: " << layer->name << "\n";
        
        // Find first non-empty tile
        for (size_t i = 0; i < layer->data.size() && i < 5; ++i) {
            uint32_t gid = layer->data[i];
            if (gid == 0) continue; // Skip empty tiles
            
            PrintTileInfo(map, gid);
            break;
        }
    }
    std::cout << "\n";
    
    // ========================================================================
    // Step 4: Enumerate all required images
    // ========================================================================
    std::vector<std::string> images = GetAllImagePaths(map);
    
    std::cout << "Required images (" << images.size() << "):\n";
    for (const auto& imagePath : images) {
        std::cout << "  - " << imagePath << "\n";
    }
    std::cout << "\nNote: Runtime must ensure these images are loaded before rendering.\n\n";
    
    // ========================================================================
    // Step 5: Demonstrate manual GID lookup
    // ========================================================================
    std::cout << "Testing manual GID lookup:\n";
    
    if (!map.tilesets.empty()) {
        // Test with first GID of first tileset
        uint32_t testGid = map.tilesets[0].firstgid;
        
        std::cout << "Looking up GID " << testGid << ":\n";
        
        // Method 1: Using map helper
        const TiledTileset* tileset = map.FindTilesetForGid(testGid);
        if (tileset) {
            std::cout << "  Found in tileset: " << tileset->name << "\n";
            
            int localId = tileset->GetLocalId(testGid);
            std::cout << "  Local ID: " << localId << "\n";
            
            int tileX, tileY;
            tileset->GetTileCoords(testGid, tileX, tileY);
            std::cout << "  Atlas coords: (" << tileX << ", " << tileY << ")\n";
        }
        
        // Method 2: Using comprehensive resolver
        ResolvedGid resolved = ResolveGid(map, testGid);
        if (resolved.IsValid()) {
            std::cout << "  Resolver confirms: " << resolved.tileset->name << "\n";
        }
    }
    
    std::cout << "\nExample completed successfully!\n";
    return 0;
}
