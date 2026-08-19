#pragma once

#include "Framework/IGraphDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/AnimationGraphFrameworkDocument.h"  // Phase 55: Framework adapter (AnimationGraphFrameworkDocument)
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"  // Legacy data model
#include "Utilities/CustomCanvasEditor.h"  // Phase 55: Zoom/pan support
#include "Utilities/CanvasMinimapRenderer.h"  // Phase 55: Minimap support
#include "../third_party/imgui/imgui.h"
#include <memory>
#include <string>
#include <vector>

namespace Olympe {

class AnimationGraphDocument;  // Legacy data model
class AnimationGraphFrameworkDocument;  // Framework adapter

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
    void RenderTransitionPreview();  // Phase 53: Render transition line preview during drag
    void RenderSingleTransition(size_t transitionIndex, const ImVec2& canvasOrigin);  // Phase 54: Render single transition with arrow
    void EnsureStatePositions();
    void RenderGraphNodePorts(const nlohmann::json& state, int stateIndex, bool isSelected);
    int FindStateIndexByName(const std::string& name) const;
    int FindEventIndexByState(const std::string& stateName) const;
    std::string MakeUniqueStateName(const std::string& baseName) const;

    std::unique_ptr<AnimationGraphDocument> m_document;
    std::string m_currentPath;
    std::string m_canvasStateJson;
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
    std::vector<int> m_selectedStateIndices;
    int m_linkStartStateIndex;
    // Transition drag state (Phase 52 fix for pin-based connection creation)
    bool m_hasPendingTransitionDrag;
    int m_transitionFromIndex;
    ImVec2 m_linkDragStartPos;
    ImVec2 m_transitionPreviewEnd;  // Phase 53: Current mouse position during transition drag for preview rendering
    bool m_isSelectingRectangle = false;
    ImVec2 m_selectionRectStart = ImVec2(0.0f, 0.0f);
    ImVec2 m_selectionRectEnd = ImVec2(0.0f, 0.0f);
    std::vector<std::string> m_logs;

    // Phase 55: Framework integration for unified toolbar/modals
    std::unique_ptr<AnimationGraphFrameworkDocument> m_frameworkDocument;  // IGraphDocument adapter
    std::unique_ptr<CanvasFramework> m_framework;  // Framework for toolbar/modals

    // Phase 55: Canvas interaction
    std::unique_ptr<CustomCanvasEditor> m_canvasEditor;  // Zoom/pan support

};

} // namespace Olympe
