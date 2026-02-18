/**
 * @file DeleteNodeCommand.cpp
 * @brief Implementation of DeleteNodeCommand
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "DeleteNodeCommand.h"

namespace Olympe {
namespace NodeGraph {

DeleteNodeCommand::DeleteNodeCommand(GraphDocument* graph, NodeId nodeId)
    : m_graph(graph)
    , m_nodeId(nodeId)
    , m_wasExecuted(false)
{
}

void DeleteNodeCommand::Execute()
{
    if (m_graph == nullptr)
        return;
    
    // Save node data before deleting
    const NodeData* node = m_graph->GetNode(m_nodeId);
    if (node != nullptr)
    {
        m_savedNode = *node;
        
        // Save connected links
        m_savedLinks.clear();
        for (const auto& link : m_graph->GetLinks())
        {
            // For now, save all links (we'd need to check pin ownership)
            m_savedLinks.push_back(link);
        }
        
        m_graph->DeleteNode(m_nodeId);
        m_wasExecuted = true;
    }
}

void DeleteNodeCommand::Undo()
{
    if (m_graph == nullptr || !m_wasExecuted)
        return;
    
    // Restore the node
    m_graph->GetNodesRef().push_back(m_savedNode);
    
    // Restore links if needed
    // (This is simplified; in practice we'd need better link management)
}

std::string DeleteNodeCommand::GetDescription() const
{
    return "Delete Node";
}

} // namespace NodeGraph
} // namespace Olympe
