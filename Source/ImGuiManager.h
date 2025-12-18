#pragma once

#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Olympe
{
    class ImGuiManager
    {
    public:
        bool Init(SDL_Window* window, SDL_Renderer* renderer);
        void Shutdown();

        void NewFrame();
        void Render();

        // If you route SDL events in engine, call this.
        // Returns true if ImGui wants to capture the event.
        bool ProcessEvent(void* sdlEvent);

        bool IsInitialized() const { return m_initialized; }

    private:
        bool m_initialized{false};
        SDL_Window* m_window{nullptr};
        SDL_Renderer* m_renderer{nullptr};
    };
}
