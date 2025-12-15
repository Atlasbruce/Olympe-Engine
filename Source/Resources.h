/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Resources purpose: Centralized registry of all global resources for ECS systems.
This removes the need for implicit singleton access and makes dependencies explicit.
*/

#pragma once

#include <SDL3/SDL.h>

// Forward declarations to avoid circular dependencies
class EventManager;
class InputsManager;
class CameraManager;
class KeyboardManager;
class JoystickManager;
class MouseManager;
class DataManager;
class World;

// Resources: Centralized registry of all global resources
// This struct is passed explicitly to ECS systems, making dependencies clear
// and facilitating testing, debugging, and potential multi-instance scenarios
struct Resources
{
    // SDL Rendering Context
    SDL_Renderer* renderer = nullptr;
    
    // Timing
    float deltaTime = 0.0f;  // Time since last frame (in seconds)
    
    // Display Configuration
    int screenWidth = 640;
    int screenHeight = 320;
    
    // System Managers (non-owning pointers)
    EventManager* eventManager = nullptr;
    InputsManager* inputsManager = nullptr;
    CameraManager* cameraManager = nullptr;
    KeyboardManager* keyboardManager = nullptr;
    JoystickManager* joystickManager = nullptr;
    MouseManager* mouseManager = nullptr;
    DataManager* dataManager = nullptr;
    
    // ECS World (for component access)
    World* world = nullptr;
    
    // Default constructor
    Resources() = default;
    
    // Utility method to check if all required resources are initialized
    bool IsValid() const
    {
        return renderer != nullptr &&
               eventManager != nullptr &&
               inputsManager != nullptr &&
               cameraManager != nullptr &&
               keyboardManager != nullptr &&
               joystickManager != nullptr &&
               mouseManager != nullptr &&
               dataManager != nullptr &&
               world != nullptr;
    }
};
