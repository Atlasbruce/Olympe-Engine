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
{
    m_bankPathBuffer[0] = 0;
    m_stateNameBuffer[0] = 0;
    m_animationNameBuffer[0] = 0;
    EnsureDocument();
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

    ImGui::Separator();
    ImGui::Text("States");
    if (data.contains("states") && data["states"].is_array())
    {
        const nlohmann::json& states = data["states"];
        for (size_t i = 0; i < states.size(); ++i)
        {
            std::string stateName = "";
            std::string clipName = "";
            if (states[i].contains("name") && states[i]["name"].is_string())
                stateName = states[i]["name"].get<std::string>();
            if (states[i].contains("animation") && states[i]["animation"].is_string())
                clipName = states[i]["animation"].get<std::string>();
            ImGui::BulletText("%s -> %s", stateName.c_str(), clipName.c_str());
        }
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
}

void AnimationGraphRenderer::RenderInspectorPanel()
{
    ImGui::Text("Properties");
    ImGui::Separator();
    ImGui::Text("Blueprint Type: AnimationGraph");
    ImGui::Text("Minimap: %s", m_minimapVisible ? "On" : "Off");
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
    ImGui::BeginChild("AnimGraphToolbar", ImVec2(0, 0), false);
    RenderToolbar();
    ImGui::Separator();
    RenderMainPanel();
    ImGui::Separator();
    RenderStateEditorPanel();
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
