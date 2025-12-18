#pragma once

#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

namespace Olympe
{
    // Forward declarations
    class ImGuiManager;
    class GridPanel;
    struct GridSettings_data;

    /**
     * ImGuiOverlayRuntime - Separate module for ImGui overlay integration
     * 
     * This module encapsulates the ImGui overlay system for runtime use,
     * integrating ImGuiManager, GridPanel, and GridSettings.
     * It provides a simple API to integrate into the SDL3 callbacks.
     * 
     * Usage:
     *   - Call Init() after SDL window/renderer creation (in SDL_AppInit)
     *   - Call ProcessEvent() at the beginning of SDL_AppEvent
     *   - Call BeginFrame() after SDL_RenderClear
     *   - Call Draw() and Render() after world rendering, before SDL_RenderPresent
     *   - Call Shutdown() in SDL_AppQuit before destroying SDL objects
     */
    class ImGuiOverlayRuntime
    {
    public:
        ImGuiOverlayRuntime();
        ~ImGuiOverlayRuntime();

        /**
         * Initialize the ImGui overlay runtime with SDL window and renderer.
         * Must be called after SDL objects are created.
         * 
         * @param window   SDL window pointer
         * @param renderer SDL renderer pointer
         * @return true if initialization succeeded, false otherwise
         */
        bool Init(SDL_Window* window, SDL_Renderer* renderer);

        /**
         * Shutdown the ImGui overlay runtime.
         * Must be called before destroying SDL objects.
         */
        void Shutdown();

        /**
         * Process SDL events for ImGui.
         * Should be called at the beginning of SDL_AppEvent.
         * 
         * @param event SDL event pointer
         */
        void ProcessEvent(SDL_Event* event);

        /**
         * Begin a new ImGui frame.
         * Should be called after SDL_RenderClear, before drawing UI.
         */
        void BeginFrame();

        /**
         * Draw ImGui windows (GridPanel, etc.).
         * Should be called after world rendering, before Render().
         */
        void Draw();

        /**
         * Render ImGui draw data to the screen.
         * Should be called after Draw(), before SDL_RenderPresent.
         */
        void Render();

        /**
         * Check if the overlay is initialized.
         */
        bool IsInitialized() const { return m_initialized; }

        /**
         * Check if the overlay is currently visible.
         */
        bool IsVisible() const { return m_visible; }

        /**
         * Set overlay visibility.
         */
        void SetVisible(bool visible) { m_visible = visible; }

    private:
        bool m_initialized;
        bool m_visible; // Toggle overlay visibility (default: true)

        // Owned objects
        ImGuiManager*      m_imguiManager;
        GridPanel*         m_gridPanel;
        GridSettings_data* m_gridSettings;
    };
}
