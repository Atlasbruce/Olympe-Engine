/**
 * @file VisualScriptNodeRenderer.cpp
 * @brief Implementation of VS node rendering helpers (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant.
 */

#include "VisualScriptNodeRenderer.h"
#include "../system/system_consts.h"
#include "AtomicTaskUIRegistry.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

#include <string>
#include <vector>

namespace Olympe {


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

unsigned int GetExecPinColor()
{
    return SystemColors::EXEC_PIN_COLOR;
}

unsigned int GetDataPinColor(VariableType type)
{
    // All data pins use the same violet color regardless of type
    return SystemColors::DATA_PIN_COLOR;
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
    const std::vector<std::pair<std::string, VariableType>>& dataOutputPins)
{
    (void)nodeID; (void)graphID; // reserved for future per-node UID derivation

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

    // Attribute UIDs use the same scheme as VisualScriptEditorPanel helpers:
    //   nodeUID * 10000 + offset
    //   offset 0–99   → exec-in  (Input)
    //   offset 100–199 → exec-out (Output)
    //   offset 200–299 → data-in  (Input)
    //   offset 300–399 → data-out (Output)

    // Use 2-column layout to align input pins (left) with output pins (right) on the same Y
    ImGui::Columns(2, "node_pins", false);
    ImGui::SetColumnWidth(0, 80.0f);

    // ---- LEFT COLUMN: Input Pins (Exec + Data) ----

    // Exec input pins (left side triangles)
    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Triangle);
        ImGui::Text("%s", execInputPins[i].c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Data input pins (left side circles)  — offset 200–299
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 200 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataInputPins[i].second));
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Circle);
        ImGui::Text("%s", dataInputPins[i].first.c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // ---- RIGHT COLUMN: Output Pins (Exec + Data) ----
    ImGui::NextColumn();

    // Exec output pins (right side triangles) — offset 100–199
    for (size_t i = 0; i < execOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 100 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_TriangleFilled);
        ImGui::Text("%s", execOutputPins[i].c_str());
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
     }

    // Data output pins (right side circles) — offset 300–399
    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 300 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataOutputPins[i].second));
        ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_Circle);
        ImGui::Text("%s", dataOutputPins[i].first.c_str());
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
    }

    ImGui::Columns(1);  // End columns

    ImNodes::EndNode();

    if (isActive)
        ImNodes::PopColorStyle();

    ImNodes::PopColorStyle(); // TitleBarSelected
    ImNodes::PopColorStyle(); // TitleBarHovered
    ImNodes::PopColorStyle(); // TitleBar
}

// ============================================================================
// VisualScriptNodeRenderer::RenderNode — extended overload with inline params
// ============================================================================

void VisualScriptNodeRenderer::RenderNode(
    int                                                      nodeUID,
    int                                                      nodeID,
    int                                                      graphID,
    const TaskNodeDefinition&                                def,
    bool                                                     hasBreakpoint,
    bool                                                     isActive,
    const std::vector<std::string>&                          execInputPins,
    const std::vector<std::string>&                          execOutputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataOutputPins,
    void (*onAddPin)(int nodeID, void* userData),
    void* onAddPinUserData,
    void (*onRemovePin)(int nodeID, int dynamicPinIndex, void* userData),
    void* onRemovePinUserData)
{
    (void)graphID;

    VSNodeStyle style = GetNodeStyle(def.Type);

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
        ImNodes::PushColorStyle(ImNodesCol_NodeOutline, IM_COL32(80, 255, 80, 255));

    ImNodes::BeginNode(nodeUID);

    // Title bar
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(def.NodeName.c_str());
    ImNodes::EndNodeTitleBar();

    // Use 2-column layout to align input pins (left) with output pins (right) on the same Y
    // PINS FIRST for better UX consistency
    ImGui::Columns(2, "node_pins_extended", false);
    ImGui::SetColumnWidth(0, 80.0f);

    // ---- LEFT COLUMN: Input Pins (Exec + Data) ----

    // Exec input pins (left side triangles) — offset 0–99
    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Triangle);
        ImGui::Text("%s", execInputPins[i].c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Data input pins (left side circles) — offset 200–299
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 200 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataInputPins[i].second));
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Circle);
        ImGui::Text("%s", dataInputPins[i].first.c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Phase 24 — Dynamic data pins from conditionRefs (violet, offset 400–499)
    if (def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While)
    {
        for (size_t i = 0; i < def.dynamicPins.size(); ++i)
        {
            int attrID = nodeUID * 10000 + 400 + static_cast<int>(i);
            ImNodes::PushColorStyle(ImNodesCol_Pin, SystemColors::DATA_PIN_COLOR);
            ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Circle);
            const std::string lbl = def.dynamicPins[i].GetDisplayLabel();
            ImGui::Text("%s", lbl.c_str());
            ImNodes::EndInputAttribute();
            ImNodes::PopColorStyle();
        }
    }

    // ---- RIGHT COLUMN: Output Pins (Exec + Data + Dynamic removal button) ----
    ImGui::NextColumn();

    // Determine how many static (non-removable) exec-out pins this node has.
    // Dynamic pins start at index numStaticPins in execOutputPins.
    const bool hasDynamicPins = (def.Type == TaskNodeType::VSSequence ||
                                  def.Type == TaskNodeType::Switch);
    const int numStaticPins = hasDynamicPins
        ? static_cast<int>(execOutputPins.size()) -
          static_cast<int>(def.DynamicExecOutputPins.size())
        : static_cast<int>(execOutputPins.size());

    // Exec output pins (right side triangles) — offset 100–199
    // Dynamic pins render with an inline [-] remove button.
    for (size_t i = 0; i < execOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 100 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());

        // Handle dynamic pin removal button placement
        if (hasDynamicPins && static_cast<int>(i) >= numStaticPins && onRemovePin)
        {
            int dynIdx = static_cast<int>(i) - numStaticPins;

            ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_TriangleFilled);

            ImGui::PushID(nodeUID * 10000 + 5000 + static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(140, 30, 30, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 50, 50, 220));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(230, 80, 80, 240));
            if (ImGui::SmallButton("[-]"))
                onRemovePin(nodeID, dynIdx, onRemovePinUserData);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Remove Execution Output");
            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImNodes::EndOutputAttribute();
        }
        else
        {
            // Regular exec output pin
            ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_TriangleFilled);
            ImGui::Text("%s", execOutputPins[i].c_str());
            ImNodes::EndOutputAttribute();
        }

        ImNodes::PopColorStyle();
    }

    // [+] button — below the last exec-out pin, for VSSequence and Switch
    if (hasDynamicPins && onAddPin)
    {
        ImGui::PushID(nodeUID);
        ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(30, 100, 30, 200));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 160, 50, 220));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(80, 200, 80, 240));
        if (ImGui::SmallButton("[+]"))
            onAddPin(nodeID, onAddPinUserData);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Add Execution Output");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }

    // Data output pins (right side circles) — offset 300–399
    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 300 + static_cast<int>(i);
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataOutputPins[i].second));
        ImNodes::BeginOutputAttribute(attrID, ImNodesPinShape_Circle);
        ImGui::Text("%s", dataOutputPins[i].first.c_str());
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
    }

    ImGui::Columns(1);  // End columns

    // ---- Inline parameter display (AFTER pins for better UX consistency) ----
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:
        {
            if (!def.AtomicTaskID.empty())
            {
                // Get task spec to retrieve parameters
                const TaskSpec* taskSpec = AtomicTaskUIRegistry::Get().GetTaskSpec(def.AtomicTaskID);
                if (taskSpec && !taskSpec->parameters.empty())
                {
                    // Display parameters in format: "paramName: value"
                    for (const auto& param : taskSpec->parameters)
                    {
                        // Get parameter value from def.Parameters
                        std::string paramValue = param.defaultValue;
                        auto paramIt = def.Parameters.find(param.name);
                        if (paramIt != def.Parameters.end() && 
                            paramIt->second.Type == ParameterBindingType::Literal)
                        {
                            paramValue = paramIt->second.LiteralValue.AsString();
                        }

                        // Display as "paramName: value" in light color
                        ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), 
                                         "  %s: %s", param.name.c_str(), paramValue.c_str());
                    }
                }
                else
                {
                    // Fallback if no parameters defined for this task
                    ImGui::TextDisabled("  %s", def.AtomicTaskID.c_str());
                }
            }
            break;
        }

        case TaskNodeType::Delay:
            ImGui::TextDisabled("  %.2f s", def.DelaySeconds);
            break;

        case TaskNodeType::GetBBValue:
        case TaskNodeType::SetBBValue:
            if (!def.BBKey.empty())
                ImGui::TextDisabled("  %s", def.BBKey.c_str());
            break;

        case TaskNodeType::Branch:
        case TaskNodeType::While:
        {
            // Phase 24 — render conditionRefs in green if available
            if (!def.conditionRefs.empty())
            {
                const ImVec4 condColor(0.0f, 1.0f, 0.0f, 1.0f);
                for (size_t ci = 0; ci < def.conditionRefs.size(); ++ci)
                {
                    const NodeConditionRef& ref = def.conditionRefs[ci];
                    const char* opLabel;
                    if (ci == 0)
                        opLabel = "   ";
                    else if (ref.logicalOp == LogicalOp::And)
                        opLabel = "And";
                    else
                        opLabel = "Or ";
                    ImGui::PushStyleColor(ImGuiCol_Text, condColor);
                    ImGui::Text("  %s %s", opLabel, ref.presetID.c_str());
                    ImGui::PopStyleColor();
                }
            }
            else if (!def.ConditionID.empty())
            {
                // Fallback: legacy ConditionID (Phase 23 / pre-Phase 24)
                ImGui::TextDisabled("  %s", def.ConditionID.c_str());
            }
            // Phase 24 — render dynamic pins in yellow (Section 4 preview)
            if (!def.dynamicPins.empty())
            {
                ImGui::Separator();
                const ImVec4 pinColor(1.0f, 0.843f, 0.0f, 1.0f);
                for (const auto& pin : def.dynamicPins)
                {
                    const std::string lbl = pin.GetDisplayLabel();
                    ImGui::TextColored(pinColor, "  %s", lbl.c_str());
                }
            }
            break;
        }

        case TaskNodeType::SubGraph:
        {
            if (!def.SubGraphPath.empty())
            {
                // Extract basename without path or extension
                const std::string& p = def.SubGraphPath;
                size_t slashPos = p.find_last_of("/\\");
                std::string base = (slashPos != std::string::npos)
                                   ? p.substr(slashPos + 1)
                                   : p;
                size_t dotPos = base.rfind('.');
                if (dotPos != std::string::npos)
                    base = base.substr(0, dotPos);
                ImGui::TextDisabled("  %s", base.c_str());
            }
            break;
        }

        case TaskNodeType::MathOp:
            if (!def.MathOperator.empty())
                ImGui::TextDisabled("  %s", def.MathOperator.c_str());
            break;

        default:
            break;
    }

    ImNodes::EndNode();

    if (isActive)
        ImNodes::PopColorStyle();

    ImNodes::PopColorStyle(); // TitleBarSelected
    ImNodes::PopColorStyle(); // TitleBarHovered
    ImNodes::PopColorStyle(); // TitleBar
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
