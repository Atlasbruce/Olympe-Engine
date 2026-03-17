/**
 * @file NodeBranchRenderer.cpp
 * @brief Implementation of NodeBranchRenderer (Phase 24.4).
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

    // Node title
    ImGui::Text("Branch: %s", data.nodeName.c_str());
    if (data.breakpoint)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f), "[BP]");
    }

    RenderSectionSeparator();

    // ── Section 1: Static input pin ─────────────────────────────────────────
    RenderStaticInputPin(data);

    RenderSectionSeparator();

    // ── Section 2: Conditions ────────────────────────────────────────────────
    RenderConditionsSection(data);

    RenderSectionSeparator();

    // ── Section 3: Dynamic data pins ────────────────────────────────────────
    RenderDynamicPinsSection(data);

    RenderSectionSeparator();

    // ── Section 4: Execution flow pins ──────────────────────────────────────
    RenderExecutionFlowPins(data);

    ImGui::PopID();
#endif
}

// ============================================================================
// Section 1 — Static input pin
// ============================================================================

void NodeBranchRenderer::RenderStaticInputPin(const NodeBranchData& /*data*/)
{
#ifndef OLYMPE_HEADLESS
    RenderSectionHeader("Inputs");
    // Placeholder: actual ImNodes pin registration happens in the parent node
    // renderer using the node ID.  Here we display the pin label only.
    ImGui::BulletText("In (execution)");
#endif
}

// ============================================================================
// Section 2 — Conditions (read-only)
// ============================================================================

void NodeBranchRenderer::RenderConditionsSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    RenderSectionHeader("Conditions");

    if (data.conditionRefs.empty())
    {
        ImGui::TextDisabled("(no conditions)");
        return;
    }

    for (int i = 0; i < static_cast<int>(data.conditionRefs.size()); ++i)
    {
        const NodeConditionRef& ref = data.conditionRefs[i];

        ImGui::PushID(i);

        // Logical operator label
        if (i == 0)
            ImGui::TextDisabled("Start");
        else if (ref.logicalOp == LogicalOp::And)
            ImGui::TextDisabled("And  ");
        else
            ImGui::TextDisabled("Or   ");

        ImGui::SameLine();

        // Condition preview
        const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
        if (preset)
        {
            const std::string label = preset->GetPreview();
            ImGui::Text("%s", label.c_str());

            // Hover tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Preset: %s", preset->name.c_str());
                ImGui::Text("ID:     %s", preset->id.c_str());
                ImGui::Text("Expr:   %s", label.c_str());
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
                               "(missing: %s)", ref.presetID.c_str());
        }

        ImGui::PopID();
    }
#endif
}

// ============================================================================
// Section 3 — Dynamic data pins (yellow)
// ============================================================================

void NodeBranchRenderer::RenderDynamicPinsSection(const NodeBranchData& data)
{
#ifndef OLYMPE_HEADLESS
    RenderSectionHeader("Data Inputs");

    if (data.dynamicPins.empty())
    {
        ImGui::TextDisabled("(none)");
        return;
    }

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
// Section 4 — Execution flow pins
// ============================================================================

void NodeBranchRenderer::RenderExecutionFlowPins(const NodeBranchData& /*data*/)
{
#ifndef OLYMPE_HEADLESS
    RenderSectionHeader("Outputs");
    ImGui::BulletText("Then (true branch)");
    ImGui::BulletText("Else (false branch)");
#endif
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
