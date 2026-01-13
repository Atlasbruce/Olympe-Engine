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

        LevelMetadata()
            : author(""), created(""), lastModified("") {}
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
