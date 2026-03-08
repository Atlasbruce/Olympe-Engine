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
    else if (schemaVersion == 4)
    {
        tmpl = ParseSchemaV4(data, outErrors);
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

        // Must have rootNodeId.
        // Node IDs are always >= 1 in all supported schemas; -1 is the sentinel
        // returned by GetInt when the key is absent or invalid.
        if (JsonHelper::GetInt(dataSection, "rootNodeId", -1) < 0)
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

    // Root node ID: prefer top-level "rootNodeId" if present, fall back to data section
    tmpl->RootNodeID = ResolveRootNodeId(data, dataSection);

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

    // Root node ID: prefer top-level "rootNodeId" if present, fall back to data section
    tmpl->RootNodeID = ResolveRootNodeId(data, dataSection);

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
                json valueJson;
                if (GetChildValue(paramValue, "value", valueJson))
                {
                    binding.LiteralValue = ParsePrimitiveValue(valueJson);
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

// ============================================================================
// Safe child-value accessor
// ============================================================================

bool TaskGraphLoader::GetChildValue(const json& obj, const std::string& key, json& outVal)
{
    if (obj.contains(key))
    {
        outVal = obj[key];
        return true;
    }
    return false;
}

// ============================================================================
// Root node ID resolution
// ============================================================================

int TaskGraphLoader::ResolveRootNodeId(const json& data, const json& dataSection)
{
    // Prefer top-level "rootNodeId" when present (some schemas place it at document root)
    int topLevelRoot = JsonHelper::GetInt(data, "rootNodeId", -1);
    if (topLevelRoot >= 0)
    {
        return topLevelRoot;
    }
    // Fall back to standard location inside the data section
    return JsonHelper::GetInt(dataSection, "rootNodeId", -1);
}

// ============================================================================
// Schema v4 parsing (ATS Visual Scripting – Phase 1)
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::ParseSchemaV4(const json& data,
                                                   std::vector<std::string>& outErrors)
{
    // v4 does not require rootNodeId (uses entryPointId instead).
    // Only check that nodes array exists.
    if (!JsonHelper::IsObject(data, "data"))
    {
        outErrors.push_back("[v4] Missing required 'data' object in JSON");
        return nullptr;
    }

    const json& dataSection = data["data"];

    if (!JsonHelper::IsArray(dataSection, "nodes"))
    {
        outErrors.push_back("[v4] Missing required 'nodes' array in data section");
        return nullptr;
    }

    TaskGraphTemplate* tmpl = new TaskGraphTemplate();

    tmpl->Name        = JsonHelper::GetString(data, "name", "Unnamed");
    tmpl->Description = JsonHelper::GetString(data, "description", "");
    tmpl->GraphType   = JsonHelper::GetString(data, "graph_type", "VisualScript");

    // Entry point ID
    tmpl->EntryPointID = JsonHelper::GetInt(dataSection, "entryPointId", NODE_INDEX_NONE);

    // For backward-compat: if rootNodeId is present, read it too
    tmpl->RootNodeID = ResolveRootNodeId(data, dataSection);
    // For VS graphs, entry point is the root
    if (tmpl->RootNodeID == NODE_INDEX_NONE && tmpl->EntryPointID != NODE_INDEX_NONE)
    {
        tmpl->RootNodeID = tmpl->EntryPointID;
    }

    // Parse nodes
    JsonHelper::ForEachInArray(dataSection, "nodes",
        [&](const json& nodeJson, size_t /*index*/)
    {
        TaskNodeDefinition nodeDef = ParseNodeV4(nodeJson, tmpl->GraphType, outErrors);
        tmpl->Nodes.push_back(nodeDef);
    });

    // Parse blackboard
    ParseBlackboard(dataSection, tmpl, outErrors);

    // Parse exec connections
    ParseExecConnections(dataSection, tmpl);

    // Parse data connections
    ParseDataConnections(dataSection, tmpl);

    return tmpl;
}

// ----------------------------------------------------------------------------

TaskNodeDefinition TaskGraphLoader::ParseNodeV4(const json& nodeJson,
                                                 const std::string& graphType,
                                                 std::vector<std::string>& outErrors)
{
    TaskNodeDefinition nodeDef;

    nodeDef.NodeID   = JsonHelper::GetInt(nodeJson, "id", -1);
    nodeDef.NodeName = JsonHelper::GetString(nodeJson, "name", "");

    std::string typeStr = JsonHelper::GetString(nodeJson, "type", "");

    bool typeOk = true;
    nodeDef.Type = StringToNodeType(typeStr, graphType, typeOk);

    if (!typeOk)
    {
        std::string warnMsg = "Node " + std::to_string(nodeDef.NodeID)
                            + " has unknown type '" + typeStr + "'; treating as AtomicTask(unknown)";
        SYSTEM_LOG << "[TaskGraphLoader] WARNING: " << warnMsg << std::endl;
        outErrors.push_back(warnMsg);
        nodeDef.AtomicTaskID = "unknown";
    }

    // Parse exec pins (informational in Phase 1)
    ParseExecPins(nodeJson, nodeDef);

    // Parse data pins
    ParseDataPins(nodeJson, nodeDef);

    // Parse parameters object
    if (JsonHelper::IsObject(nodeJson, "parameters"))
    {
        ParseParameters(nodeJson["parameters"], nodeDef.Parameters);

        // Extract VS-specific fields from parameters
        const json& params = nodeJson["parameters"];
        nodeDef.ConditionID    = JsonHelper::GetString(nodeJson, "conditionId", "");
        nodeDef.BBKey          = JsonHelper::GetString(params, "key", "");
        nodeDef.MathOperator   = JsonHelper::GetString(params, "operator", "");
        nodeDef.DelaySeconds   = JsonHelper::GetFloat(params, "delaySeconds", 0.0f);
        nodeDef.SubGraphPath   = JsonHelper::GetString(params, "subGraphPath", "");
    }

    // conditionId may also be at the node level
    if (nodeDef.ConditionID.empty())
    {
        nodeDef.ConditionID = JsonHelper::GetString(nodeJson, "conditionId", "");
    }

    // Switch cases
    if (JsonHelper::IsArray(nodeJson, "switch_cases"))
    {
        JsonHelper::ForEachInArray(nodeJson, "switch_cases",
            [&](const json& caseJson, size_t /*idx*/)
        {
            if (caseJson.is_string())
            {
                nodeDef.SwitchCases.push_back(caseJson.get<std::string>());
            }
        });
    }

    // nextOnSuccess / nextOnFailure (optional compat fields)
    nodeDef.NextOnSuccess = JsonHelper::GetInt(nodeJson, "nextOnSuccess", NODE_INDEX_NONE);
    nodeDef.NextOnFailure = JsonHelper::GetInt(nodeJson, "nextOnFailure", NODE_INDEX_NONE);

    return nodeDef;
}

// ----------------------------------------------------------------------------

void TaskGraphLoader::ParseExecPins(const json& nodeJson, TaskNodeDefinition& /*nodeDef*/)
{
    // Exec pins are informational in Phase 1; the runtime will use ExecConnections
    // to navigate. No storage needed in NodeDefinition for exec pins at this stage.
    (void)nodeJson;
}

// ----------------------------------------------------------------------------

void TaskGraphLoader::ParseDataPins(const json& nodeJson, TaskNodeDefinition& nodeDef)
{
    if (!JsonHelper::IsArray(nodeJson, "data_pins"))
    {
        return;
    }

    JsonHelper::ForEachInArray(nodeJson, "data_pins",
        [&](const json& pinJson, size_t /*idx*/)
    {
        DataPinDefinition pin;
        pin.PinName = JsonHelper::GetString(pinJson, "name", "");
        pin.PinType = StringToVariableType(JsonHelper::GetString(pinJson, "type", "None"));
        pin.Dir     = StringToDataPinDir(JsonHelper::GetString(pinJson, "dir", "Input"));

        if (JsonHelper::IsObject(pinJson, "default") || pinJson.contains("default"))
        {
            json defaultVal;
            if (GetChildValue(pinJson, "default", defaultVal))
            {
                pin.Default = ParsePrimitiveValue(defaultVal);
            }
        }

        nodeDef.DataPins.push_back(pin);
    });
}

// ----------------------------------------------------------------------------

void TaskGraphLoader::ParseBlackboard(const json& dataSection,
                                      TaskGraphTemplate* tmpl,
                                      std::vector<std::string>& /*outErrors*/)
{
    if (!JsonHelper::IsArray(dataSection, "blackboard"))
    {
        return;
    }

    JsonHelper::ForEachInArray(dataSection, "blackboard",
        [&](const json& entryJson, size_t /*idx*/)
    {
        BlackboardEntry entry;
        entry.Key      = JsonHelper::GetString(entryJson, "key", "");
        entry.Type     = StringToVariableType(JsonHelper::GetString(entryJson, "type", "None"));
        entry.IsGlobal = JsonHelper::GetBool(entryJson, "global", false);

        json defaultVal;
        if (GetChildValue(entryJson, "default", defaultVal))
        {
            entry.Default = ParsePrimitiveValue(defaultVal);
        }

        if (!entry.Key.empty())
        {
            tmpl->Blackboard.push_back(entry);
        }
    });
}

// ----------------------------------------------------------------------------

void TaskGraphLoader::ParseExecConnections(const json& dataSection,
                                           TaskGraphTemplate* tmpl)
{
    if (!JsonHelper::IsArray(dataSection, "exec_connections"))
    {
        return;
    }

    JsonHelper::ForEachInArray(dataSection, "exec_connections",
        [&](const json& connJson, size_t /*idx*/)
    {
        ExecPinConnection conn;
        conn.SourceNodeID  = JsonHelper::GetInt(connJson, "from_node", NODE_INDEX_NONE);
        conn.SourcePinName = JsonHelper::GetString(connJson, "from_pin", "");
        conn.TargetNodeID  = JsonHelper::GetInt(connJson, "to_node", NODE_INDEX_NONE);
        conn.TargetPinName = JsonHelper::GetString(connJson, "to_pin", "");
        tmpl->ExecConnections.push_back(conn);
    });
}

// ----------------------------------------------------------------------------

void TaskGraphLoader::ParseDataConnections(const json& dataSection,
                                           TaskGraphTemplate* tmpl)
{
    if (!JsonHelper::IsArray(dataSection, "data_connections"))
    {
        return;
    }

    JsonHelper::ForEachInArray(dataSection, "data_connections",
        [&](const json& connJson, size_t /*idx*/)
    {
        DataPinConnection conn;
        conn.SourceNodeID  = JsonHelper::GetInt(connJson, "from_node", NODE_INDEX_NONE);
        conn.SourcePinName = JsonHelper::GetString(connJson, "from_pin", "");
        conn.TargetNodeID  = JsonHelper::GetInt(connJson, "to_node", NODE_INDEX_NONE);
        conn.TargetPinName = JsonHelper::GetString(connJson, "to_pin", "");
        tmpl->DataConnections.push_back(conn);
    });
}

// ============================================================================
// String → enum conversion helpers
// ============================================================================

TaskNodeType TaskGraphLoader::StringToNodeType(const std::string& s,
                                               const std::string& graphType,
                                               bool& outOk)
{
    outOk = true;

    if (s == "EntryPoint")  { return TaskNodeType::EntryPoint; }
    if (s == "Branch")      { return TaskNodeType::Branch; }
    if (s == "Switch")      { return TaskNodeType::Switch; }
    if (s == "Sequence")
    {
        // In a VisualScript graph, "Sequence" maps to VSSequence to avoid
        // confusion with BehaviorTree Sequence semantics.
        if (graphType == "VisualScript")
        {
            return TaskNodeType::VSSequence;
        }
        return TaskNodeType::Sequence;
    }
    if (s == "While")       { return TaskNodeType::While; }
    if (s == "ForEach")     { return TaskNodeType::ForEach; }
    if (s == "DoOnce")      { return TaskNodeType::DoOnce; }
    if (s == "Delay")       { return TaskNodeType::Delay; }
    if (s == "GetBBValue")  { return TaskNodeType::GetBBValue; }
    if (s == "SetBBValue")  { return TaskNodeType::SetBBValue; }
    if (s == "Math")        { return TaskNodeType::MathOp; }
    if (s == "SubGraph")    { return TaskNodeType::SubGraph; }

    // Compat with v2 type strings
    if (s == "Action")      { return TaskNodeType::AtomicTask; }
    if (s == "Condition")   { return TaskNodeType::AtomicTask; }
    if (s == "AtomicTask")  { return TaskNodeType::AtomicTask; }

    // v3 / shared type strings
    if (s == "Selector")    { return TaskNodeType::Selector; }
    if (s == "Parallel")    { return TaskNodeType::Parallel; }
    if (s == "Decorator")   { return TaskNodeType::Decorator; }
    if (s == "Root")        { return TaskNodeType::Root; }

    outOk = false;
    return TaskNodeType::AtomicTask;
}

// ----------------------------------------------------------------------------

VariableType TaskGraphLoader::StringToVariableType(const std::string& s)
{
    if (s == "Bool")      { return VariableType::Bool; }
    if (s == "Int")       { return VariableType::Int; }
    if (s == "Float")     { return VariableType::Float; }
    if (s == "Vector")    { return VariableType::Vector; }
    if (s == "EntityID")  { return VariableType::EntityID; }
    if (s == "String")    { return VariableType::String; }
    if (s == "List")      { return VariableType::List; }
    if (s == "GlobalRef") { return VariableType::GlobalRef; }
    return VariableType::None;
}

// ----------------------------------------------------------------------------

DataPinDir TaskGraphLoader::StringToDataPinDir(const std::string& s)
{
    if (s == "Output") { return DataPinDir::Output; }
    return DataPinDir::Input;
}

// ----------------------------------------------------------------------------

ExecPinRole TaskGraphLoader::StringToExecPinRole(const std::string& s)
{
    if (s == "Out")          { return ExecPinRole::Out; }
    if (s == "OutElse")      { return ExecPinRole::OutElse; }
    if (s == "OutLoop")      { return ExecPinRole::OutLoop; }
    if (s == "OutCompleted") { return ExecPinRole::OutCompleted; }
    if (s == "OutCase")      { return ExecPinRole::OutCase; }
    return ExecPinRole::In;
}

} // namespace Olympe
