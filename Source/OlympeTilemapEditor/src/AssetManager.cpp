/*
 * Olympe Tilemap Editor - Asset Manager Implementation
 */

#include "../include/AssetManager.h"
#include <iostream>
#include <algorithm>
#include <sys/stat.h>

namespace Olympe {
namespace Editor {

AssetManager::AssetManager(SDL_Renderer* renderer)
    : renderer_(renderer)
    , max_cache_size_(256 * 1024 * 1024) // 256 MB default
    , current_cache_size_(0)
{
    if (!renderer_) {
        std::cerr << "[AssetManager] Error: Renderer is null" << std::endl;
    }
    std::cout << "[AssetManager] Initialized with max cache size: " 
              << (max_cache_size_ / 1024 / 1024) << " MB" << std::endl;
}

AssetManager::~AssetManager()
{
    clear_cache();
    std::cout << "[AssetManager] Shutdown" << std::endl;
}

SDL_Texture* AssetManager::load_texture(const std::string& file_path)
{
    // Check if texture is already loaded
    auto it = texture_cache_.find(file_path);
    if (it != texture_cache_.end()) {
        std::cout << "[AssetManager] Texture already loaded: " << file_path << std::endl;
        return it->second->texture;
    }
    
    std::cout << "[AssetManager] Loading texture: " << file_path << std::endl;
    
    SDL_Texture* texture = load_texture_from_file(file_path);
    if (!texture) {
        std::cerr << "[AssetManager] Error: Failed to load texture: " << file_path << std::endl;
        return nullptr;
    }
    
    // Create texture asset
    auto asset = std::make_unique<TextureAsset>();
    asset->texture = texture;
    float width, height;
    SDL_GetTextureSize(texture, &width, &height);
    asset->width = static_cast<int>(width);
    asset->height = static_cast<int>(height);
    asset->file_path = file_path;
    asset->is_valid = true;
    
    // Store in cache
    SDL_Texture* result = asset->texture;
    texture_cache_[file_path] = std::move(asset);
    
    update_cache_size();
    
    return result;
}

const TextureAsset* AssetManager::get_texture(const std::string& file_path) const
{
    auto it = texture_cache_.find(file_path);
    if (it != texture_cache_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool AssetManager::unload_texture(const std::string& file_path)
{
    auto it = texture_cache_.find(file_path);
    if (it != texture_cache_.end()) {
        std::cout << "[AssetManager] Unloading texture: " << file_path << std::endl;
        if (it->second->texture) {
            SDL_DestroyTexture(it->second->texture);
        }
        texture_cache_.erase(it);
        update_cache_size();
        return true;
    }
    return false;
}

bool AssetManager::is_texture_loaded(const std::string& file_path) const
{
    return texture_cache_.find(file_path) != texture_cache_.end();
}

bool AssetManager::load_tileset(const std::string& file_path)
{
    // TODO: Implement tileset loading from JSON
    std::cout << "[AssetManager] Loading tileset: " << file_path << std::endl;
    return false;
}

bool AssetManager::unload_tileset(const std::string& tileset_name)
{
    // TODO: Implement tileset unloading
    std::cout << "[AssetManager] Unloading tileset: " << tileset_name << std::endl;
    return false;
}

bool AssetManager::is_tileset_loaded(const std::string& tileset_name) const
{
    // TODO: Check if tileset is loaded
    return false;
}

std::vector<std::string> AssetManager::get_loaded_tilesets() const
{
    // TODO: Return list of loaded tilesets
    return std::vector<std::string>();
}

std::vector<std::string> AssetManager::get_available_assets(const std::string& directory, AssetType type) const
{
    // TODO: Scan directory for assets of specified type
    std::cout << "[AssetManager] Scanning directory: " << directory << std::endl;
    return std::vector<std::string>();
}

const AssetMetadata* AssetManager::get_asset_metadata(const std::string& file_path) const
{
    auto it = asset_metadata_.find(file_path);
    if (it != asset_metadata_.end()) {
        return &it->second;
    }
    return nullptr;
}

size_t AssetManager::get_total_memory_usage() const
{
    return current_cache_size_;
}

void AssetManager::clear_cache()
{
    std::cout << "[AssetManager] Clearing cache..." << std::endl;
    
    for (auto& pair : texture_cache_) {
        if (pair.second->texture) {
            SDL_DestroyTexture(pair.second->texture);
        }
    }
    
    texture_cache_.clear();
    asset_metadata_.clear();
    current_cache_size_ = 0;
}

void AssetManager::clear_unused_assets()
{
    // TODO: Implement LRU cache eviction
    std::cout << "[AssetManager] Clearing unused assets..." << std::endl;
}

void AssetManager::set_max_cache_size(size_t size_bytes)
{
    max_cache_size_ = size_bytes;
    std::cout << "[AssetManager] Max cache size set to: " 
              << (max_cache_size_ / 1024 / 1024) << " MB" << std::endl;
}

size_t AssetManager::get_cache_size() const
{
    return texture_cache_.size();
}

bool AssetManager::validate_asset(const std::string& file_path) const
{
    // TODO: Validate asset file
    struct stat buffer;
    return (stat(file_path.c_str(), &buffer) == 0);
}

AssetType AssetManager::detect_asset_type(const std::string& file_path) const
{
    std::string ext = get_file_extension(file_path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
        return AssetType::TEXTURE;
    } else if (ext == ".json") {
        return AssetType::TILESET;
    } else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") {
        return AssetType::AUDIO;
    } else if (ext == ".ttf" || ext == ".otf") {
        return AssetType::FONT;
    }
    
    return AssetType::UNKNOWN;
}

std::vector<std::string> AssetManager::get_supported_extensions(AssetType type) const
{
    switch (type) {
        case AssetType::TEXTURE:
            return {".png", ".jpg", ".jpeg", ".bmp"};
        case AssetType::TILESET:
            return {".json"};
        case AssetType::AUDIO:
            return {".wav", ".mp3", ".ogg"};
        case AssetType::FONT:
            return {".ttf", ".otf"};
        default:
            return {};
    }
}

void AssetManager::watch_asset(const std::string& file_path)
{
    // TODO: Add file to watch list
    std::cout << "[AssetManager] Watching asset: " << file_path << std::endl;
}

void AssetManager::unwatch_asset(const std::string& file_path)
{
    // TODO: Remove file from watch list
    std::cout << "[AssetManager] Unwatching asset: " << file_path << std::endl;
}

bool AssetManager::check_for_changes()
{
    // TODO: Check if any watched files have been modified
    return false;
}

void AssetManager::render_texture(SDL_Texture* texture, const SDL_FRect* src_rect, const SDL_FRect* dst_rect)
{
    if (renderer_ && texture) {
        SDL_RenderTexture(renderer_, texture, src_rect, dst_rect);
    }
}

void AssetManager::render_texture_tiled(SDL_Texture* texture, const SDL_FRect* dst_rect, int tile_width, int tile_height)
{
    // TODO: Implement tiled texture rendering
    if (renderer_ && texture && dst_rect) {
        // Placeholder: just render once for now
        SDL_RenderTexture(renderer_, texture, nullptr, dst_rect);
    }
}

SDL_Texture* AssetManager::load_texture_from_file(const std::string& file_path)
{
    if (!renderer_) {
        std::cerr << "[AssetManager] Error: Renderer is null" << std::endl;
        return nullptr;
    }
    
    // TODO: Implement proper texture loading with SDL_image
    // For now, return a placeholder
    SDL_Surface* surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        std::cerr << "[AssetManager] Error creating surface: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_DestroySurface(surface);
    
    if (!texture) {
        std::cerr << "[AssetManager] Error creating texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    
    return texture;
}

void AssetManager::update_cache_size()
{
    // TODO: Calculate actual memory usage
    current_cache_size_ = texture_cache_.size() * 1024 * 1024; // Rough estimate
}

bool AssetManager::is_supported_format(const std::string& file_path, AssetType type) const
{
    std::string ext = get_file_extension(file_path);
    auto supported = get_supported_extensions(type);
    return std::find(supported.begin(), supported.end(), ext) != supported.end();
}

std::string AssetManager::get_file_extension(const std::string& file_path) const
{
    size_t pos = file_path.find_last_of('.');
    if (pos != std::string::npos) {
        return file_path.substr(pos);
    }
    return "";
}

void AssetManager::evict_lru_asset()
{
    // TODO: Implement LRU eviction policy
    std::cout << "[AssetManager] Evicting LRU asset..." << std::endl;
}

long AssetManager::get_file_modified_time(const std::string& file_path) const
{
    struct stat attrib;
    if (stat(file_path.c_str(), &attrib) == 0) {
        return attrib.st_mtime;
    }
    return 0;
}

} // namespace Editor
} // namespace Olympe
