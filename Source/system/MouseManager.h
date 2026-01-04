#pragma once

#include <SDL3/SDL.h>
#include <mutex>
#include <string>


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
    std::string name;
    std::mutex m_mutex;
    void PostButtonEvent(const SDL_MouseButtonEvent& be);
    void PostMotionEvent(const SDL_MouseMotionEvent& me);
};
