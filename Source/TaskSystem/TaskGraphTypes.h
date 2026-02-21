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

#include "vector.h"
#include "ECS_Entity.h"

namespace Olympe {

// ============================================================================
// Enumerations
// ============================================================================

/**
 * @enum TaskNodeType
 * @brief Identifies the role of a node in the task graph.
 */
enum class TaskNodeType : uint8_t {
    AtomicTask,    ///< Leaf node that executes a single atomic task
    Sequence,      ///< Executes children in order; stops on first failure
    Selector,      ///< Executes children in order; stops on first success
    Parallel,      ///< Executes all children simultaneously
    Decorator,     ///< Wraps a single child and modifies its behaviour
    Root           ///< Entry point of the graph (exactly one per template)
};

/**
 * @enum VariableType
 * @brief Type tags used by TaskValue to identify stored data.
 */
enum class VariableType : uint8_t {
    None,       ///< Uninitialized / empty value
    Bool,       ///< Boolean
    Int,        ///< 32-bit signed integer
    Float,      ///< Single-precision float
    Vector,     ///< 3-component vector (Vector from vector.h)
    EntityID,   ///< Entity identifier (uint64_t)
    String      ///< std::string
};

/**
 * @enum ParameterBindingType
 * @brief Describes how a parameter value is provided to a task node.
 */
enum class ParameterBindingType : uint8_t {
    Literal,        ///< Value is embedded directly in the template
    LocalVariable   ///< Value is read from the local blackboard at runtime
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

} // namespace Olympe
