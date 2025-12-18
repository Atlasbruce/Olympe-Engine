/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Singleton class that manages game resources (textures, sprites, sounds,
  animations, levels, navigation/collision maps, game object data, etc.)
- Loads resources on demand and keeps them in memory until released or
  the engine shuts down. Provides categorized listings and safe unload.
- All resource metadata files are expected to be JSON (parsing helpers can
  be added later). This implementation focuses on texture loading and the
  generic resource lifetime management with placeholders for extended types.

Notes:
- This implementation uses SDL for texture loading (BMP via SDL_LoadBMP to
  avoid additional image dependencies). It stores SDL_Texture* in the
  Resource struct. Extend loading functions to support PNG/JPEG/OGG/etc
  using the appropriate libraries when available.
*/

#include "DataManager.h"
#include "GameEngine.h"
#include "system/system_utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cerrno>
#include "sdl3_image/sdl_image.h"
#include "ECS_Components.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

DataManager::DataManager()
{
    name = "DataManager";
    SYSTEM_LOG << "DataManager created\n";
}
//-------------------------------------------------------------
DataManager::~DataManager()
{
    SYSTEM_LOG << "DataManager destroyed\n";
    // Ensure resources are freed if shutdown wasn't explicitly called
    UnloadAll();
}
//-------------------------------------------------------------
DataManager& DataManager::GetInstance()
{
    static DataManager instance;
    return instance;
}
//-------------------------------------------------------------
void DataManager::Initialize()
{
    // Placeholder for initialization logic (e.g. preload system icons)
    SYSTEM_LOG << "DataManager Initialized\n";
}
//-------------------------------------------------------------
void DataManager::Shutdown()
{
    SYSTEM_LOG << "DataManager Shutdown - unloading all resources\n";
    UnloadAll();
}
//-------------------------------------------------------------
bool DataManager::PreloadTexture(const std::string& id, const std::string& path, ResourceCategory category)
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    if (id.empty() || path.empty()) return false;
    if (m_resources_.find(id) != m_resources_.end())
    {
        // already loaded
        return true;
    }

    // try to load BMP surface first (no external deps required)
    SDL_Surface* surf = IMG_Load(path.c_str()); //SDL_LoadBMP(path.c_str());

    if (!surf)
    {
        SYSTEM_LOG << "DataManager::PreloadTexture IMG_Load failed for '" << path << "' : " << SDL_GetError() << "\n";
        return false;
    }

    SDL_Renderer* renderer = GameEngine::renderer;
    SDL_Texture* tex = nullptr;
    if (renderer)
    {
        tex = SDL_CreateTextureFromSurface(renderer, surf);
        if (!tex)
        {
            SYSTEM_LOG << "DataManager::PreloadTexture SDL_CreateTextureFromSurface failed for '" << path << "' : " << SDL_GetError() << "\n";
            SDL_DestroySurface(surf);
            return false;
        }
    }
    else
    {
        // no global renderer available: keep the surface as raw data pointer for future conversion
        // store surface pointer in Resource::data
    }

    auto res = std::make_shared<Resource>();
    res->type = ResourceType::Texture;
    res->category = category;
    res->id = id;
    res->path = path;
    res->sprite_texture = tex;
    if (!tex)
    {
        // store surface pointer for deferred texture creation
        res->data = surf;
    }
    else
    {
        // we no longer need the surface
        SDL_DestroySurface(surf);
    }

    m_resources_.emplace(id, res);
    SYSTEM_LOG << "DataManager: Loaded texture '" << id << "' from '" << path << "'\n";
    return true;
}
//-------------------------------------------------------------
bool DataManager::PreloadSprite(const std::string& id, const std::string& path, ResourceCategory category)
{
	return PreloadTexture(id, path, category);
}
//-------------------------------------------------------------
SDL_Texture* DataManager::GetTexture(const std::string& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    auto it = m_resources_.find(id);
    if (it == m_resources_.end()) return nullptr;
    auto res = it->second;
    if (res->sprite_texture) return res->sprite_texture;

    // If texture not created yet but we have a surface stored, try to create it now
    if (res->data)
    {
        SDL_Surface* surf = reinterpret_cast<SDL_Surface*>(res->data);
        SDL_Renderer* renderer = GameEngine::renderer;
        if (surf && renderer)
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            if (tex)
            {
                res->sprite_texture = tex;
                // we can free the surface now
                SDL_DestroySurface(surf);
                res->data = nullptr;
                return res->sprite_texture;
            }
            else
            {
                SYSTEM_LOG << "DataManager: Failed to create deferred texture for '" << id << "' : " << SDL_GetError() << "\n";
            }
        }
    }

    return nullptr;
}
//-------------------------------------------------------------
Sprite* DataManager::GetSprite(const std::string& id, const std::string& path, ResourceCategory category)
{
    // Optimized: Check existence without locking twice
    {
        std::lock_guard<std::mutex> lock(m_mutex_);
        auto it = m_resources_.find(id);
        if (it != m_resources_.end() && it->second->sprite_texture) {
            return it->second->sprite_texture;
        }
    }
    
    // Not found or texture not ready, try to load it
    if (PreloadSprite(id, path, category))
    {
        return GetTexture(id);
    }
    return nullptr;
}
//-------------------------------------------------------------
VisualSprite_data* DataManager::GetSprite_data(const std::string& id, const std::string& path)
{
    static VisualSprite_data vsprite;
	vsprite.sprite = GetSprite(id, path, ResourceCategory::GameEntity);
	// set srcRect based on texture size
	if (vsprite.sprite)
	{
		int w = ((SDL_Texture*)vsprite.sprite)->h, h = ((SDL_Texture*)vsprite.sprite)->h;
		vsprite.srcRect = { 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) };
		vsprite.hotSpot = Vector(w / 2.0f, h / 2.0f, 0.0f);
		return &vsprite;
	}

	SYSTEM_LOG << "DataManager: GetSprite_data failed for '" << id << "' file/path '" << path << "' does not exists or is incorrect\n";

    return nullptr;
}
//-------------------------------------------------------------
bool DataManager::ReleaseResource(const std::string& id)
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    auto it = m_resources_.find(id);
    if (it == m_resources_.end()) return false;
    auto res = it->second;

    if (res->sprite_texture)
    {
        SDL_DestroyTexture(res->sprite_texture);
        res->sprite_texture = nullptr;
    }
    if (res->data)
    {
        // if it was a surface store, free it
        SDL_Surface* surf = reinterpret_cast<SDL_Surface*>(res->data);
        if (surf) SDL_DestroySurface(surf);
        res->data = nullptr;
    }

    m_resources_.erase(it);
    SYSTEM_LOG << "DataManager: Released resource '" << id << "'\n";
    return true;
}
//-------------------------------------------------------------
void DataManager::UnloadAll()
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    for (auto& kv : m_resources_)
    {
        auto res = kv.second;
        if (res->sprite_texture)
        {
            SDL_DestroyTexture(res->sprite_texture);
            res->sprite_texture = nullptr;
        }
        if (res->data)
        {
            SDL_Surface* surf = reinterpret_cast<SDL_Surface*>(res->data);
            if (surf) SDL_DestroySurface(surf);
            res->data = nullptr;
        }
    }
    m_resources_.clear();
}
//-------------------------------------------------------------
bool DataManager::HasResource(const std::string& id) const
{
    std::lock_guard<std::mutex> lock(m_mutex_);
    return m_resources_.find(id) != m_resources_.end();
}
//-------------------------------------------------------------
std::vector<std::string> DataManager::ListResourcesByType(ResourceType type) const
{
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(m_mutex_);
    // Reserve capacity to avoid reallocations
    out.reserve(m_resources_.size());
    for (const auto& kv : m_resources_)
    {
        if (kv.second->type == type) out.push_back(kv.first);
    }
    return out;
}
//-------------------------------------------------------------
std::vector<std::string> DataManager::ListResourcesByCategory(ResourceCategory category) const
{
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(m_mutex_);
    // Reserve capacity to avoid reallocations
    out.reserve(m_resources_.size());
    for (const auto& kv : m_resources_)
    {
        if (kv.second->category == category) out.push_back(kv.first);
    }
    return out;
}
//-------------------------------------------------------------
// Build standard game data path: ./Gamedata/{videogameName}/{objectName}.json
std::string DataManager::BuildGameDataPath(const std::string& videogameName, const std::string& objectName)
{
    std::string game = videogameName.empty() ? std::string("default") : videogameName;
    std::string obj = objectName.empty() ? std::string("object") : objectName;
    std::string path = std::string(".") + "/Gamedata/" + game + "/" + obj + ".json";
    return path;
}
//-------------------------------------------------------------
bool DataManager::SaveTextFile(const std::string& filepath, const std::string& content) const
{
    if (filepath.empty()) return false;
    // ensure directory exists
    auto pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
    {
        std::string dir = filepath.substr(0, pos);
        if (!EnsureDirectoryExists(dir))
        {
            SYSTEM_LOG << "DataManager: Failed to ensure directory exists for '" << dir << "'\n";
            // continue attempt to write; fallthrough may fail
        }
    }

    std::ofstream ofs(filepath.c_str(), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    return ofs.good();
}
//-------------------------------------------------------------
bool DataManager::LoadTextFile(const std::string& filepath, std::string& outContent) const
{
    outContent.clear();
    if (filepath.empty()) return false;
    std::ifstream ifs(filepath.c_str(), std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    outContent = ss.str();
    return true;
}
//-------------------------------------------------------------
bool DataManager::SaveJSONForObject(const std::string& videogameName, const std::string& objectName, const std::string& jsonContent) const
{
    std::string path = BuildGameDataPath(videogameName, objectName);
    return SaveTextFile(path, jsonContent);
}
//-------------------------------------------------------------
bool DataManager::LoadJSONForObject(const std::string& videogameName, const std::string& objectName, std::string& outJson) const
{
    std::string path = BuildGameDataPath(videogameName, objectName);
    return LoadTextFile(path, outJson);
}
//-------------------------------------------------------------
bool DataManager::EnsureDirectoryExists(const std::string& dirpath) const
{
    if (dirpath.empty()) return false;

    std::string path = dirpath;
    // normalize separators to '/'
    std::replace(path.begin(), path.end(), '\\', '/');
    // remove trailing slash if present
    if (!path.empty() && path.back() == '/') path.pop_back();
    if (path.empty()) return true;

    // iterate and create each subpath
    std::string accum;
    size_t pos = 0;
    // if path starts with '/', keep it (unix absolute)
    if (!path.empty() && path[0] == '/') { accum = "/"; pos = 1; }

    while (true)
    {
        size_t next = path.find('/', pos);
        std::string part = (next == std::string::npos) ? path.substr(pos) : path.substr(pos, next - pos);
        if (!accum.empty() && accum.back() != '/') accum += '/';
        accum += part;

        // attempt to create directory
        #ifdef _WIN32
        int r = _mkdir(accum.c_str());
        #else
        int r = mkdir(accum.c_str(), 0755);
        #endif
        if (r != 0)
        {
            if (errno == EEXIST)
            {
                // already exists - ok
            }
            else
            {
                // failed for other reason
                SYSTEM_LOG << "DataManager: mkdir failed for '" << accum << "' (errno=" << errno << ")\n";
                return false;
            }
        }

        if (next == std::string::npos) break;
        pos = next + 1;
    }

    return true;
}
//-------------------------------------------------------------
bool DataManager::PreloadSystemResources(const std::string& configFilePath)
{
    std::string content;
    if (!LoadTextFile(configFilePath, content))
    {
        SYSTEM_LOG << "DataManager: PreloadSystemResources failed to read '" << configFilePath << "'\n";
        return false;
    }

    try
    {
        nlohmann::json root = nlohmann::json::parse(content);
        if (!root.contains("system_resources")) return true; // nothing to do
        const auto& arr = root["system_resources"];
        if (!arr.is_array()) return false;
        for (size_t i = 0; i < arr.size(); ++i)
        {
            const auto& item = arr[i];
            if (!item.is_object()) continue;
            std::string id = item.contains("id") ? item["id"].get<std::string>() : std::string();
            std::string path = item.contains("path") ? item["path"].get<std::string>() : std::string();
            std::string type = item.contains("type") ? item["type"].get<std::string>() : std::string();
            if (id.empty() || path.empty()) continue;
            if (type == "texture")
            {
                PreloadTexture(id, path, ResourceCategory::Level);
            }
            else
                if (type == "sprite" || type == "animation")
                {
                    PreloadSprite(id, path, ResourceCategory::GameEntity);
				}
                else
                {
                    PreloadTexture(id, path, ResourceCategory::System);
                }
        }
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "DataManager: JSON parse error in PreloadSystemResources: " << e.what() << "\n";
        return false;
    }

    return true;
}
