/**
 * @file VisualScriptEditorPanel_Interaction.cpp
 * @brief Node manipulation, drag-drop, and Undo/Redo functionality for VS Editor Panel.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Part of the VisualScriptEditorPanel refactoring (Phase 1).
 * This file contains:
 * - Node creation (AddNode)
 * - Node deletion (RemoveNode)
 * - Undo/Redo operations (PerformUndo, PerformRedo)
 * - Drag & drop state management
 * - Dynamic pin operations
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
#include "AtomicTaskUIRegistry.h"
#include "OperatorRegistry.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"

#include <algorithm>
#include <cmath>

namespace Olympe {

// ============================================================================
// Node Management: AddNode / RemoveNode
// ============================================================================

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

// ============================================================================
// Undo / Redo
// ============================================================================

void VisualScriptEditorPanel::PerformUndo()
{
    m_undoStack.Undo(m_template);
    // After undo, rebuild the canvas from the reverted template state
    SyncEditorNodesFromTemplate();
    RebuildLinks();
    // Flag to skip position sync on this frame (let new positions render before extracting)
    m_skipPositionSyncNextFrame = true;
    m_justPerformedUndoRedo = true;
    m_dirty = true;
    m_verificationDone = false;
}

void VisualScriptEditorPanel::PerformRedo()
{
    m_undoStack.Redo(m_template);
    // After redo, rebuild the canvas from the re-applied template state
    SyncEditorNodesFromTemplate();
    RebuildLinks();
    // Flag to skip position sync on this frame (let new positions render before extracting)
    m_skipPositionSyncNextFrame = true;
    m_justPerformedUndoRedo = true;
    m_dirty = true;
    m_verificationDone = false;
}

} // namespace Olympe
