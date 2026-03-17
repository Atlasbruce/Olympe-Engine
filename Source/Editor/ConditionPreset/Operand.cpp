/**
 * @file Operand.cpp
 * @brief Implementation of Operand — construction, helpers, and serialization.
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Operand.h"

#include <stdexcept>
#include <string>

namespace Olympe {

// ---------------------------------------------------------------------------
// Default constructor
// ---------------------------------------------------------------------------

Operand::Operand()
    : mode(OperandMode::Variable)
    , stringValue("")
    , constValue(0.0)
{
}

// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

/*static*/
Operand Operand::CreateVariable(const std::string& variableID)
{
    Operand op;
    op.mode        = OperandMode::Variable;
    op.stringValue = variableID;
    op.constValue  = 0.0;
    return op;
}

/*static*/
Operand Operand::CreateConst(double constVal)
{
    Operand op;
    op.mode        = OperandMode::Const;
    op.constValue  = constVal;
    op.stringValue = "";
    return op;
}

/*static*/
Operand Operand::CreatePin(const std::string& pinLabel)
{
    Operand op;
    op.mode        = OperandMode::Pin;
    op.stringValue = pinLabel;
    op.constValue  = 0.0;
    return op;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string Operand::GetDisplayString() const
{
    switch (mode)
    {
    case OperandMode::Variable:
        return "[" + stringValue + "]";

    case OperandMode::Const:
    {
        // Format without trailing zeros when the value is a whole number.
        // e.g. 2.0 → "2", 2.5 → "2.5"
        if (constValue == static_cast<double>(static_cast<long long>(constValue)))
        {
            // Whole number — omit decimal point.
            std::string s = std::to_string(static_cast<long long>(constValue));
            return "[" + s + "]";
        }
        // Has fractional part — use default float representation.
        std::string s = std::to_string(constValue);
        // Trim trailing zeros after decimal point.
        size_t dot = s.find('.');
        if (dot != std::string::npos)
        {
            size_t last = s.find_last_not_of('0');
            if (last != std::string::npos && last > dot)
                s = s.substr(0, last + 1);
            else if (last == dot)
                s = s.substr(0, dot);
        }
        return "[" + s + "]";
    }

    case OperandMode::Pin:
        return "[" + stringValue + "]";
    }
    return "[]";
}

bool Operand::IsPin() const
{
    return mode == OperandMode::Pin;
}

bool Operand::IsVariable() const
{
    return mode == OperandMode::Variable;
}

bool Operand::IsConst() const
{
    return mode == OperandMode::Const;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

nlohmann::json Operand::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();

    switch (mode)
    {
    case OperandMode::Variable:
        j["mode"]  = "Variable";
        j["value"] = stringValue;
        break;

    case OperandMode::Const:
        j["mode"]  = "Const";
        j["value"] = constValue;
        break;

    case OperandMode::Pin:
        j["mode"]  = "Pin";
        j["value"] = stringValue;
        break;
    }

    return j;
}

/*static*/
Operand Operand::FromJson(const nlohmann::json& data)
{
    Operand op;

    if (!data.is_object())
        return op;

    std::string modeStr;
    if (data.contains("mode") && data["mode"].is_string())
        modeStr = data["mode"].get<std::string>();

    if (modeStr == "Variable")
    {
        op.mode = OperandMode::Variable;
        if (data.contains("value") && data["value"].is_string())
            op.stringValue = data["value"].get<std::string>();
    }
    else if (modeStr == "Const")
    {
        op.mode = OperandMode::Const;
        if (data.contains("value") && data["value"].is_number())
            op.constValue = data["value"].get<double>();
    }
    else if (modeStr == "Pin")
    {
        op.mode = OperandMode::Pin;
        if (data.contains("value") && data["value"].is_string())
            op.stringValue = data["value"].get<std::string>();
    }

    return op;
}

} // namespace Olympe
