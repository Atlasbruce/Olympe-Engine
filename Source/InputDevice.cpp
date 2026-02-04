/*
Olympe Engine V2 - 2026
Input System Refactor - Implementation

Implementation of the new input device abstraction layer.
*/

#include "InputDevice.h"
#include "system_utils.h"
#include <algorithm>
#include <unordered_set>

//==============================================================================
// InputProfile Implementation
//==============================================================================

bool InputProfile::ValidateNoOverlaps() const {
    if (deviceType != InputDeviceType::KeyboardMouse || !validateOverlaps) {
        return true;  // Only validate keyboard profiles when enabled
    }
    
    std::unordered_set<int> usedKeys;
    bool hasOverlap = false;
    
    for (const auto& [actionName, binding] : actionMappings) {
        if (binding.type == InputType::Key) {
            // Check primary key
            if (binding.primaryInput != -1) {
                if (usedKeys.count(binding.primaryInput) > 0) {
                    SYSTEM_LOG << "[InputProfile][Error] Keyboard profile '" << profileName 
                              << "' has overlapping keys for action: " << actionName << "\n";
                    hasOverlap = true;
                } else {
                    usedKeys.insert(binding.primaryInput);
                }
            }
            
            // Check alternate key
            if (binding.alternateInput != -1) {
                if (usedKeys.count(binding.alternateInput) > 0) {
                    SYSTEM_LOG << "[InputProfile][Error] Keyboard profile '" << profileName 
                              << "' has overlapping alternate key for action: " << actionName << "\n";
                    hasOverlap = true;
                } else {
                    usedKeys.insert(binding.alternateInput);
                }
            }
        }
    }
    
    if (hasOverlap) {
        SYSTEM_LOG << "[InputProfile][Error] Profile validation failed: " << profileName << "\n";
        return false;
    }
    
    SYSTEM_LOG << "[InputProfile][Info] Profile validation passed: " << profileName << "\n";
    return true;
}

void InputProfile::InitializeDefaults() {
    // Initialize default bindings based on device type
    if (deviceType == InputDeviceType::KeyboardMouse) {
        // Default keyboard bindings (WASD + Arrows)
        InputBinding moveUp;
        moveUp.type = InputType::Key;
        moveUp.primaryInput = SDL_SCANCODE_W;
        moveUp.alternateInput = SDL_SCANCODE_UP;
        AddAction("move_up", moveUp);
        
        InputBinding moveDown;
        moveDown.type = InputType::Key;
        moveDown.primaryInput = SDL_SCANCODE_S;
        moveDown.alternateInput = SDL_SCANCODE_DOWN;
        AddAction("move_down", moveDown);
        
        InputBinding moveLeft;
        moveLeft.type = InputType::Key;
        moveLeft.primaryInput = SDL_SCANCODE_A;
        moveLeft.alternateInput = SDL_SCANCODE_LEFT;
        AddAction("move_left", moveLeft);
        
        InputBinding moveRight;
        moveRight.type = InputType::Key;
        moveRight.primaryInput = SDL_SCANCODE_D;
        moveRight.alternateInput = SDL_SCANCODE_RIGHT;
        AddAction("move_right", moveRight);
        
        InputBinding jump;
        jump.type = InputType::Key;
        jump.primaryInput = SDL_SCANCODE_SPACE;
        AddAction("jump", jump);
        
        InputBinding shoot;
        shoot.type = InputType::Key;
        shoot.primaryInput = SDL_SCANCODE_LCTRL;
        AddAction("shoot", shoot);
        
        InputBinding interact;
        interact.type = InputType::Key;
        interact.primaryInput = SDL_SCANCODE_E;
        AddAction("interact", interact);
    }
    else if (deviceType == InputDeviceType::Joystick) {
        // Default gamepad bindings
        InputBinding jump;
        jump.type = InputType::Button;
        jump.primaryInput = 0;  // A button
        AddAction("jump", jump);
        
        InputBinding shoot;
        shoot.type = InputType::Button;
        shoot.primaryInput = 1;  // B button
        AddAction("shoot", shoot);
        
        InputBinding interact;
        interact.type = InputType::Button;
        interact.primaryInput = 2;  // X button
        AddAction("interact", interact);
    }
    
    SYSTEM_LOG << "[InputProfile][Info] Initialized default bindings for profile: " << profileName << "\n";
}

//==============================================================================
// InputDeviceManager Implementation
//==============================================================================

void InputDeviceManager::RegisterDevice(const InputDeviceSlot& slot) {
    m_deviceSlots[slot.deviceIndex] = slot;
    
    // Assign default profile if available
    auto defaultProfileIt = m_defaultProfiles.find(slot.type);
    if (defaultProfileIt != m_defaultProfiles.end()) {
        auto profile = GetProfile(defaultProfileIt->second);
        if (profile) {
            m_deviceSlots[slot.deviceIndex].profile = profile;
            if (m_logLevel >= 2) {
                SYSTEM_LOG << "[InputDevice][Info] Assigned profile '" << profile->profileName 
                          << "' to device: " << slot.deviceName << " (ID: " << slot.deviceIndex << ")\n";
            }
        }
    }
    
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputDevice][Info] Device registered: " << slot.deviceName 
                  << " (Type: " << (slot.type == InputDeviceType::Joystick ? "Joystick" : "KeyboardMouse")
                  << ", ID: " << slot.deviceIndex << ")\n";
    }
}

void InputDeviceManager::UnregisterDevice(int deviceIndex) {
    auto it = m_deviceSlots.find(deviceIndex);
    if (it != m_deviceSlots.end()) {
        if (m_logLevel >= 2) {
            SYSTEM_LOG << "[InputDevice][Info] Device unregistered: " << it->second.deviceName 
                      << " (ID: " << deviceIndex << ")\n";
        }
        
        // Remove player assignment if any
        if (it->second.IsAssigned()) {
            m_playerAssignments.erase(it->second.assignedPlayerID);
        }
        
        m_deviceSlots.erase(it);
    }
}

InputDeviceSlot* InputDeviceManager::AutoAssignDevice(short playerID) {
    // Check if player already has a device
    auto assignmentIt = m_playerAssignments.find(playerID);
    if (assignmentIt != m_playerAssignments.end()) {
        auto deviceIt = m_deviceSlots.find(assignmentIt->second);
        if (deviceIt != m_deviceSlots.end() && deviceIt->second.isConnected) {
            if (m_logLevel >= 1) {
                SYSTEM_LOG << "[InputDevice][Warning] Player " << playerID 
                          << " already has device assigned: " << deviceIt->second.deviceName << "\n";
            }
            return &deviceIt->second;
        }
    }
    
    // Find first available device (prefer joysticks)
    InputDeviceSlot* availableDevice = FindFirstAvailableDevice();
    
    if (availableDevice) {
        availableDevice->assignedPlayerID = playerID;
        m_playerAssignments[playerID] = availableDevice->deviceIndex;
        
        if (m_logLevel >= 2) {
            SYSTEM_LOG << "[InputDevice][Info] Auto-assigned " 
                      << (availableDevice->type == InputDeviceType::Joystick ? "Joystick" : "KeyboardMouse")
                      << " " << availableDevice->deviceIndex << " to Player " << playerID 
                      << " (" << availableDevice->deviceName << ")\n";
        }
        
        return availableDevice;
    }
    
    if (m_logLevel >= 1) {
        SYSTEM_LOG << "[InputDevice][Warning] No available devices to assign to Player " << playerID << "\n";
    }
    return nullptr;
}

bool InputDeviceManager::AssignDeviceToPlayer(int deviceIndex, short playerID) {
    auto deviceIt = m_deviceSlots.find(deviceIndex);
    if (deviceIt == m_deviceSlots.end()) {
        if (m_logLevel >= 0) {
            SYSTEM_LOG << "[InputDevice][Error] Cannot assign device " << deviceIndex 
                      << " to Player " << playerID << ": device not found\n";
        }
        return false;
    }
    
    InputDeviceSlot& device = deviceIt->second;
    
    if (!device.isConnected) {
        if (m_logLevel >= 0) {
            SYSTEM_LOG << "[InputDevice][Error] Cannot assign device " << deviceIndex 
                      << " to Player " << playerID << ": device not connected\n";
        }
        return false;
    }
    
    if (device.IsAssigned() && device.assignedPlayerID != playerID) {
        if (m_logLevel >= 1) {
            SYSTEM_LOG << "[InputDevice][Warning] Device " << deviceIndex 
                      << " already assigned to Player " << device.assignedPlayerID 
                      << ", reassigning to Player " << playerID << "\n";
        }
        m_playerAssignments.erase(device.assignedPlayerID);
    }
    
    device.assignedPlayerID = playerID;
    m_playerAssignments[playerID] = deviceIndex;
    
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputDevice][Info] Assigned device " << deviceIndex 
                  << " (" << device.deviceName << ") to Player " << playerID << "\n";
    }
    
    return true;
}

bool InputDeviceManager::UnassignDevice(short playerID) {
    auto assignmentIt = m_playerAssignments.find(playerID);
    if (assignmentIt == m_playerAssignments.end()) {
        if (m_logLevel >= 1) {
            SYSTEM_LOG << "[InputDevice][Warning] Player " << playerID << " has no device assigned\n";
        }
        return false;
    }
    
    int deviceIndex = assignmentIt->second;
    auto deviceIt = m_deviceSlots.find(deviceIndex);
    if (deviceIt != m_deviceSlots.end()) {
        deviceIt->second.assignedPlayerID = -1;
    }
    
    m_playerAssignments.erase(assignmentIt);
    
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputDevice][Info] Unassigned device from Player " << playerID << "\n";
    }
    
    return true;
}

InputDeviceSlot* InputDeviceManager::GetDeviceForPlayer(short playerID) {
    auto assignmentIt = m_playerAssignments.find(playerID);
    if (assignmentIt != m_playerAssignments.end()) {
        auto deviceIt = m_deviceSlots.find(assignmentIt->second);
        if (deviceIt != m_deviceSlots.end()) {
            return &deviceIt->second;
        }
    }
    return nullptr;
}

const InputDeviceSlot* InputDeviceManager::GetDeviceForPlayer(short playerID) const {
    auto assignmentIt = m_playerAssignments.find(playerID);
    if (assignmentIt != m_playerAssignments.end()) {
        auto deviceIt = m_deviceSlots.find(assignmentIt->second);
        if (deviceIt != m_deviceSlots.end()) {
            return &deviceIt->second;
        }
    }
    return nullptr;
}

std::vector<InputDeviceSlot*> InputDeviceManager::GetAvailableDevices() {
    std::vector<InputDeviceSlot*> available;
    for (auto& [idx, slot] : m_deviceSlots) {
        if (slot.IsAvailable()) {
            available.push_back(&slot);
        }
    }
    return available;
}

std::vector<InputDeviceSlot*> InputDeviceManager::GetAllDevices() {
    std::vector<InputDeviceSlot*> all;
    for (auto& [idx, slot] : m_deviceSlots) {
        all.push_back(&slot);
    }
    return all;
}

void InputDeviceManager::AddProfile(std::shared_ptr<InputProfile> profile) {
    m_profiles[profile->profileName] = profile;
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputProfile][Info] Added profile: " << profile->profileName << "\n";
    }
}

std::shared_ptr<InputProfile> InputDeviceManager::GetProfile(const std::string& profileName) {
    auto it = m_profiles.find(profileName);
    if (it != m_profiles.end()) {
        return it->second;
    }
    return nullptr;
}

void InputDeviceManager::SetDefaultProfile(InputDeviceType deviceType, const std::string& profileName) {
    m_defaultProfiles[deviceType] = profileName;
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputProfile][Info] Set default profile for " 
                  << (deviceType == InputDeviceType::Joystick ? "Joystick" : "KeyboardMouse")
                  << ": " << profileName << "\n";
    }
}

void InputDeviceManager::AddActionMap(const ActionMap& actionMap) {
    m_actionMaps.push_back(actionMap);
    if (m_logLevel >= 2) {
        SYSTEM_LOG << "[InputContext][Info] Added action map: " << actionMap.mapName 
                  << " (Priority: " << actionMap.priority << ")\n";
    }
}

ActionMap* InputDeviceManager::GetActionMap(const std::string& mapName) {
    for (auto& map : m_actionMaps) {
        if (map.mapName == mapName) {
            return &map;
        }
    }
    return nullptr;
}

std::vector<ActionMap*> InputDeviceManager::GetActionMapsForContext(ActionMapContext context) {
    std::vector<ActionMap*> maps;
    for (auto& map : m_actionMaps) {
        if (map.context == context) {
            maps.push_back(&map);
        }
    }
    
    // Sort by priority (highest first)
    std::sort(maps.begin(), maps.end(), [](ActionMap* a, ActionMap* b) {
        return a->priority > b->priority;
    });
    
    return maps;
}

void InputDeviceManager::SetLogLevel(const std::string& level) {
    if (level == "error") m_logLevel = 0;
    else if (level == "warning") m_logLevel = 1;
    else if (level == "info") m_logLevel = 2;
    else if (level == "debug") m_logLevel = 3;
    
    SYSTEM_LOG << "[InputDevice][Info] Log level set to: " << level << "\n";
}

void InputDeviceManager::LogDeviceStatus() const {
    SYSTEM_LOG << "[InputDevice][Info] === Device Status ===\n";
    SYSTEM_LOG << "[InputDevice][Info] Total devices: " << m_deviceSlots.size() << "\n";
    
    for (const auto& [idx, slot] : m_deviceSlots) {
        SYSTEM_LOG << "[InputDevice][Info]   Device " << idx << ": " << slot.deviceName 
                  << " (Type: " << (slot.type == InputDeviceType::Joystick ? "Joystick" : "KeyboardMouse")
                  << ", Connected: " << (slot.isConnected ? "Yes" : "No")
                  << ", Assigned: " << (slot.IsAssigned() ? "Player " + std::to_string(slot.assignedPlayerID) : "None")
                  << ", Profile: " << (slot.profile ? slot.profile->profileName : "None") << ")\n";
    }
    
    SYSTEM_LOG << "[InputDevice][Info] ====================\n";
}

InputDeviceSlot* InputDeviceManager::FindFirstAvailableDevice() {
    // First, try to find available joystick (preferred)
    for (auto& [idx, slot] : m_deviceSlots) {
        if (slot.type == InputDeviceType::Joystick && slot.IsAvailable()) {
            return &slot;
        }
    }
    
    // If no joystick available, try keyboard-mouse
    for (auto& [idx, slot] : m_deviceSlots) {
        if (slot.type == InputDeviceType::KeyboardMouse && slot.IsAvailable()) {
            return &slot;
        }
    }
    
    return nullptr;
}

//==============================================================================
// InputContextManager Implementation
//==============================================================================

void InputContextManager::PushContext(ActionMapContext ctx) {
    m_contextStack.push_back(ctx);
    SYSTEM_LOG << "[InputContext][Info] Pushed context: " 
              << (ctx == ActionMapContext::Gameplay ? "Gameplay" :
                  ctx == ActionMapContext::UI ? "UI" :
                  ctx == ActionMapContext::Editor ? "Editor" : "System")
              << " (Stack size: " << m_contextStack.size() << ")\n";
}

void InputContextManager::PopContext() {
    if (m_contextStack.size() > 1) {
        ActionMapContext poppedContext = m_contextStack.back();
        m_contextStack.pop_back();
        SYSTEM_LOG << "[InputContext][Info] Popped context: " 
                  << (poppedContext == ActionMapContext::Gameplay ? "Gameplay" :
                      poppedContext == ActionMapContext::UI ? "UI" :
                      poppedContext == ActionMapContext::Editor ? "Editor" : "System")
                  << " (Stack size: " << m_contextStack.size() << ")\n";
    } else {
        SYSTEM_LOG << "[InputContext][Warning] Cannot pop context: stack has only one element\n";
    }
}

ActionMapContext InputContextManager::GetActiveContext() const {
    if (!m_contextStack.empty()) {
        return m_contextStack.back();
    }
    return ActionMapContext::Gameplay;
}

void InputContextManager::SetEditorEnabled(bool enabled) {
    m_editorEnabled = enabled;
    SYSTEM_LOG << "[InputContext][Info] Editor mode " << (enabled ? "enabled" : "disabled") << "\n";
    
    if (enabled) {
        // Push editor context if not already there
        if (GetActiveContext() != ActionMapContext::Editor) {
            PushContext(ActionMapContext::Editor);
        }
    } else {
        // Pop editor context if it's active
        if (GetActiveContext() == ActionMapContext::Editor) {
            PopContext();
        }
    }
}

void InputContextManager::Initialize() {
    m_contextStack = {ActionMapContext::Gameplay};
    SYSTEM_LOG << "[InputContext][Info] Initialized with Gameplay context\n";
}

void InputContextManager::LoadConfig(const std::string& configPath) {
    // This will be implemented in InputConfigLoader
    SYSTEM_LOG << "[InputContext][Info] Loading config from: " << configPath << "\n";
}

