/**
 * @file CommandHistory.h
 * @brief Undo/redo history manager for graph commands
 * @author Olympe Engine
 * @details C++14 compliant - no C++17/20 features
 */

#pragma once

#include "GraphCommand.h"
#include <vector>
#include <string>

namespace Olympe
{
    /**
     * @class CommandHistory
     * @brief Manages undo/redo stacks for graph operations
     * 
     * Implements a standard undo/redo pattern with two stacks:
     * - Undo stack: Commands that can be undone
     * - Redo stack: Commands that were undone and can be redone
     */
    class CommandHistory
    {
    public:
        CommandHistory();
        ~CommandHistory() = default;

        /**
         * @brief Execute a new command and add it to history
         * @param command The command to execute
         * @return true if command executed successfully
         */
        bool ExecuteCommand(GraphCommandPtr command);

        /**
         * @brief Undo the last command
         * @return true if undo succeeded, false if nothing to undo
         */
        bool Undo();

        /**
         * @brief Redo the last undone command
         * @return true if redo succeeded, false if nothing to redo
         */
        bool Redo();

        /**
         * @brief Check if undo is available
         * @return true if there's a command to undo
         */
        bool CanUndo() const;

        /**
         * @brief Check if redo is available
         * @return true if there's a command to redo
         */
        bool CanRedo() const;

        /**
         * @brief Get description of next undo command
         * @return Description string or empty if nothing to undo
         */
        std::string GetUndoDescription() const;

        /**
         * @brief Get description of next redo command
         * @return Description string or empty if nothing to redo
         */
        std::string GetRedoDescription() const;

        /**
         * @brief Clear all undo/redo history
         */
        void Clear();

        /**
         * @brief Get current undo stack size
         */
        size_t GetUndoStackSize() const { return m_undoStack.size(); }

        /**
         * @brief Get current redo stack size
         */
        size_t GetRedoStackSize() const { return m_redoStack.size(); }

    private:
        std::vector<GraphCommandPtr> m_undoStack;
        std::vector<GraphCommandPtr> m_redoStack;
    };

}  // namespace Olympe
