/**
 * @file OperatorRegistry.h
 * @brief Hardcoded lists of math and comparison operators for dropdown editors.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * @details
 * Provides static access to the supported math operators (+, -, *, /, %)
 * and comparison operators (==, !=, <, <=, >, >=) used by MathOp nodes
 * and condition parameters.
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

/**
 * @class OperatorRegistry
 * @brief Static registry of math and comparison operator strings.
 *
 * @details
 * All methods are static — no instance creation required.
 */
class OperatorRegistry {
public:

    /**
     * @brief Returns the list of supported math operator symbols.
     * @return Vector of strings: {"+", "-", "*", "/", "%"}
     */
    static const std::vector<std::string>& GetMathOperators();

    /**
     * @brief Returns the list of supported comparison operator symbols.
     * @return Vector of strings: {"==", "!=", "<", "<=", ">", ">="}
     */
    static const std::vector<std::string>& GetComparisonOperators();

    /**
     * @brief Returns true if the given symbol is a valid math operator.
     * @param op Operator symbol to check.
     */
    static bool IsValidMathOperator(const std::string& op);

    /**
     * @brief Returns true if the given symbol is a valid comparison operator.
     * @param op Operator symbol to check.
     */
    static bool IsValidComparisonOperator(const std::string& op);

    /**
     * @brief Returns a human-readable display name for an operator.
     *
     * Examples: "+" → "Add (+)", "==" → "Equal (==)"
     * Falls back to the operator symbol itself if not found.
     *
     * @param op Operator symbol.
     * @return Display name string.
     */
    static std::string GetDisplayName(const std::string& op);

private:
    OperatorRegistry() = delete;
};

} // namespace Olympe
