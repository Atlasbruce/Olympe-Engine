/**
 * @file BTGraphCommands.h
 * @brief Concrete command implementations for BehaviorTree operations
 * @author Olympe Engine
 * @details C++14 compliant undo/redo commands
 */

#pragma once

#include "GraphCommand.h"
#include <string>
#include <vector>
#include <map>

namespace Olympe
{
    // Forward declarations
    struct ClipboardNode;
    enum class NodeType;
    struct GraphNode;
    class NodeGraph;

    /**
     * @class CopyNodesCommand
     * @brief Command to copy selected nodes to clipboard
     */
    class CopyNodesCommand : public GraphCommand
    {
    public:
        CopyNodesCommand(NodeGraph* graph, const std::vector<int>& nodeIds);
        virtual ~CopyNodesCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        NodeGraph* m_graph;
        std::vector<int> m_nodeIds;
        std::vector<ClipboardNode> m_clipboardData;
    };

    /**
     * @class PasteNodesCommand
     * @brief Command to paste nodes from clipboard with offset
     */
    class PasteNodesCommand : public GraphCommand
    {
    public:
        PasteNodesCommand(NodeGraph* graph, float offsetX = 30.0f, float offsetY = 30.0f);
        virtual ~PasteNodesCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        NodeGraph* m_graph;
        float m_offsetX, m_offsetY;
        std::vector<int> m_pastedNodeIds;  // IDs of newly created nodes
        std::map<int, int> m_idMapping;   // Original ID -> New ID mapping
    };

    /**
     * @class DuplicateNodeCommand
     * @brief Command to duplicate selected nodes (copy + paste in one action)
     */
    class DuplicateNodeCommand : public GraphCommand
    {
    public:
        DuplicateNodeCommand(NodeGraph* graph, const std::vector<int>& nodeIds, float offsetX = 30.0f, float offsetY = 30.0f);
        virtual ~DuplicateNodeCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        NodeGraph* m_graph;
        std::vector<int> m_originalNodeIds;
        std::vector<int> m_duplicatedNodeIds;  // IDs of newly created nodes
        float m_offsetX, m_offsetY;
    };
}
