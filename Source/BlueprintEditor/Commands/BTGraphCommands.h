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
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        CopyNodesCommand(void* graph = nullptr, const std::vector<int>& nodeIds = std::vector<int>());
        virtual ~CopyNodesCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        void* m_graph;
        std::vector<int> m_nodeIds;
    };

    /**
     * @class PasteNodesCommand
     * @brief Command to paste nodes from clipboard with offset
     */
    class PasteNodesCommand : public GraphCommand
    {
    public:
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        PasteNodesCommand(void* graph = nullptr, float offsetX = 30.0f, float offsetY = 30.0f);
        virtual ~PasteNodesCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        void* m_graph;
        float m_offsetX, m_offsetY;
        std::vector<int> m_pastedNodeIds;
        std::map<int, int> m_idMapping;
    };

    /**
     * @class DuplicateNodeCommand
     * @brief Command to duplicate selected nodes (copy + paste in one action)
     */
    class DuplicateNodeCommand : public GraphCommand
    {
    public:
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        DuplicateNodeCommand(void* graph = nullptr, const std::vector<int>& nodeIds = std::vector<int>(), float offsetX = 30.0f, float offsetY = 30.0f);
        virtual ~DuplicateNodeCommand() = default;

        bool Execute() override;
        bool Undo() override;
        std::string GetDescription() const override;

    private:
        void* m_graph;
        std::vector<int> m_originalNodeIds;
        std::vector<int> m_duplicatedNodeIds;
        float m_offsetX, m_offsetY;
    };
}
