/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Camera Components: Component definitions for the advanced camera system.
Supports multiple cameras, split-screen, smooth zoom/rotation, target following,
and various camera effects.

*/

#pragma once

#include "ECS_Entity.h"
#include "vector.h"
#include <SDL3/SDL.h>

// Forward declaration for legacy GameObject support
class GameObject;

// Camera type enumeration
enum class CameraType : uint8_t {
    CameraType_2D = 0,        // Standard 2D camera
    CameraType_2_5D = 1,      // 2.5D camera (follows on X axis only)
    CameraType_Isometric = 2  // Isometric camera
};

// Camera control mode enumeration
enum class CameraControlMode : uint8_t {
    Mode_Free,              // Free camera movement
    Mode_Follow,            // Camera follows target strictly
    Mode_FollowWithControl  // Camera follows target + allows manual control
};

// --- Main Camera Component ---
// Contains all core camera properties including position, zoom, rotation, and control settings
struct Camera_data
{
    short playerId = 0;                      // ID of the player who owns this camera
    CameraType type = CameraType::CameraType_2D; // Type of camera projection
    
    // Position and offset
    Vector position = {0.f, 0.f, 0.f};       // World position of the camera
    Vector baseOffset = {0.f, 0.f, 0.f};     // Base offset for viewport centering
    Vector controlOffset = {0.f, 0.f, 0.f};  // Manual control offset from player
    
    // Zoom management
    float zoom = 1.0f;                       // Current zoom level
    float targetZoom = 1.0f;                 // Target zoom level for smooth transitions
    float zoomSpeed = 5.0f;                  // Speed of zoom interpolation
    float minZoom = 0.1f;                    // Minimum allowed zoom
    float maxZoom = 5.0f;                    // Maximum allowed zoom
    
    // Rotation management (in degrees)
    float rotation = 0.0f;                   // Current rotation angle
    float targetRotation = 0.0f;             // Target rotation angle for smooth transitions
    float rotationSpeed = 5.0f;              // Speed of rotation interpolation
    
    // Control settings
    CameraControlMode controlMode = CameraControlMode::Mode_Free; // Control mode
    SDL_FRect viewportRect = {0.f, 0.f, 800.f, 600.f}; // Viewport rectangle
    
    // Control parameters
    float manualMoveSpeed = 200.0f;          // Speed of manual camera movement (pixels/sec)
    float zoomStep = 0.1f;                   // Zoom increment per input
    float rotationStep = 15.0f;              // Rotation increment per input (degrees)
    
    bool isActive = true;                    // Is this camera active for rendering
};

// --- Camera Target Component ---
// Handles target following for both ECS entities and legacy GameObjects
struct CameraTarget_data
{
    EntityID targetEntityID = INVALID_ENTITY_ID; // ECS entity to follow
    GameObject* targetObject = nullptr;      // Legacy GameObject to follow
    
    bool followTarget = false;               // Enable/disable target following
    float smoothFactor = 5.0f;               // Smoothing factor for following (0-10, higher = smoother)
    Vector followOffset = {0.f, 0.f, 0.f};   // Additional offset when following target
    
    bool allowManualControl = true;          // Allow manual control while following
    float manualControlDecay = 2.0f;         // Speed at which manual offset decays back to target
};

// --- Camera Effects Component ---
// Visual effects such as camera shake
struct CameraEffects_data
{
    bool isShaking = false;                  // Is camera shake active
    float shakeIntensity = 0.0f;             // Intensity of the shake effect
    float shakeDuration = 0.0f;              // Total duration of shake in seconds
    float shakeTimeRemaining = 0.0f;         // Time remaining for shake effect
    Vector shakeOffset = {0.f, 0.f, 0.f};    // Current shake offset applied to position
};

// --- Camera Bounds Component ---
// Constrains camera movement to a specific area
struct CameraBounds_data
{
    bool useBounds = false;                  // Enable/disable boundary constraints
    SDL_FRect boundingBox = {0.f, 0.f, 0.f, 0.f}; // World space bounding box
    bool clampToViewport = true;             // Clamp camera so viewport stays within bounds
};

// --- Camera Input Binding Component ---
// Configures input controls for the camera (keyboard or joystick)
struct CameraInputBinding_data
{
    // Player/device identification
    short playerId = -1;                     // Player ID (-1 = keyboard, >= 0 = joystick)
    SDL_JoystickID joystickId = 0;           // Joystick device ID
    bool useKeyboard = false;                // Use keyboard controls
    
    // Keyboard bindings (using numpad by default)
    SDL_Keycode key_up = SDLK_KP_8;          // Move camera up
    SDL_Keycode key_down = SDLK_KP_2;        // Move camera down
    SDL_Keycode key_left = SDLK_KP_4;        // Move camera left
    SDL_Keycode key_right = SDLK_KP_6;       // Move camera right
    SDL_Keycode key_reset = SDLK_KP_5;       // Reset camera controls
    SDL_Keycode key_rotate_left = SDLK_KP_DIVIDE;   // Rotate camera left
    SDL_Keycode key_rotate_right = SDLK_KP_MULTIPLY; // Rotate camera right
    SDL_Keycode key_zoom_in = SDLK_KP_PLUS;  // Zoom in
    SDL_Keycode key_zoom_out = SDLK_KP_MINUS; // Zoom out
    
    // Joystick bindings
    int axis_horizontal = 2;                 // Right stick horizontal axis
    int axis_vertical = 3;                   // Right stick vertical axis
    int trigger_left = 4;                    // Left trigger for rotation
    int trigger_right = 5;                   // Right trigger for rotation
    int button_reset = 10;                   // Button to reset camera
    
    // Input thresholds
    float deadzone = 0.15f;                  // Deadzone for analog sticks
    float triggerThreshold = 0.3f;           // Threshold for trigger activation
    
    // Current input state (updated each frame)
    Vector inputDirection = {0.f, 0.f, 0.f}; // Normalized input direction
    float rotationInput = 0.0f;              // Rotation input value (-1 to 1)
    float zoomInput = 0.0f;                  // Zoom input value (-1 to 1)
    bool resetRequested = false;             // Reset button pressed this frame
};
