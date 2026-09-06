#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Framework/AnimationGraphFrameworkDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/CanvasModalRenderer.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <sstream>

namespace Olympe {

AnimationGraphRenderer::AnimationGraphRenderer()
    : m_transitionTimeBuffer(0.1f)
    , m_minimapVisible(true)
    , m_minimapSize(0.15f)
    , m_minimapPosition(1)
    , m_selectedStateIndex(-1)
    , m_selectedTransitionIndex(-1)
{
    m_stateNameBuffer[0] = 0;
    m_animationNameBuffer[0] = 0;
    m_transitionFromBuffer[0] = 0;
    m_transitionToBuffer[0] = 0;
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
    if (!m_framework)
    {
        m_frameworkDocument = std::make_unique<AnimationGraphFrameworkDocument>(this);
        m_framework = std::make_unique<CanvasFramework>(m_frameworkDocument.get());
    }
}

bool AnimationGraphRenderer::Load(const std::string& path) { EnsureDocument(); if (!m_document->Load(path)) return false; m_currentPath = path; return true; }
bool AnimationGraphRenderer::Save(const std::string& path) { EnsureDocument(); const std::string target = path.empty() ? m_currentPath : path; if (!m_document->Save(target)) return false; m_currentPath = target; return true; }
void AnimationGraphRenderer::SetCurrentPath(const std::string& path) { m_currentPath = path; }

void AnimationGraphRenderer::SaveCanvasState()
{
    nlohmann::json state = nlohmann::json::object();
    state["minimapVisible"] = m_minimapVisible;
    state["minimapSize"] = m_minimapSize;
    state["minimapPosition"] = m_minimapPosition;
    state["selectedStateIndex"] = m_selectedStateIndex;
    state["selectedTransitionIndex"] = m_selectedTransitionIndex;
    m_canvasStateJson = state.dump();
}

void AnimationGraphRenderer::RestoreCanvasState() { SetCanvasStateJSON(m_canvasStateJson); }
std::string AnimationGraphRenderer::GetCanvasStateJSON() const { return m_canvasStateJson; }

void AnimationGraphRenderer::SetCanvasStateJSON(const std::string& json)
{
    if (json.empty()) return;
    nlohmann::json state;
    try { std::istringstream iss(json); iss >> state; } catch (...) { return; }
    if (!state.is_object()) return;
    m_canvasStateJson = json;
    m_minimapVisible = state.value("minimapVisible", m_minimapVisible);
    m_minimapSize = state.value("minimapSize", m_minimapSize);
    m_minimapPosition = state.value("minimapPosition", m_minimapPosition);
    m_selectedStateIndex = state.value("selectedStateIndex", m_selectedStateIndex);
    m_selectedTransitionIndex = state.value("selectedTransitionIndex", m_selectedTransitionIndex);
}

void AnimationGraphRenderer::RenderToolbar()
{
    if (ImGui::Button("Save")) Save("");
    ImGui::SameLine();
    if (ImGui::Button("Save As"))
        CanvasModalRenderer::Get().OpenSaveFilePickerModal("./Gamedata/Animation/", m_document ? m_document->GetName() : "Untitled", Olympe::SaveFileType::AnimationGraph);
    ImGui::SameLine();
    ImGui::Button("Verify");
    ImGui::SameLine();
    ImGui::Button("Run");
}

void AnimationGraphRenderer::RenderMainPanel()
{
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    if (totalWidth <= 0.0f) totalWidth = 800.0f;
    if (totalHeight <= 0.0f) totalHeight = 600.0f;

    float topRowHeight = totalHeight * 0.70f;
    float bottomRowHeight = totalHeight - topRowHeight;
    float leftWidth = totalWidth * 0.70f;
    float rightWidth = totalWidth - leftWidth;

    ImGui::BeginChild("AnimationGraph_MainRow", ImVec2(0, topRowHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::BeginChild("AnimationGraph_CanvasZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderCanvasPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("AnimationGraph_SequenceurZone", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderEventSequencerPanel();
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::Spacing();

    ImGui::BeginChild("AnimationGraph_BottomRow", ImVec2(0, bottomRowHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::BeginChild("AnimationGraph_RendererZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderAnimationPreviewPanel();
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("AnimationGraph_RightPanel", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderRightPanelTabs();
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void AnimationGraphRenderer::RenderRightPanelTabs()
{
    const nlohmann::json& data = m_document->GetData();
    if (ImGui::BeginTabBar("AnimationGraphRightTabs"))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            ImGui::Text("Path: %s", m_currentPath.empty() ? "(unsaved)" : m_currentPath.c_str());
            ImGui::Text("Default state: %s", m_document->GetDefaultState().c_str());
            ImGui::Separator();
            ImGui::Text("TSX Sources: %d", data.contains("sources") ? (int)data["sources"].size() : 0);
            ImGui::Text("States: %d", data.contains("states") ? (int)data["states"].size() : 0);
            ImGui::Text("Transitions: %d", data.contains("transitions") ? (int)data["transitions"].size() : 0);
            ImGui::Text("Events: %d", data.contains("states") ? (int)data["states"].size() : 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Nodes"))
        {
            ImGui::TextDisabled("Animation state node palette");
            ImGui::Separator();
            ImGui::TextWrapped("This zone will host the draggable node palette for animation states.");
            ImGui::TextWrapped("Phase 1 will wire TSX loading and CRUD into Properties.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
void AnimationGraphRenderer::RenderCanvasPanel()
{
    ImGui::Text("Animation Graph Canvas");
    ImGui::Separator();
    ImGui::TextDisabled("Pan / zoom / selection / drag-drop will be wired here.");
    RenderGraphCanvas();
}
void AnimationGraphRenderer::RenderEventSequencerPanel()
{
    ImGui::Text("Event Sequencer");
    ImGui::Separator();
    ImGui::TextDisabled("State events and sequencing controls will live here.");
}
void AnimationGraphRenderer::RenderAnimationPreviewPanel()
{
    ImGui::Text("Animation Renderer");
    ImGui::Separator();
    ImGui::TextDisabled("Runtime preview and resolved clip playback will live here.");
}
void AnimationGraphRenderer::RenderTransitionEditorPanel() {}
void AnimationGraphRenderer::RenderGraphCanvas()
{
    ImGui::TextDisabled("Graph canvas placeholder");
}
void AnimationGraphRenderer::Render() { RenderToolbar(); RenderMainPanel(); }
bool AnimationGraphRenderer::IsDirty() const { return m_document && m_document->IsDirty(); }
std::string AnimationGraphRenderer::GetGraphType() const { return "AnimationGraph"; }
std::string AnimationGraphRenderer::GetCurrentPath() const { return m_currentPath; }
void AnimationGraphRenderer::SetMinimapSize(float size) { m_minimapSize = size; }
int AnimationGraphRenderer::GetMinimapPosition() const { return m_minimapPosition; }
void AnimationGraphRenderer::SetMinimapPosition(int pos) { m_minimapPosition = pos; }
void AnimationGraphRenderer::VerifyGraph() {}
void AnimationGraphRenderer::RunGraph() {}
void AnimationGraphRenderer::RenderFrameworkModals() { if (m_framework) m_framework->RenderModals(); }

} // namespace Olympe
