/*
 * Olympe Blueprint Editor Standalone - Entry Point
 * 
 * This executable provides a dedicated WYSIWYG blueprint editor without game runtime.
 * It shares all Blueprint Editor code with the runtime editor but operates in 
 * Blueprint Editor Standalone mode for full CRUD operations.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_sdl3.h"
#include "../third_party/imgui/backends/imgui_impl_sdlrenderer3.h"
#include "../BlueprintEditor/blueprinteditor.h"
#include "../BlueprintEditor/BlueprintEditorGUI.h"
#include <iostream>

static SDL_Window* g_BlueprintEditorWindow = nullptr;
static SDL_Renderer* g_BlueprintEditorRenderer = nullptr;
static Olympe::BlueprintEditorGUI* g_BlueprintEditorGUI = nullptr;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    std::cout << "=============================================" << std::endl;
    std::cout << "  Olympe Blueprint Editor Standalone" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Version: 1.0.0" << std::endl;
    std::cout << "Mode: Blueprint Editor Standalone (Full CRUD)" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        std::cerr << "[BlueprintEditorStandalone] SDL3 Init failed: " << SDL_GetError() << std::endl;
        return SDL_APP_FAILURE;
    }
    
    g_BlueprintEditorWindow = SDL_CreateWindow(
        "Olympe Blueprint Editor Standalone - WYSIWYG Editor",
        1920, 1080,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );
    
    if (!g_BlueprintEditorWindow)
    {
        std::cerr << "[BlueprintEditorStandalone] Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    
    g_BlueprintEditorRenderer = SDL_CreateRenderer(g_BlueprintEditorWindow, nullptr);
    if (!g_BlueprintEditorRenderer)
    {
        std::cerr << "[BlueprintEditorStandalone] Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(g_BlueprintEditorWindow);
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.40f, 0.60f, 1.00f);
    
    ImGui_ImplSDL3_InitForSDLRenderer(g_BlueprintEditorWindow, g_BlueprintEditorRenderer);
    ImGui_ImplSDLRenderer3_Init(g_BlueprintEditorRenderer);
    
    Olympe::BlueprintEditor::Get().Initialize();
    Olympe::BlueprintEditor::Get().InitializeStandaloneEditor();
    Olympe::BlueprintEditor::Get().SetActive(true);
    
    g_BlueprintEditorGUI = new Olympe::BlueprintEditorGUI();
    g_BlueprintEditorGUI->Initialize();
    
    std::cout << "[BlueprintEditorStandalone] Initialization complete" << std::endl;
    std::cout << "[BlueprintEditorStandalone] EditorContext: Standalone (Full CRUD enabled)" << std::endl;
    std::cout << "[BlueprintEditorStandalone] Press Ctrl+Q to quit" << std::endl;
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (!event) return SDL_APP_CONTINUE;
    
    ImGui_ImplSDL3_ProcessEvent(event);
    
    ImGuiIO& io = ImGui::GetIO();
    
    if (event->type == SDL_EVENT_QUIT || event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
    {
        return SDL_APP_SUCCESS;
    }
    
    if (event->type == SDL_EVENT_KEY_DOWN && !io.WantCaptureKeyboard)
    {
        if ((event->key.mod & SDL_KMOD_CTRL) && event->key.key == SDLK_Q)
        {
            std::cout << "[BlueprintEditorStandalone] User requested quit (Ctrl+Q)" << std::endl;
            if (Olympe::BlueprintEditor::Get().HasUnsavedChanges())
            {
                std::cout << "[BlueprintEditorStandalone] Warning: Unsaved changes detected" << std::endl;
            }
            return SDL_APP_SUCCESS;
        }
        
        if (event->key.key == SDLK_ESCAPE)
        {
            if (Olympe::BlueprintEditor::Get().HasUnsavedChanges())
            {
                std::cout << "[BlueprintEditorStandalone] ESC pressed with unsaved changes" << std::endl;
            }
            else
            {
                return SDL_APP_SUCCESS;
            }
        }
    }
    
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    Olympe::BlueprintEditor::Get().Update(0.016f);
    
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    
    if (g_BlueprintEditorGUI)
    {
        g_BlueprintEditorGUI->Render();
    }
    
    ImGui::Render();
    
    SDL_SetRenderDrawColor(g_BlueprintEditorRenderer, 30, 30, 34, 255);
    SDL_RenderClear(g_BlueprintEditorRenderer);
    
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), g_BlueprintEditorRenderer);
    
    SDL_RenderPresent(g_BlueprintEditorRenderer);
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    std::cout << "[BlueprintEditorStandalone] Shutting down..." << std::endl;
    
    if (g_BlueprintEditorGUI)
    {
        g_BlueprintEditorGUI->Shutdown();
        delete g_BlueprintEditorGUI;
        g_BlueprintEditorGUI = nullptr;
    }
    
    Olympe::BlueprintEditor::Get().Shutdown();
    
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    
    if (g_BlueprintEditorRenderer)
    {
        SDL_DestroyRenderer(g_BlueprintEditorRenderer);
        g_BlueprintEditorRenderer = nullptr;
    }
    
    if (g_BlueprintEditorWindow)
    {
        SDL_DestroyWindow(g_BlueprintEditorWindow);
        g_BlueprintEditorWindow = nullptr;
    }
    
    SDL_Quit();
    
    std::cout << "[BlueprintEditorStandalone] Shutdown complete" << std::endl;
}
