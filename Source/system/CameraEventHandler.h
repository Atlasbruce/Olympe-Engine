/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

CameraEventHandler: Singleton class that handles camera-related events
and provides a public API for camera operations.

*/

#pragma once

#include "message.h"
#include "../ECS_Entity.h"
#include <SDL3/SDL.h>

// Forward declarations
class GameObject;

// Interface for message listeners
class IMessageListener
{
public:
    virtual ~IMessageListener() = default;
    virtual void OnEvent(const Message& msg) = 0;
};

// Camera Event Handler - Singleton that handles camera events
class CameraEventHandler : public IMessageListener
{
public:
    // Singleton access
    static CameraEventHandler& GetInstance()
    {
        static CameraEventHandler instance;
        return instance;
    }
    static CameraEventHandler& Get() { return GetInstance(); }
    
    // Initialization and cleanup
    void Initialize();
    void Shutdown();
    
    // Event handling
    virtual void OnEvent(const Message& msg) override;
    
    // Public API for camera operations
    void TriggerCameraShake(short playerID, float intensity, float duration);
    void StopCameraShake(short playerID);
    void TeleportCamera(short playerID, float x, float y);
    void ZoomCameraTo(short playerID, float targetZoom, float speed);
    void RotateCameraTo(short playerID, float targetRotation, float speed);
    void ResetCamera(short playerID);
    void SetCameraBounds(short playerID, const SDL_FRect& bounds);
    void ClearCameraBounds(short playerID);
    void FollowTarget(short playerID, EntityID targetEntity);
    void FollowTarget_Legacy(short playerID, GameObject* targetObject);
    void UnfollowTarget(short playerID);
    
private:
    CameraEventHandler() = default;
    ~CameraEventHandler() = default;
    
    // Delete copy and move constructors
    CameraEventHandler(const CameraEventHandler&) = delete;
    CameraEventHandler& operator=(const CameraEventHandler&) = delete;
    CameraEventHandler(CameraEventHandler&&) = delete;
    CameraEventHandler& operator=(CameraEventHandler&&) = delete;
    
    // Event handlers
    void HandleShakeEvent(const Message& msg);
    void HandleShakeStopEvent(const Message& msg);
    void HandleTeleportEvent(const Message& msg);
    void HandleZoomEvent(const Message& msg);
    void HandleRotateEvent(const Message& msg);
    void HandleResetEvent(const Message& msg);
    void HandleBoundsEvent(const Message& msg);
    void HandleClearBoundsEvent(const Message& msg);
    void HandleFollowEvent(const Message& msg);
    void HandleUnfollowEvent(const Message& msg);
    
    bool m_initialized = false;
};
