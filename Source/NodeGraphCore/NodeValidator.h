/**
 * @file NodeValidator.h
 * @brief Real-time node and graph validation (Phase 9).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * NodeValidator provides static validation methods that inspect a
 * TaskGraphTemplate and produce a list of ValidationMessage items.
 *
 * Three validation passes are available:
 *   - CheckUnconnectedNodes : nodes with no exec-output connections
 *   - CheckMissingSubGraphPaths: SubGraph nodes with an empty SubGraphPath
 *   - CheckInfiniteLoops    : DFS cycle detection over ExecConnections
 *
 * The validator has no state and no ImGui dependency.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @enum NVSeverity
 * @brief Indicates how serious a NodeValidator finding is.
 *
 * Named NVSeverity to avoid clashing with ValidationSeverity in
 * GraphValidationPanel.h which is already in namespace Olympe.
 */
enum class NVSeverity {
    Info,     ///< Informational; no action required
    Warning,  ///< May cause unexpected behaviour
    Error     ///< Will prevent correct execution
};

/**
 * @struct ValidationMessage
 * @brief A single finding produced by NodeValidator.
 */
struct ValidationMessage {
    int         nodeId   = -1;   ///< Offending node ID; -1 for graph-level messages
    NVSeverity  severity = NVSeverity::Info;
    std::string message;
    std::string hint;            ///< Optional suggestion to fix the issue
};

// ============================================================================
// NodeValidator
// ============================================================================

/**
 * @class NodeValidator
 * @brief Static-only validator for TaskGraphTemplate instances.
 *
 * Typical usage:
 * @code
 *   auto msgs = NodeValidator::ValidateGraph(&myGraph);
 *   for (const auto& m : msgs)
 *       SYSTEM_LOG << m.message << std::endl;
 * @endcode
 */
class NodeValidator {
public:

    /**
     * @brief Validates every node in the graph and returns all findings.
     * @param graph Non-null pointer to the template to validate.
     * @return List of ValidationMessage items (may be empty if graph is valid).
     */
    static std::vector<ValidationMessage> ValidateGraph(const TaskGraphTemplate* graph);

    /**
     * @brief Validates a single node definition.
     * @param node Non-null pointer to the node to validate.
     * @return List of ValidationMessage items for that node.
     */
    static std::vector<ValidationMessage> ValidateNode(const TaskNodeDefinition* node);

private:

    /**
     * @brief Flags non-EntryPoint/ExitPoint nodes with no outgoing exec connections.
     */
    static void CheckUnconnectedNodes(const TaskGraphTemplate* graph,
                                      std::vector<ValidationMessage>& messages);

    /**
     * @brief Flags SubGraph nodes that have an empty SubGraphPath.
     */
    static void CheckMissingSubGraphPaths(const TaskGraphTemplate* graph,
                                          std::vector<ValidationMessage>& messages);

    /**
     * @brief Flags cycles detected via DFS over ExecPinConnections.
     */
    static void CheckInfiniteLoops(const TaskGraphTemplate* graph,
                                   std::vector<ValidationMessage>& messages);

    // Utility: add a message to the list
    static void AddMessage(std::vector<ValidationMessage>& messages,
                           int nodeId,
                           NVSeverity severity,
                           const std::string& msg,
                           const std::string& hint = "");
};

} // namespace Olympe
