/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

CameraEventHandler Implementation: Handles camera events and provides
convenient API for camera control.

*/

#include "CameraEventHandler.h"
#include "EventManager.h"
#include "EventQueue.h"
#include "system_utils.h"
#include "../World.h"


//-------------------------------------------------------------
void CameraEventHandler::Initialize()
{
    if (m_initialized)
        return;
    
    // Register for camera events
    EventManager& em = EventManager::Get();
    
    em.Register(this, EventType::Olympe_EventType_Camera_Shake);
    em.Register(this, EventType::Olympe_EventType_Camera_Shake_Stop);
    em.Register(this, EventType::Olympe_EventType_Camera_Teleport);
    em.Register(this, EventType::Olympe_EventType_Camera_ZoomTo);
    em.Register(this, EventType::Olympe_EventType_Camera_RotateTo);
    em.Register(this, EventType::Olympe_EventType_Camera_Reset);
    em.Register(this, EventType::Olympe_EventType_Camera_SetBounds);
    em.Register(this, EventType::Olympe_EventType_Camera_ClearBounds);
    em.Register(this, EventType::Olympe_EventType_Camera_Target_Follow);
    em.Register(this, EventType::Olympe_EventType_Camera_Target_Unfollow);
    
    m_initialized = true;
    SYSTEM_LOG << "CameraEventHandler initialized\n";
}

//-------------------------------------------------------------
void CameraEventHandler::Shutdown()
{
    if (!m_initialized)
        return;
    
    // Unregister from all events
    EventManager::Get().UnregisterAll(this);
    
    m_initialized = false;
    SYSTEM_LOG << "CameraEventHandler shut down\n";
}

//-------------------------------------------------------------
void CameraEventHandler::OnEvent(const Message& msg)
{
    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Camera_Shake:
            HandleShakeEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_Shake_Stop:
            HandleShakeStopEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_Teleport:
            HandleTeleportEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_ZoomTo:
            HandleZoomEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_RotateTo:
            HandleRotateEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_Reset:
            HandleResetEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_SetBounds:
            HandleBoundsEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_ClearBounds:
            HandleClearBoundsEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_Target_Follow:
            HandleFollowEvent(msg);
            break;
        case EventType::Olympe_EventType_Camera_Target_Unfollow:
            HandleUnfollowEvent(msg);
            break;
        default:
            break;
    }
}

//-------------------------------------------------------------
// Public API implementations
//-------------------------------------------------------------

void CameraEventHandler::TriggerCameraShake(short playerID, float intensity, float duration)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Shake,
        EventDomain::Camera,
        playerID,
        -1
    );
    msg.param1 = intensity;
    msg.param2 = duration;
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::StopCameraShake(short playerID)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Shake_Stop,
        EventDomain::Camera,
        playerID,
        -1
    );
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::TeleportCamera(short playerID, float x, float y)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Teleport,
        EventDomain::Camera,
        playerID,
        -1
    );
    msg.param1 = x;
    msg.param2 = y;
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::ZoomCameraTo(short playerID, float targetZoom, float speed)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_ZoomTo,
        EventDomain::Camera,
        playerID,
        -1
    );
    msg.param1 = targetZoom;
    msg.param2 = speed;
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::RotateCameraTo(short playerID, float targetRotation, float speed)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_RotateTo,
        EventDomain::Camera,
        playerID,
        -1
    );
    msg.param1 = targetRotation;
    msg.param2 = speed;
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::ResetCamera(short playerID)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Reset,
        EventDomain::Camera,
        playerID,
        -1
    );
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::SetCameraBounds(short playerID, const SDL_FRect& bounds)
{
    // Note: SDL_FRect cannot be passed directly through Message
    // We'll need to handle this through direct system access
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<CameraBounds_data>(cameraEntity))
        return;
    
    CameraBounds_data& boundsData = World::Get().GetComponent<CameraBounds_data>(cameraEntity);
    boundsData.useBounds = true;
    boundsData.boundingBox = bounds;
}

//-------------------------------------------------------------
void CameraEventHandler::ClearCameraBounds(short playerID)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_ClearBounds,
        EventDomain::Camera,
        playerID,
        -1
    );
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
void CameraEventHandler::FollowTarget(short playerID, EntityID targetEntity)
{
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    camSys->SetCameraTarget_ECS(cameraEntity, targetEntity);
}

//-------------------------------------------------------------
void CameraEventHandler::UnfollowTarget(short playerID)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Target_Unfollow,
        EventDomain::Camera,
        playerID,
        -1
    );
    
    EventQueue::Get().Push(msg);
}

//-------------------------------------------------------------
// Event handlers
//-------------------------------------------------------------

void CameraEventHandler::HandleShakeEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    float intensity = msg.param1;
    float duration = msg.param2;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<CameraEffects_data>(cameraEntity))
        return;
    
    CameraEffects_data& effects = World::Get().GetComponent<CameraEffects_data>(cameraEntity);
    effects.isShaking = true;
    effects.shakeIntensity = intensity;
    effects.shakeDuration = duration;
    effects.shakeTimeRemaining = duration;
}

//-------------------------------------------------------------
void CameraEventHandler::HandleShakeStopEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<CameraEffects_data>(cameraEntity))
        return;
    
    CameraEffects_data& effects = World::Get().GetComponent<CameraEffects_data>(cameraEntity);
    effects.isShaking = false;
    effects.shakeTimeRemaining = 0.0f;
    effects.shakeOffset = Vector(0.f, 0.f, 0.f);
}

//-------------------------------------------------------------
void CameraEventHandler::HandleTeleportEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    float x = msg.param1;
    float y = msg.param2;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(cameraEntity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
    cam.position = Vector(x, y, 0.f);
}

//-------------------------------------------------------------
void CameraEventHandler::HandleZoomEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    float targetZoom = msg.param1;
    float speed = msg.param2;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(cameraEntity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
    cam.targetZoom = targetZoom;
    
    if (speed > 0.0f)
    {
        cam.zoomSpeed = speed;
    }
    else
    {
        // Instant zoom if speed is 0 or negative
        cam.zoom = targetZoom;
    }
}

//-------------------------------------------------------------
void CameraEventHandler::HandleRotateEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    float targetRotation = msg.param1;
    float speed = msg.param2;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(cameraEntity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
    cam.targetRotation = targetRotation;
    
    if (speed > 0.0f)
    {
        cam.rotationSpeed = speed;
    }
    else
    {
        // Instant rotation if speed is 0 or negative
        cam.rotation = targetRotation;
    }
}

//-------------------------------------------------------------
void CameraEventHandler::HandleResetEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<Camera_data>(cameraEntity))
        return;
    
    Camera_data& cam = World::Get().GetComponent<Camera_data>(cameraEntity);
    cam.controlOffset = Vector(0.f, 0.f, 0.f);
    cam.targetZoom = 1.0f;
    cam.zoom = 1.0f;
    cam.targetRotation = 0.0f;
    cam.rotation = 0.0f;
}

//-------------------------------------------------------------
void CameraEventHandler::HandleBoundsEvent(const Message& msg)
{
    // This is handled directly in SetCameraBounds API method
    // since SDL_FRect cannot be passed through Message
}

//-------------------------------------------------------------
void CameraEventHandler::HandleClearBoundsEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    if (!World::Get().HasComponent<CameraBounds_data>(cameraEntity))
        return;
    
    CameraBounds_data& bounds = World::Get().GetComponent<CameraBounds_data>(cameraEntity);
    bounds.useBounds = false;
}

//-------------------------------------------------------------
void CameraEventHandler::HandleFollowEvent(const Message& msg)
{
    // This is handled directly in FollowTarget API methods
    // since EntityID and GameObject* cannot be passed through Message easily
}

//-------------------------------------------------------------
void CameraEventHandler::HandleUnfollowEvent(const Message& msg)
{
    short playerID = (short)msg.deviceId;
    
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys)
        return;
    
    EntityID cameraEntity = camSys->GetCameraEntityForPlayer(playerID);
    if (cameraEntity == INVALID_ENTITY_ID)
        return;
    
    camSys->ClearCameraTarget(cameraEntity);
}
