/**
 * @file BTEditorCommand.cpp
 * @brief Implementation of command pattern for behavior tree editor
 */

#include "BTEditorCommand.h"
#include <iostream>
#include <algorithm>

namespace Olympe
{
    // =============================================================================
    // BTCommandStack Implementation
    // =============================================================================
    
    void BTCommandStack::Execute(std::unique_ptr<BTEditorCommand> cmd)
    {
        if (!cmd)
            return;
        
        cmd->Execute();
        
        m_undoStack.push_back(std::move(cmd));
        
        // Clear redo stack after new action
        m_redoStack.clear();
        
        // Limit stack size
        if (m_undoStack.size() > kMaxStackSize)
        {
            m_undoStack.erase(m_undoStack.begin());
        }
    }
    
    void BTCommandStack::Undo()
    {
        if (!CanUndo())
            return;
        
        auto cmd = std::move(m_undoStack.back());
        m_undoStack.pop_back();
        
        cmd->Undo();
        
        m_redoStack.push_back(std::move(cmd));
    }
    
    void BTCommandStack::Redo()
    {
        if (!CanRedo())
            return;
        
        auto cmd = std::move(m_redoStack.back());
        m_redoStack.pop_back();
        
        cmd->Execute();
        
        m_undoStack.push_back(std::move(cmd));
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
        if (!CanUndo())
            return "";
        
        return m_undoStack.back()->GetDescription();
    }
    
    std::string BTCommandStack::GetRedoDescription() const
    {
        if (!CanRedo())
            return "";
        
        return m_redoStack.back()->GetDescription();
    }
    
    void BTCommandStack::Clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
    }
    
    // =============================================================================
    // BTAddNodeCommand Implementation
    // =============================================================================

    BTAddNodeCommand::BTAddNodeCommand(BehaviorTreeAsset* tree, BTNodeType type,
                                   const std::string& name, const Vector& position)
        : m_tree(tree)
        , m_nodeType(type)
        , m_nodeName(name)
        , m_position(position)
        , m_createdNodeId(0)
    {
    }
    
    void BTAddNodeCommand::Execute()
    {
        if (!m_tree)
            return;

        m_createdNodeId = m_tree->AddNode(m_nodeType, m_nodeName, m_position);
        std::cout << "[BTAddNodeCommand] Created node ID=" << m_createdNodeId << std::endl;
    }
    
    void BTAddNodeCommand::Undo()
    {
        if (!m_tree || m_createdNodeId == 0)
            return;

        m_tree->RemoveNode(m_createdNodeId);
        std::cout << "[BTAddNodeCommand] Removed node ID=" << m_createdNodeId << std::endl;
    }
    
    std::string BTAddNodeCommand::GetDescription() const
    {
        return "Add Node: " + m_nodeName;
    }
    
    // =============================================================================
    // BTDeleteNodeCommand Implementation
    // =============================================================================

    BTDeleteNodeCommand::BTDeleteNodeCommand(BehaviorTreeAsset* tree, uint32_t nodeId)
        : m_tree(tree)
        , m_nodeId(nodeId)
    {
        // Save the node data before deletion
        if (tree)
        {
            const BTNode* node = tree->GetNode(nodeId);
            if (node)
            {
                m_savedNode = *node;
                
                // Save all connections to/from this node
                for (const auto& otherNode : tree->nodes)
                {
                    // Check if otherNode is parent of this node
                    for (size_t i = 0; i < otherNode.childIds.size(); ++i)
                    {
                        if (otherNode.childIds[i] == nodeId)
                        {
                            Connection conn;
                            conn.parentId = otherNode.id;
                            conn.childId = nodeId;
                            conn.isDecorator = false;
                            conn.childIndex = static_cast<int>(i);
                            m_savedConnections.push_back(conn);
                        }
                    }
                    
                    if (otherNode.decoratorChildId == nodeId)
                    {
                        Connection conn;
                        conn.parentId = otherNode.id;
                        conn.childId = nodeId;
                        conn.isDecorator = true;
                        conn.childIndex = 0;
                        m_savedConnections.push_back(conn);
                    }
                }
            }
        }
    }
    
    void BTDeleteNodeCommand::Execute()
    {
        if (!m_tree)
            return;

        m_tree->RemoveNode(m_nodeId);
        std::cout << "[BTDeleteNodeCommand] Deleted node ID=" << m_nodeId << std::endl;
    }
    
    void BTDeleteNodeCommand::Undo()
    {
        if (!m_tree)
            return;

        // Restore the node
        m_tree->nodes.push_back(m_savedNode);

        // Restore connections
        for (const auto& conn : m_savedConnections)
        {
            BTNode* parent = m_tree->GetNode(conn.parentId);
            if (parent)
            {
                if (conn.isDecorator)
                {
                    parent->decoratorChildId = conn.childId;
                }
                else
                {
                    // Insert at the saved index if possible
                    if (conn.childIndex >= 0 && static_cast<size_t>(conn.childIndex) <= parent->childIds.size())
                    {
                        parent->childIds.insert(parent->childIds.begin() + conn.childIndex, conn.childId);
                    }
                    else
                    {
                        parent->childIds.push_back(conn.childId);
                    }
                }
            }
        }

        std::cout << "[BTDeleteNodeCommand] Restored node ID=" << m_nodeId << std::endl;
    }
    
    std::string BTDeleteNodeCommand::GetDescription() const
    {
        return "Delete Node ID=" + std::to_string(m_nodeId);
    }
    
    // =============================================================================
    // BTMoveNodeCommand Implementation
    // =============================================================================

    BTMoveNodeCommand::BTMoveNodeCommand(BehaviorTreeAsset* tree, uint32_t nodeId,
                                     const Vector& oldPos, const Vector& newPos)
        : m_tree(tree)
        , m_nodeId(nodeId)
        , m_oldPosition(oldPos)
        , m_newPosition(newPos)
    {
    }
    
    void BTMoveNodeCommand::Execute()
    {
        // Note: In the current implementation, node positions are not stored in BTNode
        // This would need to be added to BTNode if we want to save positions
        std::cout << "[BTMoveNodeCommand] Moved node ID=" << m_nodeId << std::endl;
    }
    
    void BTMoveNodeCommand::Undo()
    {
        std::cout << "[BTMoveNodeCommand] Restored position for node ID=" << m_nodeId << std::endl;
    }
    
    std::string BTMoveNodeCommand::GetDescription() const
    {
        return "Move Node ID=" + std::to_string(m_nodeId);
    }
    
    // =============================================================================
    // BTConnectNodesCommand Implementation
    // =============================================================================

    BTConnectNodesCommand::BTConnectNodesCommand(BehaviorTreeAsset* tree, uint32_t parentId, uint32_t childId)
        : m_tree(tree)
        , m_parentId(parentId)
        , m_childId(childId)
    {
    }
    
    void BTConnectNodesCommand::Execute()
    {
        if (!m_tree)
            return;

        m_tree->ConnectNodes(m_parentId, m_childId);
        std::cout << "[BTConnectNodesCommand] Connected " << m_parentId << " -> " << m_childId << std::endl;
    }
    
    void BTConnectNodesCommand::Undo()
    {
        if (!m_tree)
            return;

        m_tree->DisconnectNodes(m_parentId, m_childId);
        std::cout << "[BTConnectNodesCommand] Disconnected " << m_parentId << " -X-> " << m_childId << std::endl;
    }
    
    std::string BTConnectNodesCommand::GetDescription() const
    {
        return "Connect " + std::to_string(m_parentId) + " -> " + std::to_string(m_childId);
    }
    
    // =============================================================================
    // BTDisconnectNodesCommand Implementation
    // =============================================================================

    BTDisconnectNodesCommand::BTDisconnectNodesCommand(BehaviorTreeAsset* tree, uint32_t parentId, uint32_t childId)
        : m_tree(tree)
        , m_parentId(parentId)
        , m_childId(childId)
    {
    }
    
    void BTDisconnectNodesCommand::Execute()
    {
        if (!m_tree)
            return;

        m_tree->DisconnectNodes(m_parentId, m_childId);
        std::cout << "[BTDisconnectNodesCommand] Disconnected " << m_parentId << " -X-> " << m_childId << std::endl;
    }
    
    void BTDisconnectNodesCommand::Undo()
    {
        if (!m_tree)
            return;

        m_tree->ConnectNodes(m_parentId, m_childId);
        std::cout << "[BTDisconnectNodesCommand] Reconnected " << m_parentId << " -> " << m_childId << std::endl;
    }
    
    std::string BTDisconnectNodesCommand::GetDescription() const
    {
        return "Disconnect " + std::to_string(m_parentId) + " -X-> " + std::to_string(m_childId);
    }
    
    // =============================================================================
    // BTEditParameterCommand Implementation
    // =============================================================================

    BTEditParameterCommand::BTEditParameterCommand(BehaviorTreeAsset* tree, uint32_t nodeId,
                                               const std::string& paramName, const std::string& oldValue,
                                               const std::string& newValue, ParamType type)
        : m_tree(tree)
        , m_nodeId(nodeId)
        , m_paramName(paramName)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
        , m_paramType(type)
    {
    }
    
    void BTEditParameterCommand::Execute()
    {
        if (!m_tree)
            return;

        BTNode* node = m_tree->GetNode(m_nodeId);
        if (!node)
            return;

        // Apply new value based on parameter type
        if (m_paramType == ParamType::String)
        {
            node->stringParams[m_paramName] = m_newValue;
        }
        else if (m_paramType == ParamType::Int)
        {
            node->intParams[m_paramName] = std::stoi(m_newValue);
        }
        else if (m_paramType == ParamType::Float)
        {
            node->floatParams[m_paramName] = std::stof(m_newValue);
        }

        std::cout << "[BTEditParameterCommand] Set " << m_paramName << " = " << m_newValue << std::endl;
    }
    
    void BTEditParameterCommand::Undo()
    {
        if (!m_tree)
            return;

        BTNode* node = m_tree->GetNode(m_nodeId);
        if (!node)
            return;

        // Restore old value
        if (m_paramType == ParamType::String)
        {
            node->stringParams[m_paramName] = m_oldValue;
        }
        else if (m_paramType == ParamType::Int)
        {
            node->intParams[m_paramName] = std::stoi(m_oldValue);
        }
        else if (m_paramType == ParamType::Float)
        {
            node->floatParams[m_paramName] = std::stof(m_oldValue);
        }

        std::cout << "[BTEditParameterCommand] Restored " << m_paramName << " = " << m_oldValue << std::endl;
    }
    
    std::string BTEditParameterCommand::GetDescription() const
    {
        return "Edit " + m_paramName;
    }

} // namespace Olympe
