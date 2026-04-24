/**
 * @file AIEditorNodeRenderer.cpp
 * @brief Implementation of AIEditorNodeRenderer (DEPRECATED - Phase 50.3)
 * @author Olympe Engine
 * @date 2026-02-18
 * 
 * NOTE: This file contains deprecated annotation rendering logic.
 * Reimplementation scheduled after graph rendering works.
 */

#include "AIEditorNodeRenderer.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imnodes/imnodes.h"
#include <cmath>

namespace Olympe {
namespace AI {

// Type aliases for backward compatibility
using NodeId = Olympe::NodeGraphTypes::NodeId;
using NodeData = Olympe::NodeGraphTypes::NodeData;

// TODO: Reimplement with modern NodeGraphTypes schema
// Current implementation commented - annotation system removed

// ============================================================================
// Node Rendering (DEPRECATED)
// ============================================================================

void AIEditorNodeRenderer::RenderNode(
    const NodeData& nodeData,
    bool isSelected,
    bool isExecuting,
    const void* annotation)
{
    // TODO: Reimplement RenderNode with modern schema
    // - Remove NodeAnnotation dependency (removed from schema)
    // - Integrate with ImNodes rendering pipeline
    // - Support breakpoint visualization (separate system)

    // Temporary no-op to unblock compilation
    (void)nodeData;
    (void)isSelected;
    (void)isExecuting;
    (void)annotation;
}

void AIEditorNodeRenderer::RenderNodeTooltip(const NodeData& nodeData)
{
    // TODO: Reimplement tooltip with modern schema
    // - Remove NodeAnnotation dependency
    // - Render basic node info only
    (void)nodeData;
}

int AIEditorNodeRenderer::GetInputPinId(NodeId nodeId)
{
    // TODO: Pin ID calculation needs reimplementation
    // - Currently deprecated (annotation system removed)
    // - Placeholder returns base pin ID
    return static_cast<int>(nodeId.value) * 1000 + 1;
}

int AIEditorNodeRenderer::GetOutputPinId(NodeId nodeId)
{
    // TODO: Pin ID calculation needs reimplementation
    // - Currently deprecated (annotation system removed)
    // - Placeholder returns base pin ID
    return static_cast<int>(nodeId.value) * 1000 + 2;
}

int AIEditorNodeRenderer::GetChildPinId(NodeId nodeId, int childIndex)
{
    // TODO: Pin ID calculation needs reimplementation
    // - Currently deprecated (annotation system removed)
    // - Placeholder returns base pin ID
    return static_cast<int>(nodeId.value) * 1000 + 100 + childIndex;
}

} // namespace AI
} // namespace Olympe
