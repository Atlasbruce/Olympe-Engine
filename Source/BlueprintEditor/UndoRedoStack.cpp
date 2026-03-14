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
// AddDataConnectionCommand
// ============================================================================

AddDataConnectionCommand::AddDataConnectionCommand(const DataPinConnection& conn)
    : m_conn(conn)
{
}

void AddDataConnectionCommand::Execute(TaskGraphTemplate& graph)
{
    graph.DataConnections.push_back(m_conn);
}

void AddDataConnectionCommand::Undo(TaskGraphTemplate& graph)
{
    auto it = std::remove_if(graph.DataConnections.begin(), graph.DataConnections.end(),
        [this](const DataPinConnection& dc)
        {
            return dc.SourceNodeID  == m_conn.SourceNodeID  &&
                   dc.TargetNodeID  == m_conn.TargetNodeID  &&
                   dc.SourcePinName == m_conn.SourcePinName &&
                   dc.TargetPinName == m_conn.TargetPinName;
        });
    graph.DataConnections.erase(it, graph.DataConnections.end());
}

std::string AddDataConnectionCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Add Data Connection #" << m_conn.SourceNodeID << "." << m_conn.SourcePinName
       << " -> #" << m_conn.TargetNodeID << "." << m_conn.TargetPinName;
    return ss.str();
}

// ============================================================================
// DeleteLinkCommand
// ============================================================================

DeleteLinkCommand::DeleteLinkCommand(const ExecPinConnection& conn)
    : m_isExecConn(true), m_savedExecConn(conn), m_savedDataConn()
{
}

DeleteLinkCommand::DeleteLinkCommand(const DataPinConnection& conn)
    : m_isExecConn(false), m_savedExecConn(), m_savedDataConn(conn)
{
}

void DeleteLinkCommand::Execute(TaskGraphTemplate& graph)
{
    if (m_isExecConn)
    {
        auto it = std::remove_if(graph.ExecConnections.begin(), graph.ExecConnections.end(),
            [this](const ExecPinConnection& ec)
            {
                return ec.SourceNodeID  == m_savedExecConn.SourceNodeID  &&
                       ec.TargetNodeID  == m_savedExecConn.TargetNodeID  &&
                       ec.SourcePinName == m_savedExecConn.SourcePinName &&
                       ec.TargetPinName == m_savedExecConn.TargetPinName;
            });
        graph.ExecConnections.erase(it, graph.ExecConnections.end());
    }
    else
    {
        auto it = std::remove_if(graph.DataConnections.begin(), graph.DataConnections.end(),
            [this](const DataPinConnection& dc)
            {
                return dc.SourceNodeID  == m_savedDataConn.SourceNodeID  &&
                       dc.TargetNodeID  == m_savedDataConn.TargetNodeID  &&
                       dc.SourcePinName == m_savedDataConn.SourcePinName &&
                       dc.TargetPinName == m_savedDataConn.TargetPinName;
            });
        graph.DataConnections.erase(it, graph.DataConnections.end());
    }
    graph.BuildLookupCache();
}

void DeleteLinkCommand::Undo(TaskGraphTemplate& graph)
{
    if (m_isExecConn)
        graph.ExecConnections.push_back(m_savedExecConn);
    else
        graph.DataConnections.push_back(m_savedDataConn);
    graph.BuildLookupCache();
}

std::string DeleteLinkCommand::GetDescription() const
{
    std::ostringstream ss;
    if (m_isExecConn)
    {
        ss << "Delete Exec Link " << m_savedExecConn.SourceNodeID
           << "." << m_savedExecConn.SourcePinName
           << " -> " << m_savedExecConn.TargetNodeID
           << "." << m_savedExecConn.TargetPinName;
    }
    else
    {
        ss << "Delete Data Link " << m_savedDataConn.SourceNodeID
           << "." << m_savedDataConn.SourcePinName
           << " -> " << m_savedDataConn.TargetNodeID
           << "." << m_savedDataConn.TargetPinName;
    }
    return ss.str();
}

// ============================================================================
// EditNodePropertyCommand
// ============================================================================

EditNodePropertyCommand::EditNodePropertyCommand(int32_t              nodeID,
                                                  const std::string&   propertyKey,
                                                  const PropertyValue& oldValue,
                                                  const PropertyValue& newValue)
    : m_nodeID(nodeID)
    , m_propertyKey(propertyKey)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{
}

void EditNodePropertyCommand::ApplyValue(TaskNodeDefinition&  node,
                                          const std::string&   key,
                                          const PropertyValue& value)
{
    if (key == "NodeName")
        node.NodeName = value.strVal;
    else if (key == "AtomicTaskID")
        node.AtomicTaskID = value.strVal;
    else if (key == "ConditionID")
        node.ConditionID = value.strVal;
    else if (key == "BBKey")
        node.BBKey = value.strVal;
    else if (key == "MathOperator")
        node.MathOperator = value.strVal;
    else if (key == "SubGraphPath")
        node.SubGraphPath = value.strVal;
    else if (key == "DelaySeconds")
        node.DelaySeconds = value.floatVal;
}

void EditNodePropertyCommand::Execute(TaskGraphTemplate& graph)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            ApplyValue(graph.Nodes[i], m_propertyKey, m_newValue);
            break;
        }
    }
    graph.BuildLookupCache();
}

void EditNodePropertyCommand::Undo(TaskGraphTemplate& graph)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            ApplyValue(graph.Nodes[i], m_propertyKey, m_oldValue);
            break;
        }
    }
    graph.BuildLookupCache();
}

std::string EditNodePropertyCommand::GetDescription() const
{
    std::ostringstream ss;
    ss << "Edit Node #" << m_nodeID << " " << m_propertyKey;
    return ss.str();
}

// ============================================================================
// AddDynamicPinCommand
// ============================================================================

AddDynamicPinCommand::AddDynamicPinCommand(int32_t nodeID, const std::string& pinName)
    : m_nodeID(nodeID)
    , m_pinName(pinName)
{
}

void AddDynamicPinCommand::Execute(TaskGraphTemplate& graph)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            graph.Nodes[i].DynamicExecOutputPins.push_back(m_pinName);
            break;
        }
    }
    graph.BuildLookupCache();
}

void AddDynamicPinCommand::Undo(TaskGraphTemplate& graph)
{
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            std::vector<std::string>& pins = graph.Nodes[i].DynamicExecOutputPins;
            if (!pins.empty() && pins.back() == m_pinName)
                pins.pop_back();
            break;
        }
    }
    graph.BuildLookupCache();
}

std::string AddDynamicPinCommand::GetDescription() const
{
    return "Add Pin " + m_pinName + " to node #" + std::to_string(m_nodeID);
}

// ============================================================================
// RemoveExecPinCommand
// ============================================================================

RemoveExecPinCommand::RemoveExecPinCommand(int32_t            nodeID,
                                            const std::string& pinName,
                                            int                pinIndex,
                                            int32_t            linkedTargetNodeID,
                                            const std::string& linkedTargetPinName)
    : m_nodeID(nodeID)
    , m_pinName(pinName)
    , m_pinIndex(pinIndex)
    , m_linkedTargetNodeID(linkedTargetNodeID)
    , m_linkedTargetPinName(linkedTargetPinName)
{
}

void RemoveExecPinCommand::Execute(TaskGraphTemplate& graph)
{
    // Remove pin from DynamicExecOutputPins
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            std::vector<std::string>& pins = graph.Nodes[i].DynamicExecOutputPins;
            if (m_pinIndex >= 0 && m_pinIndex < static_cast<int>(pins.size()))
                pins.erase(pins.begin() + m_pinIndex);
            break;
        }
    }

    // Remove any ExecConnection originating from this pin
    auto it = std::remove_if(graph.ExecConnections.begin(), graph.ExecConnections.end(),
        [this](const ExecPinConnection& ec)
        {
            return ec.SourceNodeID == m_nodeID && ec.SourcePinName == m_pinName;
        });
    graph.ExecConnections.erase(it, graph.ExecConnections.end());

    graph.BuildLookupCache();
}

void RemoveExecPinCommand::Undo(TaskGraphTemplate& graph)
{
    // Re-insert pin at its original index
    for (size_t i = 0; i < graph.Nodes.size(); ++i)
    {
        if (graph.Nodes[i].NodeID == m_nodeID)
        {
            std::vector<std::string>& pins = graph.Nodes[i].DynamicExecOutputPins;
            int insertAt = m_pinIndex;
            if (insertAt < 0)
                insertAt = 0;
            if (insertAt > static_cast<int>(pins.size()))
                insertAt = static_cast<int>(pins.size());
            pins.insert(pins.begin() + insertAt, m_pinName);
            break;
        }
    }

    // Restore the outgoing link if one existed
    if (m_linkedTargetNodeID != -1)
    {
        ExecPinConnection conn;
        conn.SourceNodeID  = m_nodeID;
        conn.SourcePinName = m_pinName;
        conn.TargetNodeID  = m_linkedTargetNodeID;
        conn.TargetPinName = m_linkedTargetPinName;
        graph.ExecConnections.push_back(conn);
    }

    graph.BuildLookupCache();
}

std::string RemoveExecPinCommand::GetDescription() const
{
    return "Remove Pin " + m_pinName + " from node #" + std::to_string(m_nodeID);
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
