/*
 * Olympe Engine V2 - 2025
 * Nicolas Chereau
 * nchereau@gmail.com
 * 
 * JSON Helper - Centralized JSON operations
 * 
 * This header-only file provides safe and convenient functions for JSON operations
 * to eliminate code duplication across the project.
 * 
 * Features:
 * - File I/O (load/save JSON files)
 * - Safe accessors with default values
 * - Array/Object helpers
 * - Config file helpers
 * - Key validation
 */

#pragma once

#include "third_party/nlohmann/json.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <functional>

using json = nlohmann::json;

namespace JsonHelper {

// ============================================================================
// File I/O Functions
// ============================================================================

/**
 * Load and parse a JSON file
 * @param filepath Path to the JSON file
 * @param j Output json object
 * @return true if successful, false otherwise
 */
inline bool LoadJsonFromFile(const std::string& filepath, json& j)
{
    try
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "JsonHelper: Failed to open file: " << filepath << std::endl;
            return false;
        }
        
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        j = json::parse(jsonStr);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "JsonHelper: Error loading JSON from " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

/**
 * Save a JSON object to a file with formatting
 * @param filepath Path to save the file
 * @param j JSON object to save
 * @param indent Number of spaces for indentation (default: 4)
 * @return true if successful, false otherwise
 */
inline bool SaveJsonToFile(const std::string& filepath, const json& j, int indent = 4)
{
    try
    {
        std::ofstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "JsonHelper: Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        
        file << j.dump(indent);
        file.close();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "JsonHelper: Error saving JSON to " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// Safe Accessor Functions
// ============================================================================

/**
 * Safely get a string value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The string value or default
 */
inline std::string GetString(const json& j, const std::string& key, const std::string& defaultValue = "")
{
    if (j.contains(key) && j[key].is_string())
        return j[key].get<std::string>();
    return defaultValue;
}

/**
 * Safely get an integer value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The int value or default
 */
inline int GetInt(const json& j, const std::string& key, int defaultValue = 0)
{
    if (j.contains(key) && j[key].is_number())
        return j[key].get<int>();
    return defaultValue;
}

/**
 * Safely get an unsigned integer value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The uint32_t value or default
 */
inline uint32_t GetUInt(const json& j, const std::string& key, uint32_t defaultValue = 0)
{
    if (j.contains(key) && j[key].is_number())
    {
        int value = j[key].get<int>();
        // Protect against negative values
        if (value < 0)
            return defaultValue;
        return static_cast<uint32_t>(value);
    }
    return defaultValue;
}

/**
 * Safely get a float value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The float value or default
 */
inline float GetFloat(const json& j, const std::string& key, float defaultValue = 0.0f)
{
    if (j.contains(key) && j[key].is_number())
        return static_cast<float>(j[key].get<double>());
    return defaultValue;
}

/**
 * Safely get a double value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The double value or default
 */
inline double GetDouble(const json& j, const std::string& key, double defaultValue = 0.0)
{
    if (j.contains(key) && j[key].is_number())
        return j[key].get<double>();
    return defaultValue;
}

/**
 * Safely get a boolean value from JSON
 * @param j JSON object
 * @param key Key to access
 * @param defaultValue Default value if key doesn't exist or type mismatch
 * @return The bool value or default
 */
inline bool GetBool(const json& j, const std::string& key, bool defaultValue = false)
{
    if (j.contains(key) && j[key].is_boolean())
        return j[key].get<bool>();
    return defaultValue;
}

// ============================================================================
// Array/Object Helper Functions
// ============================================================================

/**
 * Check if a key contains an array
 * @param j JSON object
 * @param key Key to check
 * @return true if the key exists and is an array
 */
inline bool IsArray(const json& j, const std::string& key)
{
    return j.contains(key) && j[key].is_array();
}

/**
 * Check if a key contains an object
 * @param j JSON object
 * @param key Key to check
 * @return true if the key exists and is an object
 */
inline bool IsObject(const json& j, const std::string& key)
{
    return j.contains(key) && j[key].is_object();
}

/**
 * Get the size of an array
 * @param j JSON object
 * @param key Key to the array
 * @return Size of the array, or 0 if not an array
 */
inline size_t GetArraySize(const json& j, const std::string& key)
{
    if (IsArray(j, key))
        return j[key].size();
    return 0;
}

/**
 * Iterate over an array with a callback function
 * @param j JSON object
 * @param key Key to the array
 * @param callback Function to call for each element (takes const json& and size_t index)
 */
inline void ForEachInArray(const json& j, const std::string& key, std::function<void(const json&, size_t)> callback)
{
    if (IsArray(j, key))
    {
        const auto& arr = j[key];
        for (size_t i = 0; i < arr.size(); ++i)
        {
            callback(arr[i], i);
        }
    }
}

/**
 * Get a nested float value from a parent object
 * @param j JSON object
 * @param parentKey Key to the parent object
 * @param childKey Key within the parent object
 * @param defaultValue Default value if not found
 * @return The float value or default
 */
inline float GetNestedFloat(const json& j, const std::string& parentKey, const std::string& childKey, float defaultValue = 0.0f)
{
    if (IsObject(j, parentKey))
    {
        return GetFloat(j[parentKey], childKey, defaultValue);
    }
    return defaultValue;
}

/**
 * Get a nested string value from a parent object
 * @param j JSON object
 * @param parentKey Key to the parent object
 * @param childKey Key within the parent object
 * @param defaultValue Default value if not found
 * @return The string value or default
 */
inline std::string GetNestedString(const json& j, const std::string& parentKey, const std::string& childKey, const std::string& defaultValue = "")
{
    if (IsObject(j, parentKey))
    {
        return GetString(j[parentKey], childKey, defaultValue);
    }
    return defaultValue;
}

/**
 * Get a nested int value from a parent object
 * @param j JSON object
 * @param parentKey Key to the parent object
 * @param childKey Key within the parent object
 * @param defaultValue Default value if not found
 * @return The int value or default
 */
inline int GetNestedInt(const json& j, const std::string& parentKey, const std::string& childKey, int defaultValue = 0)
{
    if (IsObject(j, parentKey))
    {
        return GetInt(j[parentKey], childKey, defaultValue);
    }
    return defaultValue;
}

// ============================================================================
// Config Helper Functions
// ============================================================================

/**
 * Load a configuration file (alias for LoadJsonFromFile)
 * @param filepath Path to the config file
 * @param j Output json object
 * @return true if successful, false otherwise
 */
inline bool LoadConfig(const std::string& filepath, json& j)
{
    return LoadJsonFromFile(filepath, j);
}

/**
 * Save a configuration file (alias for SaveJsonToFile with indent=2)
 * @param filepath Path to save the config file
 * @param j JSON object to save
 * @return true if successful, false otherwise
 */
inline bool SaveConfig(const std::string& filepath, const json& j)
{
    return SaveJsonToFile(filepath, j, 2);
}

// ============================================================================
// Validation Functions
// ============================================================================

/**
 * Validate that all required keys exist in a JSON object
 * @param j JSON object to validate
 * @param requiredKeys Vector of required key names
 * @return true if all keys exist, false otherwise
 */
inline bool ValidateKeys(const json& j, const std::vector<std::string>& requiredKeys)
{
    for (const auto& key : requiredKeys)
    {
        if (!j.contains(key))
        {
            std::cerr << "JsonHelper: Missing required key: " << key << std::endl;
            return false;
        }
    }
    return true;
}

/**
 * Validate that all required keys exist and log which ones are missing
 * @param j JSON object to validate
 * @param requiredKeys Vector of required key names
 * @param context Context string for error messages
 * @return true if all keys exist, false otherwise
 */
inline bool ValidateKeysVerbose(const json& j, const std::vector<std::string>& requiredKeys, const std::string& context = "")
{
    bool allValid = true;
    for (const auto& key : requiredKeys)
    {
        if (!j.contains(key))
        {
            std::cerr << "JsonHelper: Missing required key '" << key << "'";
            if (!context.empty())
                std::cerr << " in " << context;
            std::cerr << std::endl;
            allValid = false;
        }
    }
    return allValid;
}

} // namespace JsonHelper
