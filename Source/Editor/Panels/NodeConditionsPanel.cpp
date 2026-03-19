/**
 * @file NodeConditionsPanel.cpp
 * @brief Implementation of NodeConditionsPanel (Phase 24.2).
 * @author Olympe Engine
 * @date 2026-03-17
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "NodeConditionsPanel.h"

#include <algorithm>
#include <sstream>

// ImGui is only compiled in the full editor build.
#ifndef OLYMPE_HEADLESS
#  include "../../third_party/imgui/imgui.h"
#endif

namespace Olympe {

// ============================================================================
// Constructor
// ============================================================================

NodeConditionsPanel::NodeConditionsPanel(ConditionPresetRegistry& registry)
    : m_registry(registry)
    , m_editModal(registry)
{
}

// ============================================================================
// State accessors
// ============================================================================

void NodeConditionsPanel::SetConditionRefs(const std::vector<NodeConditionRef>& refs)
{
    m_conditionRefs = refs;
    NormalizeLogicalOps();

    // Resize the operand-ref list to stay in sync with conditionRefs.
    // Preserve existing entries where possible; add defaults for new entries.
    if (m_conditionOperandRefs.size() != m_conditionRefs.size())
    {
        m_conditionOperandRefs.resize(m_conditionRefs.size());
        for (size_t i = 0; i < m_conditionOperandRefs.size(); ++i)
            m_conditionOperandRefs[i].conditionIndex = static_cast<int>(i);
    }

    m_dirty = false;
}

const std::vector<NodeConditionRef>& NodeConditionsPanel::GetConditionRefs() const
{
    return m_conditionRefs;
}

void NodeConditionsPanel::SetConditionOperandRefs(const std::vector<ConditionRef>& refs)
{
    m_conditionOperandRefs = refs;
}

const std::vector<ConditionRef>& NodeConditionsPanel::GetConditionOperandRefs() const
{
    return m_conditionOperandRefs;
}

void NodeConditionsPanel::SetDynamicPins(const std::vector<DynamicDataPin>& pins)
{
    m_dynamicPins = pins;
}

// ============================================================================
// Condition management
// ============================================================================

void NodeConditionsPanel::AddCondition(const std::string& presetID)
{
    if (presetID.empty())
        return;

    LogicalOp op = m_conditionRefs.empty() ? LogicalOp::Start : LogicalOp::And;
    m_conditionRefs.emplace_back(presetID, op);
    NormalizeLogicalOps();

    // Add a matching ConditionRef initialized from the preset's default operands.
    ConditionRef cref;
    cref.conditionIndex = static_cast<int>(m_conditionRefs.size()) - 1;
    const ConditionPreset* preset = m_registry.GetPreset(presetID);
    if (preset)
    {
        // Initialize from the preset's left operand
        switch (preset->left.mode)
        {
            case OperandMode::Variable:
                cref.leftOperand.mode = OperandRef::Mode::Variable;
                cref.leftOperand.variableName = preset->left.stringValue;
                break;
            case OperandMode::Pin:
                cref.leftOperand.mode = OperandRef::Mode::Pin;
                break;
            case OperandMode::Const:
            default:
                cref.leftOperand.mode = OperandRef::Mode::Const;
                {
                    std::ostringstream oss;
                    oss << preset->left.constValue;
                    cref.leftOperand.constValue = oss.str();
                }
                break;
        }
        // Initialize operator string
        switch (preset->op)
        {
            case ComparisonOp::Equal:        cref.operatorStr = "=="; break;
            case ComparisonOp::NotEqual:     cref.operatorStr = "!="; break;
            case ComparisonOp::Less:         cref.operatorStr = "<";  break;
            case ComparisonOp::LessEqual:    cref.operatorStr = "<="; break;
            case ComparisonOp::Greater:      cref.operatorStr = ">";  break;
            case ComparisonOp::GreaterEqual: cref.operatorStr = ">="; break;
            default:                         cref.operatorStr = "=="; break;
        }
        // Initialize from the preset's right operand
        switch (preset->right.mode)
        {
            case OperandMode::Variable:
                cref.rightOperand.mode = OperandRef::Mode::Variable;
                cref.rightOperand.variableName = preset->right.stringValue;
                break;
            case OperandMode::Pin:
                cref.rightOperand.mode = OperandRef::Mode::Pin;
                break;
            case OperandMode::Const:
            default:
                cref.rightOperand.mode = OperandRef::Mode::Const;
                {
                    std::ostringstream oss;
                    oss << preset->right.constValue;
                    cref.rightOperand.constValue = oss.str();
                }
                break;
        }
    }
    m_conditionOperandRefs.push_back(cref);

    m_dirty = true;

    if (OnPresetChanged)
        OnPresetChanged(presetID);
}

void NodeConditionsPanel::RemoveCondition(size_t index)
{
    if (index >= m_conditionRefs.size())
        return;

    m_conditionRefs.erase(m_conditionRefs.begin() + static_cast<int>(index));

    // Keep operand refs in sync
    if (index < m_conditionOperandRefs.size())
    {
        m_conditionOperandRefs.erase(
            m_conditionOperandRefs.begin() + static_cast<int>(index));
    }
    // Re-index remaining operand refs
    for (size_t i = index; i < m_conditionOperandRefs.size(); ++i)
        m_conditionOperandRefs[i].conditionIndex = static_cast<int>(i);

    NormalizeLogicalOps();
    m_dirty = true;
}

void NodeConditionsPanel::SetLogicalOp(size_t index, LogicalOp op)
{
    if (index >= m_conditionRefs.size())
        return;

    // Index 0 is always Start
    m_conditionRefs[index].logicalOp = (index == 0) ? LogicalOp::Start : op;
    m_dirty = true;
}

size_t NodeConditionsPanel::GetConditionCount() const
{
    return m_conditionRefs.size();
}

// ============================================================================
// Preset deletion handler
// ============================================================================

void NodeConditionsPanel::OnPresetDeleted(const std::string& deletedPresetID)
{
    bool changed = false;
    auto it = m_conditionRefs.begin();
    while (it != m_conditionRefs.end())
    {
        if (it->presetID == deletedPresetID)
        {
            it      = m_conditionRefs.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }

    if (changed)
    {
        NormalizeLogicalOps();
        m_dirty = true;
    }
}

// ============================================================================
// Validation
// ============================================================================

bool NodeConditionsPanel::IsValid() const
{
    return Validate().empty();
}

std::vector<std::string> NodeConditionsPanel::Validate() const
{
    std::vector<std::string> errors;

    for (size_t i = 0; i < m_conditionRefs.size(); ++i)
    {
        const NodeConditionRef& ref = m_conditionRefs[i];

        if (ref.presetID.empty())
        {
            std::ostringstream oss;
            oss << "Condition " << (i + 1) << ": preset ID is empty.";
            errors.push_back(oss.str());
            continue;
        }

        if (!m_registry.ValidatePresetID(ref.presetID))
        {
            std::ostringstream oss;
            oss << "Condition " << (i + 1) << ": preset \"" << ref.presetID
                << "\" not found in registry.";
            errors.push_back(oss.str());
        }
    }

    return errors;
}

// ============================================================================
// Dropdown filter
// ============================================================================

void NodeConditionsPanel::SetDropdownFilter(const std::string& filter)
{
    m_dropdownFilter = filter;
}

std::vector<ConditionPreset>
NodeConditionsPanel::GetFilteredPresetsForDropdown() const
{
    if (m_dropdownFilter.empty())
    {
        // Return all presets in registry order
        std::vector<ConditionPreset> result;
        for (const auto& id : m_registry.GetAllPresetIDs())
        {
            const ConditionPreset* p = m_registry.GetPreset(id);
            if (p)
                result.push_back(*p);
        }
        return result;
    }

    // Filtered by name substring
    std::vector<ConditionPreset> result;
    for (const auto& id : m_registry.FindPresetsByName(m_dropdownFilter))
    {
        const ConditionPreset* p = m_registry.GetPreset(id);
        if (p)
            result.push_back(*p);
    }
    return result;
}

// ============================================================================
// Rendering
// ============================================================================

void NodeConditionsPanel::Render()
{
#ifndef OLYMPE_HEADLESS
    ImGui::PushID("NodeConditionsPanel");

    // Render the condition preview + "Edit Conditions" button
    // This opens the modal where users can add/edit/delete conditions with full UI
    RenderConditionsPreview();

    // ── Section 4: Dynamic data pins (only if any) ──────────────────────────
    if (!m_dynamicPins.empty())
    {
        ImGui::Separator();
        RenderDynamicPinsSection();
    }

    // ── Modal polling (renders when open, handles confirmation) ─────────────
    m_editModal.Render();
    if (m_editModal.IsConfirmed())
    {
        SetConditionRefs(m_editModal.GetConditionRefs());
        m_dirty = true;
        m_editModal.Close();

        if (OnDynamicPinsNeedRegeneration)
            OnDynamicPinsNeedRegeneration();
    }

    ImGui::PopID();
#endif
}

void NodeConditionsPanel::RenderTitleSection()
{
#ifndef OLYMPE_HEADLESS
    // Blue background (#0066CC equivalent) with white text — matches canvas node
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    const std::string& title = m_nodeName.empty() ? "Node" : m_nodeName;
    ImGui::Selectable(title.c_str(), true, ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);
#endif
}

void NodeConditionsPanel::RenderExecPinsSection()
{
#ifndef OLYMPE_HEADLESS
    // Static exec pins — never editable from this panel
    const float columnWidth = 120.f;

    ImGui::TextDisabled("In");
    ImGui::SameLine(columnWidth);
    ImGui::TextDisabled("Then");

    ImGui::TextDisabled("  ");
    ImGui::SameLine(columnWidth);
    ImGui::TextDisabled("Else");
#endif
}

void NodeConditionsPanel::RenderConditionsPreview()
{
#ifndef OLYMPE_HEADLESS
    // Green text for condition previews (READ-ONLY)
    const ImVec4 condColor(0.f, 1.f, 0.f, 1.f);

    if (m_conditionRefs.empty())
    {
        ImGui::TextDisabled("(no conditions)");
    }
    else
    {
        for (size_t i = 0; i < m_conditionRefs.size(); ++i)
        {
            const NodeConditionRef& ref = m_conditionRefs[i];

            ImGui::PushID(static_cast<int>(i));

            // Logical operator column (fixed width)
            const char* opLabel = "   ";
            if (i > 0)
            {
                if (ref.logicalOp == LogicalOp::And)
                    opLabel = "And";
                else if (ref.logicalOp == LogicalOp::Or)
                    opLabel = "Or ";
            }

            const ConditionPreset* preset = m_registry.GetPreset(ref.presetID);
            ImGui::PushStyleColor(ImGuiCol_Text, condColor);
            if (preset)
                ImGui::Text("%s %s", opLabel, preset->GetPreview().c_str());
            else
                ImGui::Text("%s (missing: %s)", opLabel, ref.presetID.c_str());
            ImGui::PopStyleColor();

            ImGui::PopID();
        }
    }

    ImGui::Spacing();

    // "Edit Conditions" button — full width — opens the owned modal
    if (ImGui::Button("Edit Conditions", ImVec2(-1.f, 24.f)))
    {
        m_editModalRequested = true;
        m_editModal.Open(m_conditionRefs);
    }
#endif
}

void NodeConditionsPanel::RenderConditionList()
{
#ifndef OLYMPE_HEADLESS
    // UI layout constants
    static const float  kOpComboWidth    = 60.f;  ///< Width of the And/Or combo
    static const float  kDeleteBtnWidth  = 22.f;  ///< Width of the [X] delete button
    static const size_t kFilterBufSize   = 256u;  ///< Max filter string length incl. NUL

    // Collapsible section header — green tint to match condition colour convention
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.9f, 0.f, 1.f));
    const bool open = ImGui::CollapsingHeader(
        "Structured Conditions (evaluated with implicit AND)",
        ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();

    if (!open)
        return;

    const ImVec4 condColor(0.f, 1.f, 0.f, 1.f);   // green for preview text (unused in list, kept for IDE compat)
    (void)condColor;

    // Deferred-deletion index (avoids invalidating the iterator inside the loop)
    size_t deleteIdx = static_cast<size_t>(-1);

    for (size_t i = 0; i < m_conditionRefs.size(); ++i)
    {
        NodeConditionRef& ref = m_conditionRefs[i];
        ImGui::PushID(static_cast<int>(i));

        // ── Operand editors (left operand | operator | right operand) ─────────
        // Ensure parallel operand-ref list is always in sync
        if (i >= m_conditionOperandRefs.size())
        {
            ConditionRef newCref;
            newCref.conditionIndex = static_cast<int>(i);
            m_conditionOperandRefs.push_back(newCref);
        }
        ConditionRef& cref = m_conditionOperandRefs[i];

        // Resolve short pin labels for Pin-mode operands
        std::string leftPinLabel, rightPinLabel;
        for (const auto& pin : m_dynamicPins)
        {
            if (pin.conditionIndex == static_cast<int>(i))
            {
                if (pin.position == OperandPosition::Left)
                    leftPinLabel = pin.GetShortLabel();
                else
                    rightPinLabel = pin.GetShortLabel();
            }
        }

        ImGui::PushID("left");
        RenderOperandDropdown(cref, /*isLeft=*/true, leftPinLabel);
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID("op");
        RenderOperatorDropdown(cref);
        ImGui::PopID();

        ImGui::SameLine();
        ImGui::PushID("right");
        RenderOperandDropdown(cref, /*isLeft=*/false, rightPinLabel);
        ImGui::PopID();

        ImGui::SameLine();
        if (i > 0)
        {
            ImGui::SetNextItemWidth(kOpComboWidth);
            const char* opLabel = (ref.logicalOp == LogicalOp::Or) ? "Or" : "And";
            if (ImGui::BeginCombo("##op", opLabel, ImGuiComboFlags_NoArrowButton))
            {
                if (ImGui::Selectable("And", ref.logicalOp == LogicalOp::And))
                {
                    ref.logicalOp = LogicalOp::And;
                    m_dirty = true;
                }
                if (ImGui::Selectable("Or", ref.logicalOp == LogicalOp::Or))
                {
                    ref.logicalOp = LogicalOp::Or;
                    m_dirty = true;
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
        }
        else
        {
            // Reserve the same horizontal space as the combo so [X] buttons align
            ImGui::Dummy(ImVec2(kOpComboWidth, 0.f));
            ImGui::SameLine();
        }

        // ── Delete button (X) ─────────────────────────────────────────────────
        ImGui::SetNextItemWidth(kDeleteBtnWidth);
        if (ImGui::SmallButton("X"))
            deleteIdx = i;

        ImGui::PopID();
    }

    // Apply deferred deletion outside the loop to avoid iterator invalidation
    if (deleteIdx != static_cast<size_t>(-1))
        RemoveCondition(deleteIdx);

    // ── Empty-list placeholder ────────────────────────────────────────────────
    if (m_conditionRefs.empty())
        ImGui::TextDisabled("(no conditions — click [+ Add Condition] below)");

    ImGui::Spacing();

    // ── [+ Add Condition] button → opens preset-selector popup ───────────────
    if (ImGui::Button("[+ Add Condition]", ImVec2(-1.f, 0.f)))
        ImGui::OpenPopup("##AddCondPopup");

    if (ImGui::BeginPopup("##AddCondPopup"))
    {
        // Filter input
        char filterBuf[kFilterBufSize] = {};
        {
            const std::string& cur = m_dropdownFilter;
            const size_t copyLen =
                cur.size() < (kFilterBufSize - 1u) ? cur.size() : kFilterBufSize - 1u;
            cur.copy(filterBuf, copyLen);
        }
        if (ImGui::InputText("Filter", filterBuf, kFilterBufSize))
            SetDropdownFilter(filterBuf);

        ImGui::Separator();

        const auto presets = GetFilteredPresetsForDropdown();
        for (const auto& p : presets)
        {
            const std::string rowLabel = p.name + "  —  " + p.GetPreview();
            if (ImGui::Selectable(rowLabel.c_str()))
            {
                AddCondition(p.id);
                SetDropdownFilter("");
                ImGui::CloseCurrentPopup();

                if (OnDynamicPinsNeedRegeneration)
                    OnDynamicPinsNeedRegeneration();
            }
        }

        if (presets.empty())
            ImGui::TextDisabled("(no presets in registry)");

        ImGui::EndPopup();
    }
#endif
}


void NodeConditionsPanel::RenderDynamicPinsSection()
{
#ifndef OLYMPE_HEADLESS
    // Yellow color (#FFD700) for dynamic pins
    const ImVec4 pinColor(1.0f, 0.843f, 0.0f, 1.0f);
    for (const auto& pin : m_dynamicPins)
    {
        const std::string displayLabel = pin.GetDisplayLabel();
        ImGui::TextColored(pinColor, "%s", displayLabel.c_str());

        // Hover tooltip shows full label
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", displayLabel.c_str());
        }
    }
#endif
}

void NodeConditionsPanel::RenderOperandDropdown(ConditionRef& cref, bool isLeft,
                                                 const std::string& pinLabel)
{
#ifndef OLYMPE_HEADLESS
    OperandRef& op = isLeft ? cref.leftOperand : cref.rightOperand;
    const char* modeLabels[] = { "Var", "Const", "Pin" };
    int currentMode = 0;
    switch (op.mode)
    {
        case OperandRef::Mode::Variable: currentMode = 0; break;
        case OperandRef::Mode::Const:    currentMode = 1; break;
        case OperandRef::Mode::Pin:      currentMode = 2; break;
        default:                         currentMode = 1; break;
    }

    // Mode selector: small radio-style combo
    ImGui::SetNextItemWidth(54.f);
    if (ImGui::BeginCombo("##mode", modeLabels[currentMode], ImGuiComboFlags_NoArrowButton))
    {
        for (int m = 0; m < 3; ++m)
        {
            bool selected = (m == currentMode);
            if (ImGui::Selectable(modeLabels[m], selected))
            {
                // Mode changed — clear stale data from the old mode
                if (m == 0)
                {
                    op.mode = OperandRef::Mode::Variable;
                    op.dynamicPinID.clear();
                }
                else if (m == 1)
                {
                    op.mode = OperandRef::Mode::Const;
                    op.dynamicPinID.clear();
                }
                else
                {
                    op.mode = OperandRef::Mode::Pin;
                }
                m_dirty = true;
                if (OnDynamicPinsNeedRegeneration)
                    OnDynamicPinsNeedRegeneration();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    // Value field for the selected mode
    static const size_t kBufSize = 128u;
    switch (op.mode)
    {
        case OperandRef::Mode::Variable:
        {
            char buf[kBufSize] = {};
            const std::string& cur = op.variableName;
            const size_t copyLen = cur.size() < (kBufSize - 1u) ? cur.size() : kBufSize - 1u;
            cur.copy(buf, copyLen);
            ImGui::SetNextItemWidth(80.f);
            if (ImGui::InputText("##varname", buf, kBufSize))
            {
                op.variableName = buf;
                m_dirty = true;
            }
            break;
        }
        case OperandRef::Mode::Const:
        {
            char buf[kBufSize] = {};
            const std::string& cur = op.constValue;
            const size_t copyLen = cur.size() < (kBufSize - 1u) ? cur.size() : kBufSize - 1u;
            cur.copy(buf, copyLen);
            ImGui::SetNextItemWidth(80.f);
            if (ImGui::InputText("##constval", buf, kBufSize))
            {
                op.constValue = buf;
                m_dirty = true;
            }
            break;
        }
        case OperandRef::Mode::Pin:
        {
            // Read-only: show the "Pin-in #N" label assigned by DynamicDataPinManager
            const std::string display = pinLabel.empty() ? "(pin)" : pinLabel;
            ImGui::SetNextItemWidth(80.f);
            ImGui::TextDisabled("%s", display.c_str());
            break;
        }
        default:
            break;
    }
#endif
}

void NodeConditionsPanel::RenderOperatorDropdown(ConditionRef& cref)
{
#ifndef OLYMPE_HEADLESS
    static const char* kOperators[] = { "==", "!=", "<", "<=", ">", ">=" };
    static const int   kNumOps = 6;

    int currentOp = 0;
    for (int i = 0; i < kNumOps; ++i)
    {
        if (cref.operatorStr == kOperators[i])
        {
            currentOp = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(44.f);
    if (ImGui::BeginCombo("##op2", kOperators[currentOp], ImGuiComboFlags_NoArrowButton))
    {
        for (int i = 0; i < kNumOps; ++i)
        {
            bool selected = (i == currentOp);
            if (ImGui::Selectable(kOperators[i], selected))
            {
                cref.operatorStr = kOperators[i];
                m_dirty = true;
            }
        }
        ImGui::EndCombo();
    }
#endif
}

// ============================================================================
// Internal helpers
// ============================================================================

void NodeConditionsPanel::NormalizeLogicalOps()
{
    if (!m_conditionRefs.empty())
        m_conditionRefs[0].logicalOp = LogicalOp::Start;
}

} // namespace Olympe
