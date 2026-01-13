/*
 * Olympe Tilemap Editor - Tileset Manager
 * 
 * Manages tileset data, tile selections, and tile properties.
 * Handles tileset loading, organization, and tile manipulation.
 */

#ifndef OLYMPE_EDITOR_TILESETMANAGER_H
#define OLYMPE_EDITOR_TILESETMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <SDL3/SDL.h>
#include "../../../Source/third_party/nlohmann/json.hpp"

namespace Olympe {
namespace Editor {

/**
 * Tile properties structure
 */
struct TileProperties {
    int tile_id;
    std::string name;
    std::string category;
    bool is_collidable;
    bool is_animated;
    float animation_speed;
    std::vector<int> animation_frames;
    nlohmann::json custom_properties;
};

/**
 * Tileset structure
 */
struct Tileset {
    std::string name;
    std::string texture_path;
    SDL_Texture* texture;
    int tile_width;
    int tile_height;
    int spacing;
    int margin;
    int columns;
    int rows;
    int tile_count;
    std::unordered_map<int, TileProperties> tile_properties;
};

/**
 * Tile group structure for organization
 */
struct TileGroup {
    std::string name;
    std::vector<int> tile_ids;
    bool collapsed;
};

/**
 * TilesetManager class
 * Manages tilesets and tile selections
 */
class TilesetManager {
public:
    TilesetManager();
    ~TilesetManager();

    // Tileset loading and management
    bool load_tileset(const std::string& file_path, SDL_Renderer* renderer);
    bool unload_tileset(const std::string& name);
    bool reload_tileset(const std::string& name, SDL_Renderer* renderer);
    
    // Tileset queries
    const Tileset* get_tileset(const std::string& name) const;
    Tileset* get_tileset(const std::string& name);
    std::vector<std::string> get_tileset_names() const;
    int get_tileset_count() const;
    bool has_tileset(const std::string& name) const;
    
    // Tile properties
    bool set_tile_properties(const std::string& tileset_name, int tile_id, const TileProperties& props);
    const TileProperties* get_tile_properties(const std::string& tileset_name, int tile_id) const;
    bool has_tile_properties(const std::string& tileset_name, int tile_id) const;
    
    // Tile coordinate conversion
    void get_tile_uv(const Tileset* tileset, int tile_id, SDL_FRect& out_uv) const;
    int get_tile_id_from_position(const Tileset* tileset, int pixel_x, int pixel_y) const;
    void get_tile_position(const Tileset* tileset, int tile_id, int& out_x, int& out_y) const;
    
    // Tile groups
    void create_tile_group(const std::string& name);
    void delete_tile_group(const std::string& name);
    void add_tile_to_group(const std::string& group_name, int tile_id);
    void remove_tile_from_group(const std::string& group_name, int tile_id);
    const std::vector<TileGroup>& get_tile_groups() const;
    TileGroup* get_tile_group(const std::string& name);
    
    // Tileset creation
    bool create_tileset_from_texture(const std::string& name, const std::string& texture_path,
                                     int tile_width, int tile_height, int spacing, int margin,
                                     SDL_Renderer* renderer);
    
    // Tileset import/export
    bool export_tileset(const std::string& tileset_name, const std::string& file_path) const;
    bool import_tileset(const std::string& file_path, SDL_Renderer* renderer);
    
    // Validation
    bool validate_tileset(const Tileset& tileset) const;
    std::vector<std::string> get_validation_errors() const;
    
    // Auto-tiling support
    void set_autotile_rules(const std::string& tileset_name, const nlohmann::json& rules);
    const nlohmann::json* get_autotile_rules(const std::string& tileset_name) const;
    int calculate_autotile(const std::string& tileset_name, int neighbors) const;

private:
    std::unordered_map<std::string, std::unique_ptr<Tileset>> tilesets_;
    std::vector<TileGroup> tile_groups_;
    std::unordered_map<std::string, nlohmann::json> autotile_rules_;
    std::vector<std::string> validation_errors_;
    
    // Helper methods
    bool load_tileset_metadata(const std::string& file_path, Tileset& out_tileset);
    bool save_tileset_metadata(const std::string& file_path, const Tileset& tileset) const;
    void calculate_tileset_dimensions(Tileset& tileset);
    bool validate_tile_id(const Tileset* tileset, int tile_id) const;
    void clear_validation_errors();
    void add_validation_error(const std::string& error);
};

} // namespace Editor
} // namespace Olympe

#endif // OLYMPE_EDITOR_TILESETMANAGER_H
