#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Framework/AnimationGraphFrameworkDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/CanvasToolbarRenderer.h"
#include "Framework/CanvasModalRenderer.h"
#include "Commands/CallbackGraphCommand.h"
#include "Utilities/CanvasGridRenderer.h"
#include "Utilities/CanvasMinimapRenderer.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <cstring>
#include <sstream>
#include <tuple>

namespace Olympe {

AnimationGraphRenderer::AnimationGraphRenderer()
    : m_canvasStateLoaded(false)
    , m_transitionTimeBuffer(0.1f)
    , m_eventTimeBuffer(0.5f)
    , m_minimapVisible(true)
    , m_minimapSize(0.15f)
    , m_minimapPosition(1)
    , m_gridVisible(true)
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
void AnimationGraphRenderer::SaveCanvasState()
{
    nlohmann::json state;
    state["gridVisible"] = m_gridVisible;
    state["minimapVisible"] = m_minimapVisible;
    state["minimapSize"] = m_minimapSize;
    state["minimapPosition"] = m_minimapPosition;
    if (m_canvasEditor)
    {
        const ImVec2 pan = m_canvasEditor->GetPan();
        state["panX"] = pan.x;
        state["panY"] = pan.y;
        state["zoom"] = m_canvasEditor->GetZoom();
    }
    m_canvasStateJson = state.dump();
}

void AnimationGraphRenderer::RestoreCanvasState()
{
    if (m_canvasStateJson.empty())
        return;

    try
    {
        const nlohmann::json state = nlohmann::json::parse(m_canvasStateJson);
        m_gridVisible = state.value("gridVisible", m_gridVisible);
        m_minimapVisible = state.value("minimapVisible", m_minimapVisible);
        m_minimapSize = state.value("minimapSize", m_minimapSize);
        m_minimapPosition = state.value("minimapPosition", m_minimapPosition);
        if (m_canvasEditor)
        {
            m_canvasEditor->SetPan(ImVec2(
                state.value("panX", 0.0f), state.value("panY", 0.0f)));
            m_canvasEditor->SetZoom(state.value("zoom", 1.0f));
        }
    }
    catch (...)
    {
        // A malformed persisted view must not prevent the graph itself from opening.
    }
}
std::string AnimationGraphRenderer::GetCanvasStateJSON() const { return m_canvasStateJson; }
void AnimationGraphRenderer::SetCanvasStateJSON(const std::string& json)
{
    m_canvasStateJson = json;
    m_canvasStateLoaded = false;
}
void AnimationGraphRenderer::SetMinimapSize(float size) { m_minimapSize = size; }
int AnimationGraphRenderer::GetMinimapPosition() const { return m_minimapPosition; }
void AnimationGraphRenderer::SetMinimapPosition(int pos) { m_minimapPosition = pos; }
void AnimationGraphRenderer::SetGridVisible(bool visible) { m_gridVisible = visible; }
void AnimationGraphRenderer::ResetView()
{
    if (m_canvasEditor)
        m_canvasEditor->ResetView();
}
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
    if (m_framework)
    {
        ImGui::SameLine(0.0f, 12.0f);
        if (ImGui::Button("Undo##animation_history") && m_framework->CanUndo())
            m_framework->Undo();
        if (ImGui::IsItemHovered() && m_framework->CanUndo())
            ImGui::SetTooltip("Undo: %s", m_framework->GetUndoDescription().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Redo##animation_history") && m_framework->CanRedo())
            m_framework->Redo();
        if (ImGui::IsItemHovered() && m_framework->CanRedo())
            ImGui::SetTooltip("Redo: %s", m_framework->GetRedoDescription().c_str());
    }
}

bool AnimationGraphRenderer::ExecuteDocumentCommand(
    const std::string& description, const std::function<bool()>& edit)
{
    if (!m_document || !m_framework || !edit)
        return false;

    const nlohmann::json before = m_document->CreateSnapshot();
    return m_framework->ExecuteCommand(std::unique_ptr<GraphCommand>(
        new CallbackGraphCommand(description,
            [edit]() -> bool { return edit(); },
            [document = m_document.get(), before]() -> bool {
                return document && document->RestoreSnapshot(before);
            })));
}

void AnimationGraphRenderer::HandleKeyboardShortcuts()
{
    if (!m_framework || ImGui::GetIO().WantTextInput)
        return;

    const bool ctrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    if (!ctrlDown)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Z))
    {
        if (ImGui::GetIO().KeyShift)
            m_framework->Redo();
        else
            m_framework->Undo();
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Y))
    {
        m_framework->Redo();
    }
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
        for (size_t clipIndex = 0; clipIndex < sources[i].clips.size(); ++clipIndex)
        {
            const AnimationGraphDocument::SourceDefinition::ClipDefinition& clip = sources[i].clips[clipIndex];
            ImGui::PushID(static_cast<int>(clipIndex));
            ImGui::BulletText("%s%s", clip.id.c_str(), clip.group.empty() ? "" : ("  [" + clip.group + "]").c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("ANIMATION_GRAPH_STATE", clip.id.c_str(), static_cast<int>(clip.id.size() + 1));
                ImGui::Text("%s", clip.id.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();
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
                    ExecuteDocumentCommand("Create Animation Transition",
                        [document = m_document.get(), fromState, this]() -> bool {
                            return document->AddTransition(fromState, fromState, "true", m_transitionTimeBuffer);
                        });
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
                const std::string condition = data["transitions"][i].value("condition", "true");
                strncpy_s(m_eventNameBuffer, sizeof(m_eventNameBuffer), condition.c_str(), _TRUNCATE);
                m_transitionTimeBuffer = static_cast<float>(data["transitions"][i].value("duration", 0.1));
            }
        }

        if (m_selectedTransitionIndex >= 0 && static_cast<size_t>(m_selectedTransitionIndex) < data["transitions"].size())
        {
            const nlohmann::json& selected = data["transitions"][m_selectedTransitionIndex];
            const std::string fromState = selected.value("from", "");
            const std::string toState = selected.value("to", "");
            const int priority = selected.value("priority", 0);
            const bool interruptible = selected.value("interruptible", true);
            ImGui::Separator();
            ImGui::Text("%s -> %s", fromState.c_str(), toState.c_str());
            ImGui::InputText("Condition", m_eventNameBuffer, sizeof(m_eventNameBuffer));
            ImGui::DragFloat("Blend (s)", &m_transitionTimeBuffer, 0.01f, 0.0f, 10.0f, "%.2f");
            if (ImGui::Button("Apply Transition"))
            {
                const int transitionIndex = m_selectedTransitionIndex;
                const std::string beforeCondition = selected.value("condition", "true");
                const float beforeDuration = static_cast<float>(selected.value("duration", 0.1));
                const std::string afterCondition(m_eventNameBuffer);
                const float afterDuration = m_transitionTimeBuffer;
                if (beforeCondition != afterCondition || beforeDuration != afterDuration)
                {
                    m_framework->ExecuteCommand(std::unique_ptr<GraphCommand>(
                        new CallbackGraphCommand("Edit Animation Transition",
                            [document = m_document.get(), transitionIndex, afterCondition, afterDuration, priority, interruptible]() -> bool {
                                return document->UpdateTransition(static_cast<size_t>(transitionIndex), afterCondition, afterDuration, priority, interruptible);
                            },
                            [document = m_document.get(), transitionIndex, beforeCondition, beforeDuration, priority, interruptible]() -> bool {
                                return document->UpdateTransition(static_cast<size_t>(transitionIndex), beforeCondition, beforeDuration, priority, interruptible);
                            })));
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Transition"))
            {
                const int transitionIndex = m_selectedTransitionIndex;
                if (ExecuteDocumentCommand("Delete Animation Transition",
                    [document = m_document.get(), transitionIndex]() -> bool {
                        return document->RemoveTransition(static_cast<size_t>(transitionIndex));
                    }))
                {
                    m_selectedTransitionIndex = -1;
                }
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
    HandleKeyboardShortcuts();
    RenderToolbar();
    ImGui::Separator();
    RenderMainPanel();
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
            const nlohmann::json& graphData = m_document->GetData();
            if (m_selectedStateIndex >= 0 && graphData.contains("states") && graphData["states"].is_array() &&
                static_cast<size_t>(m_selectedStateIndex) < graphData["states"].size())
            {
                const nlohmann::json& selectedState = graphData["states"][m_selectedStateIndex];
                const std::string currentName = selectedState.value("name", "");
                ImGui::Separator();
                ImGui::Text("Selected state");
                ImGui::InputText("Name##animation_state", m_stateNameBuffer, sizeof(m_stateNameBuffer));
                if (ImGui::IsItemDeactivatedAfterEdit() && currentName != m_stateNameBuffer && m_stateNameBuffer[0] != '\0')
                {
                    const std::string renamedState(m_stateNameBuffer);
                    ExecuteDocumentCommand("Rename Animation State",
                        [document = m_document.get(), currentName, renamedState]() -> bool {
                            return document->RenameState(currentName, renamedState);
                        });
                }

                const std::vector<std::string> clips = m_document->GetAllKnownClips();
                const std::string currentClip = selectedState.value("defaultClip", "");
                int selectedClip = -1;
                for (size_t clipIndex = 0; clipIndex < clips.size(); ++clipIndex)
                    if (clips[clipIndex] == currentClip) { selectedClip = static_cast<int>(clipIndex); break; }

                const char* preview = selectedClip >= 0 ? clips[static_cast<size_t>(selectedClip)].c_str() : "(no clip)";
                if (ImGui::BeginCombo("Default clip##animation_state", preview))
                {
                    for (size_t clipIndex = 0; clipIndex < clips.size(); ++clipIndex)
                    {
                        const bool isCurrent = static_cast<int>(clipIndex) == selectedClip;
                        if (ImGui::Selectable(clips[clipIndex].c_str(), isCurrent) && !isCurrent)
                        {
                            const std::string nextClip = clips[clipIndex];
                            ExecuteDocumentCommand("Set Animation State Clip",
                                [document = m_document.get(), currentName, nextClip]() -> bool {
                                    return document->SetStateDefaultClip(currentName, nextClip);
                                });
                        }
                        if (isCurrent) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Button("Make Default State##animation_state") && currentName != m_document->GetDefaultState())
                {
                    const std::string previousDefault = m_document->GetDefaultState();
                    ExecuteDocumentCommand("Set Animation Default State",
                        [document = m_document.get(), currentName]() -> bool {
                            document->SetDefaultState(currentName);
                            return true;
                        });
                    (void)previousDefault;
                }
            }
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
                    for (size_t clipIndex = 0; clipIndex < source.clips.size(); ++clipIndex)
                    {
                        const AnimationGraphDocument::SourceDefinition::ClipDefinition& clip = source.clips[clipIndex];
                        ImGui::PushID(static_cast<int>(clipIndex));
                        ImGui::Selectable(clip.id.c_str(), false);
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                        {
                            ImGui::SetDragDropPayload("ANIMATION_GRAPH_STATE", clip.id.c_str(), static_cast<int>(clip.id.size() + 1));
                            ImGui::Text("%s", clip.id.c_str());
                            ImGui::EndDragDropSource();
                        }
                        ImGui::PopID();
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

    if (!m_canvasEditor)
    {
        m_canvasEditor = std::make_unique<CustomCanvasEditor>(
            "AnimationGraphCanvas", canvasPos, canvasSize);
    }
    else
    {
        m_canvasEditor->SetCanvasScreenPos(canvasPos);
        m_canvasEditor->SetCanvasSize(canvasSize);
    }

    m_canvasEditor->SetGridVisible(m_gridVisible);
    m_canvasEditor->SetMinimapVisible(m_minimapVisible);
    m_canvasEditor->SetMinimapSize(m_minimapSize);
    m_canvasEditor->SetMinimapPosition(m_minimapPosition);
    if (!m_canvasStateLoaded)
    {
        RestoreCanvasState();
        m_canvasStateLoaded = true;
        m_canvasEditor->SetGridVisible(m_gridVisible);
        m_canvasEditor->SetMinimapVisible(m_minimapVisible);
        m_canvasEditor->SetMinimapSize(m_minimapSize);
        m_canvasEditor->SetMinimapPosition(m_minimapPosition);
    }
    m_canvasEditor->BeginRender();

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    CanvasGridRenderer::GridConfig backgroundConfig = m_canvasEditor->GetGridConfig();
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
        ImGui::ColorConvertFloat4ToU32(backgroundConfig.backgroundColor));
    m_canvasEditor->RenderGrid(CanvasGridRenderer::Style_VisualScript);

    ImGui::InvisibleButton("##AnimationGraphCanvasSurface", canvasSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_GRAPH_STATE"))
        {
            const char* sourceName = static_cast<const char*>(payload->Data);
            if (sourceName && *sourceName)
            {
                std::string stateName = m_document->MakeUniqueStateName(sourceName);
                const ImVec2 graphPos = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);
                ExecuteDocumentCommand("Create Animation State",
                    [document = m_document.get(), stateName, sourceName, graphPos]() -> bool {
                        return document->AddStateFromClip(stateName, sourceName) &&
                               document->SetStatePosition(stateName, graphPos.x, graphPos.y);
                    });
            }
        }
        ImGui::EndDragDropTarget();
    }

    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array() || data["states"].empty())
    {
        drawList->AddText(ImVec2(canvasPos.x + 16.0f, canvasPos.y + 16.0f), IM_COL32(160, 160, 160, 255), "(no states yet)");
        m_canvasEditor->RenderMinimap();
        m_canvasEditor->EndRender();
        return;
    }

    const float nodeWidth = 220.0f;
    const float nodeHeight = 72.0f;
    const float zoom = m_canvasEditor->GetZoom();
    const float pinRadius = 7.0f;
    std::vector<std::tuple<int, float, float, float, float>> minimapNodes;
    minimapNodes.reserve(data["states"].size());
    float graphMinX = 0.0f;
    float graphMaxX = 0.0f;
    float graphMinY = 0.0f;
    float graphMaxY = 0.0f;
    bool hasGraphBounds = false;

    // Connections are deliberately rendered before state cards, so cards remain the
    // primary hit targets while their transition flow stays readable at every zoom.
    const auto findTransitionAnchor = [&](const std::string& stateName, bool output, ImVec2& anchor) -> bool
    {
        for (size_t stateIndex = 0; stateIndex < data["states"].size(); ++stateIndex)
        {
            const nlohmann::json& candidate = data["states"][stateIndex];
            if (candidate.value("name", "") != stateName)
                continue;
            const ImVec2 nodeTopLeft = m_canvasEditor->CanvasToScreen(ImVec2(
                static_cast<float>(candidate.value("x", 120.0)),
                static_cast<float>(candidate.value("y", 120.0))));
            anchor = ImVec2(
                nodeTopLeft.x + (output ? nodeWidth * zoom : 0.0f),
                nodeTopLeft.y + nodeHeight * zoom * 0.5f);
            return true;
        }
        return false;
    };

    if (data.contains("transitions") && data["transitions"].is_array())
    {
        for (size_t transitionIndex = 0; transitionIndex < data["transitions"].size(); ++transitionIndex)
        {
            const nlohmann::json& transition = data["transitions"][transitionIndex];
            ImVec2 from;
            ImVec2 to;
            if (!findTransitionAnchor(transition.value("from", ""), true, from) ||
                !findTransitionAnchor(transition.value("to", ""), false, to))
                continue;

            const float controlOffset = (to.x >= from.x ? 1.0f : -1.0f) * 80.0f * zoom;
            drawList->AddBezierCubic(from,
                ImVec2(from.x + controlOffset, from.y),
                ImVec2(to.x - controlOffset, to.y),
                to, IM_COL32(120, 185, 255, 220), 2.0f, 24);
        }
    }
    for (size_t i = 0; i < data["states"].size(); ++i)
    {
        const nlohmann::json& state = data["states"][i];
        const std::string stateName = state.value("name", "");
        const float x = static_cast<float>(state.value("x", 120.0));
        const float y = static_cast<float>(state.value("y", 120.0));
        const ImVec2 nodePos = m_canvasEditor->CanvasToScreen(ImVec2(x, y));
        const ImVec2 nodeEnd(nodePos.x + nodeWidth * zoom, nodePos.y + nodeHeight * zoom);
        minimapNodes.push_back(std::make_tuple(static_cast<int>(i), x, y, nodeWidth, nodeHeight));
        if (!hasGraphBounds)
        {
            graphMinX = x; graphMaxX = x + nodeWidth;
            graphMinY = y; graphMaxY = y + nodeHeight;
            hasGraphBounds = true;
        }
        else
        {
            graphMinX = (x < graphMinX) ? x : graphMinX;
            graphMaxX = (x + nodeWidth > graphMaxX) ? x + nodeWidth : graphMaxX;
            graphMinY = (y < graphMinY) ? y : graphMinY;
            graphMaxY = (y + nodeHeight > graphMaxY) ? y + nodeHeight : graphMaxY;
        }
        const bool isSelected = m_selectedStateIndex == static_cast<int>(i);
        drawList->AddRectFilled(nodePos, nodeEnd, IM_COL32(44, 50, 66, 255), 6.0f);
        drawList->AddRect(nodePos, nodeEnd,
            isSelected ? IM_COL32(0, 210, 255, 255) : IM_COL32(90, 120, 180, 255),
            6.0f, 0, isSelected ? 3.0f : 2.0f);
        drawList->AddText(ImVec2(nodePos.x + 10.0f * zoom, nodePos.y + 10.0f * zoom), IM_COL32(255, 255, 255, 255), stateName.c_str());
        drawList->AddText(ImVec2(nodePos.x + 10.0f * zoom, nodePos.y + 30.0f * zoom), IM_COL32(180, 180, 180, 255), state.value("defaultClip", "(no clip)").c_str());

        const ImVec2 inputPin(nodePos.x, nodePos.y + nodeHeight * zoom * 0.5f);
        const ImVec2 outputPin(nodeEnd.x, nodePos.y + nodeHeight * zoom * 0.5f);
        drawList->AddCircleFilled(inputPin, pinRadius, IM_COL32(45, 185, 255, 255));
        drawList->AddCircle(inputPin, pinRadius, IM_COL32(230, 245, 255, 255), 0, 1.5f);
        drawList->AddCircleFilled(outputPin, pinRadius, IM_COL32(45, 185, 255, 255));
        drawList->AddCircle(outputPin, pinRadius, IM_COL32(230, 245, 255, 255), 0, 1.5f);
        drawList->AddText(ImVec2(nodePos.x + 10.0f * zoom, nodeEnd.y - 20.0f * zoom), IM_COL32(125, 185, 220, 255), "In");
        drawList->AddText(ImVec2(nodeEnd.x - 30.0f * zoom, nodeEnd.y - 20.0f * zoom), IM_COL32(125, 185, 220, 255), "Out");

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool nodeHovered = mouse.x >= nodePos.x && mouse.x <= nodeEnd.x &&
                                 mouse.y >= nodePos.y && mouse.y <= nodeEnd.y;
        const float inputDx = mouse.x - inputPin.x;
        const float inputDy = mouse.y - inputPin.y;
        const float outputDx = mouse.x - outputPin.x;
        const float outputDy = mouse.y - outputPin.y;
        const bool inputHovered = inputDx * inputDx + inputDy * inputDy <= pinRadius * pinRadius;
        const bool outputHovered = outputDx * outputDx + outputDy * outputDy <= pinRadius * pinRadius;

        if (outputHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_linkStartStateIndex = static_cast<int>(i);
            m_linkDragStartPos = outputPin;
        }
        else if (inputHovered && m_linkStartStateIndex >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                 m_linkStartStateIndex != static_cast<int>(i) &&
                 static_cast<size_t>(m_linkStartStateIndex) < data["states"].size())
        {
            const std::string fromState = data["states"][m_linkStartStateIndex].value("name", "");
            const std::string toState = stateName;
            ExecuteDocumentCommand("Create Animation Transition",
                [document = m_document.get(), fromState, toState]() -> bool {
                    return document->AddTransition(fromState, toState, "true", 0.1);
                });
            m_linkStartStateIndex = -1;
        }
        else if (nodeHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_selectedStateIndex = static_cast<int>(i);
            strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), stateName.c_str(), _TRUNCATE);
            m_draggedStateIndex = static_cast<int>(i);
            m_draggedStateName = stateName;
            m_dragStartPos = ImVec2(x, y);
        }
        if (m_draggedStateIndex == static_cast<int>(i) && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            m_document->SetStatePosition(stateName, x + delta.x / zoom, y + delta.y / zoom);
            m_canvasEditor->UpdateAutoPanning(true);
        }
        if (nodeHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            m_selectedStateIndex = static_cast<int>(i);
            strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), stateName.c_str(), _TRUNCATE);
            ImGui::OpenPopup((stateName + "##anim_node_context").c_str());
        }
        if (ImGui::BeginPopup((stateName + "##anim_node_context").c_str()))
        {
            if (ImGui::MenuItem("Delete"))
            {
                ExecuteDocumentCommand("Delete Animation State",
                    [document = m_document.get(), stateName]() -> bool {
                        return document->RemoveState(stateName);
                    });
                ImGui::EndPopup();
                break;
            }
            ImGui::EndPopup();
        }
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_draggedStateIndex >= 0)
    {
        const std::string movedStateName = m_draggedStateName;
        const AnimationGraphDocument::StateDefinition* movedState = m_document->GetState(movedStateName);
        if (movedState && (movedState->x != m_dragStartPos.x || movedState->y != m_dragStartPos.y))
        {
            const ImVec2 before = m_dragStartPos;
            const ImVec2 after(static_cast<float>(movedState->x), static_cast<float>(movedState->y));
            m_framework->ExecuteCommand(std::unique_ptr<GraphCommand>(
                new CallbackGraphCommand("Move Animation State",
                    [document = m_document.get(), movedStateName, after]() -> bool {
                        return document->SetStatePosition(movedStateName, after.x, after.y);
                    },
                    [document = m_document.get(), movedStateName, before]() -> bool {
                        return document->SetStatePosition(movedStateName, before.x, before.y);
                    })));
        }
        m_draggedStateIndex = -1;
        m_draggedStateName.clear();
    }
    if (m_linkStartStateIndex >= 0)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            const ImVec2 previewEnd = ImGui::GetIO().MousePos;
            drawList->AddBezierCubic(m_linkDragStartPos,
                ImVec2(m_linkDragStartPos.x + 80.0f * zoom, m_linkDragStartPos.y),
                ImVec2(previewEnd.x - 80.0f * zoom, previewEnd.y), previewEnd,
                IM_COL32(110, 220, 255, 210), 2.0f, 24);
        }
        else
        {
            m_linkStartStateIndex = -1;
        }
    }

    if (hasGraphBounds)
    {
        // A margin keeps the first and last nodes readable in the minimap.
        const float margin = 80.0f;
        graphMinX -= margin; graphMaxX += margin;
        graphMinY -= margin; graphMaxY += margin;
        ImVec2 visibleMin;
        ImVec2 visibleMax;
        m_canvasEditor->GetCanvasVisibleBounds(visibleMin, visibleMax);
        m_canvasEditor->UpdateMinimapNodes(minimapNodes, graphMinX, graphMaxX, graphMinY, graphMaxY);
        m_canvasEditor->UpdateMinimapViewport(visibleMin.x, visibleMax.x, visibleMin.y, visibleMax.y,
            graphMinX, graphMaxX, graphMinY, graphMaxY);
    }
    m_canvasEditor->RenderMinimap();
    m_canvasEditor->EndRender();
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
