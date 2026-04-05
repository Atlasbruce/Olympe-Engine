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
#include "ConditionRef.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"

#include <string>
#include <vector>
#include <unordered_set>

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
// Helper: Build condition expression display string
// ============================================================================

/**
 * @brief Builds a display string for a ConditionRef operand.
 * 
 * Format examples:
 * - Variable mode: "[health]"
 * - Const mode: "[50]"
 * - Pin mode: "[pin]"
 */
static std::string GetOperandDisplayString(const OperandRef& operand)
{
    std::string result = "[";

    if (operand.mode == OperandRef::Mode::Variable)
    {
        result += operand.variableName;
    }
    else if (operand.mode == OperandRef::Mode::Const)
    {
        result += operand.constValue;
    }
    else if (operand.mode == OperandRef::Mode::Pin)
    {
        result += "pin";
    }

    result += "]";
    return result;
}

/**
 * @brief Builds the complete condition expression string.
 * 
 * Example: "[health] > [50]"
 */
static std::string BuildConditionExpressionString(const ConditionRef& condition)
{
    std::string left = GetOperandDisplayString(condition.leftOperand);
    std::string right = GetOperandDisplayString(condition.rightOperand);

    return left + " " + condition.operatorStr + " " + right;
}

// ============================================================================
// Phase 26-B: Node Sizing Helpers (with invisible spacer approach)
// ============================================================================

/**
 * @brief Calculates the width of a Switch case output pin label.
 *
 * Format: "Case_N [customLabel(value)]" or "Case_N (value)" or "Case_N"
 * This includes the full decorated label that might be wider than just the pin name.
 */
static float CalculateSwitchCaseLabelWidth(
    const std::string& baseName,
    const SwitchCaseDefinition& caseData)
{
    std::string displayLabel = baseName;
    if (!caseData.customLabel.empty() && !caseData.value.empty())
    {
        displayLabel = baseName + " [" + caseData.customLabel + "(" + caseData.value + ")]";
    }
    else if (!caseData.value.empty())
    {
        displayLabel = baseName + " (" + caseData.value + ")";
    }

    ImVec2 textSize = ImGui::CalcTextSize(displayLabel.c_str());
    return textSize.x;
}

/**
 * @brief Calculates the minimum width required for a node to display all content.
 *
 * Phase 26-B: Calculate node frame width needed to accommodate all pin labels
 * without truncation. Uses an invisible spacer button to force ImNodes to size
 * the node frame appropriately.
 *
 * @return Width in pixels needed for the node (minimum guaranteed width)
 */
static float CalculateNodeMinimumWidth(
    const TaskNodeDefinition&                                def,
    const std::vector<std::string>&                          execInputPins,
    const std::vector<std::string>&                          execOutputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataInputPins,
    const std::vector<std::pair<std::string, VariableType>>& dataOutputPins)
{
    const float PIN_ICON_WIDTH = 10.0f;
    const float TEXT_SPACING = 4.0f;
    const float COLUMN_SEPARATOR = 20.0f;
    const float MIN_COLUMN_WIDTH = 60.0f;
    const float MIN_NODE_WIDTH = 150.0f;

    // Measure left column (exec input + data input + dynamic pins)
    float leftMaxWidth = 0.0f;

    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(execInputPins[i].c_str());
        float totalWidth = PIN_ICON_WIDTH + TEXT_SPACING + textSize.x + TEXT_SPACING;
        if (totalWidth > leftMaxWidth) leftMaxWidth = totalWidth;
    }

    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(dataInputPins[i].first.c_str());
        float totalWidth = PIN_ICON_WIDTH + TEXT_SPACING + textSize.x + TEXT_SPACING;
        if (totalWidth > leftMaxWidth) leftMaxWidth = totalWidth;
    }

    // Dynamic pins (Branch/While conditions)
    if (def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While)
    {
        for (size_t i = 0; i < def.dynamicPins.size(); ++i)
        {
            std::string lbl = def.dynamicPins[i].GetDisplayLabel();
            ImVec2 textSize = ImGui::CalcTextSize(lbl.c_str());
            float totalWidth = PIN_ICON_WIDTH + TEXT_SPACING + textSize.x + TEXT_SPACING;
            if (totalWidth > leftMaxWidth) leftMaxWidth = totalWidth;
        }
    }

    leftMaxWidth = (leftMaxWidth > 0.0f) ? leftMaxWidth : MIN_COLUMN_WIDTH;

    // Measure right column (exec output + data output)
    float rightMaxWidth = 0.0f;

    for (size_t i = 0; i < execOutputPins.size(); ++i)
    {
        float labelWidth = 0.0f;

        // For Switch nodes, measure the full decorated label
        if (def.Type == TaskNodeType::Switch && i < def.switchCases.size())
        {
            labelWidth = CalculateSwitchCaseLabelWidth(execOutputPins[i], def.switchCases[i]);
        }
        else
        {
            ImVec2 textSize = ImGui::CalcTextSize(execOutputPins[i].c_str());
            labelWidth = textSize.x;
        }

        // Add pin icon and spacing
        float totalWidth = PIN_ICON_WIDTH + TEXT_SPACING + labelWidth + TEXT_SPACING;

        // Add space for remove button if this is a dynamic pin
        const bool hasDynamicPins = (def.Type == TaskNodeType::VSSequence ||
                                     def.Type == TaskNodeType::Switch);
        const int numStaticPins = hasDynamicPins
            ? static_cast<int>(execOutputPins.size()) -
              static_cast<int>(def.DynamicExecOutputPins.size())
            : static_cast<int>(execOutputPins.size());

        if (hasDynamicPins && static_cast<int>(i) >= numStaticPins)
        {
            totalWidth += 35.0f;  // Space for [-] button
        }

        if (totalWidth > rightMaxWidth) rightMaxWidth = totalWidth;
    }

    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(dataOutputPins[i].first.c_str());
        float totalWidth = PIN_ICON_WIDTH + TEXT_SPACING + textSize.x + TEXT_SPACING;
        if (totalWidth > rightMaxWidth) rightMaxWidth = totalWidth;
    }

    rightMaxWidth = (rightMaxWidth > 0.0f) ? rightMaxWidth : MIN_COLUMN_WIDTH;

    // Total node width: left column + separator + right column
    float requiredContentWidth = leftMaxWidth + COLUMN_SEPARATOR + rightMaxWidth;

    // Also consider title width
    ImVec2 titleSize = ImGui::CalcTextSize(def.NodeName.c_str());
    float titleWidth = titleSize.x + 20.0f;  // +20 for padding/icons in title bar

    // Node width = max(title, content)
    float minimumNodeWidth = (titleWidth > requiredContentWidth) ? titleWidth : requiredContentWidth;

    // Enforce minimum
    if (minimumNodeWidth < MIN_NODE_WIDTH)
        minimumNodeWidth = MIN_NODE_WIDTH;

    return minimumNodeWidth;
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
    const std::unordered_set<int>&                connectedAttrIDs)
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
    // Keep title color consistent when selected (don't change to yellow)
    unsigned int titleSelectedCol = titleCol;

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

    // Phase 26-B: Add invisible spacer to force node width adaptation
    // This makes ImNodes size the node frame to accommodate all pin labels
    float minNodeWidth = 150.0f;  // Minimum baseline

    // Calculate width from title
    ImVec2 titleSize = ImGui::CalcTextSize(nodeName.c_str());
    float titleWidth = titleSize.x + 20.0f;
    if (titleWidth > minNodeWidth) minNodeWidth = titleWidth;

    // Calculate width from pins (simplified for basic overload)
    const float PIN_ICON_WIDTH = 10.0f;
    const float TEXT_SPACING = 4.0f;
    float maxPinLabelWidth = 0.0f;

    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(execInputPins[i].c_str());
        if (textSize.x > maxPinLabelWidth) maxPinLabelWidth = textSize.x;
    }
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(dataInputPins[i].first.c_str());
        if (textSize.x > maxPinLabelWidth) maxPinLabelWidth = textSize.x;
    }
    for (size_t i = 0; i < execOutputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(execOutputPins[i].c_str());
        if (textSize.x > maxPinLabelWidth) maxPinLabelWidth = textSize.x;
    }
    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        ImVec2 textSize = ImGui::CalcTextSize(dataOutputPins[i].first.c_str());
        if (textSize.x > maxPinLabelWidth) maxPinLabelWidth = textSize.x;
    }

    // Add column spacing and icon width
    float contentWidth = (PIN_ICON_WIDTH + TEXT_SPACING) * 2 + maxPinLabelWidth * 2 + 20.0f;
    if (contentWidth > minNodeWidth) minNodeWidth = contentWidth;

    // Render invisible spacer to force node width
    ImGui::InvisibleButton("##node_width_spacer", ImVec2(minNodeWidth - 12.0f, 1.0f));

    // Attribute UIDs use the same scheme as VisualScriptEditorPanel helpers:
    //   nodeUID * 10000 + offset
    //   offset 0–99   -> exec-in  (Input)
    //   offset 100–199 -> exec-out (Output)
    //   offset 200–299 -> data-in  (Input)
    //   offset 300–399 -> data-out (Output)

    // Use 2-column layout to align input pins (left) with output pins (right) on the same Y
    ImGui::Columns(2, "node_pins", false);
    ImGui::SetColumnWidth(0, 80.0f);

    // ---- LEFT COLUMN: Input Pins (Exec + Data) ----

    // Exec input pins (left side triangles)
    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + static_cast<int>(i);
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginInputAttribute(attrID, connected ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle);
        ImGui::Text("%s", execInputPins[i].c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Data input pins (left side circles)  — offset 200–299
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 200 + static_cast<int>(i);
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataInputPins[i].second));
        ImNodes::BeginInputAttribute(attrID, connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle);
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
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginOutputAttribute(attrID, connected ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle);
        ImGui::Text("%s", execOutputPins[i].c_str());
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
     }

    // Data output pins (right side circles) — offset 300–399
    for (size_t i = 0; i < dataOutputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 300 + static_cast<int>(i);
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataOutputPins[i].second));
        ImNodes::BeginOutputAttribute(attrID, connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle);
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
    void* onRemovePinUserData,
    const std::unordered_set<int>& connectedAttrIDs)
{
    (void)graphID;

    VSNodeStyle style = GetNodeStyle(def.Type);

    unsigned int titleCol        = hasBreakpoint
                                   ? IM_COL32(200, 30, 30, 255)
                                   : GetNodeTitleColor(style);
    unsigned int titleHoveredCol = hasBreakpoint
                                   ? IM_COL32(240, 50, 50, 255)
                                   : GetNodeTitleHoveredColor(style);
    // Keep title color consistent when selected (don't change to yellow)
    unsigned int titleSelectedCol = titleCol;

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

    // Phase 26-B: Add invisible spacer to force node width adaptation
    // This makes ImNodes size the node frame to accommodate all pin labels,
    // including Switch case decorations like "[Idle(100)]"
    float minNodeWidth = CalculateNodeMinimumWidth(
        def,
        execInputPins,
        execOutputPins,
        dataInputPins,
        dataOutputPins);

    // Render invisible spacer to force node width
    ImGui::InvisibleButton("##node_width_spacer", ImVec2(minNodeWidth - 12.0f, 1.0f));

    // Use 2-column layout to align input pins (left) with output pins (right) on the same Y
    // PINS FIRST for better UX consistency
    ImGui::Columns(2, "node_pins_extended", false);
    ImGui::SetColumnWidth(0, 80.0f);

    // ---- LEFT COLUMN: Input Pins (Exec + Data) ----

    // Exec input pins (left side triangles) — offset 0–99
    for (size_t i = 0; i < execInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + static_cast<int>(i);
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());
        ImNodes::BeginInputAttribute(attrID, connected ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle);
        ImGui::Text("%s", execInputPins[i].c_str());
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // Data input pins (left side circles) — offset 200–299
    for (size_t i = 0; i < dataInputPins.size(); ++i)
    {
        int attrID = nodeUID * 10000 + 200 + static_cast<int>(i);
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataInputPins[i].second));
        ImNodes::BeginInputAttribute(attrID, connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle);
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
            bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
            ImNodes::PushColorStyle(ImNodesCol_Pin, SystemColors::DATA_PIN_COLOR);
            ImNodes::BeginInputAttribute(attrID, connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle);
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
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin, GetExecPinColor());

        // Handle dynamic pin removal button placement
        if (hasDynamicPins && static_cast<int>(i) >= numStaticPins && onRemovePin)
        {
            int dynIdx = static_cast<int>(i) - numStaticPins;

            ImNodes::BeginOutputAttribute(attrID, connected ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle);

            // Phase 3 FIX: For Switch nodes, display the label with match value
            // even for dynamic pins with remove button
            std::string displayLabel = execOutputPins[i];
            if (def.Type == TaskNodeType::Switch && i < def.switchCases.size())
            {
                const SwitchCaseDefinition& caseData = def.switchCases[i];
                if (!caseData.customLabel.empty() && !caseData.value.empty())
                {
                    displayLabel = execOutputPins[i] + " [" + caseData.customLabel + "(" + caseData.value + ")]";
                }
                else if (!caseData.value.empty())
                {
                    displayLabel = execOutputPins[i] + " (" + caseData.value + ")";
                }
            }

            ImGui::PushID(nodeUID * 10000 + 5000 + static_cast<int>(i));
            ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(140, 30, 30, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(200, 50, 50, 220));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(230, 80, 80, 240));

            // Show label before button
            ImGui::Text("%s", displayLabel.c_str());
            ImGui::SameLine();

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
            ImNodes::BeginOutputAttribute(attrID, connected ? ImNodesPinShape_TriangleFilled : ImNodesPinShape_Triangle);

            // Phase 3 FIX: For Switch nodes, display the label with match value
            // Format: "Case_1 [Probe(1)]" instead of just "Case_1"
            std::string displayLabel = execOutputPins[i];
            if (def.Type == TaskNodeType::Switch && i < def.switchCases.size())
            {
                const SwitchCaseDefinition& caseData = def.switchCases[i];
                if (!caseData.customLabel.empty() && !caseData.value.empty())
                {
                    displayLabel = execOutputPins[i] + " [" + caseData.customLabel + "(" + caseData.value + ")]";
                }
                else if (!caseData.value.empty())
                {
                    displayLabel = execOutputPins[i] + " (" + caseData.value + ")";
                }
            }

            ImGui::Text("%s", displayLabel.c_str());
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
        bool connected = connectedAttrIDs.find(attrID) != connectedAttrIDs.end();
        ImNodes::PushColorStyle(ImNodesCol_Pin,
                                GetDataPinColor(dataOutputPins[i].second));
        ImNodes::BeginOutputAttribute(attrID, connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle);
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
                            paramValue = paramIt->second.LiteralValue.to_string();
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

                    // Display condition expression if available in conditionOperandRefs
                    if (ci < def.conditionOperandRefs.size())
                    {
                        std::string exprStr = BuildConditionExpressionString(def.conditionOperandRefs[ci]);
                        ImGui::Text("  %s %s", opLabel, exprStr.c_str());
                    }
                    else
                    {
                        // Fallback: display preset ID if no operand refs available
                        ImGui::Text("  %s %s", opLabel, ref.presetID.c_str());
                    }

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
        {
            // Phase 24 — Display algebraic expression: "A + B" in gray text
            // Build left operand display
            std::string leftStr = "A";
            switch (def.mathOpRef.leftOperand.mode)
            {
                case MathOpOperand::Mode::Const:
                    leftStr = def.mathOpRef.leftOperand.constValue;
                    break;
                case MathOpOperand::Mode::Variable:
                    leftStr = "[" + def.mathOpRef.leftOperand.variableName + "]";
                    break;
                case MathOpOperand::Mode::Pin:
                    leftStr = "[Pin]";
                    break;
                default:
                    leftStr = "A";
                    break;
            }

            // Build right operand display
            std::string rightStr = "B";
            switch (def.mathOpRef.rightOperand.mode)
            {
                case MathOpOperand::Mode::Const:
                    rightStr = def.mathOpRef.rightOperand.constValue;
                    break;
                case MathOpOperand::Mode::Variable:
                    rightStr = "[" + def.mathOpRef.rightOperand.variableName + "]";
                    break;
                case MathOpOperand::Mode::Pin:
                    rightStr = "[Pin]";
                    break;
                default:
                    rightStr = "B";
                    break;
            }

            // Get operator (default to + if empty)
            const std::string& op = def.mathOpRef.mathOperator.empty() 
                                     ? std::string("+") 
                                     : def.mathOpRef.mathOperator;

            // Display expression in gray (same style as GetBBValue/SetBBValue)
            ImGui::TextDisabled("  %s %s %s", leftStr.c_str(), op.c_str(), rightStr.c_str());
            break;
        }

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
