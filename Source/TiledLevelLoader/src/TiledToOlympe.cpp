/*
 * TiledToOlympe.cpp - Converter implementation
 */
#include "../include/TiledLevelLoader.h"
#include "../include/TiledToOlympe.h"
#include "../include/IsometricProjection.h"
#include "../include/ParallaxLayerManager.h"
#include "../../OlympeTilemapEditor/include/LevelManager.h"
#include "../../system/system_utils.h"
#include "../../vector.h"
#include "../../prefabfactory.h"
#include <algorithm>
#include <fstream>

namespace Olympe {
namespace Tiled {

    TiledToOlympe::TiledToOlympe()
    {
        mapWidth_ = 0; mapHeight_ = 0;
        chunkOriginX_ = 0; chunkOriginY_ = 0;
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

    // ✅ NEW: Calculate actual bounds by scanning all tile chunks
    TiledToOlympe::MapBounds TiledToOlympe::CalculateActualMapBounds(const TiledMap& tiledMap)
    {
        MapBounds bounds;
        bool firstTile = true;

        // Scan all layers
        for (const auto& layer : tiledMap.layers)
        {
            // Only process tile layers
            if (layer->type != LayerType::TileLayer)
                continue;

            // Scan all chunks
            for (const auto& chunk : layer->chunks)
            {
                // Chunk position in tiles
                int chunkMinX = chunk.x;
                int chunkMinY = chunk.y;
                int chunkMaxX = chunk.x + chunk.width - 1;
                int chunkMaxY = chunk.y + chunk.height - 1;

                // Update bounds
                if (firstTile)
                {
                    bounds.minTileX = chunkMinX;
                    bounds.minTileY = chunkMinY;
                    bounds.maxTileX = chunkMaxX;
                    bounds.maxTileY = chunkMaxY;
                    firstTile = false;
                }
                else
                {
                    bounds.minTileX = std::min(bounds.minTileX, chunkMinX);
                    bounds.minTileY = std::min(bounds.minTileY, chunkMinY);
                    bounds.maxTileX = std::max(bounds.maxTileX, chunkMaxX);
                    bounds.maxTileY = std::max(bounds.maxTileY, chunkMaxY);
                }
            }
        }

        // Calculate dimensions
        bounds.widthInTiles = bounds.maxTileX - bounds.minTileX + 1;
        bounds.heightInTiles = bounds.maxTileY - bounds.minTileY + 1;

        return bounds;
    }

    bool TiledToOlympe::Convert(const TiledMap& tiledMap, Olympe::Editor::LevelDefinition& outLevel)
    {
        lastError_.clear();
        Olympe::Tiled::ParallaxLayerManager::Get().Clear();
        
        SYSTEM_LOG << "\n+===========================================================+\n";
        SYSTEM_LOG << "| TILED -> OLYMPE CONVERSION - COMPLETE PIPELINE            |\n";
        SYSTEM_LOG << "+===========================================================+\n\n";
        
        // ✅ PHASE 0: Calculate actual map dimensions
        isInfiniteMap_ = tiledMap.infinite;

        if (isInfiniteMap_)
        {
            SYSTEM_LOG << "  /!\\ Map is INFINITE - calculating actual bounds...\n";
            MapBounds bounds = CalculateActualMapBounds(tiledMap);

            mapWidth_ = bounds.widthInTiles;
            mapHeight_ = bounds.heightInTiles;
            
            // Store chunk origin offset for coordinate transformations
            chunkOriginX_ = bounds.minTileX;
            chunkOriginY_ = bounds.minTileY;

            SYSTEM_LOG << "  -> TMJ declared size:  " << tiledMap.width << "x" << tiledMap.height << " (INVALID)\n";
            SYSTEM_LOG << "  -> Actual bounds:      " << bounds.minTileX << "," << bounds.minTileY
                << " to " << bounds.maxTileX << "," << bounds.maxTileY << "\n";
            SYSTEM_LOG << "  -> Chunk origin offset: (" << chunkOriginX_ << ", " << chunkOriginY_ << ")\n";
            SYSTEM_LOG << "  -> Actual map size:    " << mapWidth_ << "x" << mapHeight_ << " tiles ✅\n\n";
        }
        else
        {
            // Non-infinite maps: use declared dimensions
            mapWidth_ = tiledMap.width;
            mapHeight_ = tiledMap.height;
            
            // No chunk offset for finite maps
            chunkOriginX_ = 0;
            chunkOriginY_ = 0;

            SYSTEM_LOG << "  -> Map size (from TMJ): " << mapWidth_ << "x" << mapHeight_ << " tiles\n\n";
        }

        // Initialize config with map properties
        config_.tileWidth = tiledMap.tilewidth;
        config_.tileHeight = tiledMap.tileheight;
        
        switch (tiledMap.orientation) {
            case MapOrientation::Orthogonal: 
                config_.mapOrientation = "orthogonal"; 
                break;
            case MapOrientation::Isometric: 
                config_.mapOrientation = "isometric"; 
                break;
            default: 
                config_.mapOrientation = "orthogonal";
        }
        
        SYSTEM_LOG << "  Map Orientation: " << config_.mapOrientation 
                   << " (" << config_.tileWidth << "x" << config_.tileHeight << ")\n";
        
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
        SYSTEM_LOG << "  ok - Processed " << visualLayerCount << " visual layers\n";
        
        // ===================================================================
        // PHASE 3: SPATIAL STRUCTURES (Sectors, Collision, Navigation)
        // ===================================================================
        SYSTEM_LOG << "[Phase 3/6] Extracting Spatial Structures...\n";
        int spatialObjectCount = 0;
        ExtractSpatialStructures(tiledMap, outLevel, spatialObjectCount);
        SYSTEM_LOG << "  ok - Extracted " << spatialObjectCount << " spatial objects\n";
        
        // ===================================================================
        // PHASE 4: GAME OBJECTS (Categorized by Type)
        // ===================================================================
        SYSTEM_LOG << "[Phase 4/6] Converting Game Objects...\n";
        ConversionStats stats;
        CategorizeGameObjects(tiledMap, outLevel, stats);
        SYSTEM_LOG << "  ok - Static: " << stats.staticObjects 
                   << " | Dynamic: " << stats.dynamicObjects
                   << " | Paths: " << stats.patrolPaths
                   << " | Sounds: " << stats.soundObjects << "\n";
        
        // ===================================================================
        // POST-CONVERSION: Normalize Entity Types Immediately
        // ===================================================================
        SYSTEM_LOG << "[Post-Conversion] Normalizing Entity Types...\n";
        PrefabFactory& factory = PrefabFactory::Get();
        
        int normalizedCount = 0;
        for (auto& entity : outLevel.entities)
        {
            if (!entity) continue;
            
            std::string originalType = entity->type;
            entity->type = factory.NormalizeType(originalType);
            
            if (originalType != entity->type)
            {
                normalizedCount++;
            }
        }
        
        // Also normalize categorizedObjects (they may be copies)
        for (auto& entity : outLevel.categorizedObjects.dynamicObjects)
        {
            if (entity) entity->type = factory.NormalizeType(entity->type);
        }
        for (auto& entity : outLevel.categorizedObjects.staticObjects)
        {
            if (entity) entity->type = factory.NormalizeType(entity->type);
        }
        for (auto& entity : outLevel.categorizedObjects.patrolPaths)
        {
            if (entity) entity->type = factory.NormalizeType(entity->type);
        }
        for (auto& entity : outLevel.categorizedObjects.soundObjects)
        {
            if (entity) entity->type = factory.NormalizeType(entity->type);
        }
        
        SYSTEM_LOG << "  ok - Normalized " << normalizedCount << " entity types\n";
        
        // ===================================================================
        // PHASE 5: OBJECT RELATIONSHIPS (Links, References)
        // ===================================================================
        SYSTEM_LOG << "[Phase 5/6] Extracting Object Relationships...\n";
        int linkCount = 0;
        ExtractObjectRelationships(tiledMap, outLevel, linkCount);
        SYSTEM_LOG << "  ok - Created " << linkCount << " object links\n";
        
        // ===================================================================
        // PHASE 6: RESOURCE CATALOG
        // ===================================================================
        SYSTEM_LOG << "[Phase 6/6] Building Resource Catalog...\n";
        BuildResourceCatalog(tiledMap, outLevel);
        SYSTEM_LOG << "  ok - Tilesets: " << outLevel.resources.tilesetPaths.size()
                   << " | Images: " << outLevel.resources.imagePaths.size()
                   << " | Audio: " << outLevel.resources.audioPaths.size() << "\n";
        
        // ===================================================================
        // FINAL SUMMARY
        // ===================================================================
        SYSTEM_LOG << "\n+===========================================================+\n";
        SYSTEM_LOG << "| CONVERSION COMPLETE                                       |\n";
        SYSTEM_LOG << "+===========================================================+\n";
        SYSTEM_LOG << "| Map: " << outLevel.mapConfig.orientation 
                   << " " << outLevel.mapConfig.mapWidth << "x" << outLevel.mapConfig.mapHeight << "\n";
        SYSTEM_LOG << "| Visual Layers: " << visualLayerCount << "\n";
        SYSTEM_LOG << "| Entities: " << stats.totalObjects << "\n";
        SYSTEM_LOG << "| Relationships: " << linkCount << "\n";
        SYSTEM_LOG << "+===========================================================+\n\n";
        
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

        ParallaxLayerManager::Get().AddLayer(parallax);
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
        auto entityDescriptor = ParseEntityDescriptor(obj);
        if (entityDescriptor) {
            level.entities.push_back(std::move(entityDescriptor));
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
        
        // Transform position based on map orientation
        entity->position = TransformObjectPosition(obj.x, obj.y);

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
        
        // Transform position based on map orientation
        entity->position = TransformObjectPosition(obj.x, obj.y);
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
        
        // Transform position based on map orientation
        entity->position = TransformObjectPosition(obj.x, obj.y);

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

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::ParseEntityDescriptor(const TiledObject& obj)
    {
        auto entityDescriptor = std::make_unique<Olympe::Editor::EntityInstance>();
        
        // Generate unique ID (memory structure, NOT ECS entity)
        entityDescriptor->id = "entity_" + std::to_string(obj.id);
        entityDescriptor->name = obj.name.empty() ? ("Object " + std::to_string(obj.id)) : obj.name;
        
        // Store entity type (will be normalized later)
        entityDescriptor->type = obj.type;
        
        // Get prefab path from type mapping
        entityDescriptor->prefabPath = GetPrefabPath(obj.type);
        
        // Transform position based on map orientation (isometric vs orthogonal)
        entityDescriptor->position = TransformObjectPosition(obj.x, obj.y);
        
        SYSTEM_LOG << "  -> Parsed entity descriptor: '" << entityDescriptor->name 
                   << "' (type: " << entityDescriptor->type << ")\n";

        // Store rotation (extract to entity level field)
        entityDescriptor->rotation = obj.rotation;

        // Convert properties to overrides
        PropertiesToOverrides(obj.properties, entityDescriptor->overrides);

        // Store dimensions if present
        if (obj.width > 0 || obj.height > 0) {
            if (!entityDescriptor->overrides.contains("Transform")) {
                entityDescriptor->overrides["Transform"] = nlohmann::json::object();
            }
            entityDescriptor->overrides["Transform"]["width"] = obj.width;
            entityDescriptor->overrides["Transform"]["height"] = obj.height;
        }

        // Store rotation in Transform overrides if present
        if (obj.rotation != 0.0f) {
            if (!entityDescriptor->overrides.contains("Transform")) {
                entityDescriptor->overrides["Transform"] = nlohmann::json::object();
            }
            entityDescriptor->overrides["Transform"]["rotation"] = obj.rotation;
        }

        return entityDescriptor;
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
    
    Vector TiledToOlympe::TransformObjectPosition(float x, float y)
    {
        bool isIsometric = (config_.mapOrientation == "isometric");

        if (isIsometric)
        {
            if (config_.tileWidth <= 0 || config_.tileHeight <= 0)
            {
                SYSTEM_LOG << "  /!\\ Invalid tile dimensions for isometric conversion\n";
                return Vector(x, y, 0.0f);
            }

            // Step 1: Convert TMJ pixels → tile coordinates
            float tileX = x / static_cast<float>(config_.tileWidth);
            float tileY = y / static_cast<float>(config_.tileHeight);

            // Step 2: Apply render order transformation (Y-axis inversion)
            // For render orders with "up" (right-up, left-up), invert Y-axis
            // because Tiled's Y-axis points down (screen) but isometric Y-axis points up (world)
            // IMPORTANT: This must happen BEFORE chunk translation to avoid inverting the chunk offset
            if (config_.renderOrder == "left-up" || config_.renderOrder == "right-up") {
                tileY = -tileY;
            }

            // Step 3: Translate to chunk coordinate system
            // (align entity coords with chunk origin offset)
            // This happens AFTER Y-axis inversion so the chunk offset is applied correctly
            tileX -= chunkOriginX_;
            tileY -= chunkOriginY;

            // Step 4: Apply isometric projection
            // WorldToIso expects tile coordinates and handles the projection
            Vector isoPos = IsometricProjection::WorldToIso(
                tileX,
                tileY,
                config_.tileWidth,
                config_.tileHeight
            );

            // ✅ CLEANED DEBUG LOG (only for important objects or on demand)
#ifdef DETAILED_POSITION_DEBUG
            SYSTEM_LOG << "  [POS] " << x << "," << y << " → tile " << tileX << "," << tileY 
                       << " → ISO " << isoPos.x << "," << isoPos.y << "\n";
#endif

            return Vector(isoPos.x, isoPos.y, 0.0f);
        }

        // Orthogonal case
        return Vector(x, y, 0.0f);
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
        
        // Store render order in conversion config for use in coordinate transformations
        config_.renderOrder = outLevel.mapConfig.renderOrder;
        
        // Set world size
        outLevel.worldSize.x = (float) tiledMap.width * tiledMap.tilewidth;
        outLevel.worldSize.y = (float) tiledMap.height * tiledMap.tileheight;
        
        // Background color
        if (!tiledMap.backgroundcolor.empty()) {
            outLevel.ambientColor = tiledMap.backgroundcolor;
        }
        
        SYSTEM_LOG << "  -> Map: " << outLevel.mapConfig.orientation 
                   << " " << outLevel.mapConfig.mapWidth << "x" << outLevel.mapConfig.mapHeight
                   << " (tiles: " << outLevel.mapConfig.tileWidth << "x" << outLevel.mapConfig.tileHeight << ")\n";
        SYSTEM_LOG << "  -> Render order: " << config_.renderOrder << "\n";
    }

    void TiledToOlympe::ExtractMapMetadata(const TiledMap& tiledMap, 
                                          Olympe::Editor::LevelDefinition& outLevel)
    {
        // Store map configuration for rendering
        // FIX: Store orientation as STRING, not INT
        switch (tiledMap.orientation) {
            case MapOrientation::Orthogonal: 
                outLevel.metadata.customData["orientation"] = "orthogonal"; 
                break;
            case MapOrientation::Isometric: 
                outLevel.metadata.customData["orientation"] = "isometric"; 
                break;
            case MapOrientation::Staggered: 
                outLevel.metadata.customData["orientation"] = "staggered"; 
                break;
            case MapOrientation::Hexagonal: 
                outLevel.metadata.customData["orientation"] = "hexagonal"; 
                break;
            default: 
                outLevel.metadata.customData["orientation"] = "unknown"; 
                break;
        }
        
        outLevel.metadata.customData["tilewidth"] = tiledMap.tilewidth;
        outLevel.metadata.customData["tileheight"] = tiledMap.tileheight;
        
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
                    
                    SYSTEM_LOG << "  -> Image Layer: '" << visual.name << "' (parallax: " 
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
                        
                        SYSTEM_LOG << "  -> Tile Layer (Infinite): '" << tileDef.name << "' (" 
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
                        
                        SYSTEM_LOG << "  -> Tile Layer: '" << tileDef.name << "' (" 
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
        if (ParallaxLayerManager::Get().GetLayerCount() > 0)
        {
            nlohmann::json parallaxLayersJson = nlohmann::json::array();
            
            for (size_t i = 0; i < ParallaxLayerManager::Get().GetLayerCount(); ++i)
            {
                const ParallaxLayer* layer = ParallaxLayerManager::Get().GetLayer(i);
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
        
        // Store tile layers in metadata
        if (!outLevel.tileLayers.empty())
        {
            nlohmann::json tileLayersJson = nlohmann::json::array();
            
            for (const auto& tileLayer : outLevel.tileLayers)
            {
                nlohmann::json layerJson = nlohmann::json::object();
                layerJson["name"] = tileLayer.name;
                layerJson["type"] = "tilelayer";
                layerJson["zOrder"] = tileLayer.zOrder;
                layerJson["opacity"] = tileLayer.opacity;
                layerJson["visible"] = tileLayer.visible;
                layerJson["isInfinite"] = tileLayer.isInfinite;
                layerJson["encoding"] = "base64";  // Store encoding for decoding
                
                if (tileLayer.isInfinite && !tileLayer.chunks.empty())
                {
                    nlohmann::json chunksJson = nlohmann::json::array();
                    
                    for (const auto& chunk : tileLayer.chunks)
                    {
                        nlohmann::json chunkJson = nlohmann::json::object();
                        chunkJson["x"] = chunk.x;
                        chunkJson["y"] = chunk.y;
                        chunkJson["width"] = chunk.width;
                        chunkJson["height"] = chunk.height;
                        
                        // Flatten tile data to array (with flip flags)
                        nlohmann::json dataJson = nlohmann::json::array();
                        for (int y = 0; y < chunk.height; ++y)
                        {
                            for (int x = 0; x < chunk.width; ++x)
                            {
                                uint32_t gid = 0;
                                uint8_t flipFlags = 0;
                                
                                if (y < chunk.tiles.size() && x < chunk.tiles[y].size())
                                {
                                    gid = chunk.tiles[y][x];
                                }
                                
                                if (y < chunk.tileFlipFlags.size() && x < chunk.tileFlipFlags[y].size())
                                {
                                    flipFlags = chunk.tileFlipFlags[y][x];
                                }
                                
                                // Reconstruct GID with flip flags (Tiled format)
                                uint32_t fullGID = gid;
                                if (flipFlags & 0x1) fullGID |= 0x80000000;  // Horizontal flip
                                if (flipFlags & 0x2) fullGID |= 0x40000000;  // Vertical flip
                                if (flipFlags & 0x4) fullGID |= 0x20000000;  // Diagonal flip
                                
                                dataJson.push_back((int)fullGID);
                            }
                        }
                        
                        chunkJson["data"] = dataJson;
                        chunksJson.push_back(chunkJson);
                    }
                    
                    layerJson["chunks"] = chunksJson;
                }
                else if (!tileLayer.tiles.empty())
                {
                    // Finite map
                    int width = tileLayer.tiles.empty() ? 0 : static_cast<int>(tileLayer.tiles[0].size());
                    int height = static_cast<int>(tileLayer.tiles.size());
                    
                    layerJson["width"] = width;
                    layerJson["height"] = height;
                    
                    nlohmann::json dataJson = nlohmann::json::array();
                    for (int y = 0; y < height; ++y)
                    {
                        for (int x = 0; x < width; ++x)
                        {
                            int  gid = 0;
                            if (y < tileLayer.tiles.size() && x < tileLayer.tiles[y].size())
                            {
                                gid = tileLayer.tiles[y][x];
                            }
                            dataJson.push_back(static_cast<int>(gid));
                        }
                    }
                    layerJson["data"] = dataJson;
                }
                
                tileLayersJson.push_back(layerJson);
            }
            
            outLevel.metadata.customData["tileLayers"] = tileLayersJson;
            
            SYSTEM_LOG << "  ok - Stored " << tileLayersJson.size() 
                       << " tile layers in metadata\n";
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
                
                SYSTEM_LOG << "  -> Collision Layer: '" << layer->name << "' (filled tiles: " << collisionTileCount << ")\n";
            }
            
            // Process object layers (sectors, collision shapes)
            if (layer->type == LayerType::ObjectGroup) {
                for (const auto& obj : layer->objects) {
                    // Sector objects (polygons)
                    if (obj.objectType == ObjectType::Polygon) {
                        Olympe::Editor::LevelDefinition::SectorDef sector;
                        sector.name = obj.name.empty() ? ("Sector_" + std::to_string(obj.id)) : obj.name;
                        sector.type = obj.type;
                        sector.position = Vector(obj.x, TransformY(obj.y, 0), 0.f);
                        
                        for (const auto& pt : obj.polygon) {
                            sector.polygon.push_back(Vector(
                                pt.x, config_.flipY ? -pt.y : pt.y, 0.f
                            ));
                        }
                        
                        // Store properties
                        for (const auto& prop : obj.properties) {
                            sector.properties[prop.first] = PropertyToJSON(prop.second);
                        }
                        
                        outLevel.sectors.push_back(sector);
                        objectCount++;
                        
                        SYSTEM_LOG << "  -> Sector: '" << sector.name << "' (" << sector.polygon.size() << " points)\n";
                    }
                    
                    // Collision shapes (rectangles)
                    else if (obj.type == "collision" && obj.objectType == ObjectType::Rectangle) {
                        Olympe::Editor::LevelDefinition::CollisionShape shape;
                        shape.name = obj.name;
                        shape.type = Olympe::Editor::LevelDefinition::CollisionShape::Rectangle;
                        shape.position = Vector(obj.x, TransformY(obj.y, obj.height), 0.f);
                        shape.size = Vector(obj.width, obj.height, 0.f);
                        
                        outLevel.collisionShapes.push_back(shape);
                        objectCount++;
                        
                        SYSTEM_LOG << "  -> Collision Shape: '" << shape.name << "' (rect: " 
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
            "item", "collectible", "key", "treasure", "waypoint", "way", "trigger", "portal", "door", "exit",
            "pickup", "interactable", "checkpoint", "teleporter", "switch", "spawn"
        };
        
        const std::set<std::string> dynamicTypes = {
            "player", "npc", "guard", "enemy", "zombie"
        };
        
        const std::set<std::string> soundTypes = {
            "ambient", "sound", "music"
        };
        
        // ok - FIX #1: Track global zOrder across ALL layers for depth sorting
        int globalZOrder = 0;
        
        for (const auto& layer : tiledMap.layers) {
            // ok - Process object layers and assign zOrder
            if (layer->type == LayerType::ObjectGroup) {
                if (!layer->visible) {
                    globalZOrder++;  // ok - Increment even for invisible layers to maintain ordering
                    continue;
                }
                
                SYSTEM_LOG << "[CategorizeGameObjects] Processing object layer '" 
                           << layer->name << "' (zOrder: " << globalZOrder << ")\n";
            
            for (const auto& obj : layer->objects) {
                // PROCESS collision polylines/polygons BEFORE filtering
                std::string typeLower = obj.type;
                std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
                
                if (typeLower == "collision" || typeLower.find("collision") != std::string::npos) {
                    if (obj.objectType == ObjectType::Polyline || obj.objectType == ObjectType::Polygon) {
                        auto collisionDescriptor = ParseCollisionPolylineDescriptor(obj);
                        if (collisionDescriptor) {
                            // ok - CRITICAL FIX: Store layer zOrder in position.z
                            collisionDescriptor->position.z = static_cast<float>(globalZOrder);
                            
                            // Create a copy for the legacy entities array
                            auto entityCopy = std::make_unique<Olympe::Editor::EntityInstance>();
                            entityCopy->id = collisionDescriptor->id;
                            entityCopy->prefabPath = collisionDescriptor->prefabPath;
                            entityCopy->name = collisionDescriptor->name;
                            entityCopy->type = collisionDescriptor->type;
                            entityCopy->spritePath = collisionDescriptor->spritePath;
                            entityCopy->position = collisionDescriptor->position;
                            entityCopy->overrides = collisionDescriptor->overrides;
                            
                            outLevel.categorizedObjects.staticObjects.push_back(std::move(collisionDescriptor));
                            outLevel.entities.push_back(std::move(entityCopy));
                            stats.staticObjects++;
                            stats.totalObjects++;
                            SYSTEM_LOG << "  -> Collision Polyline: '" << obj.name << "' (zOrder: " << globalZOrder << ")\n";
                        }
                        continue;  // Skip to next object
                    }
                }
                
                // Skip polygon objects (sectors already processed in ExtractSpatialStructures)
                if (obj.objectType == ObjectType::Polygon) {
                    continue;
                }
                
                // Skip sector/zone objects (already processed)
                if (typeLower.find("sector") != std::string::npos || 
                    typeLower.find("zone") != std::string::npos) {
                    continue;
                }
                
                auto entityDescriptor = ParseEntityDescriptor(obj);
                if (!entityDescriptor) continue;
                
                // ok - CRITICAL FIX: Store layer zOrder in position.z
                entityDescriptor->position.z = static_cast<float>(globalZOrder);
                
                SYSTEM_LOG << "  -> Entity '" << entityDescriptor->name 
                           << "' assigned zOrder: " << globalZOrder << "\n";
                
                // Create a copy for the legacy entities array
                // Note: Both categorizedObjects (new system) and entities (legacy) need to be populated
                // for backward compatibility. The entity is moved into categorizedObjects and a copy
                // is placed in the legacy array.
                auto entityCopy = std::make_unique<Olympe::Editor::EntityInstance>();
                entityCopy->id = entityDescriptor->id;
                entityCopy->prefabPath = entityDescriptor->prefabPath;
                entityCopy->name = entityDescriptor->name;
                entityCopy->type = entityDescriptor->type;
                entityCopy->spritePath = entityDescriptor->spritePath;
                entityCopy->position = entityDescriptor->position;
                entityCopy->overrides = entityDescriptor->overrides;
                
                // Categorize by type (use typeLower for case-insensitive comparison)
                if (obj.objectType == ObjectType::Polyline && typeLower == "way") {
                    outLevel.categorizedObjects.patrolPaths.push_back(std::move(entityDescriptor));
                    stats.patrolPaths++;
                    SYSTEM_LOG << "  -> Patrol Path: '" << obj.name << "' (" << obj.polyline.size() << " points)\n";
                }
                else if (soundTypes.count(typeLower)) {
                    outLevel.categorizedObjects.soundObjects.push_back(std::move(entityDescriptor));
                    stats.soundObjects++;
                    SYSTEM_LOG << "  -> Sound Object: '" << obj.name << "' (type: " << obj.type << ")\n";
                }
                else if (staticTypes.count(typeLower)) {
                    outLevel.categorizedObjects.staticObjects.push_back(std::move(entityDescriptor));
                    stats.staticObjects++;
                }
                else if (dynamicTypes.count(typeLower)) {
                    outLevel.categorizedObjects.dynamicObjects.push_back(std::move(entityDescriptor));
                    stats.dynamicObjects++;
                }
                else {
                    // Default: static object
                    outLevel.categorizedObjects.staticObjects.push_back(std::move(entityDescriptor));
                    stats.staticObjects++;
                }
                
                // Add copy to legacy entities array for backward compatibility
                outLevel.entities.push_back(std::move(entityCopy));
                
                stats.totalObjects++;
            }
            
            // ok - Increment zOrder after processing object layer
            globalZOrder++;
            
            } else {
                // ok - For non-object layers (tile layers, image layers, etc.), still increment zOrder
                // to maintain correct ordering between all layer types
                globalZOrder++;
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
        
        // Build object ID -> name mapping
        std::map<int, std::string> idToName;
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup) continue;
            for (const auto& obj : layer->objects) {
                idToName[obj.id] = obj.name;
            }
        }
        
        SYSTEM_LOG << "[DEBUG] ExtractObjectRelationships - Processing objects...\n";
        
        // Extract relationships from custom properties
        for (const auto& layer : tiledMap.layers) {
            if (layer->type != LayerType::ObjectGroup) continue;
            
            for (const auto& obj : layer->objects) {
                // Debug: Show properties of guard/npc objects
                std::string typeLower = obj.type;
                std::transform(typeLower.begin(), typeLower.end(), typeLower.begin(), ::tolower);
                
                if (typeLower == "guard" || typeLower == "npc") {
                    SYSTEM_LOG << "  [DEBUG] Object '" << obj.name << "' (type: " << obj.type << ") properties:\n";
                    for (const auto& prop : obj.properties) {
                        SYSTEM_LOG << "    - '" << prop.first << "' = ";
                        if (prop.second.type == PropertyType::Object) {
                            int targetID = prop.second.intValue;
                            std::string targetName = idToName.count(targetID) ? idToName[targetID] : "(unknown)";
                            SYSTEM_LOG << "(Object ID: " << targetID << " -> '" << targetName << "')\n";
                        } else if (prop.second.type == PropertyType::String || prop.second.type == PropertyType::File) {
                            SYSTEM_LOG << "\"" << prop.second.stringValue << "\"\n";
                        } else if (prop.second.type == PropertyType::Int) {
                            SYSTEM_LOG << prop.second.intValue << "\n";
                        } else if (prop.second.type == PropertyType::Float) {
                            SYSTEM_LOG << prop.second.floatValue << "\n";
                        } else if (prop.second.type == PropertyType::Bool) {
                            SYSTEM_LOG << (prop.second.boolValue ? "true" : "false") << "\n";
                        }
                    }
                }
                
                // Check for "patrol way" property (NPC -> patrol path link)
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
                    
                    SYSTEM_LOG << "  -> Link: '" << link.sourceObjectName << "' -> '" 
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
                    
                    SYSTEM_LOG << "  -> Link: '" << link.sourceObjectName << "' -> '" 
                               << link.targetObjectName << "' (trigger_target)\n";
                }
            }
        }
        
        if (linkCount == 0) {
            SYSTEM_LOG << "  /!\\ No object relationships found. Check:\n";
            SYSTEM_LOG << "    - Guards should have 'patrol way' property (Object type)\n";
            SYSTEM_LOG << "    - Property must reference a 'way' object by ID\n";
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
        
        // Store tileset metadata
        if (!tiledMap.tilesets.empty())
        {
            nlohmann::json tilesetsJson = nlohmann::json::array();
            
            for (const auto& tileset : tiledMap.tilesets)
            {
                nlohmann::json tilesetJson = nlohmann::json::object();
                tilesetJson["firstgid"] = tileset.firstgid;
                tilesetJson["name"] = tileset.name;
                tilesetJson["tilewidth"] = tileset.tilewidth;
                tilesetJson["tileheight"] = tileset.tileheight;
                tilesetJson["tilecount"] = tileset.tilecount;
                tilesetJson["columns"] = tileset.columns;
                tilesetJson["imagewidth"] = tileset.imagewidth;
                tilesetJson["imageheight"] = tileset.imageheight;
                tilesetJson["margin"] = tileset.margin;
                tilesetJson["spacing"] = tileset.spacing;
                
                // Handle image-based tilesets
                if (!tileset.image.empty())
                {
                    tilesetJson["image"] = tileset.image;
                    tilesetJson["type"] = "image";
                }
                // Handle collection tilesets (individual tiles)
                else if (!tileset.tiles.empty())
                {
                    tilesetJson["type"] = "collection";
                    nlohmann::json tilesJson = nlohmann::json::array();
                    
                    for (const auto& tile : tileset.tiles)
                    {
                        nlohmann::json tileJson = nlohmann::json::object();
                        tileJson["id"] = tile.id;
                        tileJson["image"] = tile.image;
                        tileJson["width"] = tile.imagewidth;
                        tileJson["height"] = tile.imageheight;
                        tilesJson.push_back(tileJson);
                    }
                    
                    tilesetJson["tiles"] = tilesJson;
                }
                
                tilesetJson["source"] = tileset.source;
                
                tilesetsJson.push_back(tilesetJson);
            }
            
            outLevel.metadata.customData["tilesets"] = tilesetsJson;
            
            SYSTEM_LOG << "  ok - Stored " << tilesetsJson.size() << " tilesets in metadata\n";
        }
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
        
        // Fix potential buffer underflow - check string has content after '#'
        if (colorStr == "#") return 0xFFFFFFFF;
        
        std::string hex = colorStr;
        if (hex.length() > 0 && hex[0] == '#') {
            hex = hex.substr(1);
        }
        
        // Handle empty string after removing '#'
        if (hex.empty()) return 0xFFFFFFFF;
        
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

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::ParseSectorDescriptor(const TiledObject& obj)
    {
        auto entityDescriptor = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entityDescriptor->id = "sector_" + std::to_string(obj.id);
        entityDescriptor->name = obj.name.empty() ? ("Sector_" + std::to_string(obj.id)) : obj.name;
        entityDescriptor->type = "Sector";
        entityDescriptor->prefabPath = "Blueprints/Sector.json";
        
        // Use TMJ coordinates directly - no conversion needed
        entityDescriptor->position = Vector(obj.x, obj.y, 0.0f);
        entityDescriptor->rotation = obj.rotation;
        
        // Store polygon in overrides
        nlohmann::json polygon = nlohmann::json::array();
        for (const auto& pt : obj.polygon) {
            nlohmann::json point;
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            polygon.push_back(point);
        }
        
        entityDescriptor->overrides["Sector"] = nlohmann::json::object();
        entityDescriptor->overrides["Sector"]["polygon"] = polygon;
        entityDescriptor->overrides["Sector"]["type"] = obj.type;
        
        PropertiesToOverrides(obj.properties, entityDescriptor->overrides);
        
        return entityDescriptor;
    }

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::ParsePatrolPathDescriptor(const TiledObject& obj)
    {
        auto entityDescriptor = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entityDescriptor->id = "patrol_" + std::to_string(obj.id);
        entityDescriptor->name = obj.name.empty() ? ("PatrolPath_" + std::to_string(obj.id)) : obj.name;
        entityDescriptor->type = "PatrolPath";
        entityDescriptor->prefabPath = "Blueprints/PatrolPath.json";
        
        // Use TMJ coordinates directly - no conversion needed
        entityDescriptor->position = Vector(obj.x, obj.y, 0.0f);
        entityDescriptor->rotation = obj.rotation;
        
        // Store polyline in overrides
        nlohmann::json path = nlohmann::json::array();
        for (const auto& pt : obj.polyline) {
            nlohmann::json point;
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            path.push_back(point);
        }
        
        entityDescriptor->overrides["AIBlackboard_data"] = nlohmann::json::object();
        entityDescriptor->overrides["AIBlackboard_data"]["patrolPath"] = path;
        
        PropertiesToOverrides(obj.properties, entityDescriptor->overrides);
        
        return entityDescriptor;
    }

    std::unique_ptr<Olympe::Editor::EntityInstance> TiledToOlympe::ParseCollisionPolylineDescriptor(const TiledObject& obj)
    {
        auto entityDescriptor = std::make_unique<Olympe::Editor::EntityInstance>();
        
        entityDescriptor->id = "collision_poly_" + std::to_string(obj.id);
        entityDescriptor->name = obj.name.empty() ? ("CollisionPoly_" + std::to_string(obj.id)) : obj.name;
        entityDescriptor->type = "CollisionPolygon";
        entityDescriptor->prefabPath = "Blueprints/CollisionPolygon.json";
        
        // Use TMJ coordinates directly - no conversion needed
        entityDescriptor->position = Vector(obj.x, obj.y, 0.0f);
        entityDescriptor->rotation = obj.rotation;
        
        // Store polyline/polygon points
        nlohmann::json polygon = nlohmann::json::array();
        const auto& points = (obj.objectType == ObjectType::Polygon) ? obj.polygon : obj.polyline;
        
        for (const auto& pt : points) {
            nlohmann::json point;
            point["x"] = pt.x;
            point["y"] = config_.flipY ? -pt.y : pt.y;
            polygon.push_back(point);
        }
        
        entityDescriptor->overrides["CollisionPolygon"] = nlohmann::json::object();
        entityDescriptor->overrides["CollisionPolygon"]["points"] = polygon;
        entityDescriptor->overrides["CollisionPolygon"]["isClosed"] = (obj.objectType == ObjectType::Polygon);
        
        PropertiesToOverrides(obj.properties, entityDescriptor->overrides);
        
        return entityDescriptor;
    }

} // namespace Tiled
} // namespace Olympe
