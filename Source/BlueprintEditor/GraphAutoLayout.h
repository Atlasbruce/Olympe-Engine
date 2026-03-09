/**
 * @file GraphAutoLayout.h
 * @brief Hierarchical (Sugiyama-inspired) auto-layout for VS task graphs (Phase 6).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Assigns positions to TaskNodeDefinition nodes in a TaskGraphTemplate so that
 * the graph can be read top-to-bottom.  The EntryPoint node is placed at the
 * root (top), and nodes are arranged into layers based on their distance from
 * the entry point in the exec-connection graph.
 *
 * Algorithm summary (Sugiyama-inspired):
 *   1. BFS from the EntryPoint node → assign each node a layer index.
 *   2. Within each layer, order nodes by ascending NodeID (stable).
 *   3. Assign (x, y) screen-space positions using fixed spacing constants.
 *   4. Positions are stored in the Parameters map under "__posX" / "__posY".
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#pragma once

#include <vector>
#include <cstdint>

#include "../TaskSystem/TaskGraphTemplate.h"

namespace Olympe {

// ============================================================================
// GraphAutoLayout
// ============================================================================

/**
 * @class GraphAutoLayout
 * @brief Applies a hierarchical layout to a TaskGraphTemplate in place.
 *
 * Typical usage:
 * @code
 *   GraphAutoLayout::ApplyHierarchicalLayout(myGraph);
 * @endcode
 */
class GraphAutoLayout {
public:

    /// Horizontal gap between columns (pixels).
    static constexpr float SPACING_X = 220.0f;

    /// Vertical gap between rows (pixels).
    static constexpr float SPACING_Y = 130.0f;

    /// X coordinate of the root (EntryPoint) column.
    static constexpr float ORIGIN_X = 100.0f;

    /// Y coordinate of the first row.
    static constexpr float ORIGIN_Y = 100.0f;

    /**
     * @brief Assigns screen-space positions to every node in @p graph.
     *
     * @details
     * Positions are written into each node's Parameters map under the keys
     * "__posX" (float) and "__posY" (float) so they can be consumed by the
     * editor canvas without touching the node's semantic fields.
     *
     * If @p graph has no EntryPoint node, the first node in Nodes is used as
     * the root.  Disconnected (orphan) nodes are placed in an overflow column
     * to the right of the main layout.
     *
     * @param graph  The graph whose nodes will be positioned.
     */
    static void ApplyHierarchicalLayout(TaskGraphTemplate& graph);

    // -----------------------------------------------------------------------
    // Internal helpers exposed for unit-testing
    // -----------------------------------------------------------------------

    /**
     * @brief A single layer in the hierarchical layout.
     */
    struct Layer {
        std::vector<int32_t> NodeIDs;  ///< IDs of nodes assigned to this layer, in order
    };

    /**
     * @brief Builds layers via BFS from the entry point.
     *
     * @param graph   Source graph.
     * @return Vector of layers; layer[0] contains the root node.
     */
    static std::vector<Layer> BuildLayers(const TaskGraphTemplate& graph);

    /**
     * @brief Assigns x/y positions from the layer structure.
     *
     * @param graph   Graph whose nodes will be modified.
     * @param layers  Layer structure produced by BuildLayers().
     */
    static void AssignPositions(TaskGraphTemplate& graph,
                                const std::vector<Layer>& layers);

private:

    /**
     * @brief Writes a float position into a node's Parameters map.
     */
    static void SetPos(TaskNodeDefinition& node, float x, float y);
};

} // namespace Olympe
