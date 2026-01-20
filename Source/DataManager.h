/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- DataManager is a singleton responsible for loading, caching and
  releasing game resources (textures, sprites, animations, sounds,
  level data, navigation/collision maps, game object data, etc.).
- It provides simple file-based JSON save/load helpers used by
  VideoGame/GameEntity and related systems to persist runtime data.
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

#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include <map>

using Sprite = SDL_Texture;

struct VisualSprite_data;
struct VisualEditor_data;

// Cat�gories et types de ressources
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
    GameEntityData
};

enum class ResourceCategory : uint32_t
{
    System = 0,   // engine-level data
    GameEntity,   // data related to interactive objects
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

class DataManager 
{
public:
    DataManager();
    virtual ~DataManager();

    // Singleton access
    static DataManager& GetInstance();
    static DataManager& Get() { return GetInstance(); }

    void Initialize();
    void Shutdown();

    // Texture loading / retrieval / release
    bool PreloadTexture(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::System);
	bool PreloadSprite(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::GameEntity);
    Sprite* GetTexture(const std::string& id) const;
    Sprite* GetSprite(const std::string& id, const std::string& path, ResourceCategory category = ResourceCategory::GameEntity);
    VisualSprite_data* GetSprite_data(const std::string& id, const std::string& path);
    VisualEditor_data* GetSpriteEditor_data(const std::string& id, const std::string& path);
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

    // ========================================================================
    // PHASE 2: Batch Preloading System (for 3-Phase Level Loading)
    // ========================================================================
    
    struct PreloadStats
    {
        int totalRequested;
        int successfullyLoaded;
        int failedWithFallback;
        int completelyFailed;
        std::vector<std::string> failedPaths;
        std::map<std::string, std::string> fallbackPaths;  // original -> actual

        PreloadStats() 
            : totalRequested(0), successfullyLoaded(0), 
              failedWithFallback(0), completelyFailed(0) {}
        
        bool IsSuccess() const { return completelyFailed == 0; }
        float GetSuccessRate() const 
        { 
            return totalRequested > 0 ? 
                static_cast<float>(successfullyLoaded + failedWithFallback) / totalRequested : 1.0f;
        }
    };
    
    struct TilesetInfo
    {
        std::string sourceFile;      // .tsj file path
        std::string imageFile;       // Main tileset image
        std::vector<std::string> individualImages;  // For collection tilesets
        bool isCollection;
        
        TilesetInfo() : isCollection(false) {}
    };
    
    struct LevelPreloadResult
    {
        bool success;
        PreloadStats sprites;
        PreloadStats textures;
        PreloadStats audio;
        PreloadStats tilesets;
        
        LevelPreloadResult() : success(false) {}
        
        int GetTotalLoaded() const
        {
            return sprites.successfullyLoaded + textures.successfullyLoaded + 
                   audio.successfullyLoaded + tilesets.successfullyLoaded +
                   sprites.failedWithFallback + textures.failedWithFallback +
                   audio.failedWithFallback + tilesets.failedWithFallback;
        }
        
        int GetTotalFailed() const
        {
            return sprites.completelyFailed + textures.completelyFailed + 
                   audio.completelyFailed + tilesets.completelyFailed;
        }
        
        bool IsComplete() const
        {
            return success && GetTotalFailed() == 0;
        }
    };
    
    // Batch preload methods
    PreloadStats PreloadTextures(const std::vector<std::string>& paths, 
                                  ResourceCategory category = ResourceCategory::Level,
                                  bool enableFallbackScan = true);
    
    PreloadStats PreloadSprites(const std::vector<std::string>& paths, 
                                ResourceCategory category = ResourceCategory::GameEntity,
                                bool enableFallbackScan = true);
    
    PreloadStats PreloadAudioFiles(const std::vector<std::string>& paths,
                                   bool enableFallbackScan = true);
    
    PreloadStats PreloadTilesets(const std::vector<TilesetInfo>& tilesets,
                                bool enableFallbackScan = true);
    
    // Fallback resource discovery
    std::string FindResourceRecursive(const std::string& filename, 
                                     const std::string& rootDir = "GameData") const;

private:
    std::string name;
    mutable std::mutex m_mutex_;
    std::unordered_map<std::string, std::shared_ptr<Resource>> m_resources_;
    bool m_enableFallbackScan = true;
    
    // Platform-specific recursive search helpers
#ifdef _WIN32
    std::string FindResourceRecursive_Windows(const std::string& filename, 
                                             const std::string& rootDir) const;
#else
    std::string FindResourceRecursive_Unix(const std::string& filename, 
                                          const std::string& rootDir) const;
#endif
};
