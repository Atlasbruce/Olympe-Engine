/**
 * @file VisualScriptEditorPanel_Synchronizers.cpp
 * @brief Canvas ↔ Template synchronization methods (TIER 1)
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details Synchronization between editor canvas state and template data model.
 * Phase 24 integration: handles condition presets and global blackboard sync.
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"

#include <algorithm>
#include <cmath>

namespace Olympe {

// ============================================================================
// Template / Canvas Sync
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

} // namespace Olympe
