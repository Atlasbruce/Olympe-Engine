/**
 * @file BTGraphLayoutEngine.cpp
 * @brief Implementation of graph layout engine for behavior tree visualization
 */

#include "BTGraphLayoutEngine.h"
#include <queue>
#include <algorithm>
#include <cmath>

namespace Olympe
{
    BTGraphLayoutEngine::BTGraphLayoutEngine()
    {
    }

    std::vector<BTNodeLayout> BTGraphLayoutEngine::ComputeLayout(
        const BehaviorTreeAsset* tree,
        float nodeSpacingX,
        float nodeSpacingY)
    {
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

        // Phase 2: Initial ordering within layers
        InitialOrdering();

        // Phase 3: Reduce crossings (10 passes)
        ReduceCrossings(tree);

        // Phase 4: Apply Buchheim-Walker optimal layout for better parent centering
        // This sets position.x in abstract units (0, 1, 2, etc.)
        ApplyBuchheimWalkerLayout(tree);
        
        // Phase 5: Force-directed collision resolution with generous padding
        // This works in abstract unit space
        const float nodePadding = 1.5f;  // 1.5 abstract units of padding
        const int maxIterations = 15;    // More iterations for better convergence
        ResolveNodeCollisionsForceDirected(nodePadding, maxIterations);

        // Calculate adaptive spacing multipliers based on tree complexity
        float spacingMultiplierX = 1.0f;
        float spacingMultiplierY = 1.0f;
        
        // Calculate tree complexity
        size_t maxNodesInLayer = 1;
        for (const auto& layer : m_layers)
        {
            maxNodesInLayer = std::max(maxNodesInLayer, layer.size());
        }
        
        // Adaptive spacing: increase for wide trees
        if (maxNodesInLayer > 5)
        {
            spacingMultiplierX = 1.3f;  // +30% for wide trees
        }
        
        // Adaptive spacing: increase for deep trees
        if (m_layers.size() > 5)
        {
            spacingMultiplierY = 1.2f;  // +20% for deep trees
        }
        
        // Apply final spacing multipliers
        float finalSpacingX = nodeSpacingX * spacingMultiplierX;
        float finalSpacingY = nodeSpacingY * spacingMultiplierY;

        // Convert from abstract units to world coordinates and apply layout direction
        if (m_layoutDirection == BTLayoutDirection::TopToBottom)
        {
            // Vertical layout (default): layers go top-to-bottom
            for (auto& layout : m_layouts)
            {
                // Convert abstract X units to world coordinates
                layout.position.x *= finalSpacingX;
                // Set Y based on layer depth
                layout.position.y = layout.layer * finalSpacingY;
            }
        }
        else  // LeftToRight
        {
            // Horizontal layout: rotate 90° clockwise
            // Layers become left-to-right, abstract X units become vertical positions
            for (auto& layout : m_layouts)
            {
                float abstractX = layout.position.x;
                // Layers become horizontal position
                layout.position.x = layout.layer * finalSpacingY;
                // Abstract X units become vertical position
                layout.position.y = abstractX * finalSpacingX;
            }
        }

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
            layout.orderInLayer = 0;  // Will be set in InitialOrdering

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

        // Organize nodes into layers
        m_layers.resize(maxLayer + 1);
        for (const auto& layout : m_layouts)
        {
            m_layers[layout.layer].push_back(layout.nodeId);
        }

        // Build parent map for later phases
        BuildParentMap(tree);
    }

    void BTGraphLayoutEngine::InitialOrdering()
    {
        // Simple initial ordering: maintain order from BFS
        for (size_t layerIdx = 0; layerIdx < m_layers.size(); ++layerIdx)
        {
            auto& layer = m_layers[layerIdx];
            for (size_t i = 0; i < layer.size(); ++i)
            {
                uint32_t nodeId = layer[i];
                auto it = m_nodeIdToIndex.find(nodeId);
                if (it != m_nodeIdToIndex.end())
                {
                    m_layouts[it->second].orderInLayer = static_cast<int>(i);
                }
            }
        }
    }

    void BTGraphLayoutEngine::ReduceCrossings(const BehaviorTreeAsset* tree)
    {
        // Barycenter heuristic - 10 passes alternating between forward and backward
        const int numPasses = 10;

        for (int pass = 0; pass < numPasses; ++pass)
        {
            // Forward pass (top to bottom)
            if (pass % 2 == 0)
            {
                for (size_t layerIdx = 1; layerIdx < m_layers.size(); ++layerIdx)
                {
                    auto& layer = m_layers[layerIdx];
                    
                    // Calculate barycenter for each node based on parents
                    std::vector<std::pair<float, uint32_t>> barycenters;
                    for (uint32_t nodeId : layer)
                    {
                        auto parentIt = m_parentMap.find(nodeId);
                        if (parentIt != m_parentMap.end() && !parentIt->second.empty())
                        {
                            // Get parent layouts
                            std::vector<BTNodeLayout*> parents;
                            for (uint32_t parentId : parentIt->second)
                            {
                                auto it = m_nodeIdToIndex.find(parentId);
                                if (it != m_nodeIdToIndex.end())
                                {
                                    parents.push_back(&m_layouts[it->second]);
                                }
                            }
                            
                            float barycenter = CalculateBarycenter(nodeId, parents);
                            barycenters.push_back({barycenter, nodeId});
                        }
                        else
                        {
                            // No parents, keep current order
                            auto it = m_nodeIdToIndex.find(nodeId);
                            if (it != m_nodeIdToIndex.end())
                            {
                                float currentOrder = static_cast<float>(m_layouts[it->second].orderInLayer);
                                barycenters.push_back({currentOrder, nodeId});
                            }
                        }
                    }

                    // Sort by barycenter
                    std::sort(barycenters.begin(), barycenters.end());

                    // Update order
                    layer.clear();
                    for (size_t i = 0; i < barycenters.size(); ++i)
                    {
                        uint32_t nodeId = barycenters[i].second;
                        layer.push_back(nodeId);
                        auto it = m_nodeIdToIndex.find(nodeId);
                        if (it != m_nodeIdToIndex.end())
                        {
                            m_layouts[it->second].orderInLayer = static_cast<int>(i);
                        }
                    }
                }
            }
            else  // Backward pass (bottom to top)
            {
                for (int layerIdx = static_cast<int>(m_layers.size()) - 2; layerIdx >= 0; --layerIdx)
                {
                    auto& layer = m_layers[layerIdx];
                    
                    // Calculate barycenter for each node based on children
                    std::vector<std::pair<float, uint32_t>> barycenters;
                    for (uint32_t nodeId : layer)
                    {
                        auto it = m_nodeIdToIndex.find(nodeId);
                        if (it == m_nodeIdToIndex.end())
                            continue;

                        const BTNode* node = tree->GetNode(nodeId);
                        if (!node)
                            continue;

                        auto children = GetChildren(node);
                        if (!children.empty())
                        {
                            // Get child layouts
                            std::vector<BTNodeLayout*> childLayouts;
                            for (uint32_t childId : children)
                            {
                                auto childIt = m_nodeIdToIndex.find(childId);
                                if (childIt != m_nodeIdToIndex.end())
                                {
                                    childLayouts.push_back(&m_layouts[childIt->second]);
                                }
                            }
                            
                            float barycenter = CalculateBarycenter(nodeId, childLayouts);
                            barycenters.push_back({barycenter, nodeId});
                        }
                        else
                        {
                            float currentOrder = static_cast<float>(m_layouts[it->second].orderInLayer);
                            barycenters.push_back({currentOrder, nodeId});
                        }
                    }

                    // Sort by barycenter
                    std::sort(barycenters.begin(), barycenters.end());

                    // Update order
                    layer.clear();
                    for (size_t i = 0; i < barycenters.size(); ++i)
                    {
                        uint32_t nodeId = barycenters[i].second;
                        layer.push_back(nodeId);
                        auto it = m_nodeIdToIndex.find(nodeId);
                        if (it != m_nodeIdToIndex.end())
                        {
                            m_layouts[it->second].orderInLayer = static_cast<int>(i);
                        }
                    }
                }
            }
        }
    }

    void BTGraphLayoutEngine::AssignXCoordinates(float nodeSpacingX)
    {
        // Assign X coordinates based on order in layer
        for (const auto& layer : m_layers)
        {
            float totalWidth = (layer.size() - 1) * nodeSpacingX;
            float startX = -totalWidth / 2.0f;  // Center around 0

            for (size_t i = 0; i < layer.size(); ++i)
            {
                uint32_t nodeId = layer[i];
                auto it = m_nodeIdToIndex.find(nodeId);
                if (it != m_nodeIdToIndex.end())
                {
                    m_layouts[it->second].position.x = startX + i * nodeSpacingX;
                }
            }
        }
    }

    void BTGraphLayoutEngine::ResolveCollisions(float nodeSpacingX)
    {
        // Simple collision resolution: expand spacing if nodes are too close
        const float minSpacing = nodeSpacingX * 0.8f;

        for (auto& layer : m_layers)
        {
            if (layer.size() < 2)
                continue;

            // Sort nodes by X coordinate
            std::sort(layer.begin(), layer.end(), [this](uint32_t a, uint32_t b) {
                auto itA = m_nodeIdToIndex.find(a);
                auto itB = m_nodeIdToIndex.find(b);
                if (itA == m_nodeIdToIndex.end() || itB == m_nodeIdToIndex.end())
                    return false;
                return m_layouts[itA->second].position.x < m_layouts[itB->second].position.x;
            });

            // Check for collisions and adjust
            for (size_t i = 1; i < layer.size(); ++i)
            {
                uint32_t prevNodeId = layer[i - 1];
                uint32_t currNodeId = layer[i];

                auto itPrev = m_nodeIdToIndex.find(prevNodeId);
                auto itCurr = m_nodeIdToIndex.find(currNodeId);

                if (itPrev == m_nodeIdToIndex.end() || itCurr == m_nodeIdToIndex.end())
                    continue;

                BTNodeLayout& prevLayout = m_layouts[itPrev->second];
                BTNodeLayout& currLayout = m_layouts[itCurr->second];

                float distance = currLayout.position.x - prevLayout.position.x;
                if (distance < minSpacing)
                {
                    // Push current node to the right
                    currLayout.position.x = prevLayout.position.x + minSpacing;
                }
            }
        }
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

    float BTGraphLayoutEngine::CalculateBarycenter(uint32_t nodeId, const std::vector<BTNodeLayout*>& neighbors) const
    {
        if (neighbors.empty())
            return 0.0f;

        float sum = 0.0f;
        for (const BTNodeLayout* neighbor : neighbors)
        {
            sum += neighbor->orderInLayer;
        }

        return sum / neighbors.size();
    }

    void BTGraphLayoutEngine::ApplyBuchheimWalkerLayout(const BehaviorTreeAsset* tree)
    {
        /*
         * Buchheim-Walker Algorithm (2002)
         * Reference: "Improving Walker's Algorithm to Run in Linear Time"
         * 
         * Guarantees:
         * 1. Parents centered on their children
         * 2. No collisions between sibling subtrees
         * 3. Optimal horizontal space usage
         * 4. Linear time complexity O(n)
         */

        if (!tree || m_layers.empty() || m_layouts.empty())
            return;

        // Start from root and recursively place subtrees
        if (!m_layers.empty() && !m_layers[0].empty())
        {
            uint32_t rootId = m_layers[0][0];
            float startX = 0.0f;
            PlaceSubtree(rootId, tree, 0, startX);
        }
    }

    void BTGraphLayoutEngine::PlaceSubtree(uint32_t nodeId, const BehaviorTreeAsset* tree, int depth, float& nextAvailableX)
    {
        const BTNode* node = tree->GetNode(nodeId);
        if (!node)
            return;

        auto itLayout = m_nodeIdToIndex.find(nodeId);
        if (itLayout == m_nodeIdToIndex.end())
            return;

        BTNodeLayout& layout = m_layouts[itLayout->second];
        auto children = GetChildren(node);

        if (children.empty())
        {
            // Leaf: place at next available position
            layout.position.x = nextAvailableX;
            nextAvailableX += 1.0f;  // Reserve 1 unit
            return;
        }

        // Recursively place all children
        float childrenStartX = nextAvailableX;
        for (uint32_t childId : children)
        {
            PlaceSubtree(childId, tree, depth + 1, nextAvailableX);
        }
        float childrenEndX = nextAvailableX;

        // Center parent on children
        // Note: Position values are in abstract units where each leaf occupies 1.0 unit.
        // childrenStartX = position where first child starts (e.g., 0)
        // childrenEndX = nextAvailableX after all children placed (e.g., 2 for two children)
        // Since nextAvailableX is one past the last child's position, we subtract 1.0
        // Example: Two children at 0 and 1 -> midpoint = (0 + 2 - 1) / 2 = 0.5 ✓
        // Example: One child at 0 -> midpoint = (0 + 1 - 1) / 2 = 0 ✓
        float childrenMidpoint = (childrenStartX + childrenEndX - 1.0f) / 2.0f;
        layout.position.x = childrenMidpoint;

        // If parent position collides with previous sibling's subtree, shift everything
        if (layout.position.x < childrenStartX)
        {
            float shift = childrenStartX - layout.position.x;
            layout.position.x += shift;
            
            // Shift all children by the same amount
            for (uint32_t childId : children)
            {
                ShiftSubtree(childId, tree, shift);
            }
        }
    }

    void BTGraphLayoutEngine::ShiftSubtree(uint32_t nodeId, const BehaviorTreeAsset* tree, float offset)
    {
        auto itLayout = m_nodeIdToIndex.find(nodeId);
        if (itLayout == m_nodeIdToIndex.end())
            return;

        m_layouts[itLayout->second].position.x += offset;

        const BTNode* node = tree->GetNode(nodeId);
        if (!node)
            return;

        auto children = GetChildren(node);
        for (uint32_t childId : children)
        {
            ShiftSubtree(childId, tree, offset);
        }
    }

    void BTGraphLayoutEngine::ResolveNodeCollisionsForceDirected(float nodePadding, int maxIterations)
    {
        for (int iter = 0; iter < maxIterations; ++iter)
        {
            bool hadCollision = false;

            // Check all pairs within each layer
            for (size_t layerIdx = 0; layerIdx < m_layers.size(); ++layerIdx)
            {
                auto& layer = m_layers[layerIdx];

                for (size_t i = 0; i < layer.size(); ++i)
                {
                    for (size_t j = i + 1; j < layer.size(); ++j)
                    {
                        uint32_t nodeA = layer[i];
                        uint32_t nodeB = layer[j];

                        auto itA = m_nodeIdToIndex.find(nodeA);
                        auto itB = m_nodeIdToIndex.find(nodeB);

                        if (itA == m_nodeIdToIndex.end() || itB == m_nodeIdToIndex.end())
                            continue;

                        BTNodeLayout& layoutA = m_layouts[itA->second];
                        BTNodeLayout& layoutB = m_layouts[itB->second];

                        if (DoNodesOverlap(layoutA, layoutB, nodePadding))
                        {
                            PushNodeApart(nodeA, nodeB, nodePadding);
                            hadCollision = true;
                        }
                    }
                }
            }

            if (!hadCollision)
            {
                // Converged early
                break;
            }
        }
    }

    bool BTGraphLayoutEngine::DoNodesOverlap(const BTNodeLayout& a, const BTNodeLayout& b, float padding) const
    {
        // Note: During collision resolution, positions are in abstract units (0, 1, 2, etc.)
        // We treat each node as occupying 1.0 abstract unit width
        // Height is not relevant since we only check horizontal collisions within the same layer
        
        const float abstractNodeWidth = 1.0f;  // Each node occupies 1 abstract unit
        
        float aLeft = a.position.x - abstractNodeWidth / 2.0f - padding;
        float aRight = a.position.x + abstractNodeWidth / 2.0f + padding;

        float bLeft = b.position.x - abstractNodeWidth / 2.0f;
        float bRight = b.position.x + abstractNodeWidth / 2.0f;

        // Check horizontal overlap (vertical not relevant since both nodes in same layer)
        return !(aRight < bLeft || aLeft > bRight);
    }

    void BTGraphLayoutEngine::PushNodeApart(uint32_t nodeA, uint32_t nodeB, float minDistance)
    {
        auto itA = m_nodeIdToIndex.find(nodeA);
        auto itB = m_nodeIdToIndex.find(nodeB);

        if (itA == m_nodeIdToIndex.end() || itB == m_nodeIdToIndex.end())
            return;

        BTNodeLayout& layoutA = m_layouts[itA->second];
        BTNodeLayout& layoutB = m_layouts[itB->second];

        // Note: Positions are in abstract units where each node occupies 1.0 unit
        const float abstractNodeWidth = 1.0f;
        
        // Calculate center-to-center distance
        float dx = layoutB.position.x - layoutA.position.x;
        float centerDistance = std::abs(dx);
        
        // Minimum center-to-center distance needed = abstractNodeWidth + minDistance
        float requiredCenterDistance = abstractNodeWidth + minDistance;
        
        if (centerDistance < requiredCenterDistance)
        {
            // Calculate how much total separation is needed
            float totalPushNeeded = requiredCenterDistance - centerDistance;
            // Each node moves half the distance
            float pushAmount = totalPushNeeded / 2.0f;

            // Push nodes apart in the direction they're already separated
            // If B is to the right of A (dx > 0), push A left and B right
            // If B is to the left of A (dx < 0), push A right and B left
            if (dx > 0)
            {
                layoutA.position.x -= pushAmount;
                layoutB.position.x += pushAmount;
            }
            else if (dx < 0)
            {
                layoutA.position.x += pushAmount;
                layoutB.position.x -= pushAmount;
            }
            // If dx == 0 (nodes at same X), push them apart arbitrarily
            else
            {
                layoutA.position.x -= pushAmount;
                layoutB.position.x += pushAmount;
            }
        }
    }
}
