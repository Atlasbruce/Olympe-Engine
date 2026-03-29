/*
 * Olympe Blueprint Editor - GUI Entry Point (SDL3 Only)
 * Visual node-based editor using ImGui and ImNodes with SDL3
 */

#include <SDL3/SDL.h>
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include "BlueprintEditorGUI.h"
#include <iostream>

#ifdef _WIN32
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int argc, char* argv[])
#endif
{
    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL3 initialization failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window with SDL3
    SDL_Window* window = SDL_CreateWindow(
        "Olympe Blueprint Editor - SDL3",
        1280, 720,
        SDL_WINDOW_RESIZABLE
    );
    
    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer with SDL3
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Enable docking if available
    #ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #endif

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends for SDL3
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Create editor instance
    Olympe::BlueprintEditorGUI editor;
    editor.Initialize();

    std::cout << "Olympe Blueprint Editor - SDL3 with Asset Browser" << std::endl;
    std::cout << "Press Ctrl+Q or close window to quit" << std::endl;

    // Main loop
    bool running = true;
    while (running)
    {
        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
                running = false;

            // Keyboard shortcut to quit (Ctrl+Q)
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_Q)
                    running = false;
            }
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Render editor
        if (!editor.Render())
            running = false;  // Editor requested exit

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    editor.Shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
