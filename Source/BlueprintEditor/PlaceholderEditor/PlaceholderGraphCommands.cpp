#include "PlaceholderGraphCommands.h"
#include <algorithm>

namespace Olympe
{
    CreatePlaceholderNodeCommand::CreatePlaceholderNodeCommand(PlaceholderGraphDocument* document,
        PlaceholderNodeType type, const std::string& title, float x, float y)
        : m_document(document), m_type(type), m_title(title), m_x(x), m_y(y)
    {
    }

    bool CreatePlaceholderNodeCommand::Execute()
    {
        if (!m_document) return false;
        if (m_hasCreatedNode) return m_document->RestoreNode(m_node);

        const int nodeId = m_document->CreateNode(m_type, m_title, m_x, m_y);
        PlaceholderNode* node = m_document->GetNode(nodeId);
        if (!node) return false;
        m_node = *node;
        m_hasCreatedNode = true;
        return true;
    }

    bool CreatePlaceholderNodeCommand::Undo()
    {
        return m_document && m_hasCreatedNode && m_document->DeleteNode(m_node.nodeId);
    }

    DeletePlaceholderNodesCommand::DeletePlaceholderNodesCommand(
        PlaceholderGraphDocument* document, const std::vector<int>& nodeIds)
        : m_document(document)
    {
        if (!m_document) return;
        for (int nodeId : nodeIds) {
            PlaceholderNode* node = m_document->GetNode(nodeId);
            if (node) m_nodes.push_back(*node);
        }
        const auto& allConnections = m_document->GetAllConnections();
        for (const PlaceholderConnection& connection : allConnections) {
            if (std::find(nodeIds.begin(), nodeIds.end(), connection.fromNodeId) != nodeIds.end() ||
                std::find(nodeIds.begin(), nodeIds.end(), connection.toNodeId) != nodeIds.end()) {
                m_connections.push_back(connection);
            }
        }
    }

    bool DeletePlaceholderNodesCommand::Execute()
    {
        if (!m_document || m_nodes.empty()) return false;
        bool deletedAny = false;
        for (const PlaceholderNode& node : m_nodes) {
            deletedAny = m_document->DeleteNode(node.nodeId) || deletedAny;
        }
        return deletedAny;
    }

    bool DeletePlaceholderNodesCommand::Undo()
    {
        if (!m_document) return false;
        bool restoredAny = false;
        for (const PlaceholderNode& node : m_nodes) {
            restoredAny = m_document->RestoreNode(node) || restoredAny;
        }
        for (const PlaceholderConnection& connection : m_connections) {
            m_document->RestoreConnection(connection);
        }
        return restoredAny;
    }

    CreatePlaceholderConnectionCommand::CreatePlaceholderConnectionCommand(
        PlaceholderGraphDocument* document, const PlaceholderConnection& connection)
        : m_document(document), m_connection(connection)
    {
    }

    bool CreatePlaceholderConnectionCommand::Execute()
    {
        return m_document && m_document->RestoreConnection(m_connection);
    }

    bool CreatePlaceholderConnectionCommand::Undo()
    {
        return m_document && m_document->DeleteConnection(m_connection.fromNodeId, m_connection.toNodeId);
    }

    DeletePlaceholderConnectionCommand::DeletePlaceholderConnectionCommand(
        PlaceholderGraphDocument* document, const PlaceholderConnection& connection)
        : m_document(document), m_connection(connection)
    {
    }

    bool DeletePlaceholderConnectionCommand::Execute()
    {
        return m_document && m_document->DeleteConnection(m_connection.fromNodeId, m_connection.toNodeId);
    }

    bool DeletePlaceholderConnectionCommand::Undo()
    {
        return m_document && m_document->RestoreConnection(m_connection);
    }

    MovePlaceholderNodesCommand::MovePlaceholderNodesCommand(PlaceholderGraphDocument* document,
        const std::vector<int>& nodeIds, float deltaX, float deltaY)
        : m_document(document), m_nodeIds(nodeIds), m_deltaX(deltaX), m_deltaY(deltaY)
    {
    }

    bool MovePlaceholderNodesCommand::Execute()
    {
        if (!m_document || m_nodeIds.empty()) return false;
        if (m_hasExecuted) {
            for (const Position& position : m_after) {
                m_document->SetNodePosition(position.nodeId, position.x, position.y);
            }
            return !m_after.empty();
        }

        for (int nodeId : m_nodeIds) {
            PlaceholderNode* node = m_document->GetNode(nodeId);
            if (!node) continue;
            m_before.push_back({ nodeId, node->posX, node->posY });
            m_after.push_back({ nodeId, node->posX + m_deltaX, node->posY + m_deltaY });
        }
        for (const Position& position : m_after) {
            m_document->SetNodePosition(position.nodeId, position.x, position.y);
        }
        m_hasExecuted = !m_after.empty();
        return m_hasExecuted;
    }

    bool MovePlaceholderNodesCommand::Undo()
    {
        if (!m_document || !m_hasExecuted) return false;
        for (const Position& position : m_before) {
            m_document->SetNodePosition(position.nodeId, position.x, position.y);
        }
        return !m_before.empty();
    }

    bool MovePlaceholderNodesCommand::CanMergeWith(const GraphCommand& other) const
    {
        const MovePlaceholderNodesCommand* move = dynamic_cast<const MovePlaceholderNodesCommand*>(&other);
        return move && m_document == move->m_document && m_nodeIds == move->m_nodeIds;
    }

    bool MovePlaceholderNodesCommand::MergeWith(const GraphCommand& other)
    {
        const MovePlaceholderNodesCommand* move = dynamic_cast<const MovePlaceholderNodesCommand*>(&other);
        if (!move || !CanMergeWith(other)) return false;
        m_after = move->m_after;
        return true;
    }
}
