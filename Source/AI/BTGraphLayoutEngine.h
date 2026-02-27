/**
 * @file BTGraphLayoutEngine.h
 * @brief Graph layout engine for behavior tree visualization
 * @author Olympe Engine - Behavior Tree Debugger
 * @date 2025
 * 
 * @details
 * Implements a 2-phase hierarchical graph layout algorithm:
 * 1. Layering: Assign nodes to layers via BFS
 * 2. Fixed Grid Placement: Place nodes on a fixed 400x200 grid (horizontal layout)
 * 
 * Produces a clean horizontal layout with no overlaps.
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
     * Uses BFS layering followed by a fixed 400x200 grid to produce
     * a horizontal layout with no overlaps.
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
         * @param nodeSpacingX Horizontal spacing between nodes (unused, fixed grid used)
         * @param nodeSpacingY Vertical spacing between layers (unused, fixed grid used)
         * @param zoomFactor Zoom multiplier (unused, fixed grid used)
         * @return Vector of node layouts with computed positions
         */
        std::vector<BTNodeLayout> ComputeLayout(
            const BehaviorTreeAsset* tree,
            float nodeSpacingX = 320.0f,
            float nodeSpacingY = 180.0f,
            float zoomFactor = 1.0f
        );

        /**
         * @brief Get computed layout for a specific node
         * @param nodeId The BT node ID
         * @return Pointer to layout, or nullptr if not found
         */
        const BTNodeLayout* GetNodeLayout(uint32_t nodeId) const;

        /**
         * @brief Update the stored position for a node (e.g. after user drag).
         * @param nodeId The BT node ID.
         * @param x      New X position in canvas (grid) units.
         * @param y      New Y position in canvas (grid) units.
         * @return true if the node was found and updated, false otherwise.
         */
        bool UpdateNodePosition(uint32_t nodeId, float x, float y);

    private:
        // Phase 1: Assign nodes to layers via BFS from root
        void AssignLayers(const BehaviorTreeAsset* tree);

        // Helper: Get children of a node
        std::vector<uint32_t> GetChildren(const BTNode* node) const;

        // Helper: Get parent nodes (reverse lookup)
        void BuildParentMap(const BehaviorTreeAsset* tree);

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
