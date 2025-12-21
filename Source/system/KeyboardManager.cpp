#include "KeyboardManager.h"
#include "EventQueue.h"
#include <iostream>
#include "system_utils.h"
#include <cstring>

KeyboardManager& KeyboardManager::GetInstance()
{
    static KeyboardManager instance;
    return instance;
}

void KeyboardManager::Initialize()
{
    // Initialize state arrays
    std::memset(m_keyStates, 0, sizeof(m_keyStates));
    std::memset(m_keysPressedThisFrame, 0, sizeof(m_keysPressedThisFrame));
    std::memset(m_keysReleasedThisFrame, 0, sizeof(m_keysReleasedThisFrame));
	SYSTEM_LOG << "KeyboardManager created and Initialized\n";
}

void KeyboardManager::Shutdown()
{
    SYSTEM_LOG << "KeyboardManager deleted\n";
}

void KeyboardManager::BeginFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // Reset per-frame edge detection arrays
    std::memset(m_keysPressedThisFrame, 0, sizeof(m_keysPressedThisFrame));
    std::memset(m_keysReleasedThisFrame, 0, sizeof(m_keysReleasedThisFrame));
}

bool KeyboardManager::IsKeyHeld(SDL_Scancode sc) 
{
    if (sc < 0 || sc >= SDL_SCANCODE_COUNT) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_keyStates[sc];
}

bool KeyboardManager::IsKeyPressed(SDL_Scancode sc) 
{
    if (sc < 0 || sc >= SDL_SCANCODE_COUNT) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_keysPressedThisFrame[sc];
}

bool KeyboardManager::IsKeyReleased(SDL_Scancode sc) 
{
    if (sc < 0 || sc >= SDL_SCANCODE_COUNT) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_keysReleasedThisFrame[sc];
}

void KeyboardManager::HandleEvent(const SDL_Event* ev)
{
    switch (ev->type)
    {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            // Update state tracking for pull API
            SDL_Scancode sc = ev->key.scancode;
            if (sc >= 0 && sc < SDL_SCANCODE_COUNT)
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                bool wasDown = m_keyStates[sc];
                bool isDown = ev->key.down;
                
                m_keyStates[sc] = isDown;
                
                // Detect edges
                if (isDown && !wasDown)
                    m_keysPressedThisFrame[sc] = true;
                else if (!isDown && wasDown)
                    m_keysReleasedThisFrame[sc] = true;
            }
            
            PostKeyEvent(ev->key);
            break;
        }
        default:
            break;
    }
}

void KeyboardManager::PostKeyEvent(SDL_KeyboardEvent const& ke)
{
    Message msg = Message::Create(
        ke.down ? EventType::Olympe_EventType_Keyboard_KeyDown : EventType::Olympe_EventType_Keyboard_KeyUp,
        EventDomain::Input,
        -1, // -1 is keyboard
        static_cast<int>(ke.scancode)
    );
    msg.state = ke.down ? 1 : 0;
    msg.param1 = 0.0f;

    EventQueue::Get().Push(msg);
}
