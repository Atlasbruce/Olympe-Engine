/**
 * @file NodeConditionRef.h
 * @brief Defines NodeConditionRef — a node's reference to a global ConditionPreset.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * NodeConditionRef links a NodeBranch to a ConditionPreset via its UUID.
 * When a preset has Pin-mode operands the ref also stores the IDs of the
 * DynamicDataPin instances that were created to supply those values.
 *
 * Multiple NodeConditionRefs on a single node are combined with logical AND
 * or OR operators to form a compound boolean expression.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @enum LogicalOp
 * @brief How this condition is combined with the one preceding it.
 */
enum class LogicalOp {
    Start, ///< First condition in the list (no logical combinator)
    And,   ///< Combined with AND
    Or     ///< Combined with OR
};

/**
 * @struct NodeConditionRef
 * @brief One entry in a NodeBranch's conditions list.
 *
 * @details
 * References a ConditionPreset by UUID.  If the referenced preset's left or
 * right operand is in Pin mode, `leftPinID` / `rightPinID` store the UUID of
 * the corresponding DynamicDataPin so the runtime evaluator can find the live
 * float value.
 */
struct NodeConditionRef {
    std::string presetID;    ///< UUID of the referenced ConditionPreset
    LogicalOp   logicalOp;   ///< Combinator with previous condition (ignored for first)
    std::string leftPinID;   ///< DynamicDataPin UUID for left  Pin operand (or empty)
    std::string rightPinID;  ///< DynamicDataPin UUID for right Pin operand (or empty)

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default constructor — empty preset ID, LogicalOp::Start.
    NodeConditionRef();

    /// @brief Convenience constructor.
    /// @param pid Preset UUID to reference.
    /// @param op  Logical combinator (Start for the first condition).
    NodeConditionRef(const std::string& pid, LogicalOp op);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// @brief Returns the logical-op as a display string ("Start", "And", "Or").
    std::string GetLogicalOpString() const;

    /// @brief Returns true when a left-side DynamicDataPin is assigned.
    bool HasLeftPin() const;

    /// @brief Returns true when a right-side DynamicDataPin is assigned.
    bool HasRightPin() const;

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this ref to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes a NodeConditionRef from a JSON object.
    static NodeConditionRef FromJson(const nlohmann::json& data);
};

} // namespace Olympe
