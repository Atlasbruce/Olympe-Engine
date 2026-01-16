/*
 * TiledJsonHelper.h - JSON parsing utilities for Tiled format
 * 
 * Safe accessors and helpers for working with nlohmann::json
 * when parsing Tiled map files.
 */

#pragma once

#include "third_party/nlohmann/json.hpp"
#include <string>
#include <vector>

namespace Olympe {
namespace Tiled {

    using json = nlohmann::json;

    // Safe string accessor with default
    inline std::string GetString(const json& j, const std::string& key, const std::string& defaultValue = "")
    {
        if (j.contains(key) && j[key].is_string()) {
            return j[key].get<std::string>();
        }
        return defaultValue;
    }

    // Safe int accessor with default
    inline int GetInt(const json& j, const std::string& key, int defaultValue = 0)
    {
        if (j.contains(key) && j[key].is_number_integer()) {
            return j[key].get<int>();
        }
        return defaultValue;
    }

    // Safe float accessor with default
    inline float GetFloat(const json& j, const std::string& key, float defaultValue = 0.0f)
    {
        if (j.contains(key) && j[key].is_number()) {
            return static_cast<float>(j[key].get<double>());
        }
        return defaultValue;
    }

    // Safe double accessor with default
    inline double GetDouble(const json& j, const std::string& key, double defaultValue = 0.0)
    {
        if (j.contains(key) && j[key].is_number()) {
            return j[key].get<double>();
        }
        return defaultValue;
    }

    // Safe bool accessor with default
    inline bool GetBool(const json& j, const std::string& key, bool defaultValue = false)
    {
        if (j.contains(key) && j[key].is_boolean()) {
            return j[key].get<bool>();
        }
        return defaultValue;
    }

    // Safe array accessor
    inline json GetArray(const json& j, const std::string& key)
    {
        if (j.contains(key) && j[key].is_array()) {
            return j[key];
        }
        return json::array();
    }

    // Safe object accessor
    inline json GetObject(const json& j, const std::string& key)
    {
        if (j.contains(key) && j[key].is_object()) {
            return j[key];
        }
        return json::object();
    }

    // Check if key exists
    inline bool HasKey(const json& j, const std::string& key)
    {
        return j.contains(key);
    }

    // Get array size
    inline size_t GetArraySize(const json& j)
    {
        if (j.is_array()) {
            return j.size();
        }
        return 0;
    }

    // Convert hex color string to int (e.g., "#AARRGGBB" or "#RRGGBB")
    inline int ParseColor(const std::string& colorStr)
    {
        if (colorStr.empty() || colorStr[0] != '#') {
            return 0xFFFFFFFF; // Default white
        }

        std::string hex = colorStr.substr(1);
        unsigned long value = 0;
        
        try {
            value = std::stoul(hex, nullptr, 16);
        } catch (...) {
            return 0xFFFFFFFF;
        }

        // If 6 digits (RGB), add full alpha
        if (hex.length() == 6) {
            value |= 0xFF000000;
        }
        
        return static_cast<int>(value);
    }

    // Convert int color to hex string
    inline std::string ColorToString(int color)
    {
        constexpr size_t BUFFER_SIZE = 16;
        char buffer[BUFFER_SIZE];
        std::snprintf(buffer, BUFFER_SIZE, "#%08X", static_cast<unsigned int>(color));
        return std::string(buffer);
    }

} // namespace Tiled
} // namespace Olympe
