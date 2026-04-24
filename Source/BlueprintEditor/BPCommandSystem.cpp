/*
 * Olympe Blueprint Editor - Command System Implementation
 */

#include "BPCommandSystem.h"
#include "BTNodeGraphManager.h"
#include <iostream>

namespace Olympe
{
    // Type aliases for backward compatibility with modern NodeGraphTypes
    using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
    using NodeData = Olympe::NodeGraphTypes::NodeData;
    using NodeId = Olympe::NodeGraphTypes::NodeId;

    namespace Blueprint
    {
        // ========================================================================
        // CommandStack Implementation (DEPRECATED - Phase 50.3)
        // TODO: Reimplement with modern NodeGraphTypes schema in Phase 50.4
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
            const std::string& nodeName, int* outCreatedId)
            : m_GraphId(graphId)
            , m_NodeType(nodeType)
            , m_NodeName(nodeName)
            , m_PosX(posX)
            , m_PosY(posY)
            , m_CreatedNodeId(-1)
            , m_OutCreatedId(outCreatedId)
        {
            if (m_OutCreatedId)
                *m_OutCreatedId = -1;
        }

        void CreateNodeCommand::Execute()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes schema
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
            // This command is deprecated and unused by rendering pipeline
        }

        void CreateNodeCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void DeleteNodeCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void MoveNodeCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void LinkNodesCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void UnlinkNodesCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void SetParameterCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void DuplicateNodeCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
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
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        void EditNodeCommand::Undo()
        {
            // TODO: Phase 50.4 - Reimplement with modern NodeGraphTypes
            // NodeGraph class removed (Phase 50.3 namespace collision fix)
        }

        std::string EditNodeCommand::GetDescription() const
        {
            return "Edit Node " + std::to_string(m_NodeId);
        }
    }
}