/*
 * Olympe Blueprint Editor - Node Validator Implementation
 */

#include "../include/NodeValidator.h"
#include "../../Source/json_helper.h"
#include <iostream>

namespace Olympe
{
    ValidationResult NodeValidator::ValidateNode(const nlohmann::json& nodeJson)
    {
        ValidationResult result;

        // Check required fields
        if (!nodeJson.contains("type"))
        {
            result.AddError("Node missing 'type' field");
            return result;
        }

        std::string nodeType = JsonHelper::GetString(nodeJson, "type", "");
        
        // Validate based on node type
        if (nodeType == "Action")
        {
            auto actionResult = ValidateAction(nodeJson);
            result.errors.insert(result.errors.end(), 
                               actionResult.errors.begin(), 
                               actionResult.errors.end());
            result.warnings.insert(result.warnings.end(),
                                 actionResult.warnings.begin(),
                                 actionResult.warnings.end());
            if (!actionResult.isValid)
            {
                result.isValid = false;
            }
        }
        else if (nodeType == "Condition")
        {
            auto conditionResult = ValidateCondition(nodeJson);
            result.errors.insert(result.errors.end(),
                               conditionResult.errors.begin(),
                               conditionResult.errors.end());
            result.warnings.insert(result.warnings.end(),
                                 conditionResult.warnings.begin(),
                                 conditionResult.warnings.end());
            if (!conditionResult.isValid)
            {
                result.isValid = false;
            }
        }
        else if (nodeType == "Decorator")
        {
            auto decoratorResult = ValidateDecorator(nodeJson);
            result.errors.insert(result.errors.end(),
                               decoratorResult.errors.begin(),
                               decoratorResult.errors.end());
            result.warnings.insert(result.warnings.end(),
                                 decoratorResult.warnings.begin(),
                                 decoratorResult.warnings.end());
            if (!decoratorResult.isValid)
            {
                result.isValid = false;
            }
        }
        else if (nodeType == "Selector" || nodeType == "Sequence")
        {
            // Composite nodes are always valid structurally
            // Child validation would be done separately
        }
        else
        {
            result.AddError("Unknown node type: " + nodeType);
        }

        return result;
    }

    ValidationResult NodeValidator::ValidateAction(const nlohmann::json& actionJson)
    {
        ValidationResult result;

        if (!actionJson.contains("actionType"))
        {
            result.AddError("Action node missing 'actionType' field");
            return result;
        }

        std::string actionType = JsonHelper::GetString(actionJson, "actionType", "");
        CheckEnumValidity("Action", actionType, result);

        // Check parameters if we have type info
        const auto* typeInfo = EnumCatalogManager::Instance().FindActionType(actionType);
        if (typeInfo && actionJson.contains("parameters"))
        {
            CheckRequiredParameters(typeInfo, actionJson["parameters"], result);
        }

        return result;
    }

    ValidationResult NodeValidator::ValidateCondition(const nlohmann::json& conditionJson)
    {
        ValidationResult result;

        if (!conditionJson.contains("conditionType"))
        {
            result.AddError("Condition node missing 'conditionType' field");
            return result;
        }

        std::string conditionType = JsonHelper::GetString(conditionJson, "conditionType", "");
        CheckEnumValidity("Condition", conditionType, result);

        // Check parameters if we have type info
        const auto* typeInfo = EnumCatalogManager::Instance().FindConditionType(conditionType);
        if (typeInfo && conditionJson.contains("parameters"))
        {
            CheckRequiredParameters(typeInfo, conditionJson["parameters"], result);
        }

        return result;
    }

    ValidationResult NodeValidator::ValidateDecorator(const nlohmann::json& decoratorJson)
    {
        ValidationResult result;

        if (!decoratorJson.contains("decoratorType"))
        {
            result.AddError("Decorator node missing 'decoratorType' field");
            return result;
        }

        std::string decoratorType = JsonHelper::GetString(decoratorJson, "decoratorType", "");
        CheckEnumValidity("Decorator", decoratorType, result);

        // Check parameters if we have type info
        const auto* typeInfo = EnumCatalogManager::Instance().FindDecoratorType(decoratorType);
        if (typeInfo && decoratorJson.contains("parameters"))
        {
            CheckRequiredParameters(typeInfo, decoratorJson["parameters"], result);
        }

        return result;
    }

    void NodeValidator::CheckEnumValidity(const std::string& enumType,
                                          const std::string& value,
                                          ValidationResult& result)
    {
        bool isValid = false;

        if (enumType == "Action")
        {
            isValid = EnumCatalogManager::Instance().IsValidActionType(value);
        }
        else if (enumType == "Condition")
        {
            isValid = EnumCatalogManager::Instance().IsValidConditionType(value);
        }
        else if (enumType == "Decorator")
        {
            isValid = EnumCatalogManager::Instance().IsValidDecoratorType(value);
        }

        if (!isValid)
        {
            result.AddError("Invalid " + enumType + " type: '" + value + "'");
        }
    }

    void NodeValidator::CheckRequiredParameters(const EnumTypeInfo* typeInfo,
                                                const nlohmann::json& params,
                                                ValidationResult& result)
    {
        if (!typeInfo)
        {
            return;
        }

        // Check each required parameter
        for (size_t i = 0; i < typeInfo->parameters.size(); ++i)
        {
            const auto& paramDef = typeInfo->parameters[i];
            
            if (!paramDef.contains("name"))
            {
                continue;
            }

            std::string paramName = paramDef["name"].get<std::string>();
            bool required = JsonHelper::GetBool(paramDef, "required", false);

            if (required && !params.contains(paramName))
            {
                result.AddError("Missing required parameter: '" + paramName + 
                              "' for type '" + typeInfo->id + "'");
            }
        }

        // Warn about unknown parameters
        if (params.is_object())
        {
            // Note: This would require iterating over params keys
            // The minimal json library doesn't expose key iteration easily
            // So we'll skip this for now
        }
    }

} // namespace Olympe
