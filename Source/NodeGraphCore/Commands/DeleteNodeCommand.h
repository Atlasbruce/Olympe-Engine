/**
 * @file DeleteNodeCommand.h
 * @brief Command for deleting a node
 * @author Olympe Engine
 * @date 2026-02-18
 */

#pragma once

#include "../CommandSystem.h"
#include "../GraphDocument.h"

namespace Olympe {
namespace NodeGraph {

/**
 * @class DeleteNodeCommand
 * @brief Deletes a node from the graph
 */
class DeleteNodeCommand : public ICommand {
public:
    DeleteNodeCommand(GraphDocument* graph, NodeId nodeId);
    
    void Execute() override;
    void Undo() override;
    std::string GetDescription() const override;
    
private:
    GraphDocument* m_graph;
    NodeId m_nodeId;
    NodeData m_savedNode;
    std::vector<LinkData> m_savedLinks;
    bool m_wasExecuted = false;
};

} // namespace NodeGraph
} // namespace Olympe
