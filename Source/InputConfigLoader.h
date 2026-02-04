/*
Olympe Engine V2 - 2026
Input Configuration Loader

Loads input configuration from JSON files including:
- Input profiles
- Action maps
- Device assignments
- User overrides
*/

#pragma once

#include "InputDevice.h"
#include <string>
#include "third_party/nlohmann/json.hpp"

using json = nlohmann::json;

class InputConfigLoader {
public:
    InputConfigLoader() = default;
    ~InputConfigLoader() = default;
    
    // Singleton access
    static InputConfigLoader& Get() {
        static InputConfigLoader instance;
        return instance;
    }
    
    // Load complete input configuration
    bool LoadInputConfig(const std::string& path);
    
    // Load olympe-config.json for engine settings
    bool LoadEngineConfig(const std::string& path);
    
    // Override specific profiles from user config
    bool LoadProfileOverride(const std::string& path);
    
    // Save current configuration
    bool SaveInputConfig(const std::string& path);
    
private:
    // Parsing helpers
    bool ParseProfiles(const json& j);
    bool ParseActionMaps(const json& j);
    bool ParseGlobalSettings(const json& j);
    bool ParseDefaultAssignments(const json& j);
    
    InputBinding ParseInputBinding(const json& j);
    SDL_Scancode ParseKeyName(const std::string& keyName);
    
    // Logging
    int m_logLevel = 2;  // 0=error, 1=warning, 2=info, 3=debug
};

