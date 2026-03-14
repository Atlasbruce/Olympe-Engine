/**
 * @file OperatorRegistry.cpp
 * @brief Implementation of OperatorRegistry — hardcoded operator lists.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "OperatorRegistry.h"

#include <algorithm>

namespace Olympe {

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

static const std::vector<std::string> s_mathOperators = {"+", "-", "*", "/", "%"};
static const std::vector<std::string> s_comparisonOperators = {"==", "!=", "<", "<=", ">", ">="};

// ---------------------------------------------------------------------------
// OperatorRegistry
// ---------------------------------------------------------------------------

const std::vector<std::string>& OperatorRegistry::GetMathOperators()
{
    return s_mathOperators;
}

const std::vector<std::string>& OperatorRegistry::GetComparisonOperators()
{
    return s_comparisonOperators;
}

bool OperatorRegistry::IsValidMathOperator(const std::string& op)
{
    for (size_t i = 0; i < s_mathOperators.size(); ++i)
    {
        if (s_mathOperators[i] == op)
            return true;
    }
    return false;
}

bool OperatorRegistry::IsValidComparisonOperator(const std::string& op)
{
    for (size_t i = 0; i < s_comparisonOperators.size(); ++i)
    {
        if (s_comparisonOperators[i] == op)
            return true;
    }
    return false;
}

std::string OperatorRegistry::GetDisplayName(const std::string& op)
{
    if (op == "+")   return "Add (+)";
    if (op == "-")   return "Subtract (-)";
    if (op == "*")   return "Multiply (*)";
    if (op == "/")   return "Divide (/)";
    if (op == "%")   return "Modulo (%)";
    if (op == "==")  return "Equal (==)";
    if (op == "!=")  return "Not Equal (!=)";
    if (op == "<")   return "Less Than (<)";
    if (op == "<=")  return "Less or Equal (<=)";
    if (op == ">")   return "Greater Than (>)";
    if (op == ">=")  return "Greater or Equal (>=)";
    return op;
}

} // namespace Olympe
