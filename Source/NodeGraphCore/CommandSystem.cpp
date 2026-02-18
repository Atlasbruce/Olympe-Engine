/**
 * @file CommandSystem.cpp
 * @brief Implementation of Command System
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "CommandSystem.h"

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// CommandStack Implementation
// ============================================================================

void CommandStack::ExecuteCommand(std::unique_ptr<ICommand> cmd)
{
    if (cmd == nullptr)
        return;
    
    cmd->Execute();
    
    m_undoStack.push_back(std::move(cmd));
    
    // Limit stack size
    if (m_undoStack.size() > MAX_STACK_SIZE)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
    
    // Clear redo stack when new command is executed
    m_redoStack.clear();
}

void CommandStack::Undo()
{
    if (!CanUndo())
        return;
    
    std::unique_ptr<ICommand> cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    
    cmd->Undo();
    
    m_redoStack.push_back(std::move(cmd));
}

void CommandStack::Redo()
{
    if (!CanRedo())
        return;
    
    std::unique_ptr<ICommand> cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    
    cmd->Execute();
    
    m_undoStack.push_back(std::move(cmd));
}

bool CommandStack::CanUndo() const
{
    return !m_undoStack.empty();
}

bool CommandStack::CanRedo() const
{
    return !m_redoStack.empty();
}

std::string CommandStack::GetUndoDescription() const
{
    if (m_undoStack.empty())
        return "";
    
    return m_undoStack.back()->GetDescription();
}

std::string CommandStack::GetRedoDescription() const
{
    if (m_redoStack.empty())
        return "";
    
    return m_redoStack.back()->GetDescription();
}

void CommandStack::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

} // namespace NodeGraph
} // namespace Olympe
