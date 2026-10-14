#pragma once

#include "Framework/IGraphDocument.h"
#include "Framework/CanvasFramework.h"
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Utilities/CustomCanvasEditor.h"
#include "Utilities/CanvasMinimapRenderer.h"
#include "Utilities/CanvasContextMenu.h"
#include "../third_party/imgui/imgui.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace Olympe {

class AnimationGraphDocument;

class AnimationGraphRenderer : public IGraphRenderer
{
public:
    AnimationGraphRenderer();
    ~AnimationGraphRenderer() override;

    bool Load(const std::string& path) override;
    bool Save(const std::string& path) override;
    void Render() override;
    bool IsDirty() const override;
    std::string GetGraphType() const override;
    std::string GetCurrentPath() const override;
    void SetCurrentPath(const std::string& path);
    void SaveCanvasState() override;
    void RestoreCanvasState() override;
    std::string GetCanvasStateJSON() const override;
    void SetCanvasStateJSON(const std::string& json) override;
    void VerifyGraph() override;
    void RunGraph() override;
    bool SupportsVerification() const override { return true; }
    bool SupportsExecution() const override { return true; }
    bool SupportsMinimap() const override { return true; }
    bool IsMinimapVisible() const override { return m_minimapVisible; }
    void SetMinimapVisible(bool visible) override { m_minimapVisible = visible; }
    float GetMinimapSize() const override { return m_minimapSize; }
    void SetMinimapSize(float size) override;
    int GetMinimapPosition() const override;
    void SetMinimapPosition(int pos) override;
    bool SupportsGrid() const override { return true; }
    bool IsGridVisible() const override { return m_gridVisible; }
    void SetGridVisible(bool visible) override;
    bool SupportsResetView() const override { return true; }
    void ResetView() override;
    void RenderFrameworkModals() override;

    IGraphDocument* GetDocument() const { return static_cast<IGraphDocument*>(m_document.get()); }

private:
    void EnsureDocument();
    void RenderToolbar();
    void RenderMainPanel();
    void RenderInspectorPanel();
    void RenderVerificationPanel();
    void RenderStateEditorPanel();
    void RenderAnimationBankPanel();
    void RenderTransitionEditorPanel();
    void RenderTimelinePanel();
    void RenderAnimationPreviewPanel();
    void RenderGraphCanvas();
    void RenderRightPanel();
    void RenderRightPanelTabs();
    void RenderSingleTransition(size_t transitionIndex, const ImVec2& canvasOrigin);
    void EnsureStatePositions();
    void RenderGraphNodePorts(const nlohmann::json& state, int stateIndex, bool isSelected);
    bool ExecuteDocumentCommand(const std::string& description, const std::function<bool()>& edit);
    bool DeleteSelectedStates();
    void CopySelectedStatesToClipboard();
    void PasteClipboardStates();
    void SelectStatesByName(const std::vector<std::string>& stateNames);
    std::vector<std::string> GetSelectedStateNames() const;
    void RestoreSelectionAfterHistory(const std::vector<std::string>& stateNames);
    void RefreshSelectedEventEditor();
    void HandleKeyboardShortcuts();
    void RefreshSelectedTransitionEditor();
    int FindStateIndexByName(const std::string& name) const;
    int FindEventIndexByState(const std::string& stateName) const;
    std::string MakeUniqueStateName(const std::string& baseName) const;

    std::unique_ptr<AnimationGraphDocument> m_document;
    std::string m_currentPath;
    std::string m_canvasStateJson;
    bool m_canvasStateLoaded;
    char m_bankPathBuffer[512];
    char m_stateNameBuffer[128];
    char m_animationNameBuffer[128];
    char m_transitionFromBuffer[128];
    char m_transitionToBuffer[128];
    char m_eventNameBuffer[128];
    char m_eventStateBuffer[128];
    char m_eventPayloadBuffer[256];
    float m_transitionTimeBuffer;
    float m_eventTimeBuffer;
    bool m_minimapVisible;
    float m_minimapSize;
    int m_minimapPosition;
    bool m_gridVisible;
    bool m_showVerification;
    bool m_showRunPreview;
    float m_previewTime;
    float m_previewDuration;
    float m_previousPreviewTime;
    std::string m_previewStateName;
    std::vector<std::string> m_previewEventLog;
    float m_rightPanelWidth;
    float m_timelinePanelHeight;
    int m_draggedStateIndex;
    ImVec2 m_dragStartPos;
    std::string m_draggedStateName;
    struct StatePosition
    {
        std::string name;
        ImVec2 position;
    };
    std::vector<StatePosition> m_draggedStateStartPositions;
    int m_selectedStateIndex;
    int m_selectedTransitionIndex;
    int m_contextTransitionIndex;
    int m_selectedEventIndex;
    std::vector<int> m_selectedStateIndices;
    int m_linkStartStateIndex;
    bool m_hasPendingTransitionDrag;
    int m_transitionFromIndex;
    ImVec2 m_linkDragStartPos;
    ImVec2 m_transitionPreviewEnd;
    bool m_isSelectingRectangle = false;
    ImVec2 m_selectionRectStart = ImVec2(0.0f, 0.0f);
    ImVec2 m_selectionRectEnd = ImVec2(0.0f, 0.0f);
    // Shared canvas context routing.  A right click has exactly one target:
    // state, transition, or empty canvas (same contract as Placeholder).
    // Never share ImGui popup IDs with other graph renderers.  Animation
    // Graph lives inside a nested split layout and therefore needs an
    // explicit identity even when another editor is alive in the tab host.
    CanvasContextMenu m_canvasContextMenu{"##AnimationGraphCanvasContextMenu"};
    // A renderer may only emit its interactive canvas once per ImGui frame.
    // This defensive guard exposes/prevents a host-level double render from
    // opening two independent context popup stacks.
    int m_lastGraphCanvasFrame = -1;
    int m_pasteSequence = 0;
    std::vector<std::string> m_logs;

    std::unique_ptr<CanvasFramework> m_framework;
    std::unique_ptr<CustomCanvasEditor> m_canvasEditor;
};

} // namespace Olympe
