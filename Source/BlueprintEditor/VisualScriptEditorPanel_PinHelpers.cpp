/**
 * @file VisualScriptEditorPanel_PinHelpers.cpp
 * @brief Pin name and type helper methods for VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details This file contains utility methods for querying pin information:
 *          - GetExecInputPins() — Static exec input pins by node type
 *          - GetExecOutputPins() — Static exec output pins by node type
 *          - GetExecOutputPinsForNode() — Exec pins including dynamic pins
 *          - GetDataInputPins() — Static data input pins by node type
 *          - GetDataOutputPins() — Static data output pins by node type
 *
 * These helpers are used throughout the editor for:
 *   - Canvas node rendering (determining which pins to draw)
 *   - Connection validation (checking if a link is allowed)
 *   - UID generation (mapping pin names to attribute IDs)
 *   - Serialization (storing connection information)
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "ConditionRegistry.h"
#include "OperatorRegistry.h"
#include "BBVariableRegistry.h"
#include "MathOpOperand.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../NodeGraphCore/GlobalTemplateBlackboard.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <unordered_set>

namespace Olympe {

// ============================================================================
// Execution Pin Helpers
// ============================================================================

/**
 * @brief Get the static execution input pins for a given node type
 * 
 * Returns a list of pin names that represent the execution control flow INTO a node.
 * Most nodes have a single "In" pin, but some special nodes have no exec-in:
 * - EntryPoint: No exec-in (it's the entry to the graph)
 * - GetBBValue: No exec-in (data-pure node, no control flow)
 * - MathOp: No exec-in (data-pure node, no control flow)
 * 
 * All other node types receive execution from upstream via an "In" pin.
 * 
 * @param type The node type to query
 * @return std::vector<std::string> List of exec input pin names (usually empty or {"In"})
 * @note Phase 24.2: GetBBValue and MathOp are data-pure, not control-flow nodes
 * @see GetExecOutputPins(), ExecInAttrUID()
 */
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

/**
 * @brief Get the static execution output pins for a given node type
 * 
 * Returns a list of pin names that represent the execution control flow OUT OF a node.
 * Different node types have different numbers of exec-out pins based on their semantics:
 * - EntryPoint: {"Out"} — single flow entry
 * - Branch: {"Then", "Else"} — conditional split
 * - While: {"Loop", "Completed"} — loop control
 * - ForEach: {"Loop Body", "Completed"} — iteration control
 * - DoOnce: {"Out"} — single output
 * - Delay: {"Completed"} — async completion
 * - SubGraph: {"Completed"} — subgraph completion
 * - VSSequence: {"Out"} — sequence step (may have dynamic pins)
 * - Switch: {"Case_0"} — multi-way branch (may have dynamic pins)
 * - AtomicTask: {"Completed"} — action completion
 * - GetBBValue: {} — data-pure node (no exec-out)
 * - SetBBValue: {"Completed"} — update completion
 * - MathOp: {} — data-pure node (no exec-out)
 * 
 * VSSequence and Switch nodes may add dynamic pins at runtime, so use
 * GetExecOutputPinsForNode() to include those.
 * 
 * @param type The node type to query
 * @return std::vector<std::string> List of exec output pin names
 * @note Phase 24.2: GetBBValue and MathOp are data-pure, not control-flow nodes
 * @see GetExecOutputPinsForNode(), GetExecInputPins(), ExecOutAttrUID()
 */
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

/**
 * @brief Get execution output pins for a node, including dynamic pins
 * 
 * This is the full version of GetExecOutputPins() that accounts for dynamically
 * added pins. Some node types (VSSequence, Switch) can have user-added exec-out pins
 * that are not in the static list:
 * - VSSequence nodes may have "Out_2", "Out_3", etc. added by the user
 * - Switch nodes may have "Case_1", "Case_2", etc. added by the user
 * 
 * For all other node types, this returns the same as GetExecOutputPins().
 * 
 * @param def The node definition to query
 * @return std::vector<std::string> List of exec output pin names including dynamic pins
 * @note Required for accurate pin counting during rendering and UID generation
 * @see GetExecOutputPins()
 */
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

// ============================================================================
// Data Pin Helpers
// ============================================================================

/**
 * @brief Get the static data input pins for a given node type
 * 
 * Returns a list of pin names that represent VALUE inputs to a node.
 * Data pins are distinct from execution pins and carry variable values:
 * - SetBBValue: {"Value"} — the value to store in the blackboard
 * - MathOp: {"A", "B"} — left and right operands
 * - Branch: {} — uses ONLY dynamic data-in pins (no static condition pin)
 *                 to avoid pin name conflicts when conditions are edited
 * - All others: {} — no standard data inputs
 * 
 * Phase 24 Notes:
 *   - Branch nodes use only dynamic pins generated from condition definitions
 *   - This allows Branch conditions to be added/removed without hardcoded pin lists
 *   - No static "Condition" pin is present to prevent naming conflicts
 * 
 * @param type The node type to query
 * @return std::vector<std::string> List of data input pin names
 * @note Data pins are independent of execution pins; a node can have both types
 * @see GetDataOutputPins(), DataInAttrUID()
 */
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

/**
 * @brief Get the static data output pins for a given node type
 * 
 * Returns a list of pin names that represent VALUE outputs from a node.
 * Data pins carry computed or retrieved values to downstream nodes:
 * - GetBBValue: {"Value"} — the retrieved variable value
 * - MathOp: {"Result"} — the computed arithmetic result
 * - All others: {} — no standard data outputs
 * 
 * Phase 24 Notes:
 *   - GetBBValue is a data-pure node (no exec-in/out, only data-out)
 *   - MathOp is a data-pure node (no exec-in/out, only data in/out)
 *   - These nodes are used to build data flow networks without control flow
 * 
 * @param type The node type to query
 * @return std::vector<std::string> List of data output pin names
 * @see GetDataInputPins(), DataOutAttrUID()
 */
std::vector<std::string> VisualScriptEditorPanel::GetDataOutputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::GetBBValue:  return {"Value"};
        case TaskNodeType::MathOp:      return {"Result"};
        default:                        return {};
    }
}

}  // namespace Olympe
