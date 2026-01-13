/*
 * Olympe Tilemap Editor - Entry Point
 * 
 * SDL3-based standalone tilemap editor for Olympe Engine.
 */

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include "../include/TilemapEditorApp.h"
#include <iostream>

// This provides the platform-specific entry point implementation for SDL3
#include <SDL3/SDL_main_impl.h>

static SDL_Window* g_Window = nullptr;
static SDL_Renderer* g_Renderer = nullptr;
static Olympe::Editor::TilemapEditorApp* g_EditorApp = nullptr;
static Uint64 g_LastFrameTime = 0;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    std::cout << "=============================================" << std::endl;
    std::cout << "  Olympe Tilemap Editor" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Version: 1.0.0 (Phase 1 - Foundation)" << std::endl;
    std::cout << "Mode: Standalone Tilemap/Level Editor" << std::endl;
    std::cout << "=============================================" << std::endl;

    // Initialize SDL3
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false)
    {
        std::cerr << "[TilemapEditor] SDL3 Init failed: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }

    // Create window
    g_Window = SDL_CreateWindow(
        "Olympe Tilemap Editor - Phase 1",
        1920,
        1080,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    if (!g_Window)
    {
        std::cerr << "[TilemapEditor] Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // Create renderer
    g_Renderer = SDL_CreateRenderer(g_Window, nullptr);
    if (!g_Renderer)
    {
        std::cerr << "[TilemapEditor] Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Note: Docking not available in current ImGui version

    // Setup ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.40f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.60f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.45f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.50f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.60f, 0.80f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.45f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.50f, 0.70f, 1.00f);

    // Initialize ImGui backends
    ImGui_ImplSDL3_InitForSDLRenderer(g_Window, g_Renderer);
    ImGui_ImplSDLRenderer3_Init(g_Renderer);

    // Create and initialize editor application
    g_EditorApp = new Olympe::Editor::TilemapEditorApp();
    if (!g_EditorApp->Initialize())
    {
        std::cerr << "[TilemapEditor] Editor app initialization failed" << std::endl;
        delete g_EditorApp;
        g_EditorApp = nullptr;
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyRenderer(g_Renderer);
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }

    // Initialize frame timing
    g_LastFrameTime = SDL_GetTicks();

    std::cout << "[TilemapEditor] Initialization complete" << std::endl;
    std::cout << "[TilemapEditor] Press Ctrl+Q to quit" << std::endl;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!event)
        return SDL_APP_CONTINUE;

    ImGui_ImplSDL3_ProcessEvent(event);

    ImGuiIO& io = ImGui::GetIO();

    // Handle quit events
    if (event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        if (g_EditorApp && g_EditorApp->HasUnsavedChanges())
        {
            std::cout << "[TilemapEditor] Warning: Unsaved changes detected" << std::endl;
            // TODO: Show confirmation dialog
        }
        return SDL_APP_SUCCESS;
    }

    // Handle keyboard shortcuts
    if (event->type == SDL_EVENT_KEY_DOWN && !io.WantCaptureKeyboard)
    {
        // Ctrl+Q: Quit
        if ((event->key.mod & SDL_KMOD_CTRL) && event->key.key == SDLK_Q)
        {
            std::cout << "[TilemapEditor] User requested quit (Ctrl+Q)" << std::endl;
            if (g_EditorApp && g_EditorApp->HasUnsavedChanges())
            {
                std::cout << "[TilemapEditor] Warning: Unsaved changes detected" << std::endl;
            }
            return SDL_APP_SUCCESS;
        }

        // Ctrl+N: New Level
        if ((event->key.mod & SDL_KMOD_CTRL) && event->key.key == SDLK_N)
        {
            if (g_EditorApp)
                g_EditorApp->NewLevel();
        }

        // Ctrl+O: Open Level
        if ((event->key.mod & SDL_KMOD_CTRL) && event->key.key == SDLK_O)
        {
            if (g_EditorApp)
                g_EditorApp->OpenLevel();
        }

        // Ctrl+S: Save Level
        if ((event->key.mod & SDL_KMOD_CTRL) && event->key.key == SDLK_S)
        {
            if (g_EditorApp)
            {
                if (event->key.mod & SDL_KMOD_SHIFT)
                    g_EditorApp->SaveLevelAs();
                else
                    g_EditorApp->SaveLevel();
            }
        }

        // ESC: Deselect / Cancel
        if (event->key.key == SDLK_ESCAPE)
        {
            // TODO: Clear selection
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    // Calculate delta time
    Uint64 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - g_LastFrameTime) / 1000.0f;
    g_LastFrameTime = currentTime;

    // Clamp delta time to prevent large jumps (e.g., when debugging)
    if (deltaTime > 0.1f)
    {
        deltaTime = 0.016f; // Fall back to ~60 FPS if delta is too large
    }

    // Start ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Render editor UI
    if (g_EditorApp)
    {
        g_EditorApp->Render();
    }

    // Render ImGui
    ImGui::Render();

    // Clear and render
    SDL_SetRenderDrawColor(g_Renderer, 30, 30, 34, 255);
    SDL_RenderClear(g_Renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_Renderer);

    SDL_RenderPresent(g_Renderer);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    std::cout << "[TilemapEditor] Shutting down..." << std::endl;

    // Cleanup editor app
    if (g_EditorApp)
    {
        g_EditorApp->Shutdown();
        delete g_EditorApp;
        g_EditorApp = nullptr;
    }

    // Cleanup ImGui
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    if (g_Renderer)
    {
        SDL_DestroyRenderer(g_Renderer);
        g_Renderer = nullptr;
    }

    if (g_Window)
    {
        SDL_DestroyWindow(g_Window);
        g_Window = nullptr;
    }

    SDL_Quit();

    std::cout << "[TilemapEditor] Shutdown complete" << std::endl;
}
