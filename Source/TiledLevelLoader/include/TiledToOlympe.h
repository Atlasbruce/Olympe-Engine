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
#include "../../vector.h"
#include <string>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include "../../third_party/nlohmann/json.hpp"

// Forward declare Olympe::Editor types
namespace Olympe {
namespace Editor {
    struct LevelDefinition;
    struct EntityInstance;
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
        
        // Map orientation ("orthogonal" or "isometric")
        std::string mapOrientation;
        
        // Tile dimensions for isometric projection
        int tileWidth;
        int tileHeight;
        
        // Render order ("right-down", "right-up", "left-down", "left-up")
        std::string renderOrder;

        ConversionConfig()
            : flipY(true), mapOrientation("orthogonal"), tileWidth(0), tileHeight(0), renderOrder("right-down") {}
    };



    class TiledToOlympe
    {
    public:
        TiledToOlympe();
        ~TiledToOlympe();

        // Set conversion configuration
        void SetConfig(const ConversionConfig& config);

        // Calculate actual map bounds for infinite maps
        struct MapBounds
        {
            int minTileX = 0;
            int minTileY = 0;
            int maxTileX = 0;
            int maxTileY = 0;
            int widthInTiles = 0;   // Actual width in tiles
            int heightInTiles = 0;  // Actual height in tiles
        };

        TiledToOlympe::MapBounds CalculateActualMapBounds(const TiledMap& tiledMap);

        // Convert Tiled map to Olympe LevelDefinition
        bool Convert(const TiledMap& tiledMap, Olympe::Editor::LevelDefinition& outLevel);

        // Load prefab mapping from JSON file
        bool LoadPrefabMapping(const std::string& jsonFilePath);

        // Get last error message
        const std::string& GetLastError() const { return lastError_; }

    private:
        // Conversion statistics
        struct ConversionStats {
            int staticObjects = 0;
            int dynamicObjects = 0;
            int patrolPaths = 0;
            int soundObjects = 0;
            int totalObjects = 0;
        };

        // New 6-phase pipeline methods
        void ExtractMapConfiguration(const TiledMap& tiledMap, 
                                    Olympe::Editor::LevelDefinition& outLevel);
        void ExtractMapMetadata(const TiledMap& tiledMap, 
                               Olympe::Editor::LevelDefinition& outLevel);
        void ProcessVisualLayers(const TiledMap& tiledMap, 
                                Olympe::Editor::LevelDefinition& outLevel,
                                int& layerCount);
        void ExtractSpatialStructures(const TiledMap& tiledMap,
                                     Olympe::Editor::LevelDefinition& outLevel,
                                     int& objectCount);
        void CategorizeGameObjects(const TiledMap& tiledMap,
                                  Olympe::Editor::LevelDefinition& outLevel,
                                  ConversionStats& stats);
        void ExtractObjectRelationships(const TiledMap& tiledMap,
                                       Olympe::Editor::LevelDefinition& outLevel,
                                       int& linkCount);
        void BuildResourceCatalog(const TiledMap& tiledMap,
                                 Olympe::Editor::LevelDefinition& outLevel);

        // Helper methods
        std::string ResolveImagePath(const std::string& imagePath);
        nlohmann::json PropertyToJSON(const TiledProperty& prop);
        void ProcessGroupLayers(const TiledLayer& groupLayer,
                               Olympe::Editor::LevelDefinition& outLevel,
                               int& zOrder,
                               int& layerCount);

        // Convert layers (legacy methods kept for compatibility)
        void ConvertTileLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);
        void ConvertObjectLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);
        void ConvertImageLayer(const TiledLayer& layer);
        void ConvertGroupLayer(const TiledLayer& layer, Olympe::Editor::LevelDefinition& level);

        // Convert objects (with layer offset support)
        void ConvertObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level, 
                          float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);
        void ConvertCollisionObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level,
                                   float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);
        void ConvertSectorObject(const TiledObject& obj, Olympe::Editor::LevelDefinition& level,
                                float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);
        void ConvertPatrolPath(const TiledObject& obj, Olympe::Editor::LevelDefinition& level,
                              float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);
        void ConvertPolygonCollision(const TiledObject& obj, Olympe::Editor::LevelDefinition& level,
                                    float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);

        // Parse entity descriptor from Tiled object (memory structure, NOT ECS entity)
        std::unique_ptr<Olympe::Editor::EntityInstance> ParseEntityDescriptor(const TiledObject& obj,
                                                                               float layerOffsetX = 0.0f, 
                                                                               float layerOffsetY = 0.0f);

        // Convert properties to JSON overrides
        void PropertiesToOverrides(const std::map<std::string, TiledProperty>& properties,
                                   nlohmann::json& overrides);

        // Get prefab path for object type
        std::string GetPrefabPath(const std::string& objectType);

        // Check if layer name matches pattern
        bool MatchesPattern(const std::string& layerName, const std::vector<std::string>& patterns);

        // Transform Y coordinate if needed
        float TransformY(float y, float height);
        
        // Transform object position - returns Vector directly (no conversion needed, TMJ coordinates are already in pixels)
        // Added layer offsets for proper position adjustment
        Vector TransformObjectPosition(float x, float y, float layerOffsetX = 0.0f, float layerOffsetY = 0.0f);

        // Initialize collision map
        void InitializeCollisionMap(Olympe::Editor::LevelDefinition& level, int width, int height);

        // Merge tile layer into tilemap
        void MergeTileLayer(const TiledLayer& layer, 
                           std::vector<std::vector<int>>& tileMap,
                           int mapWidth, int mapHeight);

        // Parse tint color from hex string to uint32_t
        uint32_t ParseTintColor(const std::string& colorStr);

        // Parse sector entity descriptor from polygon object (convenience wrapper)
        std::unique_ptr<Olympe::Editor::EntityInstance> ParseSectorDescriptor(const TiledObject& obj,
                                                                               float layerOffsetX = 0.0f, 
                                                                               float layerOffsetY = 0.0f);

        // Parse patrol path entity descriptor from polyline object (convenience wrapper)
        std::unique_ptr<Olympe::Editor::EntityInstance> ParsePatrolPathDescriptor(const TiledObject& obj,
                                                                                   float layerOffsetX = 0.0f, 
                                                                                   float layerOffsetY = 0.0f);

        // Parse collision polyline entity descriptor from polyline/polygon object
        std::unique_ptr<Olympe::Editor::EntityInstance> ParseCollisionPolylineDescriptor(const TiledObject& obj,
                                                                                         float layerOffsetX = 0.0f, 
                                                                                         float layerOffsetY = 0.0f);

        ConversionConfig config_;
        std::string lastError_;

        // ? MODIFIED: Actual map dimensions (calculated, not from TMJ)
        int mapWidth_;   // Actual width in tiles (from bounds calculation)
        int mapHeight_;  // Actual height in tiles (from bounds calculation)

		// ? NEW: Track if map is infinite. if true the map width/height are not reliable => we need to calculate bounds of the map while it loads
        bool isInfiniteMap_ = false;
        
        // Chunk coordinate system origin (for infinite maps)
        int chunkOriginX_ = 0;
        int chunkOriginY_ = 0;
        
        // Global offsets for entity position adjustment
        // These offsets correct systematic positioning errors in isometric coordinate transformation
        float globalOffsetX_ = 0.0f;
        float globalOffsetY_ = 0.0f;
        
        // Cached flags for performance optimization (updated during configuration)
        bool hasOffsets_ = false;  // true if any chunk origin or global offsets are non-zero
        bool requiresYFlip_ = false;  // true if render order requires Y-flip ("left-up" or "right-up")
    };

} // namespace Tiled
} // namespace Olympe
