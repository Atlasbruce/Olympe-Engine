/*
 * Olympe Blueprint Editor - Command System
 * 
 * Command pattern infrastructure for undo/redo functionality
 * All editing operations should be wrapped in commands
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "../../Source/third_party/nlohmann/json.hpp"

// Forward declare ImVec2
struct ImVec2;

namespace Olympe
{
    using json = nlohmann::json;

    /**
     * EditorCommand - Base class for all undoable editor commands
     * Implements command pattern for undo/redo support
     */
    class EditorCommand
    {
    public:
        virtual ~EditorCommand() = default;

        // Execute the command (perform the operation)
        virtual void Execute() = 0;

        // Undo the command (revert the operation)
        virtual void Undo() = 0;

        // Get a human-readable description of the command
        virtual std::string GetDescription() const = 0;
    };

    /**
     * CommandStack - Manages undo/redo command history
     * Maintains two stacks for undo and redo operations
     */
    class CommandStack
    {
    public:
        CommandStack();
        ~CommandStack();

        // Execute a new command and add to undo stack
        void ExecuteCommand(std::unique_ptr<EditorCommand> cmd);

        // Undo the last command
        void Undo();

        // Redo the last undone command
        void Redo();

        // Query state
        bool CanUndo() const { return !m_UndoStack.empty(); }
        bool CanRedo() const { return !m_RedoStack.empty(); }

        // Get command information
        const EditorCommand* GetLastCommand() const;
        std::string GetLastCommandDescription() const;
        std::string GetNextRedoDescription() const;

        // Get stack contents for UI display
        std::vector<std::string> GetUndoStackDescriptions() const;
        std::vector<std::string> GetRedoStackDescriptions() const;

        // Clear all history
        void Clear();

        // Get stack sizes
        size_t GetUndoStackSize() const { return m_UndoStack.size(); }
        size_t GetRedoStackSize() const { return m_RedoStack.size(); }

    private:
        std::vector<std::unique_ptr<EditorCommand>> m_UndoStack;
        std::vector<std::unique_ptr<EditorCommand>> m_RedoStack;
        size_t m_MaxStackSize;  // Limit memory usage
    };

    // ========================================================================
    // Concrete Command Classes
    // ========================================================================

    /**
     * CreateNodeCommand - Command to create a new node in graph
     */
    class CreateNodeCommand : public EditorCommand
    {
    public:
        CreateNodeCommand(const std::string& graphId, const std::string& nodeType, 
                         float posX, float posY, const std::string& nodeName = "");
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        std::string m_NodeType;
        std::string m_NodeName;
        float m_PosX;
        float m_PosY;
        int m_CreatedNodeId;  // Set during Execute
    };

    /**
     * DeleteNodeCommand - Command to delete a node from graph
     */
    class DeleteNodeCommand : public EditorCommand
    {
    public:
        DeleteNodeCommand(const std::string& graphId, int nodeId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_NodeId;
        json m_NodeData;  // Saved node data for undo
    };

    /**
     * MoveNodeCommand - Command to move a node's position
     */
    class MoveNodeCommand : public EditorCommand
    {
    public:
        MoveNodeCommand(const std::string& graphId, int nodeId, 
                       float oldX, float oldY, float newX, float newY);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_NodeId;
        float m_OldX, m_OldY;
        float m_NewX, m_NewY;
    };

    /**
     * LinkNodesCommand - Command to create a link between nodes
     */
    class LinkNodesCommand : public EditorCommand
    {
    public:
        LinkNodesCommand(const std::string& graphId, int parentId, int childId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;
        
        // Check if the link is valid before executing
        bool IsValid() const { return m_IsValid; }
        std::string GetValidationError() const { return m_ValidationError; }

    private:
        std::string m_GraphId;
        int m_ParentId;
        int m_ChildId;
        bool m_IsValid;
        std::string m_ValidationError;
    };

    /**
     * UnlinkNodesCommand - Command to remove a link between nodes
     */
    class UnlinkNodesCommand : public EditorCommand
    {
    public:
        UnlinkNodesCommand(const std::string& graphId, int parentId, int childId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_ParentId;
        int m_ChildId;
    };

    /**
     * SetParameterCommand - Command to set a node parameter
     */
    class SetParameterCommand : public EditorCommand
    {
    public:
        SetParameterCommand(const std::string& graphId, int nodeId,
                           const std::string& paramName,
                           const std::string& oldValue,
                           const std::string& newValue);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_NodeId;
        std::string m_ParamName;
        std::string m_OldValue;
        std::string m_NewValue;
    };

    /**
     * DuplicateNodeCommand - Command to duplicate a node
     */
    class DuplicateNodeCommand : public EditorCommand
    {
    public:
        DuplicateNodeCommand(const std::string& graphId, int sourceNodeId);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_SourceNodeId;
        int m_CreatedNodeId;  // Set during Execute
        json m_NodeData;      // Backup for undo
    };

    /**
     * EditNodeCommand - Command to edit node properties (name, type-specific fields)
     */
    class EditNodeCommand : public EditorCommand
    {
    public:
        EditNodeCommand(const std::string& graphId, int nodeId,
                       const std::string& oldName, const std::string& newName,
                       const std::string& oldSubtype, const std::string& newSubtype);
        
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override;

    private:
        std::string m_GraphId;
        int m_NodeId;
        std::string m_OldName;
        std::string m_NewName;
        std::string m_OldSubtype;  // actionType, conditionType, or decoratorType
        std::string m_NewSubtype;
    };
}
