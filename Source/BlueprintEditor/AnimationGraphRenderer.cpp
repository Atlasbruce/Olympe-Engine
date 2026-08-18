#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"  // Legacy data model
#include "Framework/AnimationGraphFrameworkDocument.h"  // Phase 55: Framework adapter
#include "Framework/CanvasFramework.h"  // Phase 55: Framework orchestrator
#include "Framework/CanvasToolbarRenderer.h"  // Phase 55: Toolbar rendering
#include "Framework/CanvasModalRenderer.h"
#include "Framework/BlueprintDropRouting.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <fstream>

namespace Olympe {

AnimationGraphRenderer::AnimationGraphRenderer()
    : m_minimapVisible(true)
    , m_minimapSize(0.15f)
    , m_minimapPosition(1)
    , m_showVerification(false)
    , m_showRunPreview(false)
    , m_previewTime(0.0f)
    , m_previewDuration(1.0f)
    , m_rightPanelWidth(320.0f)
    , m_timelinePanelHeight(220.0f)
    , m_draggedStateIndex(-1)
    , m_selectedStateIndex(-1)
    , m_selectedTransitionIndex(-1)
    , m_selectedEventIndex(-1)
    , m_linkStartStateIndex(-1)
    , m_hasPendingTransitionDrag(false)
    , m_transitionFromIndex(-1)
    , m_linkDragStartPos(0.0f, 0.0f)
{
    m_bankPathBuffer[0] = 0;
    m_stateNameBuffer[0] = 0;
    m_animationNameBuffer[0] = 0;
    m_transitionFromBuffer[0] = 0;
    m_transitionToBuffer[0] = 0;
    m_eventNameBuffer[0] = 0;
    m_eventStateBuffer[0] = 0;
    m_transitionTimeBuffer = 0.1f;
    m_eventTimeBuffer = 0.5f;
    EnsureDocument();
}

int AnimationGraphRenderer::FindStateIndexByName(const std::string& name) const
{
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return -1;
    const nlohmann::json& states = data["states"];
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (states[i].contains("name") && states[i]["name"].is_string() && states[i]["name"].get<std::string>() == name)
            return static_cast<int>(i);
    }
    return -1;
}

std::string AnimationGraphRenderer::MakeUniqueStateName(const std::string& baseName) const
{
    if (FindStateIndexByName(baseName) < 0)
        return baseName;

    for (int suffix = 2; suffix < 1000; ++suffix)
    {
        std::string candidate = baseName + "_" + std::to_string(suffix);
        if (FindStateIndexByName(candidate) < 0)
            return candidate;
    }
    return baseName;
}

int AnimationGraphRenderer::FindEventIndexByState(const std::string& stateName) const
{
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("events") || !data["events"].is_array())
        return -1;
    const nlohmann::json& events = data["events"];
    for (size_t i = 0; i < events.size(); ++i)
    {
        if (events[i].contains("state") && events[i]["state"].is_string() && events[i]["state"].get<std::string>() == stateName)
            return static_cast<int>(i);
    }
    return -1;
}

float AnimationGraphRenderer::DistanceToCubicBezier(const ImVec2& p, const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3) const
{
    float best = 1e9f;
    ImVec2 prev = p0;
    for (int i = 1; i <= 24; ++i)
    {
        float t = static_cast<float>(i) / 24.0f;
        float u = 1.0f - t;
        ImVec2 cur(
            u*u*u*p0.x + 3.0f*u*u*t*p1.x + 3.0f*u*t*t*p2.x + t*t*t*p3.x,
            u*u*u*p0.y + 3.0f*u*u*t*p1.y + 3.0f*u*t*t*p2.y + t*t*t*p3.y);
        ImVec2 a = prev;
        ImVec2 b = cur;
        float vx = b.x - a.x;
        float vy = b.y - a.y;
        float wx = p.x - a.x;
        float wy = p.y - a.y;
        float c1 = vx * wx + vy * wy;
        float c2 = vx * vx + vy * vy;
        float tseg = c2 > 0.0f ? c1 / c2 : 0.0f;
        if (tseg < 0.0f) tseg = 0.0f;
        if (tseg > 1.0f) tseg = 1.0f;
        ImVec2 proj(a.x + tseg * vx, a.y + tseg * vy);
        float dx = p.x - proj.x;
        float dy = p.y - proj.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < best) best = dist;
        prev = cur;
    }
    return best;
}

void AnimationGraphRenderer::EnsureStatePositions()
{
    nlohmann::json& data = m_document->GetDataMutable();
    if (!data.contains("states") || !data["states"].is_array())
        return;
    nlohmann::json& states = data["states"];
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (!states[i].contains("x")) states[i]["x"] = 80.0 + static_cast<double>(i % 3) * 220.0;
        if (!states[i].contains("y")) states[i]["y"] = 80.0 + static_cast<double>(i / 3) * 140.0;
    }
}

AnimationGraphRenderer::~AnimationGraphRenderer() {}

void AnimationGraphRenderer::EnsureDocument()
{
    if (!m_document)
    {
        m_document = std::make_unique<AnimationGraphDocument>();
        m_document->SetRenderer(this);
    }

    // Phase 55: Initialize framework for toolbar/modals if not already done
    if (!m_framework)
    {
        m_frameworkDocument = std::make_unique<AnimationGraphFrameworkDocument>(this);
        m_framework = std::make_unique<CanvasFramework>(m_frameworkDocument.get());
        SYSTEM_LOG << "[AnimationGraphRenderer] CanvasFramework initialized for AnimationGraph\n";
    }
}

bool AnimationGraphRenderer::Load(const std::string& path)
{
    EnsureDocument();
    if (!m_document->Load(path))
        return false;
    m_currentPath = path;
    return true;
}

bool AnimationGraphRenderer::Save(const std::string& path)
{
    EnsureDocument();
    const std::string target = path.empty() ? m_currentPath : path;
    if (!m_document->Save(target))
        return false;
    m_currentPath = target;
    return true;
}

void AnimationGraphRenderer::RenderToolbar()
{
    // Phase 55: Use unified framework toolbar instead of local buttons
    if (m_framework)
    {
        m_framework->GetToolbar()->Render();
    }
    else
    {
        // Fallback for early initialization edge cases
        ImGui::TextDisabled("[Framework not initialized]");
    }
}

void AnimationGraphRenderer::RenderStateEditorPanel()
{
    const nlohmann::json& data = m_document->GetData();
    ImGui::Text("Animation Bank");
    ImGui::Separator();

    if (m_bankPathBuffer[0] == 0 && m_document && !m_document->GetAnimationBankPath().empty())
        strncpy_s(m_bankPathBuffer, m_document->GetAnimationBankPath().c_str(), _TRUNCATE);

    if (ImGui::InputText("Bank path", m_bankPathBuffer, sizeof(m_bankPathBuffer)))
    {
        m_document->SetAnimationBankPath(m_bankPathBuffer);
    }

    if (ImGui::Button("Apply bank path"))
    {
        m_document->SetAnimationBankPath(m_bankPathBuffer);
    }
    ImGui::SameLine();
    if (ImGui::Button("Import states from bank"))
    {
        m_document->GenerateStatesFromBank();
    }

    ImGui::Separator();
    ImGui::Text("Create state from clip");
    ImGui::InputText("State name", m_stateNameBuffer, sizeof(m_stateNameBuffer));
    ImGui::InputText("Clip name", m_animationNameBuffer, sizeof(m_animationNameBuffer));
    if (m_document->GetAvailableAnimations().empty())
        ImGui::TextDisabled("No bank animations loaded yet");
    else
    {
        ImGui::Text("Available clips (drag onto canvas, or double-click to add):");
        const std::vector<std::string>& clips = m_document->GetAvailableAnimations();
        for (size_t i = 0; i < clips.size(); ++i)
        {
            ImGui::Selectable(clips[i].c_str(), false, 0, ImVec2(0, 0));

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                std::string uniqueName = MakeUniqueStateName(clips[i]);
                m_document->AddStateFromClip(uniqueName, clips[i]);
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("ANIMGRAPH_CLIP", clips[i].c_str(), clips[i].size() + 1);
                ImGui::Text("New state: %s", clips[i].c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
    if (ImGui::Button("Add state"))
    {
        if (m_document->AddStateFromClip(m_stateNameBuffer, m_animationNameBuffer))
        {
            m_stateNameBuffer[0] = 0;
            m_animationNameBuffer[0] = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Default = first"))
    {
        const std::vector<std::string>& clips = m_document->GetAvailableAnimations();
        if (!clips.empty())
            m_document->SetDefaultState(clips.front());
    }
    ImGui::SameLine();
    if (ImGui::Button("Generate defaults"))
    {
        m_document->GenerateDefaultTransitions();
    }
    ImGui::SameLine();
    if (ImGui::Button("Auto layout"))
    {
        m_document->AutoLayoutStates();
    }

    ImGui::Separator();
    ImGui::Text("States");
    if (data.contains("states") && data["states"].is_array())
    {
        const nlohmann::json& states = data["states"];
        for (size_t i = 0; i < states.size(); ++i)
        {
            std::string stateName = "";
            std::string clipName = "";
            double x = 0.0;
            double y = 0.0;
            if (states[i].contains("name") && states[i]["name"].is_string())
                stateName = states[i]["name"].get<std::string>();
            if (states[i].contains("animation") && states[i]["animation"].is_string())
                clipName = states[i]["animation"].get<std::string>();
            if (states[i].contains("x"))
                x = states[i]["x"].get<double>();
            if (states[i].contains("y"))
                y = states[i]["y"].get<double>();
            ImGui::BulletText("%s -> %s [%.0f, %.0f]", stateName.c_str(), clipName.c_str(), x, y);
        }
    }
}

void AnimationGraphRenderer::RenderTransitionEditorPanel()
{
    const nlohmann::json& data = m_document->GetData();
    ImGui::Text("Transitions");
    ImGui::Separator();
    ImGui::InputText("From state", m_transitionFromBuffer, sizeof(m_transitionFromBuffer));
    ImGui::InputText("To state", m_transitionToBuffer, sizeof(m_transitionToBuffer));
    ImGui::DragFloat("Blend time", &m_transitionTimeBuffer, 0.01f, 0.0f, 2.0f);
    if (ImGui::Button("Add transition"))
    {
        m_document->AddTransition(m_transitionFromBuffer, m_transitionToBuffer, m_transitionTimeBuffer);
    }

    if (data.contains("transitions") && data["transitions"].is_array())
    {
        const nlohmann::json& transitions = data["transitions"];
        for (size_t i = 0; i < transitions.size(); ++i)
        {
            std::string fromState = "";
            std::string toState = "";
            float blend = 0.1f;
            if (transitions[i].contains("from") && transitions[i]["from"].is_string())
                fromState = transitions[i]["from"].get<std::string>();
            if (transitions[i].contains("to") && transitions[i]["to"].is_string())
                toState = transitions[i]["to"].get<std::string>();
            if (transitions[i].contains("transitionTime"))
                blend = transitions[i]["transitionTime"].get<float>();
            ImGui::BulletText("%s -> %s (%.2fs)", fromState.c_str(), toState.c_str(), blend);
            if (ImGui::IsItemClicked(0))
                m_selectedTransitionIndex = static_cast<int>(i);
        }
    }
    if (m_selectedTransitionIndex >= 0 && ImGui::Button("Delete selected transition"))
    {
        m_document->RemoveTransition(static_cast<size_t>(m_selectedTransitionIndex));
        m_selectedTransitionIndex = -1;
    }

    ImGui::Separator();
    ImGui::Text("Events");
    ImGui::InputText("Event state", m_eventStateBuffer, sizeof(m_eventStateBuffer));
    ImGui::InputText("Event name", m_eventNameBuffer, sizeof(m_eventNameBuffer));
    ImGui::DragFloat("Event time", &m_eventTimeBuffer, 0.01f, 0.0f, 1.0f);
    if (ImGui::Button("Add event"))
    {
        m_document->AddEvent(m_eventStateBuffer, m_eventNameBuffer, m_eventTimeBuffer);
        m_eventNameBuffer[0] = 0;
    }

    if (data.contains("events") && data["events"].is_array())
    {
        const nlohmann::json& events = data["events"];
        for (size_t i = 0; i < events.size(); ++i)
        {
            std::string stateName = events[i].value("state", "");
            std::string eventName = events[i].value("name", "");
            float eventTime = events[i].value("time", 0.0f);
            ImGui::BulletText("%s @ %s (%.2f)", eventName.c_str(), stateName.c_str(), eventTime);
        }
    }
}

void AnimationGraphRenderer::RenderMainPanel()
{
    RenderGraphCanvas();
}

void AnimationGraphRenderer::RenderRightPanel()
{
    RenderRightPanelTabs();
}

void AnimationGraphRenderer::RenderRightPanelTabs()
{
    const nlohmann::json& data = m_document->GetData();
    std::string bankRef = "";
    std::string defaultState = "Idle";
    if (data.contains("animationBankRef") && data["animationBankRef"].is_string())
        bankRef = data["animationBankRef"].get<std::string>();
    if (data.contains("defaultState") && data["defaultState"].is_string())
        defaultState = data["defaultState"].get<std::string>();

    if (ImGui::BeginTabBar("AnimationGraphRightTabs"))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            ImGui::Text("Animation Graph");
            ImGui::Separator();
            ImGui::Text("Path: %s", m_currentPath.empty() ? "(unsaved)" : m_currentPath.c_str());
            ImGui::Text("Bank: %s", bankRef.c_str());
            ImGui::Text("Prefab: %s", m_document->GetPrefabPath().empty() ? "(unbound)" : m_document->GetPrefabPath().c_str());
            ImGui::Text("Graph bind: %s", m_document->GetAnimationGraphPath().empty() ? "(unbound)" : m_document->GetAnimationGraphPath().c_str());
            ImGui::Text("Default state: %s", defaultState.c_str());
            ImGui::Text("States: %d", data.contains("states") && data["states"].is_array() ? (int)data["states"].size() : 0);
            ImGui::Text("Transitions: %d", data.contains("transitions") && data["transitions"].is_array() ? (int)data["transitions"].size() : 0);
            ImGui::Text("Events: %d", data.contains("events") && data["events"].is_array() ? (int)data["events"].size() : 0);
            if (m_showRunPreview)
                ImGui::Text("Run preview active");

            ImGui::Separator();
            ImGui::Text("Editor");
            ImGui::Separator();
            ImGui::Text("Minimap: %s", m_minimapVisible ? "On" : "Off");
            ImGui::Text("Binding status: %s",
                (!m_document->GetPrefabPath().empty() && !m_document->GetAnimationGraphPath().empty() && !m_document->GetAnimationBankPath().empty()) ? "Ready" : "Incomplete");
            const std::vector<std::string> warnings = m_document->ValidateBinding();
            if (!warnings.empty())
            {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Warnings");
                for (size_t i = 0; i < warnings.size(); ++i)
                    ImGui::BulletText("%s", warnings[i].c_str());
            }
            ImGui::Separator();
            RenderInspectorPanel();
            ImGui::Separator();
            ImGui::Text("Events");
            ImGui::InputText("Event state", m_eventStateBuffer, sizeof(m_eventStateBuffer));
            ImGui::InputText("Event name", m_eventNameBuffer, sizeof(m_eventNameBuffer));
            ImGui::DragFloat("Event time", &m_eventTimeBuffer, 0.01f, 0.0f, 1.0f);
            if (ImGui::Button("Add event"))
            {
                m_document->AddEvent(m_eventStateBuffer, m_eventNameBuffer, m_eventTimeBuffer);
                m_eventNameBuffer[0] = 0;
            }

            if (data.contains("events") && data["events"].is_array())
            {
                const nlohmann::json& events = data["events"];
                for (size_t i = 0; i < events.size(); ++i)
                {
                    std::string stateName = events[i].value("state", "");
                    std::string eventName = events[i].value("name", "");
                    float eventTime = events[i].value("time", 0.0f);
                    bool selected = (m_selectedEventIndex == static_cast<int>(i));
                    if (selected)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
                    if (ImGui::Selectable((eventName + " @ " + stateName).c_str(), selected))
                    {
                        m_selectedEventIndex = static_cast<int>(i);
                        if (i < m_eventStateBuffer[0] == 0)
                            strncpy_s(m_eventStateBuffer, stateName.c_str(), _TRUNCATE);
                        strncpy_s(m_eventNameBuffer, eventName.c_str(), _TRUNCATE);
                        m_eventTimeBuffer = eventTime;
                    }
                    if (selected)
                        ImGui::PopStyleColor();
                }
            }
            if (m_selectedEventIndex >= 0 && ImGui::Button("Delete selected event"))
            {
                m_document->RemoveEvent(static_cast<size_t>(m_selectedEventIndex));
                m_selectedEventIndex = -1;
                m_eventNameBuffer[0] = 0;
                m_eventStateBuffer[0] = 0;
            }

            if (m_selectedEventIndex >= 0 && data.contains("events") && data["events"].is_array() && m_selectedEventIndex < static_cast<int>(data["events"].size()))
            {
                ImGui::Separator();
                ImGui::Text("Selected event #%d", m_selectedEventIndex);
                nlohmann::json& event = m_document->GetDataMutable()["events"][m_selectedEventIndex];
                std::string stateName = event.value("state", "");
                std::string eventName = event.value("name", "");
                float eventTime = event.value("time", 0.0f);

                char stateBuffer[128];
                char eventBuffer[128];
                strncpy_s(stateBuffer, stateName.c_str(), _TRUNCATE);
                strncpy_s(eventBuffer, eventName.c_str(), _TRUNCATE);

                if (ImGui::InputText("Selected event state", stateBuffer, sizeof(stateBuffer)))
                {
                    event["state"] = stateBuffer;
                    m_document->OnDocumentModified();
                }
                if (ImGui::InputText("Selected event name", eventBuffer, sizeof(eventBuffer)))
                {
                    event["name"] = eventBuffer;
                    m_document->OnDocumentModified();
                }
                if (ImGui::DragFloat("Selected event time", &eventTime, 0.01f, 0.0f, 1.0f))
                {
                    event["time"] = eventTime;
                    m_document->OnDocumentModified();
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Nodes"))
        {
            RenderStateEditorPanel();
            ImGui::Separator();
            RenderTransitionEditorPanel();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void AnimationGraphRenderer::RenderTimelinePanel()
{
    ImGui::Text("Timeline");
    ImGui::Separator();
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("events") || !data["events"].is_array() || data["events"].empty())
    {
        ImGui::TextDisabled("No events yet.");
        return;
    }

    const nlohmann::json& events = data["events"];
    if (m_previewDuration < 0.1f)
        m_previewDuration = 0.1f;

    ImGui::Text("Preview");
    ImGui::Checkbox("Run preview", &m_showRunPreview);
    ImGui::DragFloat("Duration", &m_previewDuration, 0.01f, 0.1f, 10.0f);
    ImGui::SliderFloat("Scrub", &m_previewTime, 0.0f, m_previewDuration);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 barMin = ImGui::GetCursorScreenPos();
    ImVec2 barSize = ImVec2(ImGui::GetContentRegionAvail().x, 24.0f);
    dl->AddRectFilled(barMin, ImVec2(barMin.x + barSize.x, barMin.y + barSize.y), IM_COL32(45, 45, 55, 255));
    dl->AddRect(barMin, ImVec2(barMin.x + barSize.x, barMin.y + barSize.y), IM_COL32(120, 120, 130, 255));

    for (size_t i = 0; i < events.size(); ++i)
    {
        float eventTime = events[i].value("time", 0.0f);
        float normalized = m_previewDuration > 0.0f ? (eventTime / m_previewDuration) : 0.0f;
        if (normalized < 0.0f) normalized = 0.0f;
        if (normalized > 1.0f) normalized = 1.0f;
        float x = barMin.x + normalized * barSize.x;
        dl->AddLine(ImVec2(x, barMin.y), ImVec2(x, barMin.y + barSize.y), IM_COL32(255, 200, 80, 255), 2.0f);
    }

    float scrubNorm = m_previewDuration > 0.0f ? (m_previewTime / m_previewDuration) : 0.0f;
    float scrubX = barMin.x + scrubNorm * barSize.x;
    dl->AddLine(ImVec2(scrubX, barMin.y - 2.0f), ImVec2(scrubX, barMin.y + barSize.y + 2.0f), IM_COL32(80, 200, 255, 255), 3.0f);
    ImGui::Dummy(ImVec2(barSize.x, barSize.y + 8.0f));

}

void AnimationGraphRenderer::RenderGraphCanvas()
{
    EnsureStatePositions();
    nlohmann::json& data = m_document->GetDataMutable();
    if (!data.contains("states") || !data["states"].is_array())
        return;

    const float nodeW = 180.0f;
    const float nodeH = 70.0f;
    const float portRadius = 6.0f;
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.y < 320.0f)
        canvasSize.y = 320.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(32, 32, 40, 255));
    ImGui::InvisibleButton("##animgraph_canvas", canvasSize);
    bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();

    // Phase 53: Update transition preview endpoint when dragging
    if (m_hasPendingTransitionDrag)
    {
        m_transitionPreviewEnd = mouse;
    }
    bool isClipPayload = activePayload && activePayload->IsDataType("ANIMGRAPH_CLIP");

    // Drag/drop: accept an animation clip dragged from the palette (RenderStateEditorPanel)
    // and create a new state node at the drop position, mirroring the EntityPrefab
    // ComponentPalettePanel -> PrefabCanvas drag-drop pattern.
    if (isClipPayload && hovered)
    {
        dl->AddRect(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(255, 255, 0, 255), 0.0f, 0, 4.0f);
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMGRAPH_CLIP"))
        {
            std::string clipName(static_cast<const char*>(payload->Data), static_cast<size_t>(payload->DataSize) - 1);
            BlueprintDropContext ctx;
            ctx.graphType = "AnimationGraph";
            ctx.payloadType = "ANIMGRAPH_CLIP";
            ctx.payloadValue = clipName;
            ctx.screenX = mouse.x;
            ctx.screenY = mouse.y;
            LogBlueprintDropReceived(ctx);
            LogBlueprintDropCreate(ctx);
            float dropX = mouse.x - canvasOrigin.x - nodeW * 0.5f;
            float dropY = mouse.y - canvasOrigin.y - nodeH * 0.5f;
            if (dropX < 0.0f) dropX = 0.0f;
            if (dropY < 0.0f) dropY = 0.0f;
            std::string uniqueName = MakeUniqueStateName(clipName);
            if (m_document->AddStateNode(uniqueName, clipName, dropX, dropY))
            {
                LogBlueprintDropCreated(ctx, uniqueName);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (data["states"].empty())
    {
        dl->AddText(ImVec2(canvasOrigin.x + 24.0f, canvasOrigin.y + 24.0f), IM_COL32(180, 180, 190, 255), "No states yet. Drag a clip here, import a bank, or add one manually.");
        return;
    }

    nlohmann::json& states = data["states"];
    // Phase 54: Render all transitions with directional arrows
    for (size_t i = 0; i < data["transitions"].size(); ++i)
    {
        RenderSingleTransition(i, canvasOrigin);
    }

    // Phase 54: Hit-test transitions for selection/context menu
    if (hovered)
    {
        for (size_t i = 0; i < data["transitions"].size(); ++i)
        {
            const nlohmann::json& tr = data["transitions"][i];
            std::string from = tr.value("from", "");
            std::string to = tr.value("to", "");
            int fi = FindStateIndexByName(from);
            int ti = FindStateIndexByName(to);
            if (fi < 0 || ti < 0) continue;

            float fx = states[fi].value("x", 0.0f);
            float fy = states[fi].value("y", 0.0f);
            float tx = states[ti].value("x", 0.0f);
            float ty = states[ti].value("y", 0.0f);

            ImVec2 p1(canvasOrigin.x + fx + nodeW, canvasOrigin.y + fy + nodeH * 0.5f);
            ImVec2 p2(canvasOrigin.x + tx, canvasOrigin.y + ty + nodeH * 0.5f);
            ImVec2 c1(p1.x + 60.0f, p1.y);
            ImVec2 c2(p2.x - 60.0f, p2.y);

            float dist = DistanceToCubicBezier(mouse, p1, c1, c2, p2);
            if (dist < 8.0f)
            {
                if (ImGui::IsMouseClicked(0))
                {
                    m_selectedTransitionIndex = static_cast<int>(i);
                    m_selectedStateIndex = -1;
                }
                if (ImGui::IsMouseClicked(1))
                {
                    m_selectedTransitionIndex = static_cast<int>(i);
                    m_selectedStateIndex = -1;
                    ImGui::OpenPopup("AnimTransitionContextMenu");
                }
            }
        }
    }

    // If a link (transition) is currently being dragged from a source state's output port,
    // draw a live preview line following the mouse (mirrors PrefabCanvas connection preview).
    bool linkResolvedThisFrame = false;

    for (size_t i = 0; i < states.size(); ++i)
    {
        std::string name = states[i].value("name", "");
        std::string anim = states[i].value("animation", "");
        float x = states[i].value("x", 0.0f);
        float y = states[i].value("y", 0.0f);
        ImVec2 min(canvasOrigin.x + x, canvasOrigin.y + y);
        ImVec2 max(min.x + nodeW, min.y + nodeH);
        ImVec2 outPortPos(max.x, min.y + nodeH * 0.5f);
        ImVec2 inPortPos(min.x, min.y + nodeH * 0.5f);
        float outDist = sqrtf((mouse.x - outPortPos.x) * (mouse.x - outPortPos.x) + (mouse.y - outPortPos.y) * (mouse.y - outPortPos.y));
        float inDist = sqrtf((mouse.x - inPortPos.x) * (mouse.x - inPortPos.x) + (mouse.y - inPortPos.y) * (mouse.y - inPortPos.y));
        bool isHoverOutPort = hovered && outDist <= (portRadius + 3.0f);
        bool isHoverInPort = hovered && inDist <= (portRadius + 3.0f);
        bool isHover = hovered && mouse.x >= min.x && mouse.x <= max.x && mouse.y >= min.y && mouse.y <= max.y;

        if (isHoverOutPort && ImGui::IsMouseClicked(0))
        {
            // Start transition creation from the output pin.
            // Consume the interaction so the node body does not begin dragging.
            m_selectedStateIndex = static_cast<int>(i);
            m_selectedTransitionIndex = -1;
            m_draggedStateIndex = -1;
            m_hasPendingTransitionDrag = true;
            m_transitionFromIndex = static_cast<int>(i);
            m_linkDragStartPos = outPortPos;
            return;
        }
        else if (isHoverInPort && ImGui::IsMouseClicked(0))
        {
            // Input pin also starts a transition gesture, but does not move the node.
            m_selectedStateIndex = static_cast<int>(i);
            m_selectedTransitionIndex = -1;
            m_draggedStateIndex = -1;
            m_hasPendingTransitionDrag = true;
            m_transitionFromIndex = static_cast<int>(i);
            m_linkDragStartPos = inPortPos;
            return;
        }
        else if (isHover && ImGui::IsMouseClicked(0))
        {
            // Only the node body should start dragging.
            m_draggedStateIndex = static_cast<int>(i);
            m_selectedStateIndex = static_cast<int>(i);
            m_selectedTransitionIndex = -1;
        }
        if (isHover && ImGui::IsMouseClicked(1))
        {
            m_selectedStateIndex = static_cast<int>(i);
            m_selectedTransitionIndex = -1;
            ImGui::OpenPopup("AnimStateContextMenu");
        }
        if (m_hasPendingTransitionDrag && m_transitionFromIndex == static_cast<int>(i))
        {
            if (ImGui::IsMouseReleased(0))
            {
                bool linkedSuccessfully = false;

                for (size_t j = 0; j < states.size(); ++j)
                {
                    if (static_cast<int>(j) == m_transitionFromIndex)
                        continue;

                    const nlohmann::json& targetState = states[j];
                    float tx = targetState.value("x", 0.0f);
                    float ty = targetState.value("y", 0.0f);
                    ImVec2 targetMin(canvasOrigin.x + tx, canvasOrigin.y + ty);
                    ImVec2 targetMax(targetMin.x + nodeW, targetMin.y + nodeH);
                    ImVec2 targetInPort(targetMin.x, targetMin.y + nodeH * 0.5f);
                    ImVec2 targetOutPort(targetMax.x, targetMin.y + nodeH * 0.5f);

                    float dxIn = mouse.x - targetInPort.x;
                    float dyIn = mouse.y - targetInPort.y;
                    float dxOut = mouse.x - targetOutPort.x;
                    float dyOut = mouse.y - targetOutPort.y;

                    bool hitTargetPort = (dxIn * dxIn + dyIn * dyIn) <= (portRadius + 3.0f) * (portRadius + 3.0f)
                        || (dxOut * dxOut + dyOut * dyOut) <= (portRadius + 3.0f) * (portRadius + 3.0f);

                    if (hitTargetPort)
                    {
                        std::string fromName = states[m_transitionFromIndex].value("name", "");
                        std::string toName = targetState.value("name", "");
                        if (m_document->AddTransition(fromName, toName, 0.1f))
                        {
                            SYSTEM_LOG << "[AnimationGraphRenderer] Created transition: " << fromName << " -> " << toName << "\n";
                            linkedSuccessfully = true;
                        }
                        break;
                    }
                }

                if (!linkedSuccessfully)
                {
                    SYSTEM_LOG << "[AnimationGraphRenderer] Transition drag cancelled (no valid target)\n";
                }

                m_hasPendingTransitionDrag = false;
                m_transitionFromIndex = -1;
            }
        }
        else if (m_draggedStateIndex == static_cast<int>(i) && ImGui::IsMouseDown(0))
        {
            states[i]["x"] = x + ImGui::GetIO().MouseDelta.x;
            states[i]["y"] = y + ImGui::GetIO().MouseDelta.y;
            m_document->OnDocumentModified();
        }
        if (ImGui::IsMouseReleased(0))
            m_draggedStateIndex = -1;

        ImU32 fill = (name == m_document->GetDefaultState()) ? IM_COL32(80, 120, 200, 255) : IM_COL32(60, 60, 70, 255);
        ImU32 border = isHover ? IM_COL32(255, 220, 80, 255) : IM_COL32(180, 180, 180, 255);
        dl->AddRectFilled(min, max, fill, 6.0f);
        dl->AddRect(min, max, border, 6.0f, 0, 2.0f);
        dl->AddText(ImVec2(min.x + 10.0f, min.y + 10.0f), IM_COL32(255,255,255,255), name.c_str());
        dl->AddText(ImVec2(min.x + 10.0f, min.y + 36.0f), IM_COL32(220,220,220,255), anim.c_str());

        // Ports: left = input, right = output (drag from output port to another node to create a transition)
        ImU32 outPortColor = isHoverOutPort ? IM_COL32(255, 240, 120, 255) : IM_COL32(200, 170, 60, 255);
        dl->AddCircleFilled(outPortPos, portRadius, outPortColor);
        dl->AddCircle(outPortPos, portRadius, IM_COL32(20, 20, 20, 255), 0, 1.5f);
        dl->AddCircleFilled(inPortPos, portRadius, IM_COL32(90, 160, 220, 255));
        dl->AddCircle(inPortPos, portRadius, IM_COL32(20, 20, 20, 255), 0, 1.5f);
    }

    // Phase 53: Render transition preview line while dragging
    RenderTransitionPreview();

    if (ImGui::BeginPopup("AnimStateContextMenu"))
    {
        if (m_selectedStateIndex >= 0 && m_selectedStateIndex < static_cast<int>(states.size()))
        {
            std::string ctxName = states[m_selectedStateIndex].value("name", "");
            ImGui::TextDisabled("State: %s", ctxName.c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Set As Default"))
                m_document->SetDefaultState(ctxName);
            if (ImGui::MenuItem("Create Event Here"))
            {
                m_eventStateBuffer[0] = 0;
                strncpy_s(m_eventStateBuffer, ctxName.c_str(), _TRUNCATE);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete State"))
            {
                m_document->RemoveState(ctxName);
                m_selectedStateIndex = -1;
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("AnimTransitionContextMenu"))
    {
        if (data.contains("transitions") && data["transitions"].is_array()
            && m_selectedTransitionIndex >= 0 && m_selectedTransitionIndex < static_cast<int>(data["transitions"].size()))
        {
            const nlohmann::json& ctxTransition = data["transitions"][m_selectedTransitionIndex];
            ImGui::TextDisabled("%s -> %s", ctxTransition.value("from", "").c_str(), ctxTransition.value("to", "").c_str());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Transition"))
            {
                m_document->RemoveTransition(static_cast<size_t>(m_selectedTransitionIndex));
                m_selectedTransitionIndex = -1;
            }
        }
        ImGui::EndPopup();
    }

    if (m_selectedStateIndex >= 0 && ImGui::Button("Delete selected state"))
    {
        const nlohmann::json& statesRef = data["states"];
        if (m_selectedStateIndex < static_cast<int>(statesRef.size()))
        {
            std::string stateName = statesRef[m_selectedStateIndex].value("name", "");
            m_document->RemoveState(stateName);
            m_selectedStateIndex = -1;
        }
    }

    if (m_selectedTransitionIndex >= 0)
    {
        ImGui::SameLine();
        if (ImGui::Button("Delete selected transition"))
        {
            m_document->RemoveTransition(static_cast<size_t>(m_selectedTransitionIndex));
            m_selectedTransitionIndex = -1;
        }
    }

    if (ImGui::Button("Create event from selected state"))
    {
        if (m_selectedStateIndex >= 0 && data.contains("states") && data["states"].is_array() && m_selectedStateIndex < static_cast<int>(data["states"].size()))
        {
            m_eventStateBuffer[0] = 0;
            const std::string stateName = data["states"][m_selectedStateIndex].value("name", "");
            strncpy_s(m_eventStateBuffer, stateName.c_str(), _TRUNCATE);
        }
    }

    if (m_selectedStateIndex >= 0 && data.contains("states") && data["states"].is_array() && m_selectedStateIndex < static_cast<int>(data["states"].size()))
    {
        const std::string currentState = data["states"][m_selectedStateIndex].value("name", "");
        int eventIndex = FindEventIndexByState(currentState);
        if (eventIndex >= 0)
        {
            m_selectedEventIndex = eventIndex;
        }
    }

    if (m_selectedTransitionIndex >= 0 && data.contains("transitions") && data["transitions"].is_array() && m_selectedTransitionIndex < static_cast<int>(data["transitions"].size()))
    {
        ImGui::Separator();
        ImGui::Text("Selected transition #%d", m_selectedTransitionIndex);
        nlohmann::json& transition = data["transitions"][m_selectedTransitionIndex];

        std::string fromState = transition.value("from", "");
        std::string toState = transition.value("to", "");
        float transitionTime = transition.value("transitionTime", 0.1f);

        char fromBuffer[128];
        char toBuffer[128];
        strncpy_s(fromBuffer, fromState.c_str(), _TRUNCATE);
        strncpy_s(toBuffer, toState.c_str(), _TRUNCATE);

        if (ImGui::InputText("Transition from", fromBuffer, sizeof(fromBuffer)))
        {
            transition["from"] = fromBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::InputText("Transition to", toBuffer, sizeof(toBuffer)))
        {
            transition["to"] = toBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("Transition time", &transitionTime, 0.01f, 0.0f, 2.0f))
        {
            transition["transitionTime"] = transitionTime;
            m_document->OnDocumentModified();
        }
    }

}

void AnimationGraphRenderer::RenderInspectorPanel()
{
    ImGui::Text("Properties");
    ImGui::Separator();
    ImGui::Text("Blueprint Type: AnimationGraph");
    ImGui::Text("Minimap: %s", m_minimapVisible ? "On" : "Off");

    char prefabBuffer[512];
    char graphBuffer[512];
    strncpy_s(prefabBuffer, m_document->GetPrefabPath().c_str(), _TRUNCATE);
    strncpy_s(graphBuffer, m_document->GetAnimationGraphPath().c_str(), _TRUNCATE);
    ImGui::Separator();
    ImGui::Text("Binding");
    if (ImGui::InputText("Prefab ref", prefabBuffer, sizeof(prefabBuffer)))
        m_document->SetPrefabPath(prefabBuffer);
    if (ImGui::InputText("Animation graph ref", graphBuffer, sizeof(graphBuffer)))
        m_document->SetAnimationGraphPath(graphBuffer);
    if (!m_document->GetPrefabPath().empty() && !m_document->GetAnimationGraphPath().empty() && !m_document->GetAnimationBankPath().empty())
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Binding ready");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "Binding incomplete");

    if (ImGui::Button("Copy runtime export to clipboard"))
    {
        std::string runtimeJson = m_document->BuildRuntimeJson().dump(2);
        ImGui::SetClipboardText(runtimeJson.c_str());
    }

    nlohmann::json& data = m_document->GetDataMutable();
    if (m_selectedStateIndex >= 0 && data.contains("states") && data["states"].is_array() && m_selectedStateIndex < static_cast<int>(data["states"].size()))
    {
        nlohmann::json& state = data["states"][m_selectedStateIndex];
        ImGui::Separator();
        ImGui::Text("Selected state");

        std::string stateName = state.value("name", "");
        std::string animation = state.value("animation", "");
        float x = state.value("x", 0.0f);
        float y = state.value("y", 0.0f);
        bool loop = state.value("loop", true);
        float speed = state.value("speed", 1.0f);
        int priority = state.value("priority", 0);

        char nameBuffer[128];
        char animBuffer[128];
        strncpy_s(nameBuffer, stateName.c_str(), _TRUNCATE);
        strncpy_s(animBuffer, animation.c_str(), _TRUNCATE);

        if (ImGui::InputText("State name", nameBuffer, sizeof(nameBuffer)))
        {
            state["name"] = nameBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::InputText("Animation", animBuffer, sizeof(animBuffer)))
        {
            state["animation"] = animBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("X", &x, 1.0f))
        {
            state["x"] = x;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("Y", &y, 1.0f))
        {
            state["y"] = y;
            m_document->OnDocumentModified();
        }
        if (ImGui::Checkbox("Loop", &loop))
        {
            state["loop"] = loop;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 8.0f))
        {
            state["speed"] = speed;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragInt("Priority", &priority, 1.0f, 0, 100))
        {
            state["priority"] = priority;
            m_document->OnDocumentModified();
        }
        if (ImGui::Button("Set as default"))
        {
            m_document->SetDefaultState(state.value("name", ""));
        }
    }
    else if (m_selectedTransitionIndex >= 0 && data.contains("transitions") && data["transitions"].is_array() && m_selectedTransitionIndex < static_cast<int>(data["transitions"].size()))
    {
        nlohmann::json& transition = data["transitions"][m_selectedTransitionIndex];
        ImGui::Separator();
        ImGui::Text("Selected transition");

        std::string fromState = transition.value("from", "");
        std::string toState = transition.value("to", "");
        float transitionTime = transition.value("transitionTime", 0.1f);

        char fromBuffer[128];
        char toBuffer[128];
        strncpy_s(fromBuffer, fromState.c_str(), _TRUNCATE);
        strncpy_s(toBuffer, toState.c_str(), _TRUNCATE);

        if (ImGui::InputText("From", fromBuffer, sizeof(fromBuffer)))
        {
            transition["from"] = fromBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::InputText("To", toBuffer, sizeof(toBuffer)))
        {
            transition["to"] = toBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("Blend time", &transitionTime, 0.01f, 0.0f, 2.0f))
        {
            transition["transitionTime"] = transitionTime;
            m_document->OnDocumentModified();
        }
    }
}

void AnimationGraphRenderer::RenderVerificationPanel()
{
    ImGui::Text("Verification Output");
    ImGui::Separator();
    if (m_logs.empty())
    {
        ImGui::TextDisabled("No verification logs");
        return;
    }
    for (size_t i = 0; i < m_logs.size(); ++i)
        ImGui::BulletText("%s", m_logs[i].c_str());
}

void AnimationGraphRenderer::Render()
{
    EnsureDocument();
    ImGui::BeginChild("AnimGraphToolbar", ImVec2(0, -1.0f), false);
    RenderToolbar();
    ImGui::Separator();
    float contentWidth = ImGui::GetContentRegionAvail().x;
    float contentHeight = ImGui::GetContentRegionAvail().y;
    float splitterSize = 6.0f;

    if (m_rightPanelWidth < 240.0f) m_rightPanelWidth = 240.0f;
    if (m_rightPanelWidth > contentWidth * 0.45f) m_rightPanelWidth = contentWidth * 0.45f;
    if (m_timelinePanelHeight < 160.0f) m_timelinePanelHeight = 160.0f;
    if (m_timelinePanelHeight > contentHeight * 0.6f) m_timelinePanelHeight = contentHeight * 0.6f;

    float leftWidth = contentWidth - m_rightPanelWidth - splitterSize;
    float canvasHeight = contentHeight - m_timelinePanelHeight - splitterSize;

    // Left column: canvas on top, timeline at bottom
    ImGui::BeginChild("AnimGraphLeftColumn", ImVec2(leftWidth, contentHeight), false);
    ImGui::BeginChild("AnimGraphCanvasPane", ImVec2(0, canvasHeight), true, ImGuiWindowFlags_NoScrollbar);
    // No-op placeholder: keep canvas pane render path explicit for re-evaluation.
    RenderMainPanel();
    ImGui::EndChild();

    ImGui::Button("##canvas_timeline_splitter", ImVec2(-1.0f, splitterSize));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_timelinePanelHeight -= ImGui::GetIO().MouseDelta.y;
        if (m_timelinePanelHeight < 160.0f) m_timelinePanelHeight = 160.0f;
        if (m_timelinePanelHeight > contentHeight * 0.6f) m_timelinePanelHeight = contentHeight * 0.6f;
        canvasHeight = contentHeight - m_timelinePanelHeight - splitterSize;
    }

    ImGui::BeginChild("AnimGraphTimelinePane", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
    RenderTimelinePanel();
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel with resize handle
    ImGui::Button("##left_right_splitter", ImVec2(splitterSize, contentHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
        if (m_rightPanelWidth < 240.0f) m_rightPanelWidth = 240.0f;
        if (m_rightPanelWidth > contentWidth * 0.45f) m_rightPanelWidth = contentWidth * 0.45f;
    }
    ImGui::SameLine();

    ImGui::BeginChild("AnimGraphRightPanel", ImVec2(0, contentHeight), true, ImGuiWindowFlags_NoScrollbar);
    RenderRightPanel();
    ImGui::EndChild();
    ImGui::EndChild();
}

bool AnimationGraphRenderer::IsDirty() const
{
    return m_document ? m_document->IsDirty() : false;
}

std::string AnimationGraphRenderer::GetGraphType() const
{
    return "AnimationGraph";
}

std::string AnimationGraphRenderer::GetCurrentPath() const
{
    return m_currentPath;
}

void AnimationGraphRenderer::SetMinimapSize(float size)
{
    if (size < 0.05f) size = 0.05f;
    if (size > 0.5f) size = 0.5f;
    m_minimapSize = size;
}

void AnimationGraphRenderer::SetMinimapPosition(int pos)
{
    if (pos < 0) pos = 0;
    if (pos > 3) pos = 3;
    m_minimapPosition = pos;
}

void AnimationGraphRenderer::VerifyGraph()
{
    m_logs.clear();
    const nlohmann::json& data = m_document->GetData();
    bool hasBankRef = data.contains("animationBankRef") && data["animationBankRef"].is_string() && !data["animationBankRef"].get<std::string>().empty();
    if (!hasBankRef)
        m_logs.push_back("Missing animationBankRef");
    if (!data.contains("states") || !data["states"].is_array() || data["states"].empty())
        m_logs.push_back("No states defined");
    if (!data.contains("transitions") || !data["transitions"].is_array())
        m_logs.push_back("No transitions section");
    if (m_logs.empty())
        m_logs.push_back("Graph valid");
}

void AnimationGraphRenderer::RunGraph()
{
    m_showRunPreview = true;
    m_logs.push_back("Run preview started");
}

void AnimationGraphRenderer::RenderTransitionPreview()
{
    // Phase 53: Render transition preview line while dragging
    if (!m_hasPendingTransitionDrag || m_transitionFromIndex < 0)
        return;

    nlohmann::json& data = m_document->GetDataMutable();
    if (!data.contains("states") || !data["states"].is_array())
        return;

    auto& states = data["states"];
    if (m_transitionFromIndex >= static_cast<int>(states.size()))
        return;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Use the saved m_linkDragStartPos (exact port position clicked) instead of recalculating
    // This ensures the preview line starts from the correct port location
    ImVec2 sourcePortPos = m_linkDragStartPos;

    // Yellow bezier curve from source port to mouse position
    float controlPointOffset = (m_transitionPreviewEnd.x - sourcePortPos.x) * 0.4f;
    ImVec2 cp1(sourcePortPos.x + controlPointOffset, sourcePortPos.y);
    ImVec2 cp2(m_transitionPreviewEnd.x - controlPointOffset, m_transitionPreviewEnd.y);

    // Draw bezier curve preview (yellow)
    dl->PathLineTo(sourcePortPos);
    dl->PathBezierCubicCurveTo(cp1, cp2, m_transitionPreviewEnd, 32);
    dl->PathStroke(IM_COL32(255, 200, 0, 255), false, 3.0f);

    // Optional: Pulsing highlight on hovered target state
    if (ImGui::IsMouseDown(0))  // Only while mouse is held
    {
        const float nodeW = 180.0f;
        const float nodeH = 70.0f;
        const float portRadius = 6.0f;
        const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
        ImVec2 mousePos = ImGui::GetIO().MousePos;

        // Check each state for hover
        for (size_t i = 0; i < states.size(); ++i)
        {
            if (static_cast<int>(i) == m_transitionFromIndex)
                continue;  // Skip self

            const auto& targetState = states[i];
            float tx = targetState.value("x", 100.0f);
            float ty = targetState.value("y", 100.0f);

            ImVec2 nodeTopLeft(canvasOrigin.x + tx, canvasOrigin.y + ty);
            ImVec2 nodeBottomRight(nodeTopLeft.x + nodeW, nodeTopLeft.y + nodeH);

            // Check if mouse is over this node
            if (mousePos.x >= nodeTopLeft.x && mousePos.x <= nodeBottomRight.x &&
                mousePos.y >= nodeTopLeft.y && mousePos.y <= nodeBottomRight.y)
            {
                // Pulsing highlight on target input port (left side, middle)
                float t = ImGui::GetTime();
                float pulse = 0.5f + 0.5f * sinf(t * 6.0f);
                float highlightRadius = 8.0f + pulse * 4.0f;
                ImU32 highlightCol = ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 1.0f, 0.5f + 0.5f * pulse));

                ImVec2 targetInputPort(nodeTopLeft.x, nodeTopLeft.y + nodeH * 0.5f);
                dl->AddCircleFilled(targetInputPort, highlightRadius, highlightCol);
                break;
            }
        }
    }
}

// Phase 54: Evaluate cubic Bezier at parameter t in [0,1]
// B(t) = (1-t)³P0 + 3(1-t)²t·P1 + 3(1-t)t²·P2 + t³·P3
ImVec2 AnimationGraphRenderer::EvaluateBezier(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t) const
{
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float mt3 = mt2 * mt;
    float t2 = t * t;
    float t3 = t2 * t;
    float w0 = mt3;
    float w1 = 3.0f * mt2 * t;
    float w2 = 3.0f * mt * t2;
    float w3 = t3;

    return ImVec2(
        w0 * p0.x + w1 * p1.x + w2 * p2.x + w3 * p3.x,
        w0 * p0.y + w1 * p1.y + w2 * p2.y + w3 * p3.y
    );
}

// Phase 54: Calculate tangent (derivative) at parameter t
// B'(t) = 3(1-t)²(P1-P0) + 6(1-t)t(P2-P1) + 3t²(P3-P2)
ImVec2 AnimationGraphRenderer::GetBezierTangent(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, float t) const
{
    float mt = 1.0f - t;
    float mt2 = mt * mt;
    float t2 = t * t;

    ImVec2 dp1 = ImVec2(p1.x - p0.x, p1.y - p0.y);
    ImVec2 dp2 = ImVec2(p2.x - p1.x, p2.y - p1.y);
    ImVec2 dp3 = ImVec2(p3.x - p2.x, p3.y - p2.y);

    ImVec2 tangent = ImVec2(
        3.0f * mt2 * dp1.x + 6.0f * mt * t * dp2.x + 3.0f * t2 * dp3.x,
        3.0f * mt2 * dp1.y + 6.0f * mt * t * dp2.y + 3.0f * t2 * dp3.y
    );

    // Normalize
    float len = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (len > 0.001f)
    {
        tangent.x /= len;
        tangent.y /= len;
    }

    return tangent;
}

// Phase 54: Draw arrow head pointing in tangent direction
void AnimationGraphRenderer::DrawArrowOnCurve(ImDrawList* dl, const ImVec2& curveEnd, const ImVec2& tangent, ImU32 color, float arrowSize)
{
    if (!dl) return;

    // Arrow back point (opposite to direction)
    ImVec2 backPoint = ImVec2(
        curveEnd.x - tangent.x * arrowSize,
        curveEnd.y - tangent.y * arrowSize
    );

    // Perpendicular vector (rotate 90 degrees)
    ImVec2 perp = ImVec2(-tangent.y, tangent.x);
    float arrowWidth = arrowSize * 0.5f;

    ImVec2 leftPoint = ImVec2(
        backPoint.x + perp.x * arrowWidth,
        backPoint.y + perp.y * arrowWidth
    );
    ImVec2 rightPoint = ImVec2(
        backPoint.x - perp.x * arrowWidth,
        backPoint.y - perp.y * arrowWidth
    );

    // Draw filled triangle: curveEnd, leftPoint, rightPoint
    dl->AddTriangleFilled(curveEnd, leftPoint, rightPoint, color);
}

// Phase 54: Render a single transition with arrow and source-based color
void AnimationGraphRenderer::RenderSingleTransition(size_t transitionIndex, const ImVec2& canvasOrigin)
{
    if (!m_document) return;

    auto data = m_document->GetDataMutable();
    if (data["transitions"].size() <= transitionIndex) return;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const nlohmann::json& transitions = data["transitions"];
    const nlohmann::json& states = data["states"];

    const nlohmann::json& tr = transitions[transitionIndex];
    std::string from = tr.value("from", "");
    std::string to = tr.value("to", "");

    int fi = FindStateIndexByName(from);
    int ti = FindStateIndexByName(to);
    if (fi < 0 || ti < 0) return;

    float fx = states[fi].value("x", 0.0f);
    float fy = states[fi].value("y", 0.0f);
    float tx = states[ti].value("x", 0.0f);
    float ty = states[ti].value("y", 0.0f);

    // Node dimensions
    float nodeW = 140.0f;
    float nodeH = 60.0f;

    // Output port is on the right (from-state)
    ImVec2 p1(canvasOrigin.x + fx + nodeW, canvasOrigin.y + fy + nodeH * 0.5f);
    // Input port is on the left (to-state)
    ImVec2 p2(canvasOrigin.x + tx, canvasOrigin.y + ty + nodeH * 0.5f);

    // Control points for Bézier
    ImVec2 c1(p1.x + 60.0f, p1.y);
    ImVec2 c2(p2.x - 60.0f, p2.y);

    // Draw the Bézier curve
    dl->PathLineTo(p1);
    dl->PathBezierCubicCurveTo(c1, c2, p2, 20);

    // Phase 54: Determine color based on source port (output = orange/yellow)
    ImU32 color = IM_COL32(255, 200, 80, 255);  // Output port default = orange
    if (m_selectedTransitionIndex == static_cast<int>(transitionIndex))
        color = IM_COL32(120, 200, 255, 255);  // Selected = light blue

    float lineWidth = (m_selectedTransitionIndex == static_cast<int>(transitionIndex)) ? 3.0f : 2.0f;
    dl->PathStroke(color, false, lineWidth);

    // Phase 54: Draw arrow head at the end (pointing toward input)
    ImVec2 tangent = GetBezierTangent(p1, c1, c2, p2, 0.95f);
    DrawArrowOnCurve(dl, p2, tangent, color, 10.0f);
}

void AnimationGraphRenderer::RenderFrameworkModals()
{
    CanvasModalRenderer::Get().RenderSaveFilePickerModal();
}

} // namespace Olympe
