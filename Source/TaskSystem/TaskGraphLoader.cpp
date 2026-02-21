/**
 * @file TaskGraphLoader.cpp
 * @brief Implementation of TaskGraphLoader for the Atomic Task System
 * @author Olympe Engine
 * @date 2026-02-21
 */

#include "TaskGraphLoader.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "../system/system_utils.h"
#include "../json_helper.h"

namespace Olympe {

// ============================================================================
// Public API
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::LoadFromFile(const std::string& path,
                                                  std::vector<std::string>& outErrors)
{
    SYSTEM_LOG << "[TaskGraphLoader] Loading from file: " << path << std::endl;

    json data;
    if (!JsonHelper::LoadJsonFromFile(path, data))
    {
        std::string msg = "Failed to open or parse JSON file: " + path;
        outErrors.push_back(msg);
        SYSTEM_LOG << "[TaskGraphLoader] ERROR: " << msg << std::endl;
        return nullptr;
    }

    return LoadFromJson(data, outErrors);
}

// ----------------------------------------------------------------------------

TaskGraphTemplate* TaskGraphLoader::LoadFromJson(const json& data,
                                                  std::vector<std::string>& outErrors)
{
    // Detect schema version; default to 2 if not present
    int schemaVersion = JsonHelper::GetInt(data, "schema_version", 2);

    SYSTEM_LOG << "[TaskGraphLoader] Schema version: " << schemaVersion << std::endl;

    TaskGraphTemplate* tmpl = nullptr;

    if (schemaVersion == 3)
    {
        tmpl = ParseSchemaV3(data, outErrors);
    }
    else
    {
        // Treat anything else (including 2 and absent) as schema v2
        tmpl = ParseSchemaV2(data, outErrors);
    }

    if (tmpl == nullptr)
    {
        return nullptr;
    }

    // Build lookup cache and run validation
    tmpl->BuildLookupCache();

    if (!tmpl->Validate())
    {
        outErrors.push_back("TaskGraphTemplate::Validate() failed for template '" + tmpl->Name + "'");
        delete tmpl;
        return nullptr;
    }

    SYSTEM_LOG << "[TaskGraphLoader] Successfully loaded template '" << tmpl->Name
               << "' with " << tmpl->Nodes.size() << " nodes" << std::endl;

    return tmpl;
}

// ----------------------------------------------------------------------------

bool TaskGraphLoader::ValidateJson(const json& data,
                                   std::vector<std::string>& outErrors)
{
    bool valid = true;

    // Must have a data section
    if (!JsonHelper::IsObject(data, "data"))
    {
        outErrors.push_back("Missing required 'data' object in JSON");
        valid = false;
    }
    else
    {
        const json& dataSection = data["data"];

        // Must have nodes array
        if (!JsonHelper::IsArray(dataSection, "nodes"))
        {
            outErrors.push_back("Missing required 'nodes' array in data section");
            valid = false;
        }

        // Must have rootNodeId
        if (!dataSection.contains("rootNodeId"))
        {
            outErrors.push_back("Missing required 'rootNodeId' in data section");
            valid = false;
        }
    }

    return valid;
}

// ============================================================================
// Schema v2 parsing
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::ParseSchemaV2(const json& data,
                                                   std::vector<std::string>& outErrors)
{
    // Validate structure first
    std::vector<std::string> validationErrors;
    if (!ValidateJson(data, validationErrors))
    {
        for (size_t i = 0; i < validationErrors.size(); ++i)
        {
            outErrors.push_back(validationErrors[i]);
        }
        return nullptr;
    }

    TaskGraphTemplate* tmpl = new TaskGraphTemplate();

    // Top-level metadata
    tmpl->Name        = JsonHelper::GetString(data, "name", "Unnamed");
    tmpl->Description = JsonHelper::GetString(data, "description", "");

    const json& dataSection = data["data"];

    // Root node ID
    tmpl->RootNodeID = JsonHelper::GetInt(dataSection, "rootNodeId", -1);

    // Parse nodes array
    JsonHelper::ForEachInArray(dataSection, "nodes", [&](const json& nodeJson, size_t /*index*/)
    {
        TaskNodeDefinition nodeDef = ParseNodeV2(nodeJson, outErrors);
        tmpl->Nodes.push_back(nodeDef);
    });

    // Parse local variables (optional in v2)
    if (JsonHelper::IsArray(dataSection, "localVariables"))
    {
        JsonHelper::ForEachInArray(dataSection, "localVariables",
            [&](const json& varJson, size_t /*index*/)
        {
            VariableDefinition varDef;
            varDef.Name    = JsonHelper::GetString(varJson, "name", "");
            varDef.IsLocal = JsonHelper::GetBool(varJson, "isLocal", true);

            std::string typeStr = JsonHelper::GetString(varJson, "type", "None");
            if      (typeStr == "Bool")     { varDef.Type = VariableType::Bool; }
            else if (typeStr == "Int")      { varDef.Type = VariableType::Int; }
            else if (typeStr == "Float")    { varDef.Type = VariableType::Float; }
            else if (typeStr == "Vector")   { varDef.Type = VariableType::Vector; }
            else if (typeStr == "EntityID") { varDef.Type = VariableType::EntityID; }
            else if (typeStr == "String")   { varDef.Type = VariableType::String; }
            else                            { varDef.Type = VariableType::None; }

            if (!varDef.Name.empty())
            {
                tmpl->LocalVariables.push_back(varDef);
            }
        });
    }

    return tmpl;
}

// ============================================================================
// Schema v3 parsing
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::ParseSchemaV3(const json& data,
                                                   std::vector<std::string>& outErrors)
{
    // Validate structure first
    std::vector<std::string> validationErrors;
    if (!ValidateJson(data, validationErrors))
    {
        for (size_t i = 0; i < validationErrors.size(); ++i)
        {
            outErrors.push_back(validationErrors[i]);
        }
        return nullptr;
    }

    TaskGraphTemplate* tmpl = new TaskGraphTemplate();

    tmpl->Name        = JsonHelper::GetString(data, "name", "Unnamed");
    tmpl->Description = JsonHelper::GetString(data, "description", "");

    const json& dataSection = data["data"];

    tmpl->RootNodeID = JsonHelper::GetInt(dataSection, "rootNodeId", -1);

    // Parse nodes array
    JsonHelper::ForEachInArray(dataSection, "nodes", [&](const json& nodeJson, size_t /*index*/)
    {
        TaskNodeDefinition nodeDef = ParseNodeV3(nodeJson, outErrors);
        tmpl->Nodes.push_back(nodeDef);
    });

    // Parse local variables (optional)
    if (JsonHelper::IsArray(dataSection, "localVariables"))
    {
        JsonHelper::ForEachInArray(dataSection, "localVariables",
            [&](const json& varJson, size_t /*index*/)
        {
            VariableDefinition varDef;
            varDef.Name    = JsonHelper::GetString(varJson, "name", "");
            varDef.IsLocal = JsonHelper::GetBool(varJson, "isLocal", true);

            std::string typeStr = JsonHelper::GetString(varJson, "type", "None");
            if      (typeStr == "Bool")     { varDef.Type = VariableType::Bool; }
            else if (typeStr == "Int")      { varDef.Type = VariableType::Int; }
            else if (typeStr == "Float")    { varDef.Type = VariableType::Float; }
            else if (typeStr == "Vector")   { varDef.Type = VariableType::Vector; }
            else if (typeStr == "EntityID") { varDef.Type = VariableType::EntityID; }
            else if (typeStr == "String")   { varDef.Type = VariableType::String; }
            else                            { varDef.Type = VariableType::None; }

            if (!varDef.Name.empty())
            {
                tmpl->LocalVariables.push_back(varDef);
            }
        });
    }

    return tmpl;
}

// ============================================================================
// Node parsing - schema v2
// ============================================================================

TaskNodeDefinition TaskGraphLoader::ParseNodeV2(const json& nodeJson,
                                                 std::vector<std::string>& outErrors)
{
    TaskNodeDefinition nodeDef;

    nodeDef.NodeID   = JsonHelper::GetInt(nodeJson, "id", -1);
    nodeDef.NodeName = JsonHelper::GetString(nodeJson, "name", "");

    std::string typeStr = JsonHelper::GetString(nodeJson, "type", "");

    if (typeStr == "Selector")
    {
        nodeDef.Type = TaskNodeType::Selector;
    }
    else if (typeStr == "Sequence")
    {
        nodeDef.Type = TaskNodeType::Sequence;
    }
    else if (typeStr == "Parallel")
    {
        nodeDef.Type = TaskNodeType::Parallel;
    }
    else if (typeStr == "Repeater")
    {
        nodeDef.Type = TaskNodeType::Decorator;

        // Repeater wraps a single child via decoratorChildId
        int childId = JsonHelper::GetInt(nodeJson, "decoratorChildId", -1);
        if (childId >= 0)
        {
            nodeDef.ChildrenIDs.push_back(childId);
        }

        // Store repeatCount as a Literal int parameter
        int repeatCount = JsonHelper::GetInt(nodeJson, "repeatCount", 1);
        ParameterBinding repeatBinding;
        repeatBinding.Type         = ParameterBindingType::Literal;
        repeatBinding.LiteralValue = TaskValue(repeatCount);
        nodeDef.Parameters["repeatCount"] = repeatBinding;
    }
    else if (typeStr == "Action")
    {
        nodeDef.Type         = TaskNodeType::AtomicTask;
        nodeDef.AtomicTaskID = JsonHelper::GetString(nodeJson, "actionType", "");
    }
    else if (typeStr == "Condition")
    {
        nodeDef.Type         = TaskNodeType::AtomicTask;
        nodeDef.AtomicTaskID = JsonHelper::GetString(nodeJson, "conditionType", "");
    }
    else
    {
        // Unknown node type: warn and create AtomicTask with ID "unknown"
        std::string warnMsg = "Node " + std::to_string(nodeDef.NodeID)
                            + " has unknown type '" + typeStr + "'; treating as AtomicTask(unknown)";
        SYSTEM_LOG << "[TaskGraphLoader] WARNING: " << warnMsg << std::endl;
        outErrors.push_back(warnMsg);
        nodeDef.Type         = TaskNodeType::AtomicTask;
        nodeDef.AtomicTaskID = "unknown";
    }

    // Parse children array (for composite nodes); skip for Repeater (handled above)
    if (typeStr != "Repeater" && JsonHelper::IsArray(nodeJson, "children"))
    {
        const json& children = nodeJson["children"];
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (children[i].is_number())
            {
                nodeDef.ChildrenIDs.push_back(children[i].get<int>());
            }
        }
    }

    // Parse parameters object
    if (JsonHelper::IsObject(nodeJson, "parameters"))
    {
        ParseParameters(nodeJson["parameters"], nodeDef.Parameters);
    }

    // nextOnSuccess / nextOnFailure (optional flow overrides in v2)
    nodeDef.NextOnSuccess = JsonHelper::GetInt(nodeJson, "nextOnSuccess", -1);
    nodeDef.NextOnFailure = JsonHelper::GetInt(nodeJson, "nextOnFailure", -1);

    return nodeDef;
}

// ============================================================================
// Node parsing - schema v3
// ============================================================================

TaskNodeDefinition TaskGraphLoader::ParseNodeV3(const json& nodeJson,
                                                 std::vector<std::string>& outErrors)
{
    TaskNodeDefinition nodeDef;

    nodeDef.NodeID   = JsonHelper::GetInt(nodeJson, "id", -1);
    nodeDef.NodeName = JsonHelper::GetString(nodeJson, "name", "");

    std::string typeStr = JsonHelper::GetString(nodeJson, "type", "");

    if (typeStr == "Selector")
    {
        nodeDef.Type = TaskNodeType::Selector;
    }
    else if (typeStr == "Sequence")
    {
        nodeDef.Type = TaskNodeType::Sequence;
    }
    else if (typeStr == "Parallel")
    {
        nodeDef.Type = TaskNodeType::Parallel;
    }
    else if (typeStr == "Decorator")
    {
        nodeDef.Type = TaskNodeType::Decorator;
    }
    else if (typeStr == "Root")
    {
        nodeDef.Type = TaskNodeType::Root;
    }
    else if (typeStr == "AtomicTask")
    {
        nodeDef.Type         = TaskNodeType::AtomicTask;
        nodeDef.AtomicTaskID = JsonHelper::GetString(nodeJson, "atomicTaskId", "");
    }
    else
    {
        // Unknown type: warn and treat as AtomicTask
        std::string warnMsg = "Node " + std::to_string(nodeDef.NodeID)
                            + " has unknown type '" + typeStr + "'; treating as AtomicTask(unknown)";
        SYSTEM_LOG << "[TaskGraphLoader] WARNING: " << warnMsg << std::endl;
        outErrors.push_back(warnMsg);
        nodeDef.Type         = TaskNodeType::AtomicTask;
        nodeDef.AtomicTaskID = "unknown";
    }

    // Parse children array
    if (JsonHelper::IsArray(nodeJson, "children"))
    {
        const json& children = nodeJson["children"];
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (children[i].is_number())
            {
                nodeDef.ChildrenIDs.push_back(children[i].get<int>());
            }
        }
    }

    // Decorator child (v3 may also use decoratorChildId)
    if (typeStr == "Decorator")
    {
        int childId = JsonHelper::GetInt(nodeJson, "decoratorChildId", -1);
        if (childId >= 0 && nodeDef.ChildrenIDs.empty())
        {
            nodeDef.ChildrenIDs.push_back(childId);
        }

        int repeatCount = JsonHelper::GetInt(nodeJson, "repeatCount", 1);
        ParameterBinding repeatBinding;
        repeatBinding.Type         = ParameterBindingType::Literal;
        repeatBinding.LiteralValue = TaskValue(repeatCount);
        nodeDef.Parameters["repeatCount"] = repeatBinding;
    }

    // Parse parameters
    if (JsonHelper::IsObject(nodeJson, "parameters"))
    {
        ParseParameters(nodeJson["parameters"], nodeDef.Parameters);
    }

    nodeDef.NextOnSuccess = JsonHelper::GetInt(nodeJson, "nextOnSuccess", -1);
    nodeDef.NextOnFailure = JsonHelper::GetInt(nodeJson, "nextOnFailure", -1);

    return nodeDef;
}

// ============================================================================
// Parameter parsing
// ============================================================================

void TaskGraphLoader::ParseParameters(const json& paramsJson,
                                      std::unordered_map<std::string, ParameterBinding>& outParams)
{
    // Iterate over all key-value pairs in the parameters object
    for (auto it = paramsJson.begin(); it != paramsJson.end(); ++it)
    {
        std::string paramName  = it.key();
        const json& paramValue = it.value();

        ParameterBinding binding;

        if (paramValue.is_object())
        {
            // Structured binding: check for bindingType field
            std::string bindingType = JsonHelper::GetString(paramValue, "bindingType", "Literal");

            if (bindingType == "Variable" || bindingType == "LocalVariable")
            {
                binding.Type         = ParameterBindingType::LocalVariable;
                binding.VariableName = JsonHelper::GetString(paramValue, "variableName", "");
            }
            else
            {
                // Literal binding with nested value
                binding.Type = ParameterBindingType::Literal;
                if (paramValue.contains("value"))
                {
                    binding.LiteralValue = ParsePrimitiveValue(paramValue["value"]);
                }
            }
        }
        else
        {
            // Primitive value: create Literal binding
            binding.Type         = ParameterBindingType::Literal;
            binding.LiteralValue = ParsePrimitiveValue(paramValue);
        }

        outParams[paramName] = binding;
    }
}

// ============================================================================
// Primitive value parsing
// ============================================================================

TaskValue TaskGraphLoader::ParsePrimitiveValue(const json& val)
{
    if (val.is_boolean())
    {
        return TaskValue(val.get<bool>());
    }
    if (val.is_number_integer())
    {
        return TaskValue(val.get<int>());
    }
    if (val.is_number_float())
    {
        return TaskValue(static_cast<float>(val.get<double>()));
    }
    if (val.is_string())
    {
        return TaskValue(val.get<std::string>());
    }

    // Unsupported type: return default (None)
    return TaskValue();
}

} // namespace Olympe
