/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- DataManager is a singleton responsible for loading, caching and
  releasing game resources (textures, sprites, animations, sounds,
  level data, navigation/collision maps, game object data, etc.).
- It provides simple file-based JSON save/load helpers used by
  VideoGame/GameObject and related systems to persist runtime data.
- Resources are categorized by type and category so calling code can
  list and query resources by semantic groups.

Notes:
- Resource loading functions in this initial implementation focus on
  textures (BMP via SDL). Extend to support PNG/JPEG/OGG/etc. with
  appropriate libraries as needed.
- JSON serialization of complex objects is expected to be done by
  calling code; DataManager provides file IO helpers and a directory
  layout convention: "./Gamedata/{videogameName}/{objectName}.json".
*/

#pragma once

#include "object.h"
#include "system/system_utils.h"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "third_party/nlohmann/json.hpp"

using Sprite = SDL_Texture;

struct VisualSprite_data;

// Catégories et types de ressources
enum class ResourceType : uint32_t
{
    Unknown = 0,
    Texture,
    Sprite,
    Animation,
    Sound,
    Music,
    FX,
    Level,
    Sector,
    NavMap,
    CollisionMap,
    GameObjectData
};

enum class ResourceCategory : uint32_t
{
    System = 0,   // engine-level data
    GameObject,   // data related to interactive objects
    Level         // level / map data
};

// Generic resource container
struct Resource
{
    ResourceType type = ResourceType::Unknown;
    ResourceCategory category = ResourceCategory::System;
    std::string id;   // logical identifier
    std::string path; // filesystem path

    // data payloads depending on the resource type
    Sprite* sprite_texture = nullptr; // for texture/sprite resources
    void* data = nullptr;           // generic pointer for deferred objects

    Resource() = default;
    ~Resource() = default;
};

class DataManager : public Object
{
public:
    DataManager();
    virtual ~DataManager();

    virtual ObjectType GetObjectType() const override { return ObjectType::Singleton; }

    // Singleton access
    static DataManager& GetInstance();
    static DataManager& Get() { return GetInstance(); }

    void Initialize();
    void Shutdown();

    // Texture loading / retrieval / release
    bool PreloadTexture(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::System);
	bool PreloadSprite(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::GameObject);
    Sprite* GetTexture(const std::string& id) const;
    Sprite* GetSprite(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::GameObject);
    VisualSprite_data* GetSprite_data(const std::string& id, const std::string& path);
    bool ReleaseResource(const std::string& id);


    // Resource helpers
    void UnloadAll();
    bool HasResource(const std::string& id) const;

    std::vector<std::string> ListResourcesByType(ResourceType type) const;
    std::vector<std::string> ListResourcesByCategory(ResourceCategory category) const;

    // JSON file helpers
    // Save JSON content for an object inside the videogame folder.
    // Path used: "./Gamedata/{videogameName}/{objectName}.json"
    bool SaveJSONForObject(const std::string& videogameName, const std::string& objectName, const std::string& jsonContent) const;
    bool LoadJSONForObject(const std::string& videogameName, const std::string& objectName, std::string& outJson) const;

    // Generic file helpers
    bool SaveTextFile(const std::string& filepath, const std::string& content) const;
    bool LoadTextFile(const std::string& filepath, std::string& outContent) const;

    // Ensure directory exists (creates intermediate dirs if necessary)
    bool EnsureDirectoryExists(const std::string& dirpath) const;

    // Helper to build the standard game data path
    static std::string BuildGameDataPath(const std::string& videogameName, const std::string& objectName);

    // Preload system resources from a configuration JSON file (e.g. "olympe.ini")
    // Expected format:
    // { "system_resources": [ { "id":"ui_icon", "path":"assets/ui/icon.bmp", "type":"texture" }, ... ] }
    bool PreloadSystemResources(const std::string& configFilePath);

private:
    mutable std::mutex m_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> m_resources_;
};
