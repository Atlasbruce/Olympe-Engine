/*
 * Olympe Blueprint Editor - Blueprint Migrator Implementation
 */

#include "BlueprintMigrator.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <tuple>

using json = nlohmann::json;

namespace Olympe
{
    BlueprintMigrator::BlueprintMigrator()
    {
    }
    
    BlueprintMigrator::~BlueprintMigrator()
    {
    }
    
    bool BlueprintMigrator::IsV2(const json& blueprint) const
    {
        return blueprint.contains("schema_version") && 
               blueprint["schema_version"].get<int>() == 2;
    }
    
    std::string BlueprintMigrator::GetCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm timeInfo;
        #ifdef _WIN32
        localtime_s(&timeInfo, &time);
        #else
        localtime_r(&time, &timeInfo);
        #endif
        std::stringstream ss;
        ss << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }
    
    std::string BlueprintMigrator::DetectBlueprintType(const json& blueprint)
    {
        // Detect based on structure
        if (blueprint.contains("components"))
        {
            return "EntityPrefab";
        }
        
        if (blueprint.contains("rootNodeId") && blueprint.contains("nodes"))
        {
            // Check if it's HFSM (has states) or BehaviorTree
            if (blueprint.contains("states"))
            {
                return "HFSM";
            }
            return "BehaviorTree";
        }
        
        return "Unknown";
    }
    
    json BlueprintMigrator::MigrateToV2(const json& v1Blueprint)
    {
        // Check if already v2
        if (IsV2(v1Blueprint))
        {
            return v1Blueprint;
        }
        
        json v2;
        
        // Detect type
        std::string detectedType = DetectBlueprintType(v1Blueprint);
        
        // Base structure
        v2["schema_version"] = 2;
        v2["blueprintType"] = detectedType;
        v2["name"] = v1Blueprint.value<std::string>("name", "Unnamed");
        v2["description"] = "";
        
        // Metadata
        v2["metadata"]["author"] = "Atlasbruce";
        v2["metadata"]["created"] = GetCurrentTimestamp();
        v2["metadata"]["lastModified"] = GetCurrentTimestamp();
        v2["metadata"]["tags"] = json::array();
        
        // Editor state
        v2["editorState"]["zoom"] = 1.0;
        v2["editorState"]["scrollOffset"] = json::object();
        v2["editorState"]["scrollOffset"]["x"] = 0;
        v2["editorState"]["scrollOffset"]["y"] = 0;
        
        // Data section
        v2["data"] = json::object();
        
        // Type-specific migration
        if (detectedType == "BehaviorTree")
        {
            MigrateBehaviorTree(v1Blueprint, v2["data"]);
        }
        else if (detectedType == "HFSM")
        {
            MigrateHFSM(v1Blueprint, v2["data"]);
        }
        else if (detectedType == "EntityPrefab")
        {
            MigrateEntityPrefab(v1Blueprint, v2["data"]);
        }
        
        return v2;
    }
    
    void BlueprintMigrator::MigrateBehaviorTree(const json& v1, json& v2Data)
    {
        v2Data["rootNodeId"] = v1.value<int>("rootNodeId", 1);
        v2Data["nodes"] = json::array();
        
        if (!v1.contains("nodes") || !v1["nodes"].is_array())
        {
            return;
        }
        
        // Build children map for layout calculation
        std::map<int, std::vector<int>> childrenMap;
        for (size_t i = 0; i < v1["nodes"].size(); ++i)
        {
            const json& node = v1["nodes"][i];
            int id = node.value<int>("id", 0);
            if (node.contains("children") && node["children"].is_array())
            {
                std::vector<int> children;
                for (size_t j = 0; j < node["children"].size(); ++j)
                {
                    if (node["children"][j].is_number())
                    {
                        children.push_back(node["children"][j].get<int>());
                    }
                }
                childrenMap[id] = children;
            }
        }
        
        // Calculate positions
        int rootId = v1.value<int>("rootNodeId", 1);
        std::map<int, NodeLayout> layouts = CalculateHierarchicalLayout(v1["nodes"], childrenMap, rootId);
        
        // Migrate each node
        for (size_t i = 0; i < v1["nodes"].size(); ++i)
        {
            const json& v1Node = v1["nodes"][i];
            json v2Node;
            int nodeId = v1Node.value<int>("id", 0);
            
            v2Node["id"] = nodeId;
            v2Node["name"] = v1Node.value<std::string>("name", "Unnamed");
            v2Node["type"] = v1Node.value<std::string>("type", "Unknown");
            
            // Position from calculated layout
            if (layouts.count(nodeId))
            {
                v2Node["position"]["x"] = layouts[nodeId].posX;
                v2Node["position"]["y"] = layouts[nodeId].posY;
            }
            else
            {
                // Fallback position
                v2Node["position"]["x"] = 400.0;
                v2Node["position"]["y"] = 300.0;
            }
            
            // Copy children array if present
            if (v1Node.contains("children"))
            {
                v2Node["children"] = v1Node["children"];
            }
            
            // Type-specific fields
            if (v1Node.contains("actionType"))
            {
                v2Node["actionType"] = v1Node["actionType"];
            }
            if (v1Node.contains("conditionType"))
            {
                v2Node["conditionType"] = v1Node["conditionType"];
            }
            if (v1Node.contains("decoratorType"))
            {
                v2Node["decoratorType"] = v1Node["decoratorType"];
            }
            
            // Migrate parameters to unified structure
            MigrateParameters(v1Node, v2Node);
            
            v2Data["nodes"].push_back(v2Node);
        }
    }
    
    void BlueprintMigrator::MigrateHFSM(const json& v1, json& v2Data)
    {
        // Similar to BehaviorTree but for HFSM
        v2Data["initialState"] = v1.value<std::string>("initialState", "");
        v2Data["states"] = json::array();
        
        if (v1.contains("states") && v1["states"].is_array())
        {
            for (size_t i = 0; i < v1["states"].size(); ++i)
            {
                const json& state = v1["states"][i];
                json v2State = state;
                // Add position if not present
                if (!v2State.contains("position"))
                {
                    v2State["position"]["x"] = 400.0;
                    v2State["position"]["y"] = 300.0;
                }
                v2Data["states"].push_back(v2State);
            }
        }
        
        if (v1.contains("transitions"))
        {
            v2Data["transitions"] = v1["transitions"];
        }
    }
    
    void BlueprintMigrator::MigrateEntityPrefab(const json& v1, json& v2Data)
    {
        // EntityPrefab migration - mostly copy as-is
        v2Data["prefabName"] = v1.value<std::string>("name", "Unnamed");
        
        if (v1.contains("components"))
        {
            v2Data["components"] = v1["components"];
        }
        else
        {
            v2Data["components"] = json::array();
        }
    }
    
    void BlueprintMigrator::MigrateParameters(const json& v1Node, json& v2Node)
    {
        v2Node["parameters"] = json::object();
        
        // Migrate old parameter fields to new unified structure
        if (v1Node.contains("param"))
        {
            v2Node["parameters"]["param"] = v1Node["param"];
        }
        if (v1Node.contains("param1"))
        {
            v2Node["parameters"]["param1"] = v1Node["param1"];
        }
        if (v1Node.contains("param2"))
        {
            v2Node["parameters"]["param2"] = v1Node["param2"];
        }
    }
    
    std::map<int, BlueprintMigrator::NodeLayout> BlueprintMigrator::CalculateHierarchicalLayout(
        const json& nodes,
        const std::map<int, std::vector<int>>& childrenMap,
        int rootId)
    {
        std::map<int, NodeLayout> layouts;
        
        // BFS to calculate positions
        std::queue<std::tuple<int, int, int>> queue; // nodeId, depth, siblingIndex
        queue.push(std::make_tuple(rootId, 0, 0));
        
        std::map<int, int> depthCounter; // Count nodes at each depth
        
        while (!queue.empty())
        {
            std::tuple<int, int, int> front = queue.front();
            queue.pop();
            
            int nodeId = std::get<0>(front);
            int depth = std::get<1>(front);
            int siblingIndex = std::get<2>(front);
            
            // Calculate position
            NodeLayout layout;
            layout.nodeId = nodeId;
            layout.depth = depth;
            layout.siblingIndex = depthCounter[depth]++;
            
            layout.posX = START_X + depth * HORIZONTAL_SPACING;
            layout.posY = START_Y + layout.siblingIndex * VERTICAL_SPACING;
            
            layouts[nodeId] = layout;
            
            // Add children to queue
            if (childrenMap.count(nodeId))
            {
                int childIndex = 0;
                for (int childId : childrenMap.at(nodeId))
                {
                    queue.push(std::make_tuple(childId, depth + 1, childIndex++));
                }
            }
        }
        
        return layouts;
    }
}
