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
};

} // namespace Olympe
