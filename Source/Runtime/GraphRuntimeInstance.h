/**
 * @file GraphRuntimeInstance.h
 * @brief Single-threaded execution instance for a TaskGraphTemplate.
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * @details
 * GraphRuntimeInstance wraps a TaskGraphTemplate and manages the runtime
 * state needed to execute it step-by-step.  It owns a RuntimeEnvironment
 * (Blackboard + dynamic pin values) and an execution stack.
 *
 * Execution model:
 *   - StartExecution() initialises the stack from the graph's entry node.
 *   - StepExecution() advances by one node, evaluating Branch conditions
 *     using ConditionEvaluator.
 *   - IsExecuting() returns false when the stack is empty (done or error).
 *
 * Branch node evaluation:
 *   - Each NodeConditionRef is evaluated in order against the environment.
 *   - Multiple conditions are combined using their LogicalOp (And / Or).
 *   - When the combined result is true the node's NextOnSuccess path is taken;
 *     otherwise NextOnFailure.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include "../TaskSystem/TaskGraphTemplate.h"
#include "RuntimeEnvironment.h"

namespace Olympe {

/**
 * @class GraphRuntimeInstance
 * @brief Manages single-threaded step-by-step execution of a TaskGraphTemplate.
 */
class GraphRuntimeInstance {
public:

    /**
     * @brief Constructs an instance bound to the given graph template.
     *
     * Call StartExecution() to begin.
     * @param graph Immutable graph template to execute.
     */
    explicit GraphRuntimeInstance(const TaskGraphTemplate& graph);

    // -----------------------------------------------------------------------
    // Execution control
    // -----------------------------------------------------------------------

    /**
     * @brief Initialises the execution stack and marks the instance as running.
     *
     * Uses the graph's RootNodeID as the first active node.  If RootNodeID is
     * NODE_INDEX_NONE the call is a no-op and IsExecuting() remains false.
     */
    void StartExecution();

    /**
     * @brief Advances execution by one node.
     *
     * Processes the top of the execution stack:
     *   - AtomicTask / other : follows NextOnSuccess (success path).
     *   - Branch             : evaluates conditions → Then or Else path.
     *
     * Returns true when execution is still active after this step.
     * Returns false when the stack becomes empty (execution complete or error).
     *
     * @return true while executing; false when done.
     */
    bool StepExecution();

    /**
     * @brief Returns true while there are nodes left to execute.
     */
    bool IsExecuting() const;

    // -----------------------------------------------------------------------
    // Environment
    // -----------------------------------------------------------------------

    /**
     * @brief Sets a Blackboard variable in the runtime environment.
     * @param key   Variable identifier.
     * @param value Float value.
     */
    void SetBlackboardVariable(const std::string& key, float value);

    /**
     * @brief Sets the runtime value of a dynamic data pin.
     * @param pinID Pin UUID.
     * @param value Float value delivered by the connected node.
     */
    void SetDynamicPinValue(const std::string& pinID, float value);

    // -----------------------------------------------------------------------
    // Breakpoints
    // -----------------------------------------------------------------------

    /**
     * @brief Registers a breakpoint on a node.
     *
     * When StepExecution() is about to process a node that has a breakpoint
     * the execution pauses (StepExecution returns false without consuming the
     * node) and IsBreakpointHit() returns true.
     *
     * @param nodeID Node to break on.
     */
    void AddBreakpoint(int32_t nodeID);

    /**
     * @brief Removes a previously registered breakpoint.
     * @param nodeID Node whose breakpoint to remove.
     */
    void RemoveBreakpoint(int32_t nodeID);

    /**
     * @brief Returns true when execution is paused at a breakpoint.
     */
    bool IsBreakpointHit() const;

    /**
     * @brief Resumes execution after a breakpoint.
     *
     * Clears the breakpoint-hit flag so the next StepExecution() call will
     * process the current node normally.
     */
    void ResumeFromBreakpoint();

    // -----------------------------------------------------------------------
    // Inspection
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the IDs of all nodes currently on the execution stack.
     */
    const std::vector<int32_t>& GetActiveNodeIDs() const;

    /**
     * @brief Returns the last error message produced during execution.
     *
     * Empty when no error has occurred.
     */
    const std::string& GetLastError() const;

private:

    /**
     * @brief Evaluates all NodeConditionRefs on @p node and returns the result.
     *
     * Conditions are combined with their LogicalOp (And / Or) in the order
     * they appear in node.conditionRefs.  The LogicalOp of the first entry
     * (LogicalOp::Start) is ignored.
     *
     * @return Combined boolean result; false on evaluation error.
     */
    bool EvaluateBranchConditions(const TaskNodeDefinition& node);

    const TaskGraphTemplate& m_graph;           ///< Bound graph template (not owned)
    RuntimeEnvironment       m_environment;     ///< Blackboard + pin values
    std::vector<int32_t>     m_executionStack;  ///< Active node IDs (front = next)
    bool                     m_isExecuting;     ///< True while stack is non-empty
    bool                     m_breakpointHit;   ///< True when paused at a breakpoint
    bool                     m_skipBreakpointOnce; ///< Skip breakpoint check for one step after Resume
    std::set<int32_t>        m_breakpoints;     ///< Registered breakpoint node IDs
    std::string              m_lastError;        ///< Last error from condition evaluation
};

} // namespace Olympe
