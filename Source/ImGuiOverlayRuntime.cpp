#include "ImGuiOverlayRuntime.h"

#include <SDL3/SDL.h>
#include <imgui.h>

#include "ImGuiManager.h"
#include "Editor/GridPanel.h"
#include "ECS_Grid.h"

namespace Olympe
{
    ImGuiOverlayRuntime::ImGuiOverlayRuntime()
        : m_initialized(false)
        , m_visible(true) // Overlay visible by default
        , m_imguiManager(nullptr)
        , m_gridPanel(nullptr)
        , m_gridSettings(nullptr)
    {
    }

    ImGuiOverlayRuntime::~ImGuiOverlayRuntime()
    {
        Shutdown();
    }

    bool ImGuiOverlayRuntime::Init(SDL_Window* window, SDL_Renderer* renderer)
    {
        if (m_initialized)
            return true;

        if (!window || !renderer)
            return false;

        // Create ImGuiManager and initialize it
        m_imguiManager = new ImGuiManager();
        if (!m_imguiManager->Init(window, renderer))
        {
            delete m_imguiManager;
            m_imguiManager = nullptr;
            return false;
        }

        // Create GridPanel for UI
        m_gridPanel = new GridPanel();

        // Create GridSettings data (could be loaded from config or ECS singleton)
        m_gridSettings = new GridSettings_data();
        // Initialize with default values (already done in struct definition)

        m_initialized = true;
        return true;
    }

    void ImGuiOverlayRuntime::Shutdown()
    {
        if (!m_initialized)
            return;

        // Clean up owned objects
        if (m_imguiManager)
        {
            m_imguiManager->Shutdown();
            delete m_imguiManager;
            m_imguiManager = nullptr;
        }

        if (m_gridPanel)
        {
            delete m_gridPanel;
            m_gridPanel = nullptr;
        }

        if (m_gridSettings)
        {
            delete m_gridSettings;
            m_gridSettings = nullptr;
        }

        m_initialized = false;
    }

    void ImGuiOverlayRuntime::ProcessEvent(SDL_Event* event)
    {
        if (!m_initialized || !event)
            return;

        // Process ImGui events (keyboard, mouse, etc.)
        m_imguiManager->ProcessEvent(event);

        // Handle overlay toggle with F10 key
        if (event->type == SDL_EVENT_KEY_DOWN)
        {
            if (event->key.key == SDLK_F10)
            {
                m_visible = !m_visible;
            }
        }

        // Optional: If you want to prevent gameplay events when ImGui captures input,
        // you can check ImGuiIO flags here:
        // ImGuiIO& io = ImGui::GetIO();
        // if (io.WantCaptureMouse || io.WantCaptureKeyboard)
        // {
        //     // Don't forward events to gameplay systems
        // }
    }

    void ImGuiOverlayRuntime::BeginFrame()
    {
        if (!m_initialized)
            return;

        // Start a new ImGui frame
        m_imguiManager->NewFrame();
    }

    void ImGuiOverlayRuntime::Draw()
    {
        if (!m_initialized || !m_visible)
            return;

        // Draw ImGui windows
        // Note: ImGui::NewFrame() must have been called before (in BeginFrame)

        // Draw GridPanel window
        if (m_gridPanel && m_gridSettings)
        {
            m_gridPanel->Draw(*m_gridSettings);
        }

        // Here you can add more ImGui windows/panels as needed
        // Example:
        // ImGui::Begin("Another Panel");
        // ImGui::Text("Hello from overlay!");
        // ImGui::End();

        // Optional: Show a simple overlay toggle hint
        constexpr ImGuiWindowFlags overlayInfoFlags = 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_AlwaysAutoResize;
        
        ImGui::Begin("Overlay Info", nullptr, overlayInfoFlags);
        ImGui::Text("Press F10 to toggle overlay");
        ImGui::End();
    }

    void ImGuiOverlayRuntime::Render()
    {
        if (!m_initialized)
            return;

        // Render ImGui draw data
        // This calls ImGui::Render() internally and submits to the SDL renderer
        m_imguiManager->Render();
    }
}
