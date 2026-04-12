/**
 * @file BehaviorTreeGraphAdapter.h
 * @brief Adapter layer for converting BehaviorTree graphs to generic TaskGraphTemplate format
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * Implements the Adapter pattern to convert hierarchical BehaviorTree structures into
 * a flat graph representation compatible with the generic GraphExecutionSimulator framework.
 * This eliminates code duplication and enables both VisualScript and BehaviorTree to share
 * the same simulation infrastructure.
 *
 * Phase: BT-SIM-001 (BehaviorTree Graph Simulation - Framework Generic)
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include "../AI/BehaviorTree.h"
#include "../TaskSystem/TaskGraphTemplate.h"
#include "../BlueprintEditor/GraphExecutionTracer.h"

namespace Olympe {

/**
 * @class BehaviorTreeGraphAdapter
 * @brief Converts BehaviorTree structures to generic TaskGraphTemplate format for simulation.
 *
 * @details
 * This adapter layer enables the reuse of the generic GraphExecutionSimulator framework
 * for BehaviorTree graphs without code duplication. It performs bidirectional conversion:
 *
 * Forward (BehaviorTree → Graph):
 * - Converts hierarchical BTNode array into flat TaskGraphTemplate
 * - Maps BTNode.childIds relationships to explicit ExecPinConnection entries
 * - Preserves BehaviorTree-specific metadata where applicable
 * - Validates tree structure before conversion
 *
 * Reverse (Trace → BehaviorTree Format):
 * - Post-processes generic ExecutionEvent trace from GraphExecutionSimulator
 * - Adds BehaviorTree-specific formatting (status symbols, keywords, indentation)
 * - Enhances trace with hierarchical context from original BTNode structure
 *
 * Example Usage:
 * @code
 *   BehaviorTreeAsset myTree = ...;
 *   
 *   // Convert to graph format
 *   auto taskGraph = BehaviorTreeGraphAdapter::AdaptToTaskGraph(myTree);
 *   
 *   // Simulate using generic framework
 *   GraphExecutionSimulator simulator;
 *   GraphExecutionTracer tracer;
 *   SimulationOptions options;
 *   simulator.SimulateExecution(*taskGraph, options, tracer);
 *   
 *   // Format trace back to BehaviorTree context
 *   std::string btTrace = BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree(
 *       tracer, myTree);
 *   
 *   // Display via existing panel
 *   executionPanel->DisplayResults(tracer, btTrace);
 * @endcode
 */
class BehaviorTreeGraphAdapter {
public:

    /**
     * @brief Converts a BehaviorTreeAsset to TaskGraphTemplate format.
     *
     * @param btAsset   Source BehaviorTree asset containing hierarchical BTNode structure.
     * @return          Unique pointer to TaskGraphTemplate suitable for GraphExecutionSimulator.
     *                  Returns nullptr if validation fails.
     *
     * @details
     * Performs the following transformations:
     * 1. Validates tree structure (no orphaned nodes, valid cycles, etc.)
     * 2. Iterates BTNode[] and creates TaskNodeDefinition for each
     * 3. Maps BTNode.type enum to appropriate TaskNodeType:
     *    - Selector → Selector (BT type 0)
     *    - Sequence → Sequence (BT type 1)
     *    - Condition → AtomicTask with condition ID
     *    - Action → AtomicTask with action ID
     *    - Inverter/Repeater → Decorator
     * 4. Converts hierarchical childIds relationships to explicit ExecPinConnection entries:
     *    - Each child becomes a connection with SourceNodeID=parent, TargetNodeID=child
     *    - Pin names: "Control" (output) → "In" (input)
     * 5. Returns fully populated TaskGraphTemplate ready for simulation
     *
     * @note
     * - The conversion is one-way: BT → Graph (graph is flattened, hierarchy inferred at runtime)
     * - All BTNode properties are preserved; nothing is lost
     * - BT-specific details (condition type, action type) stored in AtomicTaskID field
     */
    static std::unique_ptr<TaskGraphTemplate> AdaptToTaskGraph(
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Formats generic execution trace back to BehaviorTree-specific context.
     *
     * @param tracer    GraphExecutionTracer containing generic ExecutionEvent trace.
     * @param btAsset   Original BehaviorTreeAsset for context and hierarchy information.
     * @return          Human-readable string with BehaviorTree-specific formatting.
     *
     * @details
     * Post-processes the generic execution trace from GraphExecutionSimulator with
     * BehaviorTree-specific enhancements:
     *
     * 1. Status Symbol Mapping (from event message keywords):
     *    - SUCCESS → "✓ " (check mark)
     *    - FAILURE → "✗ " (cross mark)
     *    - RUNNING → "⊙ " (circle - in-progress)
     *    - Other → (blank) - informational message
     *
     * 2. Type Name Replacement:
     *    - Maps node types back to BT-specific names
     *
     * 3. Hierarchical Indentation:
     *    - Calculates node depth in original BT hierarchy
     *    - Indents output with (depth * 2) spaces for tree-like visualization
     *
     * @note
     * - Called AFTER simulation completes (post-processing layer)
     * - Does not modify tracer or btAsset; both remain const
     * - Output is human-readable string (for logging, UI display, etc.)
     */
    static std::string FormatTraceForBehaviorTree(
        const GraphExecutionTracer& tracer,
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Validates BehaviorTree structure before conversion.
     *
     * @param btAsset   BehaviorTreeAsset to validate.
     * @return          true if valid for conversion; false otherwise.
     *
     * @details
     * Performs structural validation to catch issues before conversion:
     * - Node ID uniqueness
     * - Child references validity
     * - Cycle detection
     * - Node type consistency
     */
    static bool ValidateTreeStructure(
        const BehaviorTreeAsset& btAsset);

private:

    /**
     * @brief Helper: Recursively adds a node and its children to the graph.
     */
    static void AddNodeToGraph(
        const BTNode& btNode,
        const BehaviorTreeAsset& btAsset,
        TaskGraphTemplate& outGraph,
        std::map<uint32_t, int32_t>& btToGraphIdMap);

    /**
     * @brief Helper: Calculates depth of a node in BT hierarchy.
     */
    static int32_t CalculateNodeDepth(
        uint32_t nodeId,
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Helper: Finds parent node ID for a given child node.
     */
    static uint32_t FindParentNodeId(
        uint32_t childNodeId,
        const BehaviorTreeAsset& btAsset);

    /**
     * @brief Helper: Gets BehaviorTree node type name.
     */
    static std::string GetBTNodeTypeName(uint8_t type);

};

}  // namespace Olympe
