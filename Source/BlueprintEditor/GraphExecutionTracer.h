/**
 * @file GraphExecutionTracer.h
 * @brief Graph execution tracing for simulation and validation.
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * GraphExecutionTracer records each step of graph execution during simulation,
 * tracking node visits, condition evaluations, and execution flow for debugging
 * and validation purposes.
 *
 * Phase 24.3 — Integration with execution simulation for blueprint validation.
 * C++14 compliant.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

/**
 * @enum ExecutionEventType
 * @brief Types of events that can be recorded during graph execution trace.
 */
enum class ExecutionEventType {
    NodeEntered,          ///< Execution entered a node
    NodeExited,           ///< Execution exited a node
    ConditionEvaluated,   ///< A condition was evaluated
    DataPinResolved,      ///< A data pin was resolved
    BranchTaken,          ///< A branch condition was true
    BranchNotTaken,       ///< A branch condition was false
    ErrorOccurred,        ///< An error happened during execution
    ExecutionBlocked,     ///< Execution was blocked (dead-end path, etc.)
    ExecutionCompleted    ///< Graph execution completed
};

/**
 * @struct ExecutionEvent
 * @brief A single event recorded during graph execution trace.
 */
struct ExecutionEvent {
    ExecutionEventType type;
    int32_t nodeId;
    std::string nodeName;
    std::string nodeType;           ///< TaskNodeType as string
    std::string message;            ///< Additional context/details
    std::string conditionExpression; ///< For condition evaluation events
    bool conditionResult;           ///< Result of condition (if applicable)
    float timestamp;                ///< Time in execution (frame count, step count)
    int32_t stepNumber;             ///< Sequential step number
    int32_t depth;                  ///< Call stack depth (for subgraphs)

    ExecutionEvent() : type(ExecutionEventType::NodeEntered), nodeId(-1),
                      conditionResult(false), timestamp(0.0f), 
                      stepNumber(0), depth(0) {}
};

/**
 * @class GraphExecutionTracer
 * @brief Records execution trace during graph simulation.
 *
 * @details
 * Used by GraphExecutionSimulator to capture execution flow. Can be queried
 * to understand why a graph failed or which path was taken.
 */
class GraphExecutionTracer {
public:

    GraphExecutionTracer();
    ~GraphExecutionTracer();

    /**
     * @brief Clears all recorded events and resets state.
     */
    void Reset();

    /**
     * @brief Records that execution entered a node.
     * @param nodeId        ID of the node being entered.
     * @param nodeName      Display name of the node.
     * @param nodeType      Type of the node (from TaskNodeType).
     */
    void RecordNodeEntered(int32_t nodeId, const std::string& nodeName, const std::string& nodeType);

    /**
     * @brief Records that execution exited a node successfully.
     * @param nodeId        ID of the node being exited.
     * @param nextNodeId    ID of the next node in execution (NODE_INDEX_NONE if end).
     */
    void RecordNodeExited(int32_t nodeId, int32_t nextNodeId);

    /**
     * @brief Records a condition evaluation result.
     * @param nodeId        ID of the node containing the condition.
     * @param expression    String representation of the condition.
     * @param result        Whether the condition evaluated to true.
     * @param message       Optional additional details.
     */
    void RecordConditionEvaluated(int32_t nodeId, const std::string& expression,
                                  bool result, const std::string& message = "");

    /**
     * @brief Records that a branch was taken based on condition.
     * @param nodeId        ID of the branch node.
     * @param branchName    Name of the branch taken ("True", "False", case name, etc.).
     * @param nextNodeId    ID of the node being branched to.
     */
    void RecordBranchTaken(int32_t nodeId, const std::string& branchName, int32_t nextNodeId);

    /**
     * @brief Records an execution error.
     * @param nodeId        ID of the node where error occurred (NODE_INDEX_NONE for graph-level).
     * @param nodeName      Name of the node (if applicable).
     * @param errorMessage  Description of the error.
     * @param severity      "Error" or "Critical".
     */
    void RecordError(int32_t nodeId, const std::string& nodeName,
                     const std::string& errorMessage, const std::string& severity = "Error");

    /**
     * @brief Records that execution was blocked (dead-end, etc.).
     * @param nodeId        ID of the node causing the block.
     * @param reason        Why execution was blocked.
     */
    void RecordExecutionBlocked(int32_t nodeId, const std::string& reason);

    /**
     * @brief Records data pin resolution.
     * @param nodeId        ID of the node.
     * @param pinName       Name of the pin.
     * @param value         Resolved value as string.
     */
    void RecordDataPinResolved(int32_t nodeId, const std::string& pinName, const std::string& value);

    /**
     * @brief Records graph execution completion.
     * @param success       true if completed successfully, false if ended with error.
     * @param message       Summary message.
     */
    void RecordExecutionCompleted(bool success, const std::string& message);

    /**
     * @brief Returns all recorded events.
     */
    const std::vector<ExecutionEvent>& GetEvents() const { return m_events; }

    /**
     * @brief Returns the number of steps executed.
     */
    int32_t GetStepCount() const { return m_stepCount; }

    /**
     * @brief Returns true if any errors were recorded.
     */
    bool HasErrors() const { return !m_errorNodes.empty(); }

    /**
     * @brief Returns the set of node IDs that encountered errors.
     */
    const std::vector<int32_t>& GetErrorNodes() const { return m_errorNodes; }

    /**
     * @brief Returns a formatted trace log as a multi-line string.
     */
    std::string GetTraceLog() const;

    /**
     * @brief Returns a summary of the execution (steps, errors, etc.).
     */
    std::string GetExecutionSummary() const;

private:

    void AddEvent(const ExecutionEvent& event);

    std::vector<ExecutionEvent> m_events;
    std::vector<int32_t> m_errorNodes;
    int32_t m_stepCount = 0;
    bool m_executionComplete = false;
    bool m_executionSuccess = false;
};

} // namespace Olympe
