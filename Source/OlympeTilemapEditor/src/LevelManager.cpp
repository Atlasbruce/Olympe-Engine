/*
 * Olympe Tilemap Editor - Level Manager Implementation
 */

#include "../include/LevelManager.h"
#include <iostream>
#include <fstream>
#include <ctime>

using json = nlohmann::json;

namespace Olympe {
namespace Editor {

LevelManager::LevelManager()
    : has_unsaved_changes_(false)
{
    // TODO: Initialize level manager
    std::cout << "[LevelManager] Initialized" << std::endl;
}

LevelManager::~LevelManager()
{
    std::cout << "[LevelManager] Shutdown" << std::endl;
}

bool LevelManager::load_level(const std::string& file_path)
{
    // TODO: Implement level loading from JSON file
    std::cout << "[LevelManager] Loading level: " << file_path << std::endl;
    
    if (!read_level_file(file_path, level_data_)) {
        std::cerr << "[LevelManager] Error: Failed to read level file" << std::endl;
        return false;
    }
    
    current_file_path_ = file_path;
    has_unsaved_changes_ = false;
    
    // TODO: Parse metadata from level data
    
    return true;
}

bool LevelManager::save_level(const std::string& file_path)
{
    // TODO: Implement level saving to JSON file
    std::cout << "[LevelManager] Saving level: " << file_path << std::endl;
    
    if (!validate_level_data(level_data_)) {
        std::cerr << "[LevelManager] Error: Level data validation failed" << std::endl;
        return false;
    }
    
    update_modified_date();
    
    if (!write_level_file(file_path, level_data_)) {
        std::cerr << "[LevelManager] Error: Failed to write level file" << std::endl;
        return false;
    }
    
    current_file_path_ = file_path;
    has_unsaved_changes_ = false;
    
    return true;
}

bool LevelManager::create_new_level(const std::string& name, int width, int height, int tile_size)
{
    // TODO: Create new level with specified parameters
    std::cout << "[LevelManager] Creating new level: " << name 
              << " (" << width << "x" << height << ", tile_size=" << tile_size << ")" << std::endl;
    
    clear_level_data();
    
    // Initialize basic level structure
    level_data_ = nlohmann::json::object();
    level_data_["version"] = "1.0.0";
    level_data_["name"] = name;
    level_data_["width"] = width;
    level_data_["height"] = height;
    level_data_["tile_size"] = tile_size;
    level_data_["layers"] = nlohmann::json::array();
    level_data_["tilesets"] = nlohmann::json::array();
    
    metadata_.name = name;
    metadata_.width = width;
    metadata_.height = height;
    metadata_.tile_size = tile_size;
    
    has_unsaved_changes_ = true;
    
    return true;
}

const nlohmann::json& LevelManager::get_level_data() const
{
    return level_data_;
}

nlohmann::json& LevelManager::get_level_data()
{
    return level_data_;
}

void LevelManager::set_level_data(const nlohmann::json& data)
{
    level_data_ = data;
    has_unsaved_changes_ = true;
}

const LevelMetadata& LevelManager::get_metadata() const
{
    return metadata_;
}

void LevelManager::set_metadata(const LevelMetadata& metadata)
{
    metadata_ = metadata;
    has_unsaved_changes_ = true;
}

std::vector<std::string> LevelManager::get_available_levels(const std::string& directory) const
{
    // TODO: Scan directory for .json level files
    std::cout << "[LevelManager] Scanning for levels in: " << directory << std::endl;
    std::vector<std::string> levels;
    return levels;
}

bool LevelManager::delete_level(const std::string& file_path)
{
    // TODO: Delete level file
    std::cout << "[LevelManager] Deleting level: " << file_path << std::endl;
    return false;
}

bool LevelManager::duplicate_level(const std::string& source_path, const std::string& dest_path)
{
    // TODO: Copy level file
    std::cout << "[LevelManager] Duplicating level from " << source_path << " to " << dest_path << std::endl;
    return false;
}

bool LevelManager::validate_level_data(const nlohmann::json& data) const
{
    // Clear validation errors (cast away const for this operation)
    const_cast<std::vector<std::string>&>(validation_errors_).clear();
    
    // TODO: Implement comprehensive validation
    if (!data.contains("version")) {
        const_cast<std::vector<std::string>&>(validation_errors_).push_back("Missing 'version' field");
        return false;
    }
    
    if (!data.contains("name")) {
        const_cast<std::vector<std::string>&>(validation_errors_).push_back("Missing 'name' field");
        return false;
    }
    
    return true;
}

std::vector<std::string> LevelManager::get_validation_errors() const
{
    return validation_errors_;
}

bool LevelManager::has_unsaved_changes() const
{
    return has_unsaved_changes_;
}

const std::string& LevelManager::get_current_file_path() const
{
    return current_file_path_;
}

bool LevelManager::is_level_loaded() const
{
    return !level_data_.empty();
}

bool LevelManager::read_level_file(const std::string& file_path, nlohmann::json& out_data)
{
    // TODO: Implement file reading with error handling
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        
        // Read file content into string
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        out_data = nlohmann::json::parse(content);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LevelManager] Exception reading file: " << e.what() << std::endl;
        return false;
    }
}

bool LevelManager::write_level_file(const std::string& file_path, const nlohmann::json& data)
{
    // TODO: Implement file writing with error handling
    try {
        std::ofstream file(file_path);
        if (!file.is_open()) {
            return false;
        }
        file << data.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LevelManager] Exception writing file: " << e.what() << std::endl;
        return false;
    }
}

void LevelManager::update_modified_date()
{
    // TODO: Update metadata with current timestamp
    std::time_t now = std::time(nullptr);
    metadata_.modified_date = std::to_string(now);
}

void LevelManager::clear_level_data()
{
    level_data_ = nlohmann::json::object();
    metadata_ = LevelMetadata();
    current_file_path_.clear();
    has_unsaved_changes_ = false;
    validation_errors_.clear();
}

} // namespace Editor
} // namespace Olympe
