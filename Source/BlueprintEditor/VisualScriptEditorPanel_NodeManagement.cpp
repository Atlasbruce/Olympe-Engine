/**
 * @file VisualScriptEditorPanel_NodeManagement.cpp
 * @brief Node and link creation/deletion management for VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details This file contains CRUD operations for graph nodes and connections:
 *          - AddNode() — Create a new node on the canvas
 *          - RemoveNode() — Delete a node and its connections
 *          - ConnectExec() — Create an execution pin connection
 *          - ConnectData() — Create a data pin connection
 *
 * These methods integrate with the undo/redo command system (ICommand/UndoStack)
 * to ensure all graph modifications can be reversed via Ctrl+Z. All operations
 * update both the editor canvas state and the template model.
 *
 * Key responsibilities:
 *   - Node allocation and ID assignment
 *   - Type-specific data pin initialization (MathOp, GetBBValue, SetBBValue)
 *   - Command system integration for undo/redo
 *   - Canvas state synchronization
 *   - Link graph rebuilding after modifications
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
// Node Management Methods
// ============================================================================

/**
 * @brief Create a new node and add it to the editor canvas and template graph.
 * 
 * Allocates a new unique node ID, creates a TaskNodeDefinition with type-specific
 * initialization, and pushes an AddNodeCommand to the undo stack for reversibility.
 * 
 * For special node types, this method initializes data pins:
 * - **MathOp**: Creates input pins "A" and "B" (Float type), output pin "Result"
 * - **GetBBValue**: Creates output pin "Value" (type determined by variable selection)
 * - **SetBBValue**: Creates input pin "Value" (type determined by variable target)
 * 
 * **EntryPoint Special Case**: If this is the first EntryPoint created, it is marked
 * as the template's entry and root node (m_template.EntryPointID and m_template.RootNodeID).
 * 
 * **Position Validation**: Non-finite or out-of-range coordinates are clamped to
 * prevent JSON corruption or viewport issues. Clamped coordinates default to (0, 0).
 * 
 * **Undo/Redo Integration**: The node is added to both the editor canvas (m_editorNodes)
 * and the template model via AddNodeCommand, ensuring Ctrl+Z removes the node.
 * 
 * @param type The TaskNodeType to create (EntryPoint, Branch, MathOp, etc.)
 * @param x Canvas X position (will be clamped to [-100000, 100000])
 * @param y Canvas Y position (will be clamped to [-100000, 100000])
 * 
 * @return int Allocated node ID (guaranteed unique for this editor session)
 * 
 * @note Phase 24.2: Data pins for MathOp/GetBBValue/SetBBValue initialized on creation
 * @note EntryPoint nodes are singleton — multiple creates set the same root node
 * @note Position parameters are stored in Parameters["__posX"] and Parameters["__posY"]
 *       for redo operations to restore the original placement
 * @note Sets m_dirty = true and m_verificationDone = false to trigger re-verification
 * 
 * @see RemoveNode(), AllocNodeID(), AddNodeCommand, TaskNodeDefinition
 */
int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    // Validate incoming position parameters to prevent garbage values
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        SYSTEM_LOG << "[VSEditor] AddNode: warning - non-finite position provided (x=" 
                   << x << ", y=" << y << "), resetting to (0, 0)\n";
        x = 0.0f;
        y = 0.0f;
    }

    // Clamp to a reasonable range to prevent extreme coordinate values
    if (x < -100000.0f || x > 100000.0f) x = 0.0f;
    if (y < -100000.0f || y > 100000.0f) y = 0.0f;

    int newID = AllocNodeID();

    TaskNodeDefinition def;
    def.NodeID   = newID;
    def.Type     = type;
    def.NodeName = GetNodeTypeLabel(type);

    // EntryPoint is special
    if (type == TaskNodeType::EntryPoint && m_template.EntryPointID == NODE_INDEX_NONE)
    {
        m_template.EntryPointID = newID;
        m_template.RootNodeID   = newID;
    }

    // Phase 24 FIX: Initialize DataPins for MathOp, GetBBValue, SetBBValue nodes
    // Ensures data pins are rendered correctly with proper offsets
    if (type == TaskNodeType::MathOp)
    {
        // Add input pins A and B
        DataPinDefinition pinA;
        pinA.PinName = "A";
        pinA.Dir     = DataPinDir::Input;
        pinA.PinType = VariableType::Float;
        def.DataPins.push_back(pinA);

        DataPinDefinition pinB;
        pinB.PinName = "B";
        pinB.Dir     = DataPinDir::Input;
        pinB.PinType = VariableType::Float;
        def.DataPins.push_back(pinB);

        // Add output pin Result
        DataPinDefinition pinResult;
        pinResult.PinName = "Result";
        pinResult.Dir     = DataPinDir::Output;
        pinResult.PinType = VariableType::Float;
        def.DataPins.push_back(pinResult);

        // Phase 24 Milestone 2: Initialize MathOpRef with default operands
        // left = Const "0", operator = "+", right = Const "0"
        def.mathOpRef.leftOperand.mode = MathOpOperand::Mode::Const;
        def.mathOpRef.leftOperand.constValue = "0";
        def.mathOpRef.mathOperator = "+";
        def.mathOpRef.rightOperand.mode = MathOpOperand::Mode::Const;
        def.mathOpRef.rightOperand.constValue = "0";
    }
    else if (type == TaskNodeType::GetBBValue)
    {
        // Phase 24 Milestone 3: GetBBValue outputs a data pin (Value)
        DataPinDefinition pinValue;
        pinValue.PinName = "Value";
        pinValue.Dir     = DataPinDir::Output;
        pinValue.PinType = VariableType::None;  // Type determined by selected variable
        def.DataPins.push_back(pinValue);
    }
    else if (type == TaskNodeType::SetBBValue)
    {
        // Phase 24 Milestone 3: SetBBValue inputs a data pin (Value)
        DataPinDefinition pinValue;
        pinValue.PinName = "Value";
        pinValue.Dir     = DataPinDir::Input;
        pinValue.PinType = VariableType::None;  // Type determined by target variable
        def.DataPins.push_back(pinValue);
    }

    // Persist the spawn position in Parameters so that redo (re-executing
    // AddNodeCommand) restores the node at its original position rather than
    // falling back to the default grid layout.
    {
        ParameterBinding bx, by;
        bx.Type         = ParameterBindingType::Literal;
        bx.LiteralValue = TaskValue(x);
        by.Type         = ParameterBindingType::Literal;
        by.LiteralValue = TaskValue(y);
        def.Parameters["__posX"] = bx;
        def.Parameters["__posY"] = by;
    }

    // Editor-side node (tracks canvas position independently of the template)
    VSEditorNode eNode;
    eNode.nodeID = newID;
    eNode.posX   = x;
    eNode.posY   = y;
    eNode.def    = def;
    m_editorNodes.push_back(eNode);

    // Command adds the node to m_template.Nodes and rebuilds the lookup cache
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new AddNodeCommand(def)),
        m_template);

    m_dirty = true;
    m_verificationDone = false;
    return newID;
}

/**
 * @brief Remove a node from the editor canvas and template graph.
 * 
 * Deletes the node by ID from both the editor canvas state (m_editorNodes) and
 * the template model (via DeleteNodeCommand). All execution and data connections
 * involving this node are automatically removed by the command system.
 * 
 * **Undo/Redo Integration**: The deletion is pushed to the undo stack as a
 * DeleteNodeCommand, allowing Ctrl+Z to restore the node and its connections.
 * 
 * **Link Rebuilding**: After node removal, RebuildLinks() is called to ensure
 * all remaining connections are valid and ImNodes is synchronized with the new
 * canvas state.
 * 
 * @param nodeID The ID of the node to remove (must exist in m_editorNodes)
 * 
 * @note Sets m_dirty = true and m_verificationDone = false to trigger verification
 * @note Removing a node also removes all its incoming and outgoing connections
 * @note If EntryPoint is removed, m_template.EntryPointID is reset by DeleteNodeCommand
 * 
 * @see AddNode(), DeleteNodeCommand, RebuildLinks()
 */
void VisualScriptEditorPanel::RemoveNode(int nodeID)
{
    // Remove from editor nodes (canvas-side)
    m_editorNodes.erase(
        std::remove_if(m_editorNodes.begin(), m_editorNodes.end(),
                       [nodeID](const VSEditorNode& n) { return n.nodeID == nodeID; }),
        m_editorNodes.end());

    // Command removes the node + all associated connections from m_template
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new DeleteNodeCommand(nodeID)),
        m_template);

    RebuildLinks();
    m_dirty = true;
    m_verificationDone = false;
}

/**
 * @brief Create an execution pin connection (flow link) between two nodes.
 * 
 * Connects an output execution pin on a source node to an input execution pin on
 * a destination node. Execution connections control the **control flow** (order of
 * execution) in the task graph.
 * 
 * This method validates neither the source nor destination — validation is handled
 * by CanCreateLink() or other connection validation logic in RenderCanvas().
 * 
 * **Undo/Redo Integration**: The connection is pushed to the undo stack as an
 * AddConnectionCommand, enabling Ctrl+Z to remove the link.
 * 
 * **Link Rebuilding**: After creating the connection, RebuildLinks() synchronizes
 * ImNodes with the new template state so the visual link appears on screen.
 * 
 * **Pin Naming**: Pin names are semantic strings:
 * - For exec-out: "Out", "Then", "Else", "Loop", "Loop Body", "Completed", etc.
 * - For exec-in: "In" (most nodes) or "Loop" (While loops)
 * - Dynamic pins (VSSequence/Switch): "Out_2", "Out_3", "Case_1", "Case_2", etc.
 * 
 * @param srcNodeID ID of the source node (must be valid)
 * @param srcPinName Name of the output execution pin (must match GetExecOutputPins)
 * @param dstNodeID ID of the destination node (must be valid)
 * @param dstPinName Name of the input execution pin (must match GetExecInputPins)
 * 
 * @note Sets m_dirty = true and m_verificationDone = false to trigger verification
 * @note The connection is stored in m_template.ExecConnections
 * @note Pin names are case-sensitive and must exactly match node definitions
 * 
 * @see ConnectData(), GetExecOutputPins(), GetExecInputPins(), AddConnectionCommand
 */
void VisualScriptEditorPanel::ConnectExec(int srcNodeID,
                                          const std::string& srcPinName,
                                          int dstNodeID,
                                          const std::string& dstPinName)
{
    ExecPinConnection conn;
    conn.SourceNodeID  = srcNodeID;
    conn.SourcePinName = srcPinName;
    conn.TargetNodeID  = dstNodeID;
    conn.TargetPinName = dstPinName;
    // Push to undo stack so link creation can be reversed via Ctrl+Z.
    // AddConnectionCommand::Execute() calls graph.ExecConnections.push_back().
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new AddConnectionCommand(conn)),
        m_template);
    RebuildLinks();
    m_dirty = true;
    m_verificationDone = false;
}

/**
 * @brief Create a data pin connection (value link) between two nodes.
 * 
 * Connects a data output pin on a source node to a data input pin on a destination
 * node. Data connections carry **computed or retrieved values** (not control flow).
 * 
 * **Data-Pure Nodes**: Phase 24 introduces data-pure nodes that have no execution
 * pins but only data connections:
 * - **GetBBValue**: Retrieves a blackboard variable → outputs "Value"
 * - **MathOp**: Computes arithmetic result → inputs "A", "B"; outputs "Result"
 * 
 * These nodes can be chained to build data flow networks independent of control flow.
 * 
 * **Dynamic Data Pins**: Branch nodes have dynamic input pins (Pin-in) derived from
 * their condition definitions. Condition names become pin names for data connections.
 * 
 * **Undo/Redo Integration**: The connection is pushed as an AddDataConnectionCommand,
 * enabling Ctrl+Z to remove the link.
 * 
 * **Pin Naming**: Pin names are semantic strings:
 * - Standard outputs: "Value" (GetBBValue), "Result" (MathOp)
 * - Standard inputs: "Value" (SetBBValue), "A"/"B" (MathOp)
 * - Dynamic pins (Branch): Condition names (e.g., "IsAlive", "HasAmmo")
 * 
 * @param srcNodeID ID of the source node (must be valid)
 * @param srcPinName Name of the data output pin (must match GetDataOutputPins or dynamic pins)
 * @param dstNodeID ID of the destination node (must be valid)
 * @param dstPinName Name of the data input pin (must match GetDataInputPins or dynamic pins)
 * 
 * @note Sets m_dirty = true and m_verificationDone = false to trigger verification
 * @note The connection is stored in m_template.DataConnections
 * @note Pin names are case-sensitive and must exactly match node definitions
 * @note Phase 24: Data-pure nodes enable functional data flow composition
 * 
 * @see ConnectExec(), GetDataOutputPins(), GetDataInputPins(), AddDataConnectionCommand
 */
void VisualScriptEditorPanel::ConnectData(int srcNodeID,
                                          const std::string& srcPinName,
                                          int dstNodeID,
                                          const std::string& dstPinName)
{
    DataPinConnection conn;
    conn.SourceNodeID  = srcNodeID;
    conn.SourcePinName = srcPinName;
    conn.TargetNodeID  = dstNodeID;
    conn.TargetPinName = dstPinName;
    // Push to undo stack so data link creation can be reversed via Ctrl+Z.
    // AddDataConnectionCommand::Execute() calls graph.DataConnections.push_back().
    m_undoStack.PushCommand(
        std::unique_ptr<ICommand>(new AddDataConnectionCommand(conn)),
        m_template);
    RebuildLinks();
    m_dirty = true;
    m_verificationDone = false;
}

}  // namespace Olympe
