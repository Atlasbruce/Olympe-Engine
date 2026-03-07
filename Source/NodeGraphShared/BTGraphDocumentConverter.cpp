/**
 * @file BTGraphDocumentConverter.cpp
 * @brief Implementation of BehaviorTreeAsset -> Olympe::NodeGraph conversion
 *
 * @details
 * Converts a runtime BehaviorTreeAsset into an Olympe::NodeGraph by building
 * an intermediate JSON document and using NodeGraph::FromJson() to preserve
 * original BT node IDs.  This ensures that NodeGraphPanel::SetActiveDebugNode()
 * can highlight the correct node using the BT runtime node index directly.
 *
 * Layout positions are computed via BTGraphLayoutEngine when nodes have no
 * stored positions (all zeros).
 */

#include "BTGraphDocumentConverter.h"
#include "../BlueprintEditor/BTNodeGraphManager.h"  // NodeGraph, NodeType, NodeTypeToString
#include "../AI/BTGraphLayoutEngine.h"
#include "../third_party/nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

namespace Olympe {
namespace NodeGraphShared {

// Map BTNodeType (runtime) to NodeType (Blueprint Editor) for visual consistency.
static NodeType MapBTToEditor(BTNodeType t)
{
    switch (t)
    {
    case BTNodeType::Selector:  return NodeType::BT_Selector;
    case BTNodeType::Sequence:  return NodeType::BT_Sequence;
    case BTNodeType::Condition: return NodeType::BT_Condition;
    case BTNodeType::Action:    return NodeType::BT_Action;
    case BTNodeType::Inverter:  return NodeType::BT_Decorator;
    case BTNodeType::Repeater:  return NodeType::BT_Decorator;
    default:                    return NodeType::BT_Action;
    }
}

NodeGraph* BTGraphDocumentConverter::FromBehaviorTree(const BehaviorTreeAsset* tree)
{
    if (!tree)
    {
        std::cerr << "[BTGraphDocumentConverter] ERROR: null BehaviorTreeAsset\n";
        return nullptr;
    }

    if (tree->nodes.empty())
    {
        std::cerr << "[BTGraphDocumentConverter] WARNING: empty BehaviorTreeAsset '"
                  << tree->name << "'\n";
        NodeGraph* emptyGraph = new NodeGraph();
        emptyGraph->name = tree->name;
        emptyGraph->type = "BehaviorTree";
        return emptyGraph;
    }

    // Priority 1: Load directly from the JSON source file so that the saved visual
    // positions are preserved — identical to the Blueprint Editor standalone view.
    const std::string filepath = BehaviorTreeManager::Get().GetTreePathFromId(tree->id);
    if (!filepath.empty() && filepath.rfind("TreeName:", 0) != 0)
    {
        const int tempId = NodeGraphManager::Get().LoadGraph(filepath);
        if (tempId != -1)
        {
            NodeGraph* loadedGraph = NodeGraphManager::Get().GetGraph(tempId);
            if (loadedGraph)
            {
                // Clone the graph so we can close the temporary slot immediately.
                NodeGraph* result = new NodeGraph(*loadedGraph);
                // The debugger is read-only: clear dirty so no accidental saves.
                result->ClearDirty();
                NodeGraphManager::Get().CloseGraph(tempId);
                std::cout << "[BTGraphDocumentConverter] Loaded BT '" << tree->name
                          << "' from JSON (saved positions preserved)\n";
                return result;
            }
            NodeGraphManager::Get().CloseGraph(tempId);
        }
    }

    // Priority 2: Fallback — filepath unknown or LoadGraph failed.
    // Compute BFS layout positions and build the graph from the runtime asset.
    BTGraphLayoutEngine layoutEngine;
    layoutEngine.ComputeLayout(tree);

    // Build a v2-format JSON document that NodeGraph::FromJson() can parse.
    // This preserves BT node IDs in the resulting NodeGraph.
    json j;
    j["schema_version"] = 2;
    j["name"]           = tree->name;
    j["blueprintType"]  = "BehaviorTree";

    json& data = j["data"];
    data["rootNodeId"]  = static_cast<int>(tree->rootNodeId);
    data["nodes"]       = json::array();

    for (const auto& btNode : tree->nodes)
    {
        json nj;
        nj["id"]   = static_cast<int>(btNode.id);
        nj["type"] = NodeTypeToString(MapBTToEditor(btNode.type));
        nj["name"] = btNode.name.empty() ? NodeTypeToString(MapBTToEditor(btNode.type))
                                         : btNode.name;

        // Position from layout engine
        float posX = 0.0f;
        float posY = 0.0f;
        const BTNodeLayout* layout = layoutEngine.GetNodeLayout(btNode.id);
        if (layout)
        {
            posX = layout->position.x;
            posY = layout->position.y;
        }
        nj["position"]["x"] = posX;
        nj["position"]["y"] = posY;

        // Parameters from BTNode flexible params
        nj["parameters"] = json::object();
        for (const auto& kv : btNode.stringParams)
            nj["parameters"][kv.first] = kv.second;

        // Children (composite nodes)
        nj["children"] = json::array();
        for (uint32_t childId : btNode.childIds)
            nj["children"].push_back(static_cast<int>(childId));

        // Decorator child (single child for Inverter / Repeater)
        if (btNode.decoratorChildId != 0)
            nj["decoratorChild"] = static_cast<int>(btNode.decoratorChildId);

        data["nodes"].push_back(nj);
    }

    // FromJson is a friend of NodeGraph and preserves the node IDs from JSON.
    NodeGraph converted = NodeGraph::FromJson(j);

    NodeGraph* result = new NodeGraph(std::move(converted));

    std::cout << "[BTGraphDocumentConverter] Converted BT '" << tree->name
              << "': " << tree->nodes.size() << " nodes (BFS fallback layout)\n";

    return result;
}

void BTGraphDocumentConverter::SyncActiveNode(NodeGraph* /*graph*/, uint32_t /*currentNodeId*/)
{
    // Active node highlighting is driven externally by
    // NodeGraphPanel::SetActiveDebugNode().  Nothing to do here.
}

} // namespace NodeGraphShared
} // namespace Olympe
