/*
 * Olympe Tilemap Editor - Tileset Manager Implementation
 */

#include "../include/TilesetManager.h"
#include <iostream>
#include <fstream>
#include <algorithm>

namespace Olympe {
namespace Editor {

TilesetManager::TilesetManager()
{
    std::cout << "[TilesetManager] Initialized" << std::endl;
}

TilesetManager::~TilesetManager()
{
    // Clean up textures
    for (auto& pair : tilesets_) {
        if (pair.second->texture) {
            SDL_DestroyTexture(pair.second->texture);
            pair.second->texture = nullptr;
        }
    }
    std::cout << "[TilesetManager] Shutdown" << std::endl;
}

bool TilesetManager::load_tileset(const std::string& file_path, SDL_Renderer* renderer)
{
    std::cout << "[TilesetManager] Loading tileset: " << file_path << std::endl;
    
    auto tileset = std::make_unique<Tileset>();
    
    if (!load_tileset_metadata(file_path, *tileset)) {
        std::cerr << "[TilesetManager] Error: Failed to load tileset metadata" << std::endl;
        return false;
    }
    
    // TODO: Load texture from texture_path
    // For now, create a placeholder texture
    if (renderer) {
        SDL_Surface* surface = SDL_CreateSurface(256, 256, SDL_PIXELFORMAT_RGBA32);
        if (surface) {
            tileset->texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
        }
    }
    
    calculate_tileset_dimensions(*tileset);
    
    std::string tileset_name = tileset->name;
    tilesets_[tileset_name] = std::move(tileset);
    
    return true;
}

bool TilesetManager::unload_tileset(const std::string& name)
{
    auto it = tilesets_.find(name);
    if (it != tilesets_.end()) {
        std::cout << "[TilesetManager] Unloading tileset: " << name << std::endl;
        if (it->second->texture) {
            SDL_DestroyTexture(it->second->texture);
        }
        tilesets_.erase(it);
        return true;
    }
    return false;
}

bool TilesetManager::reload_tileset(const std::string& name, SDL_Renderer* renderer)
{
    auto it = tilesets_.find(name);
    if (it != tilesets_.end()) {
        std::string file_path = it->second->texture_path;
        unload_tileset(name);
        return load_tileset(file_path, renderer);
    }
    return false;
}

const Tileset* TilesetManager::get_tileset(const std::string& name) const
{
    auto it = tilesets_.find(name);
    if (it != tilesets_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Tileset* TilesetManager::get_tileset(const std::string& name)
{
    auto it = tilesets_.find(name);
    if (it != tilesets_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> TilesetManager::get_tileset_names() const
{
    std::vector<std::string> names;
    for (const auto& pair : tilesets_) {
        names.push_back(pair.first);
    }
    return names;
}

int TilesetManager::get_tileset_count() const
{
    return static_cast<int>(tilesets_.size());
}

bool TilesetManager::has_tileset(const std::string& name) const
{
    return tilesets_.find(name) != tilesets_.end();
}

bool TilesetManager::set_tile_properties(const std::string& tileset_name, int tile_id, const TileProperties& props)
{
    auto* tileset = get_tileset(tileset_name);
    if (!tileset) {
        return false;
    }
    
    if (!validate_tile_id(tileset, tile_id)) {
        return false;
    }
    
    tileset->tile_properties[tile_id] = props;
    return true;
}

const TileProperties* TilesetManager::get_tile_properties(const std::string& tileset_name, int tile_id) const
{
    const auto* tileset = get_tileset(tileset_name);
    if (!tileset) {
        return nullptr;
    }
    
    auto it = tileset->tile_properties.find(tile_id);
    if (it != tileset->tile_properties.end()) {
        return &it->second;
    }
    
    return nullptr;
}

bool TilesetManager::has_tile_properties(const std::string& tileset_name, int tile_id) const
{
    return get_tile_properties(tileset_name, tile_id) != nullptr;
}

void TilesetManager::get_tile_uv(const Tileset* tileset, int tile_id, SDL_FRect& out_uv) const
{
    if (!tileset || !validate_tile_id(tileset, tile_id)) {
        out_uv = {0, 0, 0, 0};
        return;
    }
    
    int col = tile_id % tileset->columns;
    int row = tile_id / tileset->columns;
    
    out_uv.x = static_cast<float>(tileset->margin + col * (tileset->tile_width + tileset->spacing));
    out_uv.y = static_cast<float>(tileset->margin + row * (tileset->tile_height + tileset->spacing));
    out_uv.w = static_cast<float>(tileset->tile_width);
    out_uv.h = static_cast<float>(tileset->tile_height);
}

int TilesetManager::get_tile_id_from_position(const Tileset* tileset, int pixel_x, int pixel_y) const
{
    if (!tileset) {
        return -1;
    }
    
    int adjusted_x = pixel_x - tileset->margin;
    int adjusted_y = pixel_y - tileset->margin;
    
    int col = adjusted_x / (tileset->tile_width + tileset->spacing);
    int row = adjusted_y / (tileset->tile_height + tileset->spacing);
    
    if (col < 0 || col >= tileset->columns || row < 0 || row >= tileset->rows) {
        return -1;
    }
    
    int tile_id = row * tileset->columns + col;
    return (tile_id < tileset->tile_count) ? tile_id : -1;
}

void TilesetManager::get_tile_position(const Tileset* tileset, int tile_id, int& out_x, int& out_y) const
{
    if (!tileset || !validate_tile_id(tileset, tile_id)) {
        out_x = 0;
        out_y = 0;
        return;
    }
    
    int col = tile_id % tileset->columns;
    int row = tile_id / tileset->columns;
    
    out_x = tileset->margin + col * (tileset->tile_width + tileset->spacing);
    out_y = tileset->margin + row * (tileset->tile_height + tileset->spacing);
}

void TilesetManager::create_tile_group(const std::string& name)
{
    TileGroup group;
    group.name = name;
    group.collapsed = false;
    tile_groups_.push_back(group);
    std::cout << "[TilesetManager] Created tile group: " << name << std::endl;
}

void TilesetManager::delete_tile_group(const std::string& name)
{
    auto it = std::find_if(tile_groups_.begin(), tile_groups_.end(),
        [&name](const TileGroup& group) { return group.name == name; });
    
    if (it != tile_groups_.end()) {
        tile_groups_.erase(it);
        std::cout << "[TilesetManager] Deleted tile group: " << name << std::endl;
    }
}

void TilesetManager::add_tile_to_group(const std::string& group_name, int tile_id)
{
    auto* group = get_tile_group(group_name);
    if (group) {
        group->tile_ids.push_back(tile_id);
    }
}

void TilesetManager::remove_tile_from_group(const std::string& group_name, int tile_id)
{
    auto* group = get_tile_group(group_name);
    if (group) {
        auto it = std::find(group->tile_ids.begin(), group->tile_ids.end(), tile_id);
        if (it != group->tile_ids.end()) {
            group->tile_ids.erase(it);
        }
    }
}

const std::vector<TileGroup>& TilesetManager::get_tile_groups() const
{
    return tile_groups_;
}

TileGroup* TilesetManager::get_tile_group(const std::string& name)
{
    auto it = std::find_if(tile_groups_.begin(), tile_groups_.end(),
        [&name](const TileGroup& group) { return group.name == name; });
    
    return (it != tile_groups_.end()) ? &(*it) : nullptr;
}

bool TilesetManager::create_tileset_from_texture(const std::string& name, const std::string& texture_path,
                                                  int tile_width, int tile_height, int spacing, int margin,
                                                  SDL_Renderer* renderer)
{
    std::cout << "[TilesetManager] Creating tileset from texture: " << texture_path << std::endl;
    
    // TODO: Implement tileset creation from texture
    auto tileset = std::make_unique<Tileset>();
    tileset->name = name;
    tileset->texture_path = texture_path;
    tileset->tile_width = tile_width;
    tileset->tile_height = tile_height;
    tileset->spacing = spacing;
    tileset->margin = margin;
    
    calculate_tileset_dimensions(*tileset);
    
    tilesets_[name] = std::move(tileset);
    
    return true;
}

bool TilesetManager::export_tileset(const std::string& tileset_name, const std::string& file_path) const
{
    const auto* tileset = get_tileset(tileset_name);
    if (!tileset) {
        return false;
    }
    
    return save_tileset_metadata(file_path, *tileset);
}

bool TilesetManager::import_tileset(const std::string& file_path, SDL_Renderer* renderer)
{
    return load_tileset(file_path, renderer);
}

bool TilesetManager::validate_tileset(const Tileset& tileset) const
{
    clear_validation_errors();
    
    if (tileset.name.empty()) {
        add_validation_error("Tileset name is empty");
    }
    
    if (tileset.tile_width <= 0 || tileset.tile_height <= 0) {
        add_validation_error("Invalid tile dimensions");
    }
    
    if (tileset.columns <= 0 || tileset.rows <= 0) {
        add_validation_error("Invalid tileset dimensions");
    }
    
    return validation_errors_.empty();
}

std::vector<std::string> TilesetManager::get_validation_errors() const
{
    return validation_errors_;
}

void TilesetManager::set_autotile_rules(const std::string& tileset_name, const nlohmann::json& rules)
{
    autotile_rules_[tileset_name] = rules;
}

const nlohmann::json* TilesetManager::get_autotile_rules(const std::string& tileset_name) const
{
    auto it = autotile_rules_.find(tileset_name);
    if (it != autotile_rules_.end()) {
        return &it->second;
    }
    return nullptr;
}

int TilesetManager::calculate_autotile(const std::string& tileset_name, int neighbors) const
{
    // TODO: Implement autotile calculation based on neighbor bitmask
    return 0;
}

bool TilesetManager::load_tileset_metadata(const std::string& file_path, Tileset& out_tileset)
{
    // TODO: Load tileset metadata from JSON file
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        nlohmann::json data;
        file >> data;
        
        out_tileset.name = data.value("name", "Unnamed");
        out_tileset.texture_path = data.value("texture", "");
        out_tileset.tile_width = data.value("tile_width", 32);
        out_tileset.tile_height = data.value("tile_height", 32);
        out_tileset.spacing = data.value("spacing", 0);
        out_tileset.margin = data.value("margin", 0);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TilesetManager] Error loading metadata: " << e.what() << std::endl;
        return false;
    }
}

bool TilesetManager::save_tileset_metadata(const std::string& file_path, const Tileset& tileset) const
{
    // TODO: Save tileset metadata to JSON file
    try {
        nlohmann::json data;
        data["name"] = tileset.name;
        data["texture"] = tileset.texture_path;
        data["tile_width"] = tileset.tile_width;
        data["tile_height"] = tileset.tile_height;
        data["spacing"] = tileset.spacing;
        data["margin"] = tileset.margin;
        data["columns"] = tileset.columns;
        data["rows"] = tileset.rows;
        data["tile_count"] = tileset.tile_count;
        
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        file << data.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[TilesetManager] Error saving metadata: " << e.what() << std::endl;
        return false;
    }
}

void TilesetManager::calculate_tileset_dimensions(Tileset& tileset)
{
    // TODO: Calculate columns, rows, and tile_count based on texture size
    // For now, use default values
    if (tileset.columns == 0) {
        tileset.columns = 8;
    }
    if (tileset.rows == 0) {
        tileset.rows = 8;
    }
    tileset.tile_count = tileset.columns * tileset.rows;
}

bool TilesetManager::validate_tile_id(const Tileset* tileset, int tile_id) const
{
    return tileset && tile_id >= 0 && tile_id < tileset->tile_count;
}

void TilesetManager::clear_validation_errors() const
{
    validation_errors_.clear();
}

void TilesetManager::add_validation_error(const std::string& error) const
{
    validation_errors_.push_back(error);
}

} // namespace Editor
} // namespace Olympe
