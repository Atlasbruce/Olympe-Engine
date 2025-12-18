// OlympeEngine.cpp - SDL3 callbacks runtime with ImGuiOverlayRuntime integration
// This file provides the main SDL3 application entry points and integrates
// the ImGui overlay module for runtime debugging and visualization.

#include <SDL3/SDL.h>

#include "system/system_utils.h"
#include "World.h"
#include "GameEngine.h"

#include "ImGuiOverlayRuntime.h"

// Global / static state
static SDL_Window*   GWindow   = nullptr;
static SDL_Renderer* GRenderer = nullptr;

static Olympe::ImGuiOverlayRuntime GImGuiOverlay;

// SDL3 application entry points ------------------------------------------------

extern "C" int SDL_AppInit(void** appstate, int argc, char* argv[])
{
    (void)appstate;
    (void)argc;
    (void)argv;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD) != 0)
    {
        SYSTEM_LOG << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create window and renderer
    GWindow = SDL_CreateWindow("Olympe Engine", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!GWindow)
    {
        SYSTEM_LOG << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    GRenderer = SDL_CreateRenderer(GWindow, nullptr);
    if (!GRenderer)
    {
        SYSTEM_LOG << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Initialize engine systems
    Logging::InitLogger();
    
    // Initialize World and GameEngine singletons
    World::Get().Initialize_ECS_Systems();
    GameEngine::Get().Initialize();

    // --- ImGui overlay runtime ------------------------------------------------
    // Initialize ImGui overlay after SDL objects are created.
    // This module encapsulates ImGuiManager, GridPanel, and GridSettings.
    if (!GImGuiOverlay.Init(GWindow, GRenderer))
    {
        SYSTEM_LOG << "WARNING: ImGuiOverlayRuntime failed to initialize; continuing without overlay" << std::endl;
    }
    else
    {
        SYSTEM_LOG << "ImGuiOverlayRuntime initialized successfully (press F10 to toggle)" << std::endl;
    }

    return 0;
}

extern "C" int SDL_AppEvent(void* appstate, SDL_Event* event)
{
    (void)appstate;

    // --- ImGui overlay event processing ----------------------------------------
    // Let ImGui process events first. This allows ImGui to capture keyboard/mouse input.
    // Optional: Check ImGuiIO flags to prevent gameplay events when ImGui captures input.
    GImGuiOverlay.ProcessEvent(event);

    // Process engine events
    // Note: You can add checks here to avoid forwarding events to gameplay systems
    // when ImGui wants to capture them (e.g., io.WantCaptureMouse, io.WantCaptureKeyboard)

    if (event->type == SDL_EVENT_QUIT)
        return -1;

    // Forward events to engine systems (InputsManager, etc.)
    // GameEngine::Get().ProcessEvent(event); // If such method exists

    return 0;
}

extern "C" int SDL_AppIterate(void* appstate)
{
    (void)appstate;

    // --- ImGui overlay frame begin --------------------------------------------
    // Begin ImGui frame early, but draw/render after the world to overlay it.
    GImGuiOverlay.BeginFrame();

    // Clear the screen
    SDL_SetRenderDrawColor(GRenderer, 15, 15, 18, 255);
    SDL_RenderClear(GRenderer);

    // --- Engine update and render ---------------------------------------------
    // Update and render the world/game entities
    World::Get().Process_ECS_Systems();
    World::Get().Render_ECS_Systems();

    // --- ImGui overlay draw and render ----------------------------------------
    // Draw ImGui UI elements (GridPanel, etc.) and render them to screen.
    // This happens AFTER world rendering so the overlay appears on top,
    // but BEFORE SDL_RenderPresent to be included in the final frame.
    GImGuiOverlay.Draw();
    GImGuiOverlay.Render();

    // Present the rendered frame
    SDL_RenderPresent(GRenderer);

    return 0;
}

extern "C" void SDL_AppQuit(void* appstate)
{
    (void)appstate;

    // --- ImGui overlay shutdown -----------------------------------------------
    // Shutdown ImGui overlay BEFORE destroying SDL objects.
    GImGuiOverlay.Shutdown();

    // Cleanup engine systems
    // (World and GameEngine are singletons, they'll cleanup on program exit)

    // Cleanup SDL resources
    if (GRenderer)
    {
        SDL_DestroyRenderer(GRenderer);
        GRenderer = nullptr;
    }

    if (GWindow)
    {
        SDL_DestroyWindow(GWindow);
        GWindow = nullptr;
    }

    SDL_Quit();

    SYSTEM_LOG << "SDL and ImGui overlay shutdown complete" << std::endl;
}
