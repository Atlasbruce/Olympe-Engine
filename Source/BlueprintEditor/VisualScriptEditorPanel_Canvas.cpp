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
// - RebuildLinks()                     : Reconstruct link list (moved to Connections)
// - Utility functions for UID calculation, screen conversion
//
// Integration Points:
// - ImNodes context (m_imnodesContext) : Canvas state management
// - m_editorNodes / m_editorLinks      : Editor-side node/link tracking
// - m_template                         : Graph data
// - VSConnectionValidator              : Link validation
// ============================================================================

#include "VisualScriptEditorPanel.h"
#include <algorithm>

namespace Olympe {

// ============================================================================
// Main Canvas Rendering
// ============================================================================

void VisualScriptEditorPanel::RenderCanvas()
{
    // Activate this panel's ImNodes context
    ImNodes::EditorContextSet(m_imnodesContext);

    // Begin ImNodes editor window
    ImNodes::BeginNodeEditor();

    // Render all nodes
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        const VSEditorNode& eNode = m_editorNodes[i];
        const TaskNodeDefinition& def = eNode.def;

        // Render this node (would call RenderNode helper)
        ImNodes::BeginNode(def.NodeID);

        ImGui::TextUnformatted(def.NodeName.c_str());

        // Render input pins
        for (size_t p = 0; p < def.DataPins.size(); ++p)
        {
            if (def.DataPins[p].Dir == DataPinDir::Input)
            {
                int attrID = DataInAttrUID(def.NodeID, static_cast<int>(p));
                ImNodes::BeginInputAttribute(attrID);
                ImGui::TextUnformatted(def.DataPins[p].PinName.c_str());
                ImNodes::EndInputAttribute();
            }
        }

        // Render output pins
        for (size_t p = 0; p < def.DataPins.size(); ++p)
        {
            if (def.DataPins[p].Dir == DataPinDir::Output)
            {
                int attrID = DataOutAttrUID(def.NodeID, static_cast<int>(p));
                ImNodes::BeginOutputAttribute(attrID);
                ImGui::TextUnformatted(def.DataPins[p].PinName.c_str());
                ImNodes::EndOutputAttribute();
            }
        }

        ImNodes::EndNode();

        // Update position if changed
        ImVec2 newPos = ImNodes::GetNodeGridSpacePos(def.NodeID);
        if (newPos.x != eNode.posX || newPos.y != eNode.posY)
        {
            // User dragged node - update state
            for (auto& mNode : m_editorNodes)
            {
                if (mNode.nodeID == def.NodeID)
                {
                    mNode.posX = newPos.x;
                    mNode.posY = newPos.y;
                    break;
                }
            }
        }
    }

    // Render all links
    for (const auto& link : m_editorLinks)
    {
        ImNodes::Link(link.linkID, link.srcAttrID, link.dstAttrID);
    }

    // Handle new link creation
    {
        int startAttr = 0, endAttr = 0;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
        {
            // Decode and create connection
            // (Would call ConnectExec or ConnectData based on UID ranges)
        }
    }

    // Handle link deletion
    {
        int linkID = 0;
        if (ImNodes::IsLinkDestroyed(&linkID))
        {
            // Remove connection
            RemoveLink(linkID);
        }
    }

    ImNodes::EndNodeEditor();

    // Store viewport state if requested
    if (m_needsPositionSync)
    {
        m_needsPositionSync = false;
        for (auto& eNode : m_editorNodes)
        {
            ImNodes::SetNodeGridSpacePos(eNode.nodeID, ImVec2(eNode.posX, eNode.posY));
        }
    }
}

// ============================================================================
// Synchronization: Canvas ↔ Template
// ============================================================================

void VisualScriptEditorPanel::SyncCanvasFromTemplate()
{
    // Recreate editor nodes from template nodes
    SyncEditorNodesFromTemplate();
    // Recreate editor links from template connections
    RebuildLinks();
}

void VisualScriptEditorPanel::SyncTemplateFromCanvas()
{
    // Store node positions to template parameters
    SyncNodePositionsFromImNodes();

    // Update template connections from editor links
    // (Already handled by command pattern during link operations)
}

// ============================================================================
// Node Position Synchronization
// ============================================================================

void VisualScriptEditorPanel::SyncEditorNodesFromTemplate()
{
    // Save current positions before clearing
    std::map<int, std::pair<float, float>> savedPos;
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        float posX = m_editorNodes[i].posX;
        float posY = m_editorNodes[i].posY;

        // Validate saved positions
        if (!std::isfinite(posX) || !std::isfinite(posY) ||
            posX < -100000.0f || posX > 100000.0f ||
            posY < -100000.0f || posY > 100000.0f)
        {
            posX = DEFAULT_NODE_X_OFFSET;
            posY = DEFAULT_NODE_Y;
        }

        savedPos[m_editorNodes[i].nodeID] = std::make_pair(posX, posY);
    }

    m_editorNodes.clear();
    m_positionedNodes.clear();
    m_nodeDragStartPositions.clear();

    // Reconstruct editor nodes from template
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& def = m_template.Nodes[i];

        VSEditorNode eNode;
        eNode.nodeID = def.NodeID;
        eNode.def    = def;

        // Restore position (priority: params > saved > file > defaults)
        auto posXIt = def.Parameters.find("__posX");
        auto posYIt = def.Parameters.find("__posY");
        if (posXIt != def.Parameters.end() && posYIt != def.Parameters.end() &&
            posXIt->second.Type == ParameterBindingType::Literal &&
            posYIt->second.Type == ParameterBindingType::Literal)
        {
            eNode.posX = posXIt->second.LiteralValue.AsFloat();
            eNode.posY = posYIt->second.LiteralValue.AsFloat();
        }
        else if (savedPos.count(def.NodeID))
        {
            eNode.posX = savedPos[def.NodeID].first;
            eNode.posY = savedPos[def.NodeID].second;
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

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

        m_editorNodes.push_back(eNode);
    }

    RebuildLinks();
    m_needsPositionSync = true;
}

void VisualScriptEditorPanel::SyncNodePositionsFromImNodes()
{
    // Save all node positions back to m_template.Nodes
    for (auto& eNode : m_editorNodes)
    {
        // Find corresponding template node
        for (auto& tNode : m_template.Nodes)
        {
            if (tNode.NodeID == eNode.nodeID)
            {
                // Store in Parameters for undo/redo
                ParameterBinding bx, by;
                bx.Type = ParameterBindingType::Literal;
                bx.LiteralValue = TaskValue(eNode.posX);
                by.Type = ParameterBindingType::Literal;
                by.LiteralValue = TaskValue(eNode.posY);
                tNode.Parameters["__posX"] = bx;
                tNode.Parameters["__posY"] = by;

                // Also update EditorPos fields
                tNode.EditorPosX = eNode.posX;
                tNode.EditorPosY = eNode.posY;
                tNode.HasEditorPos = true;
                break;
            }
        }
    }
}

}  // namespace Olympe
