/**
 * @file ProfilerPanel.h
 * @brief ImGui profiler panel for ATS VS node execution metrics (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant.
 */

#pragma once

#include <string>

namespace Olympe {

/**
 * @class ProfilerPanel
 * @brief ImGui panel that visualizes PerformanceProfiler data.
 *
 * @details
 * Renders a frame timeline bar chart and a hotspot table sorted by
 * average execution time descending.  Provides an "Export CSV" button
 * that calls PerformanceProfiler::SaveToFile().
 */
class ProfilerPanel {
public:

    ProfilerPanel();
    ~ProfilerPanel();

    void Initialize();
    void Shutdown();

    /**
     * @brief Renders the profiler panel window.
     */
    void Render();

    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const  { return m_visible; }

private:

    void RenderControls();
    void RenderFrameTimeline();
    void RenderHotspotTable();

    bool        m_visible          = true;
    bool        m_autoScroll       = true;
    int         m_historyCount     = 60;   ///< Frames shown in timeline
};

} // namespace Olympe
