/**
 * @file GraphAutoLayout.cpp
 * @brief Implementation of the hierarchical auto-layout algorithm (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "GraphAutoLayout.h"

#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace Olympe {

// ============================================================================
// SetPos helper
// ============================================================================

void GraphAutoLayout::SetPos(TaskNodeDefinition& node, float x, float y)
{
    ParameterBinding bx, by;
    bx.Type = ParameterBindingType::Literal;
    bx.LiteralValue = TaskValue(x);
    by.Type = ParameterBindingType::Literal;
    by.LiteralValue = TaskValue(y);
    node.Parameters["__posX"] = bx;
    node.Parameters["__posY"] = by;
}

// ============================================================================
// BuildLayers
// ============================================================================

std::vector<GraphAutoLayout::Layer> GraphAutoLayout::BuildLayers(
    const TaskGraphTemplate& graph)
{
    std::vector<Layer> layers;
    if (graph.Nodes.empty()) return layers;

    // Determine root node: prefer EntryPoint, fall back to first node
    int32_t rootID = NODE_INDEX_NONE;
    if (graph.EntryPointID != NODE_INDEX_NONE)
    {
        rootID = graph.EntryPointID;
    }
    else
    {
        // Walk nodes to find one with type EntryPoint
        for (const auto& n : graph.Nodes)
        {
            if (n.Type == TaskNodeType::EntryPoint)
            {
                rootID = n.NodeID;
                break;
            }
        }
        if (rootID == NODE_INDEX_NONE)
        {
            rootID = graph.Nodes.front().NodeID;
        }
    }

    // Build adjacency list from ExecConnections
    std::unordered_map<int32_t, std::vector<int32_t>> adj;
    for (const auto& n : graph.Nodes) adj[n.NodeID]; // ensure all keys exist
    for (const auto& ec : graph.ExecConnections)
    {
        if (ec.SourceNodeID != NODE_INDEX_NONE && ec.TargetNodeID != NODE_INDEX_NONE)
            adj[ec.SourceNodeID].push_back(ec.TargetNodeID);
    }

    // BFS to assign layers (by shortest path from root)
    std::unordered_map<int32_t, int> layerOf;
    std::queue<int32_t> bfsQueue;

    layerOf[rootID] = 0;
    bfsQueue.push(rootID);

    while (!bfsQueue.empty())
    {
        int32_t cur = bfsQueue.front();
        bfsQueue.pop();

        int curLayer = layerOf[cur];

        for (int32_t child : adj[cur])
        {
            if (layerOf.find(child) == layerOf.end())
            {
                layerOf[child] = curLayer + 1;
                bfsQueue.push(child);
            }
        }
    }

    // Collect all layer indices
    int maxLayer = 0;
    for (const auto& kv : layerOf)
        if (kv.second > maxLayer) maxLayer = kv.second;

    layers.resize(static_cast<std::size_t>(maxLayer + 1));
    for (const auto& kv : layerOf)
    {
        layers[static_cast<std::size_t>(kv.second)].NodeIDs.push_back(kv.first);
    }

    // Collect orphan nodes (not reached by BFS)
    std::unordered_set<int32_t> visited;
    for (const auto& kv : layerOf) visited.insert(kv.first);

    Layer orphanLayer;
    for (const auto& n : graph.Nodes)
    {
        if (visited.find(n.NodeID) == visited.end())
            orphanLayer.NodeIDs.push_back(n.NodeID);
    }
    if (!orphanLayer.NodeIDs.empty())
        layers.push_back(orphanLayer);

    // Sort nodes within each layer by NodeID for deterministic output
    for (auto& layer : layers)
    {
        std::sort(layer.NodeIDs.begin(), layer.NodeIDs.end());
    }

    return layers;
}

// ============================================================================
// AssignPositions
// ============================================================================

void GraphAutoLayout::AssignPositions(TaskGraphTemplate& graph,
                                       const std::vector<Layer>& layers)
{
    if (graph.Nodes.empty() || layers.empty()) return;

    // Build a fast mutable lookup (node ID → index in Nodes vector)
    std::unordered_map<int32_t, std::size_t> indexOf;
    for (std::size_t i = 0; i < graph.Nodes.size(); ++i)
        indexOf[graph.Nodes[i].NodeID] = i;

    for (std::size_t layerIdx = 0; layerIdx < layers.size(); ++layerIdx)
    {
        const Layer& layer = layers[layerIdx];
        float x = ORIGIN_X + static_cast<float>(layerIdx) * SPACING_X;

        for (std::size_t rowIdx = 0; rowIdx < layer.NodeIDs.size(); ++rowIdx)
        {
            int32_t nodeID = layer.NodeIDs[rowIdx];
            float y = ORIGIN_Y + static_cast<float>(rowIdx) * SPACING_Y;

            auto it = indexOf.find(nodeID);
            if (it != indexOf.end())
            {
                SetPos(graph.Nodes[it->second], x, y);
            }
        }
    }
}

// ============================================================================
// ApplyHierarchicalLayout
// ============================================================================

void GraphAutoLayout::ApplyHierarchicalLayout(TaskGraphTemplate& graph)
{
    std::vector<Layer> layers = BuildLayers(graph);
    AssignPositions(graph, layers);
}

} // namespace Olympe
