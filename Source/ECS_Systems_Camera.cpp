/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

CameraSystem Implementation: Full implementation of the camera system including
input processing, target following, smooth zoom/rotation, and camera effects.

*/

#include "ECS_Systems.h"
#include "ECS_Components.h"
#include "ECS_Register.h"
#include "World.h"
#include "GameEngine.h"
#include "system/KeyboardManager.h"
#include "system/JoystickManager.h"
#include "system/ViewportManager.h"
#include "system/system_utils.h"
#include "system/system_consts.h"
#include <cmath>
#include <random>

#undef min
#undef max

// Random number generator for shake effect
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> shakeDist(-1.0f, 1.0f);

//-------------------------------------------------------------
CameraSystem::CameraSystem()
{
    // Camera system requires at minimum the Camera_data component
    requiredSignature.set(GetComponentTypeID_Static<Camera_data>(), true);
    SYSTEM_LOG << "CameraSystem initialized\n";
}

//-------------------------------------------------------------
void CameraSystem::Process()
{
    float dt = GameEngine::fDt;
    
    // Process all camera entities
    for (EntityID entity : m_entities)
    {
        try
        {
            // 1. Update input (read keyboard/joystick and calculate direction/zoom/rotation)
            UpdateCameraInput(entity, dt);
            
            // 2. Update target following
            UpdateCameraFollow(entity, dt);
            
            // 3. Update zoom (smooth interpolation)
            UpdateCameraZoom(entity, dt);
            
            // 4. Update rotation (smooth interpolation)
            UpdateCameraRotation(entity, dt);
            
            // 5. Update shake effect
            UpdateCameraShake(entity, dt);
            
            // 6. Apply bounds constraints
            ApplyCameraBounds(entity);
        }
        catch (const std::exception& e)
        {
            std::cerr << "CameraSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}

//-------------------------------------------------------------
void CameraSystem::Render()
{
    // Viewport updates could be handled here if needed
    // For now, viewport management is delegated to RenderingSystem
}

//-------------------------------------------------------------
EntityID CameraSystem::CreateCameraForPlayer(short playerID, bool bindToKeyboard)
{
    // Check if camera already exists for this player
    auto it = m_playerCameras.find(playerID);
    if (it != m_playerCameras.end())
    {
        SYSTEM_LOG << "Camera already exists for player " << playerID << "\n";
        return it->second;
    }
    
    // Create new entity
    EntityID cameraEntity = World::Get().CreateEntity();
    
    // Add Camera_data with default values
    Camera_data& cam = World::Get().AddComponent<Camera_data>(cameraEntity);
    cam.playerId = playerID;
	//cam.playerEntityID = 
    cam.type = CameraType::CameraType_2D;
    cam.position = Vector(0.f, 0.f, 0.f);
    cam.zoom = 1.0f;
    cam.targetZoom = 1.0f;
    cam.rotation = 0.0f;
    cam.targetRotation = 0.0f;
    cam.controlMode = CameraControlMode::Mode_Free;
    cam.isActive = true;
	    
    // Get viewport for this player
    SDL_FRect viewportRect;
    if (ViewportManager::Get().GetViewRectForPlayer(playerID, viewportRect))
    {
        cam.viewportRect = viewportRect;
    }
    else
        {
            // Default to full screen if no specific viewport
            cam.viewportRect = { 0.f, 0.f, (float)GameEngine::screenWidth, (float)GameEngine::screenHeight }; // default full screen
	    }
    
    // Add CameraTarget_data
    CameraTarget_data& target = World::Get().AddComponent<CameraTarget_data>(cameraEntity);
    target.followTarget = false;
    target.smoothFactor = 5.0f;
    target.allowManualControl = true;
    target.manualControlDecay = 2.0f;
    
    // Add CameraEffects_data
    CameraEffects_data& effects = World::Get().AddComponent<CameraEffects_data>(cameraEntity);
    effects.isShaking = false;
    effects.shakeIntensity = 0.0f;
    effects.shakeDuration = 0.0f;
    effects.shakeTimeRemaining = 0.0f;
    
    // Add CameraBounds_data
    CameraBounds_data& bounds = World::Get().AddComponent<CameraBounds_data>(cameraEntity);
    bounds.useBounds = false;
    bounds.clampToViewport = true;
    
    // Add CameraInputBinding_data
    CameraInputBinding_data& binding = World::Get().AddComponent<CameraInputBinding_data>(cameraEntity);
    binding.playerId = playerID;
    binding.useKeyboard = bindToKeyboard;
    
    // Bind to keyboard if requested
    if (bindToKeyboard)
    {
        BindCameraToKeyboard(cameraEntity);
    }
    
    // Register in player camera map
    m_playerCameras[playerID] = cameraEntity;
    
    SYSTEM_LOG << "Created camera for player " << playerID << " (Entity " << cameraEntity << ")\n";
    
    return cameraEntity;
}

//-------------------------------------------------------------
void CameraSystem::RemoveCameraForPlayer(short playerID)
{
    auto it = m_playerCameras.find(playerID);
    if (it != m_playerCameras.end())
    {
        World::Get().DestroyEntity(it->second);
        m_playerCameras.erase(it);
        SYSTEM_LOG << "Removed camera for player " << playerID << "\n";
    }
}

//-------------------------------------------------------------
EntityID CameraSystem::GetCameraEntityForPlayer(short playerID)
{
    auto it = m_playerCameras.find(playerID);
    if (it != m_playerCameras.end())
    {
        return it->second;
    }
    return INVALID_ENTITY_ID;
}

//-------------------------------------------------------------
void CameraSystem::BindCameraToKeyboard(EntityID cameraEntity)
{
    if (!World::Get().HasComponent<CameraInputBinding_data>(cameraEntity))
        return;
    
    CameraInputBinding_data& binding = World::Get().GetComponent<CameraInputBinding_data>(cameraEntity);
    binding.useKeyboard = true;
    binding.playerId = -1; // -1 indicates keyboard
    
    m_defaultKeyboardCamera = cameraEntity;
    
    SYSTEM_LOG << "Bound camera " << cameraEntity << " to keyboard\n";
}

//-------------------------------------------------------------
void CameraSystem::BindCameraToJoystick(EntityID cameraEntity, short playerID, SDL_JoystickID joystickId)
{
    if (!World::Get().HasComponent<CameraInputBinding_data>(cameraEntity))
        return;
    
    CameraInputBinding_data& binding = World::Get().GetComponent<CameraInputBinding_data>(cameraEntity);
    binding.useKeyboard = false;
    binding.playerId = playerID;
    binding.joystickId = joystickId;
    
    SYSTEM_LOG << "Bound camera " << cameraEntity << " to joystick " << joystickId << " (Player " << playerID << ")\n";
}

//-------------------------------------------------------------
void CameraSystem::SetCameraTarget_ECS(EntityID cameraEntity, EntityID targetEntity)
{
    if (!World::Get().HasComponent<CameraTarget_data>(cameraEntity))
        return;
    
    CameraTarget_data& target = World::Get().GetComponent<CameraTarget_data>(cameraEntity);
    target.targetEntityID = targetEntity;
    target.followTarget = true;
    
    // Switch to follow mode
    if (World::Get().HasComponent<Camera_data>(cameraEntity))
    {
        Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
        cam.controlMode = CameraControlMode::Mode_FollowWithControl;
    }
    
    SYSTEM_LOG << "Camera " << cameraEntity << " now following entity " << targetEntity << "\n";
}

//-------------------------------------------------------------
void CameraSystem::ClearCameraTarget(EntityID cameraEntity)
{
    if (!World::Get().HasComponent<CameraTarget_data>(cameraEntity))
        return;
    
    CameraTarget_data& target = World::Get().GetComponent<CameraTarget_data>(cameraEntity);
    target.targetEntityID = INVALID_ENTITY_ID;
    target.followTarget = false;
    
    // Switch to free mode
    if (World::Get().HasComponent<Camera_data>(cameraEntity))
    {
        Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
        cam.controlMode = CameraControlMode::Mode_Free;
    }
    
    SYSTEM_LOG << "Camera " << cameraEntity << " target cleared\n";
}

//-------------------------------------------------------------
void CameraSystem::UpdateCameraInput(EntityID entity, float dt)
{
    if (!World::Get().HasComponent<CameraInputBinding_data>(entity))
        return;
    
    CameraInputBinding_data& binding = World::Get().GetComponent<CameraInputBinding_data>(entity);
    
    // Reset input state
    binding.inputDirection = Vector(0.f, 0.f, 0.f);
    binding.rotationInput = 0.0f;
    binding.zoomInput = 0.0f;
    binding.resetRequested = false;
    
    // Process input based on binding type
    if (binding.useKeyboard)
    {
        ProcessKeyboardInput(entity, binding);
    }
    else
    {
        ProcessJoystickInput(entity, binding);
    }
    
    // Apply input to camera
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Apply manual movement (taking rotation into account)
    if (binding.inputDirection.x != 0.0f || binding.inputDirection.y != 0.0f)
    {
        // Rotate input direction by camera rotation
        float rotRad = cam.rotation * (float)(k_PI / 180.0);
        float cosRot = std::cos(rotRad);
        float sinRot = std::sin(rotRad);
        
        float rotatedX = binding.inputDirection.x * cosRot - binding.inputDirection.y * sinRot;
        float rotatedY = binding.inputDirection.x * sinRot + binding.inputDirection.y * cosRot;
        
        cam.controlOffset.x += rotatedX * cam.manualMoveSpeed * dt;
        cam.controlOffset.y += rotatedY * cam.manualMoveSpeed * dt;
    }
    
    // Apply zoom input
    if (binding.zoomInput != 0.0f)
    {
        cam.targetZoom += binding.zoomInput * cam.zoomStep;
        cam.targetZoom = std::max(cam.minZoom, std::min(cam.maxZoom, cam.targetZoom));
    }
    
    // Apply rotation input
    if (binding.rotationInput != 0.0f)
    {
        cam.targetRotation += binding.rotationInput * cam.rotationStep;
        
        // Wrap rotation to -360 to 360
        while (cam.targetRotation > 360.0f) cam.targetRotation -= 360.0f;
        while (cam.targetRotation < -360.0f) cam.targetRotation += 360.0f;
    }
    
    // Handle reset
    if (binding.resetRequested)
    {
        ResetCameraControls(entity);
    }
}

//-------------------------------------------------------------
void CameraSystem::ProcessKeyboardInput(EntityID entity, CameraInputBinding_data& binding)
{
    KeyboardManager& kb = KeyboardManager::Get();
    
    // Directional input
    Vector direction(0.f, 0.f, 0.f);
    
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_up, nullptr)))
        direction.y -= 1.0f;
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_down, nullptr)))
        direction.y += 1.0f;
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_left, nullptr)))
        direction.x -= 1.0f;
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_right, nullptr)))
        direction.x += 1.0f;
    
    binding.inputDirection = direction;
    
    // Rotation input
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_rotate_left, nullptr)))
        binding.rotationInput = -1.0f;
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_rotate_right, nullptr)))
        binding.rotationInput = 1.0f;
    
    // Zoom input
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_zoom_in, nullptr)))
        binding.zoomInput = 1.0f;
    if (kb.IsKeyHeld((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_zoom_out, nullptr)))
        binding.zoomInput = -1.0f;
    
    // Reset input
    if (kb.IsKeyPressed((SDL_Scancode)SDL_GetScancodeFromKey(binding.key_reset, nullptr)))
        binding.resetRequested = true;
}

//-------------------------------------------------------------
void CameraSystem::ProcessJoystickInput(EntityID entity, CameraInputBinding_data& binding)
{
    JoystickManager& joy = JoystickManager::Get();
    
    if (!joy.IsJoystickConnected(binding.joystickId))
        return;
    
    // Read right stick axes for camera movement
    float axisX = joy.GetAxis(binding.joystickId, binding.axis_horizontal);
    float axisY = joy.GetAxis(binding.joystickId, binding.axis_vertical);
    
    // Apply deadzone
    axisX = this->ApplyDeadzone(axisX, binding.deadzone);
    axisY = this->ApplyDeadzone(axisY, binding.deadzone);
    
    binding.inputDirection = Vector(axisX, axisY, 0.f);
    
    // Read triggers for rotation
    float leftTrigger = joy.GetAxis(binding.joystickId, binding.trigger_left);
    float rightTrigger = joy.GetAxis(binding.joystickId, binding.trigger_right);
    
    if (leftTrigger > binding.triggerThreshold)
        binding.rotationInput = -leftTrigger;
    if (rightTrigger > binding.triggerThreshold)
        binding.rotationInput = rightTrigger;
    
    // Reset button
    if (joy.IsButtonPressed(binding.joystickId, binding.button_reset))
        binding.resetRequested = true;
}

//-------------------------------------------------------------
float CameraSystem::ApplyDeadzone(float value, float deadzone)
{
    if (std::abs(value) < deadzone)
        return 0.0f;
    
    // Rescale from deadzone to 1.0
    float sign = (value > 0.0f) ? 1.0f : -1.0f;
    float magnitude = std::abs(value);
    return sign * ((magnitude - deadzone) / (1.0f - deadzone));
}

//-------------------------------------------------------------
void CameraSystem::UpdateCameraFollow(EntityID entity, float dt)
{
    if (!World::Get().HasComponent<CameraTarget_data>(entity))
        return;
    
    CameraTarget_data& target = World::Get().GetComponent<CameraTarget_data>(entity);
    
    if (!target.followTarget)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Get target position
    Vector targetPos(0.f, 0.f, 0.f);
    bool hasValidTarget = false;
    
    // Try ECS entity first
    if (target.targetEntityID != INVALID_ENTITY_ID)
    {
        if (World::Get().HasComponent<Position_data>(target.targetEntityID))
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(target.targetEntityID);
            targetPos = pos.position;
            hasValidTarget = true;
        }
    }
    
    if (!hasValidTarget)
        return;
    
    // Apply follow offset
    targetPos += target.followOffset;
    
    // Smooth follow based on control mode
    if (cam.controlMode == CameraControlMode::Mode_Follow)
    {
        // Strict follow: blend to target and reset manual offset
        cam.position = vBlend(targetPos, cam.position, target.smoothFactor);
        
        // Reset control offset
        cam.controlOffset = Vector(0.f, 0.f, 0.f);
    }
    else if (cam.controlMode == CameraControlMode::Mode_FollowWithControl)
    {
        // Follow with manual control: blend to target but keep manual offset
        cam.position = vBlend(targetPos, cam.position, target.smoothFactor);
        
        // Decay manual control offset back towards zero if allowed
        if (target.allowManualControl && target.manualControlDecay > 0.0f)
        {
			Vector desiredOffset = Vector(0.f, 0.f, 0.f);
            cam.controlOffset = vBlend(desiredOffset, cam.controlOffset, target.manualControlDecay);
            
            // Snap to zero when very close
            if (cam.controlOffset.Length() < 0.1f)
            {
                cam.controlOffset = Vector(0.f, 0.f, 0.f);
            }
        }
    }
}

//-------------------------------------------------------------
void CameraSystem::UpdateCameraZoom(EntityID entity, float dt)
{
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Smooth interpolation towards target zoom
    if (std::abs(cam.zoom - cam.targetZoom) > 0.01f)
    {
        cam.zoom += (cam.targetZoom - cam.zoom) * cam.zoomSpeed * dt;
    }
    else
    {
        // Snap when very close
        cam.zoom = cam.targetZoom;
    }
}

//-------------------------------------------------------------
void CameraSystem::UpdateCameraRotation(EntityID entity, float dt)
{
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Handle wrap-around for rotation (shortest path)
    float diff = cam.targetRotation - cam.rotation;
    
    // Normalize difference to -180 to 180
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    
    // Smooth interpolation towards target rotation
    if (std::abs(diff) > 1.0f)
    {
        cam.rotation += diff * cam.rotationSpeed * dt;
        
        // Wrap rotation to -360 to 360
        while (cam.rotation > 360.0f) cam.rotation -= 360.0f;
        while (cam.rotation < -360.0f) cam.rotation += 360.0f;
    }
    else
    {
        // Snap when very close
        cam.rotation = cam.targetRotation;
    }
}

//-------------------------------------------------------------
void CameraSystem::UpdateCameraShake(EntityID entity, float dt)
{
    if (!World::Get().HasComponent<CameraEffects_data>(entity))
        return;
    
    CameraEffects_data& effects = World::Get().GetComponent<CameraEffects_data>(entity);
    
    if (!effects.isShaking)
    {
        effects.shakeOffset = Vector(0.f, 0.f, 0.f);
        return;
    }
    
    // Decrement shake time
    effects.shakeTimeRemaining -= dt;
    
    if (effects.shakeTimeRemaining <= 0.0f)
    {
        // Shake ended
        effects.isShaking = false;
        effects.shakeOffset = Vector(0.f, 0.f, 0.f);
        effects.shakeTimeRemaining = 0.0f;
    }
    else
    {
        // Generate random shake offset
        float randomX = shakeDist(gen) * effects.shakeIntensity;
        float randomY = shakeDist(gen) * effects.shakeIntensity;
        effects.shakeOffset = Vector(randomX, randomY, 0.f);
    }
}

//-------------------------------------------------------------
void CameraSystem::ApplyCameraBounds(EntityID entity)
{
    if (!World::Get().HasComponent<CameraBounds_data>(entity))
        return;
    
    CameraBounds_data& bounds = World::Get().GetComponent<CameraBounds_data>(entity);
    
    if (!bounds.useBounds)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Calculate effective camera position (position + control offset)
    Vector effectivePos = cam.position + cam.controlOffset;
    
    if (bounds.clampToViewport)
    {
        // Clamp so that viewport stays within bounds
        float halfViewW = cam.viewportRect.w / (2.0f * cam.zoom);
        float halfViewH = cam.viewportRect.h / (2.0f * cam.zoom);
        
        float minX = bounds.boundingBox.x + halfViewW;
        float maxX = bounds.boundingBox.x + bounds.boundingBox.w - halfViewW;
        float minY = bounds.boundingBox.y + halfViewH;
        float maxY = bounds.boundingBox.y + bounds.boundingBox.h - halfViewH;
        
        effectivePos.x = max(minX, std::min(maxX, effectivePos.x));
        effectivePos.y = max(minY, std::min(maxY, effectivePos.y));
    }
    else
    {
        // Simple clamping of camera center
        effectivePos.x = max(bounds.boundingBox.x, min(bounds.boundingBox.x + bounds.boundingBox.w, effectivePos.x));
        effectivePos.y = std::max(bounds.boundingBox.y, min(bounds.boundingBox.y + bounds.boundingBox.h, effectivePos.y));
    }
    
    // Update control offset to reflect clamped position
    cam.controlOffset = effectivePos - cam.position;
}

//-------------------------------------------------------------
void CameraSystem::ResetCameraControls(EntityID entity)
{
    if (!World::Get().HasComponent<Camera_data>(entity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(entity);
    
    // Reset all manual controls
    cam.controlOffset = Vector(0.f, 0.f, 0.f);
    cam.targetZoom = 1.0f;
    cam.targetRotation = 0.0f;
    
    SYSTEM_LOG << "Camera " << entity << " controls reset\n";
}

//-------------------------------------------------------------
void CameraSystem::ApplyCameraToRenderer(SDL_Renderer* renderer, short playerID)
{
    // Find camera for this player
    auto it = m_playerCameras.find(playerID);
    if (it == m_playerCameras.end())
        return;
    
    EntityID cameraEntity = it->second;
    
    if (!World::Get().HasComponent<Camera_data>(cameraEntity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
    
    SDL_Rect viewportRect = {
        static_cast<int>(cam.viewportRect.x),
        static_cast<int>(cam.viewportRect.y),
        static_cast<int>(cam.viewportRect.w),
        static_cast<int>(cam.viewportRect.h)
	};

    // Set viewport
    SDL_SetRenderViewport(renderer, &viewportRect);
}

//-------------------------------------------------------------
void CameraSystem::OnEvent(const Message& msg)
{
    // Event handling will be implemented when CameraEventHandler is created
    // For now, this is a placeholder
}
