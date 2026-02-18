/**
 * @file CommandSystem.h
 * @brief Command pattern for undo/redo operations
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Implements Command pattern for all graph operations to enable undo/redo.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Olympe {
namespace NodeGraph {

/**
 * @class ICommand
 * @brief Base interface for all commands
 */
class ICommand {
public:
    virtual ~ICommand() = default;
    
    /**
     * @brief Execute the command
     */
    virtual void Execute() = 0;
    
    /**
     * @brief Undo the command
     */
    virtual void Undo() = 0;
    
    /**
     * @brief Get description of the command
     */
    virtual std::string GetDescription() const = 0;
};

/**
 * @class CommandStack
 * @brief Manages undo/redo stacks for commands
 */
class CommandStack {
public:
    /**
     * @brief Execute a command and add to undo stack
     * @param cmd Command to execute
     */
    void ExecuteCommand(std::unique_ptr<ICommand> cmd);
    
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
     * @brief Get description of next undo command
     */
    std::string GetUndoDescription() const;
    
    /**
     * @brief Get description of next redo command
     */
    std::string GetRedoDescription() const;
    
    /**
     * @brief Clear all commands
     */
    void Clear();
    
private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
    static const size_t MAX_STACK_SIZE = 100;
};

} // namespace NodeGraph
} // namespace Olympe
