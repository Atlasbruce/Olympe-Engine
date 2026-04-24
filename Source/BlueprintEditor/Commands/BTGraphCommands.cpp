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
    // TODO: Phase 50.4 - Reimplement with modern GraphDocument
    CopyNodesCommand::CopyNodesCommand(void* graph, const std::vector<int>& nodeIds)
    {
        // Deprecated - NodeGraph class removed in Phase 50.3 namespace collision fix
    }

    bool CopyNodesCommand::Execute()
    {
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        // Legacy NodeGraph class removed - functionality disabled until reimplemented
        if (!m_graph)
            return false;

        // m_graph->CopyNodesToClipboard(m_nodeIds);  // ← Legacy code, disabled
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
    // TODO: Phase 50.4 - Reimplement with modern GraphDocument
    PasteNodesCommand::PasteNodesCommand(void* graph, float offsetX, float offsetY)
    {
        // Deprecated - NodeGraph class removed in Phase 50.3 namespace collision fix
    }

    bool PasteNodesCommand::Execute()
    {
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        // Legacy NodeGraph class removed - functionality disabled until reimplemented
        if (!m_graph)
            return false;

        // m_pastedNodeIds = m_graph->PasteNodesFromClipboard(m_offsetX, m_offsetY);  // ← Legacy code, disabled
        return !m_pastedNodeIds.empty();
    }

    bool PasteNodesCommand::Undo()
    {
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        // Legacy NodeGraph class removed - functionality disabled until reimplemented
        if (!m_graph || m_pastedNodeIds.empty())
            return false;

        // Delete all pasted nodes
        for (int nodeId : m_pastedNodeIds)
        {
            // m_graph->DeleteNode(nodeId);  // ← Legacy code, disabled
        }

        return true;
    }

    std::string PasteNodesCommand::GetDescription() const
    {
        return "Paste " + std::to_string(m_pastedNodeIds.size()) + " node(s)";
    }

    // ============ DuplicateNodeCommand ============
    DuplicateNodeCommand::DuplicateNodeCommand(void* graph, const std::vector<int>& nodeIds, float offsetX, float offsetY)
    {
    }

    bool DuplicateNodeCommand::Execute()
    {
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        // Legacy NodeGraph class removed - functionality disabled until reimplemented
        if (!m_graph || m_originalNodeIds.empty())
            return false;

        // m_duplicatedNodeIds = m_graph->DuplicateNodes(m_originalNodeIds, m_offsetX, m_offsetY);  // ← Legacy code, disabled
        return !m_duplicatedNodeIds.empty();
    }

    bool DuplicateNodeCommand::Undo()
    {
        // TODO: Phase 50.4 - Reimplement with modern GraphDocument
        // Legacy NodeGraph class removed - functionality disabled until reimplemented
        if (!m_graph || m_duplicatedNodeIds.empty())
            return false;

        // Delete all duplicated nodes
        for (int nodeId : m_duplicatedNodeIds)
        {
            // m_graph->DeleteNode(nodeId);  // ← Legacy code, disabled
        }

        return true;
    }

    std::string DuplicateNodeCommand::GetDescription() const
    {
        return "Duplicate " + std::to_string(m_originalNodeIds.size()) + " node(s)";
    }
}
