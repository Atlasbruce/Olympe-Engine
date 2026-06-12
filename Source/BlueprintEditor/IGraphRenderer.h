/**
 * @file IGraphRenderer.h
 * @brief Abstract interface shared by all graph-renderer adapters.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Every concrete renderer (VisualScript, BehaviorTree, AnimGraph …) that can
 * live inside a TabManager tab must implement this interface.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>

namespace Olympe {

/**
 * @class IGraphRenderer
 * @brief Polymorphic interface for all graph editor renderers.
 *
 * Implementations are owned by TabManager via raw pointers (C++14 style).
 * Lifetime: the owning EditorTab deletes the renderer when the tab is closed.
 */
class IGraphRenderer
{
public:
    virtual ~IGraphRenderer() = default;

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    /** @brief Renders the graph canvas into the current ImGui child window. */
    virtual void Render() = 0;

    // ------------------------------------------------------------------
    // File I/O
    // ------------------------------------------------------------------

    /**
     * @brief Loads a graph from a file on disk.
     * @param path  Absolute or relative path to the .ats / .json file.
     * @return true on success.
     */
    virtual bool Load(const std::string& path) = 0;

    /**
     * @brief Saves the current graph state to disk.
     * @param path  Destination path.  If empty, save to the path passed to Load().
     * @return true on success.
     */
    virtual bool Save(const std::string& path) = 0;

    // ------------------------------------------------------------------
    // State queries
    // ------------------------------------------------------------------

    /** @brief Returns true when the graph has unsaved changes. */
    virtual bool IsDirty() const = 0;

    /**
     * @brief Returns the graph type string, e.g. "VisualScript", "BehaviorTree".
     */
    virtual std::string GetGraphType() const = 0;

    /**
     * @brief Returns the last path successfully loaded/saved, or empty string.
     */
    virtual std::string GetCurrentPath() const = 0;

    // ------------------------------------------------------------------
    // Canvas State Management (Phase 35.0 - Multi-Tab Fix)
    // ------------------------------------------------------------------

    /**
     * @brief Save the current canvas viewport state (pan, zoom, etc.)
     * Called when tab is deactivated.
     * Used to preserve viewport when switching between multiple tabs.
     */
    virtual void SaveCanvasState() {}

    /**
     * @brief Restore previously saved canvas viewport state
     * Called when tab is reactivated.
     * Ensures smooth tab switching without losing layout.
     */
    virtual void RestoreCanvasState() {}

    /**
     * @brief Get canvas state as JSON string for persistence
     * @return JSON representation of current canvas state
     */
    virtual std::string GetCanvasStateJSON() const
    {
        return "";  // Default: no persistent state
    }

    /**
     * @brief Restore canvas state from JSON string
     * @param json JSON representation of saved canvas state
     */
    virtual void SetCanvasStateJSON(const std::string& json)
    {
        (void)json;  // Default: no-op
    }

    // ------------------------------------------------------------------
    // Phase 55 - Unified Verification and Execution
    // ------------------------------------------------------------------

    /**
     * @brief Run graph verification to check for logic errors
     * Default: no-op
     */
    virtual void VerifyGraph() {}

    /**
     * @brief Execute the graph logic (Simulation/Runtime)
     * Default: no-op
     */
    virtual void RunGraph() {}

    /** @brief Returns true if this graph type supports verification. */
    virtual bool SupportsVerification() const { return false; }

    /** @brief Returns true if this graph type supports execution. */
    virtual bool SupportsExecution() const { return false; }

    // ------------------------------------------------------------------
    // Minimap Virtual Access (Unified Controls)
    // ------------------------------------------------------------------
    
    /** @brief Returns true if this graph renderer supports showing a minimap overlay. */
    virtual bool SupportsMinimap() const { return false; }

    /** @brief Get/Set current minimap visibility. */
    virtual bool IsMinimapVisible() const { return false; }
    virtual void SetMinimapVisible(bool visible) { (void)visible; }

    /** @brief Get/Set current minimap size ratio of the canvas [0.05..0.5]. */
    virtual float GetMinimapSize() const { return 0.15f; }
    virtual void SetMinimapSize(float size) { (void)size; }

    /** @brief Get/Set current minimap positions: 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight. */
    virtual int GetMinimapPosition() const { return 1; /* TopRight */ }
    virtual void SetMinimapPosition(int pos) { (void)pos; }

    // ------------------------------------------------------------------
    // Phase 43 - Framework Modal Rendering
    // ------------------------------------------------------------------

    /**
     * @brief Render all framework modals for this renderer
     * Called centrally from TabManager::RenderTabBar() to ensure single
     * rendering point per frame for all modals (Save/SaveAs/Browse).
     * This prevents duplicate UI zones and ensures consistent behavior.
     *
     * Default: no-op (renderers without modals do nothing)
     */
    virtual void RenderFrameworkModals()
    {
        // Default: no framework modals
    }
};

} // namespace Olympe

