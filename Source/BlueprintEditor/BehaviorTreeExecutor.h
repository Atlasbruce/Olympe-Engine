/**
 * @file BehaviorTreeExecutor.h
 * @brief Native BehaviorTree execution for simulation and tracing
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * Executes a BehaviorTree using its native execution model and records
 * trace events for debugging and validation. Unlike GraphExecutionSimulator
 * which is designed for VisualScript graphs, this executor understands
 * BehaviorTree semantics (Selector, Sequence, Condition, Action, etc.)
 *
 * C++14 compliant.
 */

#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "../AI/BehaviorTree.h"
#include "GraphExecutionTracer.h"

namespace Olympe {

/**
 * @class BehaviorTreeExecutor
 * @brief Executes a BehaviorTree and collects trace information.
 *
 * This executor simulates BehaviorTree execution without modifying runtime state.
 * It records each node execution, condition evaluation, and branch decision.
 */
class BehaviorTreeExecutor {
public:
    BehaviorTreeExecutor();
    ~BehaviorTreeExecutor();

    /**
     * @brief Execute a BehaviorTree and collect trace information.
     * @param btAsset       The BehaviorTree asset to execute
     * @param outTracer     [out] Execution trace collection
     * @return             Execution status (Success, Failure, or Running)
     */
    BTStatus ExecuteTree(const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer);

private:
    /**
     * @brief Recursively execute a single BehaviorTree node.
     * @param nodeId        The node to execute
     * @param btAsset       Reference to tree asset (for node lookups)
     * @param outTracer     Trace collection object
     * @return             Node execution status
     */
    BTStatus ExecuteNode(uint32_t nodeId, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer);

    /**
     * @brief Execute a Selector (OR) composite node.
     * @details A Selector succeeds if ANY child succeeds. Stops on first success.
     */
    BTStatus ExecuteSelector(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer);

    /**
     * @brief Execute a Sequence (AND) composite node.
     * @details A Sequence succeeds only if ALL children succeed. Stops on first failure.
     */
    BTStatus ExecuteSequence(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer);

    /**
     * @brief Execute a Condition leaf node.
     * @details Evaluates condition and returns Success (true) or Failure (false).
     */
    BTStatus ExecuteCondition(const BTNode& node, GraphExecutionTracer& outTracer);

    /**
     * @brief Execute an Action leaf node.
     * @details Actions always succeed in simulation (no runtime effects).
     */
    BTStatus ExecuteAction(const BTNode& node, GraphExecutionTracer& outTracer);

    /**
     * @brief Apply a Decorator (Inverter, Repeater) to a child node result.
     */
    BTStatus ExecuteDecorator(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer);

    /**
     * @brief Convert BTStatus to string for logging.
     */
    static const char* StatusToString(BTStatus status);

    int m_maxDepth = 0;        ///< Track recursion depth to detect cycles
    int m_executedNodes = 0;   ///< Count of executed nodes
};

} // namespace Olympe
