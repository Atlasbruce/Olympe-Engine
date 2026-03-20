/**
 * @file MathOpOperand.cpp
 * @brief Implementation of MathOpOperand serialization and helpers.
 */

#include "MathOpOperand.h"

namespace Olympe {

// ============================================================================
// MathOpOperand
// ============================================================================

nlohmann::json MathOpOperand::ToJson() const
{
    nlohmann::json j;
    
    // Mode (0 = Variable, 1 = Const, 2 = Pin)
    int modeValue = static_cast<int>(mode);
    j["mode"] = modeValue;
    
    // Variable / Const data
    j["variableName"] = variableName;
    j["constValue"] = constValue;
    
    // Pin mode data
    j["dynamicPinID"] = dynamicPinID;
    
    return j;
}

MathOpOperand MathOpOperand::FromJson(const nlohmann::json& data)
{
    MathOpOperand operand;
    
    if (data.contains("mode")) {
        int modeValue = data["mode"].get<int>();
        operand.mode = static_cast<Mode>(modeValue);
    }
    
    if (data.contains("variableName")) {
        operand.variableName = data["variableName"].get<std::string>();
    }
    
    if (data.contains("constValue")) {
        operand.constValue = data["constValue"].get<std::string>();
    }
    
    if (data.contains("dynamicPinID")) {
        operand.dynamicPinID = data["dynamicPinID"].get<std::string>();
    }
    
    return operand;
}

// ============================================================================
// MathOpRef
// ============================================================================

std::string MathOpRef::GetDisplayString() const
{
    std::string result = "[";
    
    // Left operand
    switch (leftOperand.mode) {
        case MathOpOperand::Mode::Variable:
            result += leftOperand.variableName;
            break;
        case MathOpOperand::Mode::Const:
            result += leftOperand.constValue;
            break;
        case MathOpOperand::Mode::Pin:
            result += "Pin:" + leftOperand.dynamicPinID;
            break;
    }
    
    result += "] " + mathOperator + " [";
    
    // Right operand
    switch (rightOperand.mode) {
        case MathOpOperand::Mode::Variable:
            result += rightOperand.variableName;
            break;
        case MathOpOperand::Mode::Const:
            result += rightOperand.constValue;
            break;
        case MathOpOperand::Mode::Pin:
            result += "Pin:" + rightOperand.dynamicPinID;
            break;
    }
    
    result += "]";
    
    return result;
}

nlohmann::json MathOpRef::ToJson() const
{
    nlohmann::json j;
    
    j["leftOperand"] = leftOperand.ToJson();
    j["mathOperator"] = mathOperator;
    j["rightOperand"] = rightOperand.ToJson();
    
    return j;
}

MathOpRef MathOpRef::FromJson(const nlohmann::json& data)
{
    MathOpRef ref;
    
    if (data.contains("leftOperand")) {
        ref.leftOperand = MathOpOperand::FromJson(data["leftOperand"]);
    }
    
    if (data.contains("mathOperator")) {
        ref.mathOperator = data["mathOperator"].get<std::string>();
    }
    
    if (data.contains("rightOperand")) {
        ref.rightOperand = MathOpOperand::FromJson(data["rightOperand"]);
    }
    
    return ref;
}

} // namespace Olympe
