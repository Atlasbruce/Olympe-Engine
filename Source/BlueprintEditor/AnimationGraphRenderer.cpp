#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Framework/AnimationGraphFrameworkDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/CanvasToolbarRenderer.h"
#include "Framework/CanvasModalRenderer.h"
#include "Utilities/CanvasGridRenderer.h"
#include "Utilities/CanvasMinimapRenderer.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <sstream>

namespace Olympe {

AnimationGraphRenderer::AnimationGraphRenderer()
    : m_transitionTimeBuffer(0.1f)
    , m_eventTimeBuffer(0.5f)
    , m_minimapVisible(true)
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
    if (!m_frameworkDocument)
    {
        m_frameworkDocument = std::make_unique<AnimationGraphFrameworkDocument>(this);
    }
    if (!m_framework)
    {
        m_framework = std::make_unique<CanvasFramework>(m_frameworkDocument.get());
    }
}

bool AnimationGraphRenderer::Load(const std::string& path) { EnsureDocument(); if (!m_document->Load(path)) return false; m_currentPath = path; return true; }
bool AnimationGraphRenderer::Save(const std::string& path) { EnsureDocument(); const std::string target = path.empty() ? m_currentPath : path; if (!m_document->Save(target)) return false; m_currentPath = target; return true; }
void AnimationGraphRenderer::SetCurrentPath(const std::string& path) { m_currentPath = path; }
void AnimationGraphRenderer::SaveCanvasState() {}
void AnimationGraphRenderer::RestoreCanvasState() {}
std::string AnimationGraphRenderer::GetCanvasStateJSON() const { return m_canvasStateJson; }
void AnimationGraphRenderer::SetCanvasStateJSON(const std::string& json) { m_canvasStateJson = json; }
void AnimationGraphRenderer::SetMinimapSize(float size) { m_minimapSize = size; }
int AnimationGraphRenderer::GetMinimapPosition() const { return m_minimapPosition; }
void AnimationGraphRenderer::SetMinimapPosition(int pos) { m_minimapPosition = pos; }
void AnimationGraphRenderer::RenderFrameworkModals() { if (m_framework) m_framework->RenderModals(); }
bool AnimationGraphRenderer::IsDirty() const { return m_document && m_document->IsDirty(); }
std::string AnimationGraphRenderer::GetGraphType() const { return "AnimationGraph"; }
std::string AnimationGraphRenderer::GetCurrentPath() const { return m_currentPath; }

void AnimationGraphRenderer::VerifyGraph()
{
    if (!m_document) return;
    std::vector<std::string> issues = m_document->ValidateBinding();
    SYSTEM_LOG << "[AnimationGraphRenderer] VerifyGraph: " << issues.size() << " issue(s)\n";
}

void AnimationGraphRenderer::RunGraph()
{
    SYSTEM_LOG << "[AnimationGraphRenderer] RunGraph requested\n";
}

void AnimationGraphRenderer::RenderToolbar()
{
    if (m_framework && m_framework->GetToolbar())
    {
        m_framework->GetToolbar()->Render();
    }
    ImGui::SameLine();
    bool minimapVisible = m_minimapVisible;
    if (ImGui::Checkbox("Minimap", &minimapVisible))
    {
        m_minimapVisible = minimapVisible;
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    ImGui::SliderFloat("Size", &m_minimapSize, 0.05f, 0.5f, "%.2f");
    ImGui::SameLine();
    const char* positions[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
    ImGui::SetNextItemWidth(120.0f);
    ImGui::Combo("Position", &m_minimapPosition, positions, 4);
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
            RenderGraphCanvas();
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
            RenderTimelinePanel();
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
            ImGui::Text("Animation Renderer");
            ImGui::Separator();
            ImGui::TextDisabled("Runtime preview and resolved clip playback will live here.");
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void AnimationGraphRenderer::RenderStateEditorPanel()
{
    ImGui::Text("TSX Sources");
    ImGui::Separator();
    if (ImGui::Button("Import TSX..."))
    {
        CanvasModalRenderer::Get().OpenAnimationGraphTsxPickerModal("./Gamedata/Animation/AnimationBanks");
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload All TSX"))
    {
        std::vector<std::string> sourcePaths;
        const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
        for (size_t i = 0; i < sources.size(); ++i)
        {
            sourcePaths.push_back(sources[i].filePath);
        }
        m_document->Clear();
        for (size_t i = 0; i < sourcePaths.size(); ++i)
        {
            m_document->ImportTSXSource(sourcePaths[i], 0);
        }
    }

    CanvasModalRenderer::Get().RenderAnimationGraphTsxPickerModal();
    if (CanvasModalRenderer::Get().IsAnimationGraphTsxModalConfirmed())
    {
        std::vector<std::string> selectedFiles = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFiles();
        if (selectedFiles.empty())
        {
            std::string selected = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFile();
            if (!selected.empty()) selectedFiles.push_back(selected);
        }
        std::vector<std::string> errors;
        for (size_t i = 0; i < selectedFiles.size(); ++i)
        {
            m_document->ImportTSXSource(selectedFiles[i], &errors);
        }
        CanvasModalRenderer::Get().CloseAnimationGraphTsxModal();
    }

    const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
    ImGui::Text("Imported TSX: %d", (int)sources.size());
    for (size_t i = 0; i < sources.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::Selectable(sources[i].sourceName.c_str());
        ImGui::TextDisabled("%s", sources[i].filePath.c_str());
        ImGui::TextDisabled("Clips: %d", (int)sources[i].clips.size());
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            const char* sourcePayload = sources[i].sourceName.c_str();
            ImGui::SetDragDropPayload("ANIMATION_GRAPH_STATE", sourcePayload, static_cast<int>(sources[i].sourceName.size() + 1));
            ImGui::Text("%s", sources[i].sourceName.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
    ImGui::Separator();
    ImGui::Text("States");
    const nlohmann::json& data = m_document->GetData();
    if (data.contains("states") && data["states"].is_array())
    {
        for (size_t i = 0; i < data["states"].size(); ++i)
            ImGui::BulletText("%s", data["states"][i].value("name", "").c_str());
    }
}

void AnimationGraphRenderer::RenderTransitionEditorPanel()
{
    ImGui::Text("Transitions");
    ImGui::Separator();
    if (ImGui::Button("Add Transition"))
    {
        if (m_selectedStateIndex >= 0 && m_document->GetData().contains("states") && m_document->GetData()["states"].is_array())
        {
            const auto& states = m_document->GetData()["states"];
            if (static_cast<size_t>(m_selectedStateIndex) < states.size())
            {
                std::string fromState = states[m_selectedStateIndex].value("name", "");
                if (!fromState.empty())
                {
                    m_document->AddTransition(fromState, fromState, "true", m_transitionTimeBuffer);
                }
            }
        }
    }
    const nlohmann::json& data = m_document->GetData();
    if (data.contains("transitions") && data["transitions"].is_array())
    {
        for (size_t i = 0; i < data["transitions"].size(); ++i)
        {
            const std::string label = std::string(data["transitions"][i].value("from", "")) + " -> " + data["transitions"][i].value("to", "");
            if (ImGui::Selectable(label.c_str(), m_selectedTransitionIndex == static_cast<int>(i)))
            {
                m_selectedTransitionIndex = static_cast<int>(i);
            }
        }
    }
}

void AnimationGraphRenderer::RenderTimelinePanel()
{
    ImGui::Text("Animation Event");
    ImGui::Separator();
    ImGui::TextDisabled("Events and timeline editor will live here.");
}

void AnimationGraphRenderer::RenderRightPanel()
{
    RenderRightPanelTabs();
}

void AnimationGraphRenderer::Render()
{
    RenderToolbar();
    ImGui::Separator();
    RenderMainPanel();
    RenderRightPanel();
    if (m_framework) m_framework->RenderModals();
}

void AnimationGraphRenderer::RenderRightPanelTabs()
{
    if (ImGui::BeginTabBar("AnimationGraphRightTabs"))
    {
        if (ImGui::BeginTabItem("Properties"))
        {
            ImGui::Text("Animation Graph");
            ImGui::Separator();
            ImGui::Text("Path: %s", m_currentPath.empty() ? "(unsaved)" : m_currentPath.c_str());
            ImGui::Text("Default state: %s", m_document->GetDefaultState().c_str());
            ImGui::Text("Sources: %d", (int)m_document->GetSources().size());
            ImGui::Text("States: %d", m_document->GetData().contains("states") ? (int)m_document->GetData()["states"].size() : 0);
            ImGui::Text("Transitions: %d", m_document->GetData().contains("transitions") ? (int)m_document->GetData()["transitions"].size() : 0);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("States"))
        {
            RenderStateEditorPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Transitions"))
        {
            RenderTransitionEditorPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Events"))
        {
            RenderTimelinePanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Nodes"))
        {
            ImGui::Text("Animation state node palette");
            ImGui::Separator();
            ImGui::TextWrapped("Imported TSX entries appear here as draggable nodes.");
            ImGui::TextWrapped("Drag a TSX entry onto the canvas to instantiate a state node.");
            ImGui::Separator();

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
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void AnimationGraphRenderer::RenderGraphCanvas()
{
    if (!m_document)
    {
        ImGui::TextDisabled("No Animation Graph document");
        return;
    }
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(28, 28, 34, 255));

    CanvasGridRenderer::GridConfig gridConfig;
    gridConfig.canvasPos = canvasPos;
    gridConfig.canvasSize = canvasSize;
    gridConfig.zoom = 1.0f;
    gridConfig.offsetX = 0.0f;
    gridConfig.offsetY = 0.0f;
    gridConfig.majorSpacing = 24.0f;
    CanvasGridRenderer::RenderGrid(drawList, gridConfig);

    ImGui::InvisibleButton("##AnimationGraphCanvasSurface", canvasSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_GRAPH_STATE"))
        {
            const char* sourceName = static_cast<const char*>(payload->Data);
            if (sourceName && *sourceName)
            {
                std::string stateName = m_document->MakeUniqueStateName(sourceName);
                m_document->AddStateFromClip(stateName, sourceName);
            }
        }
        ImGui::EndDragDropTarget();
    }

    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array() || data["states"].empty())
    {
        drawList->AddText(ImVec2(canvasPos.x + 16.0f, canvasPos.y + 16.0f), IM_COL32(160, 160, 160, 255), "(no states yet)");
        return;
    }

    const float nodeWidth = 220.0f;
    const float nodeHeight = 72.0f;
    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        const std::string stateName = state.value("name", "");
        const float x = static_cast<float>(state.value("x", 120.0));
        const float y = static_cast<float>(state.value("y", 120.0));
        ImVec2 nodePos(canvasPos.x + x, canvasPos.y + y);
        ImVec2 nodeEnd(nodePos.x + nodeWidth, nodePos.y + nodeHeight);
        drawList->AddRectFilled(nodePos, nodeEnd, IM_COL32(44, 50, 66, 255), 6.0f);
        drawList->AddRect(nodePos, nodeEnd, IM_COL32(90, 120, 180, 255), 6.0f, 0, 2.0f);
        drawList->AddText(ImVec2(nodePos.x + 10.0f, nodePos.y + 10.0f), IM_COL32(255, 255, 255, 255), stateName.c_str());
        drawList->AddText(ImVec2(nodePos.x + 10.0f, nodePos.y + 30.0f), IM_COL32(180, 180, 180, 255), state.value("defaultClip", "(no clip)").c_str());

        ImGui::SetCursorScreenPos(nodePos);
        ImGui::InvisibleButton((stateName + "##anim_node").c_str(), ImVec2(nodeWidth, nodeHeight));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_selectedStateIndex = static_cast<int>(i);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            m_document->SetStatePosition(stateName, x + delta.x, y + delta.y);
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete"))
            {
                m_document->RemoveState(stateName);
                ImGui::EndPopup();
                break;
            }
            ImGui::EndPopup();
        }
    }
}

void AnimationGraphRenderer::RenderInspectorPanel() {}
void AnimationGraphRenderer::RenderVerificationPanel() {}
void AnimationGraphRenderer::RenderTransitionPreview() {}
void AnimationGraphRenderer::RenderSingleTransition(size_t, const ImVec2&) {}
void AnimationGraphRenderer::EnsureStatePositions() {}
void AnimationGraphRenderer::RenderGraphNodePorts(const nlohmann::json&, int, bool) {}
int AnimationGraphRenderer::FindStateIndexByName(const std::string& name) const { return m_document ? m_document->FindStateIndex(name) : -1; }
int AnimationGraphRenderer::FindEventIndexByState(const std::string&) const { return -1; }
std::string AnimationGraphRenderer::MakeUniqueStateName(const std::string& baseName) const { return m_document ? m_document->MakeUniqueStateName(baseName) : baseName; }

} // namespace Olympe
