#include "ImGuiManager.h"

#include <SDL3/SDL.h>

#include <imgui.h>

// NOTE: Engine repository must provide these backends in include path.
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>

namespace Olympe
{
    bool ImGuiManager::Init(SDL_Window* window, SDL_Renderer* renderer)
    {
        if (m_initialized) return true;
        if (!window || !renderer) return false;

        m_window = window;
        m_renderer = renderer;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForSDLRenderer(window, renderer))
            return false;
        if (!ImGui_ImplSDLRenderer3_Init(renderer))
            return false;

        m_initialized = true;
        return true;
    }

    void ImGuiManager::Shutdown()
    {
        if (!m_initialized) return;

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        m_initialized = false;
        m_window = nullptr;
        m_renderer = nullptr;
    }

    void ImGuiManager::NewFrame()
    {
        if (!m_initialized) return;
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiManager::Render()
    {
        if (!m_initialized) return;
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData());
    }

    bool ImGuiManager::ProcessEvent(void* sdlEvent)
    {
        if (!m_initialized) return false;
        if (!sdlEvent) return false;
        return ImGui_ImplSDL3_ProcessEvent((SDL_Event*)sdlEvent);
    }
}
