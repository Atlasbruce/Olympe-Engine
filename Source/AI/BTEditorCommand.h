/**
 * @file BTEditorCommand.h
 * @brief Command Pattern implementation for BT Editor Undo/Redo system
 * @author Olympe Engine - Behavior Tree Editor
 * @date 2025
 *
 * @details
 * Implements the Command Pattern for reversible BT editing operations.
 * All commands can Execute() and Undo() on a BehaviorTreeAsset.
 */

#pragma once

#include "BehaviorTree.h"
#include "../vector.h"
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace Olympe
{
    /**
     * @class BTEditorCommand
     * @brief Abstract base class for all BT editor commands
     */
    class BTEditorCommand
    {
    public:
        virtual ~BTEditorCommand() = default;

        /**
         * @brief Execute the command (apply changes to tree)
         * @param tree The behavior tree to modify
         */
        virtual void Execute(BehaviorTreeAsset& tree) = 0;

        /**
         * @brief Undo the command (reverse changes to tree)
         * @param tree The behavior tree to modify
         */
        virtual void Undo(BehaviorTreeAsset& tree) = 0;

        /**
         * @brief Get a human-readable description of this command
         * @return Description string for UI display
         */
        virtual std::string GetDescription() const = 0;
    };

    /**
     * @class BTCommandStack
     * @brief Manages undo/redo stacks for BT editor
     */
    class BTCommandStack
    {
    public:
        BTCommandStack() = default;
        ~BTCommandStack() = default;

        /**
         * @brief Execute a command and add it to the undo stack
         * @param tree The behavior tree to modify
         * @param command The command to execute (takes ownership)
         */
        void Execute(BehaviorTreeAsset& tree, std::unique_ptr<BTEditorCommand> command);

        /**
         * @brief Undo the last command
         * @param tree The behavior tree to modify
         * @return true if undo was performed, false if stack was empty
         */
        bool Undo(BehaviorTreeAsset& tree);

        /**
         * @brief Redo the last undone command
         * @param tree The behavior tree to modify
         * @return true if redo was performed, false if stack was empty
         */
        bool Redo(BehaviorTreeAsset& tree);

        /**
         * @brief Check if undo is available
         * @return true if there are commands to undo
         */
        bool CanUndo() const;

        /**
         * @brief Check if redo is available
         * @return true if there are commands to redo
         */
        bool CanRedo() const;

        /**
         * @brief Get description of next undo command
         * @return Description string, or empty if no undo available
         */
        std::string GetUndoDescription() const;

        /**
         * @brief Get description of next redo command
         * @return Description string, or empty if no redo available
         */
        std::string GetRedoDescription() const;

        /**
         * @brief Clear all undo/redo history
         */
        void Clear();

    private:
        std::vector<std::unique_ptr<BTEditorCommand>> m_undoStack;
        std::vector<std::unique_ptr<BTEditorCommand>> m_redoStack;
        static const size_t MAX_STACK_SIZE = 100;
    };

    /**
     * @class AddNodeCommand
     * @brief Command to add a new node to the tree
     */
    class AddNodeCommand : public BTEditorCommand
    {
    public:
        AddNodeCommand(const BTNode& node)
            : m_node(node)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        BTNode m_node;
    };

    /**
     * @class DeleteNodeCommand
     * @brief Command to delete a node from the tree
     */
    class DeleteNodeCommand : public BTEditorCommand
    {
    public:
        DeleteNodeCommand(const BTNode& node)
            : m_node(node)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        BTNode m_node;
        std::vector<uint32_t> m_parentIds; // Nodes that had this as child
        std::vector<int> m_childIndices;   // Index in each parent's childIds
    };

    /**
     * @class MoveNodeCommand
     * @brief Command to move a node to a new position
     */
    class MoveNodeCommand : public BTEditorCommand
    {
    public:
        MoveNodeCommand(uint32_t nodeId, const Vector& oldPos, const Vector& newPos)
            : m_nodeId(nodeId)
            , m_oldPosition(oldPos)
            , m_newPosition(newPos)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        uint32_t m_nodeId = 0;
        Vector m_oldPosition;
        Vector m_newPosition;
    };

    /**
     * @class ConnectNodesCommand
     * @brief Command to create a connection between two nodes
     */
    class ConnectNodesCommand : public BTEditorCommand
    {
    public:
        ConnectNodesCommand(uint32_t parentId, uint32_t childId)
            : m_parentId(parentId)
            , m_childId(childId)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        uint32_t m_parentId = 0;
        uint32_t m_childId = 0;
    };

    /**
     * @class DisconnectNodesCommand
     * @brief Command to remove a connection between two nodes
     */
    class DisconnectNodesCommand : public BTEditorCommand
    {
    public:
        DisconnectNodesCommand(uint32_t parentId, uint32_t childId, int childIndex)
            : m_parentId(parentId)
            , m_childId(childId)
            , m_childIndex(childIndex)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        uint32_t m_parentId = 0;
        uint32_t m_childId = 0;
        int m_childIndex = 0; // Index where child was in parent's childIds
    };

    /**
     * @class EditParameterCommand
     * @brief Command to edit a node parameter
     */
    class EditParameterCommand : public BTEditorCommand
    {
    public:
        EditParameterCommand(uint32_t nodeId, const std::string& paramKey,
                           const std::string& oldValue, const std::string& newValue)
            : m_nodeId(nodeId)
            , m_parameterKey(paramKey)
            , m_oldValue(oldValue)
            , m_newValue(newValue)
        {
        }

        void Execute(BehaviorTreeAsset& tree) override;
        void Undo(BehaviorTreeAsset& tree) override;
        std::string GetDescription() const override;

    private:
        uint32_t m_nodeId = 0;
        std::string m_parameterKey;
        std::string m_oldValue;
        std::string m_newValue;
    };

} // namespace Olympe
