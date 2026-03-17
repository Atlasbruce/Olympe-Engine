/**
 * @file ConditionPreset.cpp
 * @brief Implementation of ConditionPreset (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionPreset.h"

#include <sstream>
#include <iomanip>

namespace Olympe {

// ============================================================================
// Default constructor
// ============================================================================

ConditionPreset::ConditionPreset()
    : id("")
    , name("")
    , left()
    , op(ComparisonOp::Equal)
    , right()
{
}

// ============================================================================
// Full constructor
// ============================================================================

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

// ============================================================================
// Helpers
// ============================================================================

std::string ConditionPreset::GetPreview() const
{
    return left.GetDisplayString()
         + " " + OpToString(op)
         + " " + right.GetDisplayString();
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
    return { left.IsPin(), right.IsPin() };
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

// ============================================================================
// Serialization
// ============================================================================

nlohmann::json ConditionPreset::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    j["id"]   = id;
    j["name"] = name;
    j["left"] = left.ToJson();
    j["op"]   = OpToString(op);
    j["right"] = right.ToJson();
    return j;
}

/*static*/
ConditionPreset ConditionPreset::FromJson(const nlohmann::json& data)
{
    ConditionPreset p;

    if (!data.is_object())
        return p;

    if (data.contains("id") && data["id"].is_string())
        p.id = data["id"].get<std::string>();

    if (data.contains("name") && data["name"].is_string())
        p.name = data["name"].get<std::string>();

    if (data.contains("left"))
        p.left = Operand::FromJson(data["left"]);

    if (data.contains("op") && data["op"].is_string())
        p.op = OpFromString(data["op"].get<std::string>());
    else if (data.contains("operator") && data["operator"].is_string())
        p.op = OpFromString(data["operator"].get<std::string>());

    if (data.contains("right"))
        p.right = Operand::FromJson(data["right"]);

    return p;
}

} // namespace Olympe
