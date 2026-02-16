/**
 * @file BTEditorCommand.cpp
 * @brief Implementation of Command Pattern for BT Editor
 * @author Olympe Engine - Behavior Tree Editor
 * @date 2025
 */

#include "BTEditorCommand.h"
#include "../system/system_utils.h"
#include <algorithm>

namespace Olympe
{
    // ========================================================================
    // BTCommandStack Implementation
    // ========================================================================

    void BTCommandStack::Execute(BehaviorTreeAsset& tree, std::unique_ptr<BTEditorCommand> command)
    {
        if (!command)
            return;

        // Execute the command
        command->Execute(tree);

        // Add to undo stack
        m_undoStack.push_back(std::move(command));

        // Limit stack size
        if (m_undoStack.size() > MAX_STACK_SIZE)
        {
            m_undoStack.erase(m_undoStack.begin());
        }

        // Clear redo stack (new action invalidates redo history)
        m_redoStack.clear();
    }

    bool BTCommandStack::Undo(BehaviorTreeAsset& tree)
    {
        if (m_undoStack.empty())
            return false;

        // Move command from undo to redo stack
        std::unique_ptr<BTEditorCommand> command = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        // Undo the command
        command->Undo(tree);

        // Add to redo stack
        m_redoStack.push_back(std::move(command));

        // Limit redo stack size
        if (m_redoStack.size() > MAX_STACK_SIZE)
        {
            m_redoStack.erase(m_redoStack.begin());
        }

        return true;
    }

    bool BTCommandStack::Redo(BehaviorTreeAsset& tree)
    {
        if (m_redoStack.empty())
            return false;

        // Move command from redo to undo stack
        std::unique_ptr<BTEditorCommand> command = std::move(m_redoStack.back());
        m_redoStack.pop_back();

        // Re-execute the command
        command->Execute(tree);

        // Add back to undo stack
        m_undoStack.push_back(std::move(command));

        // Limit undo stack size
        if (m_undoStack.size() > MAX_STACK_SIZE)
        {
            m_undoStack.erase(m_undoStack.begin());
        }

        return true;
    }

    bool BTCommandStack::CanUndo() const
    {
        return !m_undoStack.empty();
    }

    bool BTCommandStack::CanRedo() const
    {
        return !m_redoStack.empty();
    }

    std::string BTCommandStack::GetUndoDescription() const
    {
        if (m_undoStack.empty())
            return "";
        return m_undoStack.back()->GetDescription();
    }

    std::string BTCommandStack::GetRedoDescription() const
    {
        if (m_redoStack.empty())
            return "";
        return m_redoStack.back()->GetDescription();
    }

    void BTCommandStack::Clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
    }

    // ========================================================================
    // AddNodeCommand Implementation
    // ========================================================================

    void AddNodeCommand::Execute(BehaviorTreeAsset& tree)
    {
        // Check if node already exists (prevent duplicate adds during redo)
        for (const auto& node : tree.nodes)
        {
            if (node.id == m_node.id)
                return; // Already exists
        }

        tree.nodes.push_back(m_node);
        SYSTEM_LOG << "[BTEditor] Added node: " << m_node.name << " (ID: " << m_node.id << ")\n";
    }

    void AddNodeCommand::Undo(BehaviorTreeAsset& tree)
    {
        // Remove the node
        auto it = std::find_if(tree.nodes.begin(), tree.nodes.end(),
            [this](const BTNode& n) { return n.id == m_node.id; });

        if (it != tree.nodes.end())
        {
            tree.nodes.erase(it);
            SYSTEM_LOG << "[BTEditor] Undone add node: " << m_node.name << " (ID: " << m_node.id << ")\n";
        }
    }

    std::string AddNodeCommand::GetDescription() const
    {
        return "Add " + m_node.name;
    }

    // ========================================================================
    // DeleteNodeCommand Implementation
    // ========================================================================

    void DeleteNodeCommand::Execute(BehaviorTreeAsset& tree)
    {
        // Find all parents that reference this node
        m_parentIds.clear();
        m_childIndices.clear();

        for (auto& parent : tree.nodes)
        {
            // Check childIds
            auto it = std::find(parent.childIds.begin(), parent.childIds.end(), m_node.id);
            if (it != parent.childIds.end())
            {
                int index = static_cast<int>(it - parent.childIds.begin());
                m_parentIds.push_back(parent.id);
                m_childIndices.push_back(index);
                parent.childIds.erase(it);
            }

            // Check decoratorChildId
            if (parent.decoratorChildId == m_node.id)
            {
                parent.decoratorChildId = 0;
            }
        }

        // Remove the node itself
        auto nodeIt = std::find_if(tree.nodes.begin(), tree.nodes.end(),
            [this](const BTNode& n) { return n.id == m_node.id; });

        if (nodeIt != tree.nodes.end())
        {
            tree.nodes.erase(nodeIt);
            SYSTEM_LOG << "[BTEditor] Deleted node: " << m_node.name << " (ID: " << m_node.id << ")\n";
        }
    }

    void DeleteNodeCommand::Undo(BehaviorTreeAsset& tree)
    {
        // Re-add the node
        tree.nodes.push_back(m_node);

        // Restore parent connections
        for (size_t i = 0; i < m_parentIds.size(); ++i)
        {
            BTNode* parent = tree.GetNode(m_parentIds[i]);
            if (parent)
            {
                // Insert at the original index if possible
                int index = m_childIndices[i];
                if (index >= 0 && static_cast<size_t>(index) <= parent->childIds.size())
                {
                    parent->childIds.insert(parent->childIds.begin() + index, m_node.id);
                }
                else
                {
                    parent->childIds.push_back(m_node.id);
                }
            }
        }

        SYSTEM_LOG << "[BTEditor] Undone delete node: " << m_node.name << " (ID: " << m_node.id << ")\n";
    }

    std::string DeleteNodeCommand::GetDescription() const
    {
        return "Delete " + m_node.name;
    }

    // ========================================================================
    // MoveNodeCommand Implementation
    // ========================================================================

    void MoveNodeCommand::Execute(BehaviorTreeAsset& tree)
    {
        // Note: Position is typically stored in layout engine, not in BTNode
        // This command is a placeholder for future position persistence
        SYSTEM_LOG << "[BTEditor] Moved node ID " << m_nodeId << "\n";
    }

    void MoveNodeCommand::Undo(BehaviorTreeAsset& tree)
    {
        // Note: Position is typically stored in layout engine, not in BTNode
        SYSTEM_LOG << "[BTEditor] Undone move node ID " << m_nodeId << "\n";
    }

    std::string MoveNodeCommand::GetDescription() const
    {
        return "Move Node";
    }

    // ========================================================================
    // ConnectNodesCommand Implementation
    // ========================================================================

    void ConnectNodesCommand::Execute(BehaviorTreeAsset& tree)
    {
        BTNode* parent = tree.GetNode(m_parentId);
        if (!parent)
            return;

        // Check if connection already exists
        if (std::find(parent->childIds.begin(), parent->childIds.end(), m_childId) != parent->childIds.end())
            return;

        // Add connection
        if (parent->type == BTNodeType::Selector || parent->type == BTNodeType::Sequence)
        {
            parent->childIds.push_back(m_childId);
        }
        else if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
        {
            parent->decoratorChildId = m_childId;
            parent->childIds.push_back(m_childId); // Also add to childIds for uniform handling
        }

        SYSTEM_LOG << "[BTEditor] Connected nodes: " << m_parentId << " -> " << m_childId << "\n";
    }

    void ConnectNodesCommand::Undo(BehaviorTreeAsset& tree)
    {
        BTNode* parent = tree.GetNode(m_parentId);
        if (!parent)
            return;

        // Remove connection from childIds
        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), m_childId);
        if (it != parent->childIds.end())
        {
            parent->childIds.erase(it);
        }

        // Remove decoratorChildId if applicable
        if (parent->decoratorChildId == m_childId)
        {
            parent->decoratorChildId = 0;
        }

        SYSTEM_LOG << "[BTEditor] Undone connection: " << m_parentId << " -> " << m_childId << "\n";
    }

    std::string ConnectNodesCommand::GetDescription() const
    {
        return "Connect Nodes";
    }

    // ========================================================================
    // DisconnectNodesCommand Implementation
    // ========================================================================

    void DisconnectNodesCommand::Execute(BehaviorTreeAsset& tree)
    {
        BTNode* parent = tree.GetNode(m_parentId);
        if (!parent)
            return;

        // Remove connection from childIds
        auto it = std::find(parent->childIds.begin(), parent->childIds.end(), m_childId);
        if (it != parent->childIds.end())
        {
            m_childIndex = static_cast<int>(it - parent->childIds.begin());
            parent->childIds.erase(it);
        }

        // Remove decoratorChildId if applicable
        if (parent->decoratorChildId == m_childId)
        {
            parent->decoratorChildId = 0;
        }

        SYSTEM_LOG << "[BTEditor] Disconnected nodes: " << m_parentId << " -> " << m_childId << "\n";
    }

    void DisconnectNodesCommand::Undo(BehaviorTreeAsset& tree)
    {
        BTNode* parent = tree.GetNode(m_parentId);
        if (!parent)
            return;

        // Restore connection at original index if possible
        if (m_childIndex >= 0 && static_cast<size_t>(m_childIndex) <= parent->childIds.size())
        {
            parent->childIds.insert(parent->childIds.begin() + m_childIndex, m_childId);
        }
        else
        {
            parent->childIds.push_back(m_childId);
        }

        // Restore decoratorChildId if applicable
        if (parent->type == BTNodeType::Inverter || parent->type == BTNodeType::Repeater)
        {
            parent->decoratorChildId = m_childId;
        }

        SYSTEM_LOG << "[BTEditor] Undone disconnection: " << m_parentId << " -> " << m_childId << "\n";
    }

    std::string DisconnectNodesCommand::GetDescription() const
    {
        return "Disconnect Nodes";
    }

    // ========================================================================
    // EditParameterCommand Implementation
    // ========================================================================

    void EditParameterCommand::Execute(BehaviorTreeAsset& tree)
    {
        BTNode* node = tree.GetNode(m_nodeId);
        if (!node)
            return;

        // Store the new value in stringParams
        node->stringParams[m_parameterKey] = m_newValue;

        SYSTEM_LOG << "[BTEditor] Edited parameter '" << m_parameterKey 
                   << "' on node ID " << m_nodeId << "\n";
    }

    void EditParameterCommand::Undo(BehaviorTreeAsset& tree)
    {
        BTNode* node = tree.GetNode(m_nodeId);
        if (!node)
            return;

        // Restore the old value
        if (m_oldValue.empty())
        {
            node->stringParams.erase(m_parameterKey);
        }
        else
        {
            node->stringParams[m_parameterKey] = m_oldValue;
        }

        SYSTEM_LOG << "[BTEditor] Undone parameter edit '" << m_parameterKey 
                   << "' on node ID " << m_nodeId << "\n";
    }

    std::string EditParameterCommand::GetDescription() const
    {
        return "Edit Parameter";
    }

} // namespace Olympe
