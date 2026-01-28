/*
 * Olympe Tilemap Editor - Level Manager Implementation
 */

#include "../include/LevelManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

namespace Olympe {
namespace Editor {

    // ========================================================================
    // JSON Conversion Helpers
    // ========================================================================

    // Vector serialization (for EntityInstance.position which is now a Vector)
    void to_json(json& j, const Vector& v)
    {
        j = json::object();
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
    }

    void from_json(const json& j, Vector& v)
    {
        if (j.contains("x")) v.x = j["x"].get<float>();
        if (j.contains("y")) v.y = j["y"].get<float>();
        if (j.contains("z")) v.z = j["z"].get<float>();
        else v.z = 0.0f;  // Default z to 0 for 2D compatibility
    }

    void to_json(json& j, const EntityInstance& e)
    {
        j = json::object();
        j["id"] = e.id;
        j["prefabPath"] = e.prefabPath;
        j["name"] = e.name;
        j["type"] = e.type;
        j["rotation"] = e.rotation;
        if (!e.spritePath.empty()) {
            j["spritePath"] = e.spritePath;
        }
        json posJson;
        to_json(posJson, e.position);
        j["position"] = posJson;
        if (!e.overrides.is_null() && !e.overrides.empty())
        {
            j["overrides"] = e.overrides;
        }
        else
        {
            j["overrides"] = json::object();
        }
    }

    void from_json(const json& j, EntityInstance& e)
    {
        if (j.contains("id")) e.id = j["id"].get<std::string>();
        if (j.contains("prefabPath")) e.prefabPath = j["prefabPath"].get<std::string>();
        if (j.contains("name")) e.name = j["name"].get<std::string>();
        if (j.contains("type")) e.type = j["type"].get<std::string>();
        if (j.contains("rotation")) e.rotation = j["rotation"].get<float>();
        if (j.contains("spritePath")) e.spritePath = j["spritePath"].get<std::string>();
        if (j.contains("position")) from_json(j["position"], e.position);
        if (j.contains("overrides")) e.overrides = j["overrides"];
        else e.overrides = json::object();
    }

    // ========================================================================
    // LevelManager Implementation
    // ========================================================================

    LevelManager::LevelManager()
        : m_hasUnsavedChanges(false), m_nextEntityId(1)
    {
    }

    LevelManager::~LevelManager()
    {
    }

    std::string LevelManager::GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;

#ifdef _MSC_VER
        std::tm timeinfo;
        localtime_s(&timeinfo, &time);
        ss << std::put_time(&timeinfo, "%Y-%m-%dT%H:%M:%S");
#else
        ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
#endif

        return ss.str();
    }

    std::string LevelManager::GenerateUniqueEntityId()
    {
        std::stringstream ss;
        ss << "entity_" << m_nextEntityId++;
        return ss.str();
    }

    void LevelManager::NewLevel(const std::string& name)
    {
        m_levelDef = LevelDefinition();
        m_levelDef.name = name;
        m_levelDef.levelName = name;
        m_levelDef.metadata.author = "OlympeTilemapEditor";
        m_levelDef.metadata.created = GetCurrentTimestamp();
        m_levelDef.metadata.lastModified = GetCurrentTimestamp();
        
        // Initialize tile and collision maps (default 32x32)
        ResizeTileMap(32, 32);
        ResizeCollisionMap(32, 32);

        m_currentPath = "";
        m_hasUnsavedChanges = true;
        m_nextEntityId = 1;
    }

    bool LevelManager::LoadLevel(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "[LevelManager] Failed to open file: " << path << std::endl;
            return false;
        }

        try
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            
            json j = json::parse(buffer.str());

            if (!DeserializeFromJson(j))
            {
                std::cerr << "[LevelManager] Failed to deserialize level data" << std::endl;
                return false;
            }

            m_currentPath = path;
            m_hasUnsavedChanges = false;
            
            std::cout << "[LevelManager] Successfully loaded level: " << path << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[LevelManager] Exception loading level: " << e.what() << std::endl;
            return false;
        }
    }

    bool LevelManager::SaveLevel(const std::string& path)
    {
        try
        {
            json j;
            SerializeToJson(j);

            std::ofstream file(path);
            if (!file.is_open())
            {
                std::cerr << "[LevelManager] Failed to open file for writing: " << path << std::endl;
                return false;
            }

            file << j.dump(2);
            file.close();

            m_currentPath = path;
            m_hasUnsavedChanges = false;
            m_levelDef.metadata.lastModified = GetCurrentTimestamp();

            std::cout << "[LevelManager] Successfully saved level: " << path << std::endl;
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[LevelManager] Exception saving level: " << e.what() << std::endl;
            return false;
        }
    }

    void LevelManager::SerializeToJson(json& j) const
    {
        j = json::object();
        j["schema_version"] = m_levelDef.schema_version;
        j["type"] = m_levelDef.type;
        j["blueprintType"] = m_levelDef.blueprintType;
        j["name"] = m_levelDef.name;
        j["description"] = m_levelDef.description;

        // Metadata
        j["metadata"] = json::object();
        j["metadata"]["author"] = m_levelDef.metadata.author;
        j["metadata"]["created"] = m_levelDef.metadata.created;
        j["metadata"]["lastModified"] = m_levelDef.metadata.lastModified;
        j["metadata"]["tags"] = json::array();
        for (const auto& tag : m_levelDef.metadata.tags)
        {
            j["metadata"]["tags"].push_back(tag);
        }

        // Editor state
        j["editorState"] = json::object();
        j["editorState"]["zoom"] = m_levelDef.editorState.zoom;
        json scrollOffsetJson;
        to_json(scrollOffsetJson, m_levelDef.editorState.scrollOffset);
        j["editorState"]["scrollOffset"] = scrollOffsetJson;

        // Level data
        j["data"] = json::object();
        j["data"]["levelName"] = m_levelDef.levelName;
        json worldSizeJson;
        to_json(worldSizeJson, m_levelDef.worldSize);
        j["data"]["worldSize"] = worldSizeJson;
        j["data"]["backgroundMusic"] = m_levelDef.backgroundMusic;
        j["data"]["ambientColor"] = m_levelDef.ambientColor;

        // Entities
        j["data"]["entities"] = json::array();
        for (const auto& entity : m_levelDef.entities)
        {
            json entityJson;
            to_json(entityJson, *entity);
            j["data"]["entities"].push_back(entityJson);
        }

        // Tile map
        j["data"]["tileMap"] = json::array();
        for (const auto& row : m_levelDef.tileMap)
        {
            json rowJson = json::array();
            for (int tile : row)
            {
                rowJson.push_back(tile);
            }
            j["data"]["tileMap"].push_back(rowJson);
        }

        // Collision map
        j["data"]["collisionMap"] = json::array();
        for (const auto& row : m_levelDef.collisionMap)
        {
            json rowJson = json::array();
            for (uint8_t collision : row)
            {
                rowJson.push_back(static_cast<int>(collision));
            }
            j["data"]["collisionMap"].push_back(rowJson);
        }
    }

    bool LevelManager::DeserializeFromJson(const json& j)
    {
        try
        {
            m_levelDef = LevelDefinition();

            if (j.contains("schema_version"))
                m_levelDef.schema_version = j["schema_version"].get<int>();
            
            if (m_levelDef.schema_version != 2)
            {
                std::cerr << "[LevelManager] Unsupported schema version: " << m_levelDef.schema_version << std::endl;
            }

            if (j.contains("type"))
                m_levelDef.type = j["type"].get<std::string>();
            if (j.contains("blueprintType"))
                m_levelDef.blueprintType = j["blueprintType"].get<std::string>();
            if (j.contains("name"))
                m_levelDef.name = j["name"].get<std::string>();
            if (j.contains("description"))
                m_levelDef.description = j["description"].get<std::string>();

            // Metadata
            if (j.contains("metadata"))
            {
                const auto& metadata = j["metadata"];
                if (metadata.contains("author"))
                    m_levelDef.metadata.author = metadata["author"].get<std::string>();
                if (metadata.contains("created"))
                    m_levelDef.metadata.created = metadata["created"].get<std::string>();
                if (metadata.contains("lastModified"))
                    m_levelDef.metadata.lastModified = metadata["lastModified"].get<std::string>();
                if (metadata.contains("tags") && metadata["tags"].is_array())
                {
                    const auto& tagsArray = metadata["tags"];
                    for (size_t i = 0; i < tagsArray.size(); ++i)
                    {
                        m_levelDef.metadata.tags.push_back(tagsArray[i].get<std::string>());
                    }
                }
            }

            // Editor state
            if (j.contains("editorState"))
            {
                const auto& editorState = j["editorState"];
                if (editorState.contains("zoom"))
                    m_levelDef.editorState.zoom = editorState["zoom"].get<double>();
                if (editorState.contains("scrollOffset"))
                    from_json(editorState["scrollOffset"], m_levelDef.editorState.scrollOffset);
            }

            // Level data
            if (j.contains("data"))
            {
                const auto& data = j["data"];
                if (data.contains("levelName"))
                    m_levelDef.levelName = data["levelName"].get<std::string>();
                if (data.contains("worldSize"))
                    from_json(data["worldSize"], m_levelDef.worldSize);
                if (data.contains("backgroundMusic"))
                    m_levelDef.backgroundMusic = data["backgroundMusic"].get<std::string>();
                if (data.contains("ambientColor"))
                    m_levelDef.ambientColor = data["ambientColor"].get<std::string>();

                // Entities
                if (data.contains("entities") && data["entities"].is_array())
                {
                    const auto& entitiesArray = data["entities"];
                    for (size_t i = 0; i < entitiesArray.size(); ++i)
                    {
                        auto entity = std::make_unique<EntityInstance>();
                        from_json(entitiesArray[i], *entity);
                        
                        // Update entity ID counter
                        if (entity->id.find("entity_") == 0)
                        {
                            int idNum = std::stoi(entity->id.substr(7));
                            m_nextEntityId = std::max(m_nextEntityId, idNum + 1);
                        }
                        
                        m_levelDef.entities.push_back(std::move(entity));
                    }
                }

                // Tile map
                if (data.contains("tileMap") && data["tileMap"].is_array())
                {
                    m_levelDef.tileMap.clear();
                    const auto& tileMapArray = data["tileMap"];
                    for (size_t y = 0; y < tileMapArray.size(); ++y)
                    {
                        std::vector<int> row;
                        const auto& rowJson = tileMapArray[y];
                        if (rowJson.is_array())
                        {
                            for (size_t x = 0; x < rowJson.size(); ++x)
                            {
                                row.push_back(rowJson[x].get<int>());
                            }
                        }
                        m_levelDef.tileMap.push_back(row);
                    }
                }

                // Collision map
                if (data.contains("collisionMap") && data["collisionMap"].is_array())
                {
                    m_levelDef.collisionMap.clear();
                    const auto& collisionMapArray = data["collisionMap"];
                    for (size_t y = 0; y < collisionMapArray.size(); ++y)
                    {
                        std::vector<uint8_t> row;
                        const auto& rowJson = collisionMapArray[y];
                        if (rowJson.is_array())
                        {
                            for (size_t x = 0; x < rowJson.size(); ++x)
                            {
                                row.push_back(static_cast<uint8_t>(rowJson[x].get<int>()));
                            }
                        }
                        m_levelDef.collisionMap.push_back(row);
                    }
                }
            }

            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[LevelManager] Exception during deserialization: " << e.what() << std::endl;
            return false;
        }
    }

    // ========================================================================
    // Entity Management
    // ========================================================================

    EntityInstance* LevelManager::CreateEntity(const std::string& prefabPath)
    {
        auto entity = std::make_unique<EntityInstance>();
        entity->id = GenerateUniqueEntityId();
        entity->prefabPath = prefabPath;
        entity->name = "New Entity";
        entity->position = Vector();
        entity->overrides = json::object();

        EntityInstance* entityPtr = entity.get();
        m_levelDef.entities.push_back(std::move(entity));
        
        m_hasUnsavedChanges = true;
        return entityPtr;
    }

    void LevelManager::DeleteEntity(const std::string& id)
    {
        auto it = std::remove_if(m_levelDef.entities.begin(), m_levelDef.entities.end(),
            [&id](const std::unique_ptr<EntityInstance>& entity) {
                return entity->id == id;
            });

        if (it != m_levelDef.entities.end())
        {
            m_levelDef.entities.erase(it, m_levelDef.entities.end());
            m_hasUnsavedChanges = true;
        }
    }

    EntityInstance* LevelManager::GetEntity(const std::string& id) const
    {
        for (const auto& entity : m_levelDef.entities)
        {
            if (entity->id == id)
            {
                return entity.get();
            }
        }
        return nullptr;
    }

    std::vector<EntityInstance*> LevelManager::GetAllEntities()
    {
        std::vector<EntityInstance*> result;
        for (const auto& entity : m_levelDef.entities)
        {
            result.push_back(entity.get());
        }
        return result;
    }

    bool LevelManager::UpdateEntityPosition(const std::string& id, const Vector& position)
    {
        EntityInstance* entity = GetEntity(id);
        if (entity)
        {
            entity->position = position;
            m_hasUnsavedChanges = true;
            return true;
        }
        return false;
    }

    // ========================================================================
    // Tile Management
    // ========================================================================

    void LevelManager::SetTile(int x, int y, int tileId)
    {
        if (y >= 0 && y < static_cast<int>(m_levelDef.tileMap.size()) &&
            x >= 0 && x < static_cast<int>(m_levelDef.tileMap[y].size()))
        {
            m_levelDef.tileMap[y][x] = tileId;
            m_hasUnsavedChanges = true;
        }
    }

    int LevelManager::GetTile(int x, int y) const
    {
        if (y >= 0 && y < static_cast<int>(m_levelDef.tileMap.size()) &&
            x >= 0 && x < static_cast<int>(m_levelDef.tileMap[y].size()))
        {
            return m_levelDef.tileMap[y][x];
        }
        return -1;
    }

    void LevelManager::ResizeTileMap(int width, int height)
    {
        m_levelDef.tileMap.clear();
        m_levelDef.tileMap.resize(height);
        for (int y = 0; y < height; ++y)
        {
            m_levelDef.tileMap[y].resize(width, 0);
        }
        m_hasUnsavedChanges = true;
    }

    // ========================================================================
    // Collision Management
    // ========================================================================

    void LevelManager::SetCollision(int x, int y, uint8_t mask)
    {
        if (y >= 0 && y < static_cast<int>(m_levelDef.collisionMap.size()) &&
            x >= 0 && x < static_cast<int>(m_levelDef.collisionMap[y].size()))
        {
            m_levelDef.collisionMap[y][x] = mask;
            m_hasUnsavedChanges = true;
        }
    }

    uint8_t LevelManager::GetCollision(int x, int y) const
    {
        if (y >= 0 && y < static_cast<int>(m_levelDef.collisionMap.size()) &&
            x >= 0 && x < static_cast<int>(m_levelDef.collisionMap[y].size()))
        {
            return m_levelDef.collisionMap[y][x];
        }
        return 0;
    }

    void LevelManager::ResizeCollisionMap(int width, int height)
    {
        m_levelDef.collisionMap.clear();
        m_levelDef.collisionMap.resize(height);
        for (int y = 0; y < height; ++y)
        {
            m_levelDef.collisionMap[y].resize(width, 0);
        }
        m_hasUnsavedChanges = true;
    }

} // namespace Editor
} // namespace Olympe
