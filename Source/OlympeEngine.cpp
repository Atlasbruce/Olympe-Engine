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
//#include "system/Camerasytem.h"
#include "system/ViewportManager.h"
#include "videogame.h"
#include "DataManager.h"
#include "system/system_utils.h"
#include "PanelManager.h"

// Avoid Win32 macro collisions: PostMessage is a Win32 macro expanding to PostMessageW/A
#ifdef PostMessage
#undef PostMessage
#endif

#include <fstream>
#include <string>
#include <iterator>
#include <cctype>
#include <sstream>

using namespace std;

 /* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
const int TARGET_FPS = 100;
const Uint32 FRAME_TARGET_TIME_MS = 1000 / TARGET_FPS;

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

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!event) return SDL_APP_CONTINUE;

    // Forward to input submanagers (they will post EventManager messages)
    //JoystickManager::Get().HandleEvent(event);
    //KeyboardManager::Get().HandleEvent(event);
    //MouseManager::Get().HandleEvent(event);
    //EventManager::Get().AddMessage (EventManager::Message(static_cast<EventStructType>(event->type)));

   /* Message m;
    m.struct_type = EventStructType::EventStructType_SDL;
    m.msg_type = (EventType) event->type;
    m.sdlEvent = event;
	EventManager::Get().AddMessage(m); // forward SDL event to EventManager/**/

    InputsManager::Get().HandleEvent(event); //facto

    switch (event->type)
    {
        case SDL_EVENT_KEY_DOWN:
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

    #ifdef _WIN32
        // Poll and dispatch Win32 messages here and forward to EventManager
        {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                Message m;
                m.struct_type= EventStructType::EventStructType_System_Windows;
                m.msg_type = (EventType) msg.message;
                m.msg = &msg;

				EventManager::Get().DispatchImmediate(m); // forward Win32 message to EventManager immediately because we are in a loop
      
            }
        }
    #endif

    // 2. SDL events are polled by SDL_AppEvent callback before this function
    // 3. Calculate delta time
	GameEngine::Get().Process(); // update fDt here for all managers
    // 4. Process event bus
	EventManager::Get().Process(); // ensure queued events are dispatched to all registered listeners
    // 5. Process ECS systems (including InputMappingSystem)
	World::Get().Process(); // process all world objects/components

    // If game state requests quit, end the application loop
    if (GameStateManager::GetState() == GameState::GameState_Quit) { return SDL_APP_SUCCESS; }

    //-------------------------------------------------------------------
	// RENDER PHASE -----------------------------------------------------
	//-------------------------------------------------------------------

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
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

            // Optionally you can set camera state here using player id:
            short playerId = (i < players.size()) ? players[i] : 0;
            CameraManager::Get().Apply(renderer, playerId);

            // Draw world for this viewport. World::Render should use CameraManager to determine camera.
            World::Get().Render();
        }

        // restore previous viewport
        SDL_SetRenderViewport(renderer, &prev);
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

    // Shutdown datamanager to ensure resources freed
    DataManager::Get().Shutdown();

    SYSTEM_LOG << "----------- OLYMPE ENGINE V2 ------------" << endl;
    SYSTEM_LOG << "System shutdown completed\n" << endl;
}

