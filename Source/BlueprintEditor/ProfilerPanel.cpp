/**
 * @file ProfilerPanel.cpp
 * @brief Implementation of ProfilerPanel (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant.
 */

#include "ProfilerPanel.h"
#include "PerformanceProfiler.h"

#include "../third_party/imgui/imgui.h"

#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

ProfilerPanel::ProfilerPanel()
{
}

ProfilerPanel::~ProfilerPanel()
{
}

void ProfilerPanel::Initialize()
{
}

void ProfilerPanel::Shutdown()
{
}

// ============================================================================
// Render
// ============================================================================

void ProfilerPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("Profiler", &m_visible);

    RenderControls();
    ImGui::Separator();
    RenderFrameTimeline();
    ImGui::Separator();
    RenderHotspotTable();

    ImGui::End();
}

// ============================================================================
// Controls toolbar
// ============================================================================

void ProfilerPanel::RenderControls()
{
    PerformanceProfiler& prof = PerformanceProfiler::Get();

    if (!prof.IsProfiling())
    {
        if (ImGui::Button("▶ Start Profiling"))
            prof.BeginProfiling();
    }
    else
    {
        if (ImGui::Button("⏹ Stop Profiling"))
            prof.StopProfiling();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear##profClear"))
        prof.Clear();

    ImGui::SameLine();

    // Export CSV
    if (ImGui::Button("Export CSV"))
    {
        // Build timestamp filename
        std::time_t t = std::time(nullptr);
        char timeBuf[32];
#ifdef _WIN32
        struct tm tmInfo;
        localtime_s(&tmInfo, &t);
        std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &tmInfo);
#else
        struct tm tmInfo;
        localtime_r(&t, &tmInfo);
        std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &tmInfo);
#endif
        std::string csvPath = std::string("profiler_export_") + timeBuf + ".csv";
        if (prof.SaveToFile(csvPath))
            ImGui::OpenPopup("ExportOK");
        else
            ImGui::OpenPopup("ExportFail");
    }

    if (ImGui::BeginPopup("ExportOK"))
    {
        ImGui::Text("CSV exported successfully.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopup("ExportFail"))
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "Export failed.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderInt("Frames##profFrames", &m_historyCount, 10, 120);

    ImGui::SameLine();
    ImGui::Checkbox("Auto Scroll##profScroll", &m_autoScroll);
}

// ============================================================================
// Frame timeline
// ============================================================================

void ProfilerPanel::RenderFrameTimeline()
{
    ImGui::TextDisabled("Frame Timeline");

    PerformanceProfiler& prof = PerformanceProfiler::Get();
    auto history = prof.GetFrameHistory(m_historyCount);

    if (history.empty())
    {
        ImGui::TextDisabled("(no frame data — start profiling)");
        return;
    }

    ImGui::BeginChild("FrameTimeline", ImVec2(0, 160), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    for (size_t f = 0; f < history.size(); ++f)
    {
        const FrameProfile& frame = history[f];
        ImGui::Text("F%llu (%.1fms):",
                    static_cast<unsigned long long>(frame.frameNumber),
                    frame.totalFrameTimeMs);

        for (size_t n = 0; n < frame.nodeMetrics.size() && n < 8; ++n)
        {
            const NodeExecutionMetrics& m = frame.nodeMetrics[n];
            bool isHot = m.executionTimeMs > PerformanceProfiler::HOTSPOT_THRESHOLD_MS;

            // Simple ASCII bar
            int barLen = static_cast<int>(m.executionTimeMs * 4.0f);
            if (barLen < 1) barLen = 1;
            if (barLen > 40) barLen = 40;

            std::string bar(static_cast<size_t>(barLen), isHot ? '█' : '░');

            if (isHot)
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                                   "  Node %-3d %-18s %s %.2fms",
                                   m.nodeID, m.nodeName.c_str(),
                                   bar.c_str(), m.executionTimeMs);
            else
                ImGui::Text("  Node %-3d %-18s %s %.2fms",
                            m.nodeID, m.nodeName.c_str(),
                            bar.c_str(), m.executionTimeMs);
        }
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
}

// ============================================================================
// Hotspot table
// ============================================================================

void ProfilerPanel::RenderHotspotTable()
{
    ImGui::TextDisabled("Hotspot Table  (sorted by avgTimeMs ▼)");

    PerformanceProfiler& prof = PerformanceProfiler::Get();
    auto hotspots = prof.GetHotspots();

    if (hotspots.empty())
    {
        ImGui::TextDisabled("(no data)");
        return;
    }

    ImGui::BeginChild("HotspotTable", ImVec2(0, 180), true);

    ImGui::Text("%-6s %-24s %9s %9s %9s",
                "NodeID", "Name", "AvgMs", "MaxMs", "Count");
    ImGui::Separator();

    for (size_t i = 0; i < hotspots.size(); ++i)
    {
        const NodeExecutionMetrics& m = hotspots[i];
        bool isHot = m.avgTimeMs > PerformanceProfiler::HOTSPOT_THRESHOLD_MS;

        if (isHot)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                               "%-6d %-24s %9.3f %9.3f %9llu",
                               m.nodeID, m.nodeName.c_str(),
                               m.avgTimeMs, m.maxTimeMs, m.executionCount);
        else
            ImGui::Text("%-6d %-24s %9.3f %9.3f %9llu",
                        m.nodeID, m.nodeName.c_str(),
                        m.avgTimeMs, m.maxTimeMs, m.executionCount);
    }

    ImGui::EndChild();
}

} // namespace Olympe
