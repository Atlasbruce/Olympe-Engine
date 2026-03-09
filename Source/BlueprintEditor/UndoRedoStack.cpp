/**
 * @file UndoRedoStack.cpp
 * @brief Implementation of the undo/redo command stack (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "UndoRedoStack.h"

#include <algorithm>
#include <sstream>

namespace Olympe {

// ============================================================================
// AddNodeCommand
// ============================================================================

AddNodeCommand::AddNodeCommand(const TaskNodeDefinition& nodeDef)
    : m_node(nodeDef)
{
}

void AddNodeCommand::Execute(TaskGraphTemplate& graph)
{
    graph.Nodes.push_back(m_node);
    graph.BuildLookupCache();
}

void AddNodeCommand::Undo(TaskGraphTemplate& graph)
{
    auto it = std::remove_if(graph.Nodes.begin(), graph.Nodes.end(),
        [this](const TaskNodeDefinition& n) { return n.NodeID == m_node.NodeID; });
    graph.Nodes.erase(it, graph.Nodes.end());
    graph.BuildLookupCache();
}

std::string AddNodeCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Add Node #" << m_node.NodeID << " (" << m_node.NodeName << ")";
    return ss.str();
}

// ============================================================================
// DeleteNodeCommand
// ============================================================================

DeleteNodeCommand::DeleteNodeCommand(int32_t nodeID)
    : m_nodeID(nodeID)
{
}

void DeleteNodeCommand::Execute(TaskGraphTemplate& graph)
{
    // Save the node before deletion
    for (const auto& n : graph.Nodes)
    {
        if (n.NodeID == m_nodeID)
        {
            m_savedNode = n;
            break;
        }
    }

    // Save exec connections referencing this node
    m_savedExecConns.clear();
    for (const auto& ec : graph.ExecConnections)
    {
        if (ec.SourceNodeID == m_nodeID || ec.TargetNodeID == m_nodeID)
            m_savedExecConns.push_back(ec);
    }

    // Save data connections referencing this node
    m_savedDataConns.clear();
    for (const auto& dc : graph.DataConnections)
    {
        if (dc.SourceNodeID == m_nodeID || dc.TargetNodeID == m_nodeID)
            m_savedDataConns.push_back(dc);
    }

    // Remove the node
    auto it = std::remove_if(graph.Nodes.begin(), graph.Nodes.end(),
        [this](const TaskNodeDefinition& n) { return n.NodeID == m_nodeID; });
    graph.Nodes.erase(it, graph.Nodes.end());

    // Remove exec connections referencing this node
    auto eit = std::remove_if(graph.ExecConnections.begin(), graph.ExecConnections.end(),
        [this](const ExecPinConnection& ec)
        { return ec.SourceNodeID == m_nodeID || ec.TargetNodeID == m_nodeID; });
    graph.ExecConnections.erase(eit, graph.ExecConnections.end());

    // Remove data connections referencing this node
    auto dit = std::remove_if(graph.DataConnections.begin(), graph.DataConnections.end(),
        [this](const DataPinConnection& dc)
        { return dc.SourceNodeID == m_nodeID || dc.TargetNodeID == m_nodeID; });
    graph.DataConnections.erase(dit, graph.DataConnections.end());

    graph.BuildLookupCache();
}

void DeleteNodeCommand::Undo(TaskGraphTemplate& graph)
{
    // Restore node
    graph.Nodes.push_back(m_savedNode);

    // Restore connections
    for (const auto& ec : m_savedExecConns)
        graph.ExecConnections.push_back(ec);
    for (const auto& dc : m_savedDataConns)
        graph.DataConnections.push_back(dc);

    graph.BuildLookupCache();
}

std::string DeleteNodeCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Delete Node #" << m_nodeID;
    return ss.str();
}

// ============================================================================
// MoveNodeCommand
// ============================================================================

MoveNodeCommand::MoveNodeCommand(int32_t nodeID,
                                  float oldX, float oldY,
                                  float newX, float newY)
    : m_nodeID(nodeID)
    , m_oldX(oldX), m_oldY(oldY)
    , m_newX(newX), m_newY(newY)
{
}

void MoveNodeCommand::SetNodePos(TaskGraphTemplate& graph,
                                  int32_t nodeID, float x, float y)
{
    for (auto& n : graph.Nodes)
    {
        if (n.NodeID == nodeID)
        {
            // Encode position in Parameters map under "__posX" / "__posY"
            ParameterBinding bx, by;
            bx.Type = ParameterBindingType::Literal;
            bx.LiteralValue = TaskValue(x);
            by.Type = ParameterBindingType::Literal;
            by.LiteralValue = TaskValue(y);
            n.Parameters["__posX"] = bx;
            n.Parameters["__posY"] = by;
            break;
        }
    }
}

void MoveNodeCommand::Execute(TaskGraphTemplate& graph)
{
    SetNodePos(graph, m_nodeID, m_newX, m_newY);
}

void MoveNodeCommand::Undo(TaskGraphTemplate& graph)
{
    SetNodePos(graph, m_nodeID, m_oldX, m_oldY);
}

std::string MoveNodeCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Move Node #" << m_nodeID
       << " (" << m_oldX << "," << m_oldY
       << ") -> (" << m_newX << "," << m_newY << ")";
    return ss.str();
}

// ============================================================================
// AddConnectionCommand
// ============================================================================

AddConnectionCommand::AddConnectionCommand(const ExecPinConnection& conn)
    : m_conn(conn)
{
}

void AddConnectionCommand::Execute(TaskGraphTemplate& graph)
{
    graph.ExecConnections.push_back(m_conn);
}

void AddConnectionCommand::Undo(TaskGraphTemplate& graph)
{
    auto it = std::remove_if(graph.ExecConnections.begin(), graph.ExecConnections.end(),
        [this](const ExecPinConnection& ec)
        {
            return ec.SourceNodeID  == m_conn.SourceNodeID  &&
                   ec.TargetNodeID  == m_conn.TargetNodeID  &&
                   ec.SourcePinName == m_conn.SourcePinName &&
                   ec.TargetPinName == m_conn.TargetPinName;
        });
    graph.ExecConnections.erase(it, graph.ExecConnections.end());
}

std::string AddConnectionCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Add Connection #" << m_conn.SourceNodeID << "." << m_conn.SourcePinName
       << " -> #" << m_conn.TargetNodeID << "." << m_conn.TargetPinName;
    return ss.str();
}

// ============================================================================
// UndoRedoStack
// ============================================================================

UndoRedoStack::UndoRedoStack()
{
}

void UndoRedoStack::PushCommand(std::unique_ptr<ICommand> cmd,
                                 TaskGraphTemplate& graph)
{
    if (!cmd) return;

    // Execute the command on the graph
    cmd->Execute(graph);

    // Evict oldest entry if the stack is full
    if (m_undoStack.size() >= MAX_STACK_SIZE)
    {
        m_undoStack.erase(m_undoStack.begin());
    }

    m_undoStack.push_back(std::move(cmd));

    // Any new command clears the redo stack
    m_redoStack.clear();
}

void UndoRedoStack::Undo(TaskGraphTemplate& graph)
{
    if (m_undoStack.empty()) return;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    cmd->Undo(graph);

    m_redoStack.push_back(std::move(cmd));
}

void UndoRedoStack::Redo(TaskGraphTemplate& graph)
{
    if (m_redoStack.empty()) return;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    cmd->Execute(graph);

    m_undoStack.push_back(std::move(cmd));
}

void UndoRedoStack::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

bool UndoRedoStack::CanUndo() const
{
    return !m_undoStack.empty();
}

bool UndoRedoStack::CanRedo() const
{
    return !m_redoStack.empty();
}

std::size_t UndoRedoStack::UndoSize() const
{
    return m_undoStack.size();
}

std::size_t UndoRedoStack::RedoSize() const
{
    return m_redoStack.size();
}

std::string UndoRedoStack::PeekUndoDescription() const
{
    if (m_undoStack.empty()) return "";
    return m_undoStack.back()->GetDescription();
}

std::string UndoRedoStack::PeekRedoDescription() const
{
    if (m_redoStack.empty()) return "";
    return m_redoStack.back()->GetDescription();
}

} // namespace Olympe
