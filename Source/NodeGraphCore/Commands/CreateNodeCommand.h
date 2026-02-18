/**
 * @file CreateNodeCommand.h
 * @brief Command for creating a node
 * @author Olympe Engine
 * @date 2026-02-18
 */

#pragma once

#include "../CommandSystem.h"
#include "../GraphDocument.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class CreateNodeCommand
 * @brief Creates a new node in the graph
 */
class CreateNodeCommand : public ICommand {
public:
    CreateNodeCommand(GraphDocument* graph, const std::string& nodeType, Vector2 position);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;
    
private:
    GraphDocument* m_graph;
    std::string m_nodeType;
    Vector2 m_position;
    NodeId m_createdNodeId;
};

} // namespace NodeGraph
} // namespace Olympe
