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
    
    // ========== JSON Schema Validation and Normalization ==========
    
    std::string BlueprintValidator::DetectType(const nlohmann::json& blueprint)
    {
        // Check if type is already specified
        if (blueprint.contains("type") && blueprint["type"].is_string())
        {
            return blueprint["type"].get<std::string>();
        }
        
        // Use heuristics to detect type
        // BehaviorTree: has rootNodeId + nodes array
        if (blueprint.contains("rootNodeId") && blueprint.contains("nodes"))
        {
            return "BehaviorTree";
        }
        
        // HFSM: has states or initialState
        if (blueprint.contains("states") || blueprint.contains("initialState"))
        {
            return "HFSM";
        }
        
        // EntityBlueprint: has components array at root
        if (blueprint.contains("components") && blueprint["components"].is_array())
        {
            return "EntityBlueprint";
        }
        
        // EntityPrefab: has data.prefabName or data.components
        if (blueprint.contains("data"))
        {
            const auto& data = blueprint["data"];
            if (data.contains("prefabName") || data.contains("components"))
            {
                return "EntityPrefab";
            }
        }
        
        // UI Blueprint: has elements array
        if (blueprint.contains("elements") && blueprint["elements"].is_array())
        {
            return "UIBlueprint";
        }
        
        // Level: has worldSize or entities
        if (blueprint.contains("worldSize") || blueprint.contains("entities"))
        {
            return "Level";
        }
        
        // Catalog types
        if (blueprint.contains("catalogType"))
        {
            return "Catalog";
        }
        
        // Template
        if (blueprint.contains("blueprintData"))
        {
            return "Template";
        }
        
        // Default to Generic
        return "Generic";
    }
    
    bool BlueprintValidator::Normalize(nlohmann::json& blueprint)
    {
        bool modified = false;
        
        // Ensure schema_version exists
        if (!blueprint.contains("schema_version"))
        {
            blueprint["schema_version"] = 2;
            modified = true;
        }
        
        // Detect and add type if missing
        std::string detectedType = DetectType(blueprint);
        if (!blueprint.contains("type"))
        {
            blueprint["type"] = detectedType;
            modified = true;
        }
        
        // Add blueprintType if missing (should match type)
        if (!blueprint.contains("blueprintType"))
        {
            std::string type = blueprint.contains("type") ? 
                blueprint["type"].get<std::string>() : detectedType;
            blueprint["blueprintType"] = type;
            modified = true;
        }
        
        // Ensure metadata exists
        if (!blueprint.contains("metadata"))
        {
            blueprint["metadata"] = nlohmann::json::object();
            blueprint["metadata"]["author"] = "Unknown";
            blueprint["metadata"]["created"] = "";
            blueprint["metadata"]["lastModified"] = "";
            blueprint["metadata"]["tags"] = nlohmann::json::array();
            modified = true;
        }
        
        // Ensure editorState exists
        if (!blueprint.contains("editorState"))
        {
            blueprint["editorState"] = nlohmann::json::object();
            blueprint["editorState"]["zoom"] = 1.0f;
            blueprint["editorState"]["scrollOffset"] = nlohmann::json::object();
            blueprint["editorState"]["scrollOffset"]["x"] = 0.0f;
            blueprint["editorState"]["scrollOffset"]["y"] = 0.0f;
            modified = true;
        }
        
        return modified;
    }
    
    bool BlueprintValidator::ValidateJSON(const nlohmann::json& blueprint, std::string& errors)
    {
        errors.clear();
        
        // Check if type exists
        if (!blueprint.contains("type"))
        {
            errors = "Missing 'type' field";
            return false;
        }
        
        std::string type = blueprint["type"].get<std::string>();
        
        // Validate based on type
        if (type == "BehaviorTree")
        {
            return ValidateBehaviorTree(blueprint, errors);
        }
        else if (type == "HFSM")
        {
            return ValidateHFSM(blueprint, errors);
        }
        else if (type == "EntityPrefab" || type == "EntityBlueprint")
        {
            return ValidateEntityPrefab(blueprint, errors);
        }
        else if (type == "UIBlueprint")
        {
            return ValidateUIBlueprint(blueprint, errors);
        }
        else if (type == "Level")
        {
            return ValidateLevel(blueprint, errors);
        }
        
        // Generic or unknown types are considered valid
        return true;
    }
    
    bool BlueprintValidator::ValidateBehaviorTree(const nlohmann::json& blueprint, std::string& errors)
    {
        // Check for data wrapper (v2 format)
        const nlohmann::json* data = &blueprint;
        if (blueprint.contains("data") && blueprint["data"].is_object())
        {
            data = &blueprint["data"];
        }
        
        // Required fields for BehaviorTree
        if (!data->contains("nodes") || !(*data)["nodes"].is_array())
        {
            errors = "BehaviorTree missing 'nodes' array";
            return false;
        }
        
        if (!data->contains("rootNodeId"))
        {
            errors = "BehaviorTree missing 'rootNodeId'";
            return false;
        }
        
        return true;
    }
    
    bool BlueprintValidator::ValidateHFSM(const nlohmann::json& blueprint, std::string& errors)
    {
        // Check for data wrapper (v2 format)
        const nlohmann::json* data = &blueprint;
        if (blueprint.contains("data") && blueprint["data"].is_object())
        {
            data = &blueprint["data"];
        }
        
        // Required fields for HFSM
        if (!data->contains("states") || !(*data)["states"].is_array())
        {
            errors = "HFSM missing 'states' array";
            return false;
        }
        
        if (!data->contains("initialState"))
        {
            errors = "HFSM missing 'initialState'";
            return false;
        }
        
        return true;
    }
    
    bool BlueprintValidator::ValidateEntityPrefab(const nlohmann::json& blueprint, std::string& errors)
    {
        // Check for data wrapper (v2 format)
        const nlohmann::json* data = &blueprint;
        if (blueprint.contains("data") && blueprint["data"].is_object())
        {
            data = &blueprint["data"];
        }
        
        // Required fields for EntityPrefab
        if (!data->contains("components") || !(*data)["components"].is_array())
        {
            errors = "EntityPrefab missing 'components' array";
            return false;
        }
        
        return true;
    }
    
    bool BlueprintValidator::ValidateUIBlueprint(const nlohmann::json& blueprint, std::string& errors)
    {
        // Required fields for UIBlueprint
        if (!blueprint.contains("elements") || !blueprint["elements"].is_array())
        {
            errors = "UIBlueprint missing 'elements' array";
            return false;
        }
        
        return true;
    }
    
    bool BlueprintValidator::ValidateLevel(const nlohmann::json& blueprint, std::string& errors)
    {
        // Required fields for Level
        if (!blueprint.contains("worldSize") && !blueprint.contains("entities"))
        {
            errors = "Level missing 'worldSize' or 'entities'";
            return false;
        }
        
        return true;
    }
}
