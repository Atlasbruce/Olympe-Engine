/**
 * @file ConditionPreset.h
 * @brief Defines ConditionPreset — a reusable, globally-stored condition expression.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * A ConditionPreset represents a single boolean comparison of the form:
 *   <left Operand> <ComparisonOp> <right Operand>
 *
 * Presets are stored in the global ConditionPresetRegistry and referenced by
 * NodeBranch nodes via NodeConditionRef.  This separation allows the same
 * condition to be reused across multiple nodes and edited in one place.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <utility>
#include "Operand.h"
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @enum ComparisonOp
 * @brief The relational operator used in a ConditionPreset.
 */
enum class ComparisonOp {
    Equal,        ///< ==
    NotEqual,     ///< !=
    Less,         ///< <
    LessEqual,    ///< <=
    Greater,      ///< >
    GreaterEqual  ///< >=
};

/**
 * @struct ConditionPreset
 * @brief A globally-stored, reusable condition expression.
 *
 * @details
 * Identified by a UUID string.  Holds a left Operand, a ComparisonOp, and a
 * right Operand.  Serialized to / loaded from
 * `./Blueprints/Presets/condition_presets.json`.
 */
struct ConditionPreset {
    std::string  id;   ///< Global unique UUID (e.g. "preset_001")
    std::string  name; ///< Human-readable display name (e.g. "Condition #1")
    Operand      left;
    ComparisonOp op;
    Operand      right;

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default constructor — generates an empty preset.
    ConditionPreset();

    /// @brief Full constructor.
    /// @param id   UUID string (caller must supply a unique value).
    /// @param l    Left operand.
    /// @param o    Comparison operator.
    /// @param r    Right operand.
    ConditionPreset(const std::string& id,
                    const Operand&     l,
                    ComparisonOp       o,
                    const Operand&     r);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// @brief Returns a human-readable preview string.
    /// Example: "[mHealth] <= [2]"
    std::string GetPreview() const;

    /// @brief Returns true when the left operand is in Pin mode.
    bool NeedsLeftPin() const;

    /// @brief Returns true when the right operand is in Pin mode.
    bool NeedsRightPin() const;

    /// @brief Returns a pair {needsLeft, needsRight}.
    std::pair<bool, bool> GetPinNeeds() const;

    /// @brief Returns the operator as a display string (e.g. "<=").
    static std::string OpToString(ComparisonOp o);

    /// @brief Parses a display string back to a ComparisonOp.
    /// Returns ComparisonOp::Equal on unknown input.
    static ComparisonOp OpFromString(const std::string& s);

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this preset to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes a ConditionPreset from a JSON object.
    static ConditionPreset FromJson(const nlohmann::json& data);
};

} // namespace Olympe
