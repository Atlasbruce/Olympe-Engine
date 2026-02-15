/*
 * Olympe Blueprint Editor - Blueprint Validator Implementation
 */

#include "BlueprintValidator.h"
#include "EnumCatalogManager.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>
#include <set>
#include <map>

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
        
        // NEW: Connection rule validations
        ValidateConnectionRules(graph, errors);
        ValidateMultipleParents(graph, errors);
        ValidateCycles(graph, errors);
        ValidateOrphanNodes(graph, errors);
        ValidateRootNodes(graph, errors);

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
    
    // ========== Connection Rule Validations ==========
    
    void BlueprintValidator::ValidateConnectionRules(const NodeGraph* graph, 
                                                     std::vector<ValidationError>& errors)
    {
        if (!graph)
            return;
        
        auto nodes = graph->GetAllNodes();
        for (const GraphNode* node : nodes)
        {
            // Check if decorator has exactly one child
            if (node->type == NodeType::BT_Decorator)
            {
                int childCount = (node->decoratorChildId >= 0) ? 1 : 0;
                int minChildren = m_ConnectionValidator.GetMinChildrenForType(node->type);
                
                if (childCount < minChildren)
                {
                    errors.push_back(ValidationError(node->id, node->name,
                        "Decorator must have exactly 1 child (currently has " + 
                        std::to_string(childCount) + ")", 
                        ErrorSeverity::Error, "Connection"));
                }
            }
            // Check if composite has children
            else if (node->type == NodeType::BT_Sequence || node->type == NodeType::BT_Selector)
            {
                int childCount = static_cast<int>(node->childIds.size());
                int minChildren = m_ConnectionValidator.GetMinChildrenForType(node->type);
                
                if (childCount < minChildren)
                {
                    errors.push_back(ValidationError(node->id, node->name,
                        "Composite node should have at least " + std::to_string(minChildren) + 
                        " child (currently has " + std::to_string(childCount) + ")", 
                        ErrorSeverity::Warning, "Connection"));
                }
            }
            // Check if leaf nodes have no children
            else if (node->type == NodeType::BT_Action || node->type == NodeType::BT_Condition)
            {
                int childCount = static_cast<int>(node->childIds.size());
                if (node->decoratorChildId >= 0)
                    childCount++;
                
                if (childCount > 0)
                {
                    errors.push_back(ValidationError(node->id, node->name,
                        "Leaf node (Action/Condition) cannot have children", 
                        ErrorSeverity::Error, "Connection"));
                }
            }
        }
    }
    
    void BlueprintValidator::ValidateMultipleParents(const NodeGraph* graph, 
                                                     std::vector<ValidationError>& errors)
    {
        if (!graph)
            return;
        
        auto nodes = graph->GetAllNodes();
        
        // Count how many times each node appears as a child
        std::map<int, std::vector<int>> childToParents;
        
        for (const GraphNode* node : nodes)
        {
            // Check regular children
            for (int childId : node->childIds)
            {
                childToParents[childId].push_back(node->id);
            }
            
            // Check decorator child
            if (node->decoratorChildId >= 0)
            {
                childToParents[node->decoratorChildId].push_back(node->id);
            }
        }
        
        // Report nodes with multiple parents
        for (const auto& pair : childToParents)
        {
            if (pair.second.size() > 1)
            {
                const GraphNode* childNode = graph->GetNode(pair.first);
                std::string parentList;
                for (size_t i = 0; i < pair.second.size(); ++i)
                {
                    if (i > 0) parentList += ", ";
                    parentList += std::to_string(pair.second[i]);
                }
                
                errors.push_back(ValidationError(pair.first, 
                    childNode ? childNode->name : "Unknown",
                    "Node has multiple parents (" + parentList + "). Each node can only have one parent.", 
                    ErrorSeverity::Error, "Connection"));
            }
        }
    }
    
    void BlueprintValidator::ValidateCycles(const NodeGraph* graph, 
                                           std::vector<ValidationError>& errors)
    {
        if (!graph)
            return;
        
        // Check for cycles by verifying no node can reach itself
        auto nodes = graph->GetAllNodes();
        
        for (const GraphNode* node : nodes)
        {
            if (m_ConnectionValidator.WouldCreateCycle(graph, node->id, node->id))
            {
                errors.push_back(ValidationError(node->id, node->name,
                    "Node is part of a cycle in the tree", 
                    ErrorSeverity::Critical, "Connection"));
            }
        }
    }
    
    void BlueprintValidator::ValidateOrphanNodes(const NodeGraph* graph, 
                                                std::vector<ValidationError>& errors)
    {
        if (!graph)
            return;
        
        std::set<int> orphans = m_ConnectionValidator.GetOrphanNodes(graph);
        
        for (int orphanId : orphans)
        {
            const GraphNode* orphanNode = graph->GetNode(orphanId);
            errors.push_back(ValidationError(orphanId, 
                orphanNode ? orphanNode->name : "Unknown",
                "Orphan node detected - node has no parent and is not the root", 
                ErrorSeverity::Warning, "Connection"));
        }
    }
    
    void BlueprintValidator::ValidateRootNodes(const NodeGraph* graph, 
                                              std::vector<ValidationError>& errors)
    {
        if (!graph)
            return;
        
        std::set<int> roots = m_ConnectionValidator.GetRootNodes(graph);
        
        // Check if there are multiple roots (including orphans)
        if (roots.size() > 1)
        {
            errors.push_back(ValidationError(-1, graph->name,
                "Multiple root nodes detected (" + std::to_string(roots.size()) + 
                "). Tree should have exactly one root.", 
                ErrorSeverity::Error, "Connection"));
        }
        
        // Check if root node exists
        if (graph->rootNodeId >= 0)
        {
            const GraphNode* rootNode = graph->GetNode(graph->rootNodeId);
            if (!rootNode)
            {
                errors.push_back(ValidationError(graph->rootNodeId, "",
                    "Root node does not exist in graph", 
                    ErrorSeverity::Critical, "Connection"));
            }
            else
            {
                // Check if root has a parent
                int parentId = m_ConnectionValidator.GetParentNode(graph, graph->rootNodeId);
                if (parentId >= 0)
                {
                    errors.push_back(ValidationError(graph->rootNodeId, rootNode->name,
                        "Root node has a parent (node " + std::to_string(parentId) + "). Root cannot have a parent.", 
                        ErrorSeverity::Error, "Connection"));
                }
            }
        }
    }
}
