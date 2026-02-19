/**
 * @file AIEditorNodeRenderer.cpp
 * @brief Implementation of AIEditorNodeRenderer
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "AIEditorNodeRenderer.h"
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imnodes/imnodes.h"

namespace Olympe {
namespace AI {

// ============================================================================
// Node Rendering
// ============================================================================

void AIEditorNodeRenderer::RenderNode(
    const NodeGraph::NodeData& nodeData,
    bool isSelected,
    bool isExecuting,
    const NodeGraph::NodeAnnotation* annotation)
{
    BTNodeRegistry& registry = BTNodeRegistry::Get();
    const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(nodeData.type);
    
    if (typeInfo == nullptr) {
        return;
    }
    
    // Begin node
    int iNodeId = static_cast<int>(nodeData.id.value);
    ImNodes::BeginNode(iNodeId);
    
    // Title bar with color
    ImNodes::BeginNodeTitleBar();
    uint32_t color = typeInfo->color;
    
    // Highlight if executing
    if (isExecuting) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    }
    
    ImGui::TextColored(
        ImVec4(
            ((color >> 16) & 0xFF) / 255.0f,
            ((color >> 8) & 0xFF) / 255.0f,
            ((color >> 0) & 0xFF) / 255.0f,
            ((color >> 24) & 0xFF) / 255.0f
        ),
        "%s %s", typeInfo->icon.c_str(), nodeData.name.c_str()
    );
    
    if (isExecuting) {
        ImGui::PopStyleColor();
    }
    
    // Phase 2.0 - Breakpoint indicator (red dot) and comment badge
    if (annotation != nullptr)
    {
        if (annotation->hasBreakpoint)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.15f, 0.15f, 1.0f), "[B]");
        }
        if (!annotation->comment.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "[C]");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", annotation->comment.c_str());
            }
        }
    }
    
    ImNodes::EndNodeTitleBar();
    
    // Input pin (for composites and decorators)
    if (typeInfo->category == BTNodeCategory::Composite ||
        typeInfo->category == BTNodeCategory::Decorator) {
        int inputPinId = GetInputPinId(nodeData.id);
        ImNodes::BeginInputAttribute(inputPinId);
        ImGui::Text("In");
        ImNodes::EndInputAttribute();
    }
    
    // Parameters
    for (auto it = nodeData.parameters.begin(); it != nodeData.parameters.end(); ++it) {
        ImGui::Text("%s: %s", it->first.c_str(), it->second.c_str());
    }
    
    // Output pins (for nodes that can have children)
    if (typeInfo->category == BTNodeCategory::Composite ||
        typeInfo->category == BTNodeCategory::Decorator) {
        int outputPinId = GetOutputPinId(nodeData.id);
        ImNodes::BeginOutputAttribute(outputPinId);
        ImGui::Text("Out");
        ImNodes::EndOutputAttribute();
    }
    
    ImNodes::EndNode();
    
    // Set node position
    ImNodes::SetNodeGridSpacePos(iNodeId, ImVec2(nodeData.position.x, nodeData.position.y));
    
    // Tooltip on hover
    int hoveredNode = -1;
    if (ImNodes::IsNodeHovered(&hoveredNode) && hoveredNode == iNodeId) {
        RenderNodeTooltip(nodeData);
    }
    
    (void)isSelected; // Unused for now
}

void AIEditorNodeRenderer::RenderNodeTooltip(const NodeGraph::NodeData& nodeData)
{
    BTNodeRegistry& registry = BTNodeRegistry::Get();
    const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(nodeData.type);
    
    if (typeInfo != nullptr) {
        ImGui::SetTooltip("%s", typeInfo->description.c_str());
    }
}

// ============================================================================
// Pin ID Helpers
// ============================================================================

int AIEditorNodeRenderer::GetInputPinId(NodeGraph::NodeId nodeId)
{
    return static_cast<int>(nodeId.value) * 1000;
}

int AIEditorNodeRenderer::GetOutputPinId(NodeGraph::NodeId nodeId)
{
    return static_cast<int>(nodeId.value) * 1000 + 1;
}

int AIEditorNodeRenderer::GetChildPinId(NodeGraph::NodeId nodeId, int childIndex)
{
    return static_cast<int>(nodeId.value) * 1000 + 10 + childIndex;
}

} // namespace AI
} // namespace Olympe
