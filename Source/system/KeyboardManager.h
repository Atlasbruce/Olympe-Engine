#pragma once

#include "../object.h"
#include "message.h"
#include <SDL3/SDL.h>
#include <mutex>
#include "system_utils.h"

class KeyboardManager 
{
public:
    KeyboardManager()
    {
        name = "KeyboardManager";
		Initialize();
    }
    virtual ~KeyboardManager()
    {
		Shutdown();
	}

    static KeyboardManager& GetInstance();
    static KeyboardManager& Get() { return GetInstance(); }

    void Initialize();
    void Shutdown();

    void HandleEvent(const SDL_Event* ev);

    // Pull API for reading keyboard state
    void BeginFrame();
    bool IsKeyHeld(SDL_Scancode sc) ;
    bool IsKeyPressed(SDL_Scancode sc) ;
    bool IsKeyReleased(SDL_Scancode sc) ;

private:
    string name;
    std::mutex m_mutex;
    void PostKeyEvent(SDL_KeyboardEvent const& ke);

    // State tracking for pull API
    bool m_keyStates[SDL_SCANCODE_COUNT] = {false};
    bool m_keysPressedThisFrame[SDL_SCANCODE_COUNT] = {false};
    bool m_keysReleasedThisFrame[SDL_SCANCODE_COUNT] = {false};
};
