#include "AnimationGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/CanvasToolbarRenderer.h"
#include "Framework/CanvasModalRenderer.h"
#include "Commands/CallbackGraphCommand.h"
#include "Utilities/CanvasGridRenderer.h"
#include "Utilities/CanvasMinimapRenderer.h"
#include "../DataManager.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace Olympe {

namespace {
class AnimationGraphClipboardPayload final : public IGraphClipboardPayload
{
public:
    AnimationGraphClipboardPayload(nlohmann::json copiedStates,
                                   nlohmann::json copiedTransitions,
                                   float sourceMinX, float sourceMinY)
        : states(std::move(copiedStates))
        , transitions(std::move(copiedTransitions))
        , minX(sourceMinX)
        , minY(sourceMinY)
    {
    }

    const char* GetGraphTypeId() const override { return "AnimationGraph"; }

    nlohmann::json states;
    nlohmann::json transitions;
    float minX;
    float minY;
};

std::string MakeStateNameFromBank(const std::string& sourceName)
{
    std::string name = sourceName;
    const size_t separator = name.find_last_of('_');
    if (separator != std::string::npos && separator + 1 < name.size())
        name = name.substr(separator + 1);
    if (!name.empty())
        name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    return name.empty() ? "State" : name;
}
}

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
    , m_previousPreviewTime(0.0f)
    , m_rightPanelWidth(320.0f)
    , m_timelinePanelHeight(220.0f)
    , m_draggedStateIndex(-1)
    , m_selectedStateIndex(-1)
    , m_selectedTransitionIndex(-1)
    , m_contextTransitionIndex(-1)
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
    m_eventPayloadBuffer[0] = 0;
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
        // AnimationGraphDocument already implements IGraphDocument.  Keeping
        // a second adapter here created two competing lifecycle paths (the
        // exact anti-pattern absent from VS/BT/Prefab).  The framework and
        // this renderer now operate on the same authoritative document.
        m_framework = std::make_unique<CanvasFramework>(m_document.get());
        SYSTEM_LOG << "[AnimationGraphRenderer][ContextMenu-v6] Dedicated popup routing enabled\n";
    }
}

bool AnimationGraphRenderer::Load(const std::string& path)
{
    EnsureDocument();
    if (!m_document->Load(path))
        return false;

    m_currentPath = path;
    // A loaded graph starts a new document session.  Keeping commands from a
    // previous file is a legacy source of mutations against stale state.
    if (m_framework)
        m_framework->ClearCommandHistory();
    m_selectedStateIndices.clear();
    m_selectedStateIndex = -1;
    m_selectedTransitionIndex = -1;
    m_contextTransitionIndex = -1;
    m_draggedStateIndex = -1;
    m_draggedStateStartPositions.clear();
    m_canvasContextMenu.Clear();
    return true;
}
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
    const std::vector<std::string> issues = m_document->ValidateBinding();
    m_logs.clear();
    if (issues.empty())
        m_logs.push_back("Validation passed: the Animation Graph is internally consistent.");
    else
        m_logs = issues;
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
        {
            const std::vector<std::string> selectedStates = GetSelectedStateNames();
            if (m_framework->Undo())
                RestoreSelectionAfterHistory(selectedStates);
        }
        if (ImGui::IsItemHovered() && m_framework->CanUndo())
            ImGui::SetTooltip("Undo: %s", m_framework->GetUndoDescription().c_str());
        ImGui::SameLine();
        if (ImGui::Button("Redo##animation_history") && m_framework->CanRedo())
        {
            const std::vector<std::string> selectedStates = GetSelectedStateNames();
            if (m_framework->Redo())
                RestoreSelectionAfterHistory(selectedStates);
        }
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

bool AnimationGraphRenderer::DeleteSelectedStates()
{
    if (!m_document || m_selectedStateIndices.empty())
        return false;

    const nlohmann::json& data = m_document->GetData();
    std::vector<std::string> stateNames;
    stateNames.reserve(m_selectedStateIndices.size());
    for (size_t selectedIndex = 0; selectedIndex < m_selectedStateIndices.size(); ++selectedIndex)
    {
        const int index = m_selectedStateIndices[selectedIndex];
        if (index < 0 || static_cast<size_t>(index) >= data["states"].size())
            continue;
        const std::string stateName = data["states"][index].value("name", "");
        if (!stateName.empty() && std::find(stateNames.begin(), stateNames.end(), stateName) == stateNames.end())
            stateNames.push_back(stateName);
    }
    if (stateNames.empty())
        return false;

    const bool deleted = ExecuteDocumentCommand("Delete Animation States",
        [document = m_document.get(), stateNames]() -> bool {
            return document && document->RemoveStates(stateNames);
        });
    if (deleted)
    {
        m_selectedStateIndices.clear();
        m_selectedStateIndex = -1;
        m_selectedTransitionIndex = -1;
        m_contextTransitionIndex = -1;
    }
    return deleted;
}

void AnimationGraphRenderer::SelectStatesByName(const std::vector<std::string>& stateNames)
{
    m_selectedStateIndices.clear();
    m_selectedStateIndex = -1;
    if (!m_document)
        return;

    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return;
    for (size_t index = 0; index < data["states"].size(); ++index)
    {
        const std::string name = data["states"][index].value("name", "");
        if (std::find(stateNames.begin(), stateNames.end(), name) == stateNames.end())
            continue;
        m_selectedStateIndices.push_back(static_cast<int>(index));
    }
    if (!m_selectedStateIndices.empty())
    {
        m_selectedStateIndex = m_selectedStateIndices.front();
        strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer),
            data["states"][m_selectedStateIndex].value("name", "").c_str(), _TRUNCATE);
    }
}

std::vector<std::string> AnimationGraphRenderer::GetSelectedStateNames() const
{
    std::vector<std::string> names;
    if (!m_document)
        return names;
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return names;
    for (int index : m_selectedStateIndices)
    {
        if (index < 0 || static_cast<size_t>(index) >= data["states"].size())
            continue;
        const std::string name = data["states"][static_cast<size_t>(index)].value("name", "");
        if (!name.empty() && std::find(names.begin(), names.end(), name) == names.end())
            names.push_back(name);
    }
    return names;
}

void AnimationGraphRenderer::RestoreSelectionAfterHistory(const std::vector<std::string>& stateNames)
{
    SelectStatesByName(stateNames);
    m_selectedTransitionIndex = -1;
    m_contextTransitionIndex = -1;
    if (m_selectedStateIndex < 0)
    {
        m_selectedEventIndex = -1;
        m_eventStateBuffer[0] = 0;
    }
    else
    {
        RefreshSelectedEventEditor();
    }
    RefreshSelectedTransitionEditor();
}

void AnimationGraphRenderer::RefreshSelectedEventEditor()
{
    if (!m_document || m_selectedEventIndex < 0 || m_selectedStateIndex < 0)
        return;
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array() ||
        static_cast<size_t>(m_selectedStateIndex) >= data["states"].size())
    {
        m_selectedEventIndex = -1;
        return;
    }
    const nlohmann::json& state = data["states"][static_cast<size_t>(m_selectedStateIndex)];
    const std::string stateName = state.value("name", "");
    if (stateName != m_eventStateBuffer || !state.contains("events") || !state["events"].is_array() ||
        static_cast<size_t>(m_selectedEventIndex) >= state["events"].size())
    {
        m_selectedEventIndex = -1;
        return;
    }
    strncpy_s(m_eventPayloadBuffer, sizeof(m_eventPayloadBuffer),
        state["events"][static_cast<size_t>(m_selectedEventIndex)].value("payload", "").c_str(), _TRUNCATE);
}

void AnimationGraphRenderer::CopySelectedStatesToClipboard()
{
    if (!m_framework || !m_document || m_selectedStateIndices.empty())
        return;

    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("states") || !data["states"].is_array())
        return;

    std::set<std::string> selectedNames;
    nlohmann::json copiedStates = nlohmann::json::array();
    float minX = 0.0f;
    float minY = 0.0f;
    bool hasPosition = false;
    for (int index : m_selectedStateIndices)
    {
        if (index < 0 || static_cast<size_t>(index) >= data["states"].size())
            continue;
        const nlohmann::json& state = data["states"][index];
        const std::string name = state.value("name", "");
        if (name.empty() || !selectedNames.insert(name).second)
            continue;
        copiedStates.push_back(state);
        const float x = static_cast<float>(state.value("x", 0.0));
        const float y = static_cast<float>(state.value("y", 0.0));
        if (!hasPosition) { minX = x; minY = y; hasPosition = true; }
        else { minX = std::min(minX, x); minY = std::min(minY, y); }
    }
    if (copiedStates.empty())
        return;

    nlohmann::json copiedTransitions = nlohmann::json::array();
    if (data.contains("transitions") && data["transitions"].is_array())
    {
        for (const nlohmann::json& transition : data["transitions"])
        {
            const std::string from = transition.value("from", "");
            const std::string to = transition.value("to", "");
            if (selectedNames.count(from) && selectedNames.count(to))
                copiedTransitions.push_back(transition);
        }
    }
    m_framework->SetClipboardPayload(std::make_shared<AnimationGraphClipboardPayload>(
        std::move(copiedStates), std::move(copiedTransitions), minX, minY));
    SYSTEM_LOG << "[AnimationGraphRenderer] Copied animation state selection\n";
}

void AnimationGraphRenderer::PasteClipboardStates()
{
    if (!m_framework || !m_document || !m_framework->HasClipboardPayload("AnimationGraph"))
        return;
    const std::shared_ptr<const AnimationGraphClipboardPayload> payload =
        std::dynamic_pointer_cast<const AnimationGraphClipboardPayload>(m_framework->GetClipboardPayload());
    if (!payload || !payload->states.is_array() || payload->states.empty())
        return;

    const nlohmann::json before = m_document->CreateSnapshot();
    nlohmann::json after = before;
    if (!after.contains("states") || !after["states"].is_array()) after["states"] = nlohmann::json::array();
    if (!after.contains("transitions") || !after["transitions"].is_array()) after["transitions"] = nlohmann::json::array();

    std::set<std::string> usedNames;
    for (const nlohmann::json& state : after["states"])
        usedNames.insert(state.value("name", ""));
    std::map<std::string, std::string> pastedNames;
    std::vector<std::string> selectedNames;
    ++m_pasteSequence;
    const float fallbackOffset = 30.0f * static_cast<float>(m_pasteSequence);
    ImVec2 pasteOrigin(payload->minX + fallbackOffset, payload->minY + fallbackOffset);
    if (m_canvasEditor && m_canvasEditor->IsCanvasHovered())
        pasteOrigin = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);

    for (const nlohmann::json& sourceState : payload->states)
    {
        nlohmann::json pastedState = sourceState;
        const std::string sourceName = sourceState.value("name", "State");
        std::string pastedName = sourceName + " Copy";
        int suffix = 2;
        while (usedNames.count(pastedName))
            pastedName = sourceName + " Copy " + std::to_string(suffix++);
        usedNames.insert(pastedName);
        pastedNames[sourceName] = pastedName;
        selectedNames.push_back(pastedName);
        pastedState["name"] = pastedName;
        pastedState["x"] = pasteOrigin.x + static_cast<float>(sourceState.value("x", 0.0)) - payload->minX;
        pastedState["y"] = pasteOrigin.y + static_cast<float>(sourceState.value("y", 0.0)) - payload->minY;
        after["states"].push_back(std::move(pastedState));
    }
    for (const nlohmann::json& sourceTransition : payload->transitions)
    {
        const std::string from = sourceTransition.value("from", "");
        const std::string to = sourceTransition.value("to", "");
        const std::map<std::string, std::string>::const_iterator fromIt = pastedNames.find(from);
        const std::map<std::string, std::string>::const_iterator toIt = pastedNames.find(to);
        if (fromIt == pastedNames.end() || toIt == pastedNames.end())
            continue;
        nlohmann::json pastedTransition = sourceTransition;
        pastedTransition["from"] = fromIt->second;
        pastedTransition["to"] = toIt->second;
        after["transitions"].push_back(std::move(pastedTransition));
    }

    if (m_framework->ExecuteCommand(std::unique_ptr<GraphCommand>(new CallbackGraphCommand(
        selectedNames.size() == 1 ? "Paste Animation State" : "Paste Animation States",
        [document = m_document.get(), after]() -> bool { return document && document->RestoreSnapshot(after); },
        [document = m_document.get(), before]() -> bool { return document && document->RestoreSnapshot(before); }))))
    {
        SelectStatesByName(selectedNames);
        m_selectedTransitionIndex = -1;
    }
}

void AnimationGraphRenderer::HandleKeyboardShortcuts()
{
    if (!m_framework || ImGui::GetIO().WantTextInput)
        return;

    if (m_canvasEditor && m_canvasEditor->IsCanvasHovered() && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        DeleteSelectedStates();
        return;
    }

    const bool ctrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    if (!ctrlDown)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Z))
    {
        if (ImGui::GetIO().KeyShift)
        {
            const std::vector<std::string> selectedStates = GetSelectedStateNames();
            if (m_framework->Redo())
                RestoreSelectionAfterHistory(selectedStates);
        }
        else
        {
            const std::vector<std::string> selectedStates = GetSelectedStateNames();
            if (m_framework->Undo())
                RestoreSelectionAfterHistory(selectedStates);
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_Y))
    {
        const std::vector<std::string> selectedStates = GetSelectedStateNames();
        if (m_framework->Redo())
            RestoreSelectionAfterHistory(selectedStates);
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_A) && m_document)
    {
        const nlohmann::json& data = m_document->GetData();
        if (data.contains("states") && data["states"].is_array())
        {
            m_selectedStateIndices.clear();
            for (size_t index = 0; index < data["states"].size(); ++index)
                m_selectedStateIndices.push_back(static_cast<int>(index));
            m_selectedStateIndex = m_selectedStateIndices.empty() ? -1 : m_selectedStateIndices.front();
            if (m_selectedStateIndex >= 0)
                strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), data["states"][m_selectedStateIndex].value("name", "").c_str(), _TRUNCATE);
        }
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_C))
    {
        CopySelectedStatesToClipboard();
    }
    else if (ImGui::IsKeyPressed(ImGuiKey_V))
    {
        PasteClipboardStates();
    }
}

void AnimationGraphRenderer::RefreshSelectedTransitionEditor()
{
    if (!m_document || m_selectedTransitionIndex < 0)
        return;
    const nlohmann::json& data = m_document->GetData();
    if (!data.contains("transitions") || !data["transitions"].is_array() ||
        static_cast<size_t>(m_selectedTransitionIndex) >= data["transitions"].size())
        return;
    const nlohmann::json& transition = data["transitions"][m_selectedTransitionIndex];
    const std::string condition = transition.value("condition", "true");
    strncpy_s(m_eventNameBuffer, sizeof(m_eventNameBuffer), condition.c_str(), _TRUNCATE);
    m_transitionTimeBuffer = static_cast<float>(transition.value("duration", 0.1));
}

void AnimationGraphRenderer::RenderMainPanel()
{
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    if (totalWidth <= 0.0f) totalWidth = 800.0f;
    if (totalHeight <= 0.0f) totalHeight = 600.0f;

    static float s_splitX = 0.72f;
    static float s_splitY = 0.68f;
    static float s_splitMin = 0.20f;
    static float s_splitMax = 0.80f;
    float leftWidth = totalWidth * s_splitX;
    const float topHeight = totalHeight * s_splitY;
    const float splitterThickness = 6.0f;

    ImGui::BeginChild("AnimationGraph_GridRoot", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::BeginChild("AnimationGraph_TopRow", ImVec2(0, topHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::BeginChild("AnimationGraph_CanvasZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderGraphCanvas();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, splitterThickness);
        ImGui::InvisibleButton("##AnimationGraphWorkspaceSplitter", ImVec2(splitterThickness, topHeight));
        if (ImGui::IsItemActive())
        {
            s_splitX += ImGui::GetIO().MouseDelta.x / totalWidth;
            if (s_splitX < s_splitMin) s_splitX = s_splitMin;
            if (s_splitX > s_splitMax) s_splitX = s_splitMax;
        }

        ImGui::SameLine(0.0f, splitterThickness);
        ImGui::BeginChild("AnimationGraph_WorkspaceZone", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderRightPanelTabs();
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::InvisibleButton("##AnimationGraphTimelineSplitter", ImVec2(totalWidth, splitterThickness));
        if (ImGui::IsItemActive())
        {
            s_splitY += ImGui::GetIO().MouseDelta.y / totalHeight;
            if (s_splitY < s_splitMin) s_splitY = s_splitMin;
            if (s_splitY > s_splitMax) s_splitY = s_splitMax;
        }

        ImGui::BeginChild("AnimationGraph_BottomRow", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::BeginChild("AnimationGraph_TimelineZone", ImVec2(leftWidth, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderTimelinePanel();
        ImGui::EndChild();

        ImGui::SameLine(0.0f, splitterThickness);
        ImGui::InvisibleButton("##AnimationGraphPreviewSplitter", ImVec2(splitterThickness, ImGui::GetContentRegionAvail().y));
        if (ImGui::IsItemActive())
        {
            s_splitX += ImGui::GetIO().MouseDelta.x / totalWidth;
            if (s_splitX < s_splitMin) s_splitX = s_splitMin;
            if (s_splitX > s_splitMax) s_splitX = s_splitMax;
        }

        ImGui::SameLine(0.0f, splitterThickness);
        ImGui::BeginChild("AnimationGraph_PreviewZone", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        RenderAnimationPreviewPanel();
        ImGui::EndChild();
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
        for (size_t i = sourcePaths.size(); i > 0; --i)
            m_document->RemoveSource(i - 1);
        for (size_t i = 0; i < sourcePaths.size(); ++i)
            m_document->ImportTSXSource(sourcePaths[i], nullptr);
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
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            const int sourceIndex = static_cast<int>(i);
            ImGui::SetDragDropPayload("ANIMATION_GRAPH_BANK_STATE", &sourceIndex, sizeof(sourceIndex));
            ImGui::Text("Create directional state %s", MakeStateNameFromBank(sources[i].sourceName).c_str());
            ImGui::EndDragDropSource();
        }
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

void AnimationGraphRenderer::RenderAnimationBankPanel()
{
    ImGui::Text("Animation Banks");
    ImGui::Separator();
    ImGui::TextWrapped("1. Import a TSX bank.  2. Drag its header onto the canvas to create one directional state.");
    if (ImGui::Button("Import TSX...##animation_bank"))
        CanvasModalRenderer::Get().OpenAnimationGraphTsxPickerModal("./Gamedata/Animation/AnimationBanks");
    ImGui::SameLine();
    if (ImGui::Button("Reload All##animation_bank"))
    {
        std::vector<std::string> sourcePaths;
        const std::vector<AnimationGraphDocument::SourceDefinition>& existingSources = m_document->GetSources();
        for (const AnimationGraphDocument::SourceDefinition& source : existingSources)
            sourcePaths.push_back(source.filePath);
        for (size_t i = sourcePaths.size(); i > 0; --i)
            m_document->RemoveSource(i - 1);
        for (const std::string& sourcePath : sourcePaths)
            m_document->ImportTSXSource(sourcePath, nullptr);
    }

    CanvasModalRenderer::Get().RenderAnimationGraphTsxPickerModal();
    if (CanvasModalRenderer::Get().IsAnimationGraphTsxModalConfirmed())
    {
        std::vector<std::string> selectedFiles = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFiles();
        if (selectedFiles.empty())
        {
            const std::string selected = CanvasModalRenderer::Get().GetSelectedAnimationGraphTsxFile();
            if (!selected.empty()) selectedFiles.push_back(selected);
        }
        std::vector<std::string> errors;
        for (const std::string& selectedFile : selectedFiles)
            m_document->ImportTSXSource(selectedFile, &errors);
        CanvasModalRenderer::Get().CloseAnimationGraphTsxModal();
    }

    ImGui::Separator();
    const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
    if (sources.empty())
    {
        ImGui::TextDisabled("Import a TSX source to inspect its animation clips.");
        return;
    }

    for (size_t sourceIndex = 0; sourceIndex < sources.size(); ++sourceIndex)
    {
        const AnimationGraphDocument::SourceDefinition& source = sources[sourceIndex];
        ImGui::PushID(static_cast<int>(sourceIndex));
        const bool sourceOpen = ImGui::CollapsingHeader(source.sourceName.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            const int payloadSourceIndex = static_cast<int>(sourceIndex);
            ImGui::SetDragDropPayload("ANIMATION_GRAPH_BANK_STATE", &payloadSourceIndex, sizeof(payloadSourceIndex));
            ImGui::Text("Create directional state %s", MakeStateNameFromBank(source.sourceName).c_str());
            ImGui::EndDragDropSource();
        }
        if (sourceOpen)
        {
            ImGui::TextDisabled("%s", source.filePath.c_str());
            if (!source.imagePath.empty())
                ImGui::TextDisabled("Atlas: %s (%d x %d, tiles %d x %d, %d columns)",
                    source.imagePath.c_str(), source.imageWidth, source.imageHeight,
                    source.tileWidth, source.tileHeight, source.columns);
            ImGui::Text("%d clip(s)", static_cast<int>(source.clips.size()));
            ImGui::TextDisabled("Drag this bank onto the canvas to create state '%s' with directional clips.",
                MakeStateNameFromBank(source.sourceName).c_str());
            for (size_t clipIndex = 0; clipIndex < source.clips.size(); ++clipIndex)
            {
                const AnimationGraphDocument::SourceDefinition::ClipDefinition& clip = source.clips[clipIndex];
                ImGui::PushID(static_cast<int>(clipIndex));
                const bool isOpen = ImGui::TreeNodeEx(clip.id.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("ANIMATION_GRAPH_STATE", clip.id.c_str(), static_cast<int>(clip.id.size() + 1));
                    ImGui::Text("Create state from %s", clip.id.c_str());
                    ImGui::EndDragDropSource();
                }
                if (isOpen)
                {
                    int totalDurationMs = 0;
                    for (const AnimationGraphDocument::SourceDefinition::FrameDefinition& frame : clip.frames)
                        totalDurationMs += frame.durationMs;
                    const float fps = totalDurationMs > 0
                        ? static_cast<float>(clip.frames.size()) * 1000.0f / static_cast<float>(totalDurationMs)
                        : 0.0f;
                    ImGui::Text("Group: %s", clip.group.empty() ? "(none)" : clip.group.c_str());
                    ImGui::Text("Direction: %s", clip.direction.empty() ? "(unspecified)" : clip.direction.c_str());
                    ImGui::Text("Playback: %s", clip.loop ? "Loop" : "Once");
                    ImGui::Text("Source tile: %d", clip.sourceTileId);
                    if (clip.frames.empty())
                        ImGui::TextDisabled("Provisional clip: no Tiled frame sequence.");
                    else
                    {
                        ImGui::Text("%d frame(s), %d ms, %.1f FPS", static_cast<int>(clip.frames.size()), totalDurationMs, fps);
                        for (size_t frameIndex = 0; frameIndex < clip.frames.size(); ++frameIndex)
                            ImGui::BulletText("Frame %d: tile %d — %d ms", static_cast<int>(frameIndex),
                                clip.frames[frameIndex].tileId, clip.frames[frameIndex].durationMs);
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
        ImGui::PopID();
    }
}

void AnimationGraphRenderer::RenderTransitionEditorPanel()
{
    ImGui::Text("Transitions");
    ImGui::Separator();
    const nlohmann::json& data = m_document->GetData();
    if (data.contains("transitions") && data["transitions"].is_array())
    {
        for (size_t i = 0; i < data["transitions"].size(); ++i)
        {
            const std::string label = std::string(data["transitions"][i].value("from", "")) + " -> " + data["transitions"][i].value("to", "");
            if (ImGui::Selectable(label.c_str(), m_selectedTransitionIndex == static_cast<int>(i)))
            {
                m_selectedTransitionIndex = static_cast<int>(i);
                RefreshSelectedTransitionEditor();
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                m_contextTransitionIndex = static_cast<int>(i);
                ImGui::OpenPopup("AnimationTransitionListContext");
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
            const bool conditionCommitted = ImGui::IsItemDeactivatedAfterEdit();
            ImGui::DragFloat("Blend (s)", &m_transitionTimeBuffer, 0.01f, 0.0f, 10.0f, "%.2f");
            const bool blendCommitted = ImGui::IsItemDeactivatedAfterEdit();
            if (conditionCommitted || blendCommitted)
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
        }
        if (ImGui::BeginPopup("AnimationTransitionListContext"))
        {
            if (ImGui::MenuItem("Delete Transition") && m_contextTransitionIndex >= 0)
            {
                const int transitionIndex = m_contextTransitionIndex;
                if (ExecuteDocumentCommand("Delete Animation Transition",
                    [document = m_document.get(), transitionIndex]() -> bool {
                        return document->RemoveTransition(static_cast<size_t>(transitionIndex));
                    }))
                {
                    if (m_selectedTransitionIndex == transitionIndex)
                        m_selectedTransitionIndex = -1;
                    m_contextTransitionIndex = -1;
                }
            }
            ImGui::EndPopup();
        }
    }
}

void AnimationGraphRenderer::RenderTimelinePanel()
{
    ImGui::Text("Event Timeline");
    ImGui::Separator();
    const nlohmann::json& data = m_document->GetData();
    if (m_selectedStateIndex < 0 || !data.contains("states") || !data["states"].is_array() ||
        static_cast<size_t>(m_selectedStateIndex) >= data["states"].size())
    {
        ImGui::TextDisabled("Select a state in the canvas to edit its events.");
        return;
    }

    const nlohmann::json& state = data["states"][m_selectedStateIndex];
    const std::string stateName = state.value("name", "State");
    const std::string clip = state.value("defaultClip", "");
    const nlohmann::json events = state.contains("events") && state["events"].is_array()
        ? state["events"] : nlohmann::json::array();
    int frameCount = 1;
    int durationMs = 0;
    for (const AnimationGraphDocument::SourceDefinition& source : m_document->GetSources())
        for (const AnimationGraphDocument::SourceDefinition::ClipDefinition& sourceClip : source.clips)
            if (sourceClip.id == clip && !sourceClip.frames.empty())
            {
                frameCount = static_cast<int>(sourceClip.frames.size());
                for (const AnimationGraphDocument::SourceDefinition::FrameDefinition& frame : sourceClip.frames)
                    durationMs += frame.durationMs;
            }
    ImGui::Text("State: %s", stateName.c_str());
    ImGui::TextDisabled("Clip: %s", clip.empty() ? "(none)" : clip.c_str());
    ImGui::TextDisabled("%d frame(s), %.2f s, %d event(s)", frameCount,
        static_cast<float>(durationMs) / 1000.0f, static_cast<int>(events.size()));
    ImGui::Separator();
    const char* trackNames[] = { "Sound", "Gameplay", "VFX", "Spawn" };
    float previewPhase = m_previewDuration > 0.0f ? m_previewTime / m_previewDuration : 0.0f;
    previewPhase = std::max(0.0f, std::min(1.0f, previewPhase));
    const int previewFrame = frameCount <= 1 ? 0 : static_cast<int>(std::round(previewPhase * (frameCount - 1)));
    ImGui::TextDisabled("Click a frame on the desired track to add an event.  Playhead: frame %d", previewFrame + 1);
    ImGui::Separator();

    for (int track = 0; track < IM_ARRAYSIZE(trackNames); ++track)
    {
        ImGui::Text("%s", trackNames[track]);
        ImGui::SameLine(92.0f);
        for (int frame = 0; frame < frameCount; ++frame)
        {
            if (frame > 0) ImGui::SameLine();
            const float phase = frameCount <= 1 ? 0.0f : static_cast<float>(frame) / static_cast<float>(frameCount - 1);
            int eventIndex = -1;
            for (size_t index = 0; index < events.size(); ++index)
            {
                const nlohmann::json& event = events[index];
                if (event.value("type", "") == trackNames[track] &&
                    std::fabs(event.value("phase", 0.0f) - phase) < 0.001f)
                {
                    eventIndex = static_cast<int>(index);
                    break;
                }
            }
            ImGui::PushID(track * 1000 + frame);
            const bool isPlayhead = frame == previewFrame;
            if (isPlayhead)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.42f, 0.70f, 1.0f));
            if (ImGui::SmallButton(eventIndex >= 0 ? "*" : (isPlayhead ? ">" : ".")))
            {
                if (eventIndex >= 0)
                {
                    m_selectedEventIndex = eventIndex;
                    strncpy_s(m_eventStateBuffer, sizeof(m_eventStateBuffer), stateName.c_str(), _TRUNCATE);
                    strncpy_s(m_eventPayloadBuffer, sizeof(m_eventPayloadBuffer),
                        events[static_cast<size_t>(eventIndex)].value("payload", "").c_str(), _TRUNCATE);
                }
                else
                {
                    AnimationGraphDocument::EventDefinition event;
                    event.type = trackNames[track];
                    event.phase = phase;
                    event.payload = trackNames[track];
                    if (ExecuteDocumentCommand("Add Animation Event", [document = m_document.get(), stateName, event]() -> bool {
                        return document->AddStateEventMarker(stateName, event);
                    }))
                    {
                        m_selectedEventIndex = static_cast<int>(events.size());
                        strncpy_s(m_eventStateBuffer, sizeof(m_eventStateBuffer), stateName.c_str(), _TRUNCATE);
                        strncpy_s(m_eventPayloadBuffer, sizeof(m_eventPayloadBuffer), event.payload.c_str(), _TRUNCATE);
                    }
                }
            }
            if (isPlayhead)
                ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
            {
                const float seconds = frameCount <= 1 ? 0.0f :
                    (static_cast<float>(durationMs) / 1000.0f) * phase;
                ImGui::SetTooltip("Frame %d — %.2f s", frame, seconds);
            }
            ImGui::PopID();
        }
    }

    if (m_selectedEventIndex >= 0 && stateName == m_eventStateBuffer &&
        static_cast<size_t>(m_selectedEventIndex) < events.size())
    {
        const nlohmann::json& selectedEvent = events[static_cast<size_t>(m_selectedEventIndex)];
        const std::string selectedType = selectedEvent.value("type", "Custom");
        ImGui::Separator();
        ImGui::Text("Selected %s event — frame %d", selectedType.c_str(),
            static_cast<int>(std::round(selectedEvent.value("phase", 0.0f) * std::max(0, frameCount - 1))));
        ImGui::InputText("Action / asset", m_eventPayloadBuffer, sizeof(m_eventPayloadBuffer));
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            AnimationGraphDocument::EventDefinition updated;
            updated.type = selectedType;
            updated.phase = selectedEvent.value("phase", 0.0f);
            updated.payload = m_eventPayloadBuffer;
            ExecuteDocumentCommand("Edit Animation Event", [document = m_document.get(), stateName, updated, selectedIndex = m_selectedEventIndex]() -> bool {
                return document->UpdateStateEventMarker(stateName, static_cast<size_t>(selectedIndex), updated);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete Event"))
        {
            const int selectedIndex = m_selectedEventIndex;
            if (ExecuteDocumentCommand("Delete Animation Event", [document = m_document.get(), stateName, selectedIndex]() -> bool {
                return document->RemoveStateEventMarker(stateName, static_cast<size_t>(selectedIndex));
            }))
                m_selectedEventIndex = -1;
        }
    }
}

void AnimationGraphRenderer::RenderAnimationPreviewPanel()
{
    ImGui::Text("Animation Preview");
    ImGui::Separator();
    const nlohmann::json& data = m_document->GetData();
    if (m_selectedStateIndex < 0 || !data.contains("states") || !data["states"].is_array() ||
        static_cast<size_t>(m_selectedStateIndex) >= data["states"].size())
    {
        ImGui::TextDisabled("Select a state to preview its resolved directional clip.");
        return;
    }

    const nlohmann::json& state = data["states"][m_selectedStateIndex];
    const std::string stateName = state.value("name", "State");
    if (m_previewStateName != stateName)
    {
        // A preview is scoped to one semantic state.  Do not carry an
        // unrelated playhead or event history into the newly selected state.
        m_previewStateName = stateName;
        m_showRunPreview = false;
        m_previewTime = 0.0f;
        m_previousPreviewTime = 0.0f;
        m_previewEventLog.clear();
    }
    static int previewDirection = static_cast<int>(AnimationGraphDocument::Direction::South);
    const char* directions[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    ImGui::Text("State: %s", stateName.c_str());
    ImGui::Combo("Facing", &previewDirection, directions, IM_ARRAYSIZE(directions));
    const std::string resolvedClip = m_document->ResolveClipForState(stateName,
        static_cast<AnimationGraphDocument::Direction>(previewDirection));
    ImGui::Text("Resolved clip: %s", resolvedClip.empty() ? "(none)" : resolvedClip.c_str());

    const AnimationGraphDocument::SourceDefinition* previewSource = nullptr;
    const AnimationGraphDocument::SourceDefinition::ClipDefinition* previewClip = nullptr;
    for (const AnimationGraphDocument::SourceDefinition& source : m_document->GetSources())
    {
        for (const AnimationGraphDocument::SourceDefinition::ClipDefinition& candidate : source.clips)
        {
            if (candidate.id == resolvedClip)
            {
                previewSource = &source;
                previewClip = &candidate;
                break;
            }
        }
        if (previewClip) break;
    }

    int durationMs = 0;
    if (previewClip)
        for (const AnimationGraphDocument::SourceDefinition::FrameDefinition& frame : previewClip->frames)
            durationMs += frame.durationMs;
    m_previewDuration = durationMs > 0 ? static_cast<float>(durationMs) / 1000.0f : 1.0f;
    bool beganPlayback = false;
    if (ImGui::Button(m_showRunPreview ? "Pause" : "Play"))
    {
        m_showRunPreview = !m_showRunPreview;
        beganPlayback = m_showRunPreview && m_previewTime <= 0.0001f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        m_showRunPreview = false;
        m_previewTime = 0.0f;
        m_previousPreviewTime = 0.0f;
        m_previewEventLog.clear();
    }
    bool previewLooped = false;
    m_previousPreviewTime = m_previewTime;
    if (m_showRunPreview)
    {
        m_previewTime += ImGui::GetIO().DeltaTime;
        if (m_previewTime >= m_previewDuration)
        {
            m_previewTime = std::fmod(m_previewTime, m_previewDuration);
            previewLooped = true;
        }
    }
    if (!previewSource || !previewClip || previewClip->frames.empty())
    {
        ImGui::TextDisabled("No animated frame is available for this clip.");
        return;
    }

    // The preview deliberately does not execute gameplay/audio/VFX systems.
    // It simulates their schedule and exposes each crossed event to the author.
    if (m_showRunPreview && state.contains("events") && state["events"].is_array())
    {
        for (const nlohmann::json& event : state["events"])
        {
            const float phase = event.value("phase", 0.0f);
            const float eventTime = phase * m_previewDuration;
            const bool crossed = previewLooped
                ? eventTime >= m_previousPreviewTime || eventTime <= m_previewTime
                : eventTime > m_previousPreviewTime && eventTime <= m_previewTime;
            const bool startsAtZero = beganPlayback && phase <= 0.0001f;
            if (!crossed && !startsAtZero)
                continue;

            std::ostringstream message;
            message << "[" << event.value("type", "Gameplay") << "] "
                << event.value("payload", "") << " at " << static_cast<int>(std::round(phase * 100.0f)) << "%";
            m_previewEventLog.push_back(message.str());
            if (m_previewEventLog.size() > 4)
                m_previewEventLog.erase(m_previewEventLog.begin());
        }
    }
    if (previewSource->imagePath.empty() || previewSource->imageWidth <= 0 || previewSource->imageHeight <= 0 ||
        previewSource->tileWidth <= 0 || previewSource->tileHeight <= 0 || previewSource->columns <= 0)
    {
        ImGui::TextDisabled("This bank has no usable atlas metadata. Reload its TSX source.");
        return;
    }

    int frameIndex = 0;
    int elapsedMs = 0;
    const int playheadMs = static_cast<int>(m_previewTime * 1000.0f);
    for (size_t index = 0; index < previewClip->frames.size(); ++index)
    {
        elapsedMs += previewClip->frames[index].durationMs;
        if (playheadMs < elapsedMs || index + 1 == previewClip->frames.size())
        {
            frameIndex = static_cast<int>(index);
            break;
        }
    }
    const int tileId = previewClip->frames[static_cast<size_t>(frameIndex)].tileId;
    const int tileColumn = tileId % previewSource->columns;
    const int tileRow = tileId / previewSource->columns;
    const float srcX = static_cast<float>(previewSource->margin + tileColumn * (previewSource->tileWidth + previewSource->spacing));
    const float srcY = static_cast<float>(previewSource->margin + tileRow * (previewSource->tileHeight + previewSource->spacing));
    const ImVec2 uv0(srcX / static_cast<float>(previewSource->imageWidth), srcY / static_cast<float>(previewSource->imageHeight));
    const ImVec2 uv1((srcX + previewSource->tileWidth) / static_cast<float>(previewSource->imageWidth),
        (srcY + previewSource->tileHeight) / static_cast<float>(previewSource->imageHeight));
    // imagePath is deliberately stored project-relative in the graph.  Resolve
    // it from the executable before handing it to SDL so the editor works from
    // its Debug/bin working directory as well as from the project root.
    const std::string texturePath = ResolveResourcePath(previewSource->imagePath);
    const std::string textureId = "AnimationGraphPreview:" + previewSource->imagePath;
    SDL_Texture* texture = DataManager::Get().GetSprite(textureId, texturePath);
    if (!texture)
    {
        ImGui::TextDisabled("Unable to load atlas: %s", texturePath.c_str());
        return;
    }
    const float scale = 2.0f;
    const ImVec2 imageSize(previewSource->tileWidth * scale, previewSource->tileHeight * scale);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > imageSize.x)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availableWidth - imageSize.x) * 0.5f);
    ImGui::Image((ImTextureID)(intptr_t)texture, imageSize, uv0, uv1);
    ImGui::TextDisabled("Frame %d / %d — %.2f s", frameIndex + 1,
        static_cast<int>(previewClip->frames.size()), m_previewTime);
    ImGui::Separator();
    ImGui::TextDisabled("Event playback (simulation)");
    if (m_previewEventLog.empty())
    {
        ImGui::TextDisabled("Play this state to inspect its scheduled events.");
    }
    else
    {
        for (const std::string& message : m_previewEventLog)
            ImGui::Text("%s", message.c_str());
    }
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
}

void AnimationGraphRenderer::RenderRightPanelTabs()
{
    if (ImGui::BeginTabBar("AnimationGraphRightTabs"))
    {
        if (ImGui::BeginTabItem("Inspector"))
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
                if (selectedState.contains("directionClips") && selectedState["directionClips"].is_array())
                {
                    if (ImGui::TreeNodeEx("Directional variants##animation_state", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::TextDisabled("Resolved by runtime from entity/player orientation.");
                        for (const nlohmann::json& mapping : selectedState["directionClips"])
                        {
                            const std::string direction = mapping.value("direction", "?");
                            const std::string clip = mapping.value("clip", "");
                            ImGui::BulletText("%s: %s", direction.c_str(),
                                clip.empty() ? "(fallback clip)" : clip.c_str());
                        }
                        ImGui::TreePop();
                    }
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
            if (!m_logs.empty())
            {
                ImGui::Separator();
                ImGui::Text("Validation");
                for (const std::string& log : m_logs)
                {
                    const bool passed = log.find("Validation passed:") == 0;
                    ImGui::TextColored(passed ? ImVec4(0.35f, 0.95f, 0.45f, 1.0f)
                                               : ImVec4(1.0f, 0.75f, 0.25f, 1.0f),
                        "%s", log.c_str());
                }
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Banks"))
        {
            RenderAnimationBankPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Transitions"))
        {
            RenderTransitionEditorPanel();
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

    const int frameNumber = ImGui::GetFrameCount();
    if (m_lastGraphCanvasFrame == frameNumber)
    {
        SYSTEM_LOG << "[AnimationGraphRenderer] Ignored duplicate canvas render in frame "
                   << frameNumber << "\n";
        return;
    }
    m_lastGraphCanvasFrame = frameNumber;
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
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_GRAPH_BANK_STATE"))
        {
            if (payload->Data && payload->DataSize == sizeof(int))
            {
                const int sourceIndex = *static_cast<const int*>(payload->Data);
                const std::vector<AnimationGraphDocument::SourceDefinition>& sources = m_document->GetSources();
                if (sourceIndex >= 0 && static_cast<size_t>(sourceIndex) < sources.size())
                {
                    const std::string stateName = m_document->MakeUniqueStateName(
                        MakeStateNameFromBank(sources[static_cast<size_t>(sourceIndex)].sourceName));
                    const ImVec2 graphPos = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);
                    if (ExecuteDocumentCommand("Create Directional Animation State",
                        [document = m_document.get(), stateName, sourceIndex, graphPos]() -> bool {
                            return document->AddStateFromSource(stateName, static_cast<size_t>(sourceIndex)) &&
                                   document->SetStatePosition(stateName, graphPos.x, graphPos.y);
                        }))
                    {
                        SelectStatesByName({ stateName });
                    }
                }
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIMATION_GRAPH_STATE"))
        {
            const char* sourceName = static_cast<const char*>(payload->Data);
            if (sourceName && *sourceName)
            {
                std::string stateName = m_document->MakeUniqueStateName(sourceName);
                const ImVec2 graphPos = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);
                if (ExecuteDocumentCommand("Create Animation State",
                    [document = m_document.get(), stateName, sourceName, graphPos]() -> bool {
                        return document->AddStateFromClip(stateName, sourceName) &&
                               document->SetStatePosition(stateName, graphPos.x, graphPos.y);
                    }))
                {
                    SelectStatesByName({ stateName });
                }
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
    // Popup widgets are rendered below, but their left-click must not also
    // be handled by this canvas first.  Otherwise a click on "Delete
    // Selected" clears the selection as an empty-canvas click and shifts the
    // menu entries underneath the pointer.
    const bool canvasInputCapturedByContextMenu = m_canvasContextMenu.IsOpen();
    std::vector<std::tuple<int, float, float, float, float>> minimapNodes;
    minimapNodes.reserve(data["states"].size());
    float graphMinX = 0.0f;
    float graphMaxX = 0.0f;
    float graphMinY = 0.0f;
    float graphMaxY = 0.0f;
    bool hasGraphBounds = false;
    bool canvasNodeInteraction = false;
    bool canvasLinkInteraction = false;
    int contextNodeIndex = -1;
    int contextTransitionIndex = -1;

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

            // Keep tangents consistent (outgoing right, incoming left) and
            // adapt their length to the horizontal separation.  This avoids
            // the accidental S-curves produced by direction-sign inversion.
            const float horizontalDistance = fabsf(to.x - from.x);
            const float controlOffset = std::max(40.0f * zoom, std::min(160.0f * zoom, horizontalDistance * 0.5f));
            const ImVec2 controlA(from.x + controlOffset, from.y);
            const ImVec2 controlB(to.x - controlOffset, to.y);
            const bool isSelectedTransition = m_selectedTransitionIndex == static_cast<int>(transitionIndex);
            drawList->AddBezierCubic(from,
                controlA, controlB, to,
                isSelectedTransition ? IM_COL32(0, 220, 255, 255) : IM_COL32(120, 185, 255, 220),
                isSelectedTransition ? 3.0f : 2.0f, 24);

            const auto bezierPoint = [&](float t) -> ImVec2 {
                const float u = 1.0f - t;
                return ImVec2(
                    u * u * u * from.x + 3.0f * u * u * t * controlA.x + 3.0f * u * t * t * controlB.x + t * t * t * to.x,
                    u * u * u * from.y + 3.0f * u * u * t * controlA.y + 3.0f * u * t * t * controlB.y + t * t * t * to.y);
            };
            const ImVec2 labelPosition = bezierPoint(0.5f);
            const std::string condition = transition.value("condition", "true");
            drawList->AddText(ImVec2(labelPosition.x + 6.0f, labelPosition.y - 16.0f),
                IM_COL32(170, 220, 255, 255), condition.c_str());

            float closestDistanceSquared = 9.0f * 9.0f;
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            for (int sample = 0; sample <= 24; ++sample)
            {
                const ImVec2 curvePoint = bezierPoint(static_cast<float>(sample) / 24.0f);
                const float dx = mouse.x - curvePoint.x;
                const float dy = mouse.y - curvePoint.y;
                const float distanceSquared = dx * dx + dy * dy;
                if (distanceSquared <= closestDistanceSquared)
                    closestDistanceSquared = distanceSquared;
            }
            if (!canvasInputCapturedByContextMenu && closestDistanceSquared < 9.0f * 9.0f && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                canvasLinkInteraction = true;
                m_selectedTransitionIndex = static_cast<int>(transitionIndex);
                RefreshSelectedTransitionEditor();
            }
            if (closestDistanceSquared < 9.0f * 9.0f && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                canvasLinkInteraction = true;
                // The node pass below can still override this hit.  A state
                // always takes priority over a link, as in the other editors.
                if (contextTransitionIndex < 0)
                    contextTransitionIndex = static_cast<int>(transitionIndex);
            }
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
        const bool isSelected = std::find(m_selectedStateIndices.begin(), m_selectedStateIndices.end(), static_cast<int>(i)) != m_selectedStateIndices.end();
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
        const float pinHitRadius = pinRadius + 5.0f;
        const bool inputHovered = inputDx * inputDx + inputDy * inputDy <= pinHitRadius * pinHitRadius;
        const bool outputHovered = outputDx * outputDx + outputDy * outputDy <= pinHitRadius * pinHitRadius;

        if (!canvasInputCapturedByContextMenu && outputHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            canvasNodeInteraction = true;
            m_linkStartStateIndex = static_cast<int>(i);
            m_linkDragStartPos = outputPin;
        }
        else if (!canvasInputCapturedByContextMenu && inputHovered && m_linkStartStateIndex >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left) &&
                 m_linkStartStateIndex != static_cast<int>(i) &&
                 static_cast<size_t>(m_linkStartStateIndex) < data["states"].size())
        {
            canvasNodeInteraction = true;
            const std::string fromState = data["states"][m_linkStartStateIndex].value("name", "");
            const std::string toState = stateName;
            ExecuteDocumentCommand("Create Animation Transition",
                [document = m_document.get(), fromState, toState]() -> bool {
                    return document->AddTransition(fromState, toState, "true", 0.1);
                });
            m_linkStartStateIndex = -1;
        }
        else if (!canvasInputCapturedByContextMenu && nodeHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            canvasNodeInteraction = true;
            const bool appendSelection = ImGui::GetIO().KeyCtrl;
            std::vector<int>::iterator selectedIt = std::find(m_selectedStateIndices.begin(), m_selectedStateIndices.end(), static_cast<int>(i));
            const bool wasSelected = selectedIt != m_selectedStateIndices.end();
            if (appendSelection)
            {
                if (!wasSelected)
                    m_selectedStateIndices.push_back(static_cast<int>(i));
                else
                    m_selectedStateIndices.erase(selectedIt);
            }
            else if (!wasSelected)
            {
                // A click on an already selected state preserves a group so
                // the whole selection can be moved together.
                m_selectedStateIndices.clear();
                m_selectedStateIndices.push_back(static_cast<int>(i));
            }
            const bool stateIsSelected = std::find(m_selectedStateIndices.begin(), m_selectedStateIndices.end(), static_cast<int>(i)) != m_selectedStateIndices.end();
            m_selectedStateIndex = m_selectedStateIndices.empty() ? -1 :
                (stateIsSelected ? static_cast<int>(i) : m_selectedStateIndices.front());
            strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), stateName.c_str(), _TRUNCATE);
            m_draggedStateIndex = stateIsSelected ? static_cast<int>(i) : -1;
            m_draggedStateName = stateName;
            m_dragStartPos = ImVec2(x, y);
            m_draggedStateStartPositions.clear();
            for (size_t selectedIndex = 0; selectedIndex < m_selectedStateIndices.size(); ++selectedIndex)
            {
                const int index = m_selectedStateIndices[selectedIndex];
                if (index < 0 || static_cast<size_t>(index) >= data["states"].size())
                    continue;
                const nlohmann::json& selectedState = data["states"][index];
                m_draggedStateStartPositions.push_back({
                    selectedState.value("name", ""),
                    ImVec2(static_cast<float>(selectedState.value("x", 0.0)),
                           static_cast<float>(selectedState.value("y", 0.0))) });
            }
        }
        if (m_draggedStateIndex == static_cast<int>(i) && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            for (size_t positionIndex = 0; positionIndex < m_draggedStateStartPositions.size(); ++positionIndex)
            {
                const std::string& draggedName = m_draggedStateStartPositions[positionIndex].name;
                AnimationGraphDocument::StateDefinition* draggedState = m_document->GetState(draggedName);
                if (draggedState)
                    m_document->SetStatePosition(draggedName,
                        draggedState->x + delta.x / zoom,
                        draggedState->y + delta.y / zoom);
            }
            m_canvasEditor->UpdateAutoPanning(true);
        }
        if (nodeHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            canvasNodeInteraction = true;
            // Context actions intentionally preserve the active selection.
            contextNodeIndex = static_cast<int>(i);
        }
    }

    // Empty-canvas drag selects every intersecting node. Ctrl keeps the
    // existing selection, matching the interaction contract of the other graph editors.
    if (!canvasInputCapturedByContextMenu && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !canvasNodeInteraction && !canvasLinkInteraction &&
        m_canvasEditor->IsPointInCanvas(ImGui::GetIO().MousePos))
    {
        m_isSelectingRectangle = true;
        m_selectionRectStart = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);
        m_selectionRectEnd = m_selectionRectStart;
        if (!ImGui::GetIO().KeyCtrl)
        {
            m_selectedStateIndices.clear();
            m_selectedStateIndex = -1;
        }
    }
    if (m_isSelectingRectangle)
    {
        m_selectionRectEnd = m_canvasEditor->ScreenToCanvas(ImGui::GetIO().MousePos);
        const ImVec2 selectionStartScreen = m_canvasEditor->CanvasToScreen(m_selectionRectStart);
        const ImVec2 selectionEndScreen = m_canvasEditor->CanvasToScreen(m_selectionRectEnd);
        const ImVec2 selectionMin(std::min(selectionStartScreen.x, selectionEndScreen.x), std::min(selectionStartScreen.y, selectionEndScreen.y));
        const ImVec2 selectionMax(std::max(selectionStartScreen.x, selectionEndScreen.x), std::max(selectionStartScreen.y, selectionEndScreen.y));
        drawList->AddRectFilled(selectionMin, selectionMax, IM_COL32(0, 190, 255, 35));
        drawList->AddRect(selectionMin, selectionMax, IM_COL32(0, 210, 255, 230), 0.0f, 0, 1.5f);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const float minX = std::min(m_selectionRectStart.x, m_selectionRectEnd.x);
            const float maxX = std::max(m_selectionRectStart.x, m_selectionRectEnd.x);
            const float minY = std::min(m_selectionRectStart.y, m_selectionRectEnd.y);
            const float maxY = std::max(m_selectionRectStart.y, m_selectionRectEnd.y);
            for (size_t stateIndex = 0; stateIndex < data["states"].size(); ++stateIndex)
            {
                const nlohmann::json& state = data["states"][stateIndex];
                const float stateX = static_cast<float>(state.value("x", 0.0));
                const float stateY = static_cast<float>(state.value("y", 0.0));
                if (stateX <= maxX && stateX + nodeWidth >= minX && stateY <= maxY && stateY + nodeHeight >= minY)
                {
                    const int index = static_cast<int>(stateIndex);
                    if (std::find(m_selectedStateIndices.begin(), m_selectedStateIndices.end(), index) == m_selectedStateIndices.end())
                        m_selectedStateIndices.push_back(index);
                    m_selectedStateIndex = index;
                    strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), state.value("name", "").c_str(), _TRUNCATE);
                }
            }
            m_isSelectingRectangle = false;
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) &&
        m_canvasEditor->IsPointInCanvas(ImGui::GetIO().MousePos))
    {
        // CanvasContextMenu owns the popup identity and target priority.  This
        // replaces the former independent state/link/canvas popups.
        m_canvasContextMenu.OpenForHitTest(contextNodeIndex, contextTransitionIndex);
        SYSTEM_LOG << "[AnimationGraphRenderer][ContextMenu-v6] Open frame="
                   << frameNumber << " node=" << contextNodeIndex
                   << " transition=" << contextTransitionIndex << "\n";
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && m_draggedStateIndex >= 0)
    {
        const std::vector<StatePosition> before = m_draggedStateStartPositions;
        std::vector<StatePosition> after;
        bool didMove = false;
        for (size_t positionIndex = 0; positionIndex < before.size(); ++positionIndex)
        {
            const AnimationGraphDocument::StateDefinition* movedState = m_document->GetState(before[positionIndex].name);
            if (!movedState)
                continue;
            const ImVec2 position(static_cast<float>(movedState->x), static_cast<float>(movedState->y));
            after.push_back({ before[positionIndex].name, position });
            didMove = didMove || position.x != before[positionIndex].position.x || position.y != before[positionIndex].position.y;
        }
        if (didMove)
        {
            m_framework->ExecuteCommand(std::unique_ptr<GraphCommand>(
                new CallbackGraphCommand("Move Animation States",
                    [document = m_document.get(), after]() -> bool {
                        bool movedAny = false;
                        for (size_t positionIndex = 0; positionIndex < after.size(); ++positionIndex)
                            movedAny = document->SetStatePosition(after[positionIndex].name,
                                after[positionIndex].position.x, after[positionIndex].position.y) || movedAny;
                        return movedAny;
                    },
                    [document = m_document.get(), before]() -> bool {
                        bool movedAny = false;
                        for (size_t positionIndex = 0; positionIndex < before.size(); ++positionIndex)
                            movedAny = document->SetStatePosition(before[positionIndex].name,
                                before[positionIndex].position.x, before[positionIndex].position.y) || movedAny;
                        return movedAny;
                    })));
        }
        m_draggedStateIndex = -1;
        m_draggedStateName.clear();
        m_draggedStateStartPositions.clear();
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
    // Exactly like PlaceholderCanvas, menu widgets only choose an action.
    // The mutation is deliberately applied after EndPopup(): changing the
    // graph or the selection while ImGui is iterating the popup was the last
    // remaining legacy interaction path and produced a second canvas popup.
    enum class ContextAction { None, DeleteState, DeleteTransition, Copy, Paste, SelectAll, ClearSelection, DeleteSelected, ResetView };
    ContextAction contextAction = ContextAction::None;
    int contextActionIndex = -1;
    std::string contextActionStateName;
    if (m_canvasContextMenu.Begin())
    {
        const CanvasContextTargetType targetType = m_canvasContextMenu.GetTargetType();
        const int targetIndex = m_canvasContextMenu.GetTargetId();
        if (targetType == CanvasContextTargetType::Node)
        {
            if (targetIndex >= 0 && static_cast<size_t>(targetIndex) < data["states"].size() &&
                ImGui::MenuItem("Delete State"))
            {
                contextAction = ContextAction::DeleteState;
                contextActionStateName = data["states"][targetIndex].value("name", "");
            }
        }
        else if (targetType == CanvasContextTargetType::Link)
        {
            if (targetIndex >= 0 && static_cast<size_t>(targetIndex) < data["transitions"].size() &&
                ImGui::MenuItem("Delete Transition"))
            {
                contextAction = ContextAction::DeleteTransition;
                contextActionIndex = targetIndex;
            }
        }
        else if (targetType == CanvasContextTargetType::Canvas)
        {
            if (ImGui::MenuItem("Copy Selection", "Ctrl+C", false, !m_selectedStateIndices.empty()))
                contextAction = ContextAction::Copy;
            if (ImGui::MenuItem("Paste", "Ctrl+V", false,
                m_framework && m_framework->HasClipboardPayload("AnimationGraph")))
                contextAction = ContextAction::Paste;
            ImGui::Separator();
            if (ImGui::MenuItem("Select All", "Ctrl+A"))
                contextAction = ContextAction::SelectAll;
            if (ImGui::MenuItem("Clear Selection"))
                contextAction = ContextAction::ClearSelection;
            if (!m_selectedStateIndices.empty() && ImGui::MenuItem("Delete Selected"))
                contextAction = ContextAction::DeleteSelected;
            ImGui::Separator();
            if (ImGui::MenuItem("Reset View"))
                contextAction = ContextAction::ResetView;
        }
        // MenuItem does not reliably close a plain popup in every ImGui
        // configuration used by the editor.  Close the *current* popup while
        // it is still active, but defer the graph mutation until after
        // EndPopup below.  This prevents the residual, selection-less canvas
        // menu that looked like a second context menu.
        if (contextAction != ContextAction::None)
        {
            SYSTEM_LOG << "[AnimationGraphRenderer][ContextMenu-v6] Action="
                       << static_cast<int>(contextAction) << " frame=" << frameNumber << "\n";
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (contextAction != ContextAction::None)
    {
        if (contextAction == ContextAction::DeleteState)
        {
            if (ExecuteDocumentCommand("Delete Animation State",
                [document = m_document.get(), contextActionStateName]() -> bool {
                    return document && !contextActionStateName.empty() && document->RemoveState(contextActionStateName);
                }))
            {
                m_selectedStateIndices.clear();
                m_selectedStateIndex = -1;
                m_selectedTransitionIndex = -1;
            }
        }
        else if (contextAction == ContextAction::DeleteTransition)
        {
            if (ExecuteDocumentCommand("Delete Animation Transition",
                [document = m_document.get(), contextActionIndex]() -> bool {
                    return document && contextActionIndex >= 0 && document->RemoveTransition(static_cast<size_t>(contextActionIndex));
                }) && m_selectedTransitionIndex == contextActionIndex)
            {
                m_selectedTransitionIndex = -1;
            }
        }
        else if (contextAction == ContextAction::Copy)
        {
            CopySelectedStatesToClipboard();
        }
        else if (contextAction == ContextAction::Paste)
        {
            PasteClipboardStates();
        }
        else if (contextAction == ContextAction::SelectAll)
        {
            m_selectedStateIndices.clear();
            for (size_t stateIndex = 0; stateIndex < data["states"].size(); ++stateIndex)
                m_selectedStateIndices.push_back(static_cast<int>(stateIndex));
            m_selectedStateIndex = m_selectedStateIndices.empty() ? -1 : m_selectedStateIndices.front();
            if (m_selectedStateIndex >= 0)
                strncpy_s(m_stateNameBuffer, sizeof(m_stateNameBuffer), data["states"][m_selectedStateIndex].value("name", "").c_str(), _TRUNCATE);
        }
        else if (contextAction == ContextAction::ClearSelection)
        {
            m_selectedStateIndices.clear();
            m_selectedStateIndex = -1;
        }
        else if (contextAction == ContextAction::DeleteSelected)
        {
            DeleteSelectedStates();
        }
        else if (contextAction == ContextAction::ResetView)
        {
            ResetView();
        }

        // Clear the routing target after the action.  The popup itself was
        // closed above while it was active.
        m_canvasContextMenu.Clear();
    }

    m_canvasEditor->RenderMinimap();
    m_canvasEditor->EndRender();
}

void AnimationGraphRenderer::RenderInspectorPanel() {}
void AnimationGraphRenderer::RenderVerificationPanel() {}
void AnimationGraphRenderer::RenderSingleTransition(size_t, const ImVec2&) {}
void AnimationGraphRenderer::EnsureStatePositions() {}
void AnimationGraphRenderer::RenderGraphNodePorts(const nlohmann::json&, int, bool) {}
int AnimationGraphRenderer::FindStateIndexByName(const std::string& name) const { return m_document ? m_document->FindStateIndex(name) : -1; }
int AnimationGraphRenderer::FindEventIndexByState(const std::string&) const { return -1; }
std::string AnimationGraphRenderer::MakeUniqueStateName(const std::string& baseName) const { return m_document ? m_document->MakeUniqueStateName(baseName) : baseName; }

} // namespace Olympe
