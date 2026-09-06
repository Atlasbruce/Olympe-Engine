#pragma once

#include "Framework/IGraphDocument.h"
#include "Framework/CanvasFramework.h"
#include "Framework/AnimationGraphFrameworkDocument.h"
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"
#include "Utilities/CustomCanvasEditor.h"
#include "Utilities/CanvasMinimapRenderer.h"
#include "../third_party/imgui/imgui.h"
#include <memory>
#include <string>
#include <vector>

namespace Olympe {

class AnimationGraphDocument;
class AnimationGraphFrameworkDocument;

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
    void RenderFrameworkModals() override;

    IGraphDocument* GetDocument() const { return static_cast<IGraphDocument*>(m_document.get()); }

private:
    void EnsureDocument();
    void RenderToolbar();
    void RenderMainPanel();
    void RenderRightPanelTabs();
    void RenderCanvasPanel();
    void RenderEventSequencerPanel();
    void RenderAnimationPreviewPanel();
    void RenderTransitionEditorPanel();
    void RenderGraphCanvas();

    std::unique_ptr<AnimationGraphDocument> m_document;
    std::string m_currentPath;
    std::string m_canvasStateJson;
    char m_stateNameBuffer[128];
    char m_animationNameBuffer[128];
    char m_transitionFromBuffer[128];
    char m_transitionToBuffer[128];
    float m_transitionTimeBuffer;
    bool m_minimapVisible;
    float m_minimapSize;
    int m_minimapPosition;
    int m_selectedStateIndex;
    int m_selectedTransitionIndex;
    std::unique_ptr<AnimationGraphFrameworkDocument> m_frameworkDocument;
    std::unique_ptr<CanvasFramework> m_framework;
    std::unique_ptr<CustomCanvasEditor> m_canvasEditor;
};

} // namespace Olympe
