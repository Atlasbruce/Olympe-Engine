/*
 * Olympe Blueprint Editor - Blueprint Validator Implementation
 */

#include "BlueprintValidator.h"
#include "EnumCatalogManager.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>

namespace Olympe
{
    BlueprintValidator::BlueprintValidator()
    {
    }

    BlueprintValidator::~BlueprintValidator()
    {
    }

    std::vector<ValidationError> BlueprintValidator::ValidateGraph(const NodeGraph* graph)
    {
        std::vector<ValidationError> errors;
        
        if (!graph)
        {
            errors.push_back(ValidationError(-1, "", "Graph is null", ErrorSeverity::Critical, "Graph"));
            return errors;
        }

        // Validate each node
        auto nodes = graph->GetAllNodes();
        for (const GraphNode* node : nodes)
        {
            ValidateNodeType(graph, node, errors);
            ValidateNodeParameters(graph, node, errors);
            ValidateNodeLinks(graph, node, errors);
        }

        // Graph-level validations
        if (graph->rootNodeId < 0)
        {
            errors.push_back(ValidationError(-1, graph->name, 
                "No root node defined", ErrorSeverity::Warning, "Graph"));
        }

        return errors;
    }

    std::vector<ValidationError> BlueprintValidator::ValidateNode(const NodeGraph* graph, int nodeId)
    {
        std::vector<ValidationError> errors;
        
        if (!graph)
        {
            errors.push_back(ValidationError(nodeId, "", "Graph is null", ErrorSeverity::Critical, "Graph"));
            return errors;
        }

        const GraphNode* node = graph->GetNode(nodeId);
        if (!node)
        {
            errors.push_back(ValidationError(nodeId, "", 
                "Node not found", ErrorSeverity::Critical, "Node"));
            return errors;
        }

        ValidateNodeType(graph, node, errors);
        ValidateNodeParameters(graph, node, errors);
        ValidateNodeLinks(graph, node, errors);

        return errors;
    }

    bool BlueprintValidator::IsGraphValid(const NodeGraph* graph)
    {
        auto errors = ValidateGraph(graph);
        
        // Check if there are any critical or error level issues
        for (const auto& error : errors)
        {
            if (error.severity == ErrorSeverity::Error || error.severity == ErrorSeverity::Critical)
            {
                return false;
            }
        }
        
        return true;
    }

    int BlueprintValidator::GetErrorCount(const std::vector<ValidationError>& errors, ErrorSeverity severity) const
    {
        int count = 0;
        for (const auto& error : errors)
        {
            if (error.severity == severity)
                count++;
        }
        return count;
    }

    const char* BlueprintValidator::SeverityToString(ErrorSeverity severity)
    {
        switch (severity)
        {
            case ErrorSeverity::Info: return "Info";
            case ErrorSeverity::Warning: return "Warning";
            case ErrorSeverity::Error: return "Error";
            case ErrorSeverity::Critical: return "Critical";
            default: return "Unknown";
        }
    }

    ImVec4 BlueprintValidator::SeverityToColor(ErrorSeverity severity)
    {
        switch (severity)
        {
            case ErrorSeverity::Info: return ImVec4(0.5f, 0.5f, 1.0f, 1.0f);      // Blue
            case ErrorSeverity::Warning: return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);   // Yellow
            case ErrorSeverity::Error: return ImVec4(1.0f, 0.5f, 0.0f, 1.0f);     // Orange
            case ErrorSeverity::Critical: return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
            default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    void BlueprintValidator::ValidateNodeType(const NodeGraph* graph, const GraphNode* node, 
                                             std::vector<ValidationError>& errors)
    {
        // Validate Action nodes
        if (node->type == NodeType::BT_Action)
        {
            if (node->actionType.empty())
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Action node has no action type specified", ErrorSeverity::Error, "Type"));
                return;
            }

            if (!EnumCatalogManager::Get().IsValidActionType(node->actionType))
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Invalid or deprecated ActionType: " + node->actionType, 
                    ErrorSeverity::Error, "Type"));
            }
        }
        // Validate Condition nodes
        else if (node->type == NodeType::BT_Condition)
        {
            if (node->conditionType.empty())
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Condition node has no condition type specified", ErrorSeverity::Error, "Type"));
                return;
            }

            if (!EnumCatalogManager::Get().IsValidConditionType(node->conditionType))
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Invalid or deprecated ConditionType: " + node->conditionType, 
                    ErrorSeverity::Error, "Type"));
            }
        }
        // Validate Decorator nodes
        else if (node->type == NodeType::BT_Decorator)
        {
            if (node->decoratorType.empty())
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Decorator node has no decorator type specified", ErrorSeverity::Error, "Type"));
                return;
            }

            if (!EnumCatalogManager::Get().IsValidDecoratorType(node->decoratorType))
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Invalid or deprecated DecoratorType: " + node->decoratorType, 
                    ErrorSeverity::Error, "Type"));
            }
        }
    }

    void BlueprintValidator::ValidateNodeParameters(const NodeGraph* graph, const GraphNode* node, 
                                                   std::vector<ValidationError>& errors)
    {
        // Validate Action parameters
        if (node->type == NodeType::BT_Action && !node->actionType.empty())
        {
            const CatalogType* actionDef = EnumCatalogManager::Get().FindActionType(node->actionType);
            if (actionDef)
            {
                // Check for required parameters
                for (const auto& paramDef : actionDef->parameters)
                {
                    if (paramDef.required)
                    {
                        auto it = node->parameters.find(paramDef.name);
                        if (it == node->parameters.end() || it->second.empty())
                        {
                            errors.push_back(ValidationError(node->id, node->name,
                                "Missing required parameter: " + paramDef.name, 
                                ErrorSeverity::Error, "Parameter"));
                        }
                    }
                }
            }
        }
        // Validate Condition parameters
        else if (node->type == NodeType::BT_Condition && !node->conditionType.empty())
        {
            const CatalogType* conditionDef = EnumCatalogManager::Get().FindConditionType(node->conditionType);
            if (conditionDef)
            {
                for (const auto& paramDef : conditionDef->parameters)
                {
                    if (paramDef.required)
                    {
                        auto it = node->parameters.find(paramDef.name);
                        if (it == node->parameters.end() || it->second.empty())
                        {
                            errors.push_back(ValidationError(node->id, node->name,
                                "Missing required parameter: " + paramDef.name, 
                                ErrorSeverity::Error, "Parameter"));
                        }
                    }
                }
            }
        }
        // Validate Decorator parameters
        else if (node->type == NodeType::BT_Decorator && !node->decoratorType.empty())
        {
            const CatalogType* decoratorDef = EnumCatalogManager::Get().FindDecoratorType(node->decoratorType);
            if (decoratorDef)
            {
                for (const auto& paramDef : decoratorDef->parameters)
                {
                    if (paramDef.required)
                    {
                        auto it = node->parameters.find(paramDef.name);
                        if (it == node->parameters.end() || it->second.empty())
                        {
                            errors.push_back(ValidationError(node->id, node->name,
                                "Missing required parameter: " + paramDef.name, 
                                ErrorSeverity::Error, "Parameter"));
                        }
                    }
                }
            }
        }
    }

    void BlueprintValidator::ValidateNodeLinks(const NodeGraph* graph, const GraphNode* node, 
                                              std::vector<ValidationError>& errors)
    {
        // Validate composite nodes have children
        if (node->type == NodeType::BT_Sequence || node->type == NodeType::BT_Selector)
        {
            if (node->childIds.empty())
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Composite node has no children", ErrorSeverity::Warning, "Link"));
            }
        }

        // Validate decorator nodes have exactly one child
        if (node->type == NodeType::BT_Decorator)
        {
            if (node->decoratorChildId < 0)
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Decorator node has no child", ErrorSeverity::Error, "Link"));
            }
        }

        // Validate that child IDs actually exist
        for (int childId : node->childIds)
        {
            if (!graph->GetNode(childId))
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Child node " + std::to_string(childId) + " does not exist", 
                    ErrorSeverity::Error, "Link"));
            }
        }

        if (node->decoratorChildId >= 0 && !graph->GetNode(node->decoratorChildId))
        {
            errors.push_back(ValidationError(node->id, node->name,
                "Decorator child node " + std::to_string(node->decoratorChildId) + " does not exist", 
                ErrorSeverity::Error, "Link"));
        }
    }
}
