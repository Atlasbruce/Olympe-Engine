/**
 * @file VisualScriptNodeRenderer.cpp
 * @brief Implementation of VS node rendering helpers (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant.
 */

#include "VisualScriptNodeRenderer.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

#include <string>
#include <vector>

namespace Olympe {

// ============================================================================
// Colour helpers
// ============================================================================

VSNodeStyle GetNodeStyle(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::EntryPoint:  return VSNodeStyle::EntryPoint;
        case TaskNodeType::Branch:
        case TaskNodeType::VSSequence:
        case TaskNodeType::While:
        case TaskNodeType::ForEach:
        case TaskNodeType::DoOnce:
        case TaskNodeType::Switch:      return VSNodeStyle::FlowControl;
        case TaskNodeType::AtomicTask:  return VSNodeStyle::Action;
        case TaskNodeType::GetBBValue:
        case TaskNodeType::SetBBValue:
        case TaskNodeType::MathOp:      return VSNodeStyle::Data;
        case TaskNodeType::SubGraph:    return VSNodeStyle::SubGraph;
        case TaskNodeType::Delay:       return VSNodeStyle::Delay;
        default:                        return VSNodeStyle::Action;
    }
}

unsigned int GetNodeTitleColor(VSNodeStyle style)
{
    switch (style)
    {
        case VSNodeStyle::EntryPoint:   return IM_COL32(30, 140, 60, 255);   // Green
        case VSNodeStyle::FlowControl:  return IM_COL32(40, 80, 160, 255);   // Blue
        case VSNodeStyle::Action:       return IM_COL32(160, 90, 30, 255);   // Orange
        case VSNodeStyle::Data:         return IM_COL32(100, 40, 140, 255);  // Purple
        case VSNodeStyle::SubGraph:     return IM_COL32(30, 120, 120, 255);  // Teal
        case VSNodeStyle::Delay:        return IM_COL32(160, 140, 20, 255);  // Yellow
        default:                        return IM_COL32(80, 80, 80, 255);
    }
}

unsigned int GetNodeTitleHoveredColor(VSNodeStyle style)
{
    switch (style)
    {
        case VSNodeStyle::EntryPoint:   return IM_COL32(50, 180, 80, 255);
        case VSNodeStyle::FlowControl:  return IM_COL32(60, 110, 200, 255);
        case VSNodeStyle::Action:       return IM_COL32(200, 120, 50, 255);
        case VSNodeStyle::Data:         return IM_COL32(130, 60, 180, 255);
        case VSNodeStyle::SubGraph:     return IM_COL32(50, 160, 160, 255);
        case VSNodeStyle::Delay:        return IM_COL32(200, 180, 40, 255);
        default:                        return IM_COL32(110, 110, 110, 255);
    }
}

unsigned int GetDataPinColor(VariableType type)
{
    switch (type)
    {
        case VariableType::Float:    return IM_COL32(60, 120, 220, 255);  // Blue
        case VariableType::Int:      return IM_COL32(60, 180, 60, 255);   // Green
        case VariableType::String:   return IM_COL32(220, 200, 40, 255);  // Yellow
        case VariableType::Vector:   return IM_COL32(60, 200, 200, 255);  // Cyan
        case VariableType::Bool:     return IM_COL32(200, 40, 200, 255);  // Magenta
        case VariableType::EntityID: return IM_COL32(220, 40, 40, 255);   // Red
        default:                     return IM_COL32(160, 160, 160, 255); // Gray
    }
}

const char* GetNodeTypeLabel(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::EntryPoint:  return "EntryPoint";
        case TaskNodeType::Branch:      return "Branch";
        case TaskNodeType::VSSequence:  return "Sequence";
        case TaskNodeType::While:       return "While";
        case TaskNodeType::ForEach:     return "ForEach";
        case TaskNodeType::DoOnce:      return "DoOnce";
        case TaskNodeType::Switch:      return "Switch";
        case TaskNodeType::AtomicTask:  return "AtomicTask";
        case TaskNodeType::GetBBValue:  return "GetBBValue";
        case TaskNodeType::SetBBValue:  return "SetBBValue";
        case TaskNodeType::MathOp:      return "MathOp";
        case TaskNodeType::SubGraph:    return "SubGraph";
        case TaskNodeType::Delay:       return "Delay";
        case TaskNodeType::Sequence:    return "Sequence(BT)";
        case TaskNodeType::Root:        return "Root";
        default:                        return "Unknown";
    }
}

const char* GetVariableTypeLabel(VariableType type)
{
    switch (type)
    {
        case VariableType::Bool:     return "Bool";
        case VariableType::Int:      return "Int";
        case VariableType::Float:    return "Float";
        case VariableType::Vector:   return "Vector";
        case VariableType::EntityID: return "EntityID";
        case VariableType::String:   return "String";
        default:                     return "None";
    }
}

// ============================================================================
// VisualScriptNodeRenderer
// ============================================================================

void VisualScriptNodeRenderer::RenderNode(
    int                                           nodeUID,
    int                                           nodeID,
    int                                           graphID,
    const std::string&                            nodeName,
    TaskNodeType                                  type,
    bool                                          hasBreakpoint,
    bool                                          isActive,
    const std::vector<std::string>&               execInputPins,
    const std::vector<std::string>&               execOutputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataOutputPins,
    const std::string&                            tooltip)
{
    (void)nodeID; (void)graphID; // used for future per-node UID derivation

    VSNodeStyle style = GetNodeStyle(type);

    // Apply colour styles
    unsigned int titleCol        = hasBreakpoint
                                   ? IM_COL32(200, 30, 30, 255)
                                   : GetNodeTitleColor(style);
    unsigned int titleHoveredCol = hasBreakpoint
                                   ? IM_COL32(240, 50, 50, 255)
                                   : GetNodeTitleHoveredColor(style);
    unsigned int titleSelectedCol = IM_COL32(255, 220, 50, 255);

    ImNodes::PushColorStyle(ImNodesCol_TitleBar,         titleCol);
    ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered,  titleHoveredCol);
    ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, titleSelectedCol);

    if (isActive)
    {
        ImNodes::PushColorStyle(ImNodesCol_NodeOutline, IM_COL32(80, 255, 80, 255));
    }

    ImNodes::BeginNode(nodeUID);

    // Title bar
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(nodeName.c_str());
    ImNodes::EndNodeTitleBar();

    // Exec input pins (left side triangles)
    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        // Each exec pin gets a unique attribute ID derived from nodeUID
        int attrID = nodeUID * 1000 + static_cast<int>(i);
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Triangle);
        ImGui::Text("%s", execInputPins[i].c_str());
        ImNodes::EndInputAttribute();
    }

    // Data input pins (left side circles)
    int dataPinOffset = 100; // offset to avoid collision with exec pins
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        int attrID = nodeUID * 1000 + dataPinOffset + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataInputPins[i].second));
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Circle);
        ImGui::Text("%s", dataInputPins[i].first.c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Exec output pins (right side triangles)
    int execOutOffset = 200;
    for (size_t i = 0; i < execOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 1000 + execOutOffset + static_cast<int>(i);
        ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_TriangleFilled);
        ImGui::Indent(60.0f);
        ImGui::Text("%s", execOutputPins[i].c_str());
        ImGui::Unindent(60.0f);
        ImNodes::EndOutputAttribute();
    }

    // Data output pins (right side circles)
    int dataOutOffset = 300;
    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 1000 + dataOutOffset + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataOutputPins[i].second));
        ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_CircleFilled);
        ImGui::Indent(60.0f);
        ImGui::Text("%s", dataOutputPins[i].first.c_str());
        ImGui::Unindent(60.0f);
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
    }

    ImNodes::EndNode();

    if (isActive)
        ImNodes::PopColorStyle();

    ImNodes::PopColorStyle(); // TitleBarSelected
    ImNodes::PopColorStyle(); // TitleBarHovered
    ImNodes::PopColorStyle(); // TitleBar

    // Hover tooltip
    int hoveredNode = -1;
    if (!tooltip.empty() && ImNodes::IsNodeHovered(&hoveredNode) && hoveredNode == nodeUID)
    {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(tooltip.c_str());
        ImGui::EndTooltip();
    }
}

void VisualScriptNodeRenderer::RenderBreakpointIndicator(int nodeUID)
{
    ImVec2 nodePos = ImNodes::GetNodeEditorSpacePos(nodeUID);
    ImVec2 screenPos = ImGui::GetWindowPos();
    ImVec2 circlePos;
    circlePos.x = screenPos.x + nodePos.x - 12.0f;
    circlePos.y = screenPos.y + nodePos.y + 10.0f;

    ImGui::GetWindowDrawList()->AddCircleFilled(
        circlePos, 7.0f, IM_COL32(255, 0, 0, 255));
    ImGui::GetWindowDrawList()->AddCircle(
        circlePos, 7.0f, IM_COL32(255, 120, 120, 255), 12, 1.5f);
}

void VisualScriptNodeRenderer::RenderActiveNodeGlow(int nodeUID)
{
    ImVec2 nodePos  = ImNodes::GetNodeEditorSpacePos(nodeUID);
    ImVec2 nodeDims = ImNodes::GetNodeDimensions(nodeUID);
    ImVec2 screenPos = ImGui::GetWindowPos();

    ImVec2 min, max;
    min.x = screenPos.x + nodePos.x - 4.0f;
    min.y = screenPos.y + nodePos.y - 4.0f;
    max.x = min.x + nodeDims.x + 8.0f;
    max.y = min.y + nodeDims.y + 8.0f;

    ImGui::GetWindowDrawList()->AddRect(
        min, max, IM_COL32(80, 255, 80, 200), 6.0f, 0, 3.0f);
}

} // namespace Olympe
