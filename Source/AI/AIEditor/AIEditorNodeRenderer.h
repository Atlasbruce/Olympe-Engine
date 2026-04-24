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
// TODO: NodeAnnotations removed in modern schema - reimplement annotation system post-rendering
// #include "../../NodeGraphCore/NodeAnnotations.h"
#include "../AIGraphPlugin_BT/BTNodeRegistry.h"

namespace Olympe {
namespace AI {

// Type aliases for backward compatibility
using NodeId = Olympe::NodeGraphTypes::NodeId;
using NodeData = Olympe::NodeGraphTypes::NodeData;

/**
 * @class AIEditorNodeRenderer
 * @brief Renders nodes with ImNodes using BT styling
 * 
 * NOTE: Phase 50.3 - Annotation system (breakpoint, comments) deprecated.
 * Reimplementation scheduled after graph rendering works.
 */
class AIEditorNodeRenderer {
public:
    /**
     * @brief Render a single node with ImNodes
     * @param nodeData Node data to render
     * @param isSelected Whether node is selected
     * @param isExecuting Whether node is currently executing (runtime debug)
     * @param annotation DEPRECATED - annotation system removed
     */
    static void RenderNode(
        const NodeData& nodeData,  // Using type alias from AIEditorGUI namespace
        bool isSelected = false,
        bool isExecuting = false,
        const void* annotation = nullptr  // Deprecated, converted to void*
    );

    /**
     * @brief Render node tooltip (DEPRECATED)
     * @param nodeData Node data
     */
    static void RenderNodeTooltip(const NodeData& nodeData);

    /**
     * @brief Get pin ID for node input (DEPRECATED)
     * @param nodeId Node ID
     * @return Pin ID for input
     */
    static int GetInputPinId(NodeId nodeId);

    /**
     * @brief Get pin ID for node output
     * @param nodeId Node ID
     * @return Pin ID for output
     */
    static int GetOutputPinId(NodeId nodeId);

    /**
     * @brief Get pin ID for child connection
     * @param nodeId Node ID
     * @param childIndex Child index
     * @return Pin ID for child
     */
    static int GetChildPinId(NodeId nodeId, int childIndex);
};

} // namespace AI
} // namespace Olympe
