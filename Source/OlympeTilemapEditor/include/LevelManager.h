/*
 * Olympe Tilemap Editor - Level Manager
 * 
 * Manages level loading, saving, and metadata for the tilemap editor.
 * Handles level file I/O operations and level data structures.
 */

#ifndef OLYMPE_EDITOR_LEVELMANAGER_H
#define OLYMPE_EDITOR_LEVELMANAGER_H

#include <string>
#include <vector>
#include <memory>
#include "../../../Source/third_party/nlohmann/json.hpp"

namespace Olympe {
namespace Editor {

/**
 * Level metadata structure
 * Contains information about a level file
 */
struct LevelMetadata {
    std::string name;
    std::string file_path;
    std::string author;
    std::string description;
    int width;
    int height;
    int tile_size;
    std::string created_date;
    std::string modified_date;
};

/**
 * LevelManager class
 * Manages level loading, saving, and metadata operations
 */
class LevelManager {
public:
    LevelManager();
    ~LevelManager();

    // Level file operations
    bool load_level(const std::string& file_path);
    bool save_level(const std::string& file_path);
    bool create_new_level(const std::string& name, int width, int height, int tile_size);
    
    // Level data access
    const nlohmann::json& get_level_data() const;
    nlohmann::json& get_level_data();
    void set_level_data(const nlohmann::json& data);
    
    // Metadata operations
    const LevelMetadata& get_metadata() const;
    void set_metadata(const LevelMetadata& metadata);
    
    // Level list management
    std::vector<std::string> get_available_levels(const std::string& directory) const;
    bool delete_level(const std::string& file_path);
    bool duplicate_level(const std::string& source_path, const std::string& dest_path);
    
    // Validation
    bool validate_level_data(const nlohmann::json& data) const;
    std::vector<std::string> get_validation_errors() const;
    
    // State queries
    bool has_unsaved_changes() const;
    const std::string& get_current_file_path() const;
    bool is_level_loaded() const;

private:
    nlohmann::json level_data_;
    LevelMetadata metadata_;
    std::string current_file_path_;
    bool has_unsaved_changes_;
    std::vector<std::string> validation_errors_;
    
    // Helper methods
    bool read_level_file(const std::string& file_path, nlohmann::json& out_data);
    bool write_level_file(const std::string& file_path, const nlohmann::json& data);
    void update_modified_date();
    void clear_level_data();
};

} // namespace Editor
} // namespace Olympe

#endif // OLYMPE_EDITOR_LEVELMANAGER_H
