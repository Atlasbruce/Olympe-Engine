/**
 * @file GraphMigrator.cpp
 * @brief Implementation of GraphMigrator
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "GraphMigrator.h"
#include "../system/system_utils.h"

using json = nlohmann::json;

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Public API
// ============================================================================

GraphDocument GraphMigrator::LoadWithMigration(const json& j)
{
    int version = DetectSchemaVersion(j);
    
    if (version == 0)
    {
        // Legacy BT format
        SYSTEM_LOG << "[GraphMigrator] Detected legacy BT format, migrating to v2..." << std::endl;
        json v2 = MigrateLegacyBTToV2(j);
        return GraphDocument::FromJson(v2);
    }
    else if (version == 1)
    {
        // v1 Blueprint format
        SYSTEM_LOG << "[GraphMigrator] Detected v1 Blueprint format, migrating to v2..." << std::endl;
        json v2 = MigrateV1BlueprintToV2(j);
        return GraphDocument::FromJson(v2);
    }
    else
    {
        // v2 unified format
        SYSTEM_LOG << "[GraphMigrator] Using v2 format (no migration needed)" << std::endl;
        return GraphDocument::FromJson(j);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

int GraphMigrator::DetectSchemaVersion(const json& j)
{
    // Check for explicit schema_version or schemaVersion
    int schemaVersion = JsonHelper::GetInt(j, "schema_version", -1);
    if (schemaVersion == -1)
    {
        schemaVersion = JsonHelper::GetInt(j, "schemaVersion", -1);
    }
    
    // If found and is 2, it's v2
    if (schemaVersion == 2)
    {
        return 2;
    }
    
    // If found and is 1, it's v1
    if (schemaVersion == 1)
    {
        return 1;
    }
    
    // Check for v1 Blueprint format (has "blueprint_version")
    if (j.contains("blueprint_version"))
    {
        return 1;
    }
    
    // Check for v2 format markers
    if (j.contains("graphKind") && j.contains("data"))
    {
        return 2;
    }
    
    // Otherwise assume legacy BT format
    return 0;
}

json GraphMigrator::MigrateLegacyBTToV2(const json& v1)
{
    json v2 = json::object();
    
    v2["schemaVersion"] = 2;
    v2["type"] = "AIGraph";
    v2["graphKind"] = "BehaviorTree";
    
    // Metadata
    json metadata = json::object();
    metadata["author"] = JsonHelper::GetString(v1, "author", "Unknown");
    metadata["created"] = JsonHelper::GetString(v1, "created", "");
    
    json tags = json::array();
    tags.push_back("AI");
    tags.push_back("BehaviorTree");
    metadata["tags"] = tags;
    
    v2["metadata"] = metadata;
    
    // Editor state
    json editorState = json::object();
    editorState["zoom"] = JsonHelper::GetFloat(v1, "zoom", 1.0f);
    
    json scrollOffset = json::object();
    if (v1.contains("editorState") && v1["editorState"].is_object())
    {
        const json& es = v1["editorState"];
        if (es.contains("scrollOffset") && es["scrollOffset"].is_object())
        {
            scrollOffset["x"] = JsonHelper::GetFloat(es["scrollOffset"], "x", 0.0f);
            scrollOffset["y"] = JsonHelper::GetFloat(es["scrollOffset"], "y", 0.0f);
        }
        else
        {
            scrollOffset["x"] = 0.0f;
            scrollOffset["y"] = 0.0f;
        }
    }
    else
    {
        scrollOffset["x"] = 0.0f;
        scrollOffset["y"] = 0.0f;
    }
    editorState["scrollOffset"] = scrollOffset;
    
    json selectedNodes = json::array();
    editorState["selectedNodes"] = selectedNodes;
    editorState["layoutDirection"] = "TopToBottom";
    
    v2["editorState"] = editorState;
    
    // Data section
    json data = json::object();
    data["rootNodeId"] = JsonHelper::GetInt(v1, "rootNodeId", 0);
    
    // Nodes
    json nodesArray = json::array();
    if (v1.contains("nodes") && v1["nodes"].is_array())
    {
        const json& oldNodes = v1["nodes"];
        for (size_t i = 0; i < oldNodes.size(); ++i)
        {
            const json& oldNode = oldNodes[i];
            
            json newNode = json::object();
            newNode["id"] = JsonHelper::GetInt(oldNode, "id", 0);
            newNode["type"] = JsonHelper::GetString(oldNode, "type", "");
            newNode["name"] = JsonHelper::GetString(oldNode, "name", "");
            
            json position = json::object();
            if (oldNode.contains("position") && oldNode["position"].is_object())
            {
                position["x"] = JsonHelper::GetFloat(oldNode["position"], "x", 0.0f);
                position["y"] = JsonHelper::GetFloat(oldNode["position"], "y", 0.0f);
            }
            else
            {
                position["x"] = 0.0f;
                position["y"] = 0.0f;
            }
            newNode["position"] = position;
            
            // Children - handle both "children" and "childIds"
            json children = json::array();
            if (oldNode.contains("children") && oldNode["children"].is_array())
            {
                const json& childrenArr = oldNode["children"];
                for (size_t c = 0; c < childrenArr.size(); ++c)
                {
                    children.push_back(childrenArr[c]);
                }
            }
            else if (oldNode.contains("childIds") && oldNode["childIds"].is_array())
            {
                const json& childrenArr = oldNode["childIds"];
                for (size_t c = 0; c < childrenArr.size(); ++c)
                {
                    children.push_back(childrenArr[c]);
                }
            }
            newNode["children"] = children;
            
            // Parameters
            json parameters = json::object();
            if (oldNode.contains("parameters") && oldNode["parameters"].is_object())
            {
                const json& params = oldNode["parameters"];
                for (auto it = params.begin(); it != params.end(); ++it)
                {
                    parameters[it.key()] = it.value();
                }
            }
            newNode["parameters"] = parameters;
            
            // Decorator child
            int decoratorChild = JsonHelper::GetInt(oldNode, "decoratorChildId", 0);
            if (decoratorChild != 0)
            {
                newNode["decoratorChildId"] = decoratorChild;
            }
            
            nodesArray.push_back(newNode);
        }
    }
    data["nodes"] = nodesArray;
    
    // Links
    json linksArray = json::array();
    if (v1.contains("links") && v1["links"].is_array())
    {
        const json& oldLinks = v1["links"];
        for (size_t i = 0; i < oldLinks.size(); ++i)
        {
            const json& oldLink = oldLinks[i];
            
            json newLink = json::object();
            newLink["id"] = JsonHelper::GetInt(oldLink, "id", 0);
            
            json fromPin = json::object();
            if (oldLink.contains("fromPin") && oldLink["fromPin"].is_object())
            {
                fromPin["nodeId"] = JsonHelper::GetInt(oldLink["fromPin"], "nodeId", 0);
                fromPin["pinId"] = JsonHelper::GetString(oldLink["fromPin"], "pinId", "output");
            }
            newLink["fromPin"] = fromPin;
            
            json toPin = json::object();
            if (oldLink.contains("toPin") && oldLink["toPin"].is_object())
            {
                toPin["nodeId"] = JsonHelper::GetInt(oldLink["toPin"], "nodeId", 0);
                toPin["pinId"] = JsonHelper::GetString(oldLink["toPin"], "pinId", "input");
            }
            newLink["toPin"] = toPin;
            
            linksArray.push_back(newLink);
        }
    }
    data["links"] = linksArray;
    
    v2["data"] = data;
    
    return v2;
}

json GraphMigrator::MigrateV1BlueprintToV2(const json& v1)
{
    // For now, just wrap it in v2 format
    // A full blueprint migration would need more complex logic
    
    json v2 = json::object();
    
    v2["schemaVersion"] = 2;
    v2["type"] = JsonHelper::GetString(v1, "type", "Blueprint");
    v2["graphKind"] = "Blueprint";
    
    // Copy metadata if exists
    if (v1.contains("metadata") && v1["metadata"].is_object())
    {
        v2["metadata"] = v1["metadata"];
    }
    else
    {
        json metadata = json::object();
        metadata["author"] = "Unknown";
        v2["metadata"] = metadata;
    }
    
    // Editor state
    json editorState = json::object();
    editorState["zoom"] = 1.0f;
    
    json scrollOffset = json::object();
    scrollOffset["x"] = 0.0f;
    scrollOffset["y"] = 0.0f;
    editorState["scrollOffset"] = scrollOffset;
    
    json selectedNodes = json::array();
    editorState["selectedNodes"] = selectedNodes;
    editorState["layoutDirection"] = "TopToBottom";
    
    v2["editorState"] = editorState;
    
    // Data - try to preserve original structure
    if (v1.contains("data") && v1["data"].is_object())
    {
        v2["data"] = v1["data"];
    }
    else
    {
        json data = json::object();
        data["rootNodeId"] = 0;
        data["nodes"] = json::array();
        data["links"] = json::array();
        v2["data"] = data;
    }
    
    return v2;
}

} // namespace NodeGraph
} // namespace Olympe
