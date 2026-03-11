/**
 * @file BehaviorTreeRenderer.h
 * @brief IGraphRenderer adapter for BehaviorTree graphs (wraps BTNodeGraphManager).
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 */

#pragma once

#include "IGraphRenderer.h"
#include "NodeGraphPanel.h"

#include <string>

namespace Olympe {

/**
 * @class BehaviorTreeRenderer
 * @brief Adapts the BTNodeGraphManager + NodeGraphPanel to IGraphRenderer.
 *
 * Each instance manages a single BT graph ID in BTNodeGraphManager.
 * When Render() is called, this renderer sets that graph as the active one and
 * delegates to a shared NodeGraphPanel reference for drawing.
 */
class BehaviorTreeRenderer : public IGraphRenderer
{
public:
    /**
     * @param panel  Reference to the shared NodeGraphPanel owned by the editor GUI.
     *               The renderer does NOT take ownership.
     */
    explicit BehaviorTreeRenderer(NodeGraphPanel& panel);
    ~BehaviorTreeRenderer();

    void        Render()                         override;
    bool        Load(const std::string& path)    override;
    bool        Save(const std::string& path)    override;
    bool        IsDirty()              const     override;
    std::string GetGraphType()         const     override;
    std::string GetCurrentPath()       const     override;

private:
    NodeGraphPanel& m_panel;     ///< Shared panel reference (not owned)
    int             m_graphId;   ///< ID in BTNodeGraphManager; -1 if not loaded
    std::string     m_filePath;  ///< Path that was loaded
};

} // namespace Olympe
