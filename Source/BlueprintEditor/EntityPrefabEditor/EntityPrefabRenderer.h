#pragma once

#include "../IGraphRenderer.h"
#include "PrefabCanvas.h"
#include "ComponentPalettePanel.h"
#include "PropertyEditorPanel.h"
#include "../Utilities/ICanvasEditor.h"
#include "../Framework/CanvasFramework.h"
#include "../../PanelManager.h"  // Access to framework panel dimensions
#include <memory>
#include <string>

namespace Olympe {

/**
 * @class EntityPrefabRenderer
 * @brief Renderer adapter for Entity Prefab graphs.
 * 
 * Implements IGraphRenderer interface for the TabManager,
 * delegating rendering to PrefabCanvas with integrated component palette and property panel.
 */
class EntityPrefabRenderer : public IGraphRenderer
{
public:
    explicit EntityPrefabRenderer(PrefabCanvas& canvas);
    ~EntityPrefabRenderer();

    void Render() override;
    bool Load(const std::string& path) override;
    bool Save(const std::string& path) override;
    bool IsDirty() const override;
    std::string GetGraphType() const override;
    std::string GetCurrentPath() const override;

    // Phase 35.0: Canvas state management
    void SaveCanvasState() override;
    void RestoreCanvasState() override;
    std::string GetCanvasStateJSON() const override;
    void SetCanvasStateJSON(const std::string& json) override;

    // Phase 43: Framework modal rendering (centralized toolbar Save/SaveAs/Browse)
    void RenderFrameworkModals() override;

private:
    PrefabCanvas& m_canvas;
    ComponentPalettePanel m_componentPalette;
    PropertyEditorPanel m_propertyEditor;
    std::unique_ptr<ICanvasEditor> m_canvasEditor;  // NEW: Standardized canvas interface
    std::unique_ptr<CanvasFramework> m_framework;   // NEW: Unified framework (Phase 41)
    std::string m_filePath;
    bool m_isDirty = false;
    // Layout: Right panel width uses PanelManager::InspectorPanelWidth (framework default: 300px)
    int m_rightPanelTabSelection = 0; // 0 = ComponentPalette, 1 = PropertyPanel

    // Canvas state snapshot (Phase 35.0)
    struct CanvasState
    {
        float panX;
        float panY;
        float zoom;
    } m_savedCanvasState;

    // Phase 37 — Minimap toolbar controls
    bool m_minimapVisible = true;
    float m_minimapSize = 0.15f;
    int m_minimapPosition = 1;  // 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight

    // Rendering methods
    void RenderToolbar();
    void RenderLayoutWithTabs();
    void RenderRightPanelTabs();
};

} // namespace Olympe
