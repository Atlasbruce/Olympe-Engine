#pragma once

#include "../Commands/GraphCommand.h"
#include "PlaceholderGraphDocument.h"
#include <vector>

namespace Olympe
{
    class CreatePlaceholderNodeCommand : public GraphCommand
    {
    public:
        CreatePlaceholderNodeCommand(PlaceholderGraphDocument* document, PlaceholderNodeType type,
            const std::string& title, float x, float y);
        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override { return "Create node"; }
        int GetNodeId() const { return m_node.nodeId; }

    private:
        PlaceholderGraphDocument* m_document;
        PlaceholderNodeType m_type;
        std::string m_title;
        float m_x;
        float m_y;
        bool m_hasCreatedNode = false;
        PlaceholderNode m_node;
    };

    class DeletePlaceholderNodesCommand : public GraphCommand
    {
    public:
        DeletePlaceholderNodesCommand(PlaceholderGraphDocument* document, const std::vector<int>& nodeIds);
        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override { return "Delete node(s)"; }

    private:
        PlaceholderGraphDocument* m_document;
        std::vector<PlaceholderNode> m_nodes;
        std::vector<PlaceholderConnection> m_connections;
    };

    class CreatePlaceholderConnectionCommand : public GraphCommand
    {
    public:
        CreatePlaceholderConnectionCommand(PlaceholderGraphDocument* document, const PlaceholderConnection& connection);
        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override { return "Create connection"; }

    private:
        PlaceholderGraphDocument* m_document;
        PlaceholderConnection m_connection;
    };

    class DeletePlaceholderConnectionCommand : public GraphCommand
    {
    public:
        DeletePlaceholderConnectionCommand(PlaceholderGraphDocument* document, const PlaceholderConnection& connection);
        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override { return "Delete connection"; }

    private:
        PlaceholderGraphDocument* m_document;
        PlaceholderConnection m_connection;
    };

    class MovePlaceholderNodesCommand : public GraphCommand
    {
    public:
        MovePlaceholderNodesCommand(PlaceholderGraphDocument* document, const std::vector<int>& nodeIds,
            float deltaX, float deltaY);
        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override { return "Move node(s)"; }
        bool CanMergeWith(const GraphCommand& other) const override;
        bool MergeWith(const GraphCommand& other) override;

    private:
        struct Position { int nodeId; float x; float y; };
        PlaceholderGraphDocument* m_document;
        std::vector<int> m_nodeIds;
        float m_deltaX;
        float m_deltaY;
        bool m_hasExecuted = false;
        std::vector<Position> m_before;
        std::vector<Position> m_after;
    };
}
