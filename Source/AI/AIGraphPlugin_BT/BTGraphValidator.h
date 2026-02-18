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
    static std::vector<BTValidationMessage> ValidateGraph(const NodeGraph::GraphDocument* graph);
    
private:
    /**
     * @brief Rule 1: Check for exactly one root node
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateRootNode(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    
    /**
     * @brief Rule 2: Detect cycles in graph
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateCycles(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    
    /**
     * @brief Rule 3: Validate child counts per node type
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateChildrenCount(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    
    /**
     * @brief Rule 4: Check for orphan nodes (disconnected from root)
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateOrphans(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages);
    
    /**
     * @brief Rule 5: Validate node types are registered
     * @param graph Graph to validate
     * @param messages Output messages
     */
    static void ValidateNodeTypes(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages);
};

} // namespace AI
} // namespace Olympe
