#include "OlympeEngine.h"

#include <SDL3/SDL.h>

#include "ImGuiManager.h"
#include "Editor/GridPanel.h"

#include "ECS_Systems_Rendering_Camera.h"
#include "ECS_Systems_Grid.h"
#include "ECS_Grid.h"

namespace Olympe
{
    // NOTE: This is an integration sketch. Hook these into your real engine loop.

    static ImGuiManager g_ImGui;
    static GridPanel g_GridPanel;
    static GridSettings_data g_GridSettings;
    static GridSystem g_GridSystem;

    bool OlympeEngine::Init()
    {
        // ... existing init ...
        // Ensure SDL window/renderer exist here.
        if (m_window && m_renderer)
        {
            g_ImGui.Init(m_window, m_renderer);
        }
        return true;
    }

    void OlympeEngine::Shutdown()
    {
        g_ImGui.Shutdown();
        // ... existing shutdown ...
    }

    void OlympeEngine::ProcessEvent(SDL_Event* e)
    {
        if (g_ImGui.IsInitialized())
        {
            g_ImGui.ProcessEvent(e);
        }
        // ... existing event processing ...
    }

    void OlympeEngine::Tick(float dt)
    {
        // ... existing update ...

        // Build camera
        CameraTransform cam;
        cam.position = m_cameraPosition; // adapt to your engine
        cam.viewportSize = {m_viewportWidth, m_viewportHeight};

        // Render grid and scene according to draw order
        if (g_GridSettings.enabled && g_GridSettings.drawBehindScene)
        {
            g_GridSystem.RenderGrid(cam, g_GridSettings, m_renderingSystem);
        }

        // ... existing scene render ...

        if (g_GridSettings.enabled && g_GridSettings.drawOverlay)
        {
            g_GridSystem.RenderGrid(cam, g_GridSettings, m_renderingSystem);
        }

        // ImGui overlay
        if (g_ImGui.IsInitialized())
        {
            g_ImGui.NewFrame();
            g_GridPanel.Draw(g_GridSettings);
            // add more editor panels here
            g_ImGui.Render();
        }
    }
}
