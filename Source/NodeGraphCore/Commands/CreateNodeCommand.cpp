/**
 * @file CreateNodeCommand.cpp
 * @brief Implementation of CreateNodeCommand
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "CreateNodeCommand.h"

namespace Olympe {
namespace NodeGraph {

CreateNodeCommand::CreateNodeCommand(GraphDocument* graph, const std::string& nodeType, Vector2 position)
    : m_graph(graph)
    , m_nodeType(nodeType)
    , m_position(position)
{
}

void CreateNodeCommand::Execute()
{
    if (m_graph != nullptr)
    {
        m_createdNodeId = m_graph->CreateNode(m_nodeType, m_position);
    }
}

void CreateNodeCommand::Undo()
{
    if (m_graph != nullptr && m_createdNodeId.value != 0)
    {
        m_graph->DeleteNode(m_createdNodeId);
    }
}

std::string CreateNodeCommand::GetDescription() const
{
    return "Create " + m_nodeType;
}

} // namespace NodeGraph
} // namespace Olympe
