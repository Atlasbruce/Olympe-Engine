/**
 * @file BTGraphCommands.cpp
 * @brief Implementation of BehaviorTree command operations
 * @author Olympe Engine
 */

#include "BTGraphCommands.h"
#include "../BTNodeGraphManager.h"
#include <algorithm>

namespace Olympe
{
    // ============ CopyNodesCommand ============
    CopyNodesCommand::CopyNodesCommand(NodeGraph* graph, const std::vector<int>& nodeIds)
        : m_graph(graph), m_nodeIds(nodeIds)
    {
    }

    bool CopyNodesCommand::Execute()
    {
        if (!m_graph)
            return false;

        m_graph->CopyNodesToClipboard(m_nodeIds);
        return true;
    }

    bool CopyNodesCommand::Undo()
    {
        // Copy operations are not undoable (they don't modify the graph)
        return true;
    }

    std::string CopyNodesCommand::GetDescription() const
    {
        return "Copy " + std::to_string(m_nodeIds.size()) + " node(s)";
    }

    // ============ PasteNodesCommand ============
    PasteNodesCommand::PasteNodesCommand(NodeGraph* graph, float offsetX, float offsetY)
        : m_graph(graph), m_offsetX(offsetX), m_offsetY(offsetY)
    {
    }

    bool PasteNodesCommand::Execute()
    {
        if (!m_graph)
            return false;

        m_pastedNodeIds = m_graph->PasteNodesFromClipboard(m_offsetX, m_offsetY);
        return !m_pastedNodeIds.empty();
    }

    bool PasteNodesCommand::Undo()
    {
        if (!m_graph || m_pastedNodeIds.empty())
            return false;

        // Delete all pasted nodes
        for (int nodeId : m_pastedNodeIds)
        {
            m_graph->DeleteNode(nodeId);
        }

        return true;
    }

    std::string PasteNodesCommand::GetDescription() const
    {
        return "Paste " + std::to_string(m_pastedNodeIds.size()) + " node(s)";
    }

    // ============ DuplicateNodeCommand ============
    DuplicateNodeCommand::DuplicateNodeCommand(NodeGraph* graph, const std::vector<int>& nodeIds, float offsetX, float offsetY)
        : m_graph(graph), m_originalNodeIds(nodeIds), m_offsetX(offsetX), m_offsetY(offsetY)
    {
    }

    bool DuplicateNodeCommand::Execute()
    {
        if (!m_graph || m_originalNodeIds.empty())
            return false;

        m_duplicatedNodeIds = m_graph->DuplicateNodes(m_originalNodeIds, m_offsetX, m_offsetY);
        return !m_duplicatedNodeIds.empty();
    }

    bool DuplicateNodeCommand::Undo()
    {
        if (!m_graph || m_duplicatedNodeIds.empty())
            return false;

        // Delete all duplicated nodes
        for (int nodeId : m_duplicatedNodeIds)
        {
            m_graph->DeleteNode(nodeId);
        }

        return true;
    }

    std::string DuplicateNodeCommand::GetDescription() const
    {
        return "Duplicate " + std::to_string(m_originalNodeIds.size()) + " node(s)";
    }
}
