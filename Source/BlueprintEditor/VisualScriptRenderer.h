/**
 * @file VisualScriptRenderer.h
 * @brief IGraphRenderer adapter that wraps VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 */

#pragma once

#include "IGraphRenderer.h"
#include "VisualScriptEditorPanel.h"

#include <string>

namespace Olympe {

/**
 * @class VisualScriptRenderer
 * @brief Adapts the existing VisualScriptEditorPanel to the IGraphRenderer interface.
 */
class VisualScriptRenderer : public IGraphRenderer
{
public:
    VisualScriptRenderer();
    ~VisualScriptRenderer();

    void        Render()                         override;
    bool        Load(const std::string& path)    override;
    bool        Save(const std::string& path)    override;
    bool        IsDirty()              const     override;
    std::string GetGraphType()         const     override;
    std::string GetCurrentPath()       const     override;

    /** @brief Direct access to the wrapped panel (for advanced operations). */
    VisualScriptEditorPanel& GetPanel() { return m_panel; }

private:
    VisualScriptEditorPanel m_panel;
};

} // namespace Olympe
