/**
 * @file BehaviorTreeRenderer.cpp
 * @brief IGraphRenderer adapter for BehaviorTree graphs.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details C++14 compliant.
 */

#include "BehaviorTreeRenderer.h"

#include "BTNodeGraphManager.h"
#include "../system/system_utils.h"

#include <iostream>

namespace Olympe {

BehaviorTreeRenderer::BehaviorTreeRenderer(NodeGraphPanel& panel)
    : m_panel(panel)
    , m_graphId(-1)
{
}

BehaviorTreeRenderer::~BehaviorTreeRenderer()
{
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_graphId);
        m_graphId = -1;
    }
}

void BehaviorTreeRenderer::Render()
{
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().SetActiveGraph(m_graphId);
    }
    m_panel.RenderContent();
}

bool BehaviorTreeRenderer::Load(const std::string& path)
{
    if (path.empty())
        return false;

    // Close the previously loaded graph, if any.
    if (m_graphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_graphId);
        m_graphId = -1;
    }

    int newId = NodeGraphManager::Get().LoadGraph(path);
    if (newId < 0)
    {
        SYSTEM_LOG << "[BehaviorTreeRenderer] Failed to load graph: " << path << "\n";
        return false;
    }

    m_graphId  = newId;
    m_filePath = path;
    NodeGraphManager::Get().SetActiveGraph(m_graphId);
    SYSTEM_LOG << "[BehaviorTreeRenderer] Loaded BT graph: " << path
               << " (id=" << m_graphId << ")\n";
    return true;
}

bool BehaviorTreeRenderer::Save(const std::string& path)
{
    if (m_graphId < 0)
        return false;

    const std::string savePath = path.empty() ? m_filePath : path;
    if (savePath.empty())
        return false;

    bool ok = NodeGraphManager::Get().SaveGraph(m_graphId, savePath);
    if (ok && !path.empty())
        m_filePath = path;
    return ok;
}

bool BehaviorTreeRenderer::IsDirty() const
{
    if (m_graphId < 0)
        return false;

    const NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_graphId);
    return (graph != nullptr) && graph->IsDirty();
}

std::string BehaviorTreeRenderer::GetGraphType() const
{
    return "BehaviorTree";
}

std::string BehaviorTreeRenderer::GetCurrentPath() const
{
    return m_filePath;
}

} // namespace Olympe
