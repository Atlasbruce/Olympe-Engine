/**
 * @file GraphValidationPanel.h
 * @brief VS graph validation with clickable error list (Phase 7).
 * @author Olympe Engine
 * @date 2026-03-10
 *
 * @details
 * GraphValidationPanel inspects a TaskGraphTemplate and produces a list of
 * GraphValidationError items covering:
 *   - Dead-end nodes (exec-output pins with no outgoing connection)
 *   - SubGraph nodes with an empty SubGraphPath
 *   - Cycles detected via DFS over ExecConnections
 *
 * UI code calls Validate(), iterates GetErrors(), and calls OnErrorClick()
 * when the user clicks an entry so the canvas can pan to the offending node.
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
 * @enum ValidationSeverity
 * @brief Indicates how serious a validation finding is.
 */
enum class ValidationSeverity {
    Warning,   ///< Informational; graph may still execute
    Error,     ///< Likely to cause incorrect behaviour
    Critical   ///< Graph cannot be executed
};

/**
 * @struct GraphValidationError
 * @brief A single validation finding.
 */
struct GraphValidationError {
    int                nodeId;   ///< Offending node ID; -1 for graph-level errors
    std::string        message;
    ValidationSeverity severity;
};

// ============================================================================
// GraphValidationPanel
// ============================================================================

/**
 * @class GraphValidationPanel
 * @brief Singleton that validates a TaskGraphTemplate and tracks selected node.
 *
 * Typical usage:
 * @code
 *   auto& panel = GraphValidationPanel::Get();
 *   panel.Validate(myGraph);
 *   if (panel.HasErrors())
 *   {
 *       for (const auto& err : panel.GetErrors())
 *           SYSTEM_LOG << err.message << std::endl;
 *   }
 * @endcode
 */
class GraphValidationPanel {
public:

    // -----------------------------------------------------------------------
    // Singleton access
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the single shared instance.
     */
    static GraphValidationPanel& Get();

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    /**
     * @brief Clears previous results and validates @p graph.
     */
    void Validate(const TaskGraphTemplate& graph);

    /**
     * @brief Returns the current list of validation findings.
     */
    const std::vector<GraphValidationError>& GetErrors() const;

    /**
     * @brief Returns true if there are any findings of any severity.
     */
    bool HasErrors()         const;

    /**
     * @brief Returns true if any finding has severity Critical.
     */
    bool HasCriticalErrors() const;

    /**
     * @brief Clears all findings and resets the selected node.
     */
    void Clear();

    // -----------------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------------

    /**
     * @brief Records @p nodeId as the currently selected node.
     *        UI can read this to pan the canvas to the offending node.
     */
    void OnErrorClick(int nodeId);

    /**
     * @brief Returns the node ID set by the most recent OnErrorClick() call
     *        (-1 if none).
     */
    int  GetSelectedNodeId() const;

private:

    GraphValidationPanel();

    // ---- Individual validation passes ----

    /**
     * @brief Flags nodes whose exec-output pins have no outgoing connection.
     */
    void CheckDeadEnds(const TaskGraphTemplate& graph);

    /**
     * @brief Flags SubGraph nodes that have no SubGraphPath set.
     */
    void CheckMissingConnections(const TaskGraphTemplate& graph);

    /**
     * @brief Flags cycles detected by DFS over ExecConnections.
     */
    void CheckCycles(const TaskGraphTemplate& graph);

    // ---- Helpers ----

    void AddError(int nodeId, const std::string& message, ValidationSeverity severity);

    std::vector<GraphValidationError> m_Errors;
    int                               m_SelectedNodeId;
};

} // namespace Olympe
