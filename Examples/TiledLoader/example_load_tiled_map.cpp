/*
 * example_load_tiled_map.cpp - Example of loading and converting a Tiled map
 * 
 * This example demonstrates:
 * 1. Loading a .tmj file
 * 2. Converting to Olympe LevelDefinition
 * 3. Handling parallax layers
 * 4. Custom prefab mapping
 */

#include "TiledLevelLoader/include/TiledLevelLoader.h"
#include "TiledLevelLoader/include/TiledToOlympe.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "system/system_utils.h"

using namespace Olympe::Tiled;

int main()
{
    // Initialize logging
    Logging::InitLogger();
    
    SYSTEM_LOG << "=== Tiled Map Loader Example ===" << std::endl;
    
    // ========================================================================
    // Step 1: Load Tiled Map
    // ========================================================================
    
    TiledLevelLoader loader;
    TiledMap tiledMap;
    
    std::string mapPath = "Resources/Maps/level1.tmj";
    
    SYSTEM_LOG << "Loading map from: " << mapPath << std::endl;
    
    if (!loader.LoadFromFile(mapPath, tiledMap)) {
        SYSTEM_LOG << "ERROR: Failed to load map: " << loader.GetLastError() << std::endl;
        return 1;
    }
    
    SYSTEM_LOG << "Map loaded successfully!" << std::endl;
    SYSTEM_LOG << "  Size: " << tiledMap.width << "x" << tiledMap.height << " tiles" << std::endl;
    SYSTEM_LOG << "  Tile size: " << tiledMap.tilewidth << "x" << tiledMap.tileheight << " pixels" << std::endl;
    SYSTEM_LOG << "  Orientation: " 
               << (tiledMap.orientation == MapOrientation::Orthogonal ? "Orthogonal" :
                   tiledMap.orientation == MapOrientation::Isometric ? "Isometric" : "Other")
               << std::endl;
    SYSTEM_LOG << "  Infinite: " << (tiledMap.infinite ? "Yes" : "No") << std::endl;
    SYSTEM_LOG << "  Layers: " << tiledMap.layers.size() << std::endl;
    SYSTEM_LOG << "  Tilesets: " << tiledMap.tilesets.size() << std::endl;
    
    // ========================================================================
    // Step 2: Inspect Layers
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Layers ---" << std::endl;
    for (const auto& layer : tiledMap.layers) {
        SYSTEM_LOG << "  [" << layer->id << "] " << layer->name;
        
        switch (layer->type) {
            case LayerType::TileLayer:
                SYSTEM_LOG << " (Tile Layer, " << layer->width << "x" << layer->height << ")";
                break;
            case LayerType::ObjectGroup:
                SYSTEM_LOG << " (Object Layer, " << layer->objects.size() << " objects)";
                break;
            case LayerType::ImageLayer:
                SYSTEM_LOG << " (Image Layer, " << layer->image << ")";
                if (layer->parallaxx != 1.0f || layer->parallaxy != 1.0f) {
                    SYSTEM_LOG << " - Parallax(" << layer->parallaxx << ", " << layer->parallaxy << ")";
                }
                break;
            case LayerType::Group:
                SYSTEM_LOG << " (Group, " << layer->layers.size() << " children)";
                break;
        }
        
        SYSTEM_LOG << std::endl;
    }
    
    // ========================================================================
    // Step 3: Inspect Tilesets
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Tilesets ---" << std::endl;
    for (const auto& tileset : tiledMap.tilesets) {
        SYSTEM_LOG << "  [" << tileset.firstgid << "] " << tileset.name << std::endl;
        SYSTEM_LOG << "    Tile size: " << tileset.tilewidth << "x" << tileset.tileheight << std::endl;
        SYSTEM_LOG << "    Tile count: " << tileset.tilecount << std::endl;
        
        if (!tileset.source.empty()) {
            SYSTEM_LOG << "    Source: " << tileset.source << " (external)" << std::endl;
        } else if (!tileset.image.empty()) {
            SYSTEM_LOG << "    Image: " << tileset.image << std::endl;
        } else if (!tileset.tiles.empty()) {
            SYSTEM_LOG << "    Collection tileset with " << tileset.tiles.size() << " tiles" << std::endl;
        }
    }
    
    // ========================================================================
    // Step 4: Configure Conversion
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Converting to Olympe Format ---" << std::endl;
    
    ConversionConfig config;
    
    // Coordinate system
    config.flipY = true;  // Tiled uses top-left origin, Olympe uses bottom-left
    
    // Resource paths
    config.resourceBasePath = "Resources";
    
    // Object type to prefab mapping - Method 1: Manual configuration
    // Alternatively, you can load mappings from a JSON file using LoadPrefabMapping()
    // (see Config/tiled_prefab_mapping.json for the expected format)
    config.typeToPrefabMap["Player"] = "Blueprints/Player.json";
    config.typeToPrefabMap["Enemy"] = "Blueprints/Enemy.json";
    config.typeToPrefabMap["Coin"] = "Blueprints/Collectibles/Coin.json";
    config.typeToPrefabMap["Door"] = "Blueprints/Door.json";
    config.typeToPrefabMap["Lever"] = "Blueprints/Lever.json";
    config.typeToPrefabMap["Checkpoint"] = "Blueprints/Checkpoint.json";
    
    // Default prefab for unmapped types
    config.defaultPrefab = "Blueprints/DefaultEntity.json";
    
    // Layer patterns for collision detection
    config.collisionLayerPatterns.clear();
    config.collisionLayerPatterns.push_back("collision");
    config.collisionLayerPatterns.push_back("walls");
    config.collisionLayerPatterns.push_back("solid");
    
    // Layer patterns for sectors (trigger zones)
    config.sectorLayerPatterns.clear();
    config.sectorLayerPatterns.push_back("sector");
    config.sectorLayerPatterns.push_back("zone");
    config.sectorLayerPatterns.push_back("trigger");
    
    // ========================================================================
    // Step 5: Convert
    // ========================================================================
    
    TiledToOlympe converter;
    converter.SetConfig(config);
    
    // Method 2: Load prefab mappings from JSON file (alternative to manual config)
    // This will merge with or override the mappings set above
    // if (!converter.LoadPrefabMapping("Config/tiled_prefab_mapping.json")) {
    //     SYSTEM_LOG << "Warning: Could not load prefab mapping file" << std::endl;
    // }
    
    Olympe::Editor::LevelDefinition olympeLevel;
    
    if (!converter.Convert(tiledMap, olympeLevel)) {
        SYSTEM_LOG << "ERROR: Conversion failed: " << converter.GetLastError() << std::endl;
        return 1;
    }
    
    SYSTEM_LOG << "Conversion successful!" << std::endl;
    SYSTEM_LOG << "  World size: " << olympeLevel.worldSize.x << "x" << olympeLevel.worldSize.y << std::endl;
    SYSTEM_LOG << "  Entities: " << olympeLevel.entities.size() << std::endl;
    SYSTEM_LOG << "  Tilemap: " << olympeLevel.tileMap.size() << " rows" << std::endl;
    
    // ========================================================================
    // Step 6: Inspect Converted Entities
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Entities ---" << std::endl;
    for (const auto& entity : olympeLevel.entities) {
        SYSTEM_LOG << "  [" << entity->id << "] " << entity->name << std::endl;
        SYSTEM_LOG << "    Prefab: " << entity->prefabPath << std::endl;
        SYSTEM_LOG << "    Position: (" << entity->position.x << ", " << entity->position.y << ")" << std::endl;
        
        if (!entity->overrides.empty()) {
            SYSTEM_LOG << "    Overrides: " << entity->overrides.dump() << std::endl;
        }
    }
    
    // ========================================================================
    // Step 7: Inspect Parallax Layers
    // ========================================================================
    
    const auto& parallaxManager = converter.GetParallaxLayers();
    
    if (parallaxManager.GetLayerCount() > 0) {
        SYSTEM_LOG << "\n--- Parallax Layers ---" << std::endl;
        
        for (size_t i = 0; i < parallaxManager.GetLayerCount(); ++i) {
            const auto* layer = parallaxManager.GetLayer(i);
            if (layer) {
                SYSTEM_LOG << "  [" << i << "] " << layer->name << std::endl;
                SYSTEM_LOG << "    Image: " << layer->imagePath << std::endl;
                SYSTEM_LOG << "    Scroll factors: (" << layer->scrollFactorX << ", " << layer->scrollFactorY << ")" << std::endl;
                SYSTEM_LOG << "    Offset: (" << layer->offsetX << ", " << layer->offsetY << ")" << std::endl;
                SYSTEM_LOG << "    Repeat: " << (layer->repeatX ? "X" : "") << (layer->repeatY ? "Y" : "") << std::endl;
                SYSTEM_LOG << "    Opacity: " << layer->opacity << std::endl;
            }
        }
        
        // Example: Calculate render position for camera at (100, 50)
        float cameraX = 100.0f;
        float cameraY = 50.0f;
        
        SYSTEM_LOG << "\n  Render positions for camera at (" << cameraX << ", " << cameraY << "):" << std::endl;
        for (size_t i = 0; i < parallaxManager.GetLayerCount(); ++i) {
            const auto* layer = parallaxManager.GetLayer(i);
            if (layer) {
                float renderX, renderY;
                parallaxManager.CalculateRenderPosition(*layer, cameraX, cameraY, renderX, renderY);
                SYSTEM_LOG << "    " << layer->name << ": (" << renderX << ", " << renderY << ")" << std::endl;
            }
        }
    }
    
    // ========================================================================
    // Step 8: Save to Olympe Format (optional)
    // ========================================================================
    
    SYSTEM_LOG << "\n--- Saving Olympe Level ---" << std::endl;
    
    Olympe::Editor::LevelManager levelManager;
    levelManager.GetLevelDefinition() = olympeLevel;
    
    if (levelManager.SaveLevel("Resources/Levels/level1_converted.json")) {
        SYSTEM_LOG << "Level saved successfully!" << std::endl;
    } else {
        SYSTEM_LOG << "Failed to save level" << std::endl;
    }
    
    SYSTEM_LOG << "\n=== Example Complete ===" << std::endl;
    
    return 0;
}
