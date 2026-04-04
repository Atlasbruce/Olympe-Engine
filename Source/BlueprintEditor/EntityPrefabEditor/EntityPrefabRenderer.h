#pragma once

#include "../IGraphRenderer.h"
#include "PrefabCanvas.h"
#include "ComponentPalettePanel.h"
#include "PropertyEditorPanel.h"
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

private:
    PrefabCanvas& m_canvas;
    ComponentPalettePanel m_componentPalette;
    PropertyEditorPanel m_propertyEditor;
    std::string m_filePath;
    bool m_isDirty = false;
    float m_canvasPanelWidth = 0.75f; // 75% for canvas, 25% for side panel
    int m_rightPanelTabSelection = 0; // 0 = ComponentPalette, 1 = PropertyPanel

    // Rendering helpers
    void RenderLayoutWithTabs();
    void RenderRightPanelTabs();
};

} // namespace Olympe
