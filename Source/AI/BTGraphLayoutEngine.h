/**
 * @file BTGraphLayoutEngine.h
 * @brief Graph layout engine for behavior tree visualization
 * @author Olympe Engine - Behavior Tree Debugger
 * @date 2025
 * 
 * @details
 * Implements a 5-phase hierarchical graph layout algorithm:
 * 1. Layering: Assign nodes to layers via BFS
 * 2. Initial Ordering: Order nodes within layers
 * 3. Crossing Reduction: Minimize edge crossings using barycenter heuristic
 * 4. Buchheim-Walker Layout: Optimal parent centering in abstract unit space
 * 5. Force-Directed Collision Resolution: Iterative overlap elimination
 * 
 * The algorithm works in abstract unit space (each node = 1.0 unit) and converts
 * to world coordinates at the end with adaptive spacing based on tree complexity.
 */

#pragma once

#include "../vector.h"
#include "../AI/BehaviorTree.h"
#include <vector>
#include <map>

namespace Olympe
{
    /**
     * @enum BTLayoutDirection
     * @brief Layout direction for behavior tree visualization
     */
    enum class BTLayoutDirection
    {
        TopToBottom,    ///< Traditional top-down layout (vertical)
        LeftToRight     ///< Horizontal left-to-right layout
    };

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
        float width = 200.0f;           ///< Node visual width (increased for readability)
        float height = 100.0f;          ///< Node visual height (increased for readability)
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
         * @brief Set layout direction
         * @param direction Layout direction (TopToBottom or LeftToRight)
         */
        void SetLayoutDirection(BTLayoutDirection direction) { m_layoutDirection = direction; }

        /**
         * @brief Get current layout direction
         * @return Current layout direction
         */
        BTLayoutDirection GetLayoutDirection() const { return m_layoutDirection; }

        /**
         * @brief Compute layout for a behavior tree
         * @param tree The behavior tree asset to layout
         * @param nodeSpacingX Horizontal spacing between nodes (default: 180px, reduced from 250px)
         * @param nodeSpacingY Vertical spacing between layers (default: 120px, reduced from 180px)
         * @param zoomFactor Zoom multiplier applied to final positions (default: 1.0)
         * @return Vector of node layouts with computed positions
         */
        std::vector<BTNodeLayout> ComputeLayout(
            const BehaviorTreeAsset* tree,
            float nodeSpacingX = 180.0f,
            float nodeSpacingY = 120.0f,
            float zoomFactor = 1.0f
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

        // Phase 3: Reduce edge crossings (barycenter heuristic)
        void ReduceCrossings(const BehaviorTreeAsset* tree);

        // Phase 4: Buchheim-Walker optimal layout (in abstract unit space)
        void ApplyBuchheimWalkerLayout(const BehaviorTreeAsset* tree);
        
        // Phase 5: Force-directed collision resolution (in abstract unit space)
        void ResolveNodeCollisionsForceDirected(float nodePadding, int maxIterations);

        // DEPRECATED: Old phases replaced by Buchheim-Walker
        void AssignXCoordinates(float nodeSpacingX);
        void ResolveCollisions(float nodeSpacingX);

        // Helper: Get children of a node
        std::vector<uint32_t> GetChildren(const BTNode* node) const;

        // Helper: Get parent nodes (reverse lookup)
        void BuildParentMap(const BehaviorTreeAsset* tree);

        // Helper: Calculate barycenter for a node
        float CalculateBarycenter(uint32_t nodeId, const std::vector<BTNodeLayout*>& neighbors) const;
        
        // NEW: Helper methods for Buchheim-Walker
        void PlaceSubtree(uint32_t nodeId, const BehaviorTreeAsset* tree, int depth, float& nextAvailableX);
        void ShiftSubtree(uint32_t nodeId, const BehaviorTreeAsset* tree, float offset);
        
        // NEW: Helper methods for collision detection
        bool DoNodesOverlap(const BTNodeLayout& a, const BTNodeLayout& b, float padding) const;
        void PushNodeApart(uint32_t nodeA, uint32_t nodeB, float minDistance);

        // Layout configuration
        BTLayoutDirection m_layoutDirection = BTLayoutDirection::TopToBottom;  ///< Default vertical

        // Computed layouts
        std::vector<BTNodeLayout> m_layouts;
        std::map<uint32_t, size_t> m_nodeIdToIndex;  // nodeId -> index in m_layouts

        // Temporary data structures for algorithm
        std::vector<std::vector<uint32_t>> m_layers;  // layer -> [nodeIds]
        std::map<uint32_t, std::vector<uint32_t>> m_parentMap;  // nodeId -> [parentIds]
    };
}
