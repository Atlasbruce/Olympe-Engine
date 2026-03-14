/**
 * @file TaskGraphLoader.cpp
 * @brief Clean schema v4 parser for ATS Visual Script task graphs.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * Primary path: schema v4 flat ATS VisualScript JSON.
 * For schema v3: delegates to TaskGraphMigrator_v3_to_v4, then parses v4.
 * For schema v2/legacy: minimal BehaviorTree-style parsing for backward compat.
 *
 * C++14 compliant - no std::filesystem, no C++17/20 features.
 */

#include "TaskGraphLoader.h"
#include "TaskGraphMigrator_v3_to_v4.h"
#include "../BlueprintEditor/BTtoVSMigrator.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <algorithm>

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#else
#   include <dirent.h>
#endif

#include "../system/system_utils.h"
#include "../json_helper.h"

namespace Olympe {

// ============================================================================
// Public: LoadFromFile
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::LoadFromFile(const std::string& path,
                                                  std::vector<std::string>& outErrors)
{
    SYSTEM_LOG << "[TaskGraphLoader] Loading from file: " << path << std::endl;

    // Warn if the file does not carry the expected .ats extension.
    {
        const size_t dotPos = path.find_last_of('.');
        const bool hasAtsExt = (dotPos != std::string::npos)
                               && (path.substr(dotPos + 1) == "ats");
        if (!hasAtsExt)
        {
            SYSTEM_LOG << "[TaskGraphLoader] WARNING: Expected .ats extension for: "
                       << path << std::endl;
        }
    }

    json data;
    if (!JsonHelper::LoadJsonFromFile(path, data))
    {
        outErrors.push_back("Failed to open or parse JSON file: " + path);
        SYSTEM_LOG << "[TaskGraphLoader] ERROR: Failed to open or parse: " << path << std::endl;
        return nullptr;
    }

    return LoadFromJson(data, outErrors);
}

// ============================================================================
// Public: LoadFromJson
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::LoadFromJson(const json& data,
                                                  std::vector<std::string>& outErrors)
{
    const int schemaVersion = JsonHelper::GetInt(data, "schema_version", 2);
    SYSTEM_LOG << "[TaskGraphLoader] Schema version: " << schemaVersion << std::endl;

    TaskGraphTemplate* tmpl = nullptr;

    if (schemaVersion == 4)
    {
        // Primary path: parse v4 flat ATS VS format directly.
        tmpl = ParseSchemaV4(data, outErrors);
    }
    else if (schemaVersion == 3)
    {
        // Delegate v3 to migrator, then parse the resulting v4 JSON.
        // The migrator expects flat nodes with NextOnSuccess/NextOnFailure fields.
        // If the v3 JSON uses the older BT-style "data.nodes" structure instead,
        // the migrator will produce 0 nodes, and we fall back to direct legacy parsing.
        std::vector<std::string> migrateErrors;
        json v4data = TaskGraphMigrator_v3_to_v4::MigrateJson(data, migrateErrors);

        bool migratorProducedNodes = !v4data.empty()
            && v4data.contains("nodes")
            && v4data["nodes"].is_array()
            && !v4data["nodes"].empty();

        if (!migratorProducedNodes)
        {
            // Migrator produced no nodes — fall back to direct v4 parse.
            SYSTEM_LOG << "[TaskGraphLoader] v3 migrator produced no nodes; "
                          "using direct v4 parse\n";
            tmpl = ParseSchemaV4(data, outErrors);
        }
        else
        {
            tmpl = ParseSchemaV4(v4data, outErrors);
        }
    }
    else
    {
        // Schema v2: route through BTtoVSMigrator only for doubly-nested
        // BT v2 documents (data.data.nodes).  Singly-nested documents
        // (data.nodes) are handled by ParseSchemaV4, which already understands
        // both formats and preserves BT node types (Sequence, Decorator, etc.).
        bool doublyNested = data.contains("data") &&
                            data["data"].is_object() &&
                            data["data"].contains("data");

        if (BTtoVSMigrator::IsBTv2(data) && doublyNested)
        {
            std::vector<std::string> migrateErrors;
            TaskGraphTemplate migrated = BTtoVSMigrator::Convert(data, migrateErrors);
            for (const auto& e : migrateErrors)
                outErrors.push_back(e);

            tmpl = new TaskGraphTemplate(migrated);
        }
        else
        {
            // Flat, singly-nested, or unknown v2 format: use ParseSchemaV4
            // which already handles data.nodes and preserves BT structure.
            tmpl = ParseSchemaV4(data, outErrors);
        }
    }

    if (tmpl == nullptr)
    {
        return nullptr;
    }

    tmpl->BuildLookupCache();

    if (!tmpl->Validate())
    {
        outErrors.push_back("TaskGraphTemplate::Validate() failed for template '" + tmpl->Name + "'");
        delete tmpl;
        return nullptr;
    }

    SYSTEM_LOG << "[TaskGraphLoader] Loaded '" << tmpl->Name
               << "' (" << tmpl->Nodes.size() << " nodes)\n";
    return tmpl;
}

// ============================================================================
// Public: ValidateJson
// ============================================================================

bool TaskGraphLoader::ValidateJson(const json& data,
                                   std::vector<std::string>& outErrors)
{
    bool valid = true;

    // Check for nodes array: flat (v4) or nested (v2/v3).
    bool hasNodes = JsonHelper::IsArray(data, "nodes");
    bool hasDataSection = JsonHelper::IsObject(data, "data");

    if (!hasNodes && !hasDataSection)
    {
        outErrors.push_back("Missing required 'data' object or 'nodes' array in JSON");
        valid = false;
    }
    else if (!hasNodes && hasDataSection)
    {
        const json& dataSection = data["data"];
        if (!JsonHelper::IsArray(dataSection, "nodes"))
        {
            outErrors.push_back("Missing required 'nodes' array in data section");
            valid = false;
        }
        if (JsonHelper::GetInt(dataSection, "rootNodeId", -1) < 0 &&
            JsonHelper::GetInt(data, "rootNodeId", -1) < 0)
        {
            outErrors.push_back("Missing required 'rootNodeId' in data section");
            valid = false;
        }
    }

    return valid;
}

// ============================================================================
// v4 parsing: ParseSchemaV4
// ============================================================================

TaskGraphTemplate* TaskGraphLoader::ParseSchemaV4(const json& data,
                                                   std::vector<std::string>& outErrors)
{
    // Determine if JSON uses flat format (nodes at root) or nested (data.nodes).
    const bool flatFormat = JsonHelper::IsArray(data, "nodes");
    const json* rootForNodes = flatFormat ? &data : nullptr;

    // If not flat, try nested data section for backward compat with old assets.
    if (!flatFormat)
    {
        if (!JsonHelper::IsObject(data, "data"))
        {
            outErrors.push_back("[v4] Missing 'nodes' array or 'data' object in JSON");
            return nullptr;
        }
        const json& dataSection = data["data"];
        if (!JsonHelper::IsArray(dataSection, "nodes"))
        {
            outErrors.push_back("[v4] Missing 'nodes' array in data section");
            return nullptr;
        }
        rootForNodes = &dataSection;
    }

    TaskGraphTemplate* tmpl = new TaskGraphTemplate();

    // Read metadata — accept both camelCase and PascalCase field names.
    tmpl->Name = JsonHelper::GetString(data, "name", "Unnamed");

    // Infer the default graph type:
    //   - Explicit "blueprintType": "BehaviorTree" → BehaviorTree legacy asset.
    //   - Nested data.nodes format without an explicit graphType → treat as BehaviorTree.
    //   - Otherwise → VisualScript (the primary v4 format).
    {
        const std::string blueprintType = JsonHelper::GetString(data, "blueprintType", "");
        const std::string defaultGraphType =
            (blueprintType == "BehaviorTree" || (!flatFormat && blueprintType.empty()
             && !data.contains("graphType") && !data.contains("graph_type")))
            ? "BehaviorTree"
            : "VisualScript";
        tmpl->GraphType = JsonHelper::GetString(data, "graphType",
                          JsonHelper::GetString(data, "graph_type", defaultGraphType));
    }

    // Entry point ID (flat v4) or rootNodeId (legacy nested).
    if (flatFormat)
    {
        // For the flat v4 format, the first EntryPoint node implicitly is the root.
        // entryPointId is optional; if missing, VSGraphExecutor scans for EntryPoint node.
        tmpl->EntryPointID = JsonHelper::GetInt(data, "entryPointId", NODE_INDEX_NONE);
        tmpl->RootNodeID   = tmpl->EntryPointID;
    }
    else
    {
        const json& ds = data["data"];
        tmpl->EntryPointID = JsonHelper::GetInt(ds, "entryPointId", NODE_INDEX_NONE);
        tmpl->RootNodeID   = ResolveRootNodeId(data, ds);
        if (tmpl->RootNodeID == NODE_INDEX_NONE && tmpl->EntryPointID != NODE_INDEX_NONE)
        {
            tmpl->RootNodeID = tmpl->EntryPointID;
        }
    }

    // Parse nodes.
    JsonHelper::ForEachInArray(*rootForNodes, "nodes",
        [&](const json& nodeJson, size_t /*idx*/)
    {
        TaskNodeDefinition nd = ParseNodeV4(nodeJson, tmpl->GraphType, outErrors);
        tmpl->Nodes.push_back(nd);
        // First EntryPoint node sets the root if not already set.
        if (tmpl->RootNodeID == NODE_INDEX_NONE &&
            nd.Type == TaskNodeType::EntryPoint)
        {
            tmpl->RootNodeID   = nd.NodeID;
            tmpl->EntryPointID = nd.NodeID;
        }
    });

    // Parse blackboard.
    ParseBlackboardV4(data, tmpl, outErrors);

    // Parse exec connections (handles camelCase, PascalCase, and nested variants).
    ParseExecConnectionsV4(data, tmpl);

    // Parse data connections.
    ParseDataConnectionsV4(data, tmpl);

    // SubGraph metadata (Phase 3)
    tmpl->IsSubGraph = JsonHelper::GetBool(data, "isSubGraph", false);

    if (JsonHelper::IsArray(data, "inputParameters"))
    {
        JsonHelper::ForEachInArray(data, "inputParameters",
            [&](const json& paramJson, size_t /*idx*/)
        {
            SubGraphParameterDef param;
            param.Name = JsonHelper::GetString(paramJson, "name", "");
            param.Type = StringToVariableType(JsonHelper::GetString(paramJson, "type", "None"));
            if (!param.Name.empty()) tmpl->InputParameters.push_back(param);
        });
    }

    if (JsonHelper::IsArray(data, "outputParameters"))
    {
        JsonHelper::ForEachInArray(data, "outputParameters",
            [&](const json& paramJson, size_t /*idx*/)
        {
            SubGraphParameterDef param;
            param.Name = JsonHelper::GetString(paramJson, "name", "");
            param.Type = StringToVariableType(JsonHelper::GetString(paramJson, "type", "None"));
            if (!param.Name.empty()) tmpl->OutputParameters.push_back(param);
        });
    }

    return tmpl;
}

// ============================================================================
// v4 node parsing
// ============================================================================

TaskNodeDefinition TaskGraphLoader::ParseNodeV4(const json& nodeJson,
                                                 const std::string& graphType,
                                                 std::vector<std::string>& outErrors)
{
    TaskNodeDefinition nd;

    // Accept both new ("id") and legacy ("nodeID") field names.
    nd.NodeID = JsonHelper::GetInt(nodeJson, "id",
                JsonHelper::GetInt(nodeJson, "nodeID", -1));

    // Accept "label", "name", or "nodeName".
    nd.NodeName = JsonHelper::GetString(nodeJson, "label",
                  JsonHelper::GetString(nodeJson, "name",
                  JsonHelper::GetString(nodeJson, "nodeName", "")));

    // Node type: accept "type" (new) or "nodeType" (legacy).
    const std::string typeStr = JsonHelper::GetString(nodeJson, "type",
                                JsonHelper::GetString(nodeJson, "nodeType", ""));
    bool typeOk = true;
    nd.Type = StringToNodeType(typeStr, graphType, typeOk);
    if (!typeOk)
    {
        outErrors.push_back("Node " + std::to_string(nd.NodeID) +
                            " has unknown type '" + typeStr + "'");
    }

    // Name-based Decorator detection for BT-style assets.
    // Some BT v2 files encode Decorator nodes as type "Action" with a
    // descriptive name.  IsDecoratorName() centralises this heuristic.
    if (nd.Type == TaskNodeType::AtomicTask &&
        BTtoVSMigrator::IsDecoratorName(nd.NodeName))
    {
        nd.Type = TaskNodeType::Decorator;
    }

    // AtomicTask type name: "taskType" (new) or "AtomicTaskID"/"atomicTaskId"/"actionType"/"conditionType" (legacy).
    nd.AtomicTaskID = JsonHelper::GetString(nodeJson, "taskType",
                      JsonHelper::GetString(nodeJson, "AtomicTaskID",
                      JsonHelper::GetString(nodeJson, "atomicTaskId",
                      JsonHelper::GetString(nodeJson, "actionType",
                      JsonHelper::GetString(nodeJson, "conditionType", "")))));

    // VS-specific node fields (new flat format).
    nd.DelaySeconds = JsonHelper::GetFloat(nodeJson, "delaySeconds", 0.0f);
    nd.BBKey        = JsonHelper::GetString(nodeJson, "bbKey", "");
    nd.SubGraphPath = JsonHelper::GetString(nodeJson, "subGraphPath", "");
    nd.ConditionID  = JsonHelper::GetString(nodeJson, "conditionKey",
                      JsonHelper::GetString(nodeJson, "conditionId", ""));

    // Math operation fields.
    nd.MathOperator = JsonHelper::GetString(nodeJson, "mathOp", "");

    // Parse parameters: accept "params" (new) or "parameters" (legacy).
    if (JsonHelper::IsObject(nodeJson, "params"))
    {
        ParseParameters(nodeJson["params"], nd.Parameters);
    }
    else if (JsonHelper::IsObject(nodeJson, "parameters"))
    {
        ParseParameters(nodeJson["parameters"], nd.Parameters);

        // Legacy nested fields inside parameters block.
        const json& params = nodeJson["parameters"];
        if (nd.BBKey.empty())
        {
            nd.BBKey = JsonHelper::GetString(params, "key", "");
        }
        if (nd.MathOperator.empty())
        {
            nd.MathOperator = JsonHelper::GetString(params, "operator", "");
        }
        if (nd.DelaySeconds == 0.0f)
        {
            nd.DelaySeconds = JsonHelper::GetFloat(params, "delaySeconds", 0.0f);
        }
        if (nd.SubGraphPath.empty())
        {
            nd.SubGraphPath = JsonHelper::GetString(params, "subGraphPath", "");
        }
    }

    // Legacy DelaySeconds at node level (patrol.json uses "DelaySeconds").
    if (nd.DelaySeconds == 0.0f)
    {
        nd.DelaySeconds = JsonHelper::GetFloat(nodeJson, "DelaySeconds", 0.0f);
    }

    // Switch cases.
    if (JsonHelper::IsArray(nodeJson, "switch_cases"))
    {
        JsonHelper::ForEachInArray(nodeJson, "switch_cases",
            [&](const json& c, size_t /*i*/) {
                if (c.is_string()) nd.SwitchCases.push_back(c.get<std::string>());
            });
    }

    // Switch variable (Phase 22-A) — the BB key whose value is switched on.
    if (nd.Type == TaskNodeType::Switch)
    {
        nd.switchVariable = JsonHelper::GetString(nodeJson, "switchVariable", "");
    }

    // Structured switch cases (Phase 22-A).
    if (nd.Type == TaskNodeType::Switch && JsonHelper::IsArray(nodeJson, "switchCases"))
    {
        std::unordered_map<std::string, size_t> seenValues; // E011: duplicate value check
        JsonHelper::ForEachInArray(nodeJson, "switchCases",
            [&](const json& c, size_t idx) {
                if (!c.is_object())
                    return;
                SwitchCaseDefinition scd;
                scd.value       = JsonHelper::GetString(c, "value", "");
                scd.pinName     = JsonHelper::GetString(c, "pin", "");
                scd.customLabel = JsonHelper::GetString(c, "label", "");

                // E011 — duplicate case value
                if (!scd.value.empty())
                {
                    auto it = seenValues.find(scd.value);
                    if (it != seenValues.end())
                    {
                        std::string msg = "[E011] Switch node #"
                            + std::to_string(nd.NodeID)
                            + ": duplicate case value '" + scd.value
                            + "' at index " + std::to_string(idx)
                            + " (first seen at index "
                            + std::to_string(it->second) + ")";
                        outErrors.push_back(msg);
                        SYSTEM_LOG << "[TaskGraphLoader] " << msg << "\n";
                    }
                    else
                    {
                        seenValues[scd.value] = idx;
                    }
                }

                // E012 — pin name must match expected pattern (non-empty)
                if (scd.pinName.empty())
                {
                    std::string msg = "[E012] Switch node #"
                        + std::to_string(nd.NodeID)
                        + ": switchCases[" + std::to_string(idx)
                        + "] has empty pin name";
                    outErrors.push_back(msg);
                    SYSTEM_LOG << "[TaskGraphLoader] " << msg << "\n";
                }

                nd.switchCases.push_back(scd);
            });
    }

    // Dynamic exec-out pins (VSSequence and Switch, Phase 20-C / Phase 21-D).
    if ((nd.Type == TaskNodeType::VSSequence || nd.Type == TaskNodeType::Switch) &&
        nodeJson.contains("dynamicExecPins") &&
        nodeJson["dynamicExecPins"].is_array())
    {
        const json& dynPins = nodeJson["dynamicExecPins"];
        for (size_t p = 0; p < dynPins.size(); ++p)
        {
            if (dynPins[p].is_string())
                nd.DynamicExecOutputPins.push_back(dynPins[p].get<std::string>());
        }
    }

    // Backward-compat children array (BT-style, may appear in migrated v3).
    if (JsonHelper::IsArray(nodeJson, "children"))
    {
        const json& ch = nodeJson["children"];
        for (size_t i = 0; i < ch.size(); ++i)
        {
            if (ch[i].is_number()) nd.ChildrenIDs.push_back(ch[i].get<int>());
        }
    }

    // Compat flow fields.
    nd.NextOnSuccess = JsonHelper::GetInt(nodeJson, "nextOnSuccess", NODE_INDEX_NONE);
    nd.NextOnFailure = JsonHelper::GetInt(nodeJson, "nextOnFailure", NODE_INDEX_NONE);

    // SubGraph: parse InputParams (Phase 3)
    if (nd.Type == TaskNodeType::SubGraph && JsonHelper::IsObject(nodeJson, "InputParams"))
    {
        const json& inputParamsJson = nodeJson["InputParams"];
        for (auto it = inputParamsJson.begin(); it != inputParamsJson.end(); ++it)
        {
            const std::string& paramName = it.key();
            const json& bindingJson = it.value();

            ParameterBinding binding;
            std::string bindingType = JsonHelper::GetString(bindingJson, "Type", "Literal");

            if (bindingType == "Literal")
            {
                binding.Type = ParameterBindingType::Literal;
                if (bindingJson.contains("LiteralValue"))
                {
                    binding.LiteralValue = ParsePrimitiveValue(bindingJson["LiteralValue"]);
                }
            }
            else if (bindingType == "LocalVariable")
            {
                binding.Type = ParameterBindingType::LocalVariable;
                binding.VariableName = JsonHelper::GetString(bindingJson, "VariableName", "");
            }
            else if (bindingType == "AtomicTaskID")
            {
                binding.Type         = ParameterBindingType::AtomicTaskID;
                binding.VariableName = JsonHelper::GetString(bindingJson, "value",
                                       JsonHelper::GetString(bindingJson, "VariableName", ""));
            }
            else if (bindingType == "ConditionID")
            {
                binding.Type         = ParameterBindingType::ConditionID;
                binding.VariableName = JsonHelper::GetString(bindingJson, "value",
                                       JsonHelper::GetString(bindingJson, "VariableName", ""));
            }
            else if (bindingType == "MathOperator")
            {
                binding.Type         = ParameterBindingType::MathOperator;
                binding.VariableName = JsonHelper::GetString(bindingJson, "value",
                                       JsonHelper::GetString(bindingJson, "VariableName", ""));
            }
            else if (bindingType == "ComparisonOp")
            {
                binding.Type         = ParameterBindingType::ComparisonOp;
                binding.VariableName = JsonHelper::GetString(bindingJson, "value",
                                       JsonHelper::GetString(bindingJson, "VariableName", ""));
            }
            else if (bindingType == "SubGraphPath")
            {
                binding.Type         = ParameterBindingType::SubGraphPath;
                binding.VariableName = JsonHelper::GetString(bindingJson, "value",
                                       JsonHelper::GetString(bindingJson, "VariableName", ""));
            }

            nd.InputParams[paramName] = binding;
        }
    }

    // SubGraph: parse OutputParams (Phase 3)
    if (nd.Type == TaskNodeType::SubGraph && JsonHelper::IsObject(nodeJson, "OutputParams"))
    {
        const json& outputParamsJson = nodeJson["OutputParams"];
        for (auto it = outputParamsJson.begin(); it != outputParamsJson.end(); ++it)
        {
            const std::string& paramName = it.key();
            if (it.value().is_string())
            {
                nd.OutputParams[paramName] = it.value().get<std::string>();
            }
        }
    }

    // Editor position (schema v4 only — ignored at runtime).
    if (JsonHelper::IsObject(nodeJson, "position"))
    {
        nd.EditorPosX   = JsonHelper::GetFloat(nodeJson["position"], "x", 0.0f);
        nd.EditorPosY   = JsonHelper::GetFloat(nodeJson["position"], "y", 0.0f);
        nd.HasEditorPos = true;
    }

    return nd;
}

// ============================================================================
// v4 blackboard parsing
// ============================================================================

void TaskGraphLoader::ParseBlackboardV4(const json& root,
                                         TaskGraphTemplate* tmpl,
                                         std::vector<std::string>& /*outErrors*/)
{
    // Accept "blackboard" (new), "localBlackboard" (legacy), or "data.blackboard".
    const json* bbArray = nullptr;

    if (JsonHelper::IsArray(root, "blackboard"))
    {
        bbArray = &root["blackboard"];
    }
    else if (JsonHelper::IsArray(root, "localBlackboard"))
    {
        bbArray = &root["localBlackboard"];
    }
    else if (JsonHelper::IsObject(root, "data"))
    {
        const json& ds = root["data"];
        if (JsonHelper::IsArray(ds, "blackboard"))
        {
            bbArray = &ds["blackboard"];
        }
        else if (JsonHelper::IsArray(ds, "localBlackboard"))
        {
            bbArray = &ds["localBlackboard"];
        }
    }

    if (bbArray == nullptr) return;

    for (const json& entryJson : *bbArray)
    {
        BlackboardEntry entry;

        // Accept "key" (new) or "Key" (PascalCase legacy).
        entry.Key = JsonHelper::GetString(entryJson, "key",
                    JsonHelper::GetString(entryJson, "Key", ""));

        // Accept "type" (new) or "Type" (PascalCase legacy).
        const std::string typeStr = JsonHelper::GetString(entryJson, "type",
                                    JsonHelper::GetString(entryJson, "Type", "None"));
        entry.Type = StringToVariableType(typeStr);

        // Accept "value" (new) or "default"/"Default" (legacy) for initial value.
        json defaultVal;
        bool hasDefault = GetChildValue(entryJson, "value", defaultVal)
                       || GetChildValue(entryJson, "default", defaultVal)
                       || GetChildValue(entryJson, "Default", defaultVal);
        if (hasDefault)
        {
            entry.Default = ParsePrimitiveValue(defaultVal);
        }

        entry.IsGlobal = JsonHelper::GetBool(entryJson, "global",
                         JsonHelper::GetBool(entryJson, "IsGlobal", false));

        if (!entry.Key.empty())
        {
            tmpl->Blackboard.push_back(entry);
        }
    }
}

// ============================================================================
// v4 exec connections parsing
// ============================================================================

void TaskGraphLoader::ParseExecConnectionsV4(const json& root,
                                              TaskGraphTemplate* tmpl)
{
    const json* arr = nullptr;

    // Priority: camelCase new → PascalCase legacy → nested data section.
    if (JsonHelper::IsArray(root, "execConnections"))
    {
        arr = &root["execConnections"];
    }
    else if (JsonHelper::IsArray(root, "ExecConnections"))
    {
        arr = &root["ExecConnections"];
    }
    else if (JsonHelper::IsObject(root, "data"))
    {
        const json& ds = root["data"];
        if (JsonHelper::IsArray(ds, "exec_connections"))
        {
            arr = &ds["exec_connections"];
        }
        else if (JsonHelper::IsArray(ds, "ExecConnections"))
        {
            arr = &ds["ExecConnections"];
        }
    }

    if (arr == nullptr) return;

    for (const json& c : *arr)
    {
        ExecPinConnection conn;

        // New format: fromNode/fromPin/toNode.
        // Legacy format: SourceNodeID/SourcePinName/TargetNodeID/TargetPinName.
        conn.SourceNodeID  = JsonHelper::GetInt(c, "fromNode",
                             JsonHelper::GetInt(c, "SourceNodeID", NODE_INDEX_NONE));
        conn.SourcePinName = JsonHelper::GetString(c, "fromPin",
                             JsonHelper::GetString(c, "SourcePinName", ""));
        conn.TargetNodeID  = JsonHelper::GetInt(c, "toNode",
                             JsonHelper::GetInt(c, "TargetNodeID", NODE_INDEX_NONE));
        conn.TargetPinName = JsonHelper::GetString(c, "toPin",
                             JsonHelper::GetString(c, "TargetPinName", "In"));

        if (conn.SourceNodeID != NODE_INDEX_NONE && conn.TargetNodeID != NODE_INDEX_NONE)
        {
            tmpl->ExecConnections.push_back(conn);
        }
    }
}

// ============================================================================
// v4 data connections parsing
// ============================================================================

void TaskGraphLoader::ParseDataConnectionsV4(const json& root,
                                              TaskGraphTemplate* tmpl)
{
    const json* arr = nullptr;

    if (JsonHelper::IsArray(root, "dataConnections"))
    {
        arr = &root["dataConnections"];
    }
    else if (JsonHelper::IsArray(root, "DataConnections"))
    {
        arr = &root["DataConnections"];
    }
    else if (JsonHelper::IsObject(root, "data"))
    {
        const json& ds = root["data"];
        if (JsonHelper::IsArray(ds, "data_connections"))
        {
            arr = &ds["data_connections"];
        }
        else if (JsonHelper::IsArray(ds, "DataConnections"))
        {
            arr = &ds["DataConnections"];
        }
    }

    if (arr == nullptr) return;

    for (const json& c : *arr)
    {
        DataPinConnection conn;
        conn.SourceNodeID  = JsonHelper::GetInt(c, "fromNode",
                             JsonHelper::GetInt(c, "SourceNodeID", NODE_INDEX_NONE));
        conn.SourcePinName = JsonHelper::GetString(c, "fromPin",
                             JsonHelper::GetString(c, "SourcePinName", ""));
        conn.TargetNodeID  = JsonHelper::GetInt(c, "toNode",
                             JsonHelper::GetInt(c, "TargetNodeID", NODE_INDEX_NONE));
        conn.TargetPinName = JsonHelper::GetString(c, "toPin",
                             JsonHelper::GetString(c, "TargetPinName", ""));

        if (conn.SourceNodeID != NODE_INDEX_NONE && conn.TargetNodeID != NODE_INDEX_NONE)
        {
            tmpl->DataConnections.push_back(conn);
        }
    }
}

// ============================================================================
// Shared helpers
// ============================================================================

void TaskGraphLoader::ParseParameters(const json& paramsJson,
                                      std::unordered_map<std::string, ParameterBinding>& outParams)
{
    for (auto it = paramsJson.begin(); it != paramsJson.end(); ++it)
    {
        const std::string& key = it.key();
        const json& val        = it.value();

        ParameterBinding binding;

        if (val.is_object())
        {
            // Structured binding — check "bindingType" / "Type" field.
            std::string btype = JsonHelper::GetString(val, "bindingType",
                                JsonHelper::GetString(val, "Type", "Literal"));

            if (btype == "Variable" || btype == "LocalVariable")
            {
                binding.Type         = ParameterBindingType::LocalVariable;
                binding.VariableName = JsonHelper::GetString(val, "variableName",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else if (btype == "AtomicTaskID")
            {
                binding.Type         = ParameterBindingType::AtomicTaskID;
                binding.VariableName = JsonHelper::GetString(val, "value",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else if (btype == "ConditionID")
            {
                binding.Type         = ParameterBindingType::ConditionID;
                binding.VariableName = JsonHelper::GetString(val, "value",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else if (btype == "MathOperator")
            {
                binding.Type         = ParameterBindingType::MathOperator;
                binding.VariableName = JsonHelper::GetString(val, "value",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else if (btype == "ComparisonOp")
            {
                binding.Type         = ParameterBindingType::ComparisonOp;
                binding.VariableName = JsonHelper::GetString(val, "value",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else if (btype == "SubGraphPath")
            {
                binding.Type         = ParameterBindingType::SubGraphPath;
                binding.VariableName = JsonHelper::GetString(val, "value",
                                       JsonHelper::GetString(val, "VariableName", ""));
            }
            else
            {
                // Literal binding with nested value.
                binding.Type = ParameterBindingType::Literal;
                json vj;
                if (GetChildValue(val, "value", vj) ||
                    GetChildValue(val, "LiteralValue", vj))
                {
                    binding.LiteralValue = ParsePrimitiveValue(vj);
                }
            }
        }
        else
        {
            // Primitive value: Literal binding.
            binding.Type         = ParameterBindingType::Literal;
            binding.LiteralValue = ParsePrimitiveValue(val);
        }

        outParams[key] = binding;
    }
}

// ============================================================================

TaskValue TaskGraphLoader::ParsePrimitiveValue(const json& val)
{
    if (val.is_boolean())        return TaskValue(val.get<bool>());
    if (val.is_number_integer()) return TaskValue(val.get<int>());
    if (val.is_number_float())   return TaskValue(static_cast<float>(val.get<double>()));
    if (val.is_string())         return TaskValue(val.get<std::string>());
    return TaskValue(); // None
}

// ============================================================================

bool TaskGraphLoader::GetChildValue(const json& obj,
                                    const std::string& key,
                                    json& outVal)
{
    if (obj.contains(key))
    {
        outVal = obj[key];
        return true;
    }
    return false;
}

// ============================================================================

int TaskGraphLoader::ResolveRootNodeId(const json& data, const json& dataSection)
{
    int top = JsonHelper::GetInt(data, "rootNodeId", -1);
    if (top >= 0) return top;
    return JsonHelper::GetInt(dataSection, "rootNodeId", -1);
}

// ============================================================================
// String → enum helpers
// ============================================================================

TaskNodeType TaskGraphLoader::StringToNodeType(const std::string& s,
                                               const std::string& graphType,
                                               bool& outOk)
{
    outOk = true;

    if (s == "EntryPoint")  return TaskNodeType::EntryPoint;
    if (s == "Branch")      return TaskNodeType::Branch;
    if (s == "Switch")      return TaskNodeType::Switch;
    if (s == "While")       return TaskNodeType::While;
    if (s == "ForEach")     return TaskNodeType::ForEach;
    if (s == "DoOnce")      return TaskNodeType::DoOnce;
    if (s == "Delay")       return TaskNodeType::Delay;
    if (s == "GetBBValue")  return TaskNodeType::GetBBValue;
    if (s == "SetBBValue")  return TaskNodeType::SetBBValue;
    if (s == "Math" || s == "MathOp") return TaskNodeType::MathOp;
    if (s == "SubGraph")    return TaskNodeType::SubGraph;
    if (s == "AtomicTask")  return TaskNodeType::AtomicTask;
    if (s == "Action")      return TaskNodeType::AtomicTask;
    if (s == "Condition")   return TaskNodeType::AtomicTask;

    if (s == "Sequence")
    {
        return (graphType == "VisualScript")
               ? TaskNodeType::VSSequence
               : TaskNodeType::Sequence;
    }
    if (s == "VSSequence")  return TaskNodeType::VSSequence;
    if (s == "Selector")    return TaskNodeType::Selector;
    if (s == "Parallel")    return TaskNodeType::Parallel;
    if (s == "Decorator")   return TaskNodeType::Decorator;
    if (s == "Repeater")    return TaskNodeType::Decorator;
    if (s == "Root")        return TaskNodeType::Root;

    outOk = false;
    return TaskNodeType::AtomicTask;
}

// ============================================================================

VariableType TaskGraphLoader::StringToVariableType(const std::string& s)
{
    if (s == "Bool")      return VariableType::Bool;
    if (s == "Int")       return VariableType::Int;
    if (s == "Float")     return VariableType::Float;
    if (s == "Vector")    return VariableType::Vector;
    if (s == "EntityID")  return VariableType::EntityID;
    if (s == "String")    return VariableType::String;
    if (s == "List")      return VariableType::List;
    if (s == "GlobalRef") return VariableType::GlobalRef;
    return VariableType::None;
}

// ============================================================================

DataPinDir TaskGraphLoader::StringToDataPinDir(const std::string& s)
{
    if (s == "Output") return DataPinDir::Output;
    return DataPinDir::Input;
}

// ============================================================================

ExecPinRole TaskGraphLoader::StringToExecPinRole(const std::string& s)
{
    if (s == "Out")          return ExecPinRole::Out;
    if (s == "OutElse")      return ExecPinRole::OutElse;
    if (s == "OutLoop")      return ExecPinRole::OutLoop;
    if (s == "OutCompleted") return ExecPinRole::OutCompleted;
    if (s == "OutCase")      return ExecPinRole::OutCase;
    return ExecPinRole::In;
}

// ============================================================================
// Public: FileExists
// ============================================================================

bool TaskGraphLoader::FileExists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

// ============================================================================
// Public: ScanTaskGraphDirectory
// ============================================================================

std::vector<std::string> TaskGraphLoader::ScanTaskGraphDirectory(const std::string& dir)
{
    std::vector<std::string> result;

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = dir + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        SYSTEM_LOG << "[TaskGraphLoader] WARNING: Directory not found: " << dir << std::endl;
        return result;
    }

    do
    {
        std::string name = findData.cFileName;
        if (name == "." || name == "..") continue;

        std::string fullPath = dir + "/" + name;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            std::vector<std::string> sub = ScanTaskGraphDirectory(fullPath);
            result.insert(result.end(), sub.begin(), sub.end());
        }
        else if (name.size() > 4 && name.substr(name.size() - 4) == ".ats")
        {
            result.push_back(fullPath);
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

#else
    DIR* d = opendir(dir.c_str());
    if (!d)
    {
        SYSTEM_LOG << "[TaskGraphLoader] WARNING: Directory not found: " << dir << std::endl;
        return result;
    }

    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;

        std::string fullPath = dir + "/" + name;
        if (entry->d_type == DT_DIR)
        {
            std::vector<std::string> sub = ScanTaskGraphDirectory(fullPath);
            result.insert(result.end(), sub.begin(), sub.end());
        }
        else if (name.size() > 4 && name.substr(name.size() - 4) == ".ats")
        {
            result.push_back(fullPath);
        }
    }

    closedir(d);
#endif

    std::sort(result.begin(), result.end());
    return result;
}

} // namespace Olympe
