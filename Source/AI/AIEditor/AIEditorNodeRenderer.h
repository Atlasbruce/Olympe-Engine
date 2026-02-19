/**
 * @file AIEditorNodeRenderer.h
 * @brief Node renderer for AI Editor with ImNodes
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Converts NodeGraphCore NodeData to ImNodes rendering with BT styling.
 * Phase 2.0: Supports breakpoint and comment annotation rendering.
 */

#pragma once

#include "../../NodeGraphCore/NodeGraphCore.h"
#include "../../NodeGraphCore/NodeAnnotations.h"
#include "../AIGraphPlugin_BT/BTNodeRegistry.h"

namespace Olympe {
namespace AI {

/**
 * @class AIEditorNodeRenderer
 * @brief Renders nodes with ImNodes using BT styling
 */
class AIEditorNodeRenderer {
public:
    /**
     * @brief Render a single node with ImNodes
     * @param nodeData Node data to render
     * @param isSelected Whether node is selected
     * @param isExecuting Whether node is currently executing (runtime debug)
     * @param annotation Optional annotation for this node (may be nullptr)
     */
    static void RenderNode(
        const NodeGraph::NodeData& nodeData,
        bool isSelected = false,
        bool isExecuting = false,
        const NodeGraph::NodeAnnotation* annotation = nullptr
    );
    
    /**
     * @brief Render node tooltip
     * @param nodeData Node data
     */
    static void RenderNodeTooltip(const NodeGraph::NodeData& nodeData);
    
    /**
     * @brief Get pin ID for node input
     * @param nodeId Node ID
     * @return Pin ID for input
     */
    static int GetInputPinId(NodeGraph::NodeId nodeId);
    
    /**
     * @brief Get pin ID for node output
     * @param nodeId Node ID
     * @return Pin ID for output
     */
    static int GetOutputPinId(NodeGraph::NodeId nodeId);
    
    /**
     * @brief Get pin ID for child connection
     * @param nodeId Node ID
     * @param childIndex Child index
     * @return Pin ID for child
     */
    static int GetChildPinId(NodeGraph::NodeId nodeId, int childIndex);
};

} // namespace AI
} // namespace Olympe
