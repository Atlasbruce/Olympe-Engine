/**
 * @file MoveNodeCommand.h
 * @brief Command for moving a node
 * @author Olympe Engine
 * @date 2026-02-18
 */

#pragma once

#include "../CommandSystem.h"
#include "../GraphDocument.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class MoveNodeCommand
 * @brief Moves a node to a new position
 */
class MoveNodeCommand : public ICommand {
public:
    MoveNodeCommand(GraphDocument* graph, NodeId nodeId, Vector2 oldPos, Vector2 newPos);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;
    
private:
    GraphDocument* m_graph;
    NodeId m_nodeId;
    Vector2 m_oldPosition;
    Vector2 m_newPosition;
};

} // namespace NodeGraph
} // namespace Olympe
