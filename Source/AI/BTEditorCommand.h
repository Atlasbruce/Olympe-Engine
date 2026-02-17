/**
 * @file BTEditorCommand.h
 * @brief Command pattern implementation for behavior tree editor undo/redo
 * @author Olympe Engine - BT Editor
 * @date 2026
 *
 * @details
 * Implements the Command pattern for all editor operations to enable undo/redo.
 * Each command encapsulates an action and stores the data needed to undo it.
 */

#pragma once

#include "BehaviorTree.h"
#include "../vector.h"
#include <string>
#include <vector>
#include <memory>

namespace Olympe
{
    /**
     * @class BTEditorCommand
     * @brief Base class for all editor commands
     */
    class BTEditorCommand
    {
    public:
        virtual ~BTEditorCommand() = default;
        
        /**
         * @brief Execute the command
         */
        virtual void Execute() = 0;
        
        /**
         * @brief Undo the command
         */
        virtual void Undo() = 0;
        
        /**
         * @brief Get a human-readable description of the command
         */
        virtual std::string GetDescription() const = 0;
    };

    /**
     * @class BTCommandStack
     * @brief Manages undo/redo stacks for editor commands
     */
    class BTCommandStack
    {
    public:
        /**
         * @brief Execute a command and add it to the undo stack
         * @param cmd Command to execute
         */
        void Execute(std::unique_ptr<BTEditorCommand> cmd);
        
        /**
         * @brief Undo the last command
         */
        void Undo();
        
        /**
         * @brief Redo the last undone command
         */
        void Redo();
        
        /**
         * @brief Check if undo is available
         */
        bool CanUndo() const;
        
        /**
         * @brief Check if redo is available
         */
        bool CanRedo() const;
        
        /**
         * @brief Get description of the next undo command
         */
        std::string GetUndoDescription() const;
        
        /**
         * @brief Get description of the next redo command
         */
        std::string GetRedoDescription() const;
        
        /**
         * @brief Clear all commands
         */
        void Clear();
        
    private:
        std::vector<std::unique_ptr<BTEditorCommand>> m_undoStack;
        std::vector<std::unique_ptr<BTEditorCommand>> m_redoStack;
        static const size_t kMaxStackSize = 100;
    };

    /**
     * @class AddNodeCommand
     * @brief Command to add a node to the tree
     */
    class AddNodeCommand : public BTEditorCommand
    {
    public:
        AddNodeCommand(BehaviorTreeAsset* tree, BTNodeType type, const std::string& name, const Vector& position);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        BTNodeType m_nodeType;
        std::string m_nodeName;
        Vector m_position;
        uint32_t m_createdNodeId;
    };

    /**
     * @class DeleteNodeCommand
     * @brief Command to delete a node from the tree
     */
    class DeleteNodeCommand : public BTEditorCommand
    {
    public:
        DeleteNodeCommand(BehaviorTreeAsset* tree, uint32_t nodeId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        uint32_t m_nodeId;
        BTNode m_savedNode;
        struct Connection {
            uint32_t parentId;
            uint32_t childId;
            bool isDecorator;
            int childIndex;
        };
        std::vector<Connection> m_savedConnections;
    };

    /**
     * @class MoveNodeCommand
     * @brief Command to move a node
     */
    class MoveNodeCommand : public BTEditorCommand
    {
    public:
        MoveNodeCommand(BehaviorTreeAsset* tree, uint32_t nodeId, const Vector& oldPos, const Vector& newPos);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        uint32_t m_nodeId;
        Vector m_oldPosition;
        Vector m_newPosition;
    };

    /**
     * @class ConnectNodesCommand
     * @brief Command to connect two nodes
     */
    class ConnectNodesCommand : public BTEditorCommand
    {
    public:
        ConnectNodesCommand(BehaviorTreeAsset* tree, uint32_t parentId, uint32_t childId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        uint32_t m_parentId;
        uint32_t m_childId;
    };

    /**
     * @class DisconnectNodesCommand
     * @brief Command to disconnect two nodes
     */
    class DisconnectNodesCommand : public BTEditorCommand
    {
    public:
        DisconnectNodesCommand(BehaviorTreeAsset* tree, uint32_t parentId, uint32_t childId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        uint32_t m_parentId;
        uint32_t m_childId;
    };

    /**
     * @class EditParameterCommand
     * @brief Command to edit a node parameter
     */
    class EditParameterCommand : public BTEditorCommand
    {
    public:
        enum class ParamType { String, Int, Float };
        
        EditParameterCommand(BehaviorTreeAsset* tree, uint32_t nodeId,
                           const std::string& paramName, const std::string& oldValue,
                           const std::string& newValue, ParamType type);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
    private:
        BehaviorTreeAsset* m_tree;
        uint32_t m_nodeId;
        std::string m_paramName;
        std::string m_oldValue;
        std::string m_newValue;
        ParamType m_paramType;
    };

} // namespace Olympe
