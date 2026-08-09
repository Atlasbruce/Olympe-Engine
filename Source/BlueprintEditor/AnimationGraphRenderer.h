#pragma once

#include "Framework/IGraphDocument.h"
#include "IGraphRenderer.h"
#include "AnimationGraphDocument.h"
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

    std::unique_ptr<AnimationGraphDocument> m_document;
    std::string m_currentPath;
    char m_bankPathBuffer[512];
    char m_stateNameBuffer[128];
    char m_animationNameBuffer[128];
    bool m_minimapVisible;
    float m_minimapSize;
    int m_minimapPosition;
    bool m_showVerification;
    bool m_showRunPreview;
    std::vector<std::string> m_logs;
};

} // namespace Olympe
