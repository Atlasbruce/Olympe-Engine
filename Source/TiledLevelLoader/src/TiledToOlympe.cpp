/*
 * TiledToOlympe.cpp - Converter implementation
 */

#include "TiledToOlympe.h"
#include "OlympeTilemapEditor/include/LevelManager.h"
#include "system/system_utils.h"
#include <sstream>
#include <algorithm>

namespace Olympe {
namespace Tiled {

    TiledToOlympe::TiledToOlympe()
        : mapWidth_(0), mapHeight_(0)
    {
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

    void TiledToOlympe::SetConfig(const ConversionConfig& config)
    {
        config_ = config;
    }

    bool TiledToOlympe::Convert(const TiledMap& tiledMap, Olympe::Editor::LevelDefinition& outLevel)
    {
        lastError_.clear();
        parallaxLayers_.Clear();

        SYSTEM_LOG << "TiledToOlympe: Converting map '" << tiledMap.type << "'" << std::endl;

        // Set map dimensions
        mapWidth_ = tiledMap.width;
        mapHeight_ = tiledMap.height;

        // Initialize level
        outLevel.worldSize.x = tiledMap.width * tiledMap.tilewidth;
        outLevel.worldSize.y = tiledMap.height * tiledMap.tileheight;
        
        if (!tiledMap.backgroundcolor.empty()) {
            outLevel.ambientColor = tiledMap.backgroundcolor;
        }

        // Initialize tile and collision maps
        InitializeCollisionMap(outLevel, mapWidth_, mapHeight_);
        outLevel.tileMap.resize(mapHeight_);
        for (int y = 0; y < mapHeight_; ++y) {
            outLevel.tileMap[y].resize(mapWidth_, 0);
        }

        // Process layers
        for (const auto& layer : tiledMap.layers) {
            if (!layer->visible) {
                continue; // Skip invisible layers
            }

            switch (layer->type) {
                case LayerType::TileLayer:
                    ConvertTileLayer(*layer, outLevel);
                    break;
                case LayerType::ObjectGroup:
                    ConvertObjectLayer(*layer, outLevel);
                    break;
                case LayerType::ImageLayer:
                    ConvertImageLayer(*layer);
                    break;
                case LayerType::Group:
                    ConvertGroupLayer(*layer, outLevel);
                    break;
            }
        }

        SYSTEM_LOG << "TiledToOlympe: Conversion complete. "
                   << outLevel.entities.size() << " entities, "
                   << parallaxLayers_.GetLayerCount() << " parallax layers" << std::endl;

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
        // Check for special object types
        if (obj.objectType == ObjectType::Polyline) {
            ConvertPatrolPath(obj, level);
            return;
        }

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
        std::ostringstream oss;
        oss << "sector_" << obj.id;
        entity->id = oss.str();
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

    void TiledToOlympe::ConvertPatrolPath(const TiledObject& obj, Olympe::Editor::LevelDefinition& level)
    {
        // Create a patrol path entity
        auto entity = std::make_unique<Olympe::Editor::EntityInstance>();
        
        std::ostringstream oss;
        oss << "patrol_" << obj.id;
        entity->id = oss.str();
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
        std::ostringstream oss;
        oss << "entity_" << obj.id;
        entity->id = oss.str();
        entity->name = obj.name.empty() ? ("Object " + std::to_string(obj.id)) : obj.name;
        
        // Get prefab path from type mapping
        entity->prefabPath = GetPrefabPath(obj.type);
        
        // Transform position (Tiled uses top-left origin, Olympe may use different)
        float transformedY = TransformY(obj.y, obj.height);
        entity->position = Olympe::Editor::Vec2(obj.x, transformedY);

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

        // Store rotation if present
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

} // namespace Tiled
} // namespace Olympe
