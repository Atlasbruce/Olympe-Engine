#pragma once

#include "../object.h"
#include <SDL3/SDL.h>
#include <mutex>


class MouseManager 
{
public:
    MouseManager()
    {
		Initialize();
    }
    virtual ~MouseManager()
    {
        Shutdown();
    }

    static MouseManager& GetInstance();
    static MouseManager& Get() { return GetInstance(); }

	void Initialize();
    void Shutdown();

    void HandleEvent(const SDL_Event* ev);
    void BeginFrame(); // Reset per-frame state

private:
    string name;
    std::mutex m_mutex;
    void PostButtonEvent(const SDL_MouseButtonEvent& be);
    void PostMotionEvent(const SDL_MouseMotionEvent& me);
};
