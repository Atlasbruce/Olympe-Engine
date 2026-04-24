/**
 * @file BTGraphValidator.h
 * @brief Validation system for Behavior Tree graph structure
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Validates BT graph structure according to business rules:
 * - Exactly one root node
 * - No cycles in graph
 * - Valid child counts per node type
 * - No orphan nodes
 * - Type-specific constraints
 */

#pragma once

#include "../../NodeGraphCore/GraphDocument.h"
#include <string>
#include <vector>
#include <cstdint>

namespace Olympe {
namespace AI {

// Type aliases for backward compatibility with modern NodeGraphTypes
using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
using NodeId = Olympe::NodeGraphTypes::NodeId;

/**
 * @enum BTValidationSeverity
 * @brief Severity level of validation message
 */
enum class BTValidationSeverity {
    Info,     ///< Informational message
    Warning,  ///< Warning (non-blocking)
    Error     ///< Error (blocking compilation)
};

/**
 * @struct BTValidationMessage
 * @brief Validation result message
 */
struct BTValidationMessage {
    BTValidationSeverity severity = BTValidationSeverity::Info;
    uint32_t nodeId = 0;           ///< Node ID (0 if global error)
    std::string message;           ///< Description of the issue
    std::string fix;               ///< Suggested fix
};

/**
 * @class BTGraphValidator
 * @brief Static validator for BT graph documents
 *
 * @details
 * Validates graph structure according to BT-specific rules. Returns list
 * of validation messages (errors, warnings, info). Graph is valid if no
 * error-level messages are returned.
 */
class BTGraphValidator {
public:
    /**
     * @brief Validate a complete BT graph
     * @param graph Graph document to validate
     * @return Vector of validation messages (empty if valid)
     */
    static std::vector<BTValidationMessage> ValidateGraph(const GraphDocument* graph);

private:
    /**
     * @brief Rule 1: Check for exactly one root node
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateRootNode(const GraphDocument* graph, std::vector<BTValidationMessage>& messages);

    /**
     * @brief Rule 2: Detect cycles in graph
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateCycles(const GraphDocument* graph, std::vector<BTValidationMessage>& messages);

    // TODO: Deprecated rules - schema changed
    // ValidateChildrenCount, ValidateOrphans, ValidateNodeTypes need reimplementation
    // Commented out to unblock build (Phase 50.4 cleanup)
    /*
    static void ValidateChildrenCount(const GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    static void ValidateOrphans(const GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    static void ValidateNodeTypes(const GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    */
};

} // namespace AI
} // namespace Olympe
