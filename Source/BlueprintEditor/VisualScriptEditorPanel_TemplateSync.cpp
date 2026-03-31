/**
 * @file VisualScriptEditorPanel_TemplateSync.cpp
 * @brief Template-canvas synchronization methods for VisualScriptEditorPanel (Phase 6 extraction).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * This file contains 5 template synchronization methods extracted from VisualScriptEditorPanel.cpp
 * for improved code organization and maintainability:
 *
 *   1. SyncCanvasFromTemplate() — Load nodes from template into editor canvas (~70 LOC)
 *   2. SyncTemplateFromCanvas() — Update template with current editor node state (~10 LOC)
 *   3. RebuildLinks() — Rebuild all visual links from template connections (~140 LOC)
 *   4. SyncEditorNodesFromTemplate() — Restore editor nodes during undo/redo (~135 LOC)
 *   5. RemoveLink(int linkID) — Delete a link and push undo command (~95 LOC)
 *
 * Phase 6 Refactoring: Extract template synchronization logic to separate compilation unit
 * for cleaner separation of concerns and faster iteration during graph manipulation development.
 *
 * Key responsibilities:
 *   - Template ↔ Canvas state synchronization
 *   - Position validation and fallback positioning
 *   - Link resolution and reconstruction
 *   - Undo/Redo state management integration
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"
#include "../third_party/imnodes/imnodes.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace Olympe {

// ============================================================================
// Template / Canvas Sync
// ============================================================================

/**
 * @brief SyncCanvasFromTemplate
 *
 * Load all nodes from m_template into the editor canvas representation (m_editorNodes).
 * This is the primary method for initializing the canvas from a loaded/pasted blueprint.
 *
 * Process:
 *   1. Clear existing canvas state (m_editorNodes, m_positionedNodes, m_nextNodeID)
 *   2. For each node in m_template.Nodes:
 *      a. Create a VSEditorNode and copy TaskNodeDefinition
 *      b. Load position from JSON if available; validate against corruption
 *      c. Fall back to auto-layout if position is missing or invalid
 *      d. For Branch nodes: regenerate dynamic pins from conditions
 *   3. Rebuild links from template connections (RebuildLinks)
 *   4. Set m_needsPositionSync flag so RenderCanvas() will push positions to ImNodes
 *
 * Position validation: Checks for finite values and ±100,000 bounds. This prevents
 * crashes from corrupted JSON (e.g., NaN, ±Infinity). Garbage values are logged and
 * replaced with auto-layout positions.
 *
 * Phase 24 integration: Branch nodes' dynamic pins are regenerated from conditionRefs
 * to ensure data-in connectors are available even if not explicitly saved.
 *
 * @note Called by LoadTemplate() and after blueprint paste operations.
 */
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

/**
 * @brief SyncTemplateFromCanvas
 *
 * Update m_template.Nodes with the current state of m_editorNodes.
 * This is called before serialization (Save/SaveAs) to reflect user edits on the canvas.
 *
 * Process:
 *   1. Clear m_template.Nodes
 *   2. Copy all TaskNodeDefinitions from m_editorNodes into m_template
 *   3. Rebuild template's lookup cache for fast node access by ID
 *
 * @note This method does NOT sync positions (that's done separately by
 *       SyncNodePositionsFromImNodes). It only syncs node definitions.
 */
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

/**
 * @brief RebuildLinks
 *
 * Reconstruct all visual links (m_editorLinks) from template connections
 * (m_template.ExecConnections and m_template.DataConnections).
 *
 * Process:
 *   1. Clear m_editorLinks
 *   2. For each ExecPinConnection in m_template:
 *      a. Find source node and resolve pin index from stored name
 *      b. Include dynamic exec-out pins for VSSequence/Switch nodes
 *      c. Create VSEditorLink with encoded attribute IDs
 *   3. For each DataPinConnection in m_template:
 *      a. Find source and destination nodes
 *      b. Resolve pin indices from stored names (try static, then DataPins vector)
 *      c. Handle Phase 24 dynamic pins for Branch nodes
 *      d. Create VSEditorLink with encoded attribute IDs
 *
 * Attribute ID encoding scheme (nodeID * 10000 + offset):
 *   0–99:     exec-in pins (exec input for node)
 *   100–199:  exec-out pins (exec outputs indexed by position)
 *   200–299:  data-in pins (data inputs indexed by position)
 *   300–399:  data-out pins (data outputs indexed by position)
 *
 * This method is resilient to missing nodes and out-of-range pin indices,
 * defaulting to "Value" or "In"/"Out" pin names as fallback.
 *
 * @note Called by SyncCanvasFromTemplate(), SyncEditorNodesFromTemplate(), and
 *       after link creation/deletion operations.
 */
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

/**
 * @brief SyncEditorNodesFromTemplate
 *
 * Restore editor nodes (m_editorNodes) from m_template during undo/redo operations.
 * This is more complex than SyncCanvasFromTemplate because it must:
 *   - Preserve canvas positions for nodes that still exist
 *   - Support MoveNodeCommand parameters (__posX, __posY) for undo/redo states
 *   - Fall back through multiple position sources (parameter -> saved -> loaded -> default)
 *
 * Position fallback order:
 *   1. Parameters["__posX/__posY"] stored by MoveNodeCommand (undo/redo target)
 *   2. Previously known position (from m_editorNodes before clear)
 *   3. File-loaded position (m_template.EditorPosX/Y)
 *   4. Auto-layout default (grid spacing)
 *
 * Process:
 *   1. Save current positions from m_editorNodes (for fallback)
 *   2. Clear m_editorNodes, m_positionedNodes, m_nodeDragStartPositions
 *   3. For each node in m_template, determine position using fallback order
 *   4. Validate all positions against corruption (NaN, ±100,000)
 *   5. Rebuild links to remove "ghost" links from deleted nodes
 *   6. Set m_needsPositionSync for the next render
 *
 * @note Called by PerformUndo() and PerformRedo() to restore graph state.
 *       Also called internally by LoadTemplate() on initial load.
 */
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

/**
 * @brief RemoveLink
 *
 * Delete a link and push a DeleteLinkCommand onto the undo stack.
 * This integrates link deletion with the undo/redo system for reversible operations.
 *
 * Process:
 *   1. Find the VSEditorLink by ID in m_editorLinks
 *   2. Decode attribute IDs to extract node IDs and pin indices
 *   3. Resolve pin names from indices using GetExecOutputPins/GetDataOutputPins
 *   4. Create ExecPinConnection or DataPinConnection based on link type
 *   5. Push DeleteLinkCommand to undo stack
 *   6. Rebuild links to update visual state
 *   7. Mark graph as dirty and clear verification cache
 *
 * Attribute ID decoding:
 *   - Node ID = attrID / 10000
 *   - Pin offset = attrID % 10000
 *   - Pin index = offset - base (0 for exec-in, 100 for exec-out, etc.)
 *
 * @param linkID ImNodes link ID to delete
 *
 * @note This is the primary method for user-initiated link deletion via context menu
 *       or Ctrl+Click. Automatic link removal during node deletion is handled separately
 *       by the command system.
 */
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
        // Decode data-out -> data-in
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
        // Decode exec-out -> exec-in
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

}  // namespace Olympe
