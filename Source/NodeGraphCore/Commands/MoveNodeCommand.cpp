/**
 * @file MoveNodeCommand.cpp
 * @brief Implementation of MoveNodeCommand
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "MoveNodeCommand.h"

namespace Olympe {
namespace NodeGraph {

MoveNodeCommand::MoveNodeCommand(GraphDocument* graph, NodeId nodeId, Vector2 oldPos, Vector2 newPos)
    : m_graph(graph)
    , m_nodeId(nodeId)
    , m_oldPosition(oldPos)
    , m_newPosition(newPos)
{
}

void MoveNodeCommand::Execute()
{
    if (m_graph != nullptr)
    {
        m_graph->UpdateNodePosition(m_nodeId, m_newPosition);
    }
}

void MoveNodeCommand::Undo()
{
    if (m_graph != nullptr)
    {
        m_graph->UpdateNodePosition(m_nodeId, m_oldPosition);
    }
}

std::string MoveNodeCommand::GetDescription() const
{
    return "Move Node";
}

} // namespace NodeGraph
} // namespace Olympe
