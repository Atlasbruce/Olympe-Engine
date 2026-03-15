/**
 * @file TaskGraphTypes.h
 * @brief Core enumerations and TaskValue type-safe variant for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-20
 *
 * @details
 * Defines the foundational types for the Atomic Task System (v3.0):
 * - TaskNodeType: Identifies the role of a node in the task graph
 * - VariableType: Type tags for TaskValue storage
 * - ParameterBindingType: How a parameter value is supplied
 * - TaskValue: C++14-compliant type-safe variant (union + non-POD fields)
 *
 * C++14 compliant - no std::variant, std::optional, or C++17/20 features.
 */

#pragma once

#include <string>
#include <stdexcept>
#include <cstdint>

#include "../vector.h"
#include "../ECS_Entity.h"

namespace Olympe {

// ============================================================================
// Sentinel constants
// ============================================================================

/**
 * @brief Sentinel value for "no node" in node index / ID fields.
 *
 * Use instead of the magic value -1 wherever a node index or node ID is
 * absent (similar to Unreal Engine's INDEX_NONE).
 */
constexpr int32_t NODE_INDEX_NONE = -1;

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @enum TaskNodeType
 * @brief Identifies the role of a node in the task graph.
 *
 * Values 0-5 are the original BehaviorTree node types (unchanged).
 * Values 6-17 are ATS Visual Scripting node types (Phase 1 - 2026-03-08).
 */
enum class TaskNodeType : uint8_t {
    AtomicTask  = 0,  ///< Leaf node that executes a single atomic task
    Sequence    = 1,  ///< Executes children in order; stops on first failure
    Selector    = 2,  ///< Executes children in order; stops on first success
    Parallel    = 3,  ///< Executes all children simultaneously
    Decorator   = 4,  ///< Wraps a single child and modifies its behaviour
    Root        = 5,  ///< Entry point of the graph (exactly one per template)

    // ATS Visual Scripting node types (Phase 1)
    EntryPoint  = 6,  ///< Unique entry node for VS graphs (replaces Root)
    Branch      = 7,  ///< If/Else conditional (Then / Else exec outputs)
    Switch      = 8,  ///< Multi-branch on value (N exec outputs)
    VSSequence  = 9,  ///< Execute N outputs in order ("VS" prefix avoids collision with BT Sequence=1)
    While       = 10, ///< Conditional loop (Loop / Completed exec outputs)
    ForEach     = 11, ///< Iterate over BB list (Loop Body / Completed exec outputs)
    DoOnce      = 12, ///< Single-fire execution (reset via Reset pin)
    Delay       = 13, ///< Timer (Completed exec output after N seconds)
    GetBBValue  = 14, ///< Data node – reads a Blackboard key
    SetBBValue  = 15, ///< Data node – writes a Blackboard key
    MathOp      = 16, ///< Data node – arithmetic operation (+, -, *, /)
    SubGraph    = 17  ///< Sub-graph call (SubTask)
};

/**
 * @enum VariableType
 * @brief Type tags used by TaskValue to identify stored data.
 *
 * Values 0-6 are the original types (unchanged).
 * Values 7-8 are ATS Visual Scripting extensions (Phase 1 - 2026-03-08).
 */
enum class VariableType : uint8_t {
    None,       ///< Uninitialized / empty value
    Bool,       ///< Boolean
    Int,        ///< 32-bit signed integer
    Float,      ///< Single-precision float
    Vector,     ///< 3-component vector (Vector from vector.h)
    EntityID,   ///< Entity identifier (uint64_t)
    String,     ///< std::string

    // ATS Visual Scripting extensions (Phase 1)
    List      = 7, ///< std::vector<TaskValue> (used by ForEach node)
    GlobalRef = 8  ///< Reference to a global blackboard key (scope "global:")
};

/**
 * @brief Converts a VariableType to its canonical string representation.
 * @param t  The VariableType to convert.
 * @return String name (e.g. "Bool", "Int").  Falls back to "Int" for unknown values.
 */
std::string VariableTypeToString(VariableType t);

/**
 * @enum ParameterBindingType
 * @brief Describes how a parameter value is provided to a task node.
 *
 * Values 0-1 are the original binding types (unchanged).
 * Values 2-6 are Phase 22-C extensions for dropdown-driven parameter editors.
 */
enum class ParameterBindingType : uint8_t {
    Literal        = 0, ///< Value is embedded directly in the template
    LocalVariable  = 1, ///< Value is read from the local blackboard at runtime

    // Phase 22-C extensions — dropdown-driven parameter types
    AtomicTaskID   = 2, ///< ID of an atomic task (from AtomicTaskUIRegistry)
    ConditionID    = 3, ///< ID of a condition type (from ConditionRegistry)
    MathOperator   = 4, ///< Math operator symbol (+, -, *, /, %) (from OperatorRegistry)
    ComparisonOp   = 5, ///< Comparison operator (==, !=, <, <=, >, >=) (from OperatorRegistry)
    SubGraphPath   = 6  ///< File path to a sub-graph .ats file
};

// ============================================================================
// ATS Visual Scripting – Node Types (Phase 1 - 2026-03-08)
// ============================================================================

// Extended TaskNodeType — appended values (existing values 0-5 unchanged)
// EntryPoint   : unique entry node (replaces Root for VS graphs)
// Branch       : If/Else conditional (2 exec outputs: Then / Else)
// Switch       : Multi-branch on value (N exec outputs)
// VSSequence   : Execute N outputs in order (VS Sequence, distinct from BT Sequence)
// While        : Conditional loop (Loop / Completed)
// ForEach      : Iterate over BB list (Loop Body / Completed)
// DoOnce       : Single-fire execution (reset via Reset pin)
// Delay        : Timer (Completed exec output after N seconds)
// GetBBValue   : Data node – reads a Blackboard key
// SetBBValue   : Data node – writes a Blackboard key
// MathOp       : Data node – arithmetic operation (+, -, *, /)
// SubGraph     : Sub-graph call (SubTask)

// NOTE: TaskNodeType enum values 0-5 (AtomicTask..Root) are unchanged.
// The following values extend the enum in TaskGraphTypes.h but cannot be
// added inline here without modifying the enum definition above.
// See the extended enum in TaskGraphTypes.h (values 6-17).

/**
 * @enum DataPinDir
 * @brief Direction of a data pin on a Visual Script node.
 */
enum class DataPinDir : uint8_t {
    Input,   ///< Value consumed by the node
    Output   ///< Value produced by the node
};

/**
 * @enum ExecPinRole
 * @brief Role of an exec pin on a Visual Script node.
 */
enum class ExecPinRole : uint8_t {
    In,           ///< Triggers execution of the node
    Out,          ///< Normal output / Then
    OutElse,      ///< Else output (Branch)
    OutLoop,      ///< Loop body output (While, ForEach)
    OutCompleted, ///< End-of-loop output (While, ForEach, Delay, DoOnce)
    OutCase       ///< Switch case output (dynamically named)
};

// ============================================================================
// TaskValue
// ============================================================================

/**
 * @class TaskValue
 * @brief C++14-compliant type-safe value container for task parameters.
 *
 * @details
 * Stores one value of type: bool, int, float, Vector, EntityID, or std::string.
 * POD types share a union; non-POD types (Vector, std::string) are stored
 * as separate members.
 *
 * Throws std::runtime_error on type mismatch when calling typed getters.
 */
class TaskValue {
public:

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Default constructor: creates a value of type VariableType::None.
     */
    TaskValue();

    /**
     * @brief Construct from bool.
     * @param v Boolean value.
     */
    explicit TaskValue(bool v);

    /**
     * @brief Construct from int.
     * @param v Integer value.
     */
    explicit TaskValue(int v);

    /**
     * @brief Construct from float.
     * @param v Float value.
     */
    explicit TaskValue(float v);

    /**
     * @brief Construct from Vector.
     * @param v Vector value (2D or 3D via z component).
     */
    explicit TaskValue(const ::Vector& v);

    /**
     * @brief Construct from EntityID.
     * @param v Entity identifier.
     */
    explicit TaskValue(EntityID v);

    /**
     * @brief Construct from std::string.
     * @param v String value.
     */
    explicit TaskValue(const std::string& v);

    // -----------------------------------------------------------------------
    // Getters (throw std::runtime_error on type mismatch)
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the bool value.
     * @throws std::runtime_error if the stored type is not Bool.
     */
    bool AsBool() const;

    /**
     * @brief Returns the int value.
     * @throws std::runtime_error if the stored type is not Int.
     */
    int AsInt() const;

    /**
     * @brief Returns the float value.
     * @throws std::runtime_error if the stored type is not Float.
     */
    float AsFloat() const;

    /**
     * @brief Returns the Vector value.
     * @throws std::runtime_error if the stored type is not Vector.
     */
    ::Vector AsVector() const;

    /**
     * @brief Returns the EntityID value.
     * @throws std::runtime_error if the stored type is not EntityID.
     */
    EntityID AsEntityID() const;

    /**
     * @brief Returns the string value.
     * @throws std::runtime_error if the stored type is not String.
     */
    std::string AsString() const;

    // -----------------------------------------------------------------------
    // Type queries
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the VariableType tag of the stored value.
     */
    VariableType GetType() const;

    /**
     * @brief Returns true if the value has not been set (type == None).
     */
    bool IsNone() const;

    /**
     * @brief Converts the stored value to a string representation.
     *
     * Never returns an empty string — falls back to "0" for None or empty values.
     * This prevents JSON builder failures when serializing uninitialized entries.
     * @return String representation of the value.
     */
    std::string to_string() const;

private:

    // POD storage (bool, int, float, EntityID share a union)
    union PodData {
        bool      boolValue;
        int       intValue;
        float     floatValue;
        EntityID  entityValue;

        PodData() : entityValue(0) {}
    } m_data;

    // Non-POD storage
    ::Vector    m_vectorValue;
    std::string m_stringValue;

    VariableType m_type;
};

// ============================================================================
// ATS Visual Scripting – DataPinDefinition (Phase 1 - 2026-03-08)
// Declared after TaskValue because it uses TaskValue as a member.
// ============================================================================

/**
 * @brief Returns a correctly-typed default TaskValue for the given VariableType.
 *
 * Used when a new blackboard variable is created or when its type is changed,
 * to ensure the Default field is always type-consistent.
 * @param type  The VariableType to create a default value for.
 * @return A TaskValue of the matching type, initialized to a zero/false/empty value.
 */
TaskValue GetDefaultValueForType(VariableType type);

/**
 * @struct DataPinDefinition
 * @brief Describes a data pin declared on a Visual Script node.
 */
struct DataPinDefinition {
    std::string  PinName;                          ///< Pin name ("Value", "Result", etc.)
    VariableType PinType  = VariableType::None;    ///< Type of the data
    DataPinDir   Dir      = DataPinDir::Input;     ///< Direction
    TaskValue    Default;                          ///< Default value when not connected
};

// ============================================================================
// ATS Visual Scripting – SubGraphParameterDef (Phase 3 - 2026-03-09)
// ============================================================================

/**
 * @struct SubGraphParameterDef
 * @brief Describes an input or output parameter declared on a SubGraph file.
 *
 * Used in TaskGraphTemplate::InputParameters and TaskGraphTemplate::OutputParameters
 * to describe the interface of a sub-graph that can receive and return values.
 */
struct SubGraphParameterDef {
    std::string  Name;                             ///< Parameter name (must match binding keys)
    VariableType Type = VariableType::None;        ///< Expected variable type
};

// ============================================================================
// ATS Visual Scripting – Condition (Phase 23-B.4 - 2026-03-15)
// ============================================================================

/**
 * @struct Condition
 * @brief Describes a single condition expression for Branch/While nodes.
 *
 * @details
 * Supports 6 combinations:
 *   Variable vs Variable  (health < max_health)
 *   Variable vs Const     (health > 50)
 *   Variable vs Pin       (health == Node#42.Out)
 *   Pin vs Pin            (Node#42.Out > Node#43.Out)
 *   Pin vs Const          (Node#42.Out >= threshold)
 *   Const vs Const        (50 < 100, for testing — W015 warning)
 *
 * Multiple conditions on one node are evaluated with implicit AND.
 */
struct Condition {
    // -- Left side value source --
    std::string leftMode;       ///< "Pin" | "Variable" | "Const"
    std::string leftPin;        ///< If Pin mode: e.g. "Node#42.Out"
    std::string leftVariable;   ///< If Variable mode: e.g. "health"
    TaskValue   leftConstValue; ///< If Const mode: e.g. TaskValue(50)

    // -- Comparison operator --
    std::string operatorStr;    ///< "==", "!=", "<", ">", "<=", ">="

    // -- Right side value source --
    std::string rightMode;      ///< "Pin" | "Variable" | "Const"
    std::string rightPin;       ///< If Pin mode: e.g. "Node#43.Out"
    std::string rightVariable;  ///< If Variable mode: e.g. "max_health"
    TaskValue   rightConstValue;///< If Const mode: e.g. TaskValue(100)

    // -- Type hint for validation --
    VariableType compareType = VariableType::None; ///< Int, Float, Bool, String, Vector
};

// ============================================================================
// ATS Visual Scripting – SwitchCaseDefinition (Phase 22-A - 2026-03-14)
// ============================================================================

/**
 * @struct SwitchCaseDefinition
 * @brief Describes a single case branch on a Switch node.
 *
 * Each case maps a match value to a named exec-out pin.
 * An optional custom label can be set by the user for readability
 * (e.g. "Patrol" instead of "Case_5").
 */
struct SwitchCaseDefinition {
    std::string value;        ///< The value to match (int as decimal string or raw string)
    std::string pinName;      ///< The exec-out pin name used internally (e.g. "Case_0")
    std::string customLabel;  ///< User-defined display label (empty = use pinName)

    /**
     * @brief Returns the display name shown in the editor.
     * Returns customLabel if non-empty, otherwise pinName.
     */
    const std::string& GetDisplayName() const
    {
        return customLabel.empty() ? pinName : customLabel;
    }
};

} // namespace Olympe
