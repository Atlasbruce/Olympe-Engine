/*
 * Olympe Tilemap Editor - Asset Manager
 * 
 * Handles asset loading and caching for textures, tilesets, and other resources.
 * Provides efficient resource management with automatic caching.
 */

#ifndef OLYMPE_EDITOR_ASSETMANAGER_H
#define OLYMPE_EDITOR_ASSETMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <SDL3/SDL.h>

namespace Olympe {
namespace Editor {

/**
 * Asset type enumeration
 */
enum class AssetType {
    TEXTURE,
    TILESET,
    SPRITE,
    AUDIO,
    FONT,
    UNKNOWN
};

/**
 * Asset metadata structure
 */
struct AssetMetadata {
    std::string name;
    std::string file_path;
    AssetType type;
    size_t memory_size;
    bool is_loaded;
};

/**
 * Texture asset structure
 */
struct TextureAsset {
    SDL_Texture* texture;
    int width;
    int height;
    std::string file_path;
    bool is_valid;
};

/**
 * AssetManager class
 * Manages loading, caching, and unloading of editor assets
 */
class AssetManager {
public:
    explicit AssetManager(SDL_Renderer* renderer);
    ~AssetManager();

    // Texture management
    SDL_Texture* load_texture(const std::string& file_path);
    const TextureAsset* get_texture(const std::string& file_path) const;
    bool unload_texture(const std::string& file_path);
    bool is_texture_loaded(const std::string& file_path) const;
    
    // Tileset management
    bool load_tileset(const std::string& file_path);
    bool unload_tileset(const std::string& tileset_name);
    bool is_tileset_loaded(const std::string& tileset_name) const;
    std::vector<std::string> get_loaded_tilesets() const;
    
    // Asset queries
    std::vector<std::string> get_available_assets(const std::string& directory, AssetType type) const;
    const AssetMetadata* get_asset_metadata(const std::string& file_path) const;
    size_t get_total_memory_usage() const;
    
    // Cache management
    void clear_cache();
    void clear_unused_assets();
    void set_max_cache_size(size_t size_bytes);
    size_t get_cache_size() const;
    
    // Resource validation
    bool validate_asset(const std::string& file_path) const;
    AssetType detect_asset_type(const std::string& file_path) const;
    std::vector<std::string> get_supported_extensions(AssetType type) const;
    
    // Hot reload support
    void watch_asset(const std::string& file_path);
    void unwatch_asset(const std::string& file_path);
    bool check_for_changes();
    
    // Rendering utilities
    void render_texture(SDL_Texture* texture, const SDL_FRect* src_rect, const SDL_FRect* dst_rect);
    void render_texture_tiled(SDL_Texture* texture, const SDL_FRect* dst_rect, int tile_width, int tile_height);

private:
    SDL_Renderer* renderer_;
    std::unordered_map<std::string, std::unique_ptr<TextureAsset>> texture_cache_;
    std::unordered_map<std::string, AssetMetadata> asset_metadata_;
    std::vector<std::string> watched_assets_;
    size_t max_cache_size_;
    size_t current_cache_size_;
    
    // Helper methods
    SDL_Texture* load_texture_from_file(const std::string& file_path);
    void update_cache_size();
    bool is_supported_format(const std::string& file_path, AssetType type) const;
    std::string get_file_extension(const std::string& file_path) const;
    void evict_lru_asset();
    
    // File watching
    struct FileWatchInfo {
        std::string file_path;
        long last_modified;
    };
    std::unordered_map<std::string, FileWatchInfo> file_watch_map_;
    long get_file_modified_time(const std::string& file_path) const;
};

} // namespace Editor
} // namespace Olympe

#endif // OLYMPE_EDITOR_ASSETMANAGER_H
