/**
 * @file BTGraphLayoutEngine.cpp
 * @brief Implementation of graph layout engine for behavior tree visualization
 */

#include "BTGraphLayoutEngine.h"
#include "../system/system_utils.h"
#include <queue>
#include <algorithm>

namespace Olympe
{
    BTGraphLayoutEngine::BTGraphLayoutEngine()
    {
    }

    std::vector<BTNodeLayout> BTGraphLayoutEngine::ComputeLayout(
        const BehaviorTreeAsset* tree,
        float nodeSpacingX,
        float nodeSpacingY,
        float zoomFactor)
    {
        // Suppress unused parameter warnings (fixed grid is used)
        static_cast<void>(nodeSpacingX);
        static_cast<void>(nodeSpacingY);
        static_cast<void>(zoomFactor);

        if (!tree || tree->nodes.empty())
        {
            return {};
        }

        // Clear previous state
        m_layouts.clear();
        m_nodeIdToIndex.clear();
        m_layers.clear();
        m_parentMap.clear();

        // Phase 1: Assign nodes to layers via BFS
        AssignLayers(tree);

        // Phase 2: Place nodes on fixed 400x200 grid (horizontal layout)
        // layers become columns (x-axis), order within layer becomes rows (y-axis)
        const float gridColumnSpacing = 400.0f;  // pixels between adjacent layers (columns)
        const float gridRowSpacing    = 200.0f;  // pixels between nodes in the same layer (rows)

        for (auto& layout : m_layouts)
        {
            layout.position.x = layout.layer * gridColumnSpacing;
            layout.position.y = layout.orderInLayer * gridRowSpacing;
        }

        SYSTEM_LOG << "[BTGraphLayout] Layout complete: " << m_layouts.size()
                   << " nodes positioned" << std::endl;

        return m_layouts;
    }

    const BTNodeLayout* BTGraphLayoutEngine::GetNodeLayout(uint32_t nodeId) const
    {
        auto it = m_nodeIdToIndex.find(nodeId);
        if (it != m_nodeIdToIndex.end())
        {
            return &m_layouts[it->second];
        }
        return nullptr;
    }

    bool BTGraphLayoutEngine::UpdateNodePosition(uint32_t nodeId, float x, float y)
    {
        auto it = m_nodeIdToIndex.find(nodeId);
        if (it == m_nodeIdToIndex.end())
            return false;
        m_layouts[it->second].position.x = x;
        m_layouts[it->second].position.y = y;
        return true;
    }

    void BTGraphLayoutEngine::AssignLayers(const BehaviorTreeAsset* tree)
    {
        // BFS from root to assign layers
        std::queue<std::pair<uint32_t, int>> queue;  // (nodeId, layer)
        std::map<uint32_t, int> visitedLayers;

        queue.push({tree->rootNodeId, 0});
        visitedLayers[tree->rootNodeId] = 0;

        int maxLayer = 0;

        while (!queue.empty())
        {
            std::pair<uint32_t, int> front = queue.front();
            uint32_t nodeId = front.first;
            int layer = front.second;
            queue.pop();

            const BTNode* node = tree->GetNode(nodeId);
            if (!node)
                continue;

            // Create layout for this node
            BTNodeLayout layout;
            layout.nodeId = nodeId;
            layout.layer = layer;
            layout.orderInLayer = 0;  // Will be set below

            size_t idx = m_layouts.size();
            m_layouts.push_back(layout);
            m_nodeIdToIndex[nodeId] = idx;

            maxLayer = std::max(maxLayer, layer);

            // Get children and add to queue
            auto children = GetChildren(node);
            for (uint32_t childId : children)
            {
                // Only visit each node once (shortest path from root)
                if (visitedLayers.find(childId) == visitedLayers.end())
                {
                    visitedLayers[childId] = layer + 1;
                    queue.push({childId, layer + 1});
                }
            }
        }

        // Organize nodes into layers and set orderInLayer
        m_layers.resize(maxLayer + 1);
        for (const auto& layout : m_layouts)
        {
            m_layers[layout.layer].push_back(layout.nodeId);
        }

        for (size_t layerIdx = 0; layerIdx < m_layers.size(); ++layerIdx)
        {
            const auto& layer = m_layers[layerIdx];
            for (size_t i = 0; i < layer.size(); ++i)
            {
                auto it = m_nodeIdToIndex.find(layer[i]);
                if (it != m_nodeIdToIndex.end())
                {
                    m_layouts[it->second].orderInLayer = static_cast<int>(i);
                }
            }
        }

        // Build parent map for reference
        BuildParentMap(tree);
    }

    std::vector<uint32_t> BTGraphLayoutEngine::GetChildren(const BTNode* node) const
    {
        if (!node)
            return {};

        std::vector<uint32_t> children;

        // Composite nodes (Selector, Sequence)
        if (node->type == BTNodeType::Selector || node->type == BTNodeType::Sequence)
        {
            children = node->childIds;
        }
        // Decorator nodes (Inverter, Repeater)
        else if (node->type == BTNodeType::Inverter || node->type == BTNodeType::Repeater)
        {
            if (node->decoratorChildId != 0)
            {
                children.push_back(node->decoratorChildId);
            }
        }

        return children;
    }

    void BTGraphLayoutEngine::BuildParentMap(const BehaviorTreeAsset* tree)
    {
        m_parentMap.clear();

        for (const auto& node : tree->nodes)
        {
            auto children = GetChildren(&node);
            for (uint32_t childId : children)
            {
                m_parentMap[childId].push_back(node.id);
            }
        }
    }

} // namespace Olympe
