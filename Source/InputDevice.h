/*
Olympe Engine V2 - 2026
Input System Refactor

This file defines the new input device abstraction layer including:
- InputDeviceSlot: Logical device representation
- InputProfile: Device-specific configuration
- ActionMap: Context-aware action grouping
- InputDeviceManager: Device assignment and management
*/

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

// Forward declarations
class InputProfile;
class ActionMap;

//==============================================================================
// Input Device Types
//==============================================================================

enum class InputDeviceType {
    None,
    Joystick,
    KeyboardMouse
};

enum class InputType {
    Button,
    Key,
    Axis,
    Stick,
    Trigger,
    MouseButton,
    MouseAxis
};

enum class ActionMapContext {
    Gameplay,
    Editor,
    UI,
    System
};

//==============================================================================
// Input Binding Structures
//==============================================================================

struct InputBinding {
    InputType type = InputType::Button;
    
    // Key/button indices
    int primaryInput = -1;      // SDL_Scancode, button index, axis index
    int alternateInput = -1;    // Optional alternate binding
    
    // Axis-specific settings
    float axisScale = 1.0f;
    float axisDeadzone = 0.15f;
    bool invertAxis = false;
    
    // Trigger settings
    float triggerThreshold = 0.1f;
    
    // Description
    std::string comment;
    
    InputBinding() = default;
};

//==============================================================================
// Input Profile - Device-specific configuration
//==============================================================================

class InputProfile {
public:
    InputProfile() = default;
    InputProfile(const std::string& name, InputDeviceType type)
        : profileName(name), deviceType(type) {}
    
    // Profile identification
    std::string profileName;
    InputDeviceType deviceType = InputDeviceType::None;
    std::string description;
    
    // Action mappings (action name -> input binding)
    std::unordered_map<std::string, InputBinding> actionMappings;
    
    // Settings
    float deadzone = 0.15f;
    float sensitivity = 1.0f;
    bool invertYAxis = false;
    bool validateOverlaps = true;  // For keyboard profiles
    
    // Add an action mapping
    void AddAction(const std::string& actionName, const InputBinding& binding) {
        actionMappings[actionName] = binding;
    }
    
    // Get action binding (returns nullptr if not found)
    const InputBinding* GetActionBinding(const std::string& actionName) const {
        auto it = actionMappings.find(actionName);
        if (it != actionMappings.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Validate keyboard profile for overlapping keys
    bool ValidateNoOverlaps() const;
    
    // Initialize default bindings
    void InitializeDefaults();
};

//==============================================================================
// Input Device Slot - Logical device representation
//==============================================================================

struct InputDeviceSlot {
    InputDeviceType type = InputDeviceType::None;
    int deviceIndex = -1;           // SDL_JoystickID for joysticks, -1 for keyboard-mouse
    short assignedPlayerID = -1;    // -1 if unassigned
    bool isConnected = false;
    std::string deviceName;
    std::shared_ptr<InputProfile> profile;  // Device-specific configuration
    
    InputDeviceSlot() = default;
    
    InputDeviceSlot(InputDeviceType t, int idx, const std::string& name)
        : type(t), deviceIndex(idx), deviceName(name), isConnected(true) {}
    
    bool IsAssigned() const { return assignedPlayerID >= 0; }
    bool IsAvailable() const { return isConnected && !IsAssigned(); }
};

//==============================================================================
// Action Map - Context-aware action grouping
//==============================================================================

class ActionMap {
public:
    ActionMap() = default;
    ActionMap(const std::string& name, ActionMapContext ctx, int prio = 0)
        : mapName(name), context(ctx), priority(prio) {}
    
    std::string mapName;
    ActionMapContext context = ActionMapContext::Gameplay;
    int priority = 0;               // Higher = processed first (0-100)
    bool exclusive = false;         // Block lower priority maps when active
    bool enabledByDefault = true;
    std::string description;
    
    std::vector<std::string> actions;  // List of action names in this map
    
    void AddAction(const std::string& actionName) {
        actions.push_back(actionName);
    }
    
    bool ContainsAction(const std::string& actionName) const {
        for (const auto& action : actions) {
            if (action == actionName) return true;
        }
        return false;
    }
};

//==============================================================================
// Input Device Manager - Device assignment and management
//==============================================================================

class InputDeviceManager {
public:
    InputDeviceManager() = default;
    ~InputDeviceManager() = default;
    
    // Singleton access
    static InputDeviceManager& Get() {
        static InputDeviceManager instance;
        return instance;
    }
    
    // Device registration (called when devices connect)
    void RegisterDevice(const InputDeviceSlot& slot);
    void UnregisterDevice(int deviceIndex);
    
    // Auto-assignment (prefers joysticks, then keyboard-mouse)
    InputDeviceSlot* AutoAssignDevice(short playerID);
    
    // Manual assignment
    bool AssignDeviceToPlayer(int deviceIndex, short playerID);
    bool UnassignDevice(short playerID);
    
    // Query
    InputDeviceSlot* GetDeviceForPlayer(short playerID);
    const InputDeviceSlot* GetDeviceForPlayer(short playerID) const;
    std::vector<InputDeviceSlot*> GetAvailableDevices();
    std::vector<InputDeviceSlot*> GetAllDevices();
    
    // Profile management
    void AddProfile(std::shared_ptr<InputProfile> profile);
    std::shared_ptr<InputProfile> GetProfile(const std::string& profileName);
    void SetDefaultProfile(InputDeviceType deviceType, const std::string& profileName);
    
    // Action map management
    void AddActionMap(const ActionMap& actionMap);
    ActionMap* GetActionMap(const std::string& mapName);
    std::vector<ActionMap*> GetActionMapsForContext(ActionMapContext context);
    
    // Logging
    void SetLogLevel(const std::string& level);
    void LogDeviceStatus() const;
    
private:
    // Device slots (deviceIndex -> slot)
    std::unordered_map<int, InputDeviceSlot> m_deviceSlots;
    
    // Player assignments (playerID -> deviceIndex)
    std::unordered_map<short, int> m_playerAssignments;
    
    // Profiles (profileName -> profile)
    std::unordered_map<std::string, std::shared_ptr<InputProfile>> m_profiles;
    
    // Default profile assignments (deviceType -> profileName)
    std::unordered_map<InputDeviceType, std::string> m_defaultProfiles;
    
    // Action maps
    std::vector<ActionMap> m_actionMaps;
    
    // Logging level (0=error, 1=warning, 2=info, 3=debug)
    int m_logLevel = 2;
    
    // Helper: Find first available device (prefers joysticks)
    InputDeviceSlot* FindFirstAvailableDevice();
};

//==============================================================================
// Input Context Manager - Context stack and switching
//==============================================================================

class InputContextManager {
public:
    InputContextManager() : m_contextStack({ActionMapContext::Gameplay}) {}
    ~InputContextManager() = default;
    
    // Singleton access
    static InputContextManager& Get() {
        static InputContextManager instance;
        return instance;
    }
    
    // Context stack operations
    void PushContext(ActionMapContext ctx);
    void PopContext();
    ActionMapContext GetActiveContext() const;
    
    // Configuration
    bool IsEditorEnabled() const { return m_editorEnabled; }
    void SetEditorEnabled(bool enabled);
    
    // Initialization
    void Initialize();
    void LoadConfig(const std::string& configPath);
    
private:
    bool m_editorEnabled = false;
    std::vector<ActionMapContext> m_contextStack;
};

