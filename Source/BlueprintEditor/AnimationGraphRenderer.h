#pragma once

#include "Framework/IGraphDocument.h"
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"
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
    void RenderStateEditorPanel();
    void RenderTransitionEditorPanel();
    void RenderTimelinePanel();
    void RenderGraphCanvas();
    void RenderRightPanel();
    void RenderRightPanelTabs();
    float DistanceToCubicBezier(const ImVec2& p, const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3) const;
    void EnsureStatePositions();
    void RenderGraphNodePorts(const nlohmann::json& state, int stateIndex, bool isSelected);
    int FindStateIndexByName(const std::string& name) const;
    int FindEventIndexByState(const std::string& stateName) const;
    std::string MakeUniqueStateName(const std::string& baseName) const;

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
    int m_linkStartStateIndex;
    // Transition drag state (Phase 52 fix for pin-based connection creation)
    bool m_hasPendingTransitionDrag;
    int m_transitionFromIndex;
    ImVec2 m_linkDragStartPos;
    std::vector<std::string> m_logs;

};

} // namespace Olympe
