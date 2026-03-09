/**
 * @file DebugPanel.h
 * @brief ImGui debug panel for ATS VS runtime debugging (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * DebugPanel provides the debug toolbar (Continue/Pause/Step/Stop),
 * breakpoints list, SubGraph call stack, and live blackboard watch.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>

namespace Olympe {

/**
 * @class DebugPanel
 * @brief ImGui panel that exposes DebugController UI.
 */
class DebugPanel {
public:

    DebugPanel();
    ~DebugPanel();

    void Initialize();
    void Shutdown();

    /**
     * @brief Renders the debug panel window.
     * Includes toolbar, breakpoints list, call stack, and watch variables.
     */
    void Render();

    /** @brief Show / hide the panel. */
    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const  { return m_visible; }

private:

    void RenderToolbar();
    void RenderBreakpointsList();
    void RenderCallStack();
    void RenderWatchVariables();
    void RenderProfilerSummary();

    bool m_visible = true;
};

} // namespace Olympe
