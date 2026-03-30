/**
 * @file VisualScriptEditorPanel_Helpers.cpp
 * @brief Helper methods for VisualScriptEditorPanel (node/link allocation and UID generation).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details This file contains utility helper methods for:
 *          - Node ID allocation (AllocNodeID)
 *          - Link ID allocation (AllocLinkID)
 *          - Attribute UID generation for execution and data pins:
 *            * ExecInAttrUID() — Execution input attribute UID
 *            * ExecOutAttrUID() — Execution output attribute UID
 *            * DataInAttrUID() — Data input attribute UID
 *            * DataOutAttrUID() — Data output attribute UID
 *
 * UID Scheme:
 *   The attribute UID system uses a deterministic formula: nodeID * 10000 + offset
 *   This allows rapid attribute lookup and maintains unique IDs across node/pin combinations.
 *   Offsets:
 *     0–99:   Reserved for exec-in pin
 *     100–199: Exec-out pins (index 0-99)
 *     200–299: Data-in pins (index 0-99)
 *     300–399: Data-out pins (index 0-99)
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
// ID Allocation Helpers
// ============================================================================

/**
 * @brief Allocate a unique node ID
 * 
 * Returns the next available node ID and increments the internal counter.
 * Each node in the editor canvas must have a unique ID for:
 * - ImNodes attribute UID generation (nodeID * 10000 + offset)
 * - Node lookup in editor node list
 * - Serialization and deserialization
 * 
 * @return int Next available node ID (starting from 0)
 * @note IDs are never reused; once a node is deleted, its ID is permanently consumed.
 *       This prevents potential ID collisions in undo/redo stacks.
 * @see AddNode(), RemoveNode()
 */
int VisualScriptEditorPanel::AllocNodeID()
{
    return m_nextNodeID++;
}

/**
 * @brief Allocate a unique link ID
 * 
 * Returns the next available link ID and increments the internal counter.
 * Each connection between nodes must have a unique ID for:
 * - ImNodes link tracking
 * - Link validation and lookup
 * - Undo/redo serialization
 * 
 * @return int Next available link ID (starting from 0)
 * @note Like node IDs, link IDs are never reused to maintain consistency across
 *       undo/redo operations and command history.
 * @see AddLink(), RemoveLink()
 */
int VisualScriptEditorPanel::AllocLinkID()
{
    return m_nextLinkID++;
}

// ============================================================================
// Attribute UID Generation
// ============================================================================

/**
 * @brief Generate a unique attribute UID for the execution input pin of a node
 * 
 * Formula: nodeID * 10000 + 0
 * 
 * The execution input (or "In" pin) is where the node receives execution flow from
 * previous nodes. Only one exec-in pin per node is allowed.
 * 
 * @param nodeID The node's unique ID
 * @return int Unique attribute UID for the node's exec-in pin
 * @note Returns the same value for the same nodeID (deterministic, reversible formula).
 *       ImNodes uses this UID to identify pin endpoints for links.
 * @see ExecOutAttrUID(), DataInAttrUID(), DataOutAttrUID()
 */
int VisualScriptEditorPanel::ExecInAttrUID(int nodeID) const
{
    return nodeID * 10000 + 0;
}

/**
 * @brief Generate a unique attribute UID for an execution output pin of a node
 * 
 * Formula: nodeID * 10000 + 100 + pinIndex
 * 
 * Execution output pins (or "Out" pins) are where the node sends execution flow
 * to downstream nodes. A node may have multiple exec-out pins for control flow:
 * - EntryPoint → {"Out"}
 * - Branch → {"Then", "Else"}
 * - While → {"Loop", "Completed"}
 * - Switch → {"Case_0", "Case_1", ...}
 * 
 * @param nodeID The node's unique ID
 * @param pinIndex The index of the output pin (0–99)
 * @return int Unique attribute UID for the specific exec-out pin
 * @note Supports up to 100 execution output pins per node (offset range: 100–199).
 * @see ExecInAttrUID(), DataInAttrUID(), DataOutAttrUID()
 */
int VisualScriptEditorPanel::ExecOutAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 100 + pinIndex;
}

/**
 * @brief Generate a unique attribute UID for a data input pin of a node
 * 
 * Formula: nodeID * 10000 + 200 + pinIndex
 * 
 * Data input pins carry variable/parameter values from other nodes:
 * - MathOp → {"A", "B"} (input operands for mathematical expressions)
 * - SetBBValue → {"Value"} (the value to store in blackboard)
 * - Branch → Dynamic condition pins
 * 
 * Data pins are distinct from execution pins and can coexist on the same node.
 * 
 * @param nodeID The node's unique ID
 * @param pinIndex The index of the input pin (0–99)
 * @return int Unique attribute UID for the specific data-in pin
 * @note Supports up to 100 data input pins per node (offset range: 200–299).
 *       Index 0–99 allows sufficient room for typical data-heavy nodes.
 * @see ExecInAttrUID(), ExecOutAttrUID(), DataOutAttrUID()
 */
int VisualScriptEditorPanel::DataInAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 200 + pinIndex;
}

/**
 * @brief Generate a unique attribute UID for a data output pin of a node
 * 
 * Formula: nodeID * 10000 + 300 + pinIndex
 * 
 * Data output pins carry computed or retrieved values to downstream nodes:
 * - GetBBValue → {"Value"} (the retrieved variable value)
 * - MathOp → {"Result"} (the computed result)
 * - Dynamic operand outputs (from condition presets)
 * 
 * @param nodeID The node's unique ID
 * @param pinIndex The index of the output pin (0–99)
 * @return int Unique attribute UID for the specific data-out pin
 * @note Supports up to 100 data output pins per node (offset range: 300–399).
 * @see ExecInAttrUID(), ExecOutAttrUID(), DataInAttrUID()
 */
int VisualScriptEditorPanel::DataOutAttrUID(int nodeID, int pinIndex) const
{
    return nodeID * 10000 + 300 + pinIndex;
}

}  // namespace Olympe
