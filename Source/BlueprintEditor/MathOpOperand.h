/**
 * @file MathOpOperand.h
 * @brief Defines MathOpOperand — operand references for MathOp nodes.
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * @details
 * MathOpOperand is a self-contained representation of one arithmetic operand
 * for a MathOp node. It mirrors the OperandRef/ConditionRef pattern, supporting
 * three modes: Variable (blackboard reference), Const (literal value), or
 * Pin (data input pin).
 *
 * A MathOp has three operands: left (A), operator, right (B).
 * Each operand independently supports Variable/Const/Pin modes with optional
 * dynamic pin generation for Pin-mode operands.
 *
 * C++14 compliant.
 */

#pragma once

#include <string>
#include "../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @struct MathOpOperand
 * @brief Represents one arithmetic operand (left A, right B) in a MathOp node.
 *
 * @details
 * Similar to OperandRef from ConditionRef.h, but adapted for arithmetic operations.
 * If mode is `Pin`, `dynamicPinID` stores the UUID of the DynamicDataPin.
 * If mode is `Variable` or `Const`, no pin is generated and `dynamicPinID` is empty.
 */
struct MathOpOperand
{
    /** @brief Discriminates the data source of this operand. */
    enum class Mode
    {
        Variable, ///< References a blackboard variable by name
        Const,    ///< Literal constant value
        Pin       ///< External data-input pin on the owning node
    };

    Mode mode = Mode::Const; ///< Active mode

    // Variable / Const modes
    std::string variableName; ///< Blackboard key  (mode == Variable), e.g. "mMoveSpeed"
    std::string constValue;   ///< Literal string  (mode == Const),    e.g. "5.0"

    // Pin mode only
    /**
     * @brief UUID of the DynamicDataPin that supplies this operand's value.
     *
     * Populated by DynamicDataPinManager when the parent MathOpOperand
     * is registered with a Pin-mode operand.
     * Format: "pin_inst_<random>", e.g. "pin_inst_abc123def456".
     * Empty string means no pin has been assigned yet.
     */
    std::string dynamicPinID;

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this MathOpOperand to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes a MathOpOperand from a JSON object.
    static MathOpOperand FromJson(const nlohmann::json& data);
};

/**
 * @struct MathOpRef
 * @brief Complete reference for a MathOp node: left operand, operator, right operand.
 *
 * @details
 * Stores the full operand configuration for one MathOp node.
 * The result is computed as: leftOperand [operator] rightOperand.
 * Operators supported: "+", "-", "*", "/", "%", "^" (power).
 *
 * Operands whose mode is Pin automatically have DynamicDataPin objects
 * generated for them by DynamicDataPinManager.
 */
struct MathOpRef
{
    // Operands
    MathOpOperand leftOperand;            ///< Left-hand side operand (A)
    std::string   mathOperator = "+";     ///< Arithmetic operator: "+", "-", "*", "/", "%", "^"
    MathOpOperand rightOperand;           ///< Right-hand side operand (B)

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// @brief Returns a human-readable display string for the operation.
    /// Example: "[mSpeed] + [5.0]" or "[Pin:0] * [mFactor]"
    std::string GetDisplayString() const;

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this MathOpRef to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes a MathOpRef from a JSON object.
    static MathOpRef FromJson(const nlohmann::json& data);
};

} // namespace Olympe
