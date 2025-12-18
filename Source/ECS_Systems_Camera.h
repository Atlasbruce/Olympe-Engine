/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

CameraSystem: ECS system that manages camera entities, handles input,
target following, smooth zoom/rotation, and applies camera transformations.

*/

#pragma once

#include "ECS_Systems.h"
#include "ECS_Components_Camera.h"
#include <unordered_map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include "system/message.h"

// CameraSystem - Manages all camera entities in the ECS
class CameraSystem : public ECS_System
{
public:
    CameraSystem();
    
    // Core ECS system methods
    virtual void Process() override;  // Update cameras each frame
    virtual void Render() override;   // Update viewport settings
    
    // Camera creation and management
    EntityID CreateCameraForPlayer(short playerID, bool bindToKeyboard = false);
    void RemoveCameraForPlayer(short playerID);
    EntityID GetCameraEntityForPlayer(short playerID);
    
    // Input binding
    void BindCameraToKeyboard(EntityID cameraEntity);
    void BindCameraToJoystick(EntityID cameraEntity, short playerID, SDL_JoystickID joystickId);
    
    // Target setting
    void SetCameraTarget_ECS(EntityID cameraEntity, EntityID targetEntity);
    void ClearCameraTarget(EntityID cameraEntity);
    
    // Rendering support
    void ApplyCameraToRenderer(SDL_Renderer* renderer, short playerID);
    
    // Event handling
    void OnEvent(const Message& msg);

private:
    // Update methods called during Process()
    void UpdateCameraInput(EntityID entity, float dt);
    void UpdateCameraFollow(EntityID entity, float dt);
    void UpdateCameraZoom(EntityID entity, float dt);
    void UpdateCameraRotation(EntityID entity, float dt);
    void UpdateCameraShake(EntityID entity, float dt);
    void ApplyCameraBounds(EntityID entity);
    
    // Input processing
    void ProcessKeyboardInput(EntityID entity, CameraInputBinding_data& binding);
    void ProcessJoystickInput(EntityID entity, CameraInputBinding_data& binding);
    float ApplyDeadzone(float value, float deadzone);
    
    // Camera control
    void ResetCameraControls(EntityID entity);
    
    // Player camera mapping
    std::unordered_map<short, EntityID> m_playerCameras; // Map player ID to camera entity
    EntityID m_defaultKeyboardCamera = INVALID_ENTITY_ID; // Default keyboard-controlled camera
};
