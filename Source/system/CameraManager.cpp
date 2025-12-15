#include "CameraManager.h"
#include "EventManager.h"
#include "ViewportManager.h"
#include "../World.h"
#include "../vector.h"
#include "system_utils.h"
#include "../GameEngine.h"

using EM = EventManager;

void CameraManager::Initialize()
{
    // Register to camera-related events
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Shake);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Teleport);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_MoveToPosition);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_ZoomTo);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Reset);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Mode_2D);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Mode_2_5D);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Mode_Isometric);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Target_Follow);
    EM::Get().Register(this, EventType::Olympe_EventType_Camera_Target_Unfollow);

	SYSTEM_LOG << "CameraManager Initialized\n";
}

void CameraManager::Shutdown()
{
    // Unregister all callbacks associated with this instance
    EventManager::Get().UnregisterAll(this);
    m_cameraInstances.clear();
	SYSTEM_LOG << "Camera Shutdown\n";
}

void CameraManager::CreateCameraForPlayer(short playerID)
{
    if (m_cameraInstances.find(playerID) != m_cameraInstances.end()) return;
    CameraInstance inst;
    inst.playerId = playerID;
	// set camera offset to screen center by default
	inst.offset = { -GameEngine::screenWidth / 2.f, -GameEngine::screenHeight / 2.f, 0.0f };
    inst.zoom = 1.0f;
    m_cameraInstances[playerID] = inst;

	// update the camera rectangles according to the viewports
	UpdateCameraRectsInstances();
}

void CameraManager::RemoveCameraForPlayer(short playerID)
{
    auto it = m_cameraInstances.find(playerID);
    if (it != m_cameraInstances.end()) m_cameraInstances.erase(it);
}

CameraManager::CameraInstance CameraManager::GetCameraForPlayer(short playerID = 0) const
{
    if (m_cameraInstances.empty()) 
    {
        CameraInstance d; 
        d.playerId = 0; 
        return d;
	}

    auto it = m_cameraInstances.find(playerID);

    if (it != m_cameraInstances.end()) 
        return it->second;
    // fallback to player 0
    auto it0 = m_cameraInstances.find(0);
    if (it0 != m_cameraInstances.end()) 
        return it0->second;

	// return default
	CameraInstance d;
	return d;
}

Vector CameraManager::GetCameraPositionForActivePlayer(short playerID) const
{
	if (m_cameraInstances.empty()) return Vector();

	short pid = GetActivePlayerID();

	auto it = m_cameraInstances.find(pid);
	if (it != m_cameraInstances.end()) 
        return it->second.position;
	// fallback to player 0
	auto it0 = m_cameraInstances.find(0);
	if (it0 != m_cameraInstances.end()) 
        return it0->second.position;

	// return default
    return Vector();
}

void CameraManager::Process()
{
	//process only if there are camera instances
	if (m_cameraInstances.empty()) return;
    
    // For each camera instance, update position if following target, apply bounds, etc.
    for (auto& kv : m_cameraInstances)
    {
        CameraInstance& cam = kv.second;
		// Follow target entity if set
        if (cam.followTargetEntity && cam.targetEntity != INVALID_ENTITY_ID)
        {
			Position_data& pos = World::Get().GetComponent<Position_data>(cam.targetEntity);
            Vector v2 = pos.position + cam.offset;
            cam.position = vBlend(cam.position, v2, 0.75f);
        }
    }
}

void CameraManager::Apply(SDL_Renderer* renderer)
{
    // Backwards-compatible: set render viewport to the first viewport rectangle
    if (!renderer) return;
    const auto& rects = ViewportManager::Get().GetViewRects();
    if (rects.size() > 0)
    {
		for (const auto& _r : rects)
        {
            const SDL_Rect r = { (int)_r.x, (int)_r.y, (int)_r.w, (int)_r.h };
            SDL_SetRenderViewport(renderer, &r);
        }
    }
}

// New overload: apply viewport for given playerID (use ViewportManager to resolve player -> rect)
void CameraManager::Apply(SDL_Renderer* renderer, short playerID)
{
    if (!renderer) return;
    
    SetActivePlayerID(playerID);

    SDL_FRect rectf;
    if (ViewportManager::Get().GetViewRectForPlayer(playerID, rectf))
    {
        const SDL_Rect r = { (int)rectf.x, (int)rectf.y, (int)rectf.w, (int)rectf.h };
        SDL_SetRenderViewport(renderer, &r);
    }
    else
    {
        // fallback to default behaviour if player not found
        Apply(renderer);
    }
}

void CameraManager::OnEvent(const Message& msg)
{
    if (msg.struct_type != EventStructType::EventStructType_Olympe)
		return;

    // Generic event handler that updates camera instances based on message payload.
    // Messages can target specific player via msg.deviceId (player index) or default to 0.
    short playerID = 0;
    if (msg.param1 >= 0) 
        playerID = static_cast<short>(msg.param1);

    auto it = m_cameraInstances.find(playerID);
    if (it == m_cameraInstances.end())
    {
        // if not present create it
        CreateCameraForPlayer(playerID);
        it = m_cameraInstances.find(playerID);
    }
    CameraInstance& cam = it->second;

    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Camera_Teleport:
        {

            break;
        }
        case EventType::Olympe_EventType_Camera_MoveToPosition:
        {
            // For now perform instant move (no blending) - could be extended
            cam.position.x = msg.param1;
            cam.position.y = msg.param2;
            break;
        }
        case EventType::Olympe_EventType_Camera_ZoomTo:
        {
            cam.zoom = msg.param1;
            break;
        }
        case EventType::Olympe_EventType_Camera_RotateTo:
        {

            break;
        }
        case EventType::Olympe_EventType_Camera_Reset:
        {
			cam.position.x = 0; cam.position.y = 0; cam.zoom = 1.0f; cam.followTargetEntity = false; cam.targetEntity = INVALID_ENTITY_ID;
            break;
        }
        case EventType::Olympe_EventType_Camera_Mode_2D:
            cam.type = CameraType::CameraType_2D;
            break;
        case EventType::Olympe_EventType_Camera_Mode_2_5D:
            cam.type = CameraType::CameraType_2_5D;
            break;
        case EventType::Olympe_EventType_Camera_Mode_Isometric:
            cam.type = CameraType::CameraType_Isometric;
            break;
        case EventType::Olympe_EventType_Camera_Target_Follow:
        {
            // follow target specified by msg.controlId as UID
			cam.followTargetEntity = false;

			//Camera follows an Entity specified by its EntityID
			if (msg.targetUid != INVALID_ENTITY_ID)
            {
				// follow entity if valid and exists in world
				if (World::Get().IsEntityValid(msg.targetUid))
                {
                    cam.targetEntity = msg.targetUid;
                    cam.followTargetEntity = true;
                }
                else
                    {
                        SYSTEM_LOG << "CameraManager::OnEvent: Cannot follow entity with invalid EntityID " << msg.targetUid << "\n";
				    }
            }

            UpdateCameraRectsInstances();
            break;
        }
        case EventType::Olympe_EventType_Camera_Target_Unfollow:
        {
			cam.followTargetEntity = false;
            break;
        }
        default:
            break;
    }
}

void CameraManager::UpdateCameraRectsInstances()
{
    // Update camera rectangles according to the viewports
    const auto& rects = ViewportManager::Get().GetViewRects();
    size_t index = 0;
    for (auto& kv : m_cameraInstances)
    {
        if (index >= rects.size()) break;
        const auto& r = rects[index];
        // Update camera offset based on viewport size
        CameraInstance& cam = kv.second;
        cam.offset = { -r.w / 2.f, -r.h / 2.f, 0.0f };
        index++;
	}
}
