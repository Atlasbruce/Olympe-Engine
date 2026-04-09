/**
 * @file CommandHistory.cpp
 * @brief Implementation of CommandHistory
 * @author Olympe Engine
 */

#include "CommandHistory.h"

namespace Olympe
{
    CommandHistory::CommandHistory()
    {
    }

    bool CommandHistory::ExecuteCommand(GraphCommandPtr command)
    {
        if (!command)
            return false;

        // Execute the command
        if (!command->Execute())
            return false;

        // Clear redo stack (new action invalidates redo history)
        m_redoStack.clear();

        // Add to undo stack
        m_undoStack.push_back(std::move(command));

        return true;
    }

    bool CommandHistory::Undo()
    {
        if (m_undoStack.empty())
            return false;

        // Get last command
        GraphCommand* cmd = m_undoStack.back().get();

        // Execute undo
        if (!cmd->Undo())
            return false;

        // Move from undo to redo stack
        m_redoStack.push_back(std::move(m_undoStack.back()));
        m_undoStack.pop_back();

        return true;
    }

    bool CommandHistory::Redo()
    {
        if (m_redoStack.empty())
            return false;

        // Get last undone command
        GraphCommand* cmd = m_redoStack.back().get();

        // Execute forward (re-execute)
        if (!cmd->Execute())
            return false;

        // Move from redo to undo stack
        m_undoStack.push_back(std::move(m_redoStack.back()));
        m_redoStack.pop_back();

        return true;
    }

    bool CommandHistory::CanUndo() const
    {
        return !m_undoStack.empty();
    }

    bool CommandHistory::CanRedo() const
    {
        return !m_redoStack.empty();
    }

    std::string CommandHistory::GetUndoDescription() const
    {
        if (m_undoStack.empty())
            return "";
        return "Undo: " + m_undoStack.back()->GetDescription();
    }

    std::string CommandHistory::GetRedoDescription() const
    {
        if (m_redoStack.empty())
            return "";
        return "Redo: " + m_redoStack.back()->GetDescription();
    }

    void CommandHistory::Clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
    }

}  // namespace Olympe
