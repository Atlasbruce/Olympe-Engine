/*
 * Olympe Blueprint Editor - Command System Implementation
 */

#include "CommandSystem.h"
#include "NodeGraphManager.h"
#include <iostream>

namespace Olympe
{
    // ========================================================================
    // CommandStack Implementation
    // ========================================================================

    CommandStack::CommandStack()
        : m_MaxStackSize(100)  // Limit to 100 commands
    {
    }

    CommandStack::~CommandStack()
    {
        Clear();
    }

    void CommandStack::ExecuteCommand(std::unique_ptr<EditorCommand> cmd)
    {
        if (!cmd)
        {
            return;
        }

        // Execute the command
        cmd->Execute();

        // Add to undo stack
        m_UndoStack.push_back(std::move(cmd));

        // Clear redo stack (new action invalidates redo history)
        m_RedoStack.clear();

        // Enforce stack size limit
        if (m_UndoStack.size() > m_MaxStackSize)
        {
            m_UndoStack.erase(m_UndoStack.begin());
        }

        std::cout << "Command executed: " << m_UndoStack.back()->GetDescription() << std::endl;
    }

    void CommandStack::Undo()
    {
        if (m_UndoStack.empty())
        {
            return;
        }

        // Get last command
        auto cmd = std::move(m_UndoStack.back());
        m_UndoStack.pop_back();

        // Undo the command
        cmd->Undo();

        // Move to redo stack
        m_RedoStack.push_back(std::move(cmd));

        std::cout << "Command undone: " << m_RedoStack.back()->GetDescription() << std::endl;
    }

    void CommandStack::Redo()
    {
        if (m_RedoStack.empty())
        {
            return;
        }

        // Get last undone command
        auto cmd = std::move(m_RedoStack.back());
        m_RedoStack.pop_back();

        // Re-execute the command
        cmd->Execute();

        // Move back to undo stack
        m_UndoStack.push_back(std::move(cmd));

        std::cout << "Command redone: " << m_UndoStack.back()->GetDescription() << std::endl;
    }

    const EditorCommand* CommandStack::GetLastCommand() const
    {
        if (m_UndoStack.empty())
        {
            return nullptr;
        }
        return m_UndoStack.back().get();
    }

    std::string CommandStack::GetLastCommandDescription() const
    {
        if (m_UndoStack.empty())
        {
            return "";
        }
        return m_UndoStack.back()->GetDescription();
    }

    std::string CommandStack::GetNextRedoDescription() const
    {
        if (m_RedoStack.empty())
        {
            return "";
        }
        return m_RedoStack.back()->GetDescription();
    }

    std::vector<std::string> CommandStack::GetUndoStackDescriptions() const
    {
        std::vector<std::string> descriptions;
        for (const auto& cmd : m_UndoStack)
        {
            descriptions.push_back(cmd->GetDescription());
        }
        return descriptions;
    }

    std::vector<std::string> CommandStack::GetRedoStackDescriptions() const
    {
        std::vector<std::string> descriptions;
        for (const auto& cmd : m_RedoStack)
        {
            descriptions.push_back(cmd->GetDescription());
        }
        return descriptions;
    }

    void CommandStack::Clear()
    {
        m_UndoStack.clear();
        m_RedoStack.clear();
    }

    // ========================================================================
    // CreateNodeCommand Implementation
    // ========================================================================

    CreateNodeCommand::CreateNodeCommand(const std::string& graphId, 
                                        const std::string& nodeType,
                                        float posX, float posY,
                                        const std::string& nodeName)
        : m_GraphId(graphId)
        , m_NodeType(nodeType)
        , m_NodeName(nodeName)
        , m_PosX(posX)
        , m_PosY(posY)
        , m_CreatedNodeId(-1)
    {
    }

    void CreateNodeCommand::Execute()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            std::cerr << "CreateNodeCommand: Graph not found: " << m_GraphId << std::endl;
            return;
        }

        // Create the node
        NodeType type = StringToNodeType(m_NodeType);
        m_CreatedNodeId = graph->CreateNode(type, m_PosX, m_PosY, m_NodeName);
    }

    void CreateNodeCommand::Undo()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            std::cerr << "CreateNodeCommand::Undo: Graph not found: " << m_GraphId << std::endl;
            return;
        }

        // Delete the created node
        if (m_CreatedNodeId != -1)
        {
            graph->DeleteNode(m_CreatedNodeId);
        }
    }

    std::string CreateNodeCommand::GetDescription() const
    {
        return "Create " + m_NodeType + " Node";
    }

    // ========================================================================
    // DeleteNodeCommand Implementation
    // ========================================================================

    DeleteNodeCommand::DeleteNodeCommand(const std::string& graphId, int nodeId)
        : m_GraphId(graphId)
        , m_NodeId(nodeId)
    {
    }

    void DeleteNodeCommand::Execute()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            std::cerr << "DeleteNodeCommand: Graph not found: " << m_GraphId << std::endl;
            return;
        }

        // Save node data for undo
        const GraphNode* node = graph->GetNode(m_NodeId);
        if (node)
        {
            m_NodeData = json::object();
            m_NodeData["id"] = node->id;
            m_NodeData["type"] = NodeTypeToString(node->type);
            m_NodeData["name"] = node->name;
            m_NodeData["posX"] = node->posX;
            m_NodeData["posY"] = node->posY;
            // TODO: Save additional node data as needed
        }

        // Delete the node
        graph->DeleteNode(m_NodeId);
    }

    void DeleteNodeCommand::Undo()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            std::cerr << "DeleteNodeCommand::Undo: Graph not found: " << m_GraphId << std::endl;
            return;
        }

        // Recreate the node from saved data
        if (m_NodeData.contains("type") && m_NodeData.contains("posX") && m_NodeData.contains("posY"))
        {
            NodeType type = StringToNodeType(m_NodeData["type"].get<std::string>());
            std::string name = m_NodeData.contains("name") ? m_NodeData["name"].get<std::string>() : "";
            
            int nodeId = graph->CreateNode(type, 
                                          m_NodeData["posX"].get<float>(), 
                                          m_NodeData["posY"].get<float>(), 
                                          name);
            
            // TODO: Restore additional node properties
        }
    }

    std::string DeleteNodeCommand::GetDescription() const
    {
        return "Delete Node " + std::to_string(m_NodeId);
    }

    // ========================================================================
    // MoveNodeCommand Implementation
    // ========================================================================

    MoveNodeCommand::MoveNodeCommand(const std::string& graphId, int nodeId,
                                    float oldX, float oldY, float newX, float newY)
        : m_GraphId(graphId)
        , m_NodeId(nodeId)
        , m_OldX(oldX)
        , m_OldY(oldY)
        , m_NewX(newX)
        , m_NewY(newY)
    {
    }

    void MoveNodeCommand::Execute()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            return;
        }

        // Move the node
        GraphNode* node = graph->GetNode(m_NodeId);
        if (node)
        {
            node->posX = m_NewX;
            node->posY = m_NewY;
        }
    }

    void MoveNodeCommand::Undo()
    {
        // Get the graph
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            return;
        }

        // Restore old position
        GraphNode* node = graph->GetNode(m_NodeId);
        if (node)
        {
            node->posX = m_OldX;
            node->posY = m_OldY;
        }
    }

    std::string MoveNodeCommand::GetDescription() const
    {
        return "Move Node " + std::to_string(m_NodeId);
    }

    // ========================================================================
    // LinkNodesCommand Implementation
    // ========================================================================

    LinkNodesCommand::LinkNodesCommand(const std::string& graphId, int parentId, int childId)
        : m_GraphId(graphId)
        , m_ParentId(parentId)
        , m_ChildId(childId)
    {
    }

    void LinkNodesCommand::Execute()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->LinkNodes(m_ParentId, m_ChildId);
        }
    }

    void LinkNodesCommand::Undo()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->UnlinkNodes(m_ParentId, m_ChildId);
        }
    }

    std::string LinkNodesCommand::GetDescription() const
    {
        return "Link Nodes " + std::to_string(m_ParentId) + " -> " + std::to_string(m_ChildId);
    }

    // ========================================================================
    // UnlinkNodesCommand Implementation
    // ========================================================================

    UnlinkNodesCommand::UnlinkNodesCommand(const std::string& graphId, int parentId, int childId)
        : m_GraphId(graphId)
        , m_ParentId(parentId)
        , m_ChildId(childId)
    {
    }

    void UnlinkNodesCommand::Execute()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->UnlinkNodes(m_ParentId, m_ChildId);
        }
    }

    void UnlinkNodesCommand::Undo()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->LinkNodes(m_ParentId, m_ChildId);
        }
    }

    std::string UnlinkNodesCommand::GetDescription() const
    {
        return "Unlink Nodes " + std::to_string(m_ParentId) + " -> " + std::to_string(m_ChildId);
    }

    // ========================================================================
    // SetParameterCommand Implementation
    // ========================================================================

    SetParameterCommand::SetParameterCommand(const std::string& graphId, int nodeId,
                                            const std::string& paramName,
                                            const std::string& oldValue,
                                            const std::string& newValue)
        : m_GraphId(graphId)
        , m_NodeId(nodeId)
        , m_ParamName(paramName)
        , m_OldValue(oldValue)
        , m_NewValue(newValue)
    {
    }

    void SetParameterCommand::Execute()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->SetNodeParameter(m_NodeId, m_ParamName, m_NewValue);
        }
    }

    void SetParameterCommand::Undo()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (graph)
        {
            graph->SetNodeParameter(m_NodeId, m_ParamName, m_OldValue);
        }
    }

    std::string SetParameterCommand::GetDescription() const
    {
        return "Set " + m_ParamName + " = " + m_NewValue;
    }

    // ========================================================================
    // DuplicateNodeCommand Implementation
    // ========================================================================

    DuplicateNodeCommand::DuplicateNodeCommand(const std::string& graphId, int sourceNodeId)
        : m_GraphId(graphId)
        , m_SourceNodeId(sourceNodeId)
        , m_CreatedNodeId(-1)
    {
    }

    void DuplicateNodeCommand::Execute()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            std::cerr << "DuplicateNodeCommand: Graph not found: " << m_GraphId << std::endl;
            return;
        }

        // Get source node
        const GraphNode* sourceNode = graph->GetNode(m_SourceNodeId);
        if (!sourceNode)
        {
            std::cerr << "DuplicateNodeCommand: Source node not found: " << m_SourceNodeId << std::endl;
            return;
        }

        // Create a duplicate node at an offset position
        m_CreatedNodeId = graph->CreateNode(sourceNode->type, 
                                           sourceNode->posX + 50.0f, 
                                           sourceNode->posY + 50.0f,
                                           sourceNode->name + " Copy");

        GraphNode* newNode = graph->GetNode(m_CreatedNodeId);
        if (newNode)
        {
            // Copy all properties
            newNode->actionType = sourceNode->actionType;
            newNode->conditionType = sourceNode->conditionType;
            newNode->decoratorType = sourceNode->decoratorType;
            newNode->parameters = sourceNode->parameters;
            
            // Save node data for undo
            m_NodeData = json::object();
            m_NodeData["id"] = newNode->id;
            m_NodeData["type"] = NodeTypeToString(newNode->type);
            m_NodeData["name"] = newNode->name;
        }

        std::cout << "Duplicated node " << m_SourceNodeId << " to " << m_CreatedNodeId << std::endl;
    }

    void DuplicateNodeCommand::Undo()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            return;
        }

        // Delete the created node
        if (m_CreatedNodeId >= 0)
        {
            graph->DeleteNode(m_CreatedNodeId);
        }
    }

    std::string DuplicateNodeCommand::GetDescription() const
    {
        return "Duplicate Node " + std::to_string(m_SourceNodeId);
    }

    // ========================================================================
    // EditNodeCommand Implementation
    // ========================================================================

    EditNodeCommand::EditNodeCommand(const std::string& graphId, int nodeId,
                                   const std::string& oldName, const std::string& newName,
                                   const std::string& oldSubtype, const std::string& newSubtype)
        : m_GraphId(graphId)
        , m_NodeId(nodeId)
        , m_OldName(oldName)
        , m_NewName(newName)
        , m_OldSubtype(oldSubtype)
        , m_NewSubtype(newSubtype)
    {
    }

    void EditNodeCommand::Execute()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            return;
        }

        GraphNode* node = graph->GetNode(m_NodeId);
        if (node)
        {
            node->name = m_NewName;
            
            // Update subtype based on node type
            if (node->type == NodeType::BT_Action)
            {
                node->actionType = m_NewSubtype;
            }
            else if (node->type == NodeType::BT_Condition)
            {
                node->conditionType = m_NewSubtype;
            }
            else if (node->type == NodeType::BT_Decorator)
            {
                node->decoratorType = m_NewSubtype;
            }
        }
    }

    void EditNodeCommand::Undo()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(std::stoi(m_GraphId));
        if (!graph)
        {
            return;
        }

        GraphNode* node = graph->GetNode(m_NodeId);
        if (node)
        {
            node->name = m_OldName;
            
            // Restore old subtype
            if (node->type == NodeType::BT_Action)
            {
                node->actionType = m_OldSubtype;
            }
            else if (node->type == NodeType::BT_Condition)
            {
                node->conditionType = m_OldSubtype;
            }
            else if (node->type == NodeType::BT_Decorator)
            {
                node->decoratorType = m_OldSubtype;
            }
        }
    }

    std::string EditNodeCommand::GetDescription() const
    {
        return "Edit Node " + std::to_string(m_NodeId);
    }
}
