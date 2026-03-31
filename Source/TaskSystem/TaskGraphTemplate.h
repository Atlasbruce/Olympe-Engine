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
#include "../Editor/ConditionPreset/NodeConditionRef.h"
#include "../Editor/ConditionPreset/DynamicDataPin.h"
#include "../Editor/ConditionPreset/ConditionPreset.h"  // Phase 24: embedded preset bank
#include "../BlueprintEditor/ConditionRef.h"  // Phase 24: inline operand->pin mapping
#include "../BlueprintEditor/MathOpOperand.h"  // Phase 24: MathOp operand system
#include "../json_helper.h"  // For json type in global variables storage

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

// ============================================================================
// ATS Visual Scripting – Template data structures (Phase 1 - 2026-03-08)
// ============================================================================

/**
 * @struct ExecPinConnection
 * @brief Explicit connection between a named exec-out pin of a source node
 *        and the exec-in pin of a target node.
 */
struct ExecPinConnection {
    int32_t     SourceNodeID  = NODE_INDEX_NONE;
    std::string SourcePinName;   ///< e.g. "Then", "Else", "Loop", "Completed"
    int32_t     TargetNodeID  = NODE_INDEX_NONE;
    std::string TargetPinName;   ///< e.g. "In"
};

/**
 * @struct DataPinConnection
 * @brief Explicit connection between an output data pin of a source node
 *        and an input data pin of a target node.
 */
struct DataPinConnection {
    int32_t     SourceNodeID  = NODE_INDEX_NONE;
    std::string SourcePinName;
    int32_t     TargetNodeID  = NODE_INDEX_NONE;
    std::string TargetPinName;
};

/**
 * @struct BlackboardEntry
 * @brief Single entry in the graph's declared blackboard schema (local or global).
 */
struct BlackboardEntry {
    std::string  Key;
    VariableType Type       = VariableType::None;
    TaskValue    Default;
    bool         IsGlobal   = false;  ///< false = local scope, true = global BB
};

/**
 * @struct TaskNodeDefinition
 * @brief Full description of a single node in the task graph.
 */
struct TaskNodeDefinition {
    int32_t     NodeID       = NODE_INDEX_NONE;     ///< Unique ID within this template
    std::string NodeName;                           ///< Human-readable name
    TaskNodeType Type        = TaskNodeType::AtomicTask; ///< Node role

    /// Child node IDs (control-flow nodes only; empty for AtomicTask/Decorator leaf)
    std::vector<int32_t> ChildrenIDs;

    /// Atomic task type identifier (used when Type == AtomicTask)
    std::string AtomicTaskID;

    /// Named parameter bindings passed to the atomic task
    std::unordered_map<std::string, ParameterBinding> Parameters;

    int32_t NextOnSuccess = NODE_INDEX_NONE;  ///< ID of next node on success  (NODE_INDEX_NONE = none)
    int32_t NextOnFailure = NODE_INDEX_NONE;  ///< ID of next node on failure  (NODE_INDEX_NONE = none)

    // ATS VS extensions (Phase 1)
    std::vector<DataPinDefinition> DataPins;       ///< Data pins declared on this node
    std::string                    ConditionID;    ///< For Branch/While/Switch: ATS condition ID
    std::string                    BBKey;          ///< For GetBBValue/SetBBValue: BB key (scope:key)
    std::string                    SubGraphPath;   ///< For SubGraph: path to the sub-graph JSON
    std::vector<std::string>       SwitchCases;    ///< For Switch: ordered case labels (legacy; prefer switchCases)
    float                          DelaySeconds = 0.0f; ///< For Delay: duration in seconds
    std::string                    MathOperator;   ///< For MathOp: "+", "-", "*", "/"

    // ATS VS Switch enhancements (Phase 22-A - 2026-03-14)
    std::string                          switchVariable; ///< For Switch: BB key of the variable to switch on
    std::vector<SwitchCaseDefinition>    switchCases;   ///< For Switch: structured case definitions

    // ATS VS Unified Condition System (Phase 23-B.4 - 2026-03-15)
    std::vector<Condition> conditions;  ///< For Branch/While: structured condition list (implicit AND)

    // Phase 24.0 — Condition Preset System
    std::vector<NodeConditionRef> conditionRefs; ///< Multi-condition refs to global presets (Phase 24)
    std::vector<DynamicDataPin>   dynamicPins;   ///< Dynamic data-input pins for Pin-mode operands (Phase 24)

    // Phase 24 Milestone 1 — Condition references with operand->pin mapping
    /// Parallel to conditions[]: each entry stores the OperandRef->DynamicDataPin
    /// UUID mapping for the corresponding condition expression.
    /// Populated by DynamicDataPinManager::RegeneratePinsFromConditions().
    std::vector<ConditionRef> conditionOperandRefs;

    // Phase 24 Milestone 2 — MathOp operand system
    /// For MathOp: complete operand configuration (left operand, operator, right operand).
    /// Parallel to MathOp nodes; empty for all other node types.
    /// Operands in Pin mode automatically generate DynamicDataPin entries.
    MathOpRef mathOpRef;

    /// For VSSequence: dynamically-added exec-out pins beyond the default "Out".
    /// Each entry is a pin name (e.g. "Out_1", "Out_2"...).
    std::vector<std::string> DynamicExecOutputPins;

    // SubGraph-specific (Phase 3)
    std::unordered_map<std::string, ParameterBinding> InputParams;  ///< Input parameter bindings
    std::unordered_map<std::string, std::string>      OutputParams; ///< Output param -> BB key mapping

    // Editor-only metadata (not used by runtime AI execution)
    float EditorPosX    = 0.0f;   ///< Canvas X position loaded from JSON
    float EditorPosY    = 0.0f;   ///< Canvas Y position loaded from JSON
    bool  HasEditorPos  = false;  ///< True when EditorPosX/Y were loaded from JSON
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

    int32_t RootNodeID = NODE_INDEX_NONE;  ///< ID of the root node (must exist in Nodes)

    // ATS VS extensions (Phase 1)
    int32_t EntryPointID = NODE_INDEX_NONE;  ///< ID of the EntryPoint node (for VS graphs)

    /// Graph type: "BehaviorTree" (legacy) or "VisualScript" (ATS VS)
    std::string GraphType = "BehaviorTree";

    /// Local blackboard declared in this graph
    std::vector<BlackboardEntry> Blackboard;

    /// Explicit exec connections (ATS VS only)
    std::vector<ExecPinConnection> ExecConnections;

    /// Explicit data connections (ATS VS only)
    std::vector<DataPinConnection> DataConnections;

    // SubGraph metadata (Phase 3)
    bool IsSubGraph = false;                                ///< True if this template is a SubGraph
    std::vector<SubGraphParameterDef> InputParameters;     ///< Declared inputs (for subgraphs)
    std::vector<SubGraphParameterDef> OutputParameters;    ///< Declared outputs (for subgraphs)

    // Phase 24 — Condition Preset Bank (embedded in graph)
    /// Presets are now stored in the graph JSON, not in external files.
    /// This makes blueprints self-contained with no external file dependencies.
    /// Populated during LoadTemplate() from "presets" array in graph JSON.
    std::vector<ConditionPreset> Presets;

    // Phase 24 Global Blackboard Integration — Global Variable Values (entity-specific overrides)
    /// Stores JSON representation of global variable values for this specific graph instance.
    /// These are entity-specific values (different from global registry defaults).
    /// Loaded from "globalVariableValues" in graph JSON during deserialization.
    /// Restored to EntityBlackboard after initialization.
    json GlobalVariableValues = json::object();

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

    /**
     * @brief Phase 24.3 - Poka-Yoke: Sanitizes exec connections to remove invalid links.
     *
     * Detects and removes exec connections that violate the data-pure node constraints:
     *   - Removes any exec-in connections TO data-pure nodes (GetBBValue, MathOp)
     *   - Removes any exec-out connections FROM data-pure nodes
     *
     * Logs all removed connections to SYSTEM_LOG for debugging.
     * Safe to call multiple times (idempotent).
     *
     * @return Number of invalid connections that were removed.
     */
    int SanitizeExecConnections();

private:

    /// Fast lookup: node ID -> pointer into Nodes vector.
    std::unordered_map<int32_t, const TaskNodeDefinition*> m_nodeLookup;
};

} // namespace Olympe
