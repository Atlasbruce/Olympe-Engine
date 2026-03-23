/**
 * @file NodeBranchRenderer.cpp
 * @brief Implementation of NodeBranchRenderer (Phase 24-REFONTE).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "NodeBranchRenderer.h"

// ImGui and ImNodes are only available in the full editor build.
#ifndef OLYMPE_HEADLESS
#  include "../../third_party/imgui/imgui.h"
#  include "../../third_party/imnodes/imnodes.h"
#endif

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

NodeBranchRenderer::NodeBranchRenderer(ConditionPresetRegistry& registry,
                                        DynamicDataPinManager&   pinManager)
    : m_registry(registry)
    , m_pinManager(pinManager)
{
}

// ============================================================================
// Main render entry point
// ============================================================================

void NodeBranchRenderer::RenderNode(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    // If a preset was changed since the last render, mark the pass as "refresh".
    const bool refreshing = m_refreshPending;
    if (refreshing)
        m_refreshPending = false;

    ImGui::PushID(data.nodeID);

    // ── TITLE BAR (blue, styled via ImNodes) ──────────────────────────────────
    ImNodes::BeginNodeTitleBar();
    ImGui::TextUnformatted(data.nodeName.c_str());
    if (data.breakpoint)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "[BP]");
    }
    ImNodes::EndNodeTitleBar();

    // ── EXEC PINS (In | Then / Else) ──────────────────────────────────────────
    // Use columns to align input pins (left) with output pins (right) on the same Y
    ImGui::Columns(2, "exec_pins", false);  // 2 columns, no border
    ImGui::SetColumnWidth(0, 80.0f);

    // LEFT COLUMN: "In" exec input pin
    {
        int inAttrID = data.nodeID * 10000 + 0;  // offset 0 = exec-in
        ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(255, 255, 255, 255));  // White for exec pins
        ImNodes::BeginInputAttribute(inAttrID, ImNodesPinShape_Triangle);
        ImGui::Text("In");
        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }

    // RIGHT COLUMN: "Then" exec output pin
    ImGui::NextColumn();
    {
        int thenAttrID = data.nodeID * 10000 + 100;  // offset 100 = exec-out #0
        ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(255, 255, 255, 255));  // White for exec pins
        ImNodes::BeginOutputAttribute(thenAttrID, ImNodesPinShape_TriangleFilled);
        ImGui::Text("Then");
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
    }

    // LEFT COLUMN: Empty spacer for row alignment
    ImGui::NextColumn();
    ImGui::Spacing();

    // RIGHT COLUMN: "Else" exec output pin
    ImGui::NextColumn();
    {
        int elseAttrID = data.nodeID * 10000 + 101;  // offset 101 = exec-out #1
        ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(255, 255, 255, 255));  // White for exec pins
        ImNodes::BeginOutputAttribute(elseAttrID, ImNodesPinShape_TriangleFilled);
        ImGui::Text("Else");
        ImNodes::EndOutputAttribute();
        ImNodes::PopColorStyle();
    }

    ImGui::Columns(1);  // End columns

    // ── CONDITIONS (green, read-only) ──────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    RenderConditionsSection(data);
    ImGui::Spacing();

    // Phase 24 FIX: Render data-in pins LAST, below conditions for better UX
    // ── DYNAMIC DATA PINS (rendered LAST, at bottom) ──
    if (!data.dynamicPins.empty())
    {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(180.0f / 255.0f, 100.0f / 255.0f, 200.0f / 255.0f, 1.0f)); // Violet header
        ImGui::TextUnformatted("=== DATA INPUTS ===");
        ImGui::PopStyleColor();
        ImGui::Spacing();
        RenderDynamicPinsSection(data);
    }

    ImGui::PopID();
#endif
}

// ============================================================================
// Section 1 — Title bar (blue background)
// ============================================================================

void NodeBranchRenderer::RenderTitleSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    // Blue background (#0066CC equivalent) with white text
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Use Selectable for proper styling but with no stretch
    // Note: Pass negative width to auto-fit content, but don't stretch
    ImGui::TextUnformatted(data.nodeName.c_str());
    ImGui::PopStyleColor(4);

    if (data.breakpoint)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "[BP]");
    }
#endif
}

// ============================================================================
// Section 2 — Static exec pins (In | Then / Else)
// ============================================================================

void NodeBranchRenderer::RenderExecPinsSection(const NodeBranchData& /*data*/)
{
#ifndef OLYMPE_HEADLESS
    // "In" on the left, "Then" and "Else" on the right.
    // These pins are STATIC and NEVER editable from the canvas node.
    const float columnWidth = 150.f;

    ImGui::Text("In");
    ImGui::SameLine(columnWidth);
    ImGui::Text("Then");

    ImGui::Text("  ");        // empty left column
    ImGui::SameLine(columnWidth);
    ImGui::Text("Else");
#endif
}

// ============================================================================
// Section 3 — Conditions preview (green, read-only, with Pin highlighting)
// ============================================================================

void NodeBranchRenderer::RenderConditionsSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    if (data.conditionRefs.empty())
    {
        ImGui::TextDisabled("(no conditions)");
        return;
    }

    // Green color for condition preview text
    const ImVec4 condColor(0.f, 1.f, 0.f, 1.f);
    // Yellow color for Pin references (highlight)
    const ImVec4 pinColor(1.0f, 0.843f, 0.0f, 1.0f);

    for (int i = 0; i < static_cast<int>(data.conditionRefs.size()); ++i)
    {
        const NodeConditionRef& ref = data.conditionRefs[i];

        ImGui::PushID(i);

        // Logical operator label
        const char* opLabel = "";
        if (i == 0)
            opLabel = "   ";        // indent first condition (no combinator)
        else if (ref.logicalOp == LogicalOp::And)
            opLabel = "And";
        else
            opLabel = "Or ";

        // Condition preview with Pin highlighting
         const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
         if (preset)
         {
             // Build the condition display with highlighted Pin references
             ImGui::Text("%s", opLabel);
             ImGui::SameLine(0.0f, 0.0f);

             // Left operand (GetDisplayString() already includes brackets)
             if (preset->left.IsPin())
             {
                 ImGui::TextColored(pinColor, "%s", preset->left.GetDisplayString().c_str());
             }
             else
             {
                 ImGui::TextColored(condColor, "%s", preset->left.GetDisplayString().c_str());
             }
             ImGui::SameLine(0.0f, 4.0f);

             // Operator
             std::string opStr;
             switch (preset->op)
             {
                 case ComparisonOp::Equal:       opStr = "=="; break;
                 case ComparisonOp::NotEqual:    opStr = "!="; break;
                 case ComparisonOp::Less:        opStr = "<"; break;
                 case ComparisonOp::LessEqual:   opStr = "<="; break;
                 case ComparisonOp::Greater:     opStr = ">"; break;
                 case ComparisonOp::GreaterEqual: opStr = ">="; break;
                 default: opStr = "?"; break;
             }
             ImGui::TextColored(condColor, "%s", opStr.c_str());
             ImGui::SameLine(0.0f, 4.0f);

             // Right operand (GetDisplayString() already includes brackets)
             if (preset->right.IsPin())
             {
                 ImGui::TextColored(pinColor, "%s", preset->right.GetDisplayString().c_str());
             }
             else
             {
                 ImGui::TextColored(condColor, "%s", preset->right.GetDisplayString().c_str());
             }

            // Hover tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Preset: %s", preset->name.c_str());
                ImGui::Text("ID:     %s", preset->id.c_str());
                ImGui::Text("Expr:   %s", preset->GetPreview().c_str());
                ImGui::EndTooltip();
            }

            // Click interaction
            if (ImGui::IsItemClicked())
            {
                m_lastClickedCondition = i;
                if (OnConditionClicked)
                    OnConditionClicked(i);
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                               "%s (missing: %s)", opLabel, ref.presetID.c_str());
        }

        ImGui::PopID();
    }
#endif
}

// ============================================================================
// Section 4 — Dynamic data pins (yellow, rendered only when non-empty)
// ============================================================================

void NodeBranchRenderer::RenderDynamicPinsSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    // Dynamic data pins are violet (data pin color)
    const ImVec4 pinColor(180.0f / 255.0f, 100.0f / 255.0f, 200.0f / 255.0f, 1.0f);  // Violet
    const ImU32 pinColorImU32 = IM_COL32(180, 100, 200, 255);  // Violet

    // Create a unique ID offset for dynamic pins to avoid conflicts with static pins
    // Static pins use offsets 0, 100-101. Dynamic pins start at 200.
    int dynamicPinIDBase = data.nodeID * 10000 + 200;

    // IMPORTANT: Render data-in pins in a way that makes them clearly selectable by ImNodes
    // Use separate ImGui groups for better hit detection
    for (size_t i = 0; i < data.dynamicPins.size(); ++i)
    {
        const auto& pin = data.dynamicPins[i];

        // Generate unique attribute ID for this pin
        int attrID = dynamicPinIDBase + static_cast<int>(i);

        // Register this pin as an input attribute in ImNodes with violet color
        ImNodes::PushColorStyle(ImNodesCol_Pin, pinColorImU32);
        ImNodes::BeginInputAttribute(attrID, ImNodesPinShape_Circle);

        // Draw a more prominent violet circle with the label
        ImGui::TextColored(pinColor, "\xe2\x97\x8f");
        ImGui::SameLine(0.0f, 4.0f);

        // Use GetShortLabel() ("Pin-in #N") for the slot label
        const std::string shortLabel = pin.GetShortLabel();
        ImGui::TextColored(pinColor, "%s", shortLabel.c_str());

        // Hover tooltip: show Pin ID, condition index, operand position, and detail label
        if (ImGui::IsItemHovered())
        {
            const char* posStr = (pin.position == OperandPosition::Left) ? "Left" : "Right";
            ImGui::BeginTooltip();
            ImGui::Text("Pin ID: %s", pin.id.c_str());
            ImGui::Text("Condition index: %d", pin.conditionIndex);
            ImGui::Text("Operand side: %s", posStr);
            ImGui::Text("Detail: %s", pin.GetDisplayLabel().c_str());
            ImGui::EndTooltip();
        }

        ImNodes::EndInputAttribute();
        ImNodes::PopColorStyle();
    }
#endif
}

// ============================================================================
// Pin connector setup (ImNodes integration)
// ============================================================================

void NodeBranchRenderer::SetupDynamicPinConnectors(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    // ImNodes header is included only in full editor builds.
    // This method is intentionally left as a no-op here; the ImNodes integration
    // is wired by the host application which controls the ImNodes frame.
    // The yellow TextColored labels are rendered by RenderDynamicPinsSection.
    (void)data;
#endif
}

// ============================================================================
// Pin regeneration (Modal-to-Canvas workflow)
// ============================================================================

void NodeBranchRenderer::TriggerPinRegeneration(
    std::vector<NodeConditionRef>& conditionRefs)
{
    m_pinManager.RegeneratePinsFromConditions(conditionRefs);
}

// ============================================================================
// Preset change notification
// ============================================================================

void NodeBranchRenderer::NotifyPresetChanged(const std::string& /*changedPresetID*/)
{
    m_refreshPending = true;
}

// ============================================================================
// Private helpers
// ============================================================================

void NodeBranchRenderer::RenderSectionHeader(const char* label)
{
#ifndef OLYMPE_HEADLESS
    ImGui::TextDisabled("── %s ──", label);
#endif
}

void NodeBranchRenderer::RenderSectionSeparator()
{
#ifndef OLYMPE_HEADLESS
    ImGui::Separator();
    ImGui::Spacing();
#endif
}

} // namespace Olympe
