/**
 * @file TaskGraphTemplate.h
 * @brief Immutable asset structure shared by all task graph runners
 * @author Olympe Engine
 * @date 2026-02-20
 *
 * @details
 * TaskGraphTemplate is a read-only description of a task graph loaded once and
 * shared by every TaskRunner that executes that graph.  It contains:
 *   - Variable definitions  (schema for the LocalBlackboard)
 *   - Node definitions      (structure and parameters of every graph node)
 *   - A lookup cache        (fast O(1) access to nodes by ID)
 *
 * C++14 compliant - no std::variant, std::optional, or C++17/20 features.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "TaskGraphTypes.h"

namespace Olympe {

// ============================================================================
// Supporting data structures
// ============================================================================

/**
 * @struct VariableDefinition
 * @brief Declares a single variable in the task graph's blackboard schema.
 */
struct VariableDefinition {
    std::string  Name;                              ///< Variable name (must be unique within the template)
    VariableType Type        = VariableType::None;  ///< Declared type
    TaskValue    DefaultValue;                      ///< Initial value (used by LocalBlackboard::Reset)
    bool         IsLocal     = true;                ///< true = local BB; false = global BB (future)
};

/**
 * @struct ParameterBinding
 * @brief Describes how a single parameter value is supplied to a task node.
 */
struct ParameterBinding {
    ParameterBindingType Type          = ParameterBindingType::Literal; ///< Binding mode
    TaskValue            LiteralValue;              ///< Used when Type == Literal
    std::string          VariableName;              ///< Used when Type == LocalVariable
};

/**
 * @struct TaskNodeDefinition
 * @brief Full description of a single node in the task graph.
 */
struct TaskNodeDefinition {
    int32_t     NodeID       = -1;                  ///< Unique ID within this template
    std::string NodeName;                           ///< Human-readable name
    TaskNodeType Type        = TaskNodeType::AtomicTask; ///< Node role

    /// Child node IDs (control-flow nodes only; empty for AtomicTask/Decorator leaf)
    std::vector<int32_t> ChildrenIDs;

    /// Atomic task type identifier (used when Type == AtomicTask)
    std::string AtomicTaskID;

    /// Named parameter bindings passed to the atomic task
    std::unordered_map<std::string, ParameterBinding> Parameters;

    int32_t NextOnSuccess = -1;  ///< ID of next node on success  (-1 = none)
    int32_t NextOnFailure = -1;  ///< ID of next node on failure  (-1 = none)
};

// ============================================================================
// TaskGraphTemplate
// ============================================================================

/**
 * @class TaskGraphTemplate
 * @brief Immutable, shareable task graph asset.
 *
 * @details
 * Load once, share across many TaskRunner instances.  Call BuildLookupCache()
 * after populating Nodes so that GetNode() runs in O(1).  Call Validate()
 * before using the template to catch structural errors early.
 */
class TaskGraphTemplate {
public:

    // -----------------------------------------------------------------------
    // Asset data (public for direct construction / serialization)
    // -----------------------------------------------------------------------

    std::string Name;         ///< Friendly name of this template (e.g. "PatrolBehaviour")
    std::string Description;  ///< Optional human-readable description

    std::vector<VariableDefinition> LocalVariables; ///< Blackboard schema
    std::vector<TaskNodeDefinition> Nodes;          ///< All graph nodes

    int32_t RootNodeID = -1;  ///< ID of the root node (must exist in Nodes)

    // -----------------------------------------------------------------------
    // Operations
    // -----------------------------------------------------------------------

    /**
     * @brief Validates the structural integrity of the template.
     *
     * Rules checked:
     *   - Nodes vector must not be empty.
     *   - RootNodeID must reference an existing node.
     *   - Every ChildrenID referenced by any node must reference an existing node.
     *
     * @return true if all validation rules pass; false otherwise.
     */
    bool Validate() const;

    /**
     * @brief Returns a pointer to the node with the given ID, or nullptr.
     *
     * Requires BuildLookupCache() to have been called after the last
     * modification to Nodes.
     *
     * @param nodeId Node identifier.
     * @return Pointer to the node, or nullptr if not found.
     */
    const TaskNodeDefinition* GetNode(int32_t nodeId) const;

    /**
     * @brief Rebuilds the internal ID-to-node lookup map from the Nodes vector.
     *
     * Must be called after Nodes is populated or modified.
     */
    void BuildLookupCache();

private:

    /// Fast lookup: node ID -> pointer into Nodes vector.
    std::unordered_map<int32_t, const TaskNodeDefinition*> m_nodeLookup;
};

} // namespace Olympe
