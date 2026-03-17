/**
 * @file ConditionPreset.cpp
 * @brief Implementation of ConditionPreset — construction, helpers, serialization.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPreset.h"

#include <string>

namespace Olympe {

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

ConditionPreset::ConditionPreset()
    : id("")
    , name("")
    , op(ComparisonOp::Equal)
{
}

ConditionPreset::ConditionPreset(const std::string& id_,
                                  const Operand&     l,
                                  ComparisonOp       o,
                                  const Operand&     r)
    : id(id_)
    , name("")
    , left(l)
    , op(o)
    , right(r)
{
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string ConditionPreset::GetPreview() const
{
    return left.GetDisplayString() + " " + OpToString(op) + " " + right.GetDisplayString();
}

bool ConditionPreset::NeedsLeftPin() const
{
    return left.IsPin();
}

bool ConditionPreset::NeedsRightPin() const
{
    return right.IsPin();
}

std::pair<bool, bool> ConditionPreset::GetPinNeeds() const
{
    return std::make_pair(NeedsLeftPin(), NeedsRightPin());
}

/*static*/
std::string ConditionPreset::OpToString(ComparisonOp o)
{
    switch (o)
    {
    case ComparisonOp::Equal:        return "==";
    case ComparisonOp::NotEqual:     return "!=";
    case ComparisonOp::Less:         return "<";
    case ComparisonOp::LessEqual:    return "<=";
    case ComparisonOp::Greater:      return ">";
    case ComparisonOp::GreaterEqual: return ">=";
    }
    return "==";
}

/*static*/
ComparisonOp ConditionPreset::OpFromString(const std::string& s)
{
    if (s == "!=") return ComparisonOp::NotEqual;
    if (s == "<")  return ComparisonOp::Less;
    if (s == "<=") return ComparisonOp::LessEqual;
    if (s == ">")  return ComparisonOp::Greater;
    if (s == ">=") return ComparisonOp::GreaterEqual;
    return ComparisonOp::Equal; // default / "=="
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json ConditionPreset::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    j["id"]       = id;
    j["name"]     = name;
    j["left"]     = left.ToJson();
    j["operator"] = OpToString(op);
    j["right"]    = right.ToJson();
    return j;
}

/*static*/
ConditionPreset ConditionPreset::FromJson(const nlohmann::json& data)
{
    ConditionPreset preset;

    if (!data.is_object())
        return preset;

    if (data.contains("id") && data["id"].is_string())
        preset.id = data["id"].get<std::string>();

    if (data.contains("name") && data["name"].is_string())
        preset.name = data["name"].get<std::string>();

    if (data.contains("left") && data["left"].is_object())
        preset.left = Operand::FromJson(data["left"]);

    if (data.contains("operator") && data["operator"].is_string())
        preset.op = OpFromString(data["operator"].get<std::string>());

    if (data.contains("right") && data["right"].is_object())
        preset.right = Operand::FromJson(data["right"]);

    return preset;
}

} // namespace Olympe
