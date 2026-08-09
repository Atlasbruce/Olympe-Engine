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
    , m_draggedStateIndex(-1)
    , m_selectedStateIndex(-1)
    , m_selectedTransitionIndex(-1)
{
    m_bankPathBuffer[0] = 0;
    m_stateNameBuffer[0] = 0;
    m_animationNameBuffer[0] = 0;
    m_transitionFromBuffer[0] = 0;
    m_transitionToBuffer[0] = 0;
    m_transitionTimeBuffer = 0.1f;
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
}

void AnimationGraphRenderer::RenderMainPanel()
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
    ImGui::Text("Default state: %s", defaultState.c_str());
    ImGui::Separator();
    ImGui::Text("States: %d", data.contains("states") && data["states"].is_array() ? (int)data["states"].size() : 0);
    ImGui::Text("Transitions: %d", data.contains("transitions") && data["transitions"].is_array() ? (int)data["transitions"].size() : 0);
    ImGui::Text("Events: %d", data.contains("events") && data["events"].is_array() ? (int)data["events"].size() : 0);
    if (m_showRunPreview)
    {
        ImGui::Separator();
        ImGui::Text("Run preview active");
    }

    ImGui::Separator();
    RenderGraphCanvas();
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

    nlohmann::json& states = data["states"];
    for (size_t i = 0; i < data["transitions"].size(); ++i)
    {
        const nlohmann::json& tr = data["transitions"][i];
        std::string from = tr.value("from", "");
        std::string to = tr.value("to", "");
        int fi = FindStateIndexByName(from);
        int ti = FindStateIndexByName(to);
        if (fi < 0 || ti < 0) continue;
        float fx = states[fi].value("x", 0.0);
        float fy = states[fi].value("y", 0.0);
        float tx = states[ti].value("x", 0.0);
        float ty = states[ti].value("y", 0.0);
        ImVec2 p1(canvasOrigin.x + fx + nodeW, canvasOrigin.y + fy + nodeH * 0.5f);
        ImVec2 p2(canvasOrigin.x + tx, canvasOrigin.y + ty + nodeH * 0.5f);
        ImVec2 c1(p1.x + 60.0f, p1.y);
        ImVec2 c2(p2.x - 60.0f, p2.y);
        dl->PathLineTo(p1);
        dl->PathBezierCubicCurveTo(c1, c2, p2, 20);
        dl->PathStroke(IM_COL32(255, 220, 80, 255), false, 2.0f);
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
}

void AnimationGraphRenderer::RenderInspectorPanel()
{
    ImGui::Text("Properties");
    ImGui::Separator();
    ImGui::Text("Blueprint Type: AnimationGraph");
    ImGui::Text("Minimap: %s", m_minimapVisible ? "On" : "Off");

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
    RenderMainPanel();
    ImGui::Separator();
    RenderStateEditorPanel();
    ImGui::Separator();
    RenderInspectorPanel();
    ImGui::Separator();
    RenderTransitionEditorPanel();
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
