/**
 * @file VisualScriptEditorPanel_Helpers.cpp
 * @brief TIER 0 - UID and Pin name helper functions (pure, no dependencies)
 * @author Olympe Engine
 * @date 2026-03-09
 */

#include "VisualScriptEditorPanel.h"
#include <vector>
#include <string>

namespace Olympe {

// ============================================================================
// UID helpers
// ============================================================================

int VisualScriptEditorPanel::AllocNodeID()
{
    return m_nextNodeID++;
}

int VisualScriptEditorPanel::AllocLinkID()
{
    return m_nextLinkID++;
}

// Attribute UIDs are built as:
//   nodeID * 10000 + offset
// The offsets are:
//   0       → exec-in  "In"
//   100–199 → exec-out pins (index 0-99)
//   200–299 → data-in  pins (index 0-99)
//   300–399 → data-out pins (index 0-99)

int VisualScriptEditorPanel::ExecInAttrUID(int nodeID) const
{
    return nodeID * 10000 + 0;
}

int VisualScriptEditorPanel::ExecOutAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 100 + pinIndex;
}

int VisualScriptEditorPanel::DataInAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 200 + pinIndex;
}

int VisualScriptEditorPanel::DataOutAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 300 + pinIndex;
}

// ============================================================================
// Pin name helpers
// ============================================================================

std::vector<std::string> VisualScriptEditorPanel::GetExecInputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::EntryPoint:
            return {};  // No exec-in on EntryPoint
        case TaskNodeType::GetBBValue:
            return {};  // Phase 24.2: Variable (GetBBValue) is data-pure (no execution pins)
        case TaskNodeType::MathOp:
            return {};  // Phase 24.2: MathOp is data-pure (no execution pins)
        default:
            return {"In"};
    }
}

std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::EntryPoint:  return {"Out"};
        case TaskNodeType::Branch:      return {"Then", "Else"};
        case TaskNodeType::While:       return {"Loop", "Completed"};
        case TaskNodeType::ForEach:     return {"Loop Body", "Completed"};
        case TaskNodeType::DoOnce:      return {"Out"};
        case TaskNodeType::Delay:       return {"Completed"};
        case TaskNodeType::SubGraph:    return {"Completed"};
        case TaskNodeType::VSSequence:  return {"Out"};
        case TaskNodeType::Switch:      return {"Case_0"};
        case TaskNodeType::AtomicTask:  return {"Completed"};
        case TaskNodeType::GetBBValue:  return {};  // Phase 24.2: Variable (GetBBValue) is data-pure (no execution pins)
        case TaskNodeType::SetBBValue:  return {"Completed"};  // SetBBValue needs exec-out for control flow
        case TaskNodeType::MathOp:      return {};  // Phase 24.2: MathOp is data-pure (no execution pins)
        default:                        return {"Out"};
    }
}

std::vector<std::string> VisualScriptEditorPanel::GetExecOutputPinsForNode(
    const TaskNodeDefinition& def) const
{
    std::vector<std::string> pins = GetExecOutputPins(def.Type);
    if (def.Type == TaskNodeType::VSSequence || def.Type == TaskNodeType::Switch)
    {
        for (size_t i = 0; i < def.DynamicExecOutputPins.size(); ++i)
            pins.push_back(def.DynamicExecOutputPins[i]);
    }
    return pins;
}

std::vector<std::string> VisualScriptEditorPanel::GetDataInputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::SetBBValue:  return {"Value"};
        case TaskNodeType::MathOp:      return {"A", "B"};
        // Phase 24: Branch nodes use ONLY dynamic data-in pins (Pin-in)
        // No static "Condition" pin to avoid conflicts with dynamic pins
        case TaskNodeType::Branch:      return {};
        default:                        return {};
    }
}

std::vector<std::string> VisualScriptEditorPanel::GetDataOutputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::GetBBValue:  return {"Value"};
        case TaskNodeType::MathOp:      return {"Result"};
        default:                        return {};
    }
}

// ============================================================================
// UX Enhancement #3 — Type-filtered variable utility
// ============================================================================

/*static*/
std::vector<BlackboardEntry> VisualScriptEditorPanel::GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType)
{
    std::vector<BlackboardEntry> filtered;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (allVars[i].Type == expectedType)
            filtered.push_back(allVars[i]);
    }
    return filtered;
}

} // namespace Olympe
