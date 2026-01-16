/*
 * TiledToOlympe.h - Converter from Tiled format to Olympe LevelDefinition
 * 
 * Converts loaded Tiled maps to Olympe Engine's internal level format,
 * including:
 * - Tile layers -> tileMap grid
 * - Object layers -> entities (with prefab mapping)
 * - Collision objects -> collisionMap
 * - Polygon objects -> sectors
 * - Polyline objects -> patrol paths
 * - Image layers -> parallax layer metadata
 * - Custom properties -> entity overrides
 */

#pragma once

#include "TiledStructures.h"
#include "ParallaxLayerManager.h"
#include "third_party/nlohmann/json.hpp"
#include <string>
#include <map>
#include <memory>
#include <functional>

// Forward declare Olympe::Editor types
namespace Olympe {
namespace Editor {
    struct LevelDefinition;
    struct EntityInstance;
    struct Vec2;
}
}

namespace Olympe {
namespace Tiled {

    // Configuration for conversion
    struct ConversionConfig
    {
        // Map Tiled object types to Olympe prefab paths
        std::map<std::string, std::string> typeToPrefabMap;
        
        // Default prefab for objects without a type
        std::string defaultPrefab;
        
        // Layer name patterns for collision (e.g., "collision", "walls")
        std::vector<std::string> collisionLayerPatterns;
        
        // Layer name patterns for sectors (e.g., "sectors", "zones")
        std::vector<std::string> sectorLayerPatterns;
        
        // Whether to flip Y coordinates (Tiled uses top-left origin)
        bool flipY;
        
        // Base path for resolving relative image paths
        std::string resourceBasePath;

        ConversionConfig()
            : flipY(true) {}
    };

    class TiledToOlympe
    {
    public:
        TiledToOlympe();
        ~TiledToOlympe();

        // Set conversion configuration
        void SetConfig(const ConversionConfig& config);

        // Load prefab mapping from JSON file
        bool LoadPrefabMapping(const std::string& jsonPath);

        // Convert Tiled map to Olympe LevelDefinition
        bool Convert(const TiledMap& tiledMap, Olympe::Editor::LevelDefinition& outLevel);

        // Get last error message
        const std::string& GetLastError() const { return lastError_; }

        // Get parallax layers extracted from the map
        const ParallaxLayerManager& GetParallaxLayers() const { return parallaxLayers_; }

    private:
        // Convert layers
        void ConvertTileLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);
        void ConvertObjectLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);
        void ConvertImageLayer(const TiledLayer& layer);
        void ConvertGroupLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);

        // Convert objects
        void ConvertObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level);
        void ConvertCollisionObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level);
        void ConvertSectorObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level);
        void ConvertPatrolPath(const TiledObject& obj, Olympe::Editor::LevelDefinition& level);

        // Create entity from Tiled object
        std::unique_ptr<Olympe::Editor::EntityInstance> CreateEntity(const TiledObject& obj);

        // Convert properties to JSON overrides
        void PropertiesToOverrides(const std::map<std::string, TiledProperty>& properties,
                                   nlohmann::json& overrides);

        // Get prefab path for object type
        std::string GetPrefabPath(const std::string& objectType);

        // Check if layer name matches pattern
        bool MatchesPattern(const std::string& layerName, const std::vector<std::string>& patterns);

        // Transform Y coordinate if needed
        float TransformY(float y, float height);

        // Initialize collision map
        void InitializeCollisionMap(Olympe::Editor::LevelDefinition& level, int width, int height);

        // Merge tile layer into tilemap
        void MergeTileLayer(const TiledLayer& layer, 
                           std::vector<std::vector<int>>& tileMap,
                           int mapWidth, int mapHeight);

        ConversionConfig config_;
        ParallaxLayerManager parallaxLayers_;
        std::string lastError_;
        int mapWidth_;
        int mapHeight_;
    };

} // namespace Tiled
} // namespace Olympe
