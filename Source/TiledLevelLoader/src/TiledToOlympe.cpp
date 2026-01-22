/*
 * TiledToOlympe.cpp - Converter implementation
 */
#include "../include/TiledLevelLoader.h"
#include "../include/TiledToOlympe.h"
#include "../../OlympeTilemapEditor/include/LevelManager.h"
#include "../../system/system_utils.h"
#include <algorithm>
#include <fstream>

namespace Olympe {
namespace Tiled {

    TiledToOlympe::TiledToOlympe()
    {
        mapWidth_ = 0; mapHeight_ = 0;
        // Set default configuration
        config_.flipY = true;
        config_.defaultPrefab = "Blueprints/DefaultEntity.json";
        config_.collisionLayerPatterns.push_back("collision");
        config_.collisionLayerPatterns.push_back("walls");
        config_.sectorLayerPatterns.push_back("sector");
        config_.sectorLayerPatterns.push_back("zone");
    }

    TiledToOlympe::~TiledToOlympe()
    {
    }

    // Property key constants
    namespace {
        const char* PROPERTY_PATROL_WAY = "patrol way";
        const char* PROPERTY_TARGET = "target";
        const char* PROPERTY_AUDIO = "audio";
        
        // Flip flag constants for compact storage
        const uint8_t FLIP_FLAG_HORIZONTAL = 0x1;
        const uint8_t FLIP_FLAG_VERTICAL   = 0x2;
        const uint8_t FLIP_FLAG_DIAGONAL   = 0x4;
        
        // Helper function to extract flip flags from Tiled GID
        inline uint8_t ExtractFlipFlags(uint32_t gid) {
            uint8_t flags = 0;
            if (IsFlippedHorizontally(gid)) flags |= FLIP_FLAG_HORIZONTAL;
            if (IsFlippedVertically(gid))   flags |= FLIP_FLAG_VERTICAL;
            if (IsFlippedDiagonally(gid))   flags |= FLIP_FLAG_DIAGONAL;
            return flags;
        }
    }

    void TiledToOlympe::SetConfig(const ConversionConfig& config)
    {
        config_ = config;
    }

    bool TiledToOlympe::Convert(const TiledMap& tiledMap, Olympe::Editor::LevelDefinition& outLevel)
    {
        lastError_.clear();
        parallaxLayers_.Clear();
        
        SYSTEM_LOG << "\n╔═══════════════════════════════════════════════════════════╗\n";
        SYSTEM_LOG << "║ TILED → OLYMPE CONVERSION - COMPLETE PIPELINE            ║\n";
        SYSTEM_LOG << "╚═══════════════════════════════════════════════════════════╝\n\n";
        
        // Store map dimensions
        mapWidth_ = tiledMap.width;
        mapHeight_ = tiledMap.height;
        
        // ===================================================================
        // PHASE 1: MAP CONFIGURATION & METADATA
        // ===================================================================
        SYSTEM_LOG << "[Phase 1/6] Extracting Map Configuration & Metadata...\n";
        ExtractMapConfiguration(tiledMap, outLevel);
        ExtractMapMetadata(tiledMap, outLevel);
        
        // ===================================================================
        // PHASE 2: VISUAL LAYERS (Parallax, Image Layers, Tile Layers)
        // ===================================================================
        SYSTEM_LOG << "[Phase 2/6] Processing Visual Layers...\n";
        int visualLayerCount = 0;
        ProcessVisualLayers(tiledMap, outLevel, visualLayerCount);
        SYSTEM_LOG << "  ✓ Processed " << visualLayerCount << " visual layers\n";
        
        // ===================================================================
        // PHASE 3: SPATIAL STRUCTURES (Sectors, Collision, Navigation)
        // ===================================================================
        SYSTEM_LOG << "[Phase 3/6] Extracting Spatial Structures...\n";
        int spatialObjectCount = 0;
        ExtractSpatialStructures(tiledMap, outLevel, spatialObjectCount);
        SYSTEM_LOG << "  ✓ Extracted " << spatialObjectCount << " spatial objects\n";
        
        // ===================================================================
        // PHASE 4: GAME OBJECTS (Categorized by Type)
        // ===================================================================
        SYSTEM_LOG << "[Phase 4/6] Converting Game Objects...\n";
        ConversionStats stats;
        CategorizeGameObjects(tiledMap, outLevel, stats);
        SYSTEM_LOG << "  ✓ Static: " << stats.staticObjects 
                   << " | Dynamic: " << stats.dynamicObjects
                   << " | Paths: " << stats.patrolPaths
                   << " | Sounds: " << stats.soundObjects << "\n";
        
        // ===================================================================
        // PHASE 5: OBJECT RELATIONSHIPS (Links, References)
        // ===================================================================
        SYSTEM_LOG << "[Phase 5/6] Extracting Object Relationships...\n";
        int linkCount = 0;
        ExtractObjectRelationships(tiledMap, outLevel, linkCount);
        SYSTEM_LOG << "  ✓ Created " << linkCount << " object links\n";
        
        // ===================================================================
        // PHASE 6: RESOURCE CATALOG
        // ===================================================================
        SYSTEM_LOG << "[Phase 6/6] Building Resource Catalog...\n";
        BuildResourceCatalog(tiledMap, outLevel);
        SYSTEM_LOG << "  ✓ Tilesets: " << outLevel.resources.tilesetPaths.size()
                   << " | Images: " << outLevel.resources.imagePaths.size()
                   << " | Audio: " << outLevel.resources.audioPaths.size() << "\n";
        
        // ===================================================================
        // FINAL SUMMARY
        // ===================================================================
        SYSTEM_LOG << "\n╔═══════════════════════════════════════════════════════════╗\n";
        SYSTEM_LOG << "║ CONVERSION COMPLETE                                       ║\n";
        SYSTEM_LOG << "╠═══════════════════════════════════════════════════════════╣\n";
        SYSTEM_LOG << "║ Map: " << outLevel.mapConfig.orientation 
                   << " " << outLevel.mapConfig.mapWidth << "x" << outLevel.mapConfig.mapHeight << "\n";
        SYSTEM_LOG << "║ Visual Layers: " << visualLayerCount << "\n";
        SYSTEM_LOG << "║ Entities: " << stats.totalObjects << "\n";
        SYSTEM_LOG << "║ Relationships: " << linkCount << "\n";
        SYSTEM_LOG << "╚═══════════════════════════════════════════════════════════╝\n\n";
        
        return true;
    }

    void TiledToOlympe::ConvertTileLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level)
    {
        SYSTEM_LOG << "TiledToOlympe: Converting tile layer '" << layer.name << "'" << std::endl;

        // Check if this is a collision layer
        if (MatchesPattern(layer.name, config_.collisionLayerPatterns)) {
            // Treat non-zero tiles as collision
            int index = 0;
            for (int y = 0; y < layer.height && y < mapHeight_; ++y) {
                for (int x = 0; x < layer.width && x < mapWidth_; ++x) {
                    if (index < static_cast<int>(layer.data.size())) {
                        uint32_t gid = layer.data[index];
                        uint32_t tileId = GetTileId(gid);
                        if (tileId > 0) {
                            level.collisionMap[y][x] = 0xFF; // Solid collision
                        }
                    }
                    ++index;
                }
            }
        }
        else {
            // Regular tile layer - merge into tilemap
            MergeTileLayer(layer, level.tileMap, mapWidth_, mapHeight_);
        }
    }

    void TiledToOlympe::ConvertObjectLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level)
    {
        SYSTEM_LOG << "TiledToOlympe: Converting object layer '" << layer.name 
                   << "' with " << layer.objects.size() << " objects" << std::endl;

        for (const auto& obj : layer.objects) {
            ConvertObject(obj, level);
        }
    }

    void TiledToOlympe::ConvertImageLayer(const TiledLayer& layer)
    {
        SYSTEM_LOG << "TiledToOlympe: Converting image layer '" << layer.name << "'" << std::endl;

        ParallaxLayer parallax;
        parallax.name = layer.name;
        
        // Resolve image path
        if (!layer.image.empty()) {
            if (!config_.resourceBasePath.empty()) {
                parallax.imagePath = config_.resourceBasePath + "/" + layer.image;
            } else {
                parallax.imagePath = layer.image;
            }
        }

        parallax.scrollFactorX = layer.parallaxx;
        parallax.scrollFactorY = layer.parallaxy;
        parallax.offsetX = layer.offsetx;
        parallax.offsetY = layer.offsety;
        parallax.opacity = layer.opacity;
        parallax.repeatX = layer.repeatx;
        parallax.repeatY = layer.repeaty;
        parallax.visible = layer.visible;
        parallax.tintColor = layer.tintcolor;

        parallaxLayers_.AddLayer(parallax);
    }

    void TiledToOlympe::ConvertGroupLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level)
    {
        SYSTEM_LOG << "TiledToOlympe: Converting group layer '" << layer.name << "'" << std::endl;

        // Recursively process child layers
        for (const auto& childLayer : layer.layers) {
            if (!childLayer->visible) {
                continue;
            }

            switch (childLayer->type) {
                case LayerType::TileLayer:
                    ConvertTileLayer(*childLayer, level);
                    break;
                case LayerType::ObjectGroup:
                    ConvertObjectLayer(*childLayer, level);
                    break;
                case LayerType::ImageLayer:
                    ConvertImageLayer(*childLayer);
                    break;
                case LayerType::Group:
                    ConvertGroupLayer(*childLayer, level);
                    break;
            }
        }
    }

    void TiledToOlympe::ConvertObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Check for collision polygons/polylines first
        std::string typeLower = obj.type;
        std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
        
        if ((typeLower == "collision" || typeLower.find("collision") != std::string::npos))
        {
            if (obj.objectType == ObjectType::Polygon || obj.objectType == ObjectType::Polyline)
            {
                ConvertPolygonCollision(obj, level);
                return;
            }
        }
        
        // Check for patrol paths (polyline objects)
        if (obj.objectType == ObjectType::Polyline) {
            ConvertPatrolPath(obj, level);
            return;
        }

        // Check for sector polygons
        if (obj.objectType == ObjectType::Polygon) {
            ConvertSectorObject(obj, level);
            return;
        }

        // Regular entity
        auto entity = CreateEntity(obj);
        if (entity) {
            level.entities.push_back(std::move(entity));
        }
    }

    void TiledToOlympe::ConvertCollisionObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Convert rectangle to collision tiles
        int tileSize = 32; // Default tile size
        int startX = static_cast<int>(obj.x / tileSize);
        int startY = static_cast<int>(TransformY(obj.y, obj.height) / tileSize);
        int endX = static_cast<int>((obj.x + obj.width) / tileSize);
        int endY = static_cast<int>((TransformY(obj.y, obj.height) + obj.height) / tileSize);

        for (int y = startY; y <= endY && y < mapHeight_; ++y) {
            for (int x = startX; x <= endX && x < mapWidth_; ++x) {
                if (y >= 0 && x >= 0) {
                    level.collisionMap[y][x] = 0xFF;
                }
            }
        }
    }

    void TiledToOlympe::ConvertSectorObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Create a sector entity
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        // Generate unique ID
        entity->id = "sector_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("Sector " + std::to_string(obj.id)) : obj.name;
        entity->prefabPath = "Blueprints/Sector.json";
        
        float transformedY = TransformY(obj.y, 0);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);

        // Store polygon points in overrides
        nlohmann::json polygon = nlohmann::json::array();
        for (const auto& pt : obj.polygon) {
            nlohmann::json point = nlohmann::json::object();
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            polygon.push_back(point);
        }

        entity->overrides["Sector"] = nlohmann::json::object();
        entity->overrides["Sector"]["polygon"] = polygon;
        entity->overrides["Sector"]["type"] = obj.type;

        // Convert properties
        PropertiesToOverrides(obj.properties, entity->overrides);

        level.entities.push_back(std::move(entity));
    }

    void TiledToOlympe::ConvertPolygonCollision(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Create a collision polygon entity
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entity->id = "collision_poly_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("CollisionPoly " + std::to_string(obj.id)) : obj.name;
        entity->type = "CollisionPolygon";
        entity->prefabPath = "Blueprints/CollisionPolygon.json";
        
        float transformedY = TransformY(obj.y, 0);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);
        entity->rotation = obj.rotation;
        
        // Store polygon/polyline points
        nlohmann::json polygon = nlohmann::json::array();
        const auto& points = (obj.objectType == ObjectType::Polygon) ? obj.polygon : obj.polyline;
        
        for (const auto& pt : points)
        {
            nlohmann::json point = nlohmann::json::object();
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            polygon.push_back(point);
        }
        
        entity->overrides["CollisionPolygon"] = nlohmann::json::object();
        entity->overrides["CollisionPolygon"]["points"] = polygon;
        entity->overrides["CollisionPolygon"]["isClosed"] = (obj.objectType == ObjectType::Polygon);
        
        // Store dimensions for bounding box fallback
        entity->overrides["width"] = obj.width;
        entity->overrides["height"] = obj.height;
        
        // Convert properties
        PropertiesToOverrides(obj.properties, entity->overrides);
        
        level.entities.push_back(std::move(entity));
    }

    void TiledToOlympe::ConvertPatrolPath(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Create a patrol path entity
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entity->id = "patrol_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("Patrol " + std::to_string(obj.id)) : obj.name;
        entity->prefabPath = "Blueprints/PatrolPath.json";
        
        float transformedY = TransformY(obj.y, 0);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);

        // Store polyline points in overrides
        nlohmann::json path = nlohmann::json::array();
        for (const auto& pt : obj.polyline) {
            nlohmann::json point = nlohmann::json::object();
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            path.push_back(point);
        }

        entity->overrides["AIBlackboard_data"] = nlohmann::json::object();
        entity->overrides["AIBlackboard_data"]["patrolPath"] = path;

        // Convert properties
        PropertiesToOverrides(obj.properties, entity->overrides);

        level.entities.push_back(std::move(entity));
    }

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::CreateEntity(const TiledObject& obj)
    {
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        // Generate unique ID
        entity->id = "entity_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("Object " + std::to_string(obj.id)) : obj.name;
        
        // Store entity type
        entity->type = obj.type;
        
        // Get prefab path from type mapping
        entity->prefabPath = GetPrefabPath(obj.type);
        
        // Transform position (Tiled uses top-left origin, Olympe may use different)
        float transformedY = TransformY(obj.y, obj.height);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);

        // Store rotation (extract to entity level field)
        entity->rotation = obj.rotation;

        // Convert properties to overrides
        PropertiesToOverrides(obj.properties, entity->overrides);

        // Store dimensions if present
        if (obj.width > 0 || obj.height > 0) {
            if (!entity->overrides.contains("Transform")) {
                entity->overrides["Transform"] = nlohmann::json::object();
            }
            entity->overrides["Transform"]["width"] = obj.width;
            entity->overrides["Transform"]["height"] = obj.height;
        }

        // Store rotation in Transform overrides if present
        if (obj.rotation != 0.0f) {
            if (!entity->overrides.contains("Transform")) {
                entity->overrides["Transform"] = nlohmann::json::object();
            }
            entity->overrides["Transform"]["rotation"] = obj.rotation;
        }

        return entity;
    }

    void TiledToOlympe::PropertiesToOverrides(
        const std::map<std::string, TiledProperty>& properties,
        nlohmann::json& overrides)
    {
        for (const auto& pair : properties) {
            const TiledProperty& prop = pair.second;
            
            switch (prop.type) {
                case PropertyType::String:
                case PropertyType::File:
                case PropertyType::Color:
                    overrides[prop.name] = prop.stringValue;
                    break;
                case PropertyType::Int:
                    overrides[prop.name] = prop.intValue;
                    break;
                case PropertyType::Float:
                    overrides[prop.name] = prop.floatValue;
                    break;
                case PropertyType::Bool:
                    overrides[prop.name] = prop.boolValue;
                    break;
                default:
                    overrides[prop.name] = prop.stringValue;
                    break;
            }
        }
    }

    std::string TiledToOlympe::GetPrefabPath(const std::string& objectType)
    {
        if (objectType.empty()) {
            return config_.defaultPrefab;
        }

        // Check type mapping
        auto it = config_.typeToPrefabMap.find(objectType);
        if (it != config_.typeToPrefabMap.end()) {
            return it->second;
        }

        // Default: try to construct path from type
        return "Blueprints/" + objectType + ".json";
    }

    bool TiledToOlympe::MatchesPattern(const std::string& layerName, 
                                       const std::vector<std::string>& patterns)
    {
        std::string lowerName = layerName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        for (const auto& pattern : patterns) {
            std::string lowerPattern = pattern;
            std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
            
            if (lowerName.find(lowerPattern) != std::string::npos) {
                return true;
            }
        }

        return false;
    }

    float TiledToOlympe::TransformY(float y, float height)
    {
        if (config_.flipY) {
            // Flip Y coordinate (Tiled top-left vs bottom-left origin)
            float worldHeight = mapHeight_ * 32.0f; // Assume 32px tiles
            return worldHeight - y - height;
        }
        return y;
    }

    void TiledToOlympe::InitializeCollisionMap(Olympe::Editor::LevelDefinition& level, 
                                                int width, int height)
    {
        level.collisionMap.resize(height);
        for (int y = 0; y < height; ++y) {
            level.collisionMap[y].resize(width, 0);
        }
    }

    void TiledToOlympe::MergeTileLayer(const TiledLayer& layer,
                                       std::vector<std::vector<int>>& tileMap,
                                       int mapWidth, int mapHeight)
    {
        if (layer.data.empty()) {
            return;
        }

        int index = 0;
        for (int y = 0; y < layer.height && y < mapHeight; ++y) {
            for (int x = 0; x < layer.width && x < mapWidth; ++x) {
                if (index < static_cast<int>(layer.data.size())) {
                    uint32_t gid = layer.data[index];
                    uint32_t tileId = GetTileId(gid);
                    
                    // Only overwrite if tile is not empty (0)
                    if (tileId > 0) {
                        tileMap[y][x] = static_cast<int>(tileId);
                    }
                }
                ++index;
            }
        }
    }

    // ===================================================================
    // NEW 6-PHASE PIPELINE IMPLEMENTATION
    // ===================================================================

    void TiledToOlympe::ExtractMapConfiguration(const TiledMap& tiledMap, 
                                                Olympe::Editor::LevelDefinition& outLevel)
    {
        outLevel.mapConfig.mapWidth = tiledMap.width;
        outLevel.mapConfig.mapHeight = tiledMap.height;
        outLevel.mapConfig.tileWidth = tiledMap.tilewidth;
        outLevel.mapConfig.tileHeight = tiledMap.tileheight;
        outLevel.mapConfig.infinite = tiledMap.infinite;
        
        // Convert orientation enum to string
        switch (tiledMap.orientation) {
            case MapOrientation::Orthogonal: outLevel.mapConfig.orientation = "orthogonal"; break;
            case MapOrientation::Isometric: outLevel.mapConfig.orientation = "isometric"; break;
            case MapOrientation::Staggered: outLevel.mapConfig.orientation = "staggered"; break;
            case MapOrientation::Hexagonal: outLevel.mapConfig.orientation = "hexagonal"; break;
            default: outLevel.mapConfig.orientation = "unknown"; break;
        }
        
        // Convert render order
        switch (tiledMap.renderorder) {
            case RenderOrder::RightDown: outLevel.mapConfig.renderOrder = "right-down"; break;
            case RenderOrder::RightUp: outLevel.mapConfig.renderOrder = "right-up"; break;
            case RenderOrder::LeftDown: outLevel.mapConfig.renderOrder = "left-down"; break;
            case RenderOrder::LeftUp: outLevel.mapConfig.renderOrder = "left-up"; break;
        }
        
        // Set world size
        outLevel.worldSize.x = tiledMap.width * tiledMap.tilewidth;
        outLevel.worldSize.y = tiledMap.height * tiledMap.tileheight;
        
        // Background color
        if (!tiledMap.backgroundcolor.empty()) {
            outLevel.ambientColor = tiledMap.backgroundcolor;
        }
        
        SYSTEM_LOG << "  → Map: " << outLevel.mapConfig.orientation 
                   << " " << outLevel.mapConfig.mapWidth << "x" << outLevel.mapConfig.mapHeight
                   << " (tiles: " << outLevel.mapConfig.tileWidth << "x" << outLevel.mapConfig.tileHeight << ")\n";
    }

    void TiledToOlympe::ExtractMapMetadata(const TiledMap& tiledMap, 
                                          Olympe::Editor::LevelDefinition& outLevel)
    {
        // Convert map custom properties to metadata
        for (const auto& prop : tiledMap.properties) {
            outLevel.metadata.customData[prop.first] = PropertyToJSON(prop.second);
        }
    }

    void TiledToOlympe::ProcessVisualLayers(const TiledMap& tiledMap, 
                                            Olympe::Editor::LevelDefinition& outLevel,
                                            int& layerCount)
    {
        layerCount = 0;
        int zOrder = 0;
        
        // Initialize tile map
        outLevel.tileMap.resize(mapHeight_);
        for (int y = 0; y < mapHeight_; ++y) {
            outLevel.tileMap[y].resize(mapWidth_, 0);
        }
        
        for (const auto& layer : tiledMap.layers) {
            if (!layer->visible) continue;
            
            switch (layer->type) {
                case LayerType::ImageLayer: {
                    // Parallax/Background layers
                    Olympe::Editor::LevelDefinition::VisualLayer visual;
                    visual.name = layer->name;
                    visual.zOrder = zOrder++;
                    visual.isParallax = (layer->parallaxx != 1.0f || layer->parallaxy != 1.0f);
                    visual.imagePath = ResolveImagePath(layer->image);
                    visual.scrollFactorX = layer->parallaxx;
                    visual.scrollFactorY = layer->parallaxy;
                    visual.offsetX = layer->offsetx;
                    visual.offsetY = layer->offsety;
                    visual.repeatX = layer->repeatx;
                    visual.repeatY = layer->repeaty;
                    visual.opacity = layer->opacity;
                    visual.tintColor = layer->tintcolor;
                    visual.visible = layer->visible;
                    
                    outLevel.visualLayers.push_back(visual);
                    layerCount++;
                    
                    // Also add to parallax layer manager for backward compatibility
                    ConvertImageLayer(*layer);
                    
                    SYSTEM_LOG << "  → Image Layer: '" << visual.name << "' (parallax: " 
                               << visual.scrollFactorX << ", z: " << visual.zOrder << ")\n";
                    break;
                }
                
                case LayerType::TileLayer: {
                    // Skip collision layers (handled in Phase 3)
                    if (MatchesPattern(layer->name, config_.collisionLayerPatterns)) {
                        break;
                    }
                    
                    // Visual tile layer
                    Olympe::Editor::LevelDefinition::TileLayerDef tileDef;
                    tileDef.name = layer->name;
                    tileDef.zOrder = zOrder++;
                    tileDef.opacity = layer->opacity;
                    tileDef.visible = layer->visible;
                    tileDef.isInfinite = !layer->chunks.empty();
                    
                    // Handle infinite maps with chunks
                    if (tileDef.isInfinite) {
                        for (const auto& chunk : layer->chunks) {
                            Olympe::Editor::LevelDefinition::TileLayerDef::Chunk chunkDef;
                            chunkDef.x = chunk.x;
                            chunkDef.y = chunk.y;
                            chunkDef.width = chunk.width;
                            chunkDef.height = chunk.height;
                            
                            // Extract chunk tile data and flip flags
                            chunkDef.tiles.resize(chunk.height);
                            chunkDef.tileFlipFlags.resize(chunk.height);
                            int index = 0;
                            for (int y = 0; y < chunk.height; ++y) {
                                chunkDef.tiles[y].resize(chunk.width, 0);
                                chunkDef.tileFlipFlags[y].resize(chunk.width, 0);
                                for (int x = 0; x < chunk.width; ++x) {
                                    if (index < static_cast<int>(chunk.data.size())) {
                                        uint32_t gid = chunk.data[index];
                                        chunkDef.tiles[y][x] = GetTileId(gid);
                                        chunkDef.tileFlipFlags[y][x] = ExtractFlipFlags(gid);
                                    }
                                    ++index;
                                }
                            }
                            
                            tileDef.chunks.push_back(chunkDef);
                        }
                        
                        SYSTEM_LOG << "  → Tile Layer (Infinite): '" << tileDef.name << "' (" 
                                   << tileDef.chunks.size() << " chunks, z: " << tileDef.zOrder << ")\n";
                    }
                    // Handle finite maps with regular data
                    else {
                        // Extract tile data and flip flags
                        tileDef.tiles.resize(layer->height);
                        tileDef.tileFlipFlags.resize(layer->height);
                        int index = 0;
                        for (int y = 0; y < layer->height; ++y) {
                            tileDef.tiles[y].resize(layer->width, 0);
                            tileDef.tileFlipFlags[y].resize(layer->width, 0);
                            for (int x = 0; x < layer->width; ++x) {
                                if (index < static_cast<int>(layer->data.size())) {
                                    uint32_t gid = layer->data[index];
                                    tileDef.tiles[y][x] = GetTileId(gid);
                                    tileDef.tileFlipFlags[y][x] = ExtractFlipFlags(gid);
                                }
                                ++index;
                            }
                        }
                        
                        SYSTEM_LOG << "  → Tile Layer: '" << tileDef.name << "' (" 
                                   << layer->width << "x" << layer->height << " tiles, z: " << tileDef.zOrder << ")\n";
                    }
                    
                    outLevel.tileLayers.push_back(tileDef);
                    layerCount++;
                    
                    // Also merge into legacy tileMap for backward compatibility
                    MergeTileLayer(*layer, outLevel.tileMap, mapWidth_, mapHeight_);
                    
                    break;
                }
                
                case LayerType::Group: {
                    // Recursively process group layers
                    ProcessGroupLayers(*layer, outLevel, zOrder, layerCount);
                    break;
                }
                
                default:
                    break;
            }
        }
        
        // Store parallax layers in metadata for backward compatibility
        if (parallaxLayers_.GetLayerCount() > 0)
        {
            nlohmann::json parallaxLayersJson = nlohmann::json::array();
            
            for (size_t i = 0; i < parallaxLayers_.GetLayerCount(); ++i)
            {
                const ParallaxLayer* layer = parallaxLayers_.GetLayer(i);
                if (layer)
                {
                    nlohmann::json layerJson = nlohmann::json::object();
                    layerJson["name"] = layer->name;
                    layerJson["imagePath"] = layer->imagePath;
                    layerJson["scrollFactorX"] = layer->scrollFactorX;
                    layerJson["scrollFactorY"] = layer->scrollFactorY;
                    layerJson["repeatX"] = layer->repeatX;
                    layerJson["repeatY"] = layer->repeatY;
                    layerJson["offsetX"] = layer->offsetX;
                    layerJson["offsetY"] = layer->offsetY;
                    layerJson["opacity"] = layer->opacity;
                    layerJson["zOrder"] = static_cast<int>(i);
                    layerJson["visible"] = layer->visible;
                    layerJson["tintColor"] = layer->tintColor;
                    parallaxLayersJson.push_back(layerJson);
                }
            }
            
            outLevel.metadata.customData["parallaxLayers"] = parallaxLayersJson;
        }
    }

    void TiledToOlympe::ExtractSpatialStructures(const TiledMap& tiledMap,
                                                 Olympe::Editor::LevelDefinition& outLevel,
                                                 int& objectCount)
    {
        objectCount = 0;
        int collisionTileCount = 0;
        
        // Initialize collision map
        InitializeCollisionMap(outLevel, mapWidth_, mapHeight_);
        
        for (const auto& layer : tiledMap.layers) {
            if (!layer->visible) continue;
            
            // Process collision tile layers
            if (layer->type == LayerType::TileLayer && 
                MatchesPattern(layer->name, config_.collisionLayerPatterns)) {
                
                int index = 0;
                for (int y = 0; y < layer->height && y < mapHeight_; ++y) {
                    for (int x = 0; x < layer->width && x < mapWidth_; ++x) {
                        if (index < static_cast<int>(layer->data.size())) {
                            uint32_t tileId = GetTileId(layer->data[index]);
                            if (tileId > 0) {
                                outLevel.collisionMap[y][x] = 0xFF;
                                collisionTileCount++;
                            }
                        }
                        ++index;
                    }
                }
                
                SYSTEM_LOG << "  → Collision Layer: '" << layer->name << "' (filled tiles: " << collisionTileCount << ")\n";
            }
            
            // Process object layers (sectors, collision shapes)
            if (layer->type == LayerType::ObjectGroup) {
                for (const auto& obj : layer->objects) {
                    // Sector objects (polygons)
                    if (obj.objectType == ObjectType::Polygon) {
                        Olympe::Editor::LevelDefinition::SectorDef sector;
                        sector.name = obj.name.empty() ? ("Sector_" + std::to_string(obj.id)) : obj.name;
                        sector.type = obj.type;
                        sector.position = Olympe::Editor::Vec2(obj.x, TransformY(obj.y, 0));
                        
                        for (const auto& pt : obj.polygon) {
                            sector.polygon.push_back(Olympe::Editor::Vec2(
                                pt.x, config_.flipY ? -pt.y : pt.y
                            ));
                        }
                        
                        // Store properties
                        for (const auto& prop : obj.properties) {
                            sector.properties[prop.first] = PropertyToJSON(prop.second);
                        }
                        
                        outLevel.sectors.push_back(sector);
                        objectCount++;
                        
                        SYSTEM_LOG << "  → Sector: '" << sector.name << "' (" << sector.polygon.size() << " points)\n";
                    }
                    
                    // Collision shapes (rectangles)
                    else if (obj.type == "collision" && obj.objectType == ObjectType::Rectangle) {
                        Olympe::Editor::LevelDefinition::CollisionShape shape;
                        shape.name = obj.name;
                        shape.type = Olympe::Editor::LevelDefinition::CollisionShape::Rectangle;
                        shape.position = Olympe::Editor::Vec2(obj.x, TransformY(obj.y, obj.height));
                        shape.size = Olympe::Editor::Vec2(obj.width, obj.height);
                        
                        outLevel.collisionShapes.push_back(shape);
                        objectCount++;
                        
                        SYSTEM_LOG << "  → Collision Shape: '" << shape.name << "' (rect: " 
                                   << shape.size.x << "x" << shape.size.y << ")\n";
                    }
                }
            }
        }
    }

    void TiledToOlympe::CategorizeGameObjects(const TiledMap& tiledMap,
                                              Olympe::Editor::LevelDefinition& outLevel,
                                              ConversionStats& stats)
    {
        // Define category rules (synchronized with World.cpp InstantiatePass3_StaticObjects)
        const std::set<std::string> staticTypes = {
            "item", "collectible", "key", "treasure", "waypoint", "trigger", "portal", "door", "exit",
            "pickup", "interactable", "checkpoint", "teleporter", "switch", "spawn"
        };
        
        const std::set<std::string> dynamicTypes = {
            "player", "npc", "guard", "enemy", "zombie"
        };
        
        const std::set<std::string> soundTypes = {
            "ambient", "sound", "music"
        };
        
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup || !layer->visible) continue;
            
            for (const auto& obj : layer->objects) {
                // Skip spatial structures (already processed)
                if (obj.objectType == ObjectType::Polygon || 
                    obj.objectType == ObjectType::Polyline ||
                    obj.type == "collision" || obj.type == "sector") {
                    continue;
                }
                
                auto entity = CreateEntity(obj);
                if (!entity) continue;
                
                // Create a copy for the legacy entities array
                auto entityCopy = std::make_unique<Olympe::Editor::EntityInstance>();
                entityCopy->id = entity->id;
                entityCopy->prefabPath = entity->prefabPath;
                entityCopy->name = entity->name;
                entityCopy->type = entity->type;
                entityCopy->spritePath = entity->spritePath;
                entityCopy->position = entity->position;
                entityCopy->overrides = entity->overrides;
                
                // Categorize by type
                if (obj.objectType == ObjectType::Polyline && obj.type == "way") {
                    outLevel.categorizedObjects.patrolPaths.push_back(std::move(entity));
                    stats.patrolPaths++;
                    SYSTEM_LOG << "  → Patrol Path: '" << obj.name << "' (" << obj.polyline.size() << " points)\n";
                }
                else if (soundTypes.count(obj.type)) {
                    outLevel.categorizedObjects.soundObjects.push_back(std::move(entity));
                    stats.soundObjects++;
                    SYSTEM_LOG << "  → Sound Object: '" << obj.name << "' (type: " << obj.type << ")\n";
                }
                else if (staticTypes.count(obj.type)) {
                    outLevel.categorizedObjects.staticObjects.push_back(std::move(entity));
                    stats.staticObjects++;
                }
                else if (dynamicTypes.count(obj.type)) {
                    outLevel.categorizedObjects.dynamicObjects.push_back(std::move(entity));
                    stats.dynamicObjects++;
                }
                else {
                    // Default: static object
                    outLevel.categorizedObjects.staticObjects.push_back(std::move(entity));
                    stats.staticObjects++;
                }
                
                // Add copy to legacy entities array for backward compatibility
                outLevel.entities.push_back(std::move(entityCopy));
                
                stats.totalObjects++;
            }
        }
        
        // Ensure totalObjects is accurate
        stats.totalObjects = stats.staticObjects + stats.dynamicObjects + stats.patrolPaths + stats.soundObjects;
    }

    void TiledToOlympe::ExtractObjectRelationships(const TiledMap& tiledMap,
                                                   Olympe::Editor::LevelDefinition& outLevel,
                                                   int& linkCount)
    {
        linkCount = 0;
        
        // Build object ID → name mapping
        std::map<int, std::string> idToName;
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup) continue;
            for (const auto& obj : layer->objects) {
                idToName[obj.id] = obj.name;
            }
        }
        
        // Extract relationships from custom properties
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup) continue;
            
            for (const auto& obj : layer->objects) {
                // Check for "patrol way" property (NPC → patrol path link)
                auto patrolProp = obj.properties.find(PROPERTY_PATROL_WAY);
                if (patrolProp != obj.properties.end() && patrolProp->second.type == PropertyType::Object) {
                    Olympe::Editor::LevelDefinition::ObjectLink link;
                    link.sourceObjectName = obj.name;
                    link.sourceObjectId = obj.id;
                    link.targetObjectId = patrolProp->second.intValue;
                    link.targetObjectName = idToName[link.targetObjectId];
                    link.linkType = "patrol_path";
                    
                    outLevel.objectLinks.push_back(link);
                    linkCount++;
                    
                    SYSTEM_LOG << "  → Link: '" << link.sourceObjectName << "' → '" 
                               << link.targetObjectName << "' (patrol_path)\n";
                }
                
                // Check for trigger targets
                auto targetProp = obj.properties.find(PROPERTY_TARGET);
                if (targetProp != obj.properties.end() && targetProp->second.type == PropertyType::Object) {
                    Olympe::Editor::LevelDefinition::ObjectLink link;
                    link.sourceObjectName = obj.name;
                    link.sourceObjectId = obj.id;
                    link.targetObjectId = targetProp->second.intValue;
                    link.targetObjectName = idToName[link.targetObjectId];
                    link.linkType = "trigger_target";
                    
                    outLevel.objectLinks.push_back(link);
                    linkCount++;
                    
                    SYSTEM_LOG << "  → Link: '" << link.sourceObjectName << "' → '" 
                               << link.targetObjectName << "' (trigger_target)\n";
                }
            }
        }
    }

    void TiledToOlympe::BuildResourceCatalog(const TiledMap& tiledMap,
                                             Olympe::Editor::LevelDefinition& outLevel)
    {
        // Extract tileset paths
        for (const auto& tileset : tiledMap.tilesets) {
            if (!tileset.source.empty()) {
                outLevel.resources.tilesetPaths.push_back(tileset.source);
            }
            else if (!tileset.image.empty()) {
                outLevel.resources.imagePaths.push_back(tileset.image);
            }
        }
        
        // Extract image layer paths
        for (const auto& layer : tiledMap.layers) {
            if (layer->type == LayerType::ImageLayer && !layer->image.empty()) {
                std::string resolvedPath = ResolveImagePath(layer->image);
                outLevel.resources.imagePaths.push_back(resolvedPath);
            }
        }
        
        // Extract object template paths (from custom properties)
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup) continue;
            for (const auto& obj : layer->objects) {
                // Check for audio properties
                auto audioProp = obj.properties.find(PROPERTY_AUDIO);
                if (audioProp != obj.properties.end() && audioProp->second.type == PropertyType::File) {
                    outLevel.resources.audioPaths.push_back(audioProp->second.stringValue);
                }
            }
        }
        
        // Remove duplicates
        auto removeDuplicates = [](std::vector<std::string>& vec) {
            std::sort(vec.begin(), vec.end());
            vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
        };
        
        removeDuplicates(outLevel.resources.tilesetPaths);
        removeDuplicates(outLevel.resources.imagePaths);
        removeDuplicates(outLevel.resources.audioPaths);
    }

    std::string TiledToOlympe::ResolveImagePath(const std::string& imagePath)
    {
        if (imagePath.empty()) return "";
        
        if (!config_.resourceBasePath.empty()) {
            return config_.resourceBasePath + "/" + imagePath;
        }
        return imagePath;
    }

    nlohmann::json TiledToOlympe::PropertyToJSON(const TiledProperty& prop)
    {
        switch (prop.type) {
            case PropertyType::String:
            case PropertyType::File:
            case PropertyType::Color:
                return prop.stringValue;
            case PropertyType::Int:
                return prop.intValue;
            case PropertyType::Float:
                return prop.floatValue;
            case PropertyType::Bool:
                return prop.boolValue;
            case PropertyType::Object:
                return prop.intValue;  // Object reference (ID)
            default:
                return nullptr;
        }
    }

    void TiledToOlympe::ProcessGroupLayers(const TiledLayer& groupLayer,
                                           Olympe::Editor::LevelDefinition& outLevel,
                                           int& zOrder,
                                           int& layerCount)
    {
        for (const auto& childLayer : groupLayer.layers) {
            if (!childLayer->visible) continue;
            
            if (childLayer->type == LayerType::ImageLayer) {
                // Process as visual layer
                Olympe::Editor::LevelDefinition::VisualLayer visual;
                visual.name = childLayer->name;
                visual.zOrder = zOrder++;
                visual.isParallax = (childLayer->parallaxx != 1.0f || childLayer->parallaxy != 1.0f);
                visual.imagePath = ResolveImagePath(childLayer->image);
                visual.scrollFactorX = childLayer->parallaxx;
                visual.scrollFactorY = childLayer->parallaxy;
                visual.offsetX = childLayer->offsetx;
                visual.offsetY = childLayer->offsety;
                visual.repeatX = childLayer->repeatx;
                visual.repeatY = childLayer->repeaty;
                visual.opacity = childLayer->opacity;
                visual.tintColor = childLayer->tintcolor;
                visual.visible = childLayer->visible;
                
                outLevel.visualLayers.push_back(visual);
                layerCount++;
                
                // Also add to parallax layer manager for backward compatibility
                ConvertImageLayer(*childLayer);
            }
            else if (childLayer->type == LayerType::TileLayer) {
                // Skip collision layers
                if (MatchesPattern(childLayer->name, config_.collisionLayerPatterns)) {
                    continue;
                }
                
                // Process as tile layer
                Olympe::Editor::LevelDefinition::TileLayerDef tileDef;
                tileDef.name = childLayer->name;
                tileDef.zOrder = zOrder++;
                tileDef.opacity = childLayer->opacity;
                tileDef.visible = childLayer->visible;
                tileDef.isInfinite = !childLayer->chunks.empty();
                
                // Handle infinite maps with chunks
                if (tileDef.isInfinite) {
                    for (const auto& chunk : childLayer->chunks) {
                        Olympe::Editor::LevelDefinition::TileLayerDef::Chunk chunkDef;
                        chunkDef.x = chunk.x;
                        chunkDef.y = chunk.y;
                        chunkDef.width = chunk.width;
                        chunkDef.height = chunk.height;
                        
                        // Extract chunk tile data and flip flags
                        chunkDef.tiles.resize(chunk.height);
                        chunkDef.tileFlipFlags.resize(chunk.height);
                        int index = 0;
                        for (int y = 0; y < chunk.height; ++y) {
                            chunkDef.tiles[y].resize(chunk.width, 0);
                            chunkDef.tileFlipFlags[y].resize(chunk.width, 0);
                            for (int x = 0; x < chunk.width; ++x) {
                                if (index < static_cast<int>(chunk.data.size())) {
                                    uint32_t gid = chunk.data[index];
                                    chunkDef.tiles[y][x] = GetTileId(gid);
                                    chunkDef.tileFlipFlags[y][x] = ExtractFlipFlags(gid);
                                }
                                ++index;
                            }
                        }
                        
                        tileDef.chunks.push_back(chunkDef);
                    }
                }
                // Handle finite maps with regular data
                else {
                    // Extract tile data and flip flags
                    tileDef.tiles.resize(childLayer->height);
                    tileDef.tileFlipFlags.resize(childLayer->height);
                    int index = 0;
                    for (int y = 0; y < childLayer->height; ++y) {
                        tileDef.tiles[y].resize(childLayer->width, 0);
                        tileDef.tileFlipFlags[y].resize(childLayer->width, 0);
                        for (int x = 0; x < childLayer->width; ++x) {
                            if (index < static_cast<int>(childLayer->data.size())) {
                                uint32_t gid = childLayer->data[index];
                                tileDef.tiles[y][x] = GetTileId(gid);
                                tileDef.tileFlipFlags[y][x] = ExtractFlipFlags(gid);
                            }
                            ++index;
                        }
                    }
                }
                
                outLevel.tileLayers.push_back(tileDef);
                layerCount++;
                
                // Also merge into legacy tileMap
                MergeTileLayer(*childLayer, outLevel.tileMap, mapWidth_, mapHeight_);
            }
            else if (childLayer->type == LayerType::Group) {
                ProcessGroupLayers(*childLayer, outLevel, zOrder, layerCount);
            }
        }
    }

    bool TiledToOlympe::LoadPrefabMapping(const std::string& jsonFilePath)
    {
        SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - Loading from: " << jsonFilePath << std::endl;

        // Read file
        std::ifstream file(jsonFilePath);
        if (!file.is_open())
        {
            lastError_ = "Failed to open prefab mapping file: " + jsonFilePath;
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - " << lastError_ << std::endl;
            return false;
        }

        // Parse JSON
        nlohmann::json j;
        try
        {
            file >> j;
        }
        catch (const std::exception& e)
        {
            lastError_ = std::string("JSON parse error: ") + e.what();
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - " << lastError_ << std::endl;
            return false;
        }

        // Validate schema
        if (!j.contains("schema_version"))
        {
            lastError_ = "Missing 'schema_version' in prefab mapping file";
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - " << lastError_ << std::endl;
            return false;
        }

        int schemaVersion = j["schema_version"].get<int>();
        if (schemaVersion != 1)
        {
            lastError_ = "Unsupported schema version: " + std::to_string(schemaVersion);
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - " << lastError_ << std::endl;
            return false;
        }

        // Load mappings
        if (!j.contains("mapping") || !j["mapping"].is_object())
        {
            lastError_ = "Missing or invalid 'mapping' object in prefab mapping file";
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - " << lastError_ << std::endl;
            return false;
        }

        const auto& mapping = j["mapping"];
        config_.typeToPrefabMap.clear();

        for (auto it = mapping.begin(); it != mapping.end(); ++it)
        {
            std::string objectType = it.key();
            std::string prefabPath = it.value().get<std::string>();
            
            config_.typeToPrefabMap[objectType] = prefabPath;
            
            SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - Mapped '" << objectType 
                       << "' -> '" << prefabPath << "'" << std::endl;
        }

        SYSTEM_LOG << "TiledToOlympe::LoadPrefabMapping - Loaded " 
                   << config_.typeToPrefabMap.size() << " prefab mappings" << std::endl;

        return true;
    }

    uint32_t TiledToOlympe::ParseTintColor(const std::string& colorStr)
    {
        if (colorStr.empty() || colorStr == "none") return 0xFFFFFFFF;
        
        std::string hex = colorStr;
        if (hex[0] == '#') hex = hex.substr(1);
        
        uint32_t color = 0xFFFFFFFF;
        try {
            color = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
            
            // If 6-digit hex (RGB), add full alpha
            if (hex.length() == 6) {
                color = 0xFF000000 | color;
            }
        } catch (...) {
            // Invalid color format, return white
        }
        
        return color;
    }

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::CreateSectorEntity(const TiledObject& obj)
    {
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entity->id = "sector_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("Sector_" + std::to_string(obj.id)) : obj.name;
        entity->type = "Sector";
        entity->prefabPath = "Blueprints/Sector.json";
        
        float transformedY = TransformY(obj.y, 0);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);
        entity->rotation = obj.rotation;
        
        // Store polygon in overrides
        nlohmann::json polygon = nlohmann::json::array();
        for (const auto& pt : obj.polygon) {
            nlohmann::json point;
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            polygon.push_back(point);
        }
        
        entity->overrides["Sector"] = nlohmann::json::object();
        entity->overrides["Sector"]["polygon"] = polygon;
        entity->overrides["Sector"]["type"] = obj.type;
        
        PropertiesToOverrides(obj.properties, entity->overrides);
        
        return entity;
    }

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::CreatePatrolPathEntity(const TiledObject& obj)
    {
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entity->id = "patrol_" + std::to_string(obj.id);
        entity->name = obj.name.empty() ? ("PatrolPath_" + std::to_string(obj.id)) : obj.name;
        entity->type = "PatrolPath";
        entity->prefabPath = "Blueprints/PatrolPath.json";
        
        float transformedY = TransformY(obj.y, 0);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);
        entity->rotation = obj.rotation;
        
        // Store polyline in overrides
        nlohmann::json path = nlohmann::json::array();
        for (const auto& pt : obj.polyline) {
            nlohmann::json point;
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            path.push_back(point);
        }
        
        entity->overrides["AIBlackboard_data"] = nlohmann::json::object();
        entity->overrides["AIBlackboard_data"]["patrolPath"] = path;
        
        PropertiesToOverrides(obj.properties, entity->overrides);
        
        return entity;
    }

} // namespace Tiled
} // namespace Olympe
