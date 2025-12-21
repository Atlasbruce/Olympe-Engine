#include "AI_Player.h"
#include "GameObject.h"
#include "ObjectFactory.h"
#include "system/EventManager.h"
#include "system/system_consts.h"
#include <iostream>
#include <cmath>
#include "InputsManager.h"
#include "VideoGame.h"
#include "Player.h"
#include "drawing.h"

using namespace std;
using EM = EventManager;
using IM = InputsManager;

bool AI_Player::FactoryRegistered =  ObjectFactory::Get().Register("AI_Player", AI_Player::Create);
ObjectComponent* AI_Player::Create()
{
    return new AI_Player();
}

AI_Player::AI_Player()
{
    // Register to input-related events
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_AxisMotion);
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_ButtonDown);
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_ButtonUp);
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_Disconnected);
    EM::Get().Register(this, EventType::Olympe_EventType_Joystick_Connected);

    EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyDown);
    EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyUp);	
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_Disconnected);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_Connected);

}
void AI_Player::Initialize()
{
	((Player*)gao)->m_PlayerID = ++VideoGame::m_playerIdCounter;
	IM::Get().AddPlayerObjectIndex(((Player*)gao)->m_PlayerID, (Player*)gao);

    IM::Get().AutoBindControllerToPlayer(((Player*)gao)->m_PlayerID); // ensure inputs are bound

	if (IM::Get().IsPlayerBound(((Player*)gao)->m_PlayerID))
    {
        ((Player*)gao)->m_ControllerID = InputsManager::Get().GetPlayerBinding(((Player*)gao)->m_PlayerID);
		SYSTEM_LOG << "AI_Player created with PlayerID=" << ((Player*)gao)->m_PlayerID << " bound to ControllerID=" << ((Player*)gao)->m_ControllerID << endl;
    }
    else
    {
		SYSTEM_LOG << "Error: AI_Player creation failed to bind any inputs for PlayerID=" << ((Player*)gao)->m_PlayerID << endl;
    }
}

AI_Player::~AI_Player()
{
	VideoGame::m_playerIdCounter--;
    // Unregister all callbacks associated with this instance
    EventManager::Get().UnregisterAll(this);
}

void AI_Player::SetOwner ( Object *_owner)
{
	AIComponent::SetOwner(_owner);
}

void AI_Player::RenderDebug()
{

    SDL_SetRenderDrawColor(GameEngine::renderer, m_debugcolor.r, m_debugcolor.g, m_debugcolor.b, SDL_ALPHA_OPAQUE);
    Vector vRenderPos = gao->GetPosition() - CameraManager::Get().GetCameraPositionForActivePlayer();
	Draw_FilledCircle(GameEngine::renderer, (int)vRenderPos.x, (int)vRenderPos.y, 5);
    //gao->boundingBox = { gao->position.x, gao->position.y, gao->width, gao->height };
	SDL_FRect box = gao->GetBoundingBox();
    SDL_RenderRect(GameEngine::renderer, &box);/**/

}


void AI_Player::Process()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    float vx = 0.0f;
    float vy = 0.0f;

    // joystick axes take priority when outside deadzone
    if (std::fabs(m_axisX) > m_deadzone) vx = m_axisX * m_speed;
    else
    {
        if (m_keyLeft) vx = -m_speed;
        if (m_keyRight) vx = m_keyRight ? m_speed : vx;
    }

    if (std::fabs(m_axisY) > m_deadzone) vy = m_axisY * m_speed;
    else
    {
        if (m_keyUp) vy = -m_speed;
        if (m_keyDown) vy = m_keyDown ? m_speed : vy;
    }
    // position
	Vector vtmp = gao->GetPosition() + Vector(vx * fDt, vy * fDt, 0.0f);
	gao->SetPosition(vtmp); 
//    gao->position.x += vx * fDt;
//    gao->position.y += vy * fDt;

}

void AI_Player::OnEvent(const Message& msg)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // is the deviceId matching our controller?
    if (msg.deviceId != ((Player*)gao)->m_ControllerID)
        return;

    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Joystick_AxisMotion:
        {
            // controlId = axis index, value = normalized [-1..1]
            if (msg.controlId == 0)
                m_axisX = msg.param1;
            else if (msg.controlId == 1)
                m_axisY = msg.param1;
            break;
        }

        case EventType::Olympe_EventType_Joystick_ButtonDown:
        case EventType::Olympe_EventType_Joystick_ButtonUp:
        {
            // optionally map buttons to movement (e.g. dpad) if desired
            // msg.controlId is button index, msg.state 1=down 0=up
            // Not implemented here.
            break;
        }

        case EventType::Olympe_EventType_Keyboard_KeyDown:
        {
            // msg.controlId contains SDL_Scancode
            switch (static_cast<SDL_Scancode>(msg.controlId))
            {
            case SDL_SCANCODE_Z:
            case SDL_SCANCODE_UP:
                m_keyUp = true; break;
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
                m_keyDown = true; break;
            case SDL_SCANCODE_Q:
            case SDL_SCANCODE_LEFT:
                m_keyLeft = true; break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                m_keyRight = true; break;
            default:
                break;
            }
            break;
        }

        case EventType::Olympe_EventType_Keyboard_KeyUp:
        {
            switch (static_cast<SDL_Scancode>(msg.controlId))
            {
            case SDL_SCANCODE_W:
            case SDL_SCANCODE_UP:
                m_keyUp = false; break;
            case SDL_SCANCODE_S:
            case SDL_SCANCODE_DOWN:
                m_keyDown = false; break;
            case SDL_SCANCODE_A:
            case SDL_SCANCODE_LEFT:
                m_keyLeft = false; break;
            case SDL_SCANCODE_D:
            case SDL_SCANCODE_RIGHT:
                m_keyRight = false; break;
            default:
                break;
            }
            break;
        }
        case EventType::Olympe_EventType_Joystick_Disconnected:
        {
			SYSTEM_LOG << "AI_Player: Controller disconnected for GameObject: " << name << endl;
			m_debugcolor = { 255, 0, 0, 255 };
            // if our controller got disconnected, reset inputs
            break;
        }
        case EventType::Olympe_EventType_Keyboard_Disconnected:
        {
			SYSTEM_LOG << "AI_Player: Keyboard disconnected for GameObject: " << name << endl;
            m_debugcolor = { 255, 0, 0, 255 };
            // if our keyboard got disconnected, reset inputs
            break;
		}
        case EventType::Olympe_EventType_Joystick_Connected:
        {
			SYSTEM_LOG << "AI_Player: Controller connected for GameObject: " << name << endl;
            m_debugcolor = { 0, 250, 0, 255 };
            break;
		}
        case EventType::Olympe_EventType_Keyboard_Connected:
        {
			SYSTEM_LOG << "AI_Player: Keyboard connected for GameObject: " << name << endl;
            m_debugcolor = { 0, 250, 0, 255 };
			break;
		}

        default:
            break;
    }
}
