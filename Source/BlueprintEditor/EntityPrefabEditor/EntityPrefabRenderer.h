#pragma once

#include "../IGraphRenderer.h"
#include "PrefabCanvas.h"
#include <memory>
#include <string>

namespace Olympe {

/**
 * @class EntityPrefabRenderer
 * @brief Renderer adapter for Entity Prefab graphs.
 * 
 * Implements IGraphRenderer interface for the TabManager,
 * delegating rendering to PrefabCanvas.
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
    std::string m_filePath;
    bool m_isDirty = false;
};

} // namespace Olympe
