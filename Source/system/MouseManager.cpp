#include "MouseManager.h"
#include "EventQueue.h"
#include <iostream>

MouseManager& MouseManager::GetInstance()
{
    static MouseManager instance;
    return instance;
}

void MouseManager::Initialize()
{
	name = "MouseManager";
    SYSTEM_LOG << "MouseManager Initialized\n";
}

void MouseManager::Shutdown()
{
	SYSTEM_LOG << "MouseManager Shutdown\n";
}

void MouseManager::BeginFrame()
{
    // Reset per-frame mouse state if needed in future
    // For now, no per-frame state to reset
}

void MouseManager::HandleEvent(const SDL_Event* ev)
{
    // if (!ev) return; // useless because it is tested before function call
    switch (ev->type)
    {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            PostButtonEvent(ev->button);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            PostMotionEvent(ev->motion);
            break;
        default:
            break;
    }
}

void MouseManager::PostButtonEvent(const SDL_MouseButtonEvent& be)
{
    Message msg = Message::Create(
        be.down ? EventType::Olympe_EventType_Mouse_ButtonDown : EventType::Olympe_EventType_Mouse_ButtonUp,
        EventDomain::Input,
        static_cast<int>(be.which),
        static_cast<int>(be.button)
    );
    msg.state = be.down ? 1 : 0;
    msg.param1 = be.x;
    msg.param2 = be.y;

    EventQueue::Get().Push(msg);
}

void MouseManager::PostMotionEvent(const SDL_MouseMotionEvent& me)
{
    Message msg = Message::Create(
        EventType::Olympe_EventType_Mouse_Motion,
        EventDomain::Input,
        static_cast<int>(me.which),
        0
    );
    msg.state = 0;
    msg.param1 = me.x;
    msg.param2 = me.y;

    EventQueue::Get().Push(msg);
}
