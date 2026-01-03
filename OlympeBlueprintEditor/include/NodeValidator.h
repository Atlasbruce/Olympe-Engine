/*
 * Olympe Blueprint Editor - Node Validator
 * 
 * Provides validation for nodes, actions, conditions, and decorators
 * using the EnumCatalogManager.
 */

#pragma once

#include "EnumCatalogManager.h"
#include <string>
#include <vector>

namespace Olympe
{
    /**
     * Result of a validation operation
     */
    struct ValidationResult
    {
        bool isValid = true;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        void AddError(const std::string& error)
        {
            errors.push_back(error);
            isValid = false;
        }

        void AddWarning(const std::string& warning)
        {
            warnings.push_back(warning);
        }

        bool HasErrors() const { return !errors.empty(); }
        bool HasWarnings() const { return !warnings.empty(); }
    };

    /**
     * Validator for behavior tree nodes
     * 
     * Validates that nodes use valid enum types from the catalog manager
     * and that required parameters are present.
     */
    class NodeValidator
    {
    public:
        /**
         * Validate a complete node from JSON
         */
        static ValidationResult ValidateNode(const nlohmann::json& nodeJson);

        /**
         * Validate an action node
         */
        static ValidationResult ValidateAction(const nlohmann::json& actionJson);

        /**
         * Validate a condition node
         */
        static ValidationResult ValidateCondition(const nlohmann::json& conditionJson);

        /**
         * Validate a decorator node
         */
        static ValidationResult ValidateDecorator(const nlohmann::json& decoratorJson);

    private:
        /**
         * Check if an enum value is valid
         */
        static void CheckEnumValidity(const std::string& enumType, 
                                       const std::string& value,
                                       ValidationResult& result);

        /**
         * Check if required parameters are present
         */
        static void CheckRequiredParameters(const EnumTypeInfo* typeInfo,
                                            const nlohmann::json& params,
                                            ValidationResult& result);
    };

} // namespace Olympe
