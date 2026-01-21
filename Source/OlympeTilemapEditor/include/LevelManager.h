/*
 * Olympe Tilemap Editor - Level Manager
 * 
 * Manages level data including entities, tiles, and collisions.
 * Handles loading/saving LevelDefinition JSON format compatible with Olympe Engine.
 */

#pragma once

#include "../../third_party/nlohmann/json.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

using json = nlohmann::json;

namespace Olympe {
namespace Editor {

    // Represents a 2D position/vector
    struct Vec2
    {
        double x;
        double y;

        Vec2() : x(0.0), y(0.0) {}
        Vec2(double x_, double y_) : x(x_), y(y_) {}
    };

    // Represents an entity instance in the level
    struct EntityInstance
    {
        std::string id;              // Unique identifier
        std::string prefabPath;      // Path to the entity prefab blueprint
        std::string name;            // Display name
		std::string type;            // Entity type (e.g., "Player", "Enemy", "Item", "Collision"...)
		std::string spritePath = ""; // Optional sprite path for visual representation
        Vec2 position;               // World position
        json overrides;              // Component property overrides

        EntityInstance()
            : id(""), prefabPath(""), name(""), position(), overrides(json::object()) {}
    };

    // Represents level metadata
    struct LevelMetadata
    {
        std::string author;
        std::string created;
        std::string lastModified;
        std::vector<std::string> tags;
        nlohmann::json customData; // For storing additional metadata like parallax layers

        LevelMetadata()
            : author(""), created(""), lastModified(""), customData(json::object()) {}
    };

    // Represents editor-specific state
    struct EditorStateData
    {
        double zoom;
        Vec2 scrollOffset;

        EditorStateData()
            : zoom(0.5), scrollOffset(0, 0) {}
    };

    // Main level definition structure
    struct LevelDefinition
    {
        int schema_version;
        std::string type;
        std::string blueprintType;
        std::string name;
        std::string description;

        LevelMetadata metadata;
        EditorStateData editorState;

        // Level data
        std::string levelName;
        Vec2 worldSize;
        std::string backgroundMusic;
        std::string ambientColor;

        std::vector<std::unique_ptr<EntityInstance>> entities;
        std::vector<std::vector<int>> tileMap;      // 2D grid of tile IDs
        std::vector<std::vector<uint8_t>> collisionMap;  // 2D grid of collision masks

        // Visual layers (Pass 1)
        struct VisualLayer {
            std::string name;
            int zOrder;
            bool isParallax;
            std::string imagePath;
            float scrollFactorX;
            float scrollFactorY;
            float offsetX;
            float offsetY;
            bool repeatX;
            bool repeatY;
            float opacity;
            uint32_t tintColor;
            bool visible;
            
            VisualLayer()
                : zOrder(0), isParallax(false), scrollFactorX(1.0f), scrollFactorY(1.0f),
                  offsetX(0.0f), offsetY(0.0f), repeatX(false), repeatY(false),
                  opacity(1.0f), tintColor(0xFFFFFFFF), visible(true) {}
        };
        std::vector<VisualLayer> visualLayers;
        
        // Tile layer definitions (Pass 1)
        struct TileLayerDef {
            std::string name;
            int zOrder;
            std::vector<std::vector<int>> tiles;  // [y][x] = tileGID
            std::vector<std::vector<uint8_t>> tileFlipFlags;  // [y][x] = flip flags (H=0x1, V=0x2, D=0x4)
            float opacity;
            bool visible;
            
            // Chunk support for infinite maps
            struct Chunk {
                int x, y;  // Chunk coordinates (in tiles)
                int width, height;  // Chunk dimensions (in tiles)
                std::vector<std::vector<int>> tiles;  // [y][x] = tileGID
                std::vector<std::vector<uint8_t>> tileFlipFlags;  // [y][x] = flip flags
                
                Chunk() : x(0), y(0), width(0), height(0) {}
            };
            std::vector<Chunk> chunks;  // For infinite maps
            bool isInfinite;  // Whether this layer uses chunks
            
            TileLayerDef()
                : zOrder(0), opacity(1.0f), visible(true), isInfinite(false) {}
        };
        std::vector<TileLayerDef> tileLayers;
        
        // Spatial structures (Pass 2)
        struct SectorDef {
            std::string name;
            std::string type;
            std::vector<Vec2> polygon;
            Vec2 position;
            nlohmann::json properties;
            
            SectorDef()
                : position(0, 0), properties(json::object()) {}
        };
        std::vector<SectorDef> sectors;
        
        struct CollisionShape {
            std::string name;
            enum Type { Rectangle, Polygon, Polyline } type;
            Vec2 position;
            Vec2 size;  // for rectangles
            std::vector<Vec2> points;  // for polygons/polylines
            
            CollisionShape()
                : type(Rectangle), position(0, 0), size(0, 0) {}
        };
        std::vector<CollisionShape> collisionShapes;
        
        // Object categorization (Pass 3, 4)
        struct ObjectCategory {
            std::vector<std::unique_ptr<EntityInstance>> staticObjects;    // items, waypoints, triggers
            std::vector<std::unique_ptr<EntityInstance>> dynamicObjects;   // player, NPCs, enemies
            std::vector<std::unique_ptr<EntityInstance>> patrolPaths;      // AI patrol paths
            std::vector<std::unique_ptr<EntityInstance>> soundObjects;     // ambient sounds, music
        };
        ObjectCategory categorizedObjects;
        
        // Relationships (Pass 5)
        struct ObjectLink {
            std::string sourceObjectName;
            int sourceObjectId;
            std::string targetObjectName;
            int targetObjectId;
            std::string linkType;  // "patrol_path", "trigger_target", "ai_target"
            nlohmann::json linkData;
            
            ObjectLink()
                : sourceObjectId(0), targetObjectId(0), linkData(json::object()) {}
        };
        std::vector<ObjectLink> objectLinks;
        
        // Resource catalog
        struct ResourceCatalog {
            std::vector<std::string> tilesetPaths;
            std::vector<std::string> imagePaths;
            std::vector<std::string> audioPaths;
        };
        ResourceCatalog resources;
        
        // Map configuration
        struct MapConfig {
            std::string orientation;  // "orthogonal", "isometric", "staggered", "hexagonal"
            int tileWidth;
            int tileHeight;
            int mapWidth;
            int mapHeight;
            std::string renderOrder;  // "right-down", "right-up", "left-down", "left-up"
            bool infinite;
            
            MapConfig()
                : tileWidth(0), tileHeight(0), mapWidth(0), mapHeight(0),
                  orientation("orthogonal"), renderOrder("right-down"), infinite(false) {}
        };
        MapConfig mapConfig;

        LevelDefinition()
            : schema_version(2), type("LevelDefinition"), blueprintType("LevelDefinition"),
              name(""), description(""), levelName(""), worldSize(1024, 768),
              backgroundMusic(""), ambientColor("#000000") {}
    };

    // LevelManager: Core class for level editing operations
    class LevelManager
    {
    public:
        LevelManager();
        ~LevelManager();

        // Level loading/saving
        bool LoadLevel(const std::string& path);
        bool SaveLevel(const std::string& path);
        void NewLevel(const std::string& name);

        // Entity management
        EntityInstance* CreateEntity(const std::string& prefabPath);
        void DeleteEntity(const std::string& id);
        EntityInstance* GetEntity(const std::string& id) const;
        std::vector<EntityInstance*> GetAllEntities();
        bool UpdateEntityPosition(const std::string& id, const Vec2& position);

        // Tile management
        void SetTile(int x, int y, int tileId);
        int GetTile(int x, int y) const;
        void ResizeTileMap(int width, int height);

        // Collision management
        void SetCollision(int x, int y, uint8_t mask);
        uint8_t GetCollision(int x, int y) const;
        void ResizeCollisionMap(int width, int height);

        // Level properties
        const LevelDefinition& GetLevelDefinition() const { return m_levelDef; }
        LevelDefinition& GetLevelDefinition() { return m_levelDef; }
        
        const std::string& GetCurrentLevelPath() const { return m_currentPath; }
        bool HasUnsavedChanges() const { return m_hasUnsavedChanges; }
        void MarkDirty() { m_hasUnsavedChanges = true; }
        void ClearDirty() { m_hasUnsavedChanges = false; }

    private:
        // JSON serialization helpers
        void SerializeToJson(json& j) const;
        bool DeserializeFromJson(const json& j);
        
        std::string GenerateUniqueEntityId();
        std::string GetCurrentTimestamp();

        LevelDefinition m_levelDef;
        std::string m_currentPath;
        bool m_hasUnsavedChanges;
        int m_nextEntityId;
    };

    // JSON conversion helpers
    void to_json(json& j, const Vec2& v);
    void from_json(const json& j, Vec2& v);
    void to_json(json& j, const EntityInstance& e);
    void from_json(const json& j, EntityInstance& e);

} // namespace Editor
} // namespace Olympe
