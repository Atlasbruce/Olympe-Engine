/**
 * @file UndoRedoStack.h
 * @brief Undo/Redo command stack for ATS Visual Scripting editor (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Implements the Command pattern for reversible graph editing operations.
 * Supported commands: AddNode, DeleteNode, MoveNode, AddConnection.
 * Stack size is bounded to MAX_STACK_SIZE (100) to cap memory usage.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstddef>

#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// ICommand — abstract base
// ============================================================================

/**
 * @interface ICommand
 * @brief Abstract base for all reversible editor commands.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Applies the command to the graph.
     */
    virtual void Execute(TaskGraphTemplate& graph) = 0;

    /**
     * @brief Reverses the command on the graph.
     */
    virtual void Undo(TaskGraphTemplate& graph) = 0;

    /**
     * @brief Returns a short human-readable description (e.g. "Add Node #3").
     */
    virtual std::string GetDescription() const = 0;
};

// ============================================================================
// Concrete commands
// ============================================================================

/**
 * @class AddNodeCommand
 * @brief Records an "add node" operation for undo/redo.
 */
class AddNodeCommand : public ICommand {
public:
    explicit AddNodeCommand(const TaskNodeDefinition& nodeDef);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    TaskNodeDefinition m_node;
};

/**
 * @class DeleteNodeCommand
 * @brief Records a "delete node" operation for undo/redo.
 *
 * @details
 * On Execute() the node is removed from the graph (and any exec/data
 * connections referencing it are also removed).
 * On Undo() the node and its connections are restored.
 */
class DeleteNodeCommand : public ICommand {
public:
    explicit DeleteNodeCommand(int32_t nodeID);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    int32_t                      m_nodeID;
    TaskNodeDefinition           m_savedNode;
    std::vector<ExecPinConnection> m_savedExecConns;
    std::vector<DataPinConnection> m_savedDataConns;
};

/**
 * @class MoveNodeCommand
 * @brief Records a "move node" (position change) for undo/redo.
 *
 * @details
 * Node position is stored in the Parameters map under the keys
 * "__posX" and "__posY" (editor-internal convention).
 */
class MoveNodeCommand : public ICommand {
public:
    MoveNodeCommand(int32_t nodeID, float oldX, float oldY, float newX, float newY);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    int32_t m_nodeID;
    float   m_oldX, m_oldY;
    float   m_newX, m_newY;

    static void SetNodePos(TaskGraphTemplate& graph, int32_t nodeID, float x, float y);
};

/**
 * @class AddConnectionCommand
 * @brief Records an "add exec connection" operation for undo/redo.
 */
class AddConnectionCommand : public ICommand {
public:
    explicit AddConnectionCommand(const ExecPinConnection& conn);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    ExecPinConnection m_conn;
};

/**
 * @class AddDataConnectionCommand
 * @brief Records an "add data connection" operation for undo/redo.
 */
class AddDataConnectionCommand : public ICommand {
public:
    explicit AddDataConnectionCommand(const DataPinConnection& conn);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    DataPinConnection m_conn;
};

/**
 * @class DeleteLinkCommand
 * @brief Records a "delete link" operation for undo/redo.
 *
 * @details
 * Supports both exec and data connections.  On Execute() the connection is
 * removed from the graph.  On Undo() it is restored.
 */
class DeleteLinkCommand : public ICommand {
public:
    explicit DeleteLinkCommand(const ExecPinConnection& conn);
    explicit DeleteLinkCommand(const DataPinConnection& conn);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph) override;
    std::string GetDescription() const override;

private:
    bool              m_isExecConn = true;
    ExecPinConnection m_savedExecConn;
    DataPinConnection m_savedDataConn;
};

// ============================================================================
// PropertyValue — typed value union for EditNodePropertyCommand (C++14)
// ============================================================================

/**
 * @struct PropertyValue
 * @brief Discriminated union of property value types (String / Float).
 *
 * C++14 compliant — no std::variant, no std::optional.
 */
struct PropertyValue {
    enum class Kind { String, Float, Int } kind;
    std::string strVal;
    float       floatVal;
    int         intVal;

    PropertyValue() : kind(Kind::String), floatVal(0.0f), intVal(0) {}

    static PropertyValue FromString(const std::string& s)
    {
        PropertyValue v; v.kind = Kind::String; v.strVal = s; return v;
    }

    static PropertyValue FromFloat(float f)
    {
        PropertyValue v; v.kind = Kind::Float; v.floatVal = f; return v;
    }

    static PropertyValue FromInt(int i)
    {
        PropertyValue v; v.kind = Kind::Int; v.intVal = i; return v;
    }
};

// ============================================================================
// EditNodePropertyCommand
// ============================================================================

/**
 * @class EditNodePropertyCommand
 * @brief Records a property edit on a single node for undo/redo.
 *
 * @details
 * Covers all editable fields exposed by the Properties panel:
 * NodeName, AtomicTaskID, DelaySeconds, ConditionID, BBKey,
 * MathOperator, SubGraphPath.
 *
 * Uses the "commit on release" pattern: the command is pushed only when the
 * ImGui widget loses focus after an edit (IsItemDeactivatedAfterEdit), so
 * a single undo entry is created per editing session, not one per keystroke.
 */
class EditNodePropertyCommand : public ICommand {
public:
    EditNodePropertyCommand(int32_t             nodeID,
                            const std::string&  propertyKey,
                            const PropertyValue& oldValue,
                            const PropertyValue& newValue);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph)    override;
    std::string GetDescription()     const override;

private:
    int32_t       m_nodeID;
    std::string   m_propertyKey;
    PropertyValue m_oldValue;
    PropertyValue m_newValue;

    /// Applies @p value to the named field of @p node.
    static void ApplyValue(TaskNodeDefinition&  node,
                           const std::string&   key,
                           const PropertyValue& value);
};

// ============================================================================
// AddDynamicPinCommand
// ============================================================================

/**
 * @class AddDynamicPinCommand
 * @brief Records "add dynamic exec-out pin" on a VSSequence or VSSwitch node for undo/redo.
 *
 * Execute() adds a new pin name (e.g. "Out_2" or "Case_2") to the node's
 * DynamicExecOutputPins vector and rebuilds the lookup cache.
 * Undo() removes the last pin.
 */
class AddDynamicPinCommand : public ICommand {
public:
    AddDynamicPinCommand(int32_t nodeID, const std::string& pinName);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph)    override;
    std::string GetDescription()     const override;

private:
    int32_t     m_nodeID;
    std::string m_pinName;
};

// ============================================================================
// RemoveExecPinCommand
// ============================================================================

/**
 * @class RemoveExecPinCommand
 * @brief Records "remove dynamic exec-out pin" on a VSSequence or VSSwitch node for undo/redo.
 *
 * Execute() removes the pin from DynamicExecOutputPins at the stored index,
 * and removes any ExecConnection originating from that pin.
 * Undo() re-inserts the pin at its original index and restores any saved
 * connection.
 *
 * @details
 * Stored data:
 *   - m_nodeID              : owning node
 *   - m_pinName             : pin name (e.g. "Out_1", "Case_2")
 *   - m_pinIndex            : 0-based index within DynamicExecOutputPins
 *   - m_linkedTargetNodeID  : target node of the outgoing link, or -1 if none
 *   - m_linkedTargetPinName : target pin name of the outgoing link
 */
class RemoveExecPinCommand : public ICommand {
public:
    RemoveExecPinCommand(int32_t            nodeID,
                         const std::string& pinName,
                         int                pinIndex,
                         int32_t            linkedTargetNodeID,
                         const std::string& linkedTargetPinName);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph)    override;
    std::string GetDescription()     const override;

private:
    int32_t     m_nodeID;
    std::string m_pinName;
    int         m_pinIndex;
    int32_t     m_linkedTargetNodeID;   ///< -1 if no outgoing link was present
    std::string m_linkedTargetPinName;
};

// ============================================================================
// EditParameterCommand (Phase 22-C)
// ============================================================================

/**
 * @class EditParameterCommand
 * @brief Records an edit to a single named parameter binding on a node.
 *
 * Stores the old and new ParameterBinding (type + literal value + variable name)
 * so that the change can be reversed via Undo() and re-applied via Execute().
 */
class EditParameterCommand : public ICommand {
public:
    EditParameterCommand(int32_t               nodeID,
                         const std::string&    paramName,
                         const ParameterBinding& oldBinding,
                         const ParameterBinding& newBinding);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph)    override;
    std::string GetDescription()     const override;

private:
    int32_t          m_nodeID;
    std::string      m_paramName;
    ParameterBinding m_oldBinding;
    ParameterBinding m_newBinding;
};

// ============================================================================
// EditNodePropertiesCommand (Phase 22-C)
// ============================================================================

/**
 * @class EditNodePropertiesCommand
 * @brief Records a batch edit of all parameter bindings on a node.
 *
 * Suitable for snapshotting the full parameter map before a complex edit and
 * restoring it atomically on Undo().
 */
class EditNodePropertiesCommand : public ICommand {
public:
    using ParameterMap = std::unordered_map<std::string, ParameterBinding>;

    EditNodePropertiesCommand(int32_t               nodeID,
                              const ParameterMap&   oldParams,
                              const ParameterMap&   newParams);

    void Execute(TaskGraphTemplate& graph) override;
    void Undo(TaskGraphTemplate& graph)    override;
    std::string GetDescription()     const override;

private:
    int32_t      m_nodeID;
    ParameterMap m_oldParams;
    ParameterMap m_newParams;
};

// ============================================================================
// UndoRedoStack
// ============================================================================

/**
 * @class UndoRedoStack
 * @brief Bounded stack of reversible editor commands.
 *
 * @details
 * Usage:
 * @code
 *   UndoRedoStack stack;
 *   stack.PushCommand(std::unique_ptr<ICommand>(new AddNodeCommand(def)));
 *   if (stack.CanUndo()) stack.Undo(graph);
 *   if (stack.CanRedo()) stack.Redo(graph);
 * @endcode
 *
 * When more than MAX_STACK_SIZE commands are pushed, the oldest undo entry is
 * dropped (FIFO eviction).  Pushing any new command clears the redo stack.
 */
class UndoRedoStack {
public:

    /// Maximum number of undo entries kept in memory.
    static const std::size_t MAX_STACK_SIZE = 100u;

    UndoRedoStack();

    /**
     * @brief Executes the command on @p graph, then pushes it onto the undo
     *        stack.  Clears the redo stack.
     *
     * @param cmd   Owning pointer to the command.  Must not be null.
     * @param graph Target graph on which Execute() is called immediately.
     */
    void PushCommand(std::unique_ptr<ICommand> cmd, TaskGraphTemplate& graph);

    /**
     * @brief Undoes the last command, moving it to the redo stack.
     *
     * @pre CanUndo() == true
     */
    void Undo(TaskGraphTemplate& graph);

    /**
     * @brief Re-applies the last undone command, moving it back to the undo
     *        stack.
     *
     * @pre CanRedo() == true
     */
    void Redo(TaskGraphTemplate& graph);

    /**
     * @brief Clears both undo and redo stacks (e.g. after save).
     */
    void Clear();

    /** @brief Returns true if there is at least one command to undo. */
    bool CanUndo() const;

    /** @brief Returns true if there is at least one command to redo. */
    bool CanRedo() const;

    /** @brief Returns the number of commands currently on the undo stack. */
    std::size_t UndoSize() const;

    /** @brief Returns the number of commands currently on the redo stack. */
    std::size_t RedoSize() const;

    /**
     * @brief Returns the description of the top undo command, or "" if empty.
     */
    std::string PeekUndoDescription() const;

    /**
     * @brief Returns the description of the top redo command, or "" if empty.
     */
    std::string PeekRedoDescription() const;

private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;
};

} // namespace Olympe
