// ============================================================================
// VisualScriptEditorPanel_Canvas.cpp
// ImNodes Canvas Rendering and Synchronization
// ============================================================================
//
// This file contains all canvas rendering, node/link synchronization, and
// position management for the ImNodes graph editor.
//
// Methods:
// - RenderCanvas()                     : Main canvas rendering loop
// - SyncCanvasFromTemplate()           : Update canvas from template data
// - SyncTemplateFromCanvas()           : Update template from canvas state
// - SyncEditorNodesFromTemplate()      : Restore node positions
// - SyncNodePositionsFromImNodes()     : Save node positions from ImNodes
// - Utility functions for UID calculation, screen conversion
//
// Integration Points:
// - ImNodes context (m_imnodesContext) : Canvas state management
// - m_editorNodes / m_editorLinks      : Editor-side node/link tracking
// - m_template                         : Graph data
// - VSConnectionValidator              : Link validation
// ============================================================================

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

    // Build connected attribute IDs set from current editor links.
    // Pins whose attribute ID is in this set will be rendered filled;
    // unconnected pins are rendered outlined (empty).
    std::unordered_set<int> connectedAttrIDs;
    for (size_t li = 0; li < m_editorLinks.size(); ++li)
    {
        connectedAttrIDs.insert(m_editorLinks[li].srcAttrID);
        connectedAttrIDs.insert(m_editorLinks[li].dstAttrID);
    }

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

        // Phase 24.2 FIX: Ensure data-pure nodes have DataPins initialized
        // This handles both newly created nodes AND nodes loaded from blueprints

        // Initialize DataPins for GetBBValue (Variable) nodes
        if (eNode.def.Type == TaskNodeType::GetBBValue && eNode.def.DataPins.empty())
        {
            DataPinDefinition pinOut;
            pinOut.PinName = "Value";
            pinOut.Dir     = DataPinDir::Output;
            pinOut.PinType = VariableType::Float;  // Will be resolved at runtime based on actual variable
            eNode.def.DataPins.push_back(pinOut);
            std::cerr << "[VSEditor] Initialized DataPins for GetBBValue (Variable) node #" << eNode.nodeID << "\n";
        }

        // Initialize DataPins for MathOp nodes
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

        // Initialize DataPins for SetBBValue nodes
        if (eNode.def.Type == TaskNodeType::SetBBValue && eNode.def.DataPins.empty())
        {
            DataPinDefinition pinIn;
            pinIn.PinName = "Value";
            pinIn.Dir     = DataPinDir::Input;
            pinIn.PinType = VariableType::Float;  // Will be resolved at runtime based on target variable
            eNode.def.DataPins.push_back(pinIn);
            std::cerr << "[VSEditor] Initialized DataPins for SetBBValue node #" << eNode.nodeID << "\n";
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
            m_branchRenderer->RenderNode(branchData, connectedAttrIDs);
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
                this,
                connectedAttrIDs);
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
                else
                {
                    SYSTEM_LOG << "[VSEditor] Exec link validation failed: node #" << srcNodeID
                               << "." << srcPinName << " -> node #" << dstNodeID << ".In\n";
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


} // namespace Olympe
