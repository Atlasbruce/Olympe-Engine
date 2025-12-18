// Restored to match master (commit 54aae1a...) and extended with ImGuiOverlayRuntime hooks

#include "OlympeEngine.h"

#include <SDL3/SDL.h>

#include "Core/Log.h"
#include "Core/Paths.h"

#include "Rendering/Renderer.h"
#include "World/World.h"

#include "ImGuiOverlayRuntime.h"

// Global / static state
static SDL_Window*   GWindow   = nullptr;
static SDL_Renderer* GRenderer = nullptr;

static Olympe::World*    GWorld    = nullptr;
static Olympe::Renderer* GEngineRdr = nullptr;

static Olympe::ImGuiOverlayRuntime GImGuiOverlay;

// SDL3 application entry points ------------------------------------------------

extern "C" int SDL_AppInit(void** appstate, int argc, char* argv[])
{
    (void)appstate;
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        OLYMPE_LOG_ERROR("SDL_Init failed: {}", SDL_GetError());
        return -1;
    }

    // Create window + renderer (master behavior)
    GWindow = SDL_CreateWindow("Olympe Engine", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!GWindow)
    {
        OLYMPE_LOG_ERROR("SDL_CreateWindow failed: {}", SDL_GetError());
        return -1;
    }

    GRenderer = SDL_CreateRenderer(GWindow, nullptr);
    if (!GRenderer)
    {
        OLYMPE_LOG_ERROR("SDL_CreateRenderer failed: {}", SDL_GetError());
        return -1;
    }

    // Engine init (master behavior)
    Olympe::Paths::Init(argv && argv[0] ? argv[0] : "");

    GEngineRdr = new Olympe::Renderer(GRenderer);
    GWorld     = new Olympe::World();

    // --- ImGui overlay runtime ------------------------------------------------
    // Initialize after SDL objects exist.
    if (!GImGuiOverlay.Init(GWindow, GRenderer))
    {
        OLYMPE_LOG_WARN("ImGuiOverlayRuntime failed to initialize; continuing without overlay");
    }

    return 0;
}

extern "C" int SDL_AppEvent(void* appstate, SDL_Event* event)
{
    (void)appstate;

    // Let ImGui consume events first/also (does not prevent engine handling unless desired)
    GImGuiOverlay.ProcessEvent(event);

    if (event->type == SDL_EVENT_QUIT)
        return -1;

    return 0;
}

extern "C" int SDL_AppIterate(void* appstate)
{
    (void)appstate;

    // Begin ImGui frame early, but keep draw/render order: overlay after world render.
    GImGuiOverlay.BeginFrame();

    // Clear
    SDL_SetRenderDrawColor(GRenderer, 15, 15, 18, 255);
    SDL_RenderClear(GRenderer);

    // Update/render world (master behavior)
    if (GWorld)
        GWorld->Tick();

    if (GEngineRdr)
        GEngineRdr->Render(*GWorld);

    // Draw ImGui overlay after world render and before present
    GImGuiOverlay.Draw();
    GImGuiOverlay.Render();

    SDL_RenderPresent(GRenderer);

    return 0;
}

extern "C" void SDL_AppQuit(void* appstate)
{
    (void)appstate;

    // Shutdown overlay before destroying SDL objects
    GImGuiOverlay.Shutdown();

    delete GWorld;
    GWorld = nullptr;

    delete GEngineRdr;
    GEngineRdr = nullptr;

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
}
