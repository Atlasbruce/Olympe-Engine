/**
 * @file DebugPanel.cpp
 * @brief Implementation of DebugPanel (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant.
 */

#include "DebugPanel.h"
#include "DebugController.h"
#include "PerformanceProfiler.h"

#include "../third_party/imgui/imgui.h"
#include "../TaskSystem/TaskGraphTypes.h"

#include <string>
#include <vector>

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

DebugPanel::DebugPanel()
{
}

DebugPanel::~DebugPanel()
{
}

void DebugPanel::Initialize()
{
}

void DebugPanel::Shutdown()
{
}

// ============================================================================
// Render
// ============================================================================

void DebugPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("Debugger", &m_visible);

    RenderToolbar();
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Breakpoints", ImGuiTreeNodeFlags_DefaultOpen))
        RenderBreakpointsList();

    if (ImGui::CollapsingHeader("Call Stack", ImGuiTreeNodeFlags_DefaultOpen))
        RenderCallStack();

    if (ImGui::CollapsingHeader("Watch Variables", ImGuiTreeNodeFlags_DefaultOpen))
        RenderWatchVariables();

    if (ImGui::CollapsingHeader("Profiler Summary"))
        RenderProfilerSummary();

    ImGui::End();
}

// ============================================================================
// Toolbar
// ============================================================================

void DebugPanel::RenderToolbar()
{
    DebugController& dc = DebugController::Get();
    DebugState state    = dc.GetState();

    bool isDebugging = state != DebugState::NotDebugging;
    bool isPaused    = state == DebugState::Paused;
    bool isRunning   = state == DebugState::Running;

    // [▶ Continue]
    if (!isPaused)
        ImGui::BeginDisabled();
    if (ImGui::Button("▶ Continue (F5)") || (isPaused && ImGui::IsKeyPressed(ImGuiKey_F5)))
    {
        dc.Continue();
    }
    if (!isPaused)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // [⏸ Pause]
    if (!isRunning)
        ImGui::BeginDisabled();
    if (ImGui::Button("⏸ Pause"))
    {
        dc.Pause();
    }
    if (!isRunning)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // [⏭ Step Next] F10
    if (!isPaused)
        ImGui::BeginDisabled();
    if (ImGui::Button("⏭ Step (F10)") || (isPaused && ImGui::IsKeyPressed(ImGuiKey_F10)))
    {
        dc.StepNext();
    }
    if (!isPaused)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // [⤵ Step Into] F11
    if (!isPaused)
        ImGui::BeginDisabled();
    if (ImGui::Button("⤵ Into (F11)") || (isPaused && ImGui::IsKeyPressed(ImGuiKey_F11)))
    {
        dc.StepInto();
    }
    if (!isPaused)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // [⤴ Step Out] Shift+F11
    if (!isPaused)
        ImGui::BeginDisabled();
    bool shiftF11 = isPaused &&
                    ImGui::IsKeyPressed(ImGuiKey_F11) &&
                    ImGui::GetIO().KeyShift;
    if (ImGui::Button("⤴ Out") || shiftF11)
    {
        dc.StepOut();
    }
    if (!isPaused)
        ImGui::EndDisabled();

    ImGui::SameLine();

    // [⏹ Stop]
    if (!isDebugging)
        ImGui::BeginDisabled();
    if (ImGui::Button("⏹ Stop"))
    {
        dc.StopDebugging();
    }
    if (!isDebugging)
        ImGui::EndDisabled();

    // State indicator
    ImGui::SameLine(0, 20);
    const char* stateStr = "●";
    ImVec4 stateCol      = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    switch (state)
    {
        case DebugState::Running:      stateStr = "● Running";   stateCol = {0.2f,0.8f,0.2f,1.f}; break;
        case DebugState::Paused:       stateStr = "⏸ Paused";   stateCol = {1.0f,0.8f,0.0f,1.f}; break;
        case DebugState::StepNext:     stateStr = "⏭ Step";     stateCol = {0.8f,0.8f,0.2f,1.f}; break;
        case DebugState::StepInto:     stateStr = "⤵ Into";     stateCol = {0.2f,0.8f,0.8f,1.f}; break;
        case DebugState::StepOut:      stateStr = "⤴ Out";      stateCol = {0.2f,0.6f,0.8f,1.f}; break;
        case DebugState::NotDebugging: stateStr = "○ Idle";     stateCol = {0.5f,0.5f,0.5f,1.f}; break;
    }
    ImGui::TextColored(stateCol, "%s", stateStr);

    // Graph/node info
    int gid = dc.GetCurrentGraphID();
    int nid = dc.GetCurrentNodeID();
    if (gid >= 0 && nid >= 0)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("  graph=%d  node=%d", gid, nid);
    }
}

// ============================================================================
// Breakpoints list
// ============================================================================

void DebugPanel::RenderBreakpointsList()
{
    DebugController& dc = DebugController::Get();
    auto bps = dc.GetAllBreakpoints();

    if (bps.empty())
    {
        ImGui::TextDisabled("(no breakpoints)");
        return;
    }

    if (ImGui::SmallButton("Clear All##bpClearAll"))
        dc.ClearAllBreakpoints();

    ImGui::BeginChild("BPList", ImVec2(0, 140), true);

    for (size_t i = 0; i < bps.size(); ++i)
    {
        BreakpointInfo& bp = bps[i];
        ImGui::PushID(static_cast<int>(i));

        bool enabled = bp.enabled;
        if (ImGui::Checkbox("##bpEnabled", &enabled))
            dc.SetBreakpointEnabled(bp.graphID, bp.nodeID, enabled);

        ImGui::SameLine();
        ImGui::Text("%s : Node %d \"%s\"",
                    bp.graphName.empty() ? "(graph)" : bp.graphName.c_str(),
                    bp.nodeID,
                    bp.nodeName.empty() ? "" : bp.nodeName.c_str());

        ImGui::SameLine();
        if (ImGui::SmallButton("×##bpDel"))
            dc.ClearBreakpoint(bp.graphID, bp.nodeID);

        ImGui::PopID();
    }

    ImGui::EndChild();
}

// ============================================================================
// Call stack
// ============================================================================

void DebugPanel::RenderCallStack()
{
    DebugController& dc = DebugController::Get();
    auto stack = dc.GetCallStack();

    if (stack.empty())
    {
        ImGui::TextDisabled("(no active call stack)");
        return;
    }

    ImGui::BeginChild("CallStack", ImVec2(0, 100), true);

    for (int idx = static_cast<int>(stack.size()) - 1; idx >= 0; --idx)
    {
        const SubGraphStackFrame& frame = stack[static_cast<size_t>(idx)];
        if (frame.isCurrent)
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                               "→ %s : Node %d \"%s\"  [CURRENT]",
                               frame.graphName.empty() ? "(graph)" : frame.graphName.c_str(),
                               frame.nodeID,
                               frame.nodeName.empty() ? "" : frame.nodeName.c_str());
        }
        else
        {
            ImGui::Text("  %s : Node %d \"%s\"",
                        frame.graphName.empty() ? "(graph)" : frame.graphName.c_str(),
                        frame.nodeID,
                        frame.nodeName.empty() ? "" : frame.nodeName.c_str());
        }
    }

    ImGui::EndChild();
}

// ============================================================================
// Watch variables
// ============================================================================

void DebugPanel::RenderWatchVariables()
{
    DebugController& dc = DebugController::Get();
    if (!dc.IsDebugging())
    {
        ImGui::TextDisabled("(not debugging)");
        return;
    }

    LocalBlackboard bb = dc.GetCurrentBlackboard();
    auto keys = bb.GetVariableNames();

    if (keys.empty())
    {
        ImGui::TextDisabled("(empty blackboard)");
        return;
    }

    ImGui::BeginChild("WatchVars", ImVec2(0, 180), true);
    ImGui::TextDisabled("Local Blackboard:");
    ImGui::Separator();

    for (size_t i = 0; i < keys.size(); ++i)
    {
        const std::string& key = keys[i];
        TaskValue val;
        try { val = bb.GetValue(key); }
        catch (...) { continue; }

        switch (val.GetType())
        {
            case VariableType::Bool:
                ImGui::Text("  %-22s : Bool(%s)",   key.c_str(), val.AsBool() ? "true" : "false");
                break;
            case VariableType::Int:
                ImGui::Text("  %-22s : Int(%d)",    key.c_str(), val.AsInt());
                break;
            case VariableType::Float:
                ImGui::Text("  %-22s : Float(%.3f)",key.c_str(), val.AsFloat());
                break;
            case VariableType::String:
                ImGui::Text("  %-22s : String(\"%s\")", key.c_str(), val.AsString().c_str());
                break;
            case VariableType::EntityID:
                ImGui::Text("  %-22s : EntityID(%llu)",
                            key.c_str(),
                            static_cast<unsigned long long>(val.AsEntityID()));
                break;
            case VariableType::Vector:
            {
                ::Vector v = val.AsVector();
                ImGui::Text("  %-22s : Vec(%.2f,%.2f,%.2f)",
                            key.c_str(), v.x, v.y, v.z);
                break;
            }
            default:
                ImGui::Text("  %-22s : (unknown)", key.c_str());
                break;
        }
    }

    ImGui::EndChild();
}

// ============================================================================
// Profiler summary
// ============================================================================

void DebugPanel::RenderProfilerSummary()
{
    PerformanceProfiler& prof = PerformanceProfiler::Get();

    if (!prof.IsProfiling())
    {
        if (ImGui::Button("Start Profiling"))
            prof.BeginProfiling();
        return;
    }

    if (ImGui::Button("Stop Profiling"))
        prof.StopProfiling();

    auto hotspots = prof.GetHotspots();
    if (hotspots.empty())
    {
        ImGui::TextDisabled("(no data yet)");
        return;
    }

    ImGui::BeginChild("ProfSummary", ImVec2(0, 120), true);
    ImGui::Text("%-6s %-24s %8s %8s %8s",
                "NodeID", "Name", "AvgMs", "MaxMs", "Count");
    ImGui::Separator();

    for (size_t i = 0; i < hotspots.size() && i < 8; ++i)
    {
        const NodeExecutionMetrics& m = hotspots[i];
        bool isHot = m.avgTimeMs > PerformanceProfiler::HOTSPOT_THRESHOLD_MS;
        if (isHot)
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                               "%-6d %-24s %8.3f %8.3f %8llu",
                               m.nodeID, m.nodeName.c_str(),
                               m.avgTimeMs, m.maxTimeMs, m.executionCount);
        else
            ImGui::Text("%-6d %-24s %8.3f %8.3f %8llu",
                        m.nodeID, m.nodeName.c_str(),
                        m.avgTimeMs, m.maxTimeMs, m.executionCount);
    }

    ImGui::EndChild();
}

} // namespace Olympe
