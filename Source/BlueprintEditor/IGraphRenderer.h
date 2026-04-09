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
    virtual ~IGraphRenderer() {}

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
};

} // namespace Olympe

