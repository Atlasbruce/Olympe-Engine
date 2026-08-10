#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"
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

bool AnimationGraphRenderer::IsPointInsideState(float x, float y, size_t stateIndex) const
{
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array() || stateIndex >= data["states"].size())
        return false;
    const nlohmann::json& s = data["states"][stateIndex];
    float sx = s.value("x", 0.0f);
    float sy = s.value("y", 0.0f);
    return x >= sx && x <= sx + 180.0f && y >= sy && y <= sy + 70.0f;
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
    if (ImGui::Button("Save")) Save("");
    ImGui::SameLine();
    if (ImGui::Button("Save As")) m_document->OnDocumentModified();
    ImGui::SameLine();
    if (ImGui::Button("Browse")) {}
    ImGui::SameLine();
    if (ImGui::Button("Verify")) VerifyGraph();
    ImGui::SameLine();
    if (ImGui::Button("Run")) RunGraph();
    ImGui::SameLine();
    ImGui::Checkbox("Minimap", &m_minimapVisible);
    ImGui::SameLine();
    ImGui::Text("Size %.2f", m_minimapSize);
    ImGui::SameLine();
    ImGui::Text("Pos %d", m_minimapPosition);
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
        ImGui::Text("Available clips:");
        const std::vector<std::string>& clips = m_document->GetAvailableAnimations();
        for (size_t i = 0; i < clips.size(); ++i)
            ImGui::BulletText("%s", clips[i].c_str());
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
    ImGui::Text("Animation Graph");
    ImGui::Separator();
    RenderGraphCanvas();
}

void AnimationGraphRenderer::RenderRightPanel()
{
    const nlohmann::json& data = m_document->GetData();
    std::string bankRef = "";
    std::string defaultState = "Idle";
    if (data.contains("animationBankRef") && data["animationBankRef"].is_string())
        bankRef = data["animationBankRef"].get<std::string>();
    if (data.contains("defaultState") && data["defaultState"].is_string())
        defaultState = data["defaultState"].get<std::string>();

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
    ImGui::Separator();
    RenderInspectorPanel();
    ImGui::Separator();
    RenderStateEditorPanel();
    ImGui::Separator();
    RenderTransitionEditorPanel();
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
            m_selectedStateIndex = -1;
            m_selectedTransitionIndex = -1;
        }
        if (selected)
        {
            ImGui::SameLine();
            ImGui::Text("(%.2f)", eventTime);
            ImGui::PopStyleColor();
        }
    }
}

void AnimationGraphRenderer::RenderGraphCanvas()
{
    EnsureStatePositions();
    nlohmann::json& data = m_document->GetDataMutable();
    if (!data.contains("states") || !data["states"].is_array())
        return;

    const float nodeW = 180.0f;
    const float nodeH = 70.0f;
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.y < 320.0f)
        canvasSize.y = 320.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasOrigin, ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y), IM_COL32(32, 32, 40, 255));
    ImGui::InvisibleButton("##animgraph_canvas", canvasSize);
    bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouse = ImGui::GetIO().MousePos;

    if (data["states"].empty())
    {
        dl->AddText(ImVec2(canvasOrigin.x + 24.0f, canvasOrigin.y + 24.0f), IM_COL32(180, 180, 190, 255), "No states yet. Import a bank or add one manually.");
        if (ImGui::Button("Add default state"))
        {
            m_document->AddStateFromClip("Idle", "Idle");
            m_document->SetDefaultState("Idle");
        }
        return;
    }

    nlohmann::json& states = data["states"];
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
        dl->PathLineTo(p1);
        dl->PathBezierCubicCurveTo(c1, c2, p2, 20);
        ImU32 color = IM_COL32(255, 220, 80, 255);
        if (m_selectedTransitionIndex == static_cast<int>(i))
            color = IM_COL32(120, 200, 255, 255);
        dl->PathStroke(color, false, (m_selectedTransitionIndex == static_cast<int>(i)) ? 3.0f : 2.0f);

        if (hovered)
        {
            float dist = DistanceToCubicBezier(mouse, p1, c1, c2, p2);
            if (dist < 8.0f && ImGui::IsMouseClicked(0))
            {
                m_selectedTransitionIndex = static_cast<int>(i);
                m_selectedStateIndex = -1;
            }
        }
    }

    for (size_t i = 0; i < states.size(); ++i)
    {
        std::string name = states[i].value("name", "");
        std::string anim = states[i].value("animation", "");
        float x = states[i].value("x", 0.0f);
        float y = states[i].value("y", 0.0f);
        ImVec2 min(canvasOrigin.x + x, canvasOrigin.y + y);
        ImVec2 max(min.x + nodeW, min.y + nodeH);
        bool isHover = hovered && mouse.x >= min.x && mouse.x <= max.x && mouse.y >= min.y && mouse.y <= max.y;
        bool isDown = isHover && ImGui::IsMouseClicked(0);
        if (isDown)
        {
            m_draggedStateIndex = static_cast<int>(i);
            m_selectedStateIndex = static_cast<int>(i);
            m_selectedTransitionIndex = -1;
        }
        if (isHover && ImGui::IsMouseClicked(1))
            m_selectedStateIndex = static_cast<int>(i);
        if (m_draggedStateIndex == static_cast<int>(i) && ImGui::IsMouseDown(0))
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

    if (m_selectedStateIndex >= 0)
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

    if (m_selectedEventIndex >= 0 && data.contains("events") && data["events"].is_array() && m_selectedEventIndex < static_cast<int>(data["events"].size()))
    {
        ImGui::Separator();
        ImGui::Text("Selected event #%d", m_selectedEventIndex);
        nlohmann::json& event = data["events"][m_selectedEventIndex];
        std::string stateName = event.value("state", "");
        std::string eventName = event.value("name", "");
        float eventTime = event.value("time", 0.0f);

        char stateBuffer[128];
        char eventBuffer[128];
        strncpy_s(stateBuffer, stateName.c_str(), _TRUNCATE);
        strncpy_s(eventBuffer, eventName.c_str(), _TRUNCATE);

        if (ImGui::InputText("Event state", stateBuffer, sizeof(stateBuffer)))
        {
            event["state"] = stateBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::InputText("Event name", eventBuffer, sizeof(eventBuffer)))
        {
            event["name"] = eventBuffer;
            m_document->OnDocumentModified();
        }
        if (ImGui::DragFloat("Event time", &eventTime, 0.01f, 0.0f, 1.0f))
        {
            event["time"] = eventTime;
            m_document->OnDocumentModified();
        }
        if (ImGui::Button("Delete selected event"))
        {
            m_document->RemoveEvent(static_cast<size_t>(m_selectedEventIndex));
            m_selectedEventIndex = -1;
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

void AnimationGraphRenderer::RenderFrameworkModals()
{
}

} // namespace Olympe
