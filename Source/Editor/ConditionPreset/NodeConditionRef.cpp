/**
 * @file NodeConditionRef.cpp
 * @brief Implementation of NodeConditionRef — construction, helpers, serialization.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "NodeConditionRef.h"

#include <string>

namespace Olympe {

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------

NodeConditionRef::NodeConditionRef()
    : presetID("")
    , logicalOp(LogicalOp::Start)
    , leftPinID("")
    , rightPinID("")
{
}

NodeConditionRef::NodeConditionRef(const std::string& pid, LogicalOp op)
    : presetID(pid)
    , logicalOp(op)
    , leftPinID("")
    , rightPinID("")
{
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string NodeConditionRef::GetLogicalOpString() const
{
    switch (logicalOp)
    {
    case LogicalOp::Start: return "Start";
    case LogicalOp::And:   return "And";
    case LogicalOp::Or:    return "Or";
    }
    return "Start";
}

bool NodeConditionRef::HasLeftPin() const
{
    return !leftPinID.empty();
}

bool NodeConditionRef::HasRightPin() const
{
    return !rightPinID.empty();
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json NodeConditionRef::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    j["presetID"]   = presetID;
    j["logicalOp"]  = GetLogicalOpString();
    j["leftPinID"]  = leftPinID;
    j["rightPinID"] = rightPinID;
    return j;
}

/*static*/
NodeConditionRef NodeConditionRef::FromJson(const nlohmann::json& data)
{
    NodeConditionRef ref;

    if (!data.is_object())
        return ref;

    if (data.contains("presetID") && data["presetID"].is_string())
        ref.presetID = data["presetID"].get<std::string>();

    if (data.contains("logicalOp") && data["logicalOp"].is_string())
    {
        std::string op = data["logicalOp"].get<std::string>();
        if (op == "And")    ref.logicalOp = LogicalOp::And;
        else if (op == "Or") ref.logicalOp = LogicalOp::Or;
        else                 ref.logicalOp = LogicalOp::Start;
    }

    if (data.contains("leftPinID") && data["leftPinID"].is_string())
        ref.leftPinID = data["leftPinID"].get<std::string>();

    if (data.contains("rightPinID") && data["rightPinID"].is_string())
        ref.rightPinID = data["rightPinID"].get<std::string>();

    return ref;
}

} // namespace Olympe
