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
    , m_selectedSourceIndex(-1)
    , m_showTsxImportModal(false)
{
    m_stateNameBuffer[0] = 0;
    m_animationNameBuffer[0] = 0;
    m_transitionFromBuffer[0] = 0;
    m_transitionToBuffer[0] = 0;
    m_tsxImportPathBuffer[0] = 0;
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
    if (ImGui::Button("Verify"))
    {
        VerifyGraph();
    }
    ImGui::SameLine();
    if (ImGui::Button("Run"))
    {
        RunGraph();
    }
}

void AnimationGraphRenderer::VerifyGraph()
{
    if (!m_document)
    {
        return;
    }
    std::vector<std::string> issues = m_document->ValidateBinding();
    SYSTEM_LOG << "[AnimationGraphRenderer] VerifyGraph: " << issues.size() << " issue(s)\n";
    for (size_t i = 0; i < issues.size(); ++i)
    {
        SYSTEM_LOG << "[AnimationGraphRenderer] - " << issues[i] << "\n";
    }
}

void AnimationGraphRenderer::RunGraph()
{
    SYSTEM_LOG << "[AnimationGraphRenderer] RunGraph requested\n";
}

void AnimationGraphRenderer::RenderMainPanel()
{
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    if (totalWidth <= 0.0f) totalWidth = 800.0f;
    if (totalHeight <= 0.0f) totalHeight = 600.0f;

    static float s_splitX = 0.72f;
    static float s_splitY = 0.70f;
    static float s_splitMin = 0.20f;
    static float s_splitMax = 0.80f;

    float leftWidth = totalWidth * s_splitX;
    float rightWidth = totalWidth - leftWidth;
    float topHeight = totalHeight * s_splitY;
    float bottomHeight = totalHeight - topHeight;
    const float splitterThickness = 6.0f;

    ImGui::BeginChild("AnimationGraph_GridRoot", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::BeginChild("AnimationGraph_TopRow", ImVec2(0, topHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImGui::BeginChild("AnimationGraph_CanvasZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            RenderCanvasPanel();
            ImGui::EndChild();

            ImGui::SameLine(0.0f, splitterThickness);
            ImGui::InvisibleButton("##AnimationGraphHorizontalSplitter", ImVec2(splitterThickness, topHeight));
            if (ImGui::IsItemActive())
            {
                s_splitX += ImGui::GetIO().MouseDelta.x / totalWidth;
                if (s_splitX < s_splitMin) s_splitX = s_splitMin;
                if (s_splitX > s_splitMax) s_splitX = s_splitMax;
            }

            ImGui::SameLine(0.0f, splitterThickness);
            ImGui::BeginChild("AnimationGraph_RightTopZone", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            RenderRightPanelTabs();
            ImGui::EndChild();
        }
        ImGui::EndChild();

        ImGui::InvisibleButton("##AnimationGraphVerticalSplitter", ImVec2(totalWidth, splitterThickness));
        if (ImGui::IsItemActive())
        {
            s_splitY += ImGui::GetIO().MouseDelta.y / totalHeight;
            if (s_splitY < s_splitMin) s_splitY = s_splitMin;
            if (s_splitY > s_splitMax) s_splitY = s_splitMax;
        }

        ImGui::BeginChild("AnimationGraph_BottomRow", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImGui::BeginChild("AnimationGraph_SequenceurZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            RenderEventSequencerPanel();
            ImGui::EndChild();

            ImGui::SameLine(0.0f, splitterThickness);
            ImGui::InvisibleButton("##AnimationGraphBottomHorizontalSplitter", ImVec2(splitterThickness, bottomHeight));
            if (ImGui::IsItemActive())
            {
                s_splitX += ImGui::GetIO().MouseDelta.x / totalWidth;
                if (s_splitX < s_splitMin) s_splitX = s_splitMin;
                if (s_splitX > s_splitMax) s_splitX = s_splitMax;
            }

            ImGui::SameLine(0.0f, splitterThickness);
            ImGui::BeginChild("AnimationGraph_RendererZone", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            RenderAnimationPreviewPanel();
            ImGui::EndChild();
        }
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
            RenderPropertiesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Nodes"))
        {
            RenderNodesTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}
void AnimationGraphRenderer::RenderPropertiesTab()
{
    const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
    int deleteSourceIndex = -1;
    ImGui::Text("Path: %s", m_currentPath.empty() ? "(unsaved)" : m_currentPath.c_str());
    ImGui::Text("Default state: %s", m_document->GetDefaultState().c_str());
    ImGui::Separator();
    ImGui::Text("TSX Sources: %d", (int)sources.size());

    if (ImGui::Button("Load TSX"))
    {
        CanvasModalRenderer::Get().OpenAnimationGraphTsxPickerModal("./Gamedata/Animation");
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload All TSX"))
    {
        std::vector<std::string> sourcePaths;
        for (size_t i = 0; i < sources.size(); ++i)
        {
            sourcePaths.push_back(sources[i].filePath);
        }
        if (m_document)
        {
            m_document->Clear();
            for (size_t i = 0; i < sourcePaths.size(); ++i)
            {
                m_document->ImportTSXSource(sourcePaths[i], 0);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Imported TSX");
    for (size_t i = 0; i < sources.size(); ++i)
    {
        const AnimationGraphDocument::SourceDefinition& source = sources[i];
        bool selected = (m_selectedSourceIndex == static_cast<int>(i));
        if (ImGui::Selectable(source.sourceName.c_str(), selected))
        {
            m_selectedSourceIndex = static_cast<int>(i);
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Entry"))
            {
                deleteSourceIndex = static_cast<int>(i);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::TextDisabled("%s", source.filePath.c_str());
        ImGui::TextDisabled("Clips: %d", (int)source.clips.size());
    }

    CanvasModalRenderer::Get().RenderAnimationGraphTsxPickerModal();
    if (CanvasModalRenderer::Get().IsAnimationGraphTsxModalConfirmed())
    {
        std::vector<std::string> selectedFiles = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFiles();
        if (selectedFiles.empty())
        {
            std::string selected = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFile();
            if (!selected.empty())
                selectedFiles.push_back(selected);
        }

        if (m_document)
        {
            std::vector<std::string> errors;
            for (size_t i = 0; i < selectedFiles.size(); ++i)
            {
                m_document->ImportTSXSource(selectedFiles[i], &errors);
            }
        }
        CanvasModalRenderer::Get().CloseAnimationGraphTsxModal();
    }

    if (deleteSourceIndex >= 0)
    {
        m_document->RemoveSource(static_cast<size_t>(deleteSourceIndex));
        if (m_selectedSourceIndex == deleteSourceIndex)
        {
            m_selectedSourceIndex = -1;
        }
        else if (m_selectedSourceIndex > deleteSourceIndex)
        {
            --m_selectedSourceIndex;
        }
    }
}
void AnimationGraphRenderer::RenderNodesTab()
{
    ImGui::TextDisabled("Animation state node palette");
    ImGui::Separator();
    ImGui::TextWrapped("This zone will host the draggable node palette for animation states.");
    ImGui::TextWrapped("Each imported TSX creates one state node entry.");
    ImGui::Separator();

    if (!m_document)
    {
        ImGui::TextDisabled("(no document)");
        return;
    }

    const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
    for (size_t i = 0; i < sources.size(); ++i)
    {
        const AnimationGraphDocument::SourceDefinition& source = sources[i];
        ImGui::PushID(static_cast<int>(i));
        if (ImGui::CollapsingHeader(source.sourceName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TextDisabled("%s", source.filePath.c_str());
            ImGui::Text("State node: %s", source.sourceName.c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                const char* sourcePayload = source.sourceName.c_str();
                ImGui::SetDragDropPayload("ANIMATION_GRAPH_STATE", sourcePayload, static_cast<int>(source.sourceName.size() + 1));
                ImGui::Text("%s", source.sourceName.c_str());
                ImGui::EndDragDropSource();
            }
        }
        ImGui::PopID();
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
    ImGui::TextDisabled("This panel sits below the canvas in the mockup.");
}
void AnimationGraphRenderer::RenderAnimationPreviewPanel()
{
    ImGui::Text("Animation Renderer");
    ImGui::Separator();
    ImGui::TextDisabled("Runtime preview and resolved clip playback will live here.");
    ImGui::TextDisabled("This panel sits below the properties area in the mockup.");
}
void AnimationGraphRenderer::RenderTransitionEditorPanel() {}
void AnimationGraphRenderer::RenderGraphCanvas()
{
    if (!m_document)
    {
        ImGui::TextDisabled("No Animation Graph document");
        return;
    }

    ImGui::TextDisabled("Drop a TSX clip here to create a state");
    ImGui::Separator();
    ImGui::Text("Known clips: %d", (int)m_document->GetAllKnownClips().size());
    ImGui::Text("Imported TSX: %d", (int)m_document->GetSources().size());
    ImGui::TextDisabled("Canvas rendering will list states once the document renderer is wired.");
}
void AnimationGraphRenderer::Render()
{
    RenderToolbar();
    RenderMainPanel();
    if (m_framework)
    {
        m_framework->RenderModals();
    }
}
bool AnimationGraphRenderer::IsDirty() const { return m_document && m_document->IsDirty(); }
std::string AnimationGraphRenderer::GetGraphType() const { return "AnimationGraph"; }
std::string AnimationGraphRenderer::GetCurrentPath() const { return m_currentPath; }
void AnimationGraphRenderer::SetMinimapSize(float size) { m_minimapSize = size; }
int AnimationGraphRenderer::GetMinimapPosition() const { return m_minimapPosition; }
void AnimationGraphRenderer::SetMinimapPosition(int pos) { m_minimapPosition = pos; }
void AnimationGraphRenderer::RenderFrameworkModals() { if (m_framework) m_framework->RenderModals(); }

} // namespace Olympe
