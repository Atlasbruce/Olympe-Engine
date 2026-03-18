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

    ImGui::PushID(data.nodeID.c_str());

    // ── Section 1: Title bar (blue background) ───────────────────────────────
    RenderTitleSection(data);

    RenderSectionSeparator();

    // ── Section 2: Static exec pins (In | Then / Else) ───────────────────────
    RenderExecPinsSection(data);

    RenderSectionSeparator();

    // ── Section 3: Conditions preview (green, read-only) ─────────────────────
    RenderConditionsSection(data);

    // ── Section 4: Dynamic data pins (yellow, only if any) ───────────────────
    if (!data.dynamicPins.empty())
    {
        RenderSectionSeparator();
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
    // Blue background matching ImGuiCol_Header
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_Header));
    ImGui::Selectable(data.nodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 24.f));
    ImGui::PopStyleColor();

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
    const float columnWidth = 120.f;

    ImGui::Text("In");
    ImGui::SameLine(columnWidth);
    ImGui::Text("Then");

    ImGui::Text("  ");        // empty left column
    ImGui::SameLine(columnWidth);
    ImGui::Text("Else");
#endif
}

// ============================================================================
// Section 3 — Conditions preview (green, read-only)
// ============================================================================

void NodeBranchRenderer::RenderConditionsSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    if (data.conditionRefs.empty())
    {
        ImGui::TextDisabled("(no conditions)");
        return;
    }

    // Green color for condition preview text (bright green, monospace hint via indent)
    const ImVec4 condColor(0.f, 1.f, 0.f, 1.f);

    for (int i = 0; i < static_cast<int>(data.conditionRefs.size()); ++i)
    {
        const NodeConditionRef& ref = data.conditionRefs[i];

        ImGui::PushID(i);
        ImGui::PushStyleColor(ImGuiCol_Text, condColor);

        // Logical operator label (left-aligned, fixed width)
        const char* opLabel = "";
        if (i == 0)
            opLabel = "   ";        // indent first condition (no combinator)
        else if (ref.logicalOp == LogicalOp::And)
            opLabel = "And";
        else
            opLabel = "Or ";

        // Condition preview
        const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
        if (preset)
        {
            const std::string preview = preset->GetPreview();
            ImGui::Text("%s %s", opLabel, preview.c_str());

            ImGui::PopStyleColor();

            // Hover tooltip (drawn without green style)
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Preset: %s", preset->name.c_str());
                ImGui::Text("ID:     %s", preset->id.c_str());
                ImGui::Text("Expr:   %s", preview.c_str());
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
            ImGui::PopStyleColor();
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
    // Dynamic pins are always yellow
    float r, g, b, a;
    DynamicDataPinManager::GetDynamicPinColor(r, g, b, a);
    const ImVec4 pinColor(r, g, b, a);

    for (const auto& pin : data.dynamicPins)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, pinColor);
        ImGui::BulletText("%s", pin.GetDisplayLabel().c_str());
        ImGui::PopStyleColor();
    }
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
#endif
}

} // namespace Olympe
