/**
 * @file ILayoutEngine.h
 * @brief Abstract interface for graph layout algorithms
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Provides interface for implementing different layout algorithms
 * (hierarchical, force-directed, etc.) for node graphs.
 */

#pragma once

#include "NodeGraphCore.h"
#include "GraphDocument.h"
#include <string>
#include <vector>

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Layout Structures
// ============================================================================

/**
 * @struct NodeLayout
 * @brief Layout information for a node
 */
struct NodeLayout {
    NodeId nodeId;
    Vector2 position;
    float width = 200.0f;
    float height = 100.0f;
    int layer = 0;
    int orderInLayer = 0;
};

/**
 * @struct LayoutParams
 * @brief Parameters for layout algorithms
 */
struct LayoutParams {
    float nodeSpacingX = 220.0f;
    float nodeSpacingY = 140.0f;
    float minNodeDistance = 180.0f;
    float maxNodeDistance = 300.0f;
    std::string direction = "TopToBottom";
    float zoomFactor = 1.0f;
};

// ============================================================================
// Layout Engine Interface
// ============================================================================

/**
 * @class ILayoutEngine
 * @brief Abstract interface for layout engines
 */
class ILayoutEngine {
public:
    virtual ~ILayoutEngine() = default;
    
    /**
     * @brief Compute layout for all nodes in graph
     * @param graph Graph document
     * @param params Layout parameters
     * @return Vector of node layouts
     */
    virtual std::vector<NodeLayout> ComputeLayout(
        const GraphDocument* graph,
        const LayoutParams& params
    ) = 0;
    
    /**
     * @brief Get name of layout engine
     * @return Engine name
     */
    virtual std::string GetName() const = 0;
};

} // namespace NodeGraph
} // namespace Olympe
