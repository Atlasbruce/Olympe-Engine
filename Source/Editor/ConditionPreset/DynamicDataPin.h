/**
 * @file DynamicDataPin.h
 * @brief Defines DynamicDataPin — a runtime data-input pin attached to a node.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * When a ConditionPreset uses a Pin-mode operand, the owning NodeBranch needs
 * a physical input pin rendered in the graph editor.  A DynamicDataPin tracks
 * all metadata required to:
 *   - Render the pin in the node UI.
 *   - Map incoming float data to the correct operand side of the correct
 *     condition during runtime evaluation.
 *   - Persist to JSON alongside the node so that connections survive save/load.
 *
 * Each DynamicDataPin has a globally unique UUID so that graph links can
 * reference it by ID without ambiguity.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include "../../third_party/nlohmann/json.hpp"

namespace Olympe {

/**
 * @enum OperandPosition
 * @brief Identifies which operand side a DynamicDataPin serves.
 */
enum class OperandPosition {
    Left,  ///< Pin provides data for the left operand of the condition
    Right  ///< Pin provides data for the right operand of the condition
};

/**
 * @struct DynamicDataPin
 * @brief A data-input pin created dynamically for a Pin-mode operand.
 *
 * @details
 * Created automatically by the Dynamic Pin Manager (Phase 24.3) when a
 * ConditionPreset containing a Pin-mode operand is assigned to a NodeBranch.
 * The `nodePinID` field is populated at render time (ImNodes pin ID).
 */
struct DynamicDataPin {
    std::string     id;             ///< Global unique UUID (e.g. "pin_inst_abc123")
    int             conditionIndex; ///< Index in node.conditions (0-based)
    OperandPosition position;       ///< Left or Right operand side
    std::string     label;          ///< Display label (set at construction)
    std::string     nodePinID;      ///< ImGui/ImNodes pin ID (assigned at render time)
    float           dataValue;      ///< Runtime float value received from connected node
    int             sequenceNumber; ///< 1-based sequence index across all pins on the node (for "Pin-in #N" label)

    // -----------------------------------------------------------------------
    // Constructors
    // -----------------------------------------------------------------------

    /// Default constructor — zero-initialised.
    DynamicDataPin();

    /// @brief Convenience constructor.
    /// @param condIdx     Index of the owning condition in node.conditions.
    /// @param pos         Which operand side (Left or Right).
    /// @param condPreview Short preview of the parent condition for the label
    ///                    (e.g. "[mSpeed] == [Pin:1]").
    DynamicDataPin(int condIdx, OperandPosition pos, const std::string& condPreview);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /// @brief Returns the full display label shown on the node.
    /// Format: "In #<condIdx+1><L|R>: <condPreview>"
    /// Example: "In #3L: [mSpeed] == [Pin:1]"
    std::string GetDisplayLabel() const;

    /// @brief Returns the short label for use as a pin connector slot.
    /// Format: "Pin-in #<sequenceNumber>"
    /// Example: "Pin-in #1"
    std::string GetShortLabel() const;

    /// @brief Generates a globally unique identifier string (UUID v4-style).
    static std::string GenerateUniqueID();

    // -----------------------------------------------------------------------
    // Serialization
    // -----------------------------------------------------------------------

    /// @brief Serializes this pin to a JSON object.
    nlohmann::json ToJson() const;

    /// @brief Deserializes a DynamicDataPin from a JSON object.
    static DynamicDataPin FromJson(const nlohmann::json& data);
};

} // namespace Olympe
