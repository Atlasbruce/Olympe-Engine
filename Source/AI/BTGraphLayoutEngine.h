/**
 * @file BTGraphLayoutEngine.h
 * @brief Graph layout engine for behavior tree visualization
 * @author Olympe Engine - Behavior Tree Debugger
 * @date 2025
 * 
 * @details
 * Implements a 5-phase Sugiyama algorithm for hierarchical graph layout:
 * 1. Layering: Assign nodes to layers via BFS
 * 2. Initial Ordering: Order nodes within layers
 * 3. Crossing Reduction: Minimize edge crossings using barycenter heuristic
 * 4. X-Coordinate Assignment: Horizontal positioning with parent centering
 * 5. Collision Resolution: Resolve overlaps with dynamic spacing
 */

#pragma once

#include "../vector.h"
#include "../AI/BehaviorTree.h"
#include <vector>
#include <map>

namespace Olympe
{
    /**
     * @struct BTNodeLayout
     * @brief Layout information for a single behavior tree node
     */
    struct BTNodeLayout
    {
        uint32_t nodeId = 0;           ///< BT node ID
        Vector position;                ///< Final position (x, y)
        int layer = 0;                  ///< Hierarchical layer (0 = root)
        int orderInLayer = 0;           ///< Order within the layer
        float width = 120.0f;           ///< Node visual width
        float height = 60.0f;           ///< Node visual height
    };

    /**
     * @class BTGraphLayoutEngine
     * @brief Computes clean hierarchical layouts for behavior trees
     * 
     * Uses the Sugiyama algorithm to create readable, professional-looking
     * node graphs without overlaps.
     */
    class BTGraphLayoutEngine
    {
    public:
        BTGraphLayoutEngine();
        ~BTGraphLayoutEngine() = default;

        /**
         * @brief Compute layout for a behavior tree
         * @param tree The behavior tree asset to layout
         * @param nodeSpacingX Horizontal spacing between nodes
         * @param nodeSpacingY Vertical spacing between layers
         * @return Vector of node layouts with computed positions
         */
        std::vector<BTNodeLayout> ComputeLayout(
            const BehaviorTreeAsset* tree,
            float nodeSpacingX = 250.0f,
            float nodeSpacingY = 150.0f
        );

        /**
         * @brief Get computed layout for a specific node
         * @param nodeId The BT node ID
         * @return Pointer to layout, or nullptr if not found
         */
        const BTNodeLayout* GetNodeLayout(uint32_t nodeId) const;

    private:
        // Phase 1: Assign nodes to layers via BFS from root
        void AssignLayers(const BehaviorTreeAsset* tree);

        // Phase 2: Initial ordering of nodes within each layer
        void InitialOrdering();

        // Phase 3: Reduce edge crossings (10 passes of barycenter heuristic)
        void ReduceCrossings(const BehaviorTreeAsset* tree);

        // Phase 4: Assign X coordinates with parent centering
        void AssignXCoordinates(float nodeSpacingX);

        // Phase 5: Resolve collisions between nodes
        void ResolveCollisions(float nodeSpacingX);

        // Helper: Get children of a node
        std::vector<uint32_t> GetChildren(const BTNode* node) const;

        // Helper: Get parent nodes (reverse lookup)
        void BuildParentMap(const BehaviorTreeAsset* tree);

        // Helper: Calculate barycenter for a node
        float CalculateBarycenter(uint32_t nodeId, const std::vector<BTNodeLayout*>& neighbors) const;

        // Computed layouts
        std::vector<BTNodeLayout> m_layouts;
        std::map<uint32_t, size_t> m_nodeIdToIndex;  // nodeId -> index in m_layouts

        // Temporary data structures for algorithm
        std::vector<std::vector<uint32_t>> m_layers;  // layer -> [nodeIds]
        std::map<uint32_t, std::vector<uint32_t>> m_parentMap;  // nodeId -> [parentIds]
    };
}
