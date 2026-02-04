/*
Olympe Engine V2 - 2026
Input Configuration Loader - Implementation

Loads input configuration from JSON files.
*/

#include "InputConfigLoader.h"
#include "InputDevice.h"
#include "system/system_utils.h"
#include "third_party/nlohmann/json.hpp"
#include <fstream>
#include <unordered_map>

using json = nlohmann::json;

//==============================================================================
// Key Name to SDL_Scancode Mapping
//==============================================================================

static std::unordered_map<std::string, SDL_Scancode> s_keyNameMap = {
    // Letters
    {"A", SDL_SCANCODE_A}, {"B", SDL_SCANCODE_B}, {"C", SDL_SCANCODE_C}, {"D", SDL_SCANCODE_D},
    {"E", SDL_SCANCODE_E}, {"F", SDL_SCANCODE_F}, {"G", SDL_SCANCODE_G}, {"H", SDL_SCANCODE_H},
    {"I", SDL_SCANCODE_I}, {"J", SDL_SCANCODE_J}, {"K", SDL_SCANCODE_K}, {"L", SDL_SCANCODE_L},
    {"M", SDL_SCANCODE_M}, {"N", SDL_SCANCODE_N}, {"O", SDL_SCANCODE_O}, {"P", SDL_SCANCODE_P},
    {"Q", SDL_SCANCODE_Q}, {"R", SDL_SCANCODE_R}, {"S", SDL_SCANCODE_S}, {"T", SDL_SCANCODE_T},
    {"U", SDL_SCANCODE_U}, {"V", SDL_SCANCODE_V}, {"W", SDL_SCANCODE_W}, {"X", SDL_SCANCODE_X},
    {"Y", SDL_SCANCODE_Y}, {"Z", SDL_SCANCODE_Z},
    
    // Numbers
    {"0", SDL_SCANCODE_0}, {"1", SDL_SCANCODE_1}, {"2", SDL_SCANCODE_2}, {"3", SDL_SCANCODE_3},
    {"4", SDL_SCANCODE_4}, {"5", SDL_SCANCODE_5}, {"6", SDL_SCANCODE_6}, {"7", SDL_SCANCODE_7},
    {"8", SDL_SCANCODE_8}, {"9", SDL_SCANCODE_9},
    
    // Function keys
    {"F1", SDL_SCANCODE_F1}, {"F2", SDL_SCANCODE_F2}, {"F3", SDL_SCANCODE_F3}, {"F4", SDL_SCANCODE_F4},
    {"F5", SDL_SCANCODE_F5}, {"F6", SDL_SCANCODE_F6}, {"F7", SDL_SCANCODE_F7}, {"F8", SDL_SCANCODE_F8},
    {"F9", SDL_SCANCODE_F9}, {"F10", SDL_SCANCODE_F10}, {"F11", SDL_SCANCODE_F11}, {"F12", SDL_SCANCODE_F12},
    
    // Special keys
    {"SPACE", SDL_SCANCODE_SPACE}, {"RETURN", SDL_SCANCODE_RETURN}, {"ESCAPE", SDL_SCANCODE_ESCAPE},
    {"TAB", SDL_SCANCODE_TAB}, {"BACKSPACE", SDL_SCANCODE_BACKSPACE}, {"DELETE", SDL_SCANCODE_DELETE},
    
    // Modifiers
    {"LSHIFT", SDL_SCANCODE_LSHIFT}, {"RSHIFT", SDL_SCANCODE_RSHIFT},
    {"LCTRL", SDL_SCANCODE_LCTRL}, {"RCTRL", SDL_SCANCODE_RCTRL},
    {"LALT", SDL_SCANCODE_LALT}, {"RALT", SDL_SCANCODE_RALT},
    
    // Arrow keys
    {"UP", SDL_SCANCODE_UP}, {"DOWN", SDL_SCANCODE_DOWN},
    {"LEFT", SDL_SCANCODE_LEFT}, {"RIGHT", SDL_SCANCODE_RIGHT},
    
    // Navigation
    {"HOME", SDL_SCANCODE_HOME}, {"END", SDL_SCANCODE_END},
    {"PAGEUP", SDL_SCANCODE_PAGEUP}, {"PAGEDOWN", SDL_SCANCODE_PAGEDOWN},
    {"INSERT", SDL_SCANCODE_INSERT}
};

SDL_Scancode InputConfigLoader::ParseKeyName(const std::string& keyName) {
    auto it = s_keyNameMap.find(keyName);
    if (it != s_keyNameMap.end()) {
        return it->second;
    }
    
    if (m_logLevel >= 1) {
        SYSTEM_LOG << "[InputConfig][Warning] Unknown key name: " << keyName << ", using UNKNOWN\n";
    }
    return SDL_SCANCODE_UNKNOWN;
}

//==============================================================================
// Configuration Loading
//==============================================================================

bool InputConfigLoader::LoadEngineConfig(const std::string& path) {
    SYSTEM_LOG << "[InputConfig][Info] Loading engine config from: " << path << "\n";
    
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            SYSTEM_LOG << "[InputConfig][Warning] Could not open engine config file: " << path << "\n";
            return false;
        }
        
        json j;
        file >> j;
        
        // Read editor_enabled flag
        if (j.contains("editor_enabled")) {
            bool editorEnabled = j["editor_enabled"].get<bool>();
            InputContextManager::Get().SetEditorEnabled(editorEnabled);
        }
        
        // Read log level
        if (j.contains("input_log_level")) {
            std::string logLevel = j["input_log_level"].get<std::string>();
            InputDeviceManager::Get().SetLogLevel(logLevel);
            
            if (logLevel == "error") m_logLevel = 0;
            else if (logLevel == "warning") m_logLevel = 1;
            else if (logLevel == "info") m_logLevel = 2;
            else if (logLevel == "debug") m_logLevel = 3;
        }
        
        SYSTEM_LOG << "[InputConfig][Info] Engine config loaded successfully\n";
        return true;
    }
    catch (const std::exception& e) {
        SYSTEM_LOG << "[InputConfig][Error] Failed to parse engine config: " << e.what() << "\n";
        return false;
    }
}

bool InputConfigLoader::LoadInputConfig(const std::string& path) {
    SYSTEM_LOG << "[InputConfig][Info] Loading input config from: " << path << "\n";
    
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            SYSTEM_LOG << "[InputConfig][Warning] Could not open input config file: " << path << "\n";
            return false;
        }
        
        json j;
        file >> j;
        
        // Parse sections
        if (j.contains("profiles")) {
            if (!ParseProfiles(j["profiles"])) {
                SYSTEM_LOG << "[InputConfig][Warning] Failed to parse profiles section\n";
            }
        }
        
        if (j.contains("action_maps")) {
            if (!ParseActionMaps(j["action_maps"])) {
                SYSTEM_LOG << "[InputConfig][Warning] Failed to parse action_maps section\n";
            }
        }
        
        if (j.contains("default_profile_assignment")) {
            if (!ParseDefaultAssignments(j["default_profile_assignment"])) {
                SYSTEM_LOG << "[InputConfig][Warning] Failed to parse default_profile_assignment section\n";
            }
        }
        
        if (j.contains("settings")) {
            if (!ParseGlobalSettings(j["settings"])) {
                SYSTEM_LOG << "[InputConfig][Warning] Failed to parse settings section\n";
            }
        }
        
        SYSTEM_LOG << "[InputConfig][Info] Input config loaded successfully\n";
        return true;
    }
    catch (const std::exception& e) {
        SYSTEM_LOG << "[InputConfig][Error] Failed to parse input config: " << e.what() << "\n";
        return false;
    }
}

bool InputConfigLoader::ParseProfiles(const json& j) {
    if (!j.is_array()) {
        SYSTEM_LOG << "[InputConfig][Error] Profiles must be an array\n";
        return false;
    }
    
    for (const auto& profileJson : j) {
        try {
            std::string name = profileJson["name"].get<std::string>();
            std::string deviceTypeStr = profileJson["device_type"].get<std::string>();
            
            InputDeviceType deviceType = InputDeviceType::None;
            if (deviceTypeStr == "Joystick") deviceType = InputDeviceType::Joystick;
            else if (deviceTypeStr == "KeyboardMouse") deviceType = InputDeviceType::KeyboardMouse;
            
            auto profile = std::make_shared<InputProfile>(name, deviceType);
            
            // Parse settings
            if (profileJson.contains("settings")) {
                const auto& settings = profileJson["settings"];
                if (settings.contains("deadzone")) {
                    profile->deadzone = settings["deadzone"].get<float>();
                }
                if (settings.contains("sensitivity")) {
                    profile->sensitivity = settings["sensitivity"].get<float>();
                }
                if (settings.contains("validate_overlaps")) {
                    profile->validateOverlaps = settings["validate_overlaps"].get<bool>();
                }
            }
            
            // Parse actions
            if (profileJson.contains("actions")) {
                const auto& actions = profileJson["actions"];
                for (auto it = actions.begin(); it != actions.end(); ++it) {
                    std::string actionName = it.key();
                    InputBinding binding = ParseInputBinding(it.value());
                    profile->AddAction(actionName, binding);
                }
            }
            
            // Validate profile
            if (profile->validateOverlaps) {
                profile->ValidateNoOverlaps();
            }
            
            // Add profile to manager
            InputDeviceManager::Get().AddProfile(profile);
            
            if (m_logLevel >= 2) {
                SYSTEM_LOG << "[InputConfig][Info] Loaded profile: " << name << " (" << deviceTypeStr << ")\n";
            }
        }
        catch (const std::exception& e) {
            SYSTEM_LOG << "[InputConfig][Error] Failed to parse profile: " << e.what() << "\n";
        }
    }
    
    return true;
}

InputBinding InputConfigLoader::ParseInputBinding(const json& j) {
    InputBinding binding;
    
    std::string typeStr = j["type"].get<std::string>();
    
    if (typeStr == "key") {
        binding.type = InputType::Key;
        if (j.contains("primary")) {
            std::string keyName = j["primary"].get<std::string>();
            binding.primaryInput = static_cast<int>(ParseKeyName(keyName));
        }
        if (j.contains("alternate")) {
            std::string keyName = j["alternate"].get<std::string>();
            binding.alternateInput = static_cast<int>(ParseKeyName(keyName));
        }
    }
    else if (typeStr == "button") {
        binding.type = InputType::Button;
        if (j.contains("button")) {
            binding.primaryInput = j["button"].get<int>();
        }
    }
    else if (typeStr == "axis") {
        binding.type = InputType::Axis;
        if (j.contains("axis")) {
            binding.primaryInput = j["axis"].get<int>();
        }
        if (j.contains("deadzone")) {
            binding.axisDeadzone = j["deadzone"].get<float>();
        }
        if (j.contains("sensitivity")) {
            binding.axisScale = j["sensitivity"].get<float>();
        }
    }
    else if (typeStr == "stick") {
        binding.type = InputType::Stick;
        // Stick binding stores stick name in comment for reference
        if (j.contains("stick")) {
            binding.comment = j["stick"].get<std::string>();
        }
        if (j.contains("deadzone")) {
            binding.axisDeadzone = j["deadzone"].get<float>();
        }
        if (j.contains("sensitivity")) {
            binding.axisScale = j["sensitivity"].get<float>();
        }
    }
    else if (typeStr == "trigger") {
        binding.type = InputType::Trigger;
        if (j.contains("trigger")) {
            binding.comment = j["trigger"].get<std::string>();
        }
        if (j.contains("threshold")) {
            binding.triggerThreshold = j["threshold"].get<float>();
        }
    }
    else if (typeStr == "mouse_button") {
        binding.type = InputType::MouseButton;
        if (j.contains("button")) {
            binding.primaryInput = j["button"].get<int>();
        }
    }
    
    return binding;
}

bool InputConfigLoader::ParseActionMaps(const json& j) {
    if (!j.is_array()) {
        SYSTEM_LOG << "[InputConfig][Error] Action maps must be an array\n";
        return false;
    }
    
    for (const auto& mapJson : j) {
        try {
            std::string name = mapJson["name"].get<std::string>();
            std::string contextStr = mapJson["context"].get<std::string>();
            
            ActionMapContext context = ActionMapContext::Gameplay;
            if (contextStr == "Gameplay") context = ActionMapContext::Gameplay;
            else if (contextStr == "UI") context = ActionMapContext::UI;
            else if (contextStr == "Editor") context = ActionMapContext::Editor;
            else if (contextStr == "System") context = ActionMapContext::System;
            
            int priority = mapJson.value("priority", 0);
            
            ActionMap actionMap(name, context, priority);
            actionMap.exclusive = mapJson.value("exclusive", false);
            
            if (mapJson.contains("actions") && mapJson["actions"].is_array()) {
                for (const auto& action : mapJson["actions"]) {
                    actionMap.AddAction(action.get<std::string>());
                }
            }
            
            InputDeviceManager::Get().AddActionMap(actionMap);
            
            if (m_logLevel >= 2) {
                SYSTEM_LOG << "[InputConfig][Info] Loaded action map: " << name 
                          << " (Context: " << contextStr << ", Priority: " << priority << ")\n";
            }
        }
        catch (const std::exception& e) {
            SYSTEM_LOG << "[InputConfig][Error] Failed to parse action map: " << e.what() << "\n";
        }
    }
    
    return true;
}

bool InputConfigLoader::ParseDefaultAssignments(const json& j) {
    try {
        if (j.contains("Joystick")) {
            std::string profileName = j["Joystick"].get<std::string>();
            InputDeviceManager::Get().SetDefaultProfile(InputDeviceType::Joystick, profileName);
        }
        
        if (j.contains("KeyboardMouse")) {
            std::string profileName = j["KeyboardMouse"].get<std::string>();
            InputDeviceManager::Get().SetDefaultProfile(InputDeviceType::KeyboardMouse, profileName);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        SYSTEM_LOG << "[InputConfig][Error] Failed to parse default assignments: " << e.what() << "\n";
        return false;
    }
}

bool InputConfigLoader::ParseGlobalSettings(const json& j) {
    try {
        if (j.contains("log_level")) {
            std::string logLevel = j["log_level"].get<std::string>();
            InputDeviceManager::Get().SetLogLevel(logLevel);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        SYSTEM_LOG << "[InputConfig][Error] Failed to parse global settings: " << e.what() << "\n";
        return false;
    }
}

bool InputConfigLoader::LoadProfileOverride(const std::string& path) {
    SYSTEM_LOG << "[InputConfig][Info] Loading profile overrides from: " << path << "\n";
    
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            if (m_logLevel >= 2) {
                SYSTEM_LOG << "[InputConfig][Info] No user override file found (this is normal): " << path << "\n";
            }
            return false;  // Not an error, just no overrides
        }
        
        json j;
        file >> j;
        
        // Parse overrides
        if (j.contains("profile_overrides") && j["profile_overrides"].is_array()) {
            for (const auto& overrideJson : j["profile_overrides"]) {
                std::string profileName = overrideJson["profile"].get<std::string>();
                auto profile = InputDeviceManager::Get().GetProfile(profileName);
                
                if (!profile) {
                    SYSTEM_LOG << "[InputConfig][Warning] Cannot override profile '" << profileName << "': not found\n";
                    continue;
                }
                
                if (overrideJson.contains("actions")) {
                    const auto& actions = overrideJson["actions"];
                    for (auto it = actions.begin(); it != actions.end(); ++it) {
                        std::string actionName = it.key();
                        InputBinding binding = ParseInputBinding(it.value());
                        profile->AddAction(actionName, binding);  // Overwrites existing binding
                        
                        if (m_logLevel >= 3) {
                            SYSTEM_LOG << "[InputConfig][Debug] Overrode action '" << actionName 
                                      << "' in profile '" << profileName << "'\n";
                        }
                    }
                }
            }
            
            SYSTEM_LOG << "[InputConfig][Info] Applied user overrides successfully\n";
        }
        
        return true;
    }
    catch (const std::exception& e) {
        SYSTEM_LOG << "[InputConfig][Error] Failed to parse profile overrides: " << e.what() << "\n";
        return false;
    }
}

bool InputConfigLoader::SaveInputConfig(const std::string& path) {
    SYSTEM_LOG << "[InputConfig][Info] Saving input config to: " << path << " (not implemented yet)\n";
    // TODO: Implement config saving
    return false;
}

