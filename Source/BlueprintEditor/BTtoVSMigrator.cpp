/**
 * @file BTtoVSMigrator.cpp
 * @brief Implementation of the BT v2 -> VS v4 migrator (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BTtoVSMigrator.h"

#include <sstream>
#include <algorithm>

namespace Olympe {

// ============================================================================
// IsBTv2
// ============================================================================

bool BTtoVSMigrator::IsBTv2(const nlohmann::json& j)
{
    if (!j.is_object()) return false;

    // Must be blueprintType == "BehaviorTree"
    if (!j.contains("blueprintType")) return false;
    if (!j["blueprintType"].is_string()) return false;
    if (j["blueprintType"].get<std::string>() != "BehaviorTree") return false;

    // schema_version must be absent, 0, 1, or 2
    if (j.contains("schema_version") && j["schema_version"].is_number())
    {
        int ver = j["schema_version"].get<int>();
        if (ver > 2) return false;
    }
    return true;
}

// ============================================================================
// IsDecoratorName
// ============================================================================

bool BTtoVSMigrator::IsDecoratorName(const std::string& nodeName)
{
    // Common BT v2 Decorator-style node name prefixes
    if (nodeName == "Repeater"  || nodeName.substr(0, 6) == "Repeat") return true;
    if (nodeName == "Inverter"  || nodeName.substr(0, 6) == "Invert") return true;
    if (nodeName == "Cooldown"  || nodeName.substr(0, 8) == "Cooldown") return true;
    if (nodeName == "Timeout"   || nodeName.substr(0, 7) == "Timeout") return true;
    return false;
}

// ============================================================================
// MapNodeType
// ============================================================================

TaskNodeType BTtoVSMigrator::MapNodeType(const std::string& btType)
{
    if (btType == "Selector")  return TaskNodeType::Branch;
    if (btType == "Sequence")  return TaskNodeType::Sequence;
    if (btType == "Action")    return TaskNodeType::AtomicTask;
    if (btType == "Condition") return TaskNodeType::Branch;
    if (btType == "Decorator") return TaskNodeType::Decorator;
    if (btType == "Root")      return TaskNodeType::EntryPoint;
    if (btType == "Start")     return TaskNodeType::EntryPoint;
    // Default: treat unknown types as AtomicTask
    return TaskNodeType::AtomicTask;
}

// ============================================================================
// ConvertNode
// ============================================================================

TaskNodeDefinition BTtoVSMigrator::ConvertNode(const nlohmann::json& btNode,
                                                std::vector<std::string>& outErrors)
{
    TaskNodeDefinition def;

    // id
    if (btNode.contains("id") && btNode["id"].is_number())
        def.NodeID = btNode["id"].get<int32_t>();
    else
    {
        outErrors.push_back("BTtoVSMigrator: node missing 'id' field");
        def.NodeID = NODE_INDEX_NONE;
    }

    // name
    if (btNode.contains("name") && btNode["name"].is_string())
        def.NodeName = btNode["name"].get<std::string>();
    else
        def.NodeName = "UnnamedNode";

    // type
    std::string btType;
    if (btNode.contains("type") && btNode["type"].is_string())
        btType = btNode["type"].get<std::string>();

    def.Type = MapNodeType(btType);

    // Name-based heuristics for special node types:
    // BT v2 sometimes encodes Decorator-style nodes as "Action" with a descriptive name.
    if (def.Type == TaskNodeType::AtomicTask && IsDecoratorName(def.NodeName))
    {
        def.Type = TaskNodeType::Decorator;
    }

    // Populate ChildrenIDs from the "children" array
    if (btNode.contains("children") && btNode["children"].is_array())
    {
        for (const auto& childVal : btNode["children"])
        {
            int32_t childID = NODE_INDEX_NONE;
            if (childVal.is_number())
                childID = childVal.get<int32_t>();
            else if (childVal.is_object() && childVal.contains("id"))
                childID = childVal["id"].get<int32_t>();
            if (childID != NODE_INDEX_NONE)
                def.ChildrenIDs.push_back(childID);
        }
    }

    // For Action nodes: determine AtomicTaskID from actionType > parameters.taskType > node name
    if (def.Type == TaskNodeType::AtomicTask || def.Type == TaskNodeType::Decorator)
    {
        // 1. Top-level "actionType" field (common in newer BT v2 files)
        if (btNode.contains("actionType") && btNode["actionType"].is_string())
        {
            std::string at = btNode["actionType"].get<std::string>();
            if (!at.empty())
                def.AtomicTaskID = at;
        }

        // 2. Fallback: "taskType" inside parameters
        if (def.AtomicTaskID.empty() &&
            btNode.contains("parameters") && btNode["parameters"].is_object())
        {
            const auto& params = btNode["parameters"];
            if (params.contains("taskType") && params["taskType"].is_string())
                def.AtomicTaskID = params["taskType"].get<std::string>();
        }

        // 3. Final fallback: use node name as AtomicTaskID
        if (def.AtomicTaskID.empty() && !def.NodeName.empty())
            def.AtomicTaskID = def.NodeName;
    }

    // Copy all string parameters as literal bindings (for all node types)
    if (btNode.contains("parameters") && btNode["parameters"].is_object())
    {
        const auto& params = btNode["parameters"];
        for (auto it = params.begin(); it != params.end(); ++it)
        {
            if (it.value().is_string())
            {
                ParameterBinding pb;
                pb.Type = ParameterBindingType::Literal;
                pb.LiteralValue = TaskValue(it.value().get<std::string>());
                def.Parameters[it.key()] = pb;
            }
        }
    }

    // Store position as editor params
    if (btNode.contains("position") && btNode["position"].is_object())
    {
        const auto& pos = btNode["position"];
        float px = pos.contains("x") && pos["x"].is_number() ? pos["x"].get<float>() : 0.0f;
        float py = pos.contains("y") && pos["y"].is_number() ? pos["y"].get<float>() : 0.0f;

        ParameterBinding bx, by;
        bx.Type = ParameterBindingType::Literal;
        bx.LiteralValue = TaskValue(px);
        by.Type = ParameterBindingType::Literal;
        by.LiteralValue = TaskValue(py);
        def.Parameters["__posX"] = bx;
        def.Parameters["__posY"] = by;
    }

    return def;
}

// ============================================================================
// ConvertConnections
// ============================================================================

std::vector<ExecPinConnection> BTtoVSMigrator::ConvertConnections(
    const nlohmann::json& nodesArray,
    std::vector<std::string>& /*outErrors*/)
{
    std::vector<ExecPinConnection> conns;
    if (!nodesArray.is_array()) return conns;

    for (const auto& btNode : nodesArray)
    {
        if (!btNode.is_object()) continue;

        int32_t srcID = NODE_INDEX_NONE;
        if (btNode.contains("id") && btNode["id"].is_number())
            srcID = btNode["id"].get<int32_t>();
        if (srcID == NODE_INDEX_NONE) continue;

        if (!btNode.contains("children") || !btNode["children"].is_array()) continue;

        for (const auto& childVal : btNode["children"])
        {
            int32_t childID = NODE_INDEX_NONE;
            if (childVal.is_number())
                childID = childVal.get<int32_t>();
            else if (childVal.is_object() && childVal.contains("id"))
                childID = childVal["id"].get<int32_t>();

            if (childID == NODE_INDEX_NONE) continue;

            ExecPinConnection ec;
            ec.SourceNodeID  = srcID;
            ec.SourcePinName = "Out";
            ec.TargetNodeID  = childID;
            ec.TargetPinName = "In";
            conns.push_back(ec);
        }
    }

    return conns;
}

// ============================================================================
// ConvertBlackboard
// ============================================================================

void BTtoVSMigrator::ConvertBlackboard(const nlohmann::json& btV2Json,
                                        TaskGraphTemplate&    vsGraph,
                                        std::vector<std::string>& /*outErrors*/)
{
    // Try multiple known locations for blackboard data in BT v2 files
    const nlohmann::json* bbArray = nullptr;
    nlohmann::json dummy;

    if (btV2Json.contains("blackboard") && btV2Json["blackboard"].is_array())
        bbArray = &btV2Json["blackboard"];
    else if (btV2Json.contains("data") && btV2Json["data"].is_object())
    {
        const auto& d1 = btV2Json["data"];
        if (d1.contains("blackboard") && d1["blackboard"].is_array())
            bbArray = &d1["blackboard"];
        else if (d1.contains("data") && d1["data"].is_object())
        {
            const auto& d2 = d1["data"];
            if (d2.contains("blackboard") && d2["blackboard"].is_array())
                bbArray = &d2["blackboard"];
        }
    }

    if (!bbArray) return;

    for (const auto& entry : *bbArray)
    {
        if (!entry.is_object()) continue;

        BlackboardEntry be;
        if (entry.contains("key") && entry["key"].is_string())
            be.Key = entry["key"].get<std::string>();
        else continue; // key is mandatory

        if (entry.contains("type") && entry["type"].is_string())
        {
            std::string t = entry["type"].get<std::string>();
            if      (t == "Bool")   be.Type = VariableType::Bool;
            else if (t == "Int")    be.Type = VariableType::Int;
            else if (t == "Float")  be.Type = VariableType::Float;
            else if (t == "String") be.Type = VariableType::String;
            else                    be.Type = VariableType::None;
        }

        vsGraph.Blackboard.push_back(be);
    }
}

// ============================================================================
// Convert (public)
// ============================================================================

TaskGraphTemplate BTtoVSMigrator::Convert(const nlohmann::json& btV2Json,
                                           std::vector<std::string>& outErrors)
{
    TaskGraphTemplate result;
    result.GraphType = "VisualScript";

    // Extract graph name
    if (btV2Json.contains("name") && btV2Json["name"].is_string())
        result.Name = btV2Json["name"].get<std::string>();
    else
        result.Name = "MigratedGraph";

    // Navigate nested structure to find node array
    const nlohmann::json* nodesPtr = nullptr;
    int32_t rootNodeID = NODE_INDEX_NONE;

    // Try flat structure first (data.nodes or nodes)
    const nlohmann::json* data1 = nullptr;
    const nlohmann::json* data2 = nullptr;

    if (btV2Json.contains("nodes") && btV2Json["nodes"].is_array())
    {
        nodesPtr = &btV2Json["nodes"];
        if (btV2Json.contains("rootNodeId") && btV2Json["rootNodeId"].is_number())
            rootNodeID = btV2Json["rootNodeId"].get<int32_t>();
    }
    else if (btV2Json.contains("data") && btV2Json["data"].is_object())
    {
        data1 = &btV2Json["data"];
        if (data1->contains("nodes") && (*data1)["nodes"].is_array())
        {
            nodesPtr = &(*data1)["nodes"];
            if (data1->contains("rootNodeId") && (*data1)["rootNodeId"].is_number())
                rootNodeID = (*data1)["rootNodeId"].get<int32_t>();
        }
        else if (data1->contains("data") && (*data1)["data"].is_object())
        {
            data2 = &(*data1)["data"];
            if (data2->contains("nodes") && (*data2)["nodes"].is_array())
            {
                nodesPtr = &(*data2)["nodes"];
                if (data2->contains("rootNodeId") && (*data2)["rootNodeId"].is_number())
                    rootNodeID = (*data2)["rootNodeId"].get<int32_t>();
            }
        }
    }

    if (!nodesPtr)
    {
        outErrors.push_back("BTtoVSMigrator: could not find 'nodes' array in BT v2 JSON");
        return result;
    }

    // Convert nodes
    for (const auto& btNode : *nodesPtr)
    {
        TaskNodeDefinition def = ConvertNode(btNode, outErrors);
        if (def.NodeID == NODE_INDEX_NONE) continue;

        // Detect entry point by matching rootNodeID or by name/type heuristic
        std::string btType;
        if (btNode.contains("type") && btNode["type"].is_string())
            btType = btNode["type"].get<std::string>();

        bool isRoot = (def.NodeID == rootNodeID) ||
                      (btType == "Root") ||
                      (btType == "Start" && result.EntryPointID == NODE_INDEX_NONE);

        if (isRoot)
        {
            def.Type = TaskNodeType::EntryPoint;
            result.EntryPointID = def.NodeID;
            result.RootNodeID   = def.NodeID;
        }

        result.Nodes.push_back(def);
    }

    // If no entry point was identified, promote the first node
    if (result.EntryPointID == NODE_INDEX_NONE && !result.Nodes.empty())
    {
        result.Nodes[0].Type = TaskNodeType::EntryPoint;
        result.EntryPointID  = result.Nodes[0].NodeID;
        result.RootNodeID    = result.Nodes[0].NodeID;
    }

    // Convert connections
    result.ExecConnections = ConvertConnections(*nodesPtr, outErrors);

    // Convert blackboard
    ConvertBlackboard(btV2Json, result, outErrors);

    // Build lookup cache
    result.BuildLookupCache();

    return result;
}

} // namespace Olympe
