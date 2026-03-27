/**
 * @file VisualScriptEditorPanel.cpp
 * @brief ImNodes graph editor implementation for ATS VS graphs (Phase 5).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details C++14 compliant — no std::optional, structured bindings, std::filesystem.
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

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

VisualScriptEditorPanel::VisualScriptEditorPanel()
{
    std::memset(m_saveAsFilename, 0, sizeof(m_saveAsFilename));
    strcpy_s(m_saveAsFilename, sizeof(m_saveAsFilename), "untitled_graph");
}

VisualScriptEditorPanel::~VisualScriptEditorPanel()
{
}

void VisualScriptEditorPanel::Initialize()
{
    // Create a dedicated ImNodes editor context for this panel instance.
    // This ensures that node positions and canvas panning are tracked
    // independently for each open tab (switching tabs preserves layout).
    m_imnodesContext = ImNodes::EditorContextCreate();

    // Phase 24 — Condition Preset UI: create helpers bound to m_presetRegistry.
    m_pinManager      = std::unique_ptr<DynamicDataPinManager>(
                            new DynamicDataPinManager(m_presetRegistry));
    m_branchRenderer  = std::unique_ptr<NodeBranchRenderer>(
                            new NodeBranchRenderer(m_presetRegistry, *m_pinManager));
    m_conditionsPanel = std::unique_ptr<NodeConditionsPanel>(
                            new NodeConditionsPanel(m_presetRegistry));
    m_mathOpPanel     = std::unique_ptr<MathOpPropertyPanel>(
                            new MathOpPropertyPanel(m_presetRegistry, *m_pinManager));
    m_getBBPanel      = std::unique_ptr<GetBBValuePropertyPanel>(
                            new GetBBValuePropertyPanel());
    m_setBBPanel      = std::unique_ptr<SetBBValuePropertyPanel>(
                            new SetBBValuePropertyPanel());
    m_libraryPanel    = std::unique_ptr<ConditionPresetLibraryPanel>(
                            new ConditionPresetLibraryPanel(m_presetRegistry));

    // Phase 24 Global Blackboard Integration: Create EntityBlackboard for managing
    // both local and global variables in the editor context (entity ID 0)
    m_entityBlackboard = std::unique_ptr<EntityBlackboard>(
                            new EntityBlackboard(0));  // 0 = editor context entity

    // Wire the pin-regeneration callback so the Edit-Conditions modal can
    // trigger a canvas update when the user confirms changes.
    m_conditionsPanel->OnDynamicPinsNeedRegeneration = [this]()
    {
        if (m_selectedNodeID < 0)
            return;
        for (size_t ni = 0; ni < m_editorNodes.size(); ++ni)
        {
            VSEditorNode& eNode = m_editorNodes[ni];
            if (eNode.nodeID != m_selectedNodeID)
                continue;

            // Phase 24: Get FRESH condition data from panel (not stale data from eNode)
            // This ensures that edits via RenderConditionList dropdown are picked up
            std::vector<NodeConditionRef> freshConditionRefs = m_conditionsPanel->GetConditionRefs();
            std::vector<ConditionRef> freshOperandRefs = m_conditionsPanel->GetConditionOperandRefs();

            // Sync fresh data to eNode
            eNode.def.conditionRefs = freshConditionRefs;
            eNode.def.conditionOperandRefs = freshOperandRefs;

            // Regenerate pins with FRESH operand data
            m_pinManager->RegeneratePinsFromConditions(freshConditionRefs, freshOperandRefs);
            eNode.def.dynamicPins = m_pinManager->GetAllPins();

            // Keep m_template in sync for serialization.
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].conditionRefs = eNode.def.conditionRefs;
                    m_template.Nodes[ti].conditionOperandRefs = eNode.def.conditionOperandRefs;
                    m_template.Nodes[ti].dynamicPins   = eNode.def.dynamicPins;
                    break;
                }
            }
            m_conditionsPanel->SetDynamicPins(eNode.def.dynamicPins);
            m_dirty = true;
            break;
        }
    };

    // Phase 24 — Load presets from the graph (now embedded in blueprint JSON)
    // instead of from an external file. This makes each blueprint self-contained.
    // If the graph has presets, populate the registry; otherwise leave empty.
    if (!m_template.Presets.empty())
    {
        m_presetRegistry.LoadFromPresetList(m_template.Presets);
        SYSTEM_LOG << "[VSEditor] Initialize: loaded " << m_template.Presets.size()
                   << " presets from graph '" << m_template.Name << "'\n";
    }
    else
    {
        SYSTEM_LOG << "[VSEditor] Initialize: graph '" << m_template.Name
                   << "' has no embedded presets\n";
    }
}

void VisualScriptEditorPanel::Shutdown()
{
    if (m_imnodesContext)
    {
        ImNodes::EditorContextFree(m_imnodesContext);
        m_imnodesContext = nullptr;
    }
    m_editorNodes.clear();
    m_editorLinks.clear();
    m_positionedNodes.clear();

    // Phase 24 — release helpers before registry is destroyed.
    m_conditionsPanel.reset();
    m_mathOpPanel.reset();
    m_getBBPanel.reset();
    m_setBBPanel.reset();
    m_libraryPanel.reset();
    m_branchRenderer.reset();
    m_pinManager.reset();
    m_condPanelNodeID = -1;
}

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
        case TaskNodeType::GetBBValue:  return {"Out"};
        case TaskNodeType::SetBBValue:  return {"Out"};
        case TaskNodeType::MathOp:      return {"Out"};
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
// Node management
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

// ============================================================================
// Template / canvas sync
// ============================================================================

void VisualScriptEditorPanel::SyncCanvasFromTemplate()
{
    m_editorNodes.clear();
    m_positionedNodes.clear();
    m_nextNodeID = 1;

    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& def = m_template.Nodes[i];

        VSEditorNode eNode;
        eNode.nodeID = def.NodeID;
        eNode.def    = def;

        // Use position loaded from JSON if available; otherwise fall back to auto-layout.
        if (def.HasEditorPos)
        {
            // Validate loaded position to prevent garbage values from corrupted JSON
            if (std::isfinite(def.EditorPosX) && std::isfinite(def.EditorPosY) &&
                def.EditorPosX >= -100000.0f && def.EditorPosX <= 100000.0f &&
                def.EditorPosY >= -100000.0f && def.EditorPosY <= 100000.0f)
            {
                eNode.posX = def.EditorPosX;
                eNode.posY = def.EditorPosY;
            }
            else
            {
                SYSTEM_LOG << "[VSEditor] SyncCanvasFromTemplate: node #" << def.NodeID 
                           << " had garbage position (" << def.EditorPosX << ", " << def.EditorPosY 
                           << "), using auto-layout\n";
                eNode.posX = 200.0f * static_cast<float>(i);
                eNode.posY = 100.0f;
            }
        }
        else
        {
            eNode.posX = 200.0f * static_cast<float>(i);  // Default auto-layout
            eNode.posY = 100.0f;
        }

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

        // Phase 24: Regenerate dynamic pins for Branch nodes after load
        // This ensures Pin-in connectors are available even if they weren't saved
        // (they are derived from conditionRefs/conditionOperandRefs)
        if (def.Type == TaskNodeType::Branch && (!def.conditionRefs.empty() || !def.conditionOperandRefs.empty()))
        {
            m_pinManager->RegeneratePinsFromConditions(eNode.def.conditionRefs,
                                                       eNode.def.conditionOperandRefs);
            eNode.def.dynamicPins = m_pinManager->GetAllPins();

            // Also update template for consistency
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == eNode.nodeID)
                {
                    m_template.Nodes[ti].dynamicPins = eNode.def.dynamicPins;
                    break;
                }
            }
        }

        m_editorNodes.push_back(eNode);
    }

    RebuildLinks();
    // Request position restore on the next RenderCanvas() call so that
    // ImNodes places each node at its stored (posX, posY) coordinates.
    m_needsPositionSync = true;
}

void VisualScriptEditorPanel::SyncTemplateFromCanvas()
{
    // Update template nodes from editor nodes
    m_template.Nodes.clear();
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        m_template.Nodes.push_back(m_editorNodes[i].def);
    }
    m_template.BuildLookupCache();
}

void VisualScriptEditorPanel::RebuildLinks()
{
    m_editorLinks.clear();

    // Exec links
    for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = m_template.ExecConnections[i];

        // Determine pin index for source
        std::vector<std::string> outPins;
        const TaskNodeDefinition* srcNode = m_template.GetNode(conn.SourceNodeID);
        if (srcNode != nullptr)
        {
            outPins = GetExecOutputPins(srcNode->Type);
            if (srcNode->Type == TaskNodeType::VSSequence ||
                srcNode->Type == TaskNodeType::Switch)
            {
                for (size_t d = 0; d < srcNode->DynamicExecOutputPins.size(); ++d)
                    outPins.push_back(srcNode->DynamicExecOutputPins[d]);
            }
        }

        int pinIdx = 0;
        for (size_t p = 0; p < outPins.size(); ++p)
        {
            if (outPins[p] == conn.SourcePinName)
            {
                pinIdx = static_cast<int>(p);
                break;
            }
        }

        VSEditorLink link;
        link.linkID    = AllocLinkID();
        link.srcAttrID = ExecOutAttrUID(conn.SourceNodeID, pinIdx);
        link.dstAttrID = ExecInAttrUID(conn.TargetNodeID);
        link.isData    = false;
        m_editorLinks.push_back(link);
    }

    // Data links — resolve pin indices from stored pin names
    for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = m_template.DataConnections[i];

        // Determine data-out pin index for source
        int srcPinIdx = 0;
        const TaskNodeDefinition* srcNode = m_template.GetNode(conn.SourceNodeID);
        if (srcNode != nullptr)
        {
            // Try static list first
            auto outPins = GetDataOutputPins(srcNode->Type);
            bool found = false;
            for (size_t p = 0; p < outPins.size(); ++p)
            {
                if (outPins[p] == conn.SourcePinName)
                {
                    srcPinIdx = static_cast<int>(p);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                // Fall back to DataPins vector
                int outIdx = 0;
                for (size_t p = 0; p < srcNode->DataPins.size(); ++p)
                {
                    if (srcNode->DataPins[p].Dir == DataPinDir::Output)
                    {
                        if (srcNode->DataPins[p].PinName == conn.SourcePinName)
                        {
                            srcPinIdx = outIdx;
                            break;
                        }
                        ++outIdx;
                    }
                }
            }
        }

        // Determine data-in pin index for destination
        int dstPinIdx = 0;
        const TaskNodeDefinition* dstNode = m_template.GetNode(conn.TargetNodeID);
        if (dstNode != nullptr)
        {
            // Phase 24: Check if destination is a Branch node with dynamic pins
            bool foundDynamicPin = false;
            if (dstNode->Type == TaskNodeType::Branch && !dstNode->dynamicPins.empty())
            {
                // Try to find a matching dynamic pin ID
                for (size_t p = 0; p < dstNode->dynamicPins.size(); ++p)
                {
                    if (dstNode->dynamicPins[p].id == conn.TargetPinName)
                    {
                        dstPinIdx = static_cast<int>(p);
                        foundDynamicPin = true;
                        break;
                    }
                }
            }

            if (!foundDynamicPin)
            {
                // Fall back to static data pins
                auto inPins = GetDataInputPins(dstNode->Type);
                bool found = false;
                for (size_t p = 0; p < inPins.size(); ++p)
                {
                    if (inPins[p] == conn.TargetPinName)
                    {
                        dstPinIdx = static_cast<int>(p);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    int inIdx = 0;
                    for (size_t p = 0; p < dstNode->DataPins.size(); ++p)
                    {
                        if (dstNode->DataPins[p].Dir == DataPinDir::Input)
                        {
                            if (dstNode->DataPins[p].PinName == conn.TargetPinName)
                            {
                                dstPinIdx = inIdx;
                                break;
                            }
                            ++inIdx;
                        }
                    }
                }
            }
        }

        VSEditorLink link;
        link.linkID    = AllocLinkID();
        link.srcAttrID = DataOutAttrUID(conn.SourceNodeID, srcPinIdx);
        link.dstAttrID = DataInAttrUID(conn.TargetNodeID,  dstPinIdx);
        link.isData    = true;
        m_editorLinks.push_back(link);
    }
}

// ============================================================================
// Undo/Redo helpers
// ============================================================================

void VisualScriptEditorPanel::SyncEditorNodesFromTemplate()
{
    // Default grid spacing used when a node has no recorded canvas position
    // (e.g. after a Redo restores a previously-deleted node).
    static const float DEFAULT_NODE_X_OFFSET  =  50.0f;
    static const float DEFAULT_NODE_X_SPACING = 200.0f;
    static const float DEFAULT_NODE_Y         = 100.0f;

    // Preserve canvas positions for nodes that still exist
    std::unordered_map<int, std::pair<float, float> > savedPos;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        float posX = m_editorNodes[i].posX;
        float posY = m_editorNodes[i].posY;

        // Validate saved positions before preserving them
        if (!std::isfinite(posX) || !std::isfinite(posY) ||
            posX < -100000.0f || posX > 100000.0f ||
            posY < -100000.0f || posY > 100000.0f)
        {
            posX = DEFAULT_NODE_X_OFFSET;
            posY = DEFAULT_NODE_Y;
            SYSTEM_LOG << "[VSEditor] SyncEditorNodesFromTemplate: node #" << m_editorNodes[i].nodeID 
                       << " had garbage position, reset to defaults\n";
        }

        savedPos[m_editorNodes[i].nodeID] = std::make_pair(posX, posY);
    }

    m_editorNodes.clear();
    m_positionedNodes.clear();
    // Clear drag-start positions on undo/redo: any in-progress drag is
    // invalidated when the graph state changes underneath it.  The user must
    // re-drag after undo/redo; the old drag-start is no longer meaningful.
    m_nodeDragStartPositions.clear();

    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& def = m_template.Nodes[i];

        VSEditorNode eNode;
        eNode.nodeID = def.NodeID;
        eNode.def    = def;

        // Prefer position stored in template Parameters by MoveNodeCommand
        // (reflects the undo/redo target state for this node).
        auto posXIt = def.Parameters.find("__posX");
        auto posYIt = def.Parameters.find("__posY");
        if (posXIt != def.Parameters.end() &&
            posYIt != def.Parameters.end() &&
            posXIt->second.Type == ParameterBindingType::Literal &&
            posYIt->second.Type == ParameterBindingType::Literal)
        {
            float paramX = posXIt->second.LiteralValue.AsFloat();
            float paramY = posYIt->second.LiteralValue.AsFloat();

            // Validate parameter positions
            if (std::isfinite(paramX) && std::isfinite(paramY) &&
                paramX >= -100000.0f && paramX <= 100000.0f &&
                paramY >= -100000.0f && paramY <= 100000.0f)
            {
                eNode.posX = paramX;
                eNode.posY = paramY;
            }
            else
            {
                SYSTEM_LOG << "[VSEditor] SyncEditorNodesFromTemplate: node #" << def.NodeID 
                           << " had garbage params (" << paramX << ", " << paramY 
                           << "), falling back\n";
                auto it = savedPos.find(def.NodeID);
                if (it != savedPos.end())
                {
                    eNode.posX = it->second.first;
                    eNode.posY = it->second.second;
                }
                else if (def.HasEditorPos)
                {
                    eNode.posX = def.EditorPosX;
                    eNode.posY = def.EditorPosY;
                }
                else
                {
                    eNode.posX = DEFAULT_NODE_X_OFFSET + DEFAULT_NODE_X_SPACING * static_cast<float>(i);
                    eNode.posY = DEFAULT_NODE_Y;
                }
            }
        }
        else
        {
            auto it = savedPos.find(def.NodeID);
            if (it != savedPos.end())
            {
                // Restore previously known position
                eNode.posX = it->second.first;
                eNode.posY = it->second.second;
            }
            else if (def.HasEditorPos)
            {
                // Validate file-loaded position
                if (std::isfinite(def.EditorPosX) && std::isfinite(def.EditorPosY) &&
                    def.EditorPosX >= -100000.0f && def.EditorPosX <= 100000.0f &&
                    def.EditorPosY >= -100000.0f && def.EditorPosY <= 100000.0f)
                {
                    eNode.posX = def.EditorPosX;
                    eNode.posY = def.EditorPosY;
                }
                else
                {
                    eNode.posX = DEFAULT_NODE_X_OFFSET + DEFAULT_NODE_X_SPACING * static_cast<float>(i);
                    eNode.posY = DEFAULT_NODE_Y;
                }
            }
            else
            {
                // New node (e.g. restored by Redo) – use a default spread position
                eNode.posX = DEFAULT_NODE_X_OFFSET + DEFAULT_NODE_X_SPACING * static_cast<float>(i);
                eNode.posY = DEFAULT_NODE_Y;
            }
        }

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

        SYSTEM_LOG << "[VSEditor] SyncEditorNodesFromTemplate: node #" << eNode.nodeID
                   << " restored to (" << eNode.posX << "," << eNode.posY << ")\n";

        m_editorNodes.push_back(eNode);
    }

    // FIX 1: Rebuild links from template so that ghost links (links that
    // belong to a deleted node) are removed from m_editorLinks after undo/redo.
    RebuildLinks();

    // Request a position-restore pass on the next RenderCanvas() call
    m_needsPositionSync = true;
}

void VisualScriptEditorPanel::RemoveLink(int linkID)
{
    // Find the link descriptor
    VSEditorLink* link = nullptr;
    for (size_t i = 0; i < m_editorLinks.size(); ++i)
    {
        if (m_editorLinks[i].linkID == linkID)
        {
            link = &m_editorLinks[i];
            break;
        }
    }
    if (!link)
        return;

    if (link->isData)
    {
        // Decode data-out → data-in
        int srcNodeID = link->srcAttrID / 10000;
        int srcPinIdx = link->srcAttrID % 10000 - 300; // data-out range 300-399
        int dstNodeID = link->dstAttrID / 10000;
        int dstPinIdx = link->dstAttrID % 10000 - 200; // data-in  range 200-299

        std::string srcPinName = "Value";
        std::string dstPinName = "Value";

        const TaskNodeDefinition* srcNode = m_template.GetNode(srcNodeID);
        const TaskNodeDefinition* dstNode = m_template.GetNode(dstNodeID);

        if (srcNode)
        {
            auto pins = GetDataOutputPins(srcNode->Type);
            if (srcPinIdx >= 0 && srcPinIdx < static_cast<int>(pins.size()))
                srcPinName = pins[static_cast<size_t>(srcPinIdx)];
        }
        if (dstNode)
        {
            auto pins = GetDataInputPins(dstNode->Type);
            if (dstPinIdx >= 0 && dstPinIdx < static_cast<int>(pins.size()))
                dstPinName = pins[static_cast<size_t>(dstPinIdx)];
        }

        DataPinConnection conn;
        conn.SourceNodeID  = srcNodeID;
        conn.SourcePinName = srcPinName;
        conn.TargetNodeID  = dstNodeID;
        conn.TargetPinName = dstPinName;
        m_undoStack.PushCommand(
            std::unique_ptr<ICommand>(new DeleteLinkCommand(conn)),
            m_template);
    }
    else
    {
        // Decode exec-out → exec-in
        int srcNodeID = link->srcAttrID / 10000;
        int srcPinIdx = link->srcAttrID % 10000 - 100; // exec-out range 100-199
        int dstNodeID = link->dstAttrID / 10000;
        int dstPinIdx = link->dstAttrID % 10000;        // exec-in  range 0-99

        std::string srcPinName = "Out";
        std::string dstPinName = "In";

        const TaskNodeDefinition* srcNode = m_template.GetNode(srcNodeID);
        if (srcNode)
        {
            auto pins = GetExecOutputPins(srcNode->Type);
            if (srcNode->Type == TaskNodeType::VSSequence ||
                srcNode->Type == TaskNodeType::Switch)
            {
                for (size_t d = 0; d < srcNode->DynamicExecOutputPins.size(); ++d)
                    pins.push_back(srcNode->DynamicExecOutputPins[d]);
            }
            if (srcPinIdx >= 0 && srcPinIdx < static_cast<int>(pins.size()))
                srcPinName = pins[static_cast<size_t>(srcPinIdx)];
        }

        const TaskNodeDefinition* dstNode = m_template.GetNode(dstNodeID);
        if (dstNode)
        {
            auto pins = GetExecInputPins(dstNode->Type);
            if (dstPinIdx >= 0 && dstPinIdx < static_cast<int>(pins.size()))
                dstPinName = pins[static_cast<size_t>(dstPinIdx)];
        }

        ExecPinConnection conn;
        conn.SourceNodeID  = srcNodeID;
        conn.SourcePinName = srcPinName;
        conn.TargetNodeID  = dstNodeID;
        conn.TargetPinName = dstPinName;
        m_undoStack.PushCommand(
            std::unique_ptr<ICommand>(new DeleteLinkCommand(conn)),
            m_template);
    }

    RebuildLinks();
    m_dirty = true;
    m_verificationDone = false;
}

// ============================================================================
// Load / Save
// ============================================================================

void VisualScriptEditorPanel::LoadTemplate(const TaskGraphTemplate* tmpl,
                                           const std::string& path)
{
    if (tmpl == nullptr)
        return;

    m_template    = *tmpl;
    m_currentPath = path;
    m_dirty       = false;

    // Rebuild lookup cache after copy (pointers from old template are now invalid)
    m_template.BuildLookupCache();

    // Phase 24 — Load embedded presets from the graph
    // This replaces the old file-based approach with graph-embedded storage
    if (!m_template.Presets.empty())
    {
        m_presetRegistry.LoadFromPresetList(m_template.Presets);
        SYSTEM_LOG << "[VSEditor] LoadTemplate: loaded " << m_template.Presets.size()
                   << " presets from graph '" << m_template.Name << "'\n";
    }
    else
    {
        // Clear registry if graph has no presets (fresh start)
        m_presetRegistry.Clear();
        SYSTEM_LOG << "[VSEditor] LoadTemplate: graph '" << m_template.Name
                   << "' has no embedded presets - starting with empty bank\n";
    }

    // Phase 24 Global Blackboard Integration: Initialize EntityBlackboard
    // This merges local (from m_template.Blackboard) + global variables (from registry)

    // Reload global variables from registry (in case they were modified outside this editor instance)
    GlobalTemplateBlackboard::Reload();

    if (m_entityBlackboard)
    {
        m_entityBlackboard->Initialize(m_template);
        SYSTEM_LOG << "[VSEditor] LoadTemplate: initialized EntityBlackboard with "
                   << m_entityBlackboard->GetLocalVariableCount() << " local + "
                   << m_entityBlackboard->GetGlobalVariableCount() << " global variables\n";

        // Phase 24 Global Blackboard Integration: Restore entity-specific global variable values
        // If the graph has stored global variable overrides, restore them now
        if (!m_template.GlobalVariableValues.is_null() && !m_template.GlobalVariableValues.empty())
        {
            m_entityBlackboard->ImportGlobalsFromJson(m_template.GlobalVariableValues);
            SYSTEM_LOG << "[VSEditor] LoadTemplate: restored global variable overrides from graph\n";
        }
    }

    // NOTE: Do NOT clear the undo stack here.  Each VisualScriptEditorPanel
    // instance owns its own stack (one per tab), so there is no cross-tab
    // contamination.  Preserving the stack lets the user undo edits made
    // before saving and reloading, and is required for undo to function
    // correctly after opening a file from the Blueprint Files browser.

    SyncCanvasFromTemplate();

    // Phase 18: Do NOT pre-populate m_nodeDragStartPositions here.
    // The former "FIX 2" block pre-populated every node's drag-start position
    // with its loaded position.  Because the guard in the drag-tracking loop is
    // "insert only if key is absent", the pre-populated value was never
    // overwritten.  On the first drag after load the key already existed, so no
    // new start position was recorded — eNode.posX/Y (kept current each frame
    // while mouseDown) serves as the correct "position before this drag" and is
    // used when the key is absent.  Keeping m_nodeDragStartPositions empty here
    // allows the tracking loop to record the true pre-drag position.
    m_nodeDragStartPositions.clear();
    m_verificationDone = false;
}

bool VisualScriptEditorPanel::Save()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] Save() called. m_currentPath='"
               << m_currentPath << "'\n";

    if (m_currentPath.empty())
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save() aborted: m_currentPath is empty\n";
        return false;
    }

    // BUG-003 Fix: Reset viewport panning BEFORE syncing positions so that
    // any residual editor-space offset from navigation is neutralised.
    // Positions are stored in grid space (GetNodeGridSpacePos), so this is
    // belt-and-suspenders safety; panning is restored by AfterSave().
    ResetViewportBeforeSave();

    // Fix #1: Commit any deferred key-name edits before save
    CommitPendingBlackboardEdits();

    // Fix #1: Remove invalid blackboard entries before save
    ValidateAndCleanBlackboardEntries();

    // Phase 24: CRITICAL - Sync conditions from panel to template BEFORE serialization
    // This ensures conditionRefs and conditionOperandRefs are up-to-date before save
    if (m_selectedNodeID >= 0)
    {
        for (size_t ni = 0; ni < m_editorNodes.size(); ++ni)
        {
            if (m_editorNodes[ni].nodeID == m_selectedNodeID && 
                m_editorNodes[ni].def.Type == TaskNodeType::Branch)
            {
                m_editorNodes[ni].def.conditionRefs = m_conditionsPanel->GetConditionRefs();
                m_editorNodes[ni].def.conditionOperandRefs = m_conditionsPanel->GetConditionOperandRefs();

                // Also sync to template
                for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
                {
                    if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[ti].conditionRefs = m_editorNodes[ni].def.conditionRefs;
                        m_template.Nodes[ti].conditionOperandRefs = m_editorNodes[ni].def.conditionOperandRefs;
                        break;
                    }
                }
                break;
            }
        }
    }

    // Phase 24: CRITICAL - Sync presets from registry to template BEFORE serialization
    // This ensures all presets (newly created, modified, duplicated) are included in save
    SyncPresetsFromRegistryToTemplate();

    // Phase 24 Global Blackboard Integration: Sync global variable values from EntityBlackboard to template
    // This ensures entity-specific global variable overrides are included in save
    if (m_entityBlackboard)
    {
        m_template.GlobalVariableValues = m_entityBlackboard->ExportGlobalsToJson();
    }

    // CRITICAL FIX: Sync node positions from ImNodes BEFORE serialization.
    // RenderToolbar() (which calls Save) executes before RenderCanvas() syncs
    // positions, so we must pull fresh positions here to avoid stale data.
    SyncNodePositionsFromImNodes();

    bool ok = SerializeAndWrite(m_currentPath);

    // BUG-003 Fix #5: Restore viewport so the canvas does not visually jump.
    AfterSave();

    SYSTEM_LOG << "[VisualScriptEditorPanel] Save() "
               << (ok ? "succeeded" : "FAILED") << ": '" << m_currentPath << "'\n";
    return ok;
}

bool VisualScriptEditorPanel::SaveAs(const std::string& path)
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs() called. path='" << path << "'\n";

    if (path.empty())
        return false;

    // BUG-003 Fix: Reset viewport before position sync (same as Save()).
    ResetViewportBeforeSave();

    // Fix #1: Commit and validate before save
    CommitPendingBlackboardEdits();
    ValidateAndCleanBlackboardEntries();

    // Phase 24: CRITICAL - Sync presets from registry to template BEFORE serialization
    // This ensures all presets (newly created, modified, duplicated) are included in save
    SyncPresetsFromRegistryToTemplate();

    // Phase 24 Global Blackboard Integration: Sync global variable values from EntityBlackboard to template
    // This ensures entity-specific global variable overrides are included in save
    if (m_entityBlackboard)
    {
        m_template.GlobalVariableValues = m_entityBlackboard->ExportGlobalsToJson();
    }

    // CRITICAL FIX: Same position sync as Save() — ensure fresh positions
    // before serialization regardless of when in the frame SaveAs is called.
    SyncNodePositionsFromImNodes();

    bool ok = SerializeAndWrite(path);

    // BUG-003 Fix #5: Restore viewport.
    AfterSave();

    if (ok)
    {
        m_currentPath = path;
        m_dirty       = false;
        SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs() succeeded: '" << path << "'\n";
    }
    else
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs() FAILED: '" << path << "'\n";
    }
    return ok;
}

void VisualScriptEditorPanel::SyncNodePositionsFromImNodes()
{
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        VSEditorNode& eNode = m_editorNodes[i];
        // Only query nodes that have been rendered at least once to avoid
        // an ImNodes assertion for nodes that have not yet gone through
        // BeginNode()/EndNode() this session.
        if (m_positionedNodes.count(eNode.nodeID) > 0)
        {
            // BUG-003 Fix: use GetNodeGridSpacePos() (pan-independent grid
            // coordinates) instead of GetNodeEditorSpacePos() which returns
            // Origin + Panning.  Storing grid-space positions means the saved
            // values are never corrupted by the current viewport pan offset.
            ImVec2 pos = ImNodes::GetNodeGridSpacePos(eNode.nodeID);
            eNode.posX = pos.x;
            eNode.posY = pos.y;

            // Keep the template's Parameters in sync so that
            // SyncEditorNodesFromTemplate() (called on undo/redo) can always
            // find the live canvas position in Parameters["__posX/__posY"],
            // even for nodes that were loaded from file and have never been
            // moved via an explicit MoveNodeCommand.
            for (size_t j = 0; j < m_template.Nodes.size(); ++j)
            {
                if (m_template.Nodes[j].NodeID == eNode.nodeID)
                {
                    ParameterBinding bx, by;
                    bx.Type         = ParameterBindingType::Literal;
                    bx.LiteralValue = TaskValue(pos.x);
                    by.Type         = ParameterBindingType::Literal;
                    by.LiteralValue = TaskValue(pos.y);
                    m_template.Nodes[j].Parameters["__posX"] = bx;
                    m_template.Nodes[j].Parameters["__posY"] = by;
                    break;
                }
            }
        }
    }
}

void VisualScriptEditorPanel::SyncPresetsFromRegistryToTemplate()
{
    // Phase 24 FIX: Sync ALL presets from the registry to the template
    // This ensures that presets created/modified via UI are included in the save
    // Previously, only modified presets were synced, missing newly created ones

    // Get all presets from the registry
    std::vector<std::string> allPresetIDs = m_presetRegistry.GetAllPresetIDs();

    // Clear template presets and rebuild from registry
    m_template.Presets.clear();

    for (const auto& presetID : allPresetIDs)
    {
        const ConditionPreset* preset = m_presetRegistry.GetPreset(presetID);
        if (preset)
        {
            m_template.Presets.push_back(*preset);
        }
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] SyncPresetsFromRegistryToTemplate: synced "
               << m_template.Presets.size() << " presets from registry to template\n";
}

bool VisualScriptEditorPanel::SerializeAndWrite(const std::string& path)
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: writing to '" << path << "'\n";

    SyncTemplateFromCanvas();

    json root;
    root["schema_version"] = 4;
    root["name"]           = m_template.Name;
    root["graphType"]      = "VisualScript";

    // Blackboard
    // BUG-001 Hotfix: skip invalid entries (empty key or VariableType::None)
    // to prevent save crash caused by unhandled None type during serialization.
    int bbSkipped = 0;
    json bbArray = json::array();
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[i];

        if (entry.Key.empty() || entry.Type == VariableType::None)
        {
            SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: skipping invalid blackboard entry"
                       << " (key='" << entry.Key << "', type=None)\n";
            ++bbSkipped;
            continue;
        }

        json e;
        e["key"]      = entry.Key;
        e["isGlobal"] = entry.IsGlobal;

        // Guard each accessor against type mismatch: if Default was not
        // initialised to the right type (e.g. loaded as Int when Float was
        // expected), fall back to a zero-value rather than throwing.
        switch (entry.Type)
        {
            case VariableType::Bool:
                e["type"]  = "Bool";
                e["value"] = (entry.Default.GetType() == VariableType::Bool)
                             ? entry.Default.AsBool() : false;
                break;
            case VariableType::Int:
                e["type"]  = "Int";
                e["value"] = (entry.Default.GetType() == VariableType::Int)
                             ? entry.Default.AsInt() : 0;
                break;
            case VariableType::Float:
                e["type"]  = "Float";
                e["value"] = (entry.Default.GetType() == VariableType::Float)
                             ? entry.Default.AsFloat() : 0.0f;
                break;
            case VariableType::String:
                e["type"]  = "String";
                e["value"] = (entry.Default.GetType() == VariableType::String)
                             ? entry.Default.AsString() : std::string("");
                break;
            case VariableType::EntityID:
                e["type"]  = "EntityID";
                e["value"] = std::to_string(
                             (entry.Default.GetType() == VariableType::EntityID)
                             ? entry.Default.AsEntityID() : 0);
                break;
            case VariableType::Vector:
            {
                // Vector default is auto-assigned at runtime from entity position.
                // Persist as a zero-initialised object so the type tag is preserved
                // across save/load and does not degrade to "None".
                const ::Vector v = (entry.Default.GetType() == VariableType::Vector)
                                   ? entry.Default.AsVector()
                                   : ::Vector{0.f, 0.f, 0.f};
                json vec;
                vec["x"] = v.x;
                vec["y"] = v.y;
                vec["z"] = v.z;
                e["type"]  = "Vector";
                e["value"] = vec;
                break;
            }
            default:
                e["type"]  = "None";
                e["value"] = nullptr;
                break;
        }
        bbArray.push_back(e);
    }
    if (bbSkipped > 0)
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: " << bbSkipped
                   << " invalid blackboard entries skipped (BUG-001)\n";
    }
    root["blackboard"] = bbArray;

    // Nodes
    json nodesArray = json::array();
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& def = m_template.Nodes[i];
        json n;
        n["id"]    = def.NodeID;
        n["label"] = def.NodeName;
        n["type"]  = GetNodeTypeLabel(def.Type);

        if (def.Type == TaskNodeType::AtomicTask)
            n["taskType"] = def.AtomicTaskID;
        if (def.Type == TaskNodeType::Delay)
            n["delaySeconds"] = def.DelaySeconds;
        if (!def.BBKey.empty())
            n["bbKey"] = def.BBKey;
        if (!def.SubGraphPath.empty())
            n["subGraphPath"] = def.SubGraphPath;
        if (!def.ConditionID.empty())
            n["conditionKey"] = def.ConditionID;
        if (!def.MathOperator.empty())
            n["mathOp"] = def.MathOperator;

        // Serialize parameters (AtomicTask and other node types with parameters)
        if (!def.Parameters.empty())
        {
            json paramsObj = json::object();
            for (const auto& paramPair : def.Parameters)
            {
                const std::string& paramName = paramPair.first;
                const ParameterBinding& binding = paramPair.second;

                json bindingObj = json::object();

                switch (binding.Type)
                {
                    case ParameterBindingType::Literal:
                        bindingObj["Type"] = "Literal";
                        // Serialize the literal value based on its type
                        if (!binding.LiteralValue.IsNone())
                        {
                            switch (binding.LiteralValue.GetType())
                            {
                                case VariableType::Bool:
                                    bindingObj["LiteralValue"] = binding.LiteralValue.AsBool();
                                    break;
                                case VariableType::Int:
                                    bindingObj["LiteralValue"] = binding.LiteralValue.AsInt();
                                    break;
                                case VariableType::Float:
                                    bindingObj["LiteralValue"] = binding.LiteralValue.AsFloat();
                                    break;
                                case VariableType::String:
                                    bindingObj["LiteralValue"] = binding.LiteralValue.AsString();
                                    break;
                                case VariableType::Vector:
                                {
                                    const ::Vector v = binding.LiteralValue.AsVector();
                                    json vec;
                                    vec["x"] = v.x;
                                    vec["y"] = v.y;
                                    vec["z"] = v.z;
                                    bindingObj["LiteralValue"] = vec;
                                    break;
                                }
                                case VariableType::EntityID:
                                    bindingObj["LiteralValue"] = std::to_string(binding.LiteralValue.AsEntityID());
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;

                    case ParameterBindingType::LocalVariable:
                        bindingObj["Type"] = "LocalVariable";
                        bindingObj["VariableName"] = binding.VariableName;
                        break;

                    case ParameterBindingType::AtomicTaskID:
                        bindingObj["Type"] = "AtomicTaskID";
                        bindingObj["value"] = binding.VariableName;
                        break;

                    case ParameterBindingType::ConditionID:
                        bindingObj["Type"] = "ConditionID";
                        bindingObj["value"] = binding.VariableName;
                        break;

                    case ParameterBindingType::MathOperator:
                        bindingObj["Type"] = "MathOperator";
                        bindingObj["value"] = binding.VariableName;
                        break;

                    case ParameterBindingType::ComparisonOp:
                        bindingObj["Type"] = "ComparisonOp";
                        bindingObj["value"] = binding.VariableName;
                        break;

                    case ParameterBindingType::SubGraphPath:
                        bindingObj["Type"] = "SubGraphPath";
                        bindingObj["value"] = binding.VariableName;
                        break;

                    default:
                        bindingObj["Type"] = "Literal";
                        break;
                }

                paramsObj[paramName] = bindingObj;
            }
            n["params"] = paramsObj;
        }

        // Switch enhancements (Phase 22-A)
        if (def.Type == TaskNodeType::Switch)
        {
            if (!def.switchVariable.empty())
                n["switchVariable"] = def.switchVariable;

            if (!def.switchCases.empty())
            {
                json casesArray = json::array();
                for (size_t c = 0; c < def.switchCases.size(); ++c)
                {
                    const SwitchCaseDefinition& sc = def.switchCases[c];
                    json caseObj;
                    caseObj["value"] = sc.value;
                    caseObj["pin"]   = sc.pinName;
                    if (!sc.customLabel.empty())
                        caseObj["label"] = sc.customLabel;
                    casesArray.push_back(caseObj);
                }
                n["switchCases"] = casesArray;
            }
        }

        // Phase 24 Milestone 2 — MathOp operand serialization
        // Serialize the complete MathOpRef (left operand, operator, right operand)
        if (def.Type == TaskNodeType::MathOp && !def.mathOpRef.mathOperator.empty())
        {
            n["mathOpRef"] = def.mathOpRef.ToJson();
            SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: serialized mathOpRef for MathOp node "
                       << def.NodeID << "\n";
        }

        // Structured conditions (Phase 23-B.4 — Branch/While)
        if ((def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While) &&
            !def.conditions.empty())
        {
            json condArray = json::array();
            for (size_t ci = 0; ci < def.conditions.size(); ++ci)
            {
                const Condition& cond = def.conditions[ci];
                json cj;

                // Left side
                cj["leftMode"]     = cond.leftMode;
                if (!cond.leftPin.empty())
                    cj["leftPin"]  = cond.leftPin;
                if (!cond.leftVariable.empty())
                    cj["leftVariable"] = cond.leftVariable;
                if (cond.leftMode == "Const" && !cond.leftConstValue.IsNone())
                {
                    const TaskValue& lv = cond.leftConstValue;
                    switch (lv.GetType()) {
                        case VariableType::Bool:   cj["leftConstValue"] = lv.AsBool();  break;
                        case VariableType::Int:    cj["leftConstValue"] = lv.AsInt();   break;
                        case VariableType::Float:  cj["leftConstValue"] = lv.AsFloat(); break;
                        case VariableType::String: cj["leftConstValue"] = lv.AsString();break;
                        default: break;
                    }
                }

                // Operator
                cj["operator"] = cond.operatorStr;

                // Right side
                cj["rightMode"]    = cond.rightMode;
                if (!cond.rightPin.empty())
                    cj["rightPin"] = cond.rightPin;
                if (!cond.rightVariable.empty())
                    cj["rightVariable"] = cond.rightVariable;
                if (cond.rightMode == "Const" && !cond.rightConstValue.IsNone())
                {
                    const TaskValue& rv = cond.rightConstValue;
                    switch (rv.GetType()) {
                        case VariableType::Bool:   cj["rightConstValue"] = rv.AsBool();  break;
                        case VariableType::Int:    cj["rightConstValue"] = rv.AsInt();   break;
                        case VariableType::Float:  cj["rightConstValue"] = rv.AsFloat(); break;
                        case VariableType::String: cj["rightConstValue"] = rv.AsString();break;
                        default: break;
                    }
                }

                // Type hint
                if (cond.compareType != VariableType::None)
                    cj["compareType"] = VariableTypeToString(cond.compareType);

                condArray.push_back(cj);
            }
            n["conditions"] = condArray;
        }

        // Phase 24 Milestone 2.2 — conditionRefs serialization (new inline system)
        // Saves OperandRef data including dynamicPinID for Pin-mode operands.
        // Coexists with legacy def.conditions[] during transition.
        if ((def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While) &&
            !def.conditionOperandRefs.empty())
        {
            json condRefsArray = json::array();

            for (size_t i = 0; i < def.conditionOperandRefs.size(); ++i)
            {
                const ConditionRef& ref = def.conditionOperandRefs[i];
                json refObj;
                refObj["conditionIndex"] = static_cast<int>(i);

                // Left operand
                {
                    json lj;
                    switch (ref.leftOperand.mode)
                    {
                        case OperandRef::Mode::Variable:
                            lj["mode"]         = "Variable";
                            lj["variableName"] = ref.leftOperand.variableName;
                            break;
                        case OperandRef::Mode::Const:
                            lj["mode"]       = "Const";
                            lj["constValue"] = ref.leftOperand.constValue;
                            break;
                        case OperandRef::Mode::Pin:
                            lj["mode"]         = "Pin";
                            lj["dynamicPinID"] = ref.leftOperand.dynamicPinID;
                            break;
                        default:
                            lj["mode"] = "Const";
                            break;
                    }
                    refObj["leftOperand"] = lj;
                }

                refObj["operator"] = ref.operatorStr;

                // Right operand
                {
                    json rj;
                    switch (ref.rightOperand.mode)
                    {
                        case OperandRef::Mode::Variable:
                            rj["mode"]         = "Variable";
                            rj["variableName"] = ref.rightOperand.variableName;
                            break;
                        case OperandRef::Mode::Const:
                            rj["mode"]       = "Const";
                            rj["constValue"] = ref.rightOperand.constValue;
                            break;
                        case OperandRef::Mode::Pin:
                            rj["mode"]         = "Pin";
                            rj["dynamicPinID"] = ref.rightOperand.dynamicPinID;
                            break;
                        default:
                            rj["mode"] = "Const";
                            break;
                    }
                    refObj["rightOperand"] = rj;
                }

                if (ref.compareType != VariableType::None)
                    refObj["compareType"] = VariableTypeToString(ref.compareType);

                condRefsArray.push_back(refObj);
            }

            n["conditionRefs"] = condRefsArray;

            SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: Phase 24: serialized "
                       << def.conditionOperandRefs.size() << " conditionRefs for node "
                       << def.NodeID << "\n";
        }

        // Phase 24 Milestone 2.3 — Node condition references (preset IDs + logical operators)
        // Save which presets are used and their logical operator chain
        if ((def.Type == TaskNodeType::Branch || def.Type == TaskNodeType::While) &&
            !def.conditionRefs.empty())
        {
            json nodeCondRefsArray = json::array();
            for (const auto& ncref : def.conditionRefs)
            {
                json nobj = ncref.ToJson();
                nodeCondRefsArray.push_back(nobj);
            }
            n["nodeConditionRefs"] = nodeCondRefsArray;

            SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: Phase 24: serialized "
                       << def.conditionRefs.size() << " nodeConditionRefs for node "
                       << def.NodeID << "\n";
        }

        // Dynamic exec-out pins (VSSequence and Switch)
        if ((def.Type == TaskNodeType::VSSequence || def.Type == TaskNodeType::Switch) &&
            !def.DynamicExecOutputPins.empty())
        {
            json dynPins = json::array();
            for (size_t p = 0; p < def.DynamicExecOutputPins.size(); ++p)
                dynPins.push_back(def.DynamicExecOutputPins[p]);
            n["dynamicExecPins"] = dynPins;
        }

        // SubGraph input and output parameters (Phase 3)
        if (def.Type == TaskNodeType::SubGraph)
        {
            // Input parameters: map of name → ParameterBinding
            if (!def.InputParams.empty())
            {
                json inputParamsObj = json::object();
                for (const auto& paramPair : def.InputParams)
                {
                    const std::string& paramName = paramPair.first;
                    const ParameterBinding& binding = paramPair.second;

                    json bindingObj = json::object();

                    switch (binding.Type)
                    {
                        case ParameterBindingType::Literal:
                            bindingObj["Type"] = "Literal";
                            if (!binding.LiteralValue.IsNone())
                            {
                                switch (binding.LiteralValue.GetType())
                                {
                                    case VariableType::Bool:
                                        bindingObj["LiteralValue"] = binding.LiteralValue.AsBool();
                                        break;
                                    case VariableType::Int:
                                        bindingObj["LiteralValue"] = binding.LiteralValue.AsInt();
                                        break;
                                    case VariableType::Float:
                                        bindingObj["LiteralValue"] = binding.LiteralValue.AsFloat();
                                        break;
                                    case VariableType::String:
                                        bindingObj["LiteralValue"] = binding.LiteralValue.AsString();
                                        break;
                                    case VariableType::Vector:
                                    {
                                        const ::Vector v = binding.LiteralValue.AsVector();
                                        json vec;
                                        vec["x"] = v.x;
                                        vec["y"] = v.y;
                                        vec["z"] = v.z;
                                        bindingObj["LiteralValue"] = vec;
                                        break;
                                    }
                                    case VariableType::EntityID:
                                        bindingObj["LiteralValue"] = std::to_string(binding.LiteralValue.AsEntityID());
                                        break;
                                    default:
                                        break;
                                }
                            }
                            break;

                        case ParameterBindingType::LocalVariable:
                            bindingObj["Type"] = "LocalVariable";
                            bindingObj["VariableName"] = binding.VariableName;
                            break;

                        case ParameterBindingType::AtomicTaskID:
                            bindingObj["Type"] = "AtomicTaskID";
                            bindingObj["value"] = binding.VariableName;
                            break;

                        case ParameterBindingType::ConditionID:
                            bindingObj["Type"] = "ConditionID";
                            bindingObj["value"] = binding.VariableName;
                            break;

                        case ParameterBindingType::MathOperator:
                            bindingObj["Type"] = "MathOperator";
                            bindingObj["value"] = binding.VariableName;
                            break;

                        case ParameterBindingType::ComparisonOp:
                            bindingObj["Type"] = "ComparisonOp";
                            bindingObj["value"] = binding.VariableName;
                            break;

                        case ParameterBindingType::SubGraphPath:
                            bindingObj["Type"] = "SubGraphPath";
                            bindingObj["value"] = binding.VariableName;
                            break;

                        default:
                            bindingObj["Type"] = "Literal";
                            break;
                    }

                    inputParamsObj[paramName] = bindingObj;
                }
                n["InputParams"] = inputParamsObj;
            }

            // Output parameters: map of name → blackboard key
            if (!def.OutputParams.empty())
            {
                json outputParamsObj = json::object();
                for (const auto& paramPair : def.OutputParams)
                {
                    outputParamsObj[paramPair.first] = paramPair.second;
                }
                n["OutputParams"] = outputParamsObj;
            }
        }

        // Position from editor node
        for (size_t j = 0; j < m_editorNodes.size(); ++j)
        {
            if (m_editorNodes[j].nodeID == def.NodeID)
            {
                json pos;
                pos["x"] = m_editorNodes[j].posX;
                pos["y"] = m_editorNodes[j].posY;
                n["position"] = pos;
                break;
            }
        }

        nodesArray.push_back(n);
    }
    root["nodes"] = nodesArray;

    // Exec connections
    json execArray = json::array();
    for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = m_template.ExecConnections[i];
        json c;
        c["fromNode"] = conn.SourceNodeID;
        c["fromPin"]  = conn.SourcePinName;
        c["toNode"]   = conn.TargetNodeID;
        c["toPin"]    = conn.TargetPinName;
        execArray.push_back(c);
    }
    root["execConnections"] = execArray;

    // Data connections
    json dataArray = json::array();
    for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = m_template.DataConnections[i];
        json c;
        c["fromNode"] = conn.SourceNodeID;
        c["fromPin"]  = conn.SourcePinName;
        c["toNode"]   = conn.TargetNodeID;
        c["toPin"]    = conn.TargetPinName;
        dataArray.push_back(c);
    }
    root["dataConnections"] = dataArray;

    // Phase 24 Global Blackboard Integration: Serialize global variable values
    // These are entity-specific values stored in the template before serialization
    if (!m_template.GlobalVariableValues.is_null() && !m_template.GlobalVariableValues.empty())
    {
        root["globalVariableValues"] = m_template.GlobalVariableValues;
        SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: Phase 24 - serialized "
                   << "global variable values\n";
    }

    // Phase 24 — Condition Preset Bank (embedded in graph JSON)
    // Presets are now serialized as part of the graph, making blueprints self-contained.
    if (!m_template.Presets.empty())
    {
        json presetsArray = json::array();
        for (size_t i = 0; i < m_template.Presets.size(); ++i)
        {
            const ConditionPreset& preset = m_template.Presets[i];
            json presetObj = preset.ToJson();  // Delegate serialization to preset's own method
            presetsArray.push_back(presetObj);
        }
        root["presets"] = presetsArray;

        SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: Phase 24 - serialized "
                   << m_template.Presets.size() << " embedded presets\n";
    }

    // Write file
    SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: opening '"
               << path << "' for writing\n";
    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        std::cerr << "[VisualScriptEditorPanel] Cannot open file for write: " << path << std::endl;
        SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite FAILED: cannot open '"
                   << path << "'\n";
        return false;
    }
    ofs << root.dump(2);
    ofs.close();
    m_dirty = false;
    SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite succeeded: '" << path << "'\n";
    return true;
}

// ============================================================================
// Blackboard validation helpers (BUG-002 Fix #1)
// ============================================================================

void VisualScriptEditorPanel::ValidateAndCleanBlackboardEntries()
{
    std::vector<BlackboardEntry>& entries = m_template.Blackboard;
    size_t before = entries.size();

    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const BlackboardEntry& e) {
                if (e.Key.empty()) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry with empty key\n";
                    return true;
                }
                if (e.Type == VariableType::None) {
                    SYSTEM_LOG << "[VSEditor] ValidateAndClean: removing entry '"
                               << e.Key << "' with VariableType::None\n";
                    return true;
                }
                return false;
            }),
        entries.end());

    size_t removed = before - entries.size();
    if (removed > 0)
    {
        SYSTEM_LOG << "[VSEditor] ValidateAndClean: removed " << removed
                   << " invalid blackboard entries\n";
        m_dirty = true;
    }
}

void VisualScriptEditorPanel::CommitPendingBlackboardEdits()
{
    for (std::unordered_map<int, std::string>::iterator it = m_pendingBlackboardEdits.begin();
         it != m_pendingBlackboardEdits.end(); ++it)
    {
        int idx = it->first;
        if (idx >= 0 && idx < static_cast<int>(m_template.Blackboard.size()))
        {
            m_template.Blackboard[static_cast<size_t>(idx)].Key = it->second;
        }
    }
    m_pendingBlackboardEdits.clear();
}

// ============================================================================
// BUG-003 Viewport helpers
// ============================================================================

void VisualScriptEditorPanel::ResetViewportBeforeSave()
{
    SYSTEM_LOG << "[VSEditor] ResetViewportBeforeSave: saving current panning\n";
    m_lastViewportPanning = Vector::FromImVec2(ImNodes::EditorContextGetPanning());
    m_viewportResetDone   = true;

    // Reset panning to (0, 0) so that any residual editor-space offset from
    // user navigation is zeroed out before SyncNodePositionsFromImNodes reads
    // GetNodeGridSpacePos (which is already pan-independent, but this ensures
    // no subtle ImNodes internal state leaks into the saved positions).
    ImNodes::EditorContextResetPanning(ImVec2(0.0f, 0.0f));
    SYSTEM_LOG << "[VSEditor] ResetViewportBeforeSave: panning reset to (0,0) "
               << "(was " << m_lastViewportPanning.x << "," << m_lastViewportPanning.y << ")\n";
}

void VisualScriptEditorPanel::AfterSave()
{
    if (!m_viewportResetDone)
        return;

    // Restore the viewport so the canvas does not visually jump for the user.
    ImNodes::EditorContextResetPanning(m_lastViewportPanning.ToImVec2());
    m_viewportResetDone = false;
    SYSTEM_LOG << "[VSEditor] AfterSave: viewport panning restored to ("
               << m_lastViewportPanning.x << "," << m_lastViewportPanning.y << ")\n";
}

ImVec2 VisualScriptEditorPanel::ScreenToCanvasPos(ImVec2 screenPos) const
{
    // Convert absolute screen-space position to ImNodes editor (canvas) space.
    // Editor space = grid space + panning, so:
    //   editorX = screenX - canvasOrigin.x - windowPos.x
    // ImNodes 0.4 has no zoom API; zoom is implicitly 1.0f.
    ImVec2 canvasPanning = ImNodes::EditorContextGetPanning();
    ImVec2 windowPos     = ImGui::GetWindowPos();
    return ImVec2(
        screenPos.x - windowPos.x - canvasPanning.x,
        screenPos.y - windowPos.y - canvasPanning.y);
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

// ============================================================================
// Rendering
// ============================================================================

// ============================================================================
// Undo/Redo wrappers
// ============================================================================

void VisualScriptEditorPanel::PerformUndo()
{
    if (!m_undoStack.CanUndo())
        return;

    std::string desc = m_undoStack.PeekUndoDescription();
    SYSTEM_LOG << "[VSEditor] UNDO: " << desc << "\n";
    m_undoStack.Undo(m_template);
    SyncEditorNodesFromTemplate();
    RebuildLinks();

    // Force-push the restored positions into ImNodes so that the next
    // BeginNode()/EndNode() cycle renders them at the correct location.
    // BUG-003 Fix: positions stored in m_editorNodes are grid-space
    // (written by SyncNodePositionsFromImNodes via GetNodeGridSpacePos),
    // so use SetNodeGridSpacePos to restore them pan-independently.
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        ImNodes::SetNodeGridSpacePos(
            m_editorNodes[i].nodeID,
            ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
    }

    // Block position sync and movement tracking for 1 frame so that stale
    // ImNodes state cannot overwrite the correct undo-target positions before
    // ImNodes has rendered the new layout at least once.
    m_justPerformedUndoRedo      = true;
    m_skipPositionSyncNextFrame  = true;
    m_nodeDragStartPositions.clear();
    m_dirty = true;
    m_verificationDone = false;
    SYSTEM_LOG << "[VSEditor] Undo complete. Template now has "
               << m_template.Nodes.size() << " nodes, "
               << m_template.ExecConnections.size() << " exec connections\n";
}

void VisualScriptEditorPanel::PerformRedo()
{
    if (!m_undoStack.CanRedo())
        return;

    std::string desc = m_undoStack.PeekRedoDescription();
    SYSTEM_LOG << "[VSEditor] REDO: " << desc << "\n";
    m_undoStack.Redo(m_template);
    SyncEditorNodesFromTemplate();
    RebuildLinks();

    // Same treatment as PerformUndo().
    // BUG-003 Fix: use SetNodeGridSpacePos (grid-space) for pan-independent restore.
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        ImNodes::SetNodeGridSpacePos(
            m_editorNodes[i].nodeID,
            ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
    }

    m_justPerformedUndoRedo      = true;
    m_skipPositionSyncNextFrame  = true;
    m_nodeDragStartPositions.clear();
    m_dirty = true;
    m_verificationDone = false;
    SYSTEM_LOG << "[VSEditor] Redo complete. Template now has "
               << m_template.Nodes.size() << " nodes, "
               << m_template.ExecConnections.size() << " exec connections\n";
}

void VisualScriptEditorPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::Begin("VS Graph Editor", &m_visible);
    RenderContent();
    ImGui::End();

    // Render the condition preset library panel (Phase 24 UI integration)
    m_libraryPanel->Render();
}

void VisualScriptEditorPanel::RenderContent()
{
    RenderToolbar();
    RenderSaveAsDialog();
    ImGui::Separator();

    // Two-column layout: canvas (left) | resize handle | properties panel (right, 3 sub-panels)
    float totalWidth = ImGui::GetContentRegionAvail().x;

    // Initialize panel width to default 28% on first use
    if (m_propertiesPanelWidth <= 0.0f)
        m_propertiesPanelWidth = totalWidth * 0.28f;

    // Clamp to a sensible range
    if (m_propertiesPanelWidth < 200.0f) m_propertiesPanelWidth = 200.0f;
    if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;

    float handleWidth = 6.0f;
    float canvasWidth = totalWidth - m_propertiesPanelWidth - handleWidth;

    ImGui::BeginChild("VSCanvas", ImVec2(canvasWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    // UX Fix #3: Drag-to-resize handle between canvas and properties panel
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##vsresize", ImVec2(handleWidth, -1.0f));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_propertiesPanelWidth -= ImGui::GetIO().MouseDelta.x;
        if (m_propertiesPanelWidth < 200.0f)          m_propertiesPanelWidth = 200.0f;
        if (m_propertiesPanelWidth > totalWidth * 0.60f) m_propertiesPanelWidth = totalWidth * 0.60f;
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    // Right panel container with 3 vertical sub-panels (A: Node Props | B: Preset Bank | C: Local Vars)
    ImGui::BeginChild("VSRightPanel", ImVec2(m_propertiesPanelWidth, 0), true);

    float rightPanelHeight = ImGui::GetContentRegionAvail().y;
    float splitterHeight = 4.0f;

    // Initialize sub-panel heights on first use (equal thirds for 3 panels)
    if (m_nodePropertiesPanelHeight <= 0.0f)
    {
        m_nodePropertiesPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
        m_presetBankPanelHeight = (rightPanelHeight - splitterHeight * 2) / 3.0f;
    }

    // Clamp heights to reasonable ranges
    float minPanelHeight = 50.0f;
    if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;

    float localVarHeight = rightPanelHeight - m_nodePropertiesPanelHeight - m_presetBankPanelHeight - splitterHeight * 2;
    if (localVarHeight < minPanelHeight) localVarHeight = minPanelHeight;

    // ---- Part A: Node Properties Panel ----
    ImGui::BeginChild("Part_A_NodeProps", ImVec2(0, m_nodePropertiesPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderNodePropertiesPanel();
    ImGui::EndChild();

    // ---- Splitter 1 (between Part A and Part B) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter1", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_nodePropertiesPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_nodePropertiesPanelHeight < minPanelHeight) m_nodePropertiesPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part B: Preset Bank Panel ----
    ImGui::BeginChild("Part_B_PresetBank", ImVec2(0, m_presetBankPanelHeight), false,
                      ImGuiWindowFlags_NoScrollbar);
    RenderPresetBankPanel();
    ImGui::EndChild();

    // ---- Splitter 2 (between Part B and Part C) ----
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.35f, 0.35f, 0.35f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.55f, 0.55f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f, 0.70f, 0.70f, 1.0f));
    ImGui::Button("##splitter2", ImVec2(-1.0f, splitterHeight));
    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_presetBankPanelHeight += ImGui::GetIO().MouseDelta.y;
        if (m_presetBankPanelHeight < minPanelHeight) m_presetBankPanelHeight = minPanelHeight;
    }
    ImGui::PopStyleColor(3);

    // ---- Part C: Local/Global Variables Panel (with tab selection) ----
    ImGui::BeginChild("Part_C_Blackboard", ImVec2(0, localVarHeight), false,
                      ImGuiWindowFlags_NoScrollbar);

    // Tab selector for Local vs Global variables
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
    ImGui::RadioButton("Local Variables", &m_blackboardTabSelection, 0);
    ImGui::SameLine(150.0f);
    ImGui::RadioButton("Global Variables", &m_blackboardTabSelection, 1);
    ImGui::PopStyleVar();
    ImGui::Separator();

    // Render appropriate panel based on tab selection
    if (m_blackboardTabSelection == 0)
        RenderLocalVariablesPanel();
    else
        RenderGlobalVariablesPanel();

    ImGui::EndChild();

    ImGui::EndChild();  // End VSRightPanel
}

void VisualScriptEditorPanel::RenderToolbar()
{
    // Title
    const char* title = m_currentPath.empty()
                        ? "Untitled VS Graph"
                        : m_currentPath.c_str();
    ImGui::TextDisabled("%s%s", title, m_dirty ? " *" : "");

    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        SYSTEM_LOG << "[VisualScriptEditorPanel] Save button clicked. m_currentPath='"
                   << m_currentPath << "'\n";
        if (m_currentPath.empty())
        {
            m_showSaveAsDialog = true;
        }
        else if (!Save())
        {
            ImGui::OpenPopup("SaveError");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As"))
    {
        m_showSaveAsDialog = true;
    }
    ImGui::SameLine();
    // Phase 24.3 — Removed "New Graph" button as requested
    // Users must create new graphs through the file browser instead

    if (ImGui::Button("Verify##gvs"))
    {
        RunVerification();
    }
    ImGui::SameLine();
    if (ImGui::Button("Condition Presets"))
    {
        m_libraryPanel->Open();
    }
    ImGui::SameLine();
    if (m_verificationDone)
    {
        if (m_verificationResult.HasErrors())
        {
            int errorCount = 0;
            for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
            {
                if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                    ++errorCount;
            }
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                               "[%d error(s)]", errorCount);
        }
        else if (m_verificationResult.HasWarnings())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[OK - warnings]");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[OK]");
        }
    }

    // Keyboard shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_S) &&
            ImGui::GetIO().KeyCtrl)
        {
            SYSTEM_LOG << "[VisualScriptEditorPanel] Ctrl+S pressed. m_currentPath='"
                       << m_currentPath << "'\n";
            if (m_currentPath.empty())
            {
                m_showSaveAsDialog = true;
            }
            else if (!Save())
            {
                ImGui::OpenPopup("SaveError");
            }
        }

        // Undo (Ctrl+Z)
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) &&
            m_undoStack.CanUndo())
        {
            PerformUndo();
        }

        // Redo (Ctrl+Y)
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y) &&
            m_undoStack.CanRedo())
        {
            PerformRedo();
        }
    }

    if (ImGui::BeginPopup("SaveError"))
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "Save failed — check file path.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void VisualScriptEditorPanel::RenderSaveAsDialog()
{
    if (m_showSaveAsDialog)
    {
        ImGui::OpenPopup("SaveAsDialog");
        m_showSaveAsDialog = false;

        // Derive the save extension from the currently loaded file so we
        // preserve .json files as .json (instead of silently renaming to .ats).
        // Fall back to .ats for new/untitled graphs.
        m_saveAsExtension = ".ats";
        if (!m_currentPath.empty())
        {
            size_t dotPos = m_currentPath.rfind('.');
            if (dotPos != std::string::npos)
                m_saveAsExtension = m_currentPath.substr(dotPos);
        }

        // Pre-fill the filename from the current path so the user doesn't have
        // to retype a name they already gave the graph.
        if (!m_currentPath.empty())
        {
            size_t lastSlash = m_currentPath.find_last_of("/\\");
            std::string fname = (lastSlash != std::string::npos)
                                ? m_currentPath.substr(lastSlash + 1)
                                : m_currentPath;
            // Strip extension
            size_t dotPos = fname.rfind('.');
            if (dotPos != std::string::npos)
                fname = fname.substr(0, dotPos);

            strncpy_s(m_saveAsFilename, sizeof(m_saveAsFilename), fname.c_str(), _TRUNCATE);
        }
        // else: keep whatever is already in the buffer (set in constructor or
        //       carried over from a previous dialog invocation).
    }

    if (ImGui::BeginPopupModal("SaveAsDialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Save Visual Script As");
        ImGui::Separator();

        // Directory dropdown
        ImGui::Text("Directory:");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##SaveDir", m_saveAsDirectory.c_str()))
        {
            static const char* dirs[] = {
                "Blueprints/AI",
                "Blueprints/AI/Tests",
                "Gamedata/TaskGraph/Examples",
                "Gamedata/TaskGraph/Templates"
            };
            for (int i = 0; i < static_cast<int>(sizeof(dirs) / sizeof(dirs[0])); ++i)
            {
                bool selected = (m_saveAsDirectory == dirs[i]);
                if (ImGui::Selectable(dirs[i], selected))
                    m_saveAsDirectory = dirs[i];
                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Filename input
        ImGui::Text("Filename:");
        ImGui::SameLine();
        ImGui::InputText("##FileName", m_saveAsFilename, sizeof(m_saveAsFilename));

        // Full path preview
        ImGui::TextDisabled("Full path: %s/%s%s",
                            m_saveAsDirectory.c_str(),
                            m_saveAsFilename,
                            m_saveAsExtension.c_str());

        ImGui::Separator();

        // Save / Cancel buttons
        bool filenameEmpty = (std::strlen(m_saveAsFilename) == 0);
        if (filenameEmpty)
            ImGui::BeginDisabled();
        if (ImGui::Button("Save", ImVec2(120, 0)))
        {
            std::string fullPath = m_saveAsDirectory + "/" +
                                   std::string(m_saveAsFilename) + m_saveAsExtension;
            SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs dialog confirmed. fullPath='"
                       << fullPath << "'\n";
            if (SaveAs(fullPath))
            {
                std::cout << "[VisualScriptEditorPanel] Saved to: " << fullPath << std::endl;
                ImGui::CloseCurrentPopup();
            }
            else
            {
                ImGui::OpenPopup("SaveAsError");
            }
        }
        if (filenameEmpty)
            ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        // Nested error popup
        if (ImGui::BeginPopupModal("SaveAsError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Save failed — check directory and permissions.");
            if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::EndPopup();
    }
}

void VisualScriptEditorPanel::RenderCanvas()
{
    // Switch to this panel's dedicated ImNodes context so that node positions
    // and canvas panning are preserved independently for each open tab.
    if (m_imnodesContext)
        ImNodes::EditorContextSet(m_imnodesContext);

    // On the first render after LoadTemplate(), push the stored (posX, posY)
    // of each node into ImNodes so the canvas matches the saved layout.
    // BUG-003 Fix: positions are stored in grid space; use SetNodeGridSpacePos
    // to restore them pan-independently (avoids double-offset with viewport pan).
    if (m_needsPositionSync)
    {
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            ImNodes::SetNodeGridSpacePos(
                m_editorNodes[i].nodeID,
                ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
        }
        m_needsPositionSync = false;
    }

    // Phase 21-B: focus/scroll to a node requested from the verification panel
    if (m_focusNodeID >= 0)
    {
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_focusNodeID)
            {
                // BUG-003 Fix: restore in grid space for pan-independent positioning.
                ImNodes::SetNodeGridSpacePos(
                    m_focusNodeID,
                    ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
                break;
            }
        }
        m_focusNodeID = -1;
    }

    ImNodes::BeginNodeEditor();

    // NOTE: Right-click context menu detection is deferred until after
    // ImNodes::EndNodeEditor() below so that IsNodeHovered() / IsLinkHovered()
    // (which require ImNodesScope_None) can be used to determine what was clicked.

    RenderNodePalette();

    // Render all nodes
    int activeNodeID = DebugController::Get().GetCurrentNodeID();

    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        VSEditorNode& eNode = m_editorNodes[i];

        bool hasBreakpoint = DebugController::Get().HasBreakpoint(
            0 /* graphID placeholder */, eNode.nodeID);
        bool isActive = (eNode.nodeID == activeNodeID &&
                         DebugController::Get().IsDebugging());

        // Phase 21-B: highlight nodes that have Error issues in the verification result
        bool hasVerifError = false;
        if (m_verificationDone)
        {
            for (size_t vi = 0; vi < m_verificationResult.issues.size(); ++vi)
            {
                if (m_verificationResult.issues[vi].nodeID == eNode.nodeID &&
                    m_verificationResult.issues[vi].severity == VSVerificationSeverity::Error)
                {
                    hasVerifError = true;
                    break;
                }
            }
        }
        if (hasVerifError)
        {
            ImNodes::PushColorStyle(ImNodesCol_NodeBackground,
                                    IM_COL32(120, 30, 30, 230));
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundHovered,
                                    IM_COL32(120, 30, 30, 230));
            ImNodes::PushColorStyle(ImNodesCol_NodeBackgroundSelected,
                                    IM_COL32(120, 30, 30, 230));
        }

        auto execIn  = GetExecInputPins(eNode.def.Type);
        auto execOut = GetExecOutputPinsForNode(eNode.def);

        // Phase 24 FIX: Ensure MathOp nodes have DataPins initialized
        // This handles both newly created nodes AND nodes loaded from blueprints
        if (eNode.def.Type == TaskNodeType::MathOp && eNode.def.DataPins.empty())
        {
            // Initialize DataPins for this MathOp node if not already present
            DataPinDefinition pinA;
            pinA.PinName = "A";
            pinA.Dir     = DataPinDir::Input;
            pinA.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinA);

            DataPinDefinition pinB;
            pinB.PinName = "B";
            pinB.Dir     = DataPinDir::Input;
            pinB.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinB);

            DataPinDefinition pinResult;
            pinResult.PinName = "Result";
            pinResult.Dir     = DataPinDir::Output;
            pinResult.PinType = VariableType::Float;
            eNode.def.DataPins.push_back(pinResult);

            std::cerr << "[VSEditor] Initialized DataPins for MathOp node #" << eNode.nodeID << "\n";
        }

        std::vector<std::pair<std::string, VariableType>> dataIn, dataOut;
        for (size_t p = 0; p < eNode.def.DataPins.size(); ++p)
        {
            const DataPinDefinition& pin = eNode.def.DataPins[p];
            if (pin.Dir == DataPinDir::Input)
                dataIn.push_back({pin.PinName, pin.PinType});
            else
                dataOut.push_back({pin.PinName, pin.PinType});
        }

        // Phase 24 — Dispatcher: Branch nodes use specialized renderer
        if (eNode.def.Type == TaskNodeType::Branch && m_branchRenderer)
        {
            // Convert TaskNodeDefinition to NodeBranchData for specialized rendering
            NodeBranchData branchData;
            branchData.nodeID        = eNode.nodeID;  // int nodeID for ImNodes attribute UIDs
            branchData.nodeName      = eNode.def.NodeName;
            branchData.conditionRefs = eNode.def.conditionRefs;
            branchData.dynamicPins   = eNode.def.dynamicPins;
            branchData.breakpoint    = hasBreakpoint;

            // Render via NodeBranchRenderer (4-section layout)
            // Must be wrapped with ImNodes::BeginNode/EndNode just like generic renderer
            ImNodes::BeginNode(eNode.nodeID);
            m_branchRenderer->RenderNode(branchData);
            ImNodes::EndNode();
        }
        else
        {
            // Use generic renderer for all other node types
            VisualScriptNodeRenderer::RenderNode(
                eNode.nodeID,
                eNode.nodeID,
                0 /* graphID placeholder */,
                eNode.def,
                hasBreakpoint,
                isActive,
                execIn, execOut,
                dataIn, dataOut,
                [](int nid, void* ud) {
                    VisualScriptEditorPanel* panel =
                        static_cast<VisualScriptEditorPanel*>(ud);
                    panel->m_pendingAddPin       = true;
                    panel->m_pendingAddPinNodeID = nid;
                },
                this,
                [](int nid, int dynIdx, void* ud) {
                    VisualScriptEditorPanel* panel =
                        static_cast<VisualScriptEditorPanel*>(ud);
                    panel->m_pendingRemovePin       = true;
                    panel->m_pendingRemovePinNodeID = nid;
                    panel->m_pendingRemovePinDynIdx = dynIdx;
                },
                this);
        }

        if (hasVerifError)
        {
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
            ImNodes::PopColorStyle();
        }

        // Breakpoint / active overlays
        if (hasBreakpoint)
            VisualScriptNodeRenderer::RenderBreakpointIndicator(eNode.nodeID);
        if (isActive)
            VisualScriptNodeRenderer::RenderActiveNodeGlow(eNode.nodeID);

        // Mark this node as rendered so position sync is safe
        m_positionedNodes.insert(eNode.nodeID);
    }

    // Render links
    for (size_t i = 0; i < m_editorLinks.size(); ++i)
    {
        const VSEditorLink& link = m_editorLinks[i];
        if (link.isData)
            ImNodes::PushColorStyle(ImNodesCol_Link, SystemColors::DATA_CONNECTION_COLOR);
        else
            ImNodes::PushColorStyle(ImNodesCol_Link, SystemColors::EXEC_CONNECTION_COLOR);
        ImNodes::Link(link.linkID, link.srcAttrID, link.dstAttrID);
        ImNodes::PopColorStyle();
    }

    ImNodes::EndNodeEditor();

    // FIX 4: Skip position sync if undo/redo just executed.  
    // SyncEditorNodesFromTemplate() has already written the correct undo-target 
    // positions into m_editorNodes and SetNodeEditorSpacePos() has pushed them 
    // to ImNodes. Reading them back here (before ImNodes has rendered the new 
    // positions once) would overwrite the correct values with stale ImNodes state.
    if (m_skipPositionSyncNextFrame)
    {
        m_skipPositionSyncNextFrame = false;
    }
    else
    {
        SyncNodePositionsFromImNodes();
    }

    // ========================================================================
    // Context menu dispatch (requires ImNodesScope_None, i.e. after EndNodeEditor)
    // Priority: node hover > link hover > canvas background.
    // ========================================================================
    {
        int  hoveredNode = -1;
        int  hoveredLink = -1;
        bool nodeHovered = ImNodes::IsNodeHovered(&hoveredNode);
        bool linkHovered = ImNodes::IsLinkHovered(&hoveredLink);

        // PHASE 1: Detect right-click and open the appropriate popup.
        // Use ImNodes::IsEditorHovered() for canvas background detection so
        // that the check works even when ImNodes has captured mouse focus.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (nodeHovered)
            {
                m_contextNodeID = hoveredNode;
                ImGui::OpenPopup("VSNodeContextMenu");
                SYSTEM_LOG << "[VSEditor] Opened context menu on NODE #" << hoveredNode << "\n";
            }
            else if (linkHovered)
            {
                m_contextLinkID = hoveredLink;
                ImGui::OpenPopup("VSLinkContextMenu");
                SYSTEM_LOG << "[VSEditor] Opened context menu on LINK #" << hoveredLink << "\n";
            }
            else if (ImNodes::IsEditorHovered())
            {
                // Convert screen-space mouse position to canvas-space by
                // subtracting the ImNodes canvas panning offset.
                // Note: ImNodes 0.4 does not expose a zoom accessor, so zoom=1.0f.
                ImVec2 mp          = ImGui::GetMousePos();
                ImVec2 canvasOrigin = ImNodes::EditorContextGetPanning();
                float  zoom        = 1.0f;
                m_contextMenuX = (mp.x - canvasOrigin.x) / zoom;
                m_contextMenuY = (mp.y - canvasOrigin.y) / zoom;
                ImGui::OpenPopup("VSNodePalette");
                SYSTEM_LOG << "[VSEditor] Opened context menu on CANVAS at ("
                           << m_contextMenuX << ", " << m_contextMenuY << ")\n";
            }
        }

        // PHASE 2: Render popups in the same ImGui window scope.
        RenderContextMenus();
    }

    // ========================================================================
    // PHASE 1: Detect drag & drop (store pending node creation).
    // AddNode() must NOT be called here — ImNodes' internal state is still
    // being finalised at this point and SetNodeEditorSpacePos would assert.
    // ========================================================================
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VS_NODE_TYPE_ENUM");
        if (payload && payload->Data && payload->DataSize == sizeof(uint8_t))
        {
            uint8_t enumValue = *static_cast<const uint8_t*>(payload->Data);
            TaskNodeType nodeType = static_cast<TaskNodeType>(enumValue);

            // Get mouse position in canvas space
            ImVec2 mousePos  = ImGui::GetMousePos();
            ImVec2 canvasPos = ImNodes::EditorContextGetPanning();
            float  zoom      = 1.0f;  // ImNodes doesn't expose zoom yet

            // Convert screen space to canvas space
            ImVec2 windowPos = ImGui::GetWindowPos();
            float canvasX = (mousePos.x - windowPos.x - canvasPos.x) / zoom;
            float canvasY = (mousePos.y - windowPos.y - canvasPos.y) / zoom;

            // CRITICAL: Don't call AddNode() here — just store the request.
            // The node will be created in Phase 2 below, safely outside the
            // ImNodes editor scope.
            m_pendingNodeDrop = true;
            m_pendingNodeType = nodeType;
            m_pendingNodeX    = canvasX;
            m_pendingNodeY    = canvasY;
        }
        ImGui::EndDragDropTarget();
    }

    // ========================================================================
    // PHASE 2: Process pending node creation (outside editor scope).
    // AddNode() and SetNodeEditorSpacePos() are both safe here — the editor
    // context is fully closed (ImNodesScope_None).
    // ========================================================================
    if (m_pendingNodeDrop)
    {
        // Ensure positions are not garbage values (defend against FLT_MAX or corrupted memory)
        float safeX = m_pendingNodeX;
        float safeY = m_pendingNodeY;
        if (!std::isfinite(safeX) || !std::isfinite(safeY) || 
            safeX < -100000.0f || safeX > 100000.0f || 
            safeY < -100000.0f || safeY > 100000.0f)
        {
            safeX = 0.0f;
            safeY = 0.0f;
            SYSTEM_LOG << "[VSEditor] Warning: pending node position was garbage; reset to (0, 0)\n";
        }

        int newNodeID = AddNode(m_pendingNodeType, safeX, safeY);

        // Pre-register the position so ImNodes places the node correctly
        // on the very first frame it is rendered (next frame).
        ImNodes::SetNodeEditorSpacePos(newNodeID, ImVec2(safeX, safeY));

        m_dirty           = true;
        m_pendingNodeDrop = false;

        std::cout << "[VisualScriptEditorPanel] Node created: ID=" << newNodeID
                  << " type=" << static_cast<int>(m_pendingNodeType)
                  << " at (" << safeX << ", " << safeY << ")"
                  << std::endl;
    }

    // ========================================================================
    // PHASE 2: Process pending dynamic pin addition (outside editor scope).
    // The [+] button callback on VSSequence/Switch stores the request here; we
    // process it after EndNodeEditor so that AddDynamicPinCommand can safely
    // modify the template and trigger RebuildLinks().
    // ========================================================================
    if (m_pendingAddPin)
    {
        m_pendingAddPin = false;

        VSEditorNode* eNode = nullptr;
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_pendingAddPinNodeID)
            {
                eNode = &m_editorNodes[i];
                break;
            }
        }
        if (eNode != nullptr &&
            (eNode->def.Type == TaskNodeType::VSSequence ||
             eNode->def.Type == TaskNodeType::Switch))
        {
            int pinIdx = static_cast<int>(eNode->def.DynamicExecOutputPins.size()) + 1;
            std::string pinName;
            if (eNode->def.Type == TaskNodeType::VSSequence)
                pinName = "Out_" + std::to_string(pinIdx);
            else
                pinName = "Case_" + std::to_string(pinIdx);

            // Update editor-side def immediately
            eNode->def.DynamicExecOutputPins.push_back(pinName);

            // Push undo command (also updates template)
            m_undoStack.PushCommand(
                std::unique_ptr<ICommand>(
                    new AddDynamicPinCommand(m_pendingAddPinNodeID, pinName)),
                m_template);

            RebuildLinks();
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] AddDynamicPin: node #" << m_pendingAddPinNodeID
                       << " added pin '" << pinName << "'\n";
        }
    }

    // ========================================================================
    // PHASE 2: Process pending dynamic pin removal (outside editor scope).
    // The [-] button callback on dynamic pins stores the request here; we
    // process it after EndNodeEditor so that RemoveExecPinCommand can safely
    // modify the template and trigger RebuildLinks().
    // ========================================================================
    if (m_pendingRemovePin)
    {
        m_pendingRemovePin = false;

        VSEditorNode* eNode = nullptr;
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            if (m_editorNodes[i].nodeID == m_pendingRemovePinNodeID)
            {
                eNode = &m_editorNodes[i];
                break;
            }
        }
        if (eNode != nullptr &&
            m_pendingRemovePinDynIdx >= 0 &&
            m_pendingRemovePinDynIdx < static_cast<int>(eNode->def.DynamicExecOutputPins.size()))
        {
            const std::string pinName =
                eNode->def.DynamicExecOutputPins[static_cast<size_t>(m_pendingRemovePinDynIdx)];

            // Find any outgoing link from this pin in the template
            int32_t linkedTargetNodeID = -1;
            std::string linkedTargetPinName;
            for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
            {
                const ExecPinConnection& ec = m_template.ExecConnections[c];
                if (ec.SourceNodeID == m_pendingRemovePinNodeID &&
                    ec.SourcePinName == pinName)
                {
                    linkedTargetNodeID  = ec.TargetNodeID;
                    linkedTargetPinName = ec.TargetPinName;
                    break;
                }
            }

            // Update editor-side def immediately
            eNode->def.DynamicExecOutputPins.erase(
                eNode->def.DynamicExecOutputPins.begin() + m_pendingRemovePinDynIdx);

            // Push undo command (also updates template)
            m_undoStack.PushCommand(
                std::unique_ptr<ICommand>(
                    new RemoveExecPinCommand(m_pendingRemovePinNodeID,
                                             pinName,
                                             m_pendingRemovePinDynIdx,
                                             linkedTargetNodeID,
                                             linkedTargetPinName)),
                m_template);

            RebuildLinks();
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] RemoveDynamicPin: node #" << m_pendingRemovePinNodeID
                       << " removed pin '" << pinName << "'\n";
        }
    }

    // Track node moves for undo/redo using MoveNodeCommand.
    // Phase 19 — snapshot-at-click approach:
    //   Step 1 (MouseClicked)  : snapshot current eNode.posX/Y for all positioned nodes.
    //   Step 2 (MouseDown)     : keep eNode.posX/Y in sync with ImNodes live positions.
    //   Step 3 (MouseReleased) : for each snapshotted node, push MoveNodeCommand if
    //                            final position differs from snapshot by more than 1px.
    //
    // Only query nodes that have been rendered at least once (present in
    // m_positionedNodes) to avoid ImNodes assertions for brand-new nodes.
    {
        // Skip movement detection for one frame immediately after an undo/redo
        // so that stale ImNodes positions (not yet updated by the new render
        // cycle) are not mistaken for user-initiated drag-start positions.
        if (m_justPerformedUndoRedo)
        {
            m_justPerformedUndoRedo = false;
        }
        else
        {
        // Step 1: on the initial click, snapshot positions of all positioned nodes.
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_nodeDragStartPositions.clear();
            for (size_t i = 0; i < m_editorNodes.size(); ++i)
            {
                const VSEditorNode& eNode = m_editorNodes[i];
                if (m_positionedNodes.count(eNode.nodeID) == 0)
                    continue;
                m_nodeDragStartPositions[eNode.nodeID] =
                    std::make_pair(eNode.posX, eNode.posY);
            }
            //SYSTEM_LOG << "[VSEditor] Mouse clicked: snapshot " << static_cast<size_t>(m_nodeDragStartPositions.size()) << " node positions\n";
        }

        // Step 2: while mouse is held, keep eNode.posX/Y current (live Save support).
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            for (size_t i = 0; i < m_editorNodes.size(); ++i)
            {
                VSEditorNode& eNode = m_editorNodes[i];
                if (m_positionedNodes.count(eNode.nodeID) == 0)
                    continue;
                const ImVec2 pos = ImNodes::GetNodeEditorSpacePos(eNode.nodeID);
                eNode.posX = pos.x;
                eNode.posY = pos.y;
            }
        }

        // Step 3: on release, push MoveNodeCommand for any node that moved > 1px.
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            for (const auto& entry : m_nodeDragStartPositions)
            {
                const int   nodeID = entry.first;
                const float startX = entry.second.first;
                const float startY = entry.second.second;

                // CRITICAL FIX: Check if node still exists before querying ImNodes
                // The node could have been deleted or the canvas reloaded between mouse click and release.
                // Without this check, GetNodeEditorSpacePos() will assert on a non-existent node.
                if (m_positionedNodes.count(nodeID) == 0)
                    continue;  // Skip this node, it was deleted or canvas state changed

                const ImVec2 finalPos = ImNodes::GetNodeEditorSpacePos(nodeID);

                // Update eNode with final position
                for (size_t i = 0; i < m_editorNodes.size(); ++i)
                {
                    if (m_editorNodes[i].nodeID == nodeID)
                    {
                        m_editorNodes[i].posX = finalPos.x;
                        m_editorNodes[i].posY = finalPos.y;
                        break;
                    }
                }

                if (std::abs(finalPos.x - startX) > 1.0f ||
                    std::abs(finalPos.y - startY) > 1.0f)
                {
                    m_undoStack.PushCommand(
                        std::unique_ptr<ICommand>(
                            new MoveNodeCommand(nodeID,
                                                startX,    startY,
                                                finalPos.x, finalPos.y)),
                        m_template);
                    //SYSTEM_LOG << "[VSEditor] MoveNodeCommand pushed node #" << nodeID
                    //           << " (" << startX << "," << startY
                    //           << ") -> (" << finalPos.x << "," << finalPos.y
                    //           << ") [UNDOABLE]\n";
                    m_dirty = true;
                }
                //else
                //{
                //    SYSTEM_LOG << "[VSEditor] Node #" << nodeID
                //               << " not moved (delta < 1px), skipping\n";
                //}
            }
            m_nodeDragStartPositions.clear();
        }
        } // end !m_justPerformedUndoRedo
    }

    // Hover tooltip — ImNodes::IsNodeHovered() requires ImNodesScope_None,
    // so it must be called here (after EndNodeEditor), never inside the
    // BeginNodeEditor/EndNodeEditor block.
    {
        int hoveredNode = -1;
        if (ImNodes::IsNodeHovered(&hoveredNode))
        {
            auto it = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                   [hoveredNode](const VSEditorNode& n) {
                                       return n.nodeID == hoveredNode;
                                   });
            if (it != m_editorNodes.end())
            {
                const char* tip = GetNodeTypeLabel(it->def.Type);
                if (tip && tip[0] != '\0')
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(tip);
                    ImGui::EndTooltip();
                }
            }
        }
    }

    // Handle new link creation
    int startAttr = -1, endAttr = -1;
    if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
    {
        int startOffset = startAttr % 10000;
        int endOffset   = endAttr   % 10000;

        // Classify pin directions by offset range:
        //   0      → exec-in  (Input)
        //   100–199 → exec-out (Output)
        //   200–299 → data-in  (Input)
        //   300–399 → data-out (Output)
        bool startIsOutput = (startOffset >= 100 && startOffset < 200) ||
                             (startOffset >= 300 && startOffset < 400);
        bool endIsInput    = (endOffset == 0) ||
                             (endOffset >= 200 && endOffset < 300);

        // Auto-swap if user dragged backwards (Input -> Output).
        // ImNodes normalises the direction automatically (Output pin is always
        // returned as startAttr), so this branch fires only in edge cases where
        // the pin type could not be determined by ImNodes.
        if (!startIsOutput && endIsInput)
        {
            std::swap(startAttr, endAttr);
            startOffset   = startAttr % 10000;
            endOffset     = endAttr   % 10000;
            // Recalculate flags from the new offsets after swap.
            startIsOutput = (startOffset >= 100 && startOffset < 200) ||
                            (startOffset >= 300 && startOffset < 400);
            endIsInput    = (endOffset == 0) ||
                            (endOffset >= 200 && endOffset < 300);
        }

        if (startIsOutput && endIsInput)
        {
            const bool isExecLink = (startOffset >= 100 && startOffset < 200);
            const bool isDataLink = (startOffset >= 300 && startOffset < 400);
            const int  srcNodeID  = startAttr / 10000;
            const int  dstNodeID  = endAttr   / 10000;

            // Phase 24: CRITICAL - Prevent data links from connecting to exec-in pin (offset 0)
            // If this is a data link and destination is exec-in, find the first available data-in pin instead
            if (isDataLink && endOffset == 0)
            {
                // Data-to-exec mismatch detected. Try to find the first available data-in pin.
                auto dstIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [dstNodeID](const VSEditorNode& n) {
                                              return n.nodeID == dstNodeID;
                                          });

                if (dstIt != m_editorNodes.end() && 
                    dstIt->def.Type == TaskNodeType::Branch && 
                    !dstIt->def.dynamicPins.empty())
                {
                    // Force endAttr to point to the first dynamic data-in pin (offset 200)
                    endAttr = dstNodeID * 10000 + 200;
                    endOffset = 200;
                }
                else
                {
                    // No valid data-in pins available, reject this link
                    m_dirty = false;
                    // Skip link creation
                    startIsOutput = false;
                    endIsInput = false;
                }
            }

            // Only proceed if we still have valid pins to connect
            if (!startIsOutput || !endIsInput)
                return;

            if (isExecLink)
            {
                // Resolve source exec-out pin name from its index
                const int srcPinIndex = startOffset - 100;
                std::string srcPinName = "Out";

                auto srcIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [srcNodeID](const VSEditorNode& n) {
                                              return n.nodeID == srcNodeID;
                                          });
                if (srcIt != m_editorNodes.end())
                {
                    auto outPins = GetExecOutputPinsForNode(srcIt->def);
                    if (srcPinIndex < static_cast<int>(outPins.size()))
                        srcPinName = outPins[srcPinIndex];
                }

                if (VSConnectionValidator::IsExecConnectionValid(m_template, srcNodeID, srcPinName, dstNodeID))
                {
                    ConnectExec(srcNodeID, srcPinName, dstNodeID, "In");
                    SYSTEM_LOG << "[VSEditor] Created exec link: node #" << srcNodeID
                               << "." << srcPinName << " -> node #" << dstNodeID << ".In\n";
                    m_dirty = true;
                }
            }
            else if (isDataLink)
            {
                // Resolve source data-out and destination data-in pin names
                int srcPinIndex = startOffset - 300;
                int dstPinIndex = endOffset   - 200;
                std::string srcPinName = "Value";
                std::string dstPinName = "Value";

                auto srcIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [srcNodeID](const VSEditorNode& n) {
                                              return n.nodeID == srcNodeID;
                                          });
                auto dstIt = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                                          [dstNodeID](const VSEditorNode& n) {
                                              return n.nodeID == dstNodeID;
                                          });

                if (srcIt != m_editorNodes.end())
                {
                    // Try static pin list first, then fall back to DataPins vector
                    auto outPins = GetDataOutputPins(srcIt->def.Type);
                    if (srcPinIndex < static_cast<int>(outPins.size()))
                    {
                        srcPinName = outPins[srcPinIndex];
                    }
                    else
                    {
                        int outIdx = 0;
                        for (size_t p = 0; p < srcIt->def.DataPins.size(); ++p)
                        {
                            if (srcIt->def.DataPins[p].Dir == DataPinDir::Output)
                            {
                                if (outIdx == srcPinIndex)
                                {
                                    srcPinName = srcIt->def.DataPins[p].PinName;
                                    break;
                                }
                                ++outIdx;
                            }
                        }
                    }
                }

                if (dstIt != m_editorNodes.end())
                {
                    // Phase 24: Check if destination is a Branch node with dynamic pins
                    if (dstIt->def.Type == TaskNodeType::Branch)
                    {
                        // For Branch nodes, data-in pins start at offset 200
                        // If dstPinIndex is negative or out of range, force it to 0 (first pin)
                        if (dstPinIndex < 0 || dstPinIndex >= static_cast<int>(dstIt->def.dynamicPins.size()))
                        {
                            if (!dstIt->def.dynamicPins.empty())
                            {
                                dstPinIndex = 0;  // Force first available data-in pin
                                std::cerr << "[VSEditor] Data-in pin index corrected to 0 (first available)\n";
                            }
                        }

                        if (dstPinIndex >= 0 && dstPinIndex < static_cast<int>(dstIt->def.dynamicPins.size()))
                        {
                            // Use the dynamic pin's ID as the target pin name
                            dstPinName = dstIt->def.dynamicPins[dstPinIndex].id;
                        }
                        else
                        {
                            std::cerr << "[VSEditor] Cannot find valid data-in pin on Branch node\n";
                            return;  // Skip this link
                        }
                    }
                    else
                    {
                        // Fall back to static data pins
                        auto inPins = GetDataInputPins(dstIt->def.Type);
                        if (dstPinIndex < static_cast<int>(inPins.size()))
                        {
                            dstPinName = inPins[dstPinIndex];
                        }
                        else
                        {
                            int inIdx = 0;
                            for (size_t p = 0; p < dstIt->def.DataPins.size(); ++p)
                            {
                                if (dstIt->def.DataPins[p].Dir == DataPinDir::Input)
                                {
                                    if (inIdx == dstPinIndex)
                                    {
                                        dstPinName = dstIt->def.DataPins[p].PinName;
                                        break;
                                    }
                                    ++inIdx;
                                }
                            }
                        }
                    }
                }

                ConnectData(srcNodeID, srcPinName, dstNodeID, dstPinName);
                std::cout << "[VisualScriptEditorPanel] Created data link: node"
                          << srcNodeID << "." << srcPinName
                          << " -> node" << dstNodeID << "." << dstPinName << "\n";
                m_dirty = true;
            }
            else
            {
                std::cerr << "[VisualScriptEditorPanel] Cannot create link"
                             " — incompatible pin types (exec/data mismatch)\n";
            }
        }
        else
        {
            std::cerr << "[VisualScriptEditorPanel] Cannot create link"
                         " — incompatible pin types (both inputs or both outputs)\n";
        }
    }

    // Handle link deletion (triggered when the user Ctrl+clicks a link in ImNodes)
    int destroyedLink = -1;
    if (ImNodes::IsLinkDestroyed(&destroyedLink))
    {
        // Delegate to RemoveLink() so that:
        //   1. The underlying template connection is removed (not just the
        //      visual m_editorLinks entry).  Without this the connection would
        //      reappear as a "ghost" link the next time RebuildLinks() is called
        //      (e.g. after any undo/redo).
        //   2. A DeleteLinkCommand is pushed onto the undo stack, making the
        //      deletion reversible via Ctrl+Z.
        RemoveLink(destroyedLink);
    }

    // Handle node selection
    if (ImNodes::NumSelectedNodes() == 1)
    {
        int selNodes[1] = {-1};
        ImNodes::GetSelectedNodes(selNodes);
        m_selectedNodeID = selNodes[0];
    }
    else if (ImNodes::NumSelectedNodes() == 0)
    {
        m_selectedNodeID = -1;
    }

    // F9 = toggle breakpoint on selected node
    if (ImGui::IsKeyPressed(ImGuiKey_F9) && m_selectedNodeID >= 0)
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                "Node " + std::to_string(m_selectedNodeID));
    }

    // Delete key = remove all selected nodes and links
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && ImGui::IsWindowFocused())
    {
        int numSelectedNodes = ImNodes::NumSelectedNodes();
        if (numSelectedNodes > 0)
        {
            if (numSelectedNodes > 5)
            {
                std::cout << "[VSEditor] Warning: Deleting " << numSelectedNodes
                          << " nodes" << std::endl;
            }

            std::vector<int> selectedNodes(static_cast<size_t>(numSelectedNodes));
            ImNodes::GetSelectedNodes(selectedNodes.data());

            for (int nodeID : selectedNodes)
            {
                if (m_selectedNodeID == nodeID)
                    m_selectedNodeID = -1;
                RemoveNode(nodeID);
                std::cout << "[VSEditor] Deleted node " << nodeID << std::endl;
            }

            m_dirty = true;
        }

        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0)
        {
            std::vector<int> selectedLinks(static_cast<size_t>(numSelectedLinks));
            ImNodes::GetSelectedLinks(selectedLinks.data());

            for (int linkID : selectedLinks)
            {
                RemoveLink(linkID);
                std::cout << "[VSEditor] Deleted link " << linkID << std::endl;
            }

            m_dirty = true;
        }
    }

    RenderValidationOverlay();
}

void VisualScriptEditorPanel::RenderNodePalette()
{
    if (!ImGui::BeginPopup("VSNodePalette"))
        return;

    ImGui::TextDisabled("Add Node");
    ImGui::Separator();

    // Flow Control
    if (ImGui::BeginMenu("Flow Control"))
    {
        auto addFlowNode = [&](TaskNodeType type, const char* label) {
            if (ImGui::MenuItem(label))
            {
                AddNode(type, m_contextMenuX, m_contextMenuY);
                ImGui::CloseCurrentPopup();
            }
        };
        addFlowNode(TaskNodeType::EntryPoint, "EntryPoint");
        addFlowNode(TaskNodeType::Branch,     "Branch");
        addFlowNode(TaskNodeType::VSSequence, "Sequence");
        addFlowNode(TaskNodeType::While,      "While");
        addFlowNode(TaskNodeType::ForEach,    "ForEach");
        addFlowNode(TaskNodeType::DoOnce,     "DoOnce");
        addFlowNode(TaskNodeType::Delay,      "Delay");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Actions"))
    {
        if (ImGui::MenuItem("AtomicTask"))
        {
            AddNode(TaskNodeType::AtomicTask, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Data"))
    {
        if (ImGui::MenuItem("GetBBValue"))
        {
            AddNode(TaskNodeType::GetBBValue, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("SetBBValue"))
        {
            AddNode(TaskNodeType::SetBBValue, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::MenuItem("MathOp"))
        {
            AddNode(TaskNodeType::MathOp, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("SubGraph"))
    {
        if (ImGui::MenuItem("SubGraph"))
        {
            AddNode(TaskNodeType::SubGraph, m_contextMenuX, m_contextMenuY);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

void VisualScriptEditorPanel::RenderContextMenus()
{
    // ========================================================================
    // Node context menu
    // ========================================================================
    if (ImGui::BeginPopup("VSNodeContextMenu"))
    {
        if (ImGui::MenuItem("Edit Properties"))
        {
            m_selectedNodeID = m_contextNodeID;
            SYSTEM_LOG << "[VSEditor] Selected node #" << m_contextNodeID
                       << " for editing\n";
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Delete Node"))
        {
            RemoveNode(m_contextNodeID);
            if (m_selectedNodeID == m_contextNodeID)
                m_selectedNodeID = -1;
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] Deleted node #" << m_contextNodeID
                       << " via context menu\n";
        }

        ImGui::Separator();

        {
            bool hasBP = DebugController::Get().HasBreakpoint(0, m_contextNodeID);
            if (ImGui::MenuItem(hasBP ? "Remove Breakpoint (F9)" : "Add Breakpoint (F9)"))
            {
                DebugController::Get().ToggleBreakpoint(0, m_contextNodeID,
                                                        m_template.Name,
                                                        "Node " + std::to_string(m_contextNodeID));
                SYSTEM_LOG << "[VSEditor] Toggled breakpoint on node #"
                           << m_contextNodeID << " -> "
                           << (hasBP ? "OFF" : "ON") << "\n";
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Duplicate"))
        {
            auto it = std::find_if(m_editorNodes.begin(), m_editorNodes.end(),
                [this](const VSEditorNode& n) { return n.nodeID == m_contextNodeID; });
            if (it != m_editorNodes.end())
            {
                TaskNodeDefinition newDef = it->def;
                newDef.NodeID    = AllocNodeID();
                newDef.NodeName += " (Copy)";
                newDef.EditorPosX = it->posX + 50.0f;
                newDef.EditorPosY = it->posY + 50.0f;
                newDef.HasEditorPos = true;

                VSEditorNode eNew;
                eNew.nodeID = newDef.NodeID;
                eNew.posX   = newDef.EditorPosX;
                eNew.posY   = newDef.EditorPosY;
                eNew.def    = newDef;
                m_editorNodes.push_back(eNew);

                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new AddNodeCommand(newDef)),
                    m_template);
                m_dirty = true;
                SYSTEM_LOG << "[VSEditor] Node " << m_contextNodeID
                           << " duplicated as #" << newDef.NodeID << "\n";
            }
        }

        ImGui::EndPopup();
    }

    // ========================================================================
    // Link context menu
    // ========================================================================
    if (ImGui::BeginPopup("VSLinkContextMenu"))
    {
        if (ImGui::MenuItem("Delete Connection"))
        {
            RemoveLink(m_contextLinkID);
            m_dirty = true;
            SYSTEM_LOG << "[VSEditor] Deleted link #" << m_contextLinkID
                       << " via context menu\n";
        }
        ImGui::EndPopup();
    }
}

// ============================================================================
// Branch / While node — dedicated Properties panel renderer
// ============================================================================

void VisualScriptEditorPanel::RenderBranchNodeProperties(VSEditorNode& eNode,
                                                         TaskNodeDefinition& def)
{
    // ── Blue header: node name (matches canvas Section 1 title bar) ──────────
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Structured Conditions (Phase 24 — NodeConditionsPanel) ───────────────
    if (m_conditionsPanel)
    {
        // Reload the panel when the selected node changes.
        if (m_condPanelNodeID != eNode.nodeID)
        {
            m_condPanelNodeID = eNode.nodeID;
            m_conditionsPanel->SetNodeName(def.NodeName);
            m_conditionsPanel->SetConditionRefs(def.conditionRefs);
            m_conditionsPanel->SetDynamicPins(def.dynamicPins);
            m_conditionsPanel->ClearDirty();
        }
        else
        {
            // Keep node name in sync with any in-frame name edits.
            m_conditionsPanel->SetNodeName(def.NodeName);
        }

        m_conditionsPanel->Render();

        if (m_conditionsPanel->IsDirty())
        {
            def.conditionRefs = m_conditionsPanel->GetConditionRefs();

            // Keep m_template in sync for serialization.
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].conditionRefs = def.conditionRefs;
                    break;
                }
            }
            m_conditionsPanel->ClearDirty();
            m_dirty = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ── Breakpoint checkbox (F9) ─────────────────────────────────────────────
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##vsbp_branch", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }

    RenderVerificationPanel();

    (void)eNode; // suppress unused-warning when branches have no eNode-specific fields
}

// ============================================================================
// MathOp node — dedicated Properties panel renderer
// ============================================================================

void VisualScriptEditorPanel::RenderMathOpNodeProperties(VSEditorNode& eNode,
                                                        TaskNodeDefinition& def)
{
    // ── Blue header: node name (matches canvas Section 1 title bar) ──────────
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.0f, 0.4f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.5f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.0f, 0.3f, 0.7f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable(def.NodeName.c_str(), true,
                      ImGuiSelectableFlags_None, ImVec2(0.f, 28.f));
    ImGui::PopStyleColor(4);

    ImGui::Separator();
    ImGui::Spacing();

    // ── Operand Editor (Phase 24 Milestone 2 — MathOpPropertyPanel) ───────────
    if (m_mathOpPanel)
    {
        // Lazy-initialize the panel when node changes
        if (!m_mathOpPanel)
        {
            m_mathOpPanel = std::unique_ptr<MathOpPropertyPanel>(
                new MathOpPropertyPanel(m_presetRegistry, *m_pinManager));
        }

        m_mathOpPanel->SetNodeName(def.NodeName);
        m_mathOpPanel->SetMathOpRef(def.mathOpRef);
        m_mathOpPanel->SetDynamicPins(def.dynamicPins);

        m_mathOpPanel->SetOnOperandChange([this]() {
            // Callback when operands change: regenerate dynamic pins
            if (m_pinManager && m_selectedNodeID >= 0)
            {
                for (size_t i = 0; i < m_editorNodes.size(); ++i)
                {
                    if (m_editorNodes[i].nodeID == m_selectedNodeID)
                    {
                        m_editorNodes[i].def.mathOpRef = m_mathOpPanel->GetMathOpRef();
                        break;
                    }
                }
                m_dirty = true;
            }
        });

        m_mathOpPanel->Render();

        if (m_mathOpPanel->IsDirty())
        {
            def.mathOpRef = m_mathOpPanel->GetMathOpRef();

            // Keep m_template in sync for serialization
            for (size_t ti = 0; ti < m_template.Nodes.size(); ++ti)
            {
                if (m_template.Nodes[ti].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[ti].mathOpRef = def.mathOpRef;
                    break;
                }
            }
            m_mathOpPanel->ClearDirty();
            m_dirty = true;
        }
    }

    ImGui::Separator();
    ImGui::Spacing();

    RenderVerificationPanel();

    (void)eNode; // suppress unused-warning
}

void VisualScriptEditorPanel::RenderNodeDataParameters(TaskNodeDefinition& def)
{
    // Phase 24 — Generic parameter editor for data nodes (GetBBValue, SetBBValue, MathOp)
    // Allows storing and serializing additional parameters on data nodes

    // Filter out system parameters (those starting with __)
    std::vector<std::string> userParams;
    for (const auto& paramPair : def.Parameters)
    {
        const std::string& paramName = paramPair.first;
        // Skip system parameters
        if (paramName.length() >= 2 && paramName[0] == '_' && paramName[1] == '_')
            continue;
        userParams.push_back(paramName);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Node Parameters:");

    if (userParams.empty())
    {
        ImGui::TextDisabled("(no user parameters - add one below)");
    }

    // Display user parameters
    for (const auto& paramName : userParams)
    {
        auto paramIt = def.Parameters.find(paramName);
        if (paramIt == def.Parameters.end())
            continue;

        ParameterBinding& binding = paramIt->second;

        ImGui::PushID(paramName.c_str());

        // Display parameter name
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "%s", paramName.c_str());

        // Build a label showing the binding type
        const char* typeLabel = "?";
        switch (binding.Type)
        {
            case ParameterBindingType::Literal:       typeLabel = "Literal"; break;
            case ParameterBindingType::LocalVariable:  typeLabel = "Variable"; break;
            case ParameterBindingType::AtomicTaskID:   typeLabel = "AtomicTaskID"; break;
            case ParameterBindingType::ConditionID:    typeLabel = "ConditionID"; break;
            case ParameterBindingType::MathOperator:   typeLabel = "MathOp"; break;
            case ParameterBindingType::ComparisonOp:   typeLabel = "CompOp"; break;
            case ParameterBindingType::SubGraphPath:   typeLabel = "SubGraph"; break;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", typeLabel);

        // Input field for editing parameter value
        if (binding.Type == ParameterBindingType::Literal)
        {
            // For literal values, show an input field
            std::string currentValue;
            if (!binding.LiteralValue.IsNone())
            {
                currentValue = binding.LiteralValue.AsString();
            }

            char buf[256];
            strncpy_s(buf, sizeof(buf), currentValue.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##" + paramName + "_val").c_str(), buf, sizeof(buf)))
            {
                // Parse and store the value
                std::string strVal(buf);
                binding.LiteralValue = TaskValue(strVal);

                // Keep template in sync
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].Parameters[paramName] = binding;
                        break;
                    }
                }
                m_dirty = true;
            }
        }
        else if (binding.Type == ParameterBindingType::LocalVariable)
        {
            // For local variables, show a dropdown of available variables
            BBVariableRegistry bbReg;
            bbReg.LoadFromTemplate(m_template);
            const std::vector<VarSpec> vars = bbReg.GetAllVariables();

            const char* preview = binding.VariableName.empty() ? "(select...)" : binding.VariableName.c_str();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo(("##" + paramName + "_var").c_str(), preview))
            {
                for (const auto& var : vars)
                {
                    bool selected = (var.name == binding.VariableName);
                    if (ImGui::Selectable(var.displayLabel.c_str(), selected))
                    {
                        binding.VariableName = var.name;

                        // Keep template in sync
                        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                        {
                            if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                            {
                                m_template.Nodes[i].Parameters[paramName] = binding;
                                break;
                            }
                        }
                        m_dirty = true;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            // For other types, show a text field for the identifier
            char buf[256];
            strncpy_s(buf, sizeof(buf), binding.VariableName.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText(("##" + paramName + "_id").c_str(), buf, sizeof(buf)))
            {
                binding.VariableName = buf;

                // Keep template in sync
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].Parameters[paramName] = binding;
                        break;
                    }
                }
                m_dirty = true;
            }
        }

        ImGui::PopID();
    }

    // Add parameter section
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Add Parameter:");

    static char paramNameBuf[256] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80.0f);
    ImGui::InputText("##new_param_name", paramNameBuf, sizeof(paramNameBuf), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if (ImGui::Button("Add", ImVec2(70.0f, 0.0f)))
    {
        std::string newParamName(paramNameBuf);
        if (!newParamName.empty() && def.Parameters.find(newParamName) == def.Parameters.end())
        {
            // Create new parameter with default Literal binding
            ParameterBinding newBinding;
            newBinding.Type = ParameterBindingType::Literal;
            newBinding.LiteralValue = TaskValue("");

            def.Parameters[newParamName] = newBinding;

            // Keep template in sync
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].Parameters[newParamName] = newBinding;
                    break;
                }
            }

            m_dirty = true;
            paramNameBuf[0] = '\0';  // Clear the input field
        }
    }
}

void VisualScriptEditorPanel::RenderProperties()
{
    ImGui::TextDisabled("Properties");

    if (m_selectedNodeID < 0)
    {
        ImGui::TextDisabled("(select a node)");
        return;
    }

    // Find the editor node
    VSEditorNode* eNode = nullptr;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        if (m_editorNodes[i].nodeID == m_selectedNodeID)
        {
            eNode = &m_editorNodes[i];
            break;
        }
    }
    if (eNode == nullptr)
        return;

    TaskNodeDefinition& def = eNode->def;

    // Reset focus-node tracking when the selected node changes.
    // Old-value snapshots do NOT need explicit resetting here — they are
    // naturally overwritten by the next IsItemActivated() event.
    m_propEditNodeIDOnFocus = m_selectedNodeID;

    // ---- NodeName (present for all node types) ----
    {
        char nameBuf[128];
        strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name##vsname", nameBuf, sizeof(nameBuf)))
        {
            def.NodeName = nameBuf;
            // Sync live to template for immediate canvas display and serialization
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].NodeName = def.NodeName;
                    break;
                }
            }
            m_dirty = true;
        }
        if (ImGui::IsItemActivated())
        {
            m_propEditOldName       = def.NodeName;
            m_propEditNodeIDOnFocus = m_selectedNodeID;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() &&
            m_propEditNodeIDOnFocus == m_selectedNodeID &&
            def.NodeName != m_propEditOldName)
        {
            m_undoStack.PushCommand(
                std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                    m_selectedNodeID, "NodeName",
                    PropertyValue::FromString(m_propEditOldName),
                    PropertyValue::FromString(def.NodeName))),
                m_template);
        }
    }

    // ---- Type-specific fields — all buffers are local (non-static) to avoid
    //      stale data when switching between selected nodes. ----
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:
        {
            // --- AtomicTaskID dropdown ---
            const std::vector<TaskSpec> tasks = AtomicTaskUIRegistry::Get().GetSortedForUI();
            const std::string& currentTask = def.AtomicTaskID;
            const char* previewLabel = currentTask.empty() ? "(select task...)" : currentTask.c_str();

            if (ImGui::IsItemActivated())
            {
                m_propEditOldTaskID     = def.AtomicTaskID;
                m_propEditNodeIDOnFocus = m_selectedNodeID;
            }

            if (ImGui::BeginCombo("TaskType##vstask", previewLabel))
            {
                if (m_propEditOldTaskID != def.AtomicTaskID)
                {
                    m_propEditOldTaskID     = def.AtomicTaskID;
                    m_propEditNodeIDOnFocus = m_selectedNodeID;
                }
                std::string lastCat;
                for (size_t ti = 0; ti < tasks.size(); ++ti)
                {
                    const TaskSpec& spec = tasks[ti];
                    // Show category header separator when category changes
                    if (spec.category != lastCat)
                    {
                        if (!lastCat.empty())
                            ImGui::Separator();
                        ImGui::TextDisabled("%s", spec.category.c_str());
                        lastCat = spec.category;
                    }
                    bool selected = (spec.id == currentTask);
                    std::string label = "  " + spec.displayName + "##" + spec.id;
                    if (ImGui::Selectable(label.c_str(), selected))
                    {
                        const std::string oldTaskID = def.AtomicTaskID;
                        def.AtomicTaskID = spec.id;
                        // Auto-fill node name with the action's display name
                        def.NodeName = spec.displayName;
                        for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                        {
                            if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                            {
                                m_template.Nodes[i].AtomicTaskID = def.AtomicTaskID;
                                m_template.Nodes[i].NodeName = def.NodeName;
                                break;
                            }
                        }
                        if (def.AtomicTaskID != oldTaskID)
                        {
                            m_undoStack.PushCommand(
                                std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                    m_selectedNodeID, "AtomicTaskID",
                                    PropertyValue::FromString(oldTaskID),
                                    PropertyValue::FromString(def.AtomicTaskID))),
                                m_template);
                        }
                        m_dirty = true;
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                    // Tooltip with description
                    if (ImGui::IsItemHovered() && !spec.description.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted(spec.description.c_str());
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndCombo();
            }
            break;
        }
        case TaskNodeType::Delay:
        {
            float delay = def.DelaySeconds;
            if (ImGui::InputFloat("Delay (s)##vsdelay", &delay, 0.1f, 1.0f))
            {
                def.DelaySeconds = delay;
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].DelaySeconds = def.DelaySeconds;
                        break;
                    }
                }
                m_dirty = true;
            }
            if (ImGui::IsItemActivated())
            {
                m_propEditOldDelay      = def.DelaySeconds;
                m_propEditNodeIDOnFocus = m_selectedNodeID;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() &&
                m_propEditNodeIDOnFocus == m_selectedNodeID &&
                def.DelaySeconds != m_propEditOldDelay)
            {
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                        m_selectedNodeID, "DelaySeconds",
                        PropertyValue::FromFloat(m_propEditOldDelay),
                        PropertyValue::FromFloat(def.DelaySeconds))),
                    m_template);
            }
            break;
        }
        case TaskNodeType::GetBBValue:
        {
            // Phase 24 Milestone 3: Delegate to dedicated GetBBValue properties renderer
            if (m_getBBPanel)
            {
                m_getBBPanel->SetNodeName(def.NodeName);
                m_getBBPanel->SetTemplate(&m_template);
                m_getBBPanel->SetBBKey(def.BBKey);

                m_getBBPanel->Render();

                if (m_getBBPanel->IsDirty())
                {
                    const std::string oldKey = def.BBKey;
                    def.BBKey = m_getBBPanel->GetBBKey();

                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].BBKey = def.BBKey;
                            break;
                        }
                    }

                    if (def.BBKey != oldKey)
                    {
                        m_undoStack.PushCommand(
                            std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                m_selectedNodeID, "BBKey",
                                PropertyValue::FromString(oldKey),
                                PropertyValue::FromString(def.BBKey))),
                            m_template);
                    }
                    m_getBBPanel->ClearDirty();
                    m_dirty = true;
                }
            }

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            break;
        }
        case TaskNodeType::SetBBValue:
        {
            // Phase 24 Milestone 3: Delegate to dedicated SetBBValue properties renderer
            if (m_setBBPanel)
            {
                m_setBBPanel->SetNodeName(def.NodeName);
                m_setBBPanel->SetTemplate(&m_template);
                m_setBBPanel->SetBBKey(def.BBKey);

                m_setBBPanel->Render();

                if (m_setBBPanel->IsDirty())
                {
                    const std::string oldKey = def.BBKey;
                    def.BBKey = m_setBBPanel->GetBBKey();

                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].BBKey = def.BBKey;
                            break;
                        }
                    }

                    if (def.BBKey != oldKey)
                    {
                        m_undoStack.PushCommand(
                            std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                m_selectedNodeID, "BBKey",
                                PropertyValue::FromString(oldKey),
                                PropertyValue::FromString(def.BBKey))),
                            m_template);
                    }
                    m_setBBPanel->ClearDirty();
                    m_dirty = true;
                }
            }

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            break;
        }
        case TaskNodeType::Branch:
        case TaskNodeType::While:
        {
            // Delegate to the dedicated Phase 24-Rendering branch properties renderer.
            // This shows: blue header → NodeConditionsPanel → Breakpoint checkbox.
            // The return prevents any legacy condition UI from also rendering.
            RenderBranchNodeProperties(*eNode, def);
            return;
        }
        case TaskNodeType::SubGraph:
        {
            char sgPathBuf[256];
            strncpy_s(sgPathBuf, sizeof(sgPathBuf), def.SubGraphPath.c_str(), _TRUNCATE);
            if (ImGui::InputText("SubGraph Path##vssg", sgPathBuf, sizeof(sgPathBuf)))
            {
                def.SubGraphPath = sgPathBuf;
                for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                {
                    if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                    {
                        m_template.Nodes[i].SubGraphPath = def.SubGraphPath;
                        break;
                    }
                }
                m_dirty = true;
            }
            if (ImGui::IsItemActivated())
            {
                m_propEditOldSubGraphPath = def.SubGraphPath;
                m_propEditNodeIDOnFocus   = m_selectedNodeID;
            }
            if (ImGui::IsItemDeactivatedAfterEdit() &&
                m_propEditNodeIDOnFocus == m_selectedNodeID &&
                def.SubGraphPath != m_propEditOldSubGraphPath)
            {
                m_undoStack.PushCommand(
                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                        m_selectedNodeID, "SubGraphPath",
                        PropertyValue::FromString(m_propEditOldSubGraphPath),
                        PropertyValue::FromString(def.SubGraphPath))),
                    m_template);
            }
            break;
        }
        case TaskNodeType::MathOp:
        {
            // Phase 24 Milestone 2: Delegate to the dedicated MathOp properties renderer.
            // This shows: blue header → MathOpPropertyPanel → operand editors.
            RenderMathOpNodeProperties(*eNode, def);

            // Render node parameters (Phase 24 — node data serialization)
            RenderNodeDataParameters(def);
            return;
        }
        case TaskNodeType::Switch:
        {
            // Sync m_propEditSwitchCases with the node's switchCases when node changes
            if (m_propEditNodeIDOnFocus != m_selectedNodeID)
                m_propEditSwitchCases = def.switchCases;

            // Find the corresponding template node once for all edits below
            TaskNodeDefinition* tmplNode = nullptr;
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    tmplNode = &m_template.Nodes[i];
                    break;
                }
            }

            // ---- Switch Variable ----
            {
                // Dropdown populated from Int-typed blackboard variables only
                // (Switch node evaluates an Int variable against integer case values)
                BBVariableRegistry bbReg;
                bbReg.LoadFromTemplate(m_template);
                const std::vector<VarSpec> vars = bbReg.GetVariablesByType(VariableType::Int);
                const std::string& curVar   = def.switchVariable;
                const char* previewVar      = curVar.empty() ? "(select variable...)" : curVar.c_str();

                if (ImGui::BeginCombo("Switch Var##vsswitchvar", previewVar))
                {
                    for (size_t vi = 0; vi < vars.size(); ++vi)
                    {
                        const VarSpec& v = vars[vi];
                        bool selected    = (v.name == curVar);
                        if (ImGui::Selectable(v.displayLabel.c_str(), selected))
                        {
                            def.switchVariable = v.name;
                            if (tmplNode)
                                tmplNode->switchVariable = def.switchVariable;
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }

            // ---- Case Labels ----
            if (!def.switchCases.empty())
            {
                ImGui::Separator();
                ImGui::TextDisabled("Case Labels");
            }

            // Ensure our edit buffer stays in sync
            if (m_propEditSwitchCases.size() != def.switchCases.size())
                m_propEditSwitchCases = def.switchCases;

            for (size_t ci = 0; ci < def.switchCases.size(); ++ci)
            {
                // Show pin name as read-only, allow editing the custom label
                const std::string pinLabel = def.switchCases[ci].pinName
                    + " (val=" + def.switchCases[ci].value + ")";
                ImGui::TextUnformatted(pinLabel.c_str());
                ImGui::SameLine();

                char labelBuf[64];
                const std::string& curLabel = m_propEditSwitchCases[ci].customLabel;
                strncpy_s(labelBuf, sizeof(labelBuf), curLabel.c_str(), _TRUNCATE);

                // Unique widget ID per case index
                std::string widgetID = "##vscaselabel" + std::to_string(ci);
                if (ImGui::InputText(widgetID.c_str(), labelBuf, sizeof(labelBuf)))
                {
                    m_propEditSwitchCases[ci].customLabel = labelBuf;
                    // Apply to the live def and template immediately
                    def.switchCases[ci].customLabel = labelBuf;
                    if (tmplNode && ci < tmplNode->switchCases.size())
                        tmplNode->switchCases[ci].customLabel = labelBuf;
                    m_dirty = true;
                }
            }
            break;
        }
        default:
            break;
    }

    // Breakpoint toggle button
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##vsbp", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }

    RenderVerificationPanel();
}

void VisualScriptEditorPanel::RenderBlackboard()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    // BUG-001 Hotfix: warn user if invalid entries exist (key empty or type None)
    // to prevent save crash caused by unhandled None type during serialization.
    bool hasInvalid = false;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(i)];
        if (entry.Key.empty() || entry.Type == VariableType::None)
        {
            hasInvalid = true;
            break;
        }
    }
    if (hasInvalid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted("[!] Invalid entries will be skipped on save");
        ImGui::PopStyleColor();
    }

    // Add entry button — BUG-001 Hotfix: init with safe defaults (non-empty key, Int type)
    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    // List existing entries
    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        // Use a local (non-static) buffer per iteration to avoid sharing across entries
        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_dirty   = true;
        }
        ImGui::SameLine();

        // Fix #2: Type selector — "None" is excluded to prevent invalid entries.
        // Enum layout: None=0, Bool=1, Int=2, Float=3, Vector=4, EntityID=5, String=6.
        // typeIdx maps to enum value minus 1 (offset by 1 to skip None).
        const char* typeLabels[] = {"Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type) - 1; // offset: Bool->0, Int->1, ...
        if (typeIdx < 0 || typeIdx >= 6)
        {
            typeIdx    = 1; // default to "Int" (array index 1; maps to VariableType::Int via typeIdx+1)
            entry.Type = VariableType::Int;
        }
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &typeIdx, typeLabels, 6))
        {
            VariableType newType = static_cast<VariableType>(typeIdx + 1); // +1 to skip None
            entry.Type    = newType;
            entry.Default = GetDefaultValueForType(newType);  // UX Fix #1: sync default
            m_dirty       = true;
        }
        ImGui::SameLine();

        // IsGlobal checkbox
        ImGui::Checkbox("G##bbglob", &entry.IsGlobal);
        ImGui::SameLine();

        // Remove button
        if (ImGui::SmallButton("x##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
            ImGui::PopID();
            continue;
        }

        // UX Fix #2: Default value editor (type-specific input field)
        ImGui::TextDisabled("Default:");
        ImGui::SameLine();
        switch (entry.Type)
        {
            case VariableType::Bool:
            {
                bool bVal = entry.Default.IsNone() ? false : entry.Default.AsBool();
                if (ImGui::Checkbox("##bbval", &bVal))
                {
                    entry.Default = TaskValue(bVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Int:
            {
                int iVal = entry.Default.IsNone() ? 0 : entry.Default.AsInt();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputInt("##bbval", &iVal))
                {
                    entry.Default = TaskValue(iVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Float:
            {
                float fVal = entry.Default.IsNone() ? 0.0f : entry.Default.AsFloat();
                ImGui::SetNextItemWidth(70.0f);
                if (ImGui::InputFloat("##bbval", &fVal, 0.0f, 0.0f, "%.3f"))
                {
                    entry.Default = TaskValue(fVal);
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::String:
            {
                std::string sVal = entry.Default.IsNone() ? "" : entry.Default.AsString();
                char sBuf[128];
                strncpy_s(sBuf, sizeof(sBuf), sVal.c_str(), _TRUNCATE);
                ImGui::SetNextItemWidth(100.0f);
                if (ImGui::InputText("##bbval", sBuf, sizeof(sBuf)))
                {
                    entry.Default = TaskValue(std::string(sBuf));
                    m_dirty       = true;
                }
                break;
            }
            case VariableType::Vector:
            {
                // UX Enhancement #1: Vector is auto-sourced from entity position at runtime.
                // Display as read-only to prevent user from entering a value that will be
                // overwritten anyway.
                ImGui::BeginDisabled(true);
                float vecVal[3] = { 0.0f, 0.0f, 0.0f };
                ImGui::SetNextItemWidth(140.0f);
                ImGui::DragFloat3("##bbval", vecVal, 0.1f);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(auto from entity position)");
                break;
            }
            case VariableType::EntityID:
            {
                // UX Enhancement #2: EntityID is assigned at runtime; read-only display.
                ImGui::BeginDisabled(true);
                int entityId = 0;
                ImGui::SetNextItemWidth(70.0f);
                ImGui::InputInt("##bbval", &entityId);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("(assigned at runtime)");
                break;
            }
            default:
                ImGui::TextDisabled("(n/a)");
                break;
        }

        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderValidationOverlay()
{
    m_validationWarnings.clear();
    m_validationErrors.clear();

    // Check: every non-EntryPoint node should have at least one exec-in connection
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        const VSEditorNode& eNode = m_editorNodes[i];
        if (eNode.def.Type == TaskNodeType::EntryPoint)
            continue;

        bool hasExecIn = false;
        for (size_t c = 0; c < m_template.ExecConnections.size(); ++c)
        {
            if (m_template.ExecConnections[c].TargetNodeID == eNode.nodeID)
            {
                hasExecIn = true;
                break;
            }
        }
        if (!hasExecIn)
        {
            m_validationErrors.push_back(
                "Node " + std::to_string(eNode.nodeID) + " (" +
                eNode.def.NodeName + "): no exec-in connection");
        }

        // SubGraph path validation
        if (eNode.def.Type == TaskNodeType::SubGraph &&
            eNode.def.SubGraphPath.empty())
        {
            m_validationWarnings.push_back(
                "Node " + std::to_string(eNode.nodeID) +
                " (SubGraph): SubGraphPath is empty");
        }
    }
}

// ============================================================================
// Phase 21-B — Graph Verification
// ============================================================================

void VisualScriptEditorPanel::RunVerification()
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() called for graph '"
               << m_template.Name << "'\n";
    m_verificationResult = VSGraphVerifier::Verify(m_template);
    m_verificationDone   = true;

    // Phase 24.3 — Populate verification logs for display in the output panel
    m_verificationLogs.clear();
    for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
    {
        const VSVerificationIssue& issue = m_verificationResult.issues[i];
        std::string logEntry;

        // Format: "[SEVERITY] message (Node: nodeID)"
        if (issue.severity == VSVerificationSeverity::Error)
            logEntry = "[ERROR] ";
        else if (issue.severity == VSVerificationSeverity::Warning)
            logEntry = "[WARN] ";
        else
            logEntry = "[INFO] ";

        logEntry += issue.message;
        if (issue.nodeID >= 0)
            logEntry += " (Node: " + std::to_string(issue.nodeID) + ")";

        m_verificationLogs.push_back(logEntry);
    }

    SYSTEM_LOG << "[VisualScriptEditorPanel] RunVerification() done: "
               << m_verificationResult.issues.size() << " issue(s), "
               << "errors=" << (m_verificationResult.HasErrors()   ? "yes" : "no") << ", "
               << "warnings=" << (m_verificationResult.HasWarnings() ? "yes" : "no") << "\n";
}

void VisualScriptEditorPanel::RenderVerificationPanel()
{
    ImGui::Separator();
    ImGui::TextDisabled("Graph Verification");

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("Click 'Verify' in toolbar to run verification.");
        return;
    }

    // Global status line
    if (m_verificationResult.HasErrors())
    {
        int errorCount = 0;
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == VSVerificationSeverity::Error)
                ++errorCount;
        }
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "Errors found: %d", errorCount);
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "OK — warnings present");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "OK — no issues");
    }

    if (m_verificationResult.issues.empty())
        return;

    // List issues grouped: Errors first, then Warnings, then Info
    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            ImGui::PushID(static_cast<int>(i));

            if (sev == VSVerificationSeverity::Error)
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "[E]");
            else if (sev == VSVerificationSeverity::Warning)
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[W]");
            else
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[I]");

            ImGui::SameLine();
            ImGui::Text("%s: %s", issue.ruleID.c_str(), issue.message.c_str());

            if (issue.nodeID >= 0)
            {
                ImGui::SameLine();
                std::string btnLabel = "Go##go" + std::to_string(i);
                if (ImGui::SmallButton(btnLabel.c_str()))
                {
                    m_focusNodeID    = issue.nodeID;
                    m_selectedNodeID = issue.nodeID;
                }
            }

            ImGui::PopID();
        }
    }
}

// ============================================================================
// Phase 24.3 — Verification Logs Panel
// ============================================================================

void VisualScriptEditorPanel::RenderVerificationLogsPanel()
{
    // Note: The header "Verification Output" is rendered by the container (BlueprintEditorGUI),
    // so we only render the content here (status + logs).

    if (!m_verificationDone)
    {
        ImGui::TextDisabled("(Click 'Verify' button to run verification)");
        return;
    }

    // Display verification result summary
    ImGui::Spacing();

    // Debug: Show issue count
    ImGui::TextDisabled("Issues found: %zu", m_verificationResult.issues.size());

    // Status line with color coding
    if (m_verificationResult.HasErrors())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                          "[ERROR] Graph has %d error(s)", 
                          (int)m_verificationResult.issues.size());
    }
    else if (m_verificationResult.HasWarnings())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), 
                          "[WARNING] Graph is valid but has warnings");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                          "[OK] Graph is valid - no issues found");
    }

    ImGui::Separator();

    // Display issues grouped by severity
    ImGui::BeginChild("VerificationLogsChild", ImVec2(0, 0), true);

    const VSVerificationSeverity orderedSev[3] = {
        VSVerificationSeverity::Error,
        VSVerificationSeverity::Warning,
        VSVerificationSeverity::Info
    };

    const char* sevLabels[3] = { "[ERROR]", "[WARN]", "[INFO]" };
    ImVec4      sevColors[3] = {
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Error: red
        ImVec4(1.0f, 0.85f, 0.0f, 1.0f), // Warning: yellow
        ImVec4(0.5f, 0.8f, 1.0f, 1.0f)   // Info: light blue
    };

    for (int s = 0; s < 3; ++s)
    {
        VSVerificationSeverity sev = orderedSev[s];
        bool hasThisSeverity = false;

        // Count issues of this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            if (m_verificationResult.issues[i].severity == sev)
            {
                hasThisSeverity = true;
                break;
            }
        }

        if (!hasThisSeverity)
            continue;

        // Display header for this severity level
        ImGui::TextColored(sevColors[s], "%s", sevLabels[s]);

        // Display all issues with this severity
        for (size_t i = 0; i < m_verificationResult.issues.size(); ++i)
        {
            const VSVerificationIssue& issue = m_verificationResult.issues[i];
            if (issue.severity != sev)
                continue;

            // Format message: "[SEVERITY] message (NodeID: xxx)"
            std::string message = issue.message;
            if (issue.nodeID >= 0)
            {
                message += " (Node: " + std::to_string(issue.nodeID) + ")";
            }

            ImGui::BulletText("%s", message.c_str());
        }

        ImGui::Spacing();
    }

    ImGui::EndChild();
}

// ============================================================================
// Phase 23-B.4 — Condition Editor UI helpers
// ============================================================================

void VisualScriptEditorPanel::RenderConditionEditor(
    Condition& condition,
    int conditionIndex,
    const std::vector<BlackboardEntry>& allVars,
    const std::vector<std::string>& availablePins)
{
    ImGui::PushID(conditionIndex);
    ImGui::Separator();
    ImGui::Text("Condition #%d", conditionIndex + 1);

    // -- LEFT SIDE --
    ImGui::Text("Left:");
    ImGui::SameLine();

    const bool isLeftPin   = (condition.leftMode == "Pin");
    const bool isLeftVar   = (condition.leftMode == "Variable");
    const bool isLeftConst = (condition.leftMode == "Const");

    if (ImGui::Button(isLeftPin ? "[PIN]" : "Pin", ImVec2(55, 0)))
    {
        condition.leftMode = "Pin";
        condition.leftPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftVar ? "[VAR]" : "Var", ImVec2(55, 0)))
    {
        condition.leftMode     = "Variable";
        condition.leftVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isLeftConst ? "[CST]" : "Const", ImVec2(55, 0)))
    {
        condition.leftMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.leftMode == "Pin")
        RenderPinSelector(condition.leftPin, availablePins, "##leftpin");
    else if (condition.leftMode == "Variable")
        RenderVariableSelector(condition.leftVariable, allVars,
                               condition.compareType, "##leftvar");
    else
        RenderConstValueInput(condition.leftConstValue,
                              condition.compareType, "##leftconst");
    ImGui::Unindent();

    // -- OPERATOR --
    ImGui::Text("Op:");
    ImGui::SameLine();
    const char* operators[] = { "==", "!=", "<", ">", "<=", ">=" };
    int opIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.operatorStr == operators[i])
        {
            opIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(70.0f);
    if (ImGui::Combo("##op", &opIdx, operators, 6))
    {
        condition.operatorStr = operators[opIdx];
        m_dirty = true;
    }

    // -- RIGHT SIDE --
    ImGui::Text("Right:");
    ImGui::SameLine();

    const bool isRightPin   = (condition.rightMode == "Pin");
    const bool isRightVar   = (condition.rightMode == "Variable");
    const bool isRightConst = (condition.rightMode == "Const");

    if (ImGui::Button(isRightPin ? "[PIN]##r" : "Pin##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Pin";
        condition.rightPin  = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightVar ? "[VAR]##r" : "Var##r", ImVec2(55, 0)))
    {
        condition.rightMode     = "Variable";
        condition.rightVariable = "";
        m_dirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(isRightConst ? "[CST]##r" : "Const##r", ImVec2(55, 0)))
    {
        condition.rightMode = "Const";
        m_dirty = true;
    }

    ImGui::Indent();
    if (condition.rightMode == "Pin")
        RenderPinSelector(condition.rightPin, availablePins, "##rightpin");
    else if (condition.rightMode == "Variable")
        RenderVariableSelector(condition.rightVariable, allVars,
                               condition.compareType, "##rightvar");
    else
        RenderConstValueInput(condition.rightConstValue,
                              condition.compareType, "##rightconst");
    ImGui::Unindent();

    // -- TYPE HINT --
    ImGui::Text("Type:");
    ImGui::SameLine();
    const char* types[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
    const VariableType typeValues[] = {
        VariableType::None, VariableType::Bool, VariableType::Int,
        VariableType::Float, VariableType::String, VariableType::Vector
    };
    int typeIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (condition.compareType == typeValues[i])
        {
            typeIdx = i;
            break;
        }
    }
    ImGui::SetNextItemWidth(80.0f);
    if (ImGui::Combo("##cmptype", &typeIdx, types, 6))
    {
        condition.compareType = typeValues[typeIdx];
        m_dirty = true;
    }

    // -- PREVIEW --
    const std::string preview = BuildConditionPreview(condition);
    ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f),
                       "Preview: %s", preview.c_str());

    ImGui::PopID();
}

// ----------------------------------------------------------------------------

void VisualScriptEditorPanel::RenderVariableSelector(
    std::string& selectedVar,
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType,
    const char* label)
{
    // Filter by type (if a type is specified)
    std::vector<std::string> names;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (expectedType == VariableType::None || allVars[i].Type == expectedType)
        {
            if (!allVars[i].Key.empty())
                names.push_back(allVars[i].Key);
        }
    }

    if (names.empty())
    {
        ImGui::TextDisabled("(no variables)");
        return;
    }

    // BUG-029 Fix: auto-initialise to the first available variable when the
    // selection is empty (e.g. right after switching to Variable mode).
    // Without this the combo visually shows the first item but selectedVar
    // remains "" so BuildConditionPreview displays "[Var: ?]".
    if (selectedVar.empty())
    {
        selectedVar = names[0];
        m_dirty = true;
    }

    int selected = 0;
    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        if (names[static_cast<size_t>(i)] == selectedVar)
        {
            selected = i;
            break;
        }
    }

    std::vector<const char*> cstrs;
    cstrs.reserve(names.size());
    for (size_t i = 0; i < names.size(); ++i)
        cstrs.push_back(names[i].c_str());

    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::Combo(label, &selected, cstrs.data(), static_cast<int>(cstrs.size())))
    {
        selectedVar = names[static_cast<size_t>(selected)];
        m_dirty = true;
    }
}

// ----------------------------------------------------------------------------

void VisualScriptEditorPanel::RenderConstValueInput(
    TaskValue& value,
    VariableType varType,
    const char* label)
{
    // BUG-029 Fix: auto-initialise to a typed default when value is None and
    // a type is known.  Without this the preview always shows "[Const: ?]"
    // until the user explicitly edits the field, because BuildConditionPreview
    // only formats the value when !IsNone().
    if (value.IsNone() && varType != VariableType::None)
    {
        switch (varType)
        {
            case VariableType::Bool:   value = TaskValue(false);                      break;
            case VariableType::Int:    value = TaskValue(0);                          break;
            case VariableType::Float:  value = TaskValue(0.0f);                       break;
            case VariableType::String: value = TaskValue(std::string(""));            break;
            case VariableType::Vector: value = TaskValue(::Vector{0.f, 0.f, 0.f});   break;
            default: break;
        }
        if (!value.IsNone())
            m_dirty = true;
    }

    switch (varType)
    {
        case VariableType::Bool:
        {
            bool bVal = value.IsNone() ? false : value.AsBool();
            if (ImGui::Checkbox(label, &bVal))
            {
                value = TaskValue(bVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::Int:
        {
            int iVal = value.IsNone() ? 0 : value.AsInt();
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputInt(label, &iVal))
            {
                value = TaskValue(iVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::Float:
        {
            float fVal = value.IsNone() ? 0.0f : value.AsFloat();
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::InputFloat(label, &fVal, 0.0f, 0.0f, "%.3f"))
            {
                value = TaskValue(fVal);
                m_dirty = true;
            }
            break;
        }
        case VariableType::String:
        {
            const std::string sVal = value.IsNone() ? "" : value.AsString();
            char sBuf[256];
            strncpy_s(sBuf, sizeof(sBuf), sVal.c_str(), _TRUNCATE);
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputText(label, sBuf, sizeof(sBuf)))
            {
                value = TaskValue(std::string(sBuf));
                m_dirty = true;
            }
            break;
        }
        case VariableType::Vector:
        {
            ::Vector vVal = value.IsNone() ? ::Vector{0.f, 0.f, 0.f} : value.AsVector();
            float v[3] = { vVal.x, vVal.y, vVal.z };
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputFloat3(label, v))
            {
                value = TaskValue(::Vector{ v[0], v[1], v[2] });
                m_dirty = true;
            }
            break;
        }
        default:
        {
            // No type set yet — show a hint
            ImGui::TextDisabled("(set Type first)");
            break;
        }
    }
}

// ----------------------------------------------------------------------------

void VisualScriptEditorPanel::RenderPinSelector(
    std::string& selectedPin,
    const std::vector<std::string>& availablePins,
    const char* label)
{
    if (availablePins.empty())
    {
        ImGui::TextDisabled("(no data-output pins in graph)");
        return;
    }

    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo(label, selectedPin.empty() ? "(select pin)" : selectedPin.c_str()))
    {
        for (size_t i = 0; i < availablePins.size(); ++i)
        {
            const bool isSelected = (selectedPin == availablePins[i]);
            if (ImGui::Selectable(availablePins[i].c_str(), isSelected))
                selectedPin = availablePins[i];
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

// ----------------------------------------------------------------------------

/*static*/
std::string VisualScriptEditorPanel::BuildConditionPreview(const Condition& cond)
{
    auto descSide = [](const std::string& mode,
                       const std::string& pin,
                       const std::string& var,
                       const TaskValue& constValue) -> std::string
    {
        if (mode == "Pin")
            return "[Pin: " + (pin.empty() ? "?" : pin) + "]";
        if (mode == "Variable")
            return "[Var: " + (var.empty() ? "?" : var) + "]";

        // Const — try to format value
        if (!constValue.IsNone())
        {
            std::ostringstream oss;
            switch (constValue.GetType())
            {
                case VariableType::Bool:   oss << (constValue.AsBool() ? "true" : "false"); break;
                case VariableType::Int:    oss << constValue.AsInt();   break;
                case VariableType::Float:  oss << constValue.AsFloat(); break;
                case VariableType::String: oss << '"' << constValue.AsString() << '"'; break;
                case VariableType::Vector:
                {
                    const ::Vector v = constValue.AsVector();
                    oss << "(" << v.x << "," << v.y << "," << v.z << ")";
                    break;
                }
                default: oss << "?"; break;
            }
            return "[Const: " + oss.str() + "]";
        }
        return "[Const: ?]";
    };

    const std::string left  = descSide(cond.leftMode,  cond.leftPin,  cond.leftVariable,  cond.leftConstValue);
    const std::string right = descSide(cond.rightMode, cond.rightPin, cond.rightVariable, cond.rightConstValue);
    const std::string op    = cond.operatorStr.empty() ? "?" : cond.operatorStr;

    return left + " " + op + " " + right;
}

// ============================================================================
// PHASE 24 Panel Integration — Part A: Node Properties
// ============================================================================

void VisualScriptEditorPanel::RenderNodePropertiesPanel()
{
    ImGui::TextDisabled("Node Properties");

    if (m_selectedNodeID < 0)
    {
        ImGui::TextDisabled("(select a node)");
        return;
    }

    // Find the editor node
    VSEditorNode* eNode = nullptr;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        if (m_editorNodes[i].nodeID == m_selectedNodeID)
        {
            eNode = &m_editorNodes[i];
            break;
        }
    }
    if (eNode == nullptr)
        return;

    TaskNodeDefinition& def = eNode->def;

    // ---- ALL node types: standard fields ----
    {
        // Node Name
        char nameBuf[128];
        strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
        if (ImGui::InputText("Name##nodeprops_name", nameBuf, sizeof(nameBuf)))
        {
            def.NodeName = nameBuf;
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].NodeName = def.NodeName;
                    break;
                }
            }
            m_dirty = true;
        }

        ImGui::Separator();
    }

    // ---- Type-specific fields (for non-Branch nodes) ----
    // For Branch nodes, the specialized renderer is handled separately
    if (def.Type != TaskNodeType::Branch)
    {
        // Call RenderProperties() which already handles all type-specific fields
        // BUT we need to inline it here to avoid infinite recursion / double-rendering
        // So instead, render just the critical type-specific parts:

        switch (def.Type)
        {
            case TaskNodeType::AtomicTask:
            {
                const std::vector<TaskSpec> tasks = AtomicTaskUIRegistry::Get().GetSortedForUI();
                const std::string& currentTask = def.AtomicTaskID;
                const char* previewLabel = currentTask.empty() ? "(select task...)" : currentTask.c_str();

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("Task##nodeprops_task", previewLabel))
                {
                    for (const auto& spec : tasks)
                    {
                        bool selected = (spec.id == currentTask);
                        if (ImGui::Selectable(spec.displayName.c_str(), selected))
                        {
                            def.AtomicTaskID = spec.id;
                            // Auto-fill node name with the action's display name
                            def.NodeName = spec.displayName;
                            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                            {
                                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                {
                                    m_template.Nodes[i].AtomicTaskID = def.AtomicTaskID;
                                    m_template.Nodes[i].NodeName = def.NodeName;
                                    break;
                                }
                            }
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Display task parameters
                if (!currentTask.empty())
                {
                    const TaskSpec* taskSpec = AtomicTaskUIRegistry::Get().GetTaskSpec(currentTask);
                    if (taskSpec && !taskSpec->parameters.empty())
                    {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Parameters:");

                        for (const auto& param : taskSpec->parameters)
                        {
                            ImGui::PushID(param.name.c_str());

                            // Build label: parameter name + type hint
                            std::string label = param.name + " (" + param.type + ")";

                            // Get current value from def.Parameters if it exists
                            std::string currentValue = param.defaultValue;
                            auto paramIt = def.Parameters.find(param.name);
                            if (paramIt != def.Parameters.end() && paramIt->second.Type == ParameterBindingType::Literal)
                            {
                                currentValue = paramIt->second.LiteralValue.AsString();
                            }

                            // Display parameter name with description as label
                            ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "%s", param.name.c_str());
                            ImGui::SameLine();

                            // Add help icon (?) next to parameter name for discoverability
                            ImGui::TextDisabled("(?)");

                            // Add tooltip with description if available (on parameter name or help icon)
                            if (ImGui::IsItemHovered() && !param.description.empty())
                            {
                                ImGui::BeginTooltip();
                                ImGui::TextWrapped("%s", param.description.c_str());
                                ImGui::Separator();
                                ImGui::TextDisabled("Type: %s", param.type.c_str());
                                ImGui::TextDisabled("Default: %s", param.defaultValue.c_str());
                                ImGui::EndTooltip();
                            }

                            // Add description text below the parameter name (smaller, grayed out) for immediate clarity
                            if (!param.description.empty())
                            {
                                ImGui::TextDisabled("%s", param.description.c_str());
                            }

                            if (param.type == "Bool")
                            {
                                bool value = (currentValue == "true" || currentValue == "1");
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::Checkbox(("##" + param.name + "_input").c_str(), &value))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "Int")
                            {
                                int value = 0;
                                try { value = std::stoi(currentValue); } catch (...) {}
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputInt(("##" + param.name + "_input").c_str(), &value))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "Float")
                            {
                                float value = 0.0f;
                                try { value = std::stof(currentValue); } catch (...) {}
                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputFloat(("##" + param.name + "_input").c_str(), &value, 0.1f))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(value);
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }
                            else if (param.type == "String")
                            {
                                static char buffer[512] = {0};
                                strncpy_s(buffer, currentValue.c_str(), sizeof(buffer) - 1);
                                buffer[sizeof(buffer) - 1] = '\0';

                                ImGui::SetNextItemWidth(-1.0f);
                                if (ImGui::InputText(("##" + param.name + "_input").c_str(), buffer, sizeof(buffer)))
                                {
                                    ParameterBinding binding;
                                    binding.Type = ParameterBindingType::Literal;
                                    binding.LiteralValue = TaskValue(std::string(buffer));
                                    def.Parameters[param.name] = binding;

                                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                                    {
                                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                        {
                                            m_template.Nodes[i].Parameters[param.name] = binding;
                                            break;
                                        }
                                    }
                                    m_dirty = true;
                                }
                            }

                            ImGui::Spacing();
                            ImGui::PopID();
                        }
                    }
                }
                break;
            }

            case TaskNodeType::Delay:
            {
                float delay = def.DelaySeconds;
                if (ImGui::InputFloat("Delay (s)##nodeprops_delay", &delay, 0.1f, 1.0f))
                {
                    def.DelaySeconds = delay;
                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].DelaySeconds = def.DelaySeconds;
                            break;
                        }
                    }
                    m_dirty = true;
                }
                break;
            }

            case TaskNodeType::Switch:
            {
                ImGui::TextDisabled("Switch node - edit via modal");
                if (ImGui::Button("Edit Switch Cases"))
                {
                    // Open switch case editor if available
                }
                break;
            }

            case TaskNodeType::GetBBValue:
            case TaskNodeType::SetBBValue:
            {
                const char* nodeType = (def.Type == TaskNodeType::GetBBValue) ? "Get" : "Set";
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s Blackboard Value", nodeType);
                ImGui::Separator();

                // BBKey dropdown selector from local blackboard variables
                BBVariableRegistry bbReg;
                bbReg.LoadFromTemplate(m_template);
                const std::vector<VarSpec> allVars = bbReg.GetAllVariables();

                const char* previewLabel = def.BBKey.empty() ? "(select variable...)" : def.BBKey.c_str();

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::BeginCombo("Blackboard Variable##bbkey_combo", previewLabel))
                {
                    for (const auto& var : allVars)
                    {
                        bool selected = (var.name == def.BBKey);
                        if (ImGui::Selectable(var.displayLabel.c_str(), selected))
                        {
                            const std::string oldBBKey = def.BBKey;
                            def.BBKey = var.name;

                            // Sync to template
                            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                            {
                                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                                {
                                    m_template.Nodes[i].BBKey = def.BBKey;
                                    break;
                                }
                            }

                            // Push undo command if changed
                            if (def.BBKey != oldBBKey)
                            {
                                m_undoStack.PushCommand(
                                    std::unique_ptr<ICommand>(new EditNodePropertyCommand(
                                        m_selectedNodeID, "BBKey",
                                        PropertyValue::FromString(oldBBKey),
                                        PropertyValue::FromString(def.BBKey))),
                                    m_template);
                            }
                            m_dirty = true;
                        }
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                // Display node parameters (common for data nodes)
                if (!def.Parameters.empty())
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Parameters:");
                    RenderNodeDataParameters(def);
                }

                break;
            }

            case TaskNodeType::MathOp:
            {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Math Operation");
                ImGui::Separator();

                // MathOpRef operator editor
                static const char* operators[] = { "+", "-", "*", "/", "%", "^" };
                static int operatorIdx = 0;

                if (!def.mathOpRef.mathOperator.empty())
                {
                    for (int i = 0; i < 6; ++i)
                    {
                        if (def.mathOpRef.mathOperator == operators[i])
                        {
                            operatorIdx = i;
                            break;
                        }
                    }
                }

                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::Combo("Operator##mathop", &operatorIdx, operators, 6))
                {
                    def.mathOpRef.mathOperator = operators[operatorIdx];
                    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
                    {
                        if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                        {
                            m_template.Nodes[i].mathOpRef.mathOperator = operators[operatorIdx];
                            break;
                        }
                    }
                    m_dirty = true;
                }

                // Display operation preview with actual operand values
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Operation:");
                ImGui::SameLine();

                // Build left operand display string
                std::string leftStr = "A";
                if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Const)
                    leftStr = def.mathOpRef.leftOperand.constValue;
                else if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Variable)
                    leftStr = "[" + def.mathOpRef.leftOperand.variableName + "]";
                else if (def.mathOpRef.leftOperand.mode == MathOpOperand::Mode::Pin)
                    leftStr = "[Pin]";

                // Build right operand display string
                std::string rightStr = "B";
                if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Const)
                    rightStr = def.mathOpRef.rightOperand.constValue;
                else if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Variable)
                    rightStr = "[" + def.mathOpRef.rightOperand.variableName + "]";
                else if (def.mathOpRef.rightOperand.mode == MathOpOperand::Mode::Pin)
                    rightStr = "[Pin]";

                // Display final operation string in bright green
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), 
                                   "%s %s %s", 
                                   leftStr.c_str(),
                                   def.mathOpRef.mathOperator.empty() ? "?" : def.mathOpRef.mathOperator.c_str(),
                                   rightStr.c_str());

                // Display node parameters
                if (!def.Parameters.empty())
                {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Custom Parameters:");
                    RenderNodeDataParameters(def);
                }

                break;
            }

            case TaskNodeType::SubGraph:
            {
                ImGui::TextDisabled("SubGraph");
                ImGui::TextDisabled("Path: %s", def.SubGraphPath.c_str());
                break;
            }

            case TaskNodeType::Sequence:
            case TaskNodeType::Selector:
            case TaskNodeType::Parallel:
            {
                ImGui::TextDisabled("Control flow node");
                break;
            }

            default:
                ImGui::TextDisabled("(type-specific properties)");
                break;
        }

        ImGui::Separator();
    }

    // ---- Branch-specific: Conditions panel ----
    if (def.Type == TaskNodeType::Branch)
    {
        // Update condition panel with current node's data
        if (m_condPanelNodeID != m_selectedNodeID)
        {
            m_conditionsPanel->SetConditionRefs(def.conditionRefs);
            m_conditionsPanel->SetConditionOperandRefs(def.conditionOperandRefs);
            m_conditionsPanel->SetDynamicPins(def.dynamicPins);
            m_conditionsPanel->SetNodeName(def.NodeName);
            m_condPanelNodeID = m_selectedNodeID;
        }

        // Render the conditions panel
        m_conditionsPanel->Render();

        // Check if dirty and sync back to node
        if (m_conditionsPanel->IsDirty())
        {
            def.conditionRefs = m_conditionsPanel->GetConditionRefs();
            def.conditionOperandRefs = m_conditionsPanel->GetConditionOperandRefs();
            m_conditionsPanel->ClearDirty();
            m_dirty = true;

            // Sync to template
            for (size_t i = 0; i < m_template.Nodes.size(); ++i)
            {
                if (m_template.Nodes[i].NodeID == m_selectedNodeID)
                {
                    m_template.Nodes[i].conditionRefs = def.conditionRefs;
                    m_template.Nodes[i].conditionOperandRefs = def.conditionOperandRefs;
                    break;
                }
            }
        }

        ImGui::Separator();
    }

    // ---- ALL nodes: Breakpoint ----
    bool hasBP = DebugController::Get().HasBreakpoint(0, m_selectedNodeID);
    if (ImGui::Checkbox("Breakpoint (F9)##nodeprops_bp", &hasBP))
    {
        DebugController::Get().ToggleBreakpoint(0, m_selectedNodeID,
                                                m_template.Name,
                                                def.NodeName);
    }
}

// ============================================================================
// PHASE 24 Panel Integration — Part B: Preset Bank
// ============================================================================

void VisualScriptEditorPanel::RenderPresetBankPanel()
{
    ImGui::TextDisabled("Preset Bank (Global)");
    ImGui::Separator();

    if (!m_libraryPanel)
        return;

    size_t presetCount = m_presetRegistry.GetPresetCount();

    // Toolbar: Add preset button
    if (ImGui::Button("+##addpreset", ImVec2(25, 0)))
    {
        m_libraryPanel->OnAddPresetClicked();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("New Preset");

    ImGui::Separator();
    ImGui::TextDisabled("Total: %zu preset(s)", presetCount);
    ImGui::Separator();

    // List all presets in compact horizontal format
    std::vector<ConditionPreset> allPresets = m_presetRegistry.GetFilteredPresets("");

    if (allPresets.empty())
    {
        ImGui::TextDisabled("(no presets - create one to get started)");
    }

    for (size_t i = 0; i < allPresets.size(); ++i)
    {
        const ConditionPreset& preset = allPresets[i];
        ImGui::PushID(preset.id.c_str());
        RenderPresetItemCompact(preset, i + 1);  // 1-indexed for display
        ImGui::PopID();
    }
}

void VisualScriptEditorPanel::RenderPresetItemCompact(const ConditionPreset& preset, size_t index)
{
#ifndef OLYMPE_HEADLESS
    // Single-line horizontal layout matching mockup:
    // [Index: Name (yellow)] [Left▼ mode] [value] [Op▼] [Right▼ mode] [value] [Edit] [Dup] [X]

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

    // Get a mutable copy of the preset for editing
    ConditionPreset editablePreset = preset;
    bool presetModified = false;

    // Condition name display with index (yellow)
    // Use PushID for unique identification, don't add UUID to visible text
    ImGui::PushID(editablePreset.id.c_str());
    ImGui::TextColored(ImVec4(1.0f, 0.843f, 0.0f, 1.0f), "Condition #%zu", index);
    ImGui::PopID();
    ImGui::SameLine(0.0f, 12.0f);

    // Left operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.left, "##left_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Operator dropdown
    std::string opStr;
    switch (editablePreset.op)
    {
        case ComparisonOp::Equal:       opStr = "=="; break;
        case ComparisonOp::NotEqual:    opStr = "!="; break;
        case ComparisonOp::Less:        opStr = "<"; break;
        case ComparisonOp::LessEqual:   opStr = "<="; break;
        case ComparisonOp::Greater:     opStr = ">"; break;
        case ComparisonOp::GreaterEqual: opStr = ">="; break;
        default: opStr = "?"; break;
    }

    const char* opNames[] = { "==", "!=", "<", "<=", ">", ">=" };
    const ComparisonOp opValues[] = {
        ComparisonOp::Equal, ComparisonOp::NotEqual,
        ComparisonOp::Less, ComparisonOp::LessEqual,
        ComparisonOp::Greater, ComparisonOp::GreaterEqual
    };
    int curOpIdx = 0;
    for (int i = 0; i < 6; ++i)
    {
        if (editablePreset.op == opValues[i])
        {
            curOpIdx = i;
            break;
        }
    }

    ImGui::SetNextItemWidth(50.0f);
    if (ImGui::Combo("##op_type", &curOpIdx, opNames, 6))
    {
        editablePreset.op = opValues[curOpIdx];
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 6.0f);

    // Right operand with unified dropdown (mode + value combined)
    if (RenderOperandEditor(editablePreset.right, "##right_op"))
    {
        presetModified = true;
    }
    ImGui::SameLine(0.0f, 12.0f);

    // Save modified preset if changed
    if (presetModified)
    {
        m_presetRegistry.UpdatePreset(editablePreset.id, editablePreset);

        // Phase 24 — Sync to template presets for graph serialization
        // Update the preset in m_template.Presets so it gets saved with the graph
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets[pi] = editablePreset;
                break;
            }
        }

        m_dirty = true;
    }

    // Duplicate button
    if (ImGui::Button("Dup##dup_preset", ImVec2(40, 0)))
    {
        std::string newPresetID = m_presetRegistry.DuplicatePreset(editablePreset.id);

        // Phase 24 — Add the duplicate to template presets as well
        if (!newPresetID.empty())
        {
            const ConditionPreset* newPreset = m_presetRegistry.GetPreset(newPresetID);
            if (newPreset)
            {
                m_template.Presets.push_back(*newPreset);
            }
        }

        m_dirty = true;
    }
    ImGui::SameLine(0.0f, 4.0f);

    // Delete button
    if (ImGui::Button("X##del_preset", ImVec2(25, 0)))
    {
        m_presetRegistry.DeletePreset(editablePreset.id);
        m_pinManager->InvalidatePreset(editablePreset.id);

        // Phase 24 — Remove from template presets as well
        for (size_t pi = 0; pi < m_template.Presets.size(); ++pi)
        {
            if (m_template.Presets[pi].id == editablePreset.id)
            {
                m_template.Presets.erase(m_template.Presets.begin() + pi);
                break;
            }
        }
        // Persist the deletion to disk
        m_presetRegistry.Save("Blueprints/Presets/condition_presets.json");
    }

    ImGui::PopStyleColor(3);

    // Add visual separator between presets
    ImGui::Separator();
#endif
}

bool VisualScriptEditorPanel::RenderOperandEditor(Operand& operand, const char* labelSuffix)
{
#ifndef OLYMPE_HEADLESS
    bool modified = false;

    // Build a unified dropdown list with this ORDER:
    // 1. [Pin-in #1], [Pin-in #2], ...
    // 2. [Const] <value>
    // 3. Variables (sorted alphabetically)

    std::vector<std::string> allOptions;
    std::vector<int> optionTypes;  // 0=Variable, 1=Const, 2=Pin
    std::vector<std::string> optionValues;  // Store the actual value for each option

    int currentSelectionIdx = -1;

    // ── Add all available pins FIRST ─────────────────────────────────────
    {
        std::vector<DynamicDataPin> allPins = m_pinManager->GetAllPins();
        for (const auto& pin : allPins)
        {
            allOptions.push_back("[Pin-in] " + pin.label);
            optionTypes.push_back(2);  // Pin
            optionValues.push_back(pin.label);

            if (operand.mode == OperandMode::Pin && 
                operand.stringValue == pin.label)
            {
                currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
            }
        }

        // If no pins are available, still show the [Pin-in] option as a category
        if (allPins.empty())
        {
            allOptions.push_back("[Pin-in] (none available)");
            optionTypes.push_back(2);  // Pin
            optionValues.push_back("");  // Empty value for unavailable pin
        }
    }

    // ── Add [Const] option SECOND ────────────────────────────────────────
    {
        std::string constLabel = "[Const] ";
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << operand.constValue;
        std::string constVal = oss.str();
        // Trim trailing zeros
        size_t dot = constVal.find('.');
        if (dot != std::string::npos)
        {
            size_t last = constVal.find_last_not_of('0');
            if (last != std::string::npos && last > dot)
                constVal = constVal.substr(0, last + 1);
            else if (last == dot)
                constVal = constVal.substr(0, dot);
        }
        constLabel += constVal;

        allOptions.push_back(constLabel);
        optionTypes.push_back(1);  // Const
        optionValues.push_back(constVal);

        if (operand.mode == OperandMode::Const)
        {
            currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
        }
    }

    // ── Add all local variables LAST (sorted alphabetically) ──────────────
    {
        std::vector<std::string> sortedVarNames;
        for (const auto& entry : m_template.Blackboard)
        {
            if (entry.Type != VariableType::None && !entry.Key.empty())
            {
                sortedVarNames.push_back(entry.Key);
            }
        }
        // Sort alphabetically
        std::sort(sortedVarNames.begin(), sortedVarNames.end());

        for (const auto& varName : sortedVarNames)
        {
            allOptions.push_back(varName);
            optionTypes.push_back(0);  // Variable
            optionValues.push_back(varName);

            // Check if this is the currently selected variable
            if (operand.mode == OperandMode::Variable && 
                operand.stringValue == varName)
            {
                currentSelectionIdx = static_cast<int>(allOptions.size() - 1);
            }
        }
    }

    // ── Render unified dropdown ──────────────────────────────────────────
    ImGui::SetNextItemWidth(120.0f);

    const char* displayText = (currentSelectionIdx >= 0) ? allOptions[currentSelectionIdx].c_str() : "(none)";

    if (ImGui::BeginCombo(labelSuffix, displayText))
    {
        // Create mutable array of C strings for ImGui
        std::vector<const char*> optionsCStr;
        for (const auto& opt : allOptions)
            optionsCStr.push_back(opt.c_str());

        for (int i = 0; i < static_cast<int>(allOptions.size()); ++i)
        {
            bool selected = (i == currentSelectionIdx);
            if (ImGui::Selectable(optionsCStr[i], selected))
            {
                // Update operand based on selected type
                switch (optionTypes[i])
                {
                    case 0:  // Variable
                        operand.mode = OperandMode::Variable;
                        operand.stringValue = optionValues[i];
                        break;
                    case 1:  // Const
                        operand.mode = OperandMode::Const;
                        try {
                            operand.constValue = std::stod(optionValues[i]);
                        } catch (...) {
                            operand.constValue = 0.0;
                        }
                        break;
                    case 2:  // Pin
                        operand.mode = OperandMode::Pin;
                        // For pins, store the pin label. If no specific pin selected (empty),
                        // use a placeholder value that indicates "any available pin"
                        operand.stringValue = optionValues[i].empty() ? "[Pin-in]" : optionValues[i];
                        break;
                }
                modified = true;
            }
        }
        ImGui::EndCombo();
    }

    // ── Add numeric input field for Const mode ──────────────────────────────
    if (operand.mode == OperandMode::Const)
    {
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::SetNextItemWidth(60.0f);
        if (ImGui::InputDouble("##const_value", &operand.constValue, 0.0, 0.0, "%.3f"))
        {
            modified = true;
        }
    }

    return modified;
#else
    return false;
#endif
}

// ============================================================================
// PHASE 24 Panel Integration — Part C: Local Variables Reference
// ============================================================================

void VisualScriptEditorPanel::RenderLocalVariablesPanel()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    // BUG-001 Hotfix: warn user if invalid entries exist (key empty or type None)
    // to prevent save crash caused by unhandled None type during serialization.
    bool hasInvalid = false;
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(i)];
        if (entry.Key.empty() || entry.Type == VariableType::None)
        {
            hasInvalid = true;
            break;
        }
    }
    if (hasInvalid)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted("[!] Invalid entries will be skipped on save");
        ImGui::PopStyleColor();
    }

    // Add entry button — BUG-001 Hotfix: init with safe defaults (non-empty key, Int type)
    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "NewVariable";
        entry.Type     = VariableType::Int;
        entry.Default  = GetDefaultValueForType(VariableType::Int);  // UX Fix #1
        entry.IsGlobal = false;
        m_template.Blackboard.push_back(entry);
        m_dirty = true;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add key");

    // List existing entries
    for (int idx = static_cast<int>(m_template.Blackboard.size()) - 1; idx >= 0; --idx)
    {
        BlackboardEntry& entry = m_template.Blackboard[static_cast<size_t>(idx)];

        ImGui::PushID(idx);

        // Use a local (non-static) buffer per iteration to avoid sharing across entries
        char keyBuf[64];
        strncpy_s(keyBuf, sizeof(keyBuf), entry.Key.c_str(), _TRUNCATE);

        // ── Name (editable text field) ──
        ImGui::SetNextItemWidth(140.0f);
        if (ImGui::InputText("##bbkey", keyBuf, sizeof(keyBuf)))
        {
            entry.Key = keyBuf;
            m_pendingBlackboardEdits[idx] = keyBuf;
            m_dirty = true;
        }

        ImGui::SameLine();

        // ── Type dropdown ──
        const char* typeNames[] = { "None", "Bool", "Int", "Float", "String", "Vector" };
        const VariableType typeValues[] = {
            VariableType::None, VariableType::Bool, VariableType::Int,
            VariableType::Float, VariableType::String, VariableType::Vector
        };
        int curTypeIdx = 0;
        for (int ti = 0; ti < 6; ++ti)
            if (entry.Type == typeValues[ti])
            { curTypeIdx = ti; break; }

        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &curTypeIdx, typeNames, 6))
        {
            entry.Type = typeValues[curTypeIdx];
            entry.Default = GetDefaultValueForType(entry.Type);
            m_dirty = true;
        }

        // ── Default value (type-aware editor) ──
        if (entry.Type != VariableType::None)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("Default:");
            ImGui::SameLine();
            RenderConstValueInput(entry.Default, entry.Type, "##bbdefault");
        }

        // ── Global toggle ──
        ImGui::SameLine();
        bool isGlobal = entry.IsGlobal;
        if (ImGui::Checkbox("G##bbglobal", &isGlobal))
        {
            entry.IsGlobal = isGlobal;
            m_dirty = true;
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Mark as global variable");

        // ── Delete button ──
        ImGui::SameLine();
        if (ImGui::Button("X##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_pendingBlackboardEdits.erase(idx);
            m_dirty = true;
        }

        ImGui::PopID();
    }
}

// ============================================================================
// Phase 24 Global Blackboard Integration — RenderGlobalVariablesPanel (Enhanced)
// ============================================================================

void VisualScriptEditorPanel::RenderGlobalVariablesPanel()
{
    ImGui::TextDisabled("Global Variables (Editor Instance)");
    ImGui::Separator();

    // Get reference to the global template registry (non-const for Add)
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    const std::vector<GlobalEntryDefinition>& globalVars = gtb.GetAllVariables();

    // Add Global Variable button
    if (ImGui::Button("+##globalVarAdd", ImVec2(30, 0)))
    {
        ImGui::OpenPopup("AddGlobalVariablePopup");
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Add global variable");

    // Add Global Variable Modal Dialog
    if (ImGui::BeginPopupModal("AddGlobalVariablePopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char newVarName[128] = "newGlobal";
        static int newVarTypeIdx = 2;  // Default to Int
        static char newVarDescription[256] = "Enter description...";

        ImGui::InputText("Variable Name##new", newVarName, sizeof(newVarName));

        const char* typeOptions[] = { "Bool", "Int", "Float", "String", "Vector", "EntityID" };
        const VariableType typeValues[] = {
            VariableType::Bool, VariableType::Int, VariableType::Float,
            VariableType::String, VariableType::Vector, VariableType::EntityID
        };
        ImGui::Combo("Type##new", &newVarTypeIdx, typeOptions, 6);

        ImGui::InputTextMultiline("Description##new", newVarDescription, sizeof(newVarDescription), ImVec2(0, 60));

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(newVarName) > 0 && !gtb.HasVariable(newVarName))
            {
                TaskValue defaultVal = GetDefaultValueForType(typeValues[newVarTypeIdx]);
                if (gtb.AddVariable(newVarName, typeValues[newVarTypeIdx], defaultVal, newVarDescription, false))
                {
                    SYSTEM_LOG << "[VSEditor] Created new global variable: " << newVarName << "\n";
                    gtb.SaveToFile();  // Use last loaded path automatically

                    // Phase 24: Hot reload to refresh registry and propagate to all panels
                    GlobalTemplateBlackboard::Reload();

                    m_dirty = true;

                    // Reset form
                    memset(newVarName, 0, sizeof(newVarName));
                    strcpy_s(newVarName, sizeof(newVarName), "newGlobal");
                    newVarTypeIdx = 2;
                    memset(newVarDescription, 0, sizeof(newVarDescription));
                    strcpy_s(newVarDescription, sizeof(newVarDescription), "Enter description...");

                    ImGui::CloseCurrentPopup();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::Separator();

    if (globalVars.empty())
    {
        ImGui::TextDisabled("(no global variables defined)");
        ImGui::TextDisabled("Click [+] above to create new global variables");
        return;
    }

    ImGui::TextDisabled("Global variables from project registry");
    ImGui::TextDisabled("Values shown are editor-specific (persisted with graph)");
    ImGui::Separator();

    // Check if EntityBlackboard is initialized
    if (!m_entityBlackboard)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "[ERROR] EntityBlackboard not initialized");
        return;
    }

    // Display each global variable with editable entity-specific value
    for (size_t gi = 0; gi < globalVars.size(); ++gi)
    {
        const GlobalEntryDefinition& globalDef = globalVars[gi];

        ImGui::PushID(static_cast<int>(gi));

        // ---- Variable name (read-only) with type label + Delete button ----
        ImGui::TextColored(ImVec4(0.8f, 0.95f, 1.0f, 1.0f), "(%s) %s",
                          VariableTypeToString(globalDef.Type).c_str(),
                          globalDef.Key.c_str());

        ImGui::SameLine();
        ImGui::TextDisabled("(%.1f KB)", 0.1f);  // Placeholder space
        ImGui::SameLine();

        // Delete button for global variable
        if (ImGui::SmallButton("Delete##globalvar"))
        {
            // Mark for deletion (we'll process after the loop to avoid iterator invalidation)
            std::string varToDelete = globalDef.Key;
            if (gtb.RemoveVariable(varToDelete))
            {
                SYSTEM_LOG << "[VSEditor] Deleted global variable: " << varToDelete << "\n";
                gtb.SaveToFile();  // Use last loaded path automatically

                // Phase 24: Hot reload to refresh registry
                GlobalTemplateBlackboard::Reload();

                m_dirty = true;
            }
            ImGui::PopID();
            continue;  // Skip rendering the rest of this variable's UI
        }

        // ---- Description (if available) ----
        if (!globalDef.Description.empty())
        {
            ImGui::TextDisabled("  %s", globalDef.Description.c_str());
        }

        // Create unique table ID per global variable to avoid ImGui::BeginTable() failures
        std::string tableId = "##GlobalVarTable_" + std::to_string(gi);
        if (ImGui::BeginTable(tableId.c_str(), 2, ImGuiTableFlags_SizingStretchSame, ImVec2(0, 0)))
        {
            ImGui::TableSetupColumn("Label", 0);
            ImGui::TableSetupColumn("Value", 0);

            // ---- Default Value (read-only) ----
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Default:");
            ImGui::TableSetColumnIndex(1);

            const TaskValue& defaultValue = globalDef.DefaultValue;
            std::string defaultStr;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                    defaultStr = defaultValue.IsNone() ? "false" : (defaultValue.AsBool() ? "true" : "false");
                    break;
                case VariableType::Int:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(defaultValue.AsInt());
                    break;
                case VariableType::Float:
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2);
                    oss << (defaultValue.IsNone() ? 0.0f : defaultValue.AsFloat());
                    defaultStr = oss.str();
                    break;
                }
                case VariableType::String:
                    defaultStr = defaultValue.IsNone() ? "" : defaultValue.AsString();
                    break;
                case VariableType::Vector:
                    defaultStr = "(vector)";
                    break;
                case VariableType::EntityID:
                    defaultStr = defaultValue.IsNone() ? "0" : std::to_string(static_cast<int>(defaultValue.AsEntityID()));
                    break;
                default:
                    defaultStr = "(unknown)";
                    break;
            }
            ImGui::TextDisabled("%s", defaultStr.c_str());

            // ---- Current Value (editable with scope resolution) ----
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Current:");
            ImGui::TableSetColumnIndex(1);

            // Use scoped variable access to get/set entity-specific value
            std::string scopedVarName = "(G)" + globalDef.Key;
            TaskValue currentValue = m_entityBlackboard->GetValueScoped(scopedVarName);

            // Create type-specific input widget
            bool valueChanged = false;
            switch (globalDef.Type)
            {
                case VariableType::Bool:
                {
                    bool bVal = currentValue.IsNone() ? false : currentValue.AsBool();
                    if (ImGui::Checkbox("##bool_val", &bVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(bVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Int:
                {
                    int iVal = currentValue.IsNone() ? 0 : currentValue.AsInt();
                    if (ImGui::InputInt("##int_val", &iVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(iVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Float:
                {
                    float fVal = currentValue.IsNone() ? 0.0f : currentValue.AsFloat();
                    if (ImGui::InputFloat("##float_val", &fVal))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(fVal));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::String:
                {
                    static std::unordered_map<size_t, std::vector<char>> stringBuffers;
                    size_t bufKey = gi; // Use index as unique key for buffer storage
                    if (stringBuffers.find(bufKey) == stringBuffers.end())
                    {
                        std::string initialStr = currentValue.IsNone() ? "" : currentValue.AsString();
                        stringBuffers[bufKey] = std::vector<char>(initialStr.begin(), initialStr.end());
                        stringBuffers[bufKey].push_back('\0');
                        stringBuffers[bufKey].resize(256);  // Allocate buffer
                    }

                    ImGui::SetNextItemWidth(-1.0f);
                    if (ImGui::InputText("##string_val", stringBuffers[bufKey].data(), 256, ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        std::string newStr(stringBuffers[bufKey].data());
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newStr));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::Vector:
                {
                    Vector vVal = currentValue.IsNone() ? Vector{0.0f, 0.0f, 0.0f} : currentValue.AsVector();
                    float vArray[3] = {vVal.x, vVal.y, vVal.z};
                    if (ImGui::InputFloat3("##vector_val", vArray))
                    {
                        Vector newVec{vArray[0], vArray[1], vArray[2]};
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(newVec));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                case VariableType::EntityID:
                {
                    int eID = currentValue.IsNone() ? 0 : static_cast<int>(currentValue.AsEntityID());
                    if (ImGui::InputInt("##entityid_val", &eID))
                    {
                        m_entityBlackboard->SetValueScoped(scopedVarName, TaskValue(eID >= 0 ? eID : 0));
                        m_dirty = true;
                        valueChanged = true;
                    }
                    break;
                }
                default:
                    ImGui::TextDisabled("(unsupported type)");
                    break;
            }

            // ---- Persistent flag ----
            if (globalDef.IsPersistent)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextDisabled("Flags:");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.5f, 1.0f), "[Persistent]");
            }

            ImGui::EndTable();
        }
        ImGui::Separator();
        ImGui::PopID();
    }
}

} // namespace Olympe
