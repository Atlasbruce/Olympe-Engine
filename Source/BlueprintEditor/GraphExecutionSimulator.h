/**
 * @file GraphExecutionSimulator.h
 * @brief Simulates graph execution without runtime side effects.
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * GraphExecutionSimulator simulates the execution of a blueprint graph without
 * actually performing runtime operations. It tracks the execution path,
 * evaluates conditions, and detects logic errors. Used during blueprint
 * validation to ensure graphs are executable and to find logic issues.
 *
 * Phase 24.3 — Integration with blueprint validation system.
 * C++14 compliant.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cstdint>

#include "../TaskSystem/TaskGraphTemplate.h"
#include "../TaskSystem/LocalBlackboard.h"
#include "GraphExecutionTracer.h"
#include "BlueprintValidator.h"

namespace Olympe {

/**
 * @struct SimulationOptions
 * @brief Configuration options for graph simulation.
 */
struct SimulationOptions {
    int32_t maxStepsPerFrame = 1000;    ///< Maximum steps to prevent infinite loops
    int32_t maxSubGraphDepth = 10;      ///< Maximum nesting depth for subgraphs
    bool validateConditions = true;     ///< Check condition syntax
    bool validateDataFlow = true;       ///< Check data connections
    bool validateBranchPaths = true;    ///< Verify all branches lead somewhere
    std::string initialBlackboardJson;  ///< Optional initial blackboard values
};

/**
 * @class GraphExecutionSimulator
 * @brief Simulates blueprint graph execution for validation purposes.
 *
 * @details
 * Simulates graph execution to detect:
 * - Unreachable nodes (orphaned branches)
 * - Infinite loops or circular paths
 * - Invalid condition expressions
 * - Data flow mismatches
 * - Blocked execution paths (conditions that always fail)
 * - Missing connections
 *
 * The simulator does NOT execute actual AtomicTasks or modify runtime state.
 */
class GraphExecutionSimulator {
public:

    GraphExecutionSimulator();
    ~GraphExecutionSimulator();

    /**
     * @brief Simulates execution of a graph template.
     * @param tmpl           The TaskGraphTemplate to simulate.
     * @param options        Simulation configuration options.
     * @param outTracer      [out] The execution tracer containing results.
     * @return               Vector of validation errors found during simulation.
     */
    std::vector<ValidationError> SimulateExecution(const TaskGraphTemplate& tmpl,
                                                   const SimulationOptions& options,
                                                   GraphExecutionTracer& outTracer);

    /**
     * @brief Validates all branch nodes in a graph.
     * @param tmpl           The TaskGraphTemplate to validate.
     * @param outErrors      [out] Vector to append validation errors to.
     * @return               true if all branches are valid.
     */
    bool ValidateAllBranches(const TaskGraphTemplate& tmpl,
                             std::vector<ValidationError>& outErrors);

    /**
     * @brief Validates all data connections in a graph.
     * @param tmpl           The TaskGraphTemplate to validate.
     * @param outErrors      [out] Vector to append validation errors to.
     * @return               true if all data connections are valid.
     */
    bool ValidateDataConnections(const TaskGraphTemplate& tmpl,
                                 std::vector<ValidationError>& outErrors);

    /**
     * @brief Checks for unreachable nodes.
     * @param tmpl           The TaskGraphTemplate to check.
     * @param outErrors      [out] Vector to append validation errors to.
     * @return               Vector of unreachable node IDs.
     */
    std::vector<int32_t> FindUnreachableNodes(const TaskGraphTemplate& tmpl,
                                              std::vector<ValidationError>& outErrors);

    /**
     * @brief Checks for potential infinite loops or cycles.
     * @param tmpl           The TaskGraphTemplate to check.
     * @param outErrors      [out] Vector to append validation errors to.
     * @return               true if potential infinite loops were found.
     */
    bool DetectPotentialInfiniteLoops(const TaskGraphTemplate& tmpl,
                                      std::vector<ValidationError>& outErrors);

private:

    // Execution simulation helpers
    int32_t SimulateStep(const TaskGraphTemplate& tmpl,
                        int32_t currentNodeId,
                        LocalBlackboard& blackboard,
                        const SimulationOptions& options,
                        GraphExecutionTracer& tracer);

    /**
     * @brief Simulates a Branch node execution.
     * @return ID of the node to execute next, or NODE_INDEX_NONE.
     */
    int32_t HandleBranchSimulation(const TaskGraphTemplate& tmpl,
                                   int32_t nodeId,
                                   LocalBlackboard& blackboard,
                                   GraphExecutionTracer& tracer);

    /**
     * @brief Simulates a Switch node execution.
     */
    int32_t HandleSwitchSimulation(const TaskGraphTemplate& tmpl,
                                   int32_t nodeId,
                                   LocalBlackboard& blackboard,
                                   GraphExecutionTracer& tracer);

    /**
     * @brief Simulates a Sequence node execution.
     */
    int32_t HandleSequenceSimulation(const TaskGraphTemplate& tmpl,
                                     int32_t nodeId,
                                     LocalBlackboard& blackboard,
                                     GraphExecutionTracer& tracer);

    /**
     * @brief Simulates a While loop execution.
     */
    int32_t HandleWhileSimulation(const TaskGraphTemplate& tmpl,
                                  int32_t nodeId,
                                  LocalBlackboard& blackboard,
                                  GraphExecutionTracer& tracer);

    /**
     * @brief Gets the next node ID from an execution link.
     * @param tmpl          The template.
     * @param nodeId        Source node ID.
     * @param pinName       Execution output pin name.
     * @return              ID of target node, or NODE_INDEX_NONE.
     */
    int32_t GetNextNodeId(const TaskGraphTemplate& tmpl,
                         int32_t nodeId,
                         const std::string& pinName);

    /**
     * @brief Validates a condition expression.
     * @param nodeId        Node containing the condition.
     * @param expression    Condition text to validate.
     * @return              true if valid.
     */
    bool ValidateConditionExpression(int32_t nodeId,
                                     const std::string& expression);

    // Graph analysis helpers
    void BuildNodeReachabilityMap(const TaskGraphTemplate& tmpl,
                                  std::map<int32_t, bool>& reachable);

    void MarkReachableNodes(const TaskGraphTemplate& tmpl,
                           int32_t nodeId,
                           std::map<int32_t, bool>& reachable);

    // Data structures
    std::map<int32_t, int32_t> m_visitCount;  ///< Track visits per node to detect loops
    std::vector<int32_t> m_pathStack;          ///< Current execution path
};

} // namespace Olympe
