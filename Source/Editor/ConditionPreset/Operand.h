/**
 * @file Operand.h
 * @brief Defines the Operand type used in condition preset expressions.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * An Operand represents one side (left or right) of a condition comparison.
 * It can refer to a blackboard variable by name (Variable mode), hold a
 * literal numeric constant (Const mode), or represent an external data input
 * pin on the node (Pin mode).
 *
 * C++14 compliant — uses std::variant via mpark::variant polyfill is NOT
 * required here; the union is modelled with explicit tagged members to stay
 * within C++14 without extra dependencies.
 */

#pragma once

#include <string>
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @enum OperandMode
 * @brief Discriminates which source the operand draws its value from.
 */
enum class OperandMode {
    Variable, ///< References a blackboard variable by ID (string key)
    Const,    ///< Literal numeric constant (double)
    Pin       ///< External data input pin on the node (identified by label)
};

/**
 * @struct Operand
 * @brief One side of a ConditionPreset comparison expression.
 *
 * @details
 * Only one of `stringValue` / `constValue` is meaningful depending on `mode`:
 *   - Variable : stringValue holds the blackboard variable ID  (e.g. "mHealth")
 *   - Const    : constValue holds the literal number           (e.g. 2.0)
 *   - Pin      : stringValue holds the pin label               (e.g. "Pin:1")
 */
struct Operand {
    OperandMode mode;          ///< Discriminator tag
    std::string stringValue;   ///< Variable ID or Pin label (mode == Variable|Pin)
    double      constValue;    ///< Literal constant            (mode == Const)

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default: Variable mode, empty string.
    Operand();

    /// @brief Factory — creates a Variable-mode operand.
    /// @param variableID Blackboard variable key (e.g. "mHealth").
    static Operand CreateVariable(const std::string& variableID);

    /// @brief Factory — creates a Const-mode operand.
    /// @param constVal Literal numeric value.
    static Operand CreateConst(double constVal);

    /// @brief Factory — creates a Pin-mode operand.
    /// @param pinLabel Pin label shown in the UI (e.g. "Pin:1").
    static Operand CreatePin(const std::string& pinLabel);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// @brief Returns a human-readable display string.
    /// Examples: "[mHealth]", "[2]", "[Pin:1]"
    std::string GetDisplayString() const;

    /// @brief Returns true when mode == Pin.
    bool IsPin() const;

    /// @brief Returns true when mode == Variable.
    bool IsVariable() const;

    /// @brief Returns true when mode == Const.
    bool IsConst() const;

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this Operand to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes an Operand from a JSON object.
    /// @param data JSON object (must contain "mode" and "value" keys).
    static Operand FromJson(const nlohmann::json& data);
};

} // namespace Olympe
