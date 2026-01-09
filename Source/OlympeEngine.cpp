/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Main application file for Olympe Engine V2 using SDL3.
Notes:
- This file implements the SDL_App* callbacks to initialize, run, and shutdown
  the engine using SDL3's application framework.
- GameEngine and its submanagers are initialized here: EventManager, InputsManager,
  DataManager, VideoGame, etc.

*/

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_messagebox.h>
#include "gameengine.h"
#include "World.h"
#include "system/JoystickManager.h"
#include "system/KeyboardManager.h"
#include "system/MouseManager.h"
#include "InputsManager.h"
#include "GameState.h"
#include "system/ViewportManager.h"
#include "DataManager.h"
#include "system/system_utils.h"
#include "PanelManager.h"
#include "BlueprintEditor/BlueprintEditor.h"
#include "BlueprintEditor/BlueprintEditorGUI.h"
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/backends/imgui_impl_sdl3.h"
#include "third_party/imgui/backends/imgui_impl_sdlrenderer3.h"

// Avoid Win32 macro collisions: PostMessage is a Win32 macro expanding to PostMessageW/A
#ifdef PostMessage
#undef PostMessage
#endif

 /* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
const int TARGET_FPS = 100;
const Uint32 FRAME_TARGET_TIME_MS = 1000 / TARGET_FPS;

// BlueprintEditor GUI instance
static Olympe::BlueprintEditorGUI* blueprintEditorGUI = nullptr;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    SYSTEM_LOG << "----------- OLYMPE ENGINE V2 ------------" << endl;
    SYSTEM_LOG << "System Initialization\n" << endl;

    // Load configuration (JSON inside "olympe.ini"). Defaults used if not present.
    LoadOlympeConfig("olympe.ini");

    SDL_SetAppMetadata("Olympe Game Engine", "2.0", "com.googlesites.olympeengine");

    // Initialize system logger so SYSTEM_LOG forwards to UI (if available)
    Logging::InitLogger();


    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Olympe Engine 2.0", GameEngine::screenWidth, GameEngine::screenHeight, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, GameEngine::screenWidth, GameEngine::screenHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	// Create and initialize PanelManager (for debug panels)
	PanelManager::Get().Initialize();

    // Initialize DataManager (load system resources if needed)
	DataManager::Get().Initialize(); // DataManager must be initialized before GameEngine to enable loading resources during GameEngine init

    //Olympe Engine and all managers singleton Initialization Here
    GameEngine::renderer = renderer; // important: set main renderer for GameEngine before GetInstance
    GameEngine::GetInstance(); // create the GameEngine itself
    GameEngine::Get().Initialize(); // initialize all submanagers

    // Attach panels/menu to main SDL window (Windows only)
    PanelManager::Get().AttachToSDLWindow(window);

    // Initialize Blueprint Editor Backend
    Olympe::BlueprintEditor::Get().Initialize();
    
    // Create Blueprint Editor GUI
    blueprintEditorGUI = new Olympe::BlueprintEditorGUI();
    blueprintEditorGUI->Initialize();
    
    SYSTEM_LOG << "BlueprintEditor initialized (toggle with F2)" << endl;

    // Initialisation (à l'initialisation de l'application)
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    // Initialiser vos implémentations (ex. SDL + renderer)
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!event) return SDL_APP_CONTINUE;

    // ===== A) ImGui Event Processing - MUST BE FIRST =====
    // Process ImGui events before game logic to enable panel interactivity
    ImGui_ImplSDL3_ProcessEvent(event);
    
    // Get ImGui IO to check if ImGui wants to capture input
    ImGuiIO& io = ImGui::GetIO();
    
    // Skip event propagation to game if ImGui wants to capture it
    bool skipEventForGame = false;
    
    // Check if ImGui wants to capture mouse events
    if (io.WantCaptureMouse)
    {
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
            event->type == SDL_EVENT_MOUSE_MOTION ||
            event->type == SDL_EVENT_MOUSE_WHEEL)
        {
            skipEventForGame = true;
        }
    }
    
    // Check if ImGui wants to capture keyboard events
    if (io.WantCaptureKeyboard)
    {
        if (event->type == SDL_EVENT_KEY_DOWN ||
            event->type == SDL_EVENT_KEY_UP ||
            event->type == SDL_EVENT_TEXT_INPUT)
        {
            skipEventForGame = true;
        }
    }
    
    // Only propagate to game systems if ImGui doesn't want the event
    if (!skipEventForGame)
    {
        InputsManager::Get().HandleEvent(event); //facto
    }

    switch (event->type)
    {
        case SDL_EVENT_KEY_DOWN:
        
        // F2 toggles Blueprint Editor (always process, even if ImGui is capturing)
        if (event->key.key == SDLK_F2)
        {
            Olympe::BlueprintEditor::Get().ToggleActive();
            SYSTEM_LOG << "BlueprintEditor " 
                      << (Olympe::BlueprintEditor::Get().IsActive() ? "activated" : "deactivated") 
                      << endl;
            return SDL_APP_CONTINUE; // Early return to avoid ESC dialog below
        }
        
        if (event->key.key == SDLK_ESCAPE)
        {
            const SDL_MessageBoxButtonData buttons[] =
            {
                { /* .flags, .buttonid, .text */        0, 0, "No" },
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Yes" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel" },
            };
            const SDL_MessageBoxColorScheme colorScheme =
            {
                { /* .colors (.r, .g, .b) */
                    /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
                    { 255,   0,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
                    {   0, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
                    { 255, 255,   0 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
                    {   0,   0, 255 },
                    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
                    { 255,   0, 255 }
                }
            };
            const SDL_MessageBoxData messageboxdata = {
                SDL_MESSAGEBOX_INFORMATION, /* .flags */
                window, /* .window */
                "Olympe Engine V2", /* .title */
                "Do you want to exit Olympe Engine?", /* .message */
                SDL_arraysize(buttons), /* .numbuttons */
                buttons, /* .buttons */
                &colorScheme /* .colorScheme */
            };
            int buttonid;

            SDL_ShowMessageBox(&messageboxdata, &buttonid);
            if (buttonid == 1)
                return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
        }
        break;
        case SDL_EVENT_QUIT:
            {
                return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
            }
            break;
        default:
            return SDL_APP_CONTINUE;  /* carry on with the program! */
            break;
    }

    return SDL_APP_CONTINUE; /* carry on with the program! */

}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
    //-------------------------------------------------------------------
	// PROCESSING PHASE -------------------------------------------------
    //-------------------------------------------------------------------
    
    // 1. Reset frame state for input managers (Pull API)
    KeyboardManager::Get().BeginFrame();
    JoystickManager::Get().BeginFrame();
    MouseManager::Get().BeginFrame();

    // Calculate delta time
	GameEngine::Get().Process(); // update fDt here for all managers

    // Update Blueprint Editor backend if active
    if (Olympe::BlueprintEditor::Get().IsActive())
    {
        Olympe::BlueprintEditor::Get().Update(GameEngine::fDt);
    }

    // Process ECS systems 
	World::Get().Process(); // process all world objects/components

    // If game state requests quit, end the application loop
    if (GameStateManager::GetState() == GameState::GameState_Quit) { return SDL_APP_SUCCESS; }

    //-------------------------------------------------------------------
	// RENDER PHASE -----------------------------------------------------
	//-------------------------------------------------------------------

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, SDL_ALPHA_OPAQUE);  /* white, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    // Render world once per viewport/player so each viewport gets its own draw pass
    const auto& rects = ViewportManager::Get().GetViewRects();
    const auto& players = ViewportManager::Get().GetPlayers();

    if (rects.empty())
    {
        // fallback to previous single-render behavior
        World::Get().Render();
    }
    else
    {
        // save current viewport to restore later
        SDL_Rect prev = { 0, 0, GameEngine::screenHeight, GameEngine::screenHeight };
        //SDL_RenderGetViewport(renderer, &prev);

        for (size_t i = 0; i < rects.size(); ++i)
        {
            const auto& rf = rects[i];
            SDL_Rect r = { (int)rf.x, (int)rf.y, (int)rf.w, (int)rf.h };
            SDL_SetRenderViewport(renderer, &r);
            SDL_SetRenderClipRect(renderer, &r);

            // Draw world for this viewport
            World::Get().Render();
            
            // Reset clip rect after each viewport
            SDL_SetRenderClipRect(renderer, nullptr);
        }

        // restore previous viewport and ensure clip rect is cleared
        SDL_SetRenderClipRect(renderer, nullptr);
        SDL_SetRenderViewport(renderer, &prev);
    }

    // Render Blueprint Editor GUI if active
    // NOTE: Requires ImGui to be initialized and integrated into main engine
    // TODO: Add ImGui initialization in SDL_AppInit and ImGui NewFrame/Render calls here
    if (Olympe::BlueprintEditor::Get().IsActive() && blueprintEditorGUI)
    {
        // ===== FIX: Synchronize ImGui DisplaySize with window size =====
        // This fixes mouse coordinates in fullscreen mode
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        
        ImGuiIO& io = ImGui::GetIO();
        
        // Update DisplaySize if window size changed
        if (io.DisplaySize.x != (float)windowWidth || io.DisplaySize.y != (float)windowHeight)
        {
            io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);
            
            // Update framebuffer scale for High DPI support
            int displayWidth, displayHeight;
            SDL_GetWindowSizeInPixels(window, &displayWidth, &displayHeight);
            
            if (windowWidth > 0 && windowHeight > 0)
            {
                io.DisplayFramebufferScale = ImVec2(
                    (float)displayWidth / windowWidth,
                    (float)displayHeight / windowHeight
                );
            }
        }
        
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();

        blueprintEditorGUI->Render(); // BeginMainMenuBar() est maintenant sûr

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    }

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

    // Update FPS counter and set window title once per second
    static int frameCount = 0;
    static Uint64 fpsLastTime = 0;
    static Uint64 frameStartTime = 0;
    if (frameStartTime > 0)
    {
        // Temps �coul� depuis le d�but de la frame pr�c�dente
        Uint64 timeTakenForFrame = SDL_GetTicks() - frameStartTime;

        // Si la frame a pris moins de temps que le temps cible (16.666 ms)...
        if (timeTakenForFrame < FRAME_TARGET_TIME_MS)
        {
            // ... alors nous "dormons" (bloquons) le thread pour le temps restant.
            Uint32 timeToWait = FRAME_TARGET_TIME_MS - (Uint32)timeTakenForFrame;
            SDL_Delay(timeToWait);
            // Ce SDL_Delay garantit que chaque frame ne sera pas trait�e plus vite que TARGET_FPS.
        }
    }
    frameStartTime = SDL_GetTicks();

    frameCount++;
    const Uint64 nowMs = SDL_GetTicks();
    if (fpsLastTime == 0) fpsLastTime = nowMs;
    const Uint64 elapsed = nowMs - fpsLastTime;
    if (elapsed >= 1000)
    {
        float fps = frameCount * 1000.0f / (float)elapsed;
        char title[256];
        snprintf(title, sizeof(title), "Olympe Engine 2.0 - FPS: %.f", fps);
        if (window) SDL_SetWindowTitle(window, title);
        frameCount = 0;
        fpsLastTime = nowMs;
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */

    // Shutdown Blueprint Editor
    if (blueprintEditorGUI)
    {
    //    blueprintEditorGUI->Shutdown(); // Double delete 
        delete blueprintEditorGUI;
        blueprintEditorGUI = nullptr;
    }

    Olympe::BlueprintEditor::Get().Shutdown();

    // Shutdown datamanager to ensure resources freed
    DataManager::Get().Shutdown();

    SYSTEM_LOG << "----------- OLYMPE ENGINE V2 ------------" << endl;
    SYSTEM_LOG << "System shutdown completed\n" << endl;
}

