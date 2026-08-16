#pragma once

#include "Framework/IGraphDocument.h"
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Framework/CanvasModalRenderer.h"
#include "AnimationGraphCanvas.h"
#include "../third_party/imgui/imgui.h"
#include <memory>
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
    void SaveCanvasState() override {}
    void RestoreCanvasState() override {}
    void VerifyGraph() override;
    void RunGraph() override;
    bool SupportsVerification() const override { return true; }
    bool SupportsExecution() const override { return true; }
    bool SupportsMinimap() const override { return true; }
    bool IsMinimapVisible() const override { return m_minimapVisible; }
    void SetMinimapVisible(bool visible) override { m_minimapVisible = visible; }
    float GetMinimapSize() const override { return m_minimapSize; }
    void SetMinimapSize(float size) override;
    int GetMinimapPosition() const override { return m_minimapPosition; }
    void SetMinimapPosition(int pos) override;
    void RenderFrameworkModals() override;

    IGraphDocument* GetDocument() const { return static_cast<IGraphDocument*>(m_document.get()); }

private:
    void EnsureDocument();
    void RenderToolbar();
    void RenderMainPanel();
    void RenderInspectorPanel();
    void RenderVerificationPanel();
    void RenderBindingPanel();
    void RenderStateEditorPanel();
    void RenderTransitionEditorPanel();
    void RenderPropertiesPanel();
    void RenderNodesPanel();
    void RenderRightPanelTabs();
    void RenderTimelinePanel();
    void RenderGraphCanvas();
    void RenderRightPanel();
    void RenderCanvasContextMenu();
    void RenderPaletteDragSource(const std::string& clipName);
    void AddStateNodeFromPalette(const std::string& clipName, const ImVec2& canvasPos);
    void UpdateCanvasLayoutFromCurrentWindow();
    std::string BuildBrowserSelectionPath(const std::string& ext) const;
    float DistanceToCubicBezier(const ImVec2& p, const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3) const;
    void EnsureStatePositions();
    std::string BuildRuntimeExportPath() const;
    int FindStateIndexByName(const std::string& name) const;
    bool IsPointInsideState(float x, float y, size_t stateIndex) const;
    int FindEventIndexByState(const std::string& stateName) const;
    std::string MakeUniqueStateName(const std::string& baseName) const;
    void RenderNodePalette();
    std::unique_ptr<AnimationGraphCanvas> m_canvas;
    std::unique_ptr<ICanvasEditor> m_canvasEditor;
    ImVec2 m_canvasScreenPos;
    bool m_showGrid;
    bool m_showContextMenu;
    bool m_pendingPaletteDrop;
    std::string m_pendingDropClip;
    std::unique_ptr<AnimationGraphDocument> m_document;
    std::string m_currentPath;
    char m_bankPathBuffer[512];
    char m_stateNameBuffer[128];
    char m_animationNameBuffer[128];
    char m_transitionFromBuffer[128];
    char m_transitionToBuffer[128];
    char m_eventNameBuffer[128];
    char m_eventStateBuffer[128];
    float m_transitionTimeBuffer;
    float m_eventTimeBuffer;
    bool m_minimapVisible;
    float m_minimapSize;
    int m_minimapPosition;
    bool m_showVerification;
    bool m_showRunPreview;
    float m_previewTime;
    float m_previewDuration;
    float m_rightPanelWidth;
    float m_timelinePanelHeight;
    int m_draggedStateIndex;
    ImVec2 m_dragStartPos;
    int m_selectedStateIndex;
    int m_selectedTransitionIndex;
    int m_selectedEventIndex;
    std::vector<std::string> m_logs;
    bool m_pendingLoadBank;
    bool m_pendingLoadPrefab;
    bool m_pendingLoadGraph;
    int m_rightPanelTab;
};

} // namespace Olympe
