/**
 * @file ConditionRef.cpp
 * @brief Implementation of OperandRef and ConditionRef serialization helpers (Phase 24 Milestone 2).
 * @author Olympe Engine
 * @date 2026-03-18
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ConditionRef.h"

#include <string>

namespace Olympe {

// ============================================================================
// OperandRef serialization
// ============================================================================

nlohmann::json OperandRef::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();

    std::string modeStr;
    switch (mode)
    {
        case Mode::Variable: modeStr = "Variable"; break;
        case Mode::Const:    modeStr = "Const";    break;
        case Mode::Pin:      modeStr = "Pin";      break;
        default:             modeStr = "Const";    break;
    }
    j["mode"]         = modeStr;
    j["variableName"] = variableName;
    j["constValue"]   = constValue;
    j["dynamicPinID"] = dynamicPinID;
    return j;
}

/*static*/
OperandRef OperandRef::FromJson(const nlohmann::json& data)
{
    OperandRef ref;
    if (!data.is_object())
        return ref;

    if (data.contains("mode") && data["mode"].is_string())
    {
        const std::string& modeStr = data["mode"].get<std::string>();
        if (modeStr == "Variable")
            ref.mode = Mode::Variable;
        else if (modeStr == "Pin")
            ref.mode = Mode::Pin;
        else
            ref.mode = Mode::Const;
    }

    if (data.contains("variableName") && data["variableName"].is_string())
        ref.variableName = data["variableName"].get<std::string>();

    if (data.contains("constValue") && data["constValue"].is_string())
        ref.constValue = data["constValue"].get<std::string>();

    if (data.contains("dynamicPinID") && data["dynamicPinID"].is_string())
        ref.dynamicPinID = data["dynamicPinID"].get<std::string>();

    return ref;
}

// ============================================================================
// ConditionRef serialization
// ============================================================================

nlohmann::json ConditionRef::ToJson() const
{
    nlohmann::json j = nlohmann::json::object();
    j["conditionIndex"] = conditionIndex;
    j["leftOperand"]    = leftOperand.ToJson();
    j["operator"]       = operatorStr;
    j["rightOperand"]   = rightOperand.ToJson();

    // Serialize compareType as a string for readability.
    std::string typeStr;
    switch (compareType)
    {
        case VariableType::Int:    typeStr = "Int";    break;
        case VariableType::Float:  typeStr = "Float";  break;
        case VariableType::Bool:   typeStr = "Bool";   break;
        case VariableType::String: typeStr = "String"; break;
        default:                   typeStr = "Float";  break;
    }
    j["compareType"] = typeStr;
    return j;
}

/*static*/
ConditionRef ConditionRef::FromJson(const nlohmann::json& data)
{
    ConditionRef ref;
    if (!data.is_object())
        return ref;

    if (data.contains("conditionIndex") && data["conditionIndex"].is_number_integer())
        ref.conditionIndex = data["conditionIndex"].get<int>();

    if (data.contains("leftOperand") && data["leftOperand"].is_object())
        ref.leftOperand = OperandRef::FromJson(data["leftOperand"]);

    if (data.contains("operator") && data["operator"].is_string())
        ref.operatorStr = data["operator"].get<std::string>();

    if (data.contains("rightOperand") && data["rightOperand"].is_object())
        ref.rightOperand = OperandRef::FromJson(data["rightOperand"]);

    if (data.contains("compareType") && data["compareType"].is_string())
    {
        const std::string& typeStr = data["compareType"].get<std::string>();
        if (typeStr == "Int")
            ref.compareType = VariableType::Int;
        else if (typeStr == "Bool")
            ref.compareType = VariableType::Bool;
        else if (typeStr == "String")
            ref.compareType = VariableType::String;
        else
            ref.compareType = VariableType::Float;
    }

    return ref;
}

} // namespace Olympe
