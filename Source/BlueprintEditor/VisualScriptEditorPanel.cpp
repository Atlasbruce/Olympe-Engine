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
#include "../system/system_utils.h"

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

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
        case TaskNodeType::Switch:      return {"Default"};
        case TaskNodeType::AtomicTask:  return {"Completed"};
        case TaskNodeType::GetBBValue:  return {"Out"};
        case TaskNodeType::SetBBValue:  return {"Out"};
        case TaskNodeType::MathOp:      return {"Out"};
        default:                        return {"Out"};
    }
}

std::vector<std::string> VisualScriptEditorPanel::GetDataInputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::SetBBValue:  return {"Value"};
        case TaskNodeType::MathOp:      return {"A", "B"};
        case TaskNodeType::Branch:      return {"Condition"};
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
            eNode.posX = def.EditorPosX;
            eNode.posY = def.EditorPosY;
        }
        else
        {
            eNode.posX = 200.0f * static_cast<float>(i);  // Default auto-layout
            eNode.posY = 100.0f;
        }

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

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
            outPins = GetExecOutputPins(srcNode->Type);

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
        savedPos[m_editorNodes[i].nodeID] =
            std::make_pair(m_editorNodes[i].posX, m_editorNodes[i].posY);
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
            eNode.posX = posXIt->second.LiteralValue.AsFloat();
            eNode.posY = posYIt->second.LiteralValue.AsFloat();
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
                // Position loaded from file (e.g. undo of delete-node)
                eNode.posX = def.EditorPosX;
                eNode.posY = def.EditorPosY;
            }
            else
            {
                // New node (e.g. restored by Redo) – use a default spread position
                eNode.posX = DEFAULT_NODE_X_OFFSET +
                             DEFAULT_NODE_X_SPACING * static_cast<float>(i);
                eNode.posY = DEFAULT_NODE_Y;
            }
        }

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

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

    // NOTE: Do NOT clear the undo stack here.  Each VisualScriptEditorPanel
    // instance owns its own stack (one per tab), so there is no cross-tab
    // contamination.  Preserving the stack lets the user undo edits made
    // before saving and reloading, and is required for undo to function
    // correctly after opening a file from the Blueprint Files browser.

    SyncCanvasFromTemplate();

    // FIX 2: Pre-populate drag-start positions from the loaded node positions.
    // Without this, the first drag after loading a file would have no "before"
    // state in m_nodeDragStartPositions, causing MoveNodeCommand to receive
    // (0,0) as the start position and undo to delete-and-recreate the node.
    m_nodeDragStartPositions.clear();
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        m_nodeDragStartPositions[m_editorNodes[i].nodeID] =
            std::make_pair(m_editorNodes[i].posX, m_editorNodes[i].posY);
    }
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

    // CRITICAL FIX: Sync node positions from ImNodes BEFORE serialization.
    // RenderToolbar() (which calls Save) executes before RenderCanvas() syncs
    // positions, so we must pull fresh positions here to avoid stale data.
    SyncNodePositionsFromImNodes();

    bool ok = SerializeAndWrite(m_currentPath);
    SYSTEM_LOG << "[VisualScriptEditorPanel] Save() "
               << (ok ? "succeeded" : "FAILED") << ": '" << m_currentPath << "'\n";
    return ok;
}

bool VisualScriptEditorPanel::SaveAs(const std::string& path)
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] SaveAs() called. path='" << path << "'\n";

    if (path.empty())
        return false;

    // CRITICAL FIX: Same position sync as Save() — ensure fresh positions
    // before serialization regardless of when in the frame SaveAs is called.
    SyncNodePositionsFromImNodes();

    bool ok = SerializeAndWrite(path);
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
            ImVec2 pos = ImNodes::GetNodeEditorSpacePos(eNode.nodeID);
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

bool VisualScriptEditorPanel::SerializeAndWrite(const std::string& path)
{
    SYSTEM_LOG << "[VisualScriptEditorPanel] SerializeAndWrite: writing to '" << path << "'\n";

    SyncTemplateFromCanvas();

    json root;
    root["schema_version"] = 4;
    root["name"]           = m_template.Name;
    root["graphType"]      = "VisualScript";

    // Blackboard
    json bbArray = json::array();
    for (size_t i = 0; i < m_template.Blackboard.size(); ++i)
    {
        const BlackboardEntry& entry = m_template.Blackboard[i];
        json e;
        e["key"]      = entry.Key;
        e["isGlobal"] = entry.IsGlobal;

        switch (entry.Type)
        {
            case VariableType::Bool:     e["type"] = "Bool";     e["value"] = entry.Default.AsBool();     break;
            case VariableType::Int:      e["type"] = "Int";      e["value"] = entry.Default.AsInt();      break;
            case VariableType::Float:    e["type"] = "Float";    e["value"] = entry.Default.AsFloat();    break;
            case VariableType::String:   e["type"] = "String";   e["value"] = entry.Default.AsString();   break;
            case VariableType::EntityID: e["type"] = "EntityID"; e["value"] = std::to_string(entry.Default.AsEntityID()); break;
            default:                     e["type"] = "None";     e["value"] = nullptr;                    break;
        }
        bbArray.push_back(e);
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
    // Prevent SyncNodePositionsFromImNodes() from overwriting the correct
    // positions set by SyncEditorNodesFromTemplate() before ImNodes has
    // rendered the new positions even once.
    m_skipPositionSyncNextFrame = true;
    m_dirty = true;
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
    // Same as PerformUndo — skip position sync on the next frame.
    m_skipPositionSyncNextFrame = true;
    m_dirty = true;
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
}

void VisualScriptEditorPanel::RenderContent()
{
    RenderToolbar();
    RenderSaveAsDialog();
    ImGui::Separator();

    // Two-column layout: canvas (left) | properties + blackboard (right)
    float totalWidth = ImGui::GetContentRegionAvail().x;
    float canvasWidth = totalWidth * 0.72f;
    float propsWidth  = totalWidth - canvasWidth - 8.0f;

    ImGui::BeginChild("VSCanvas", ImVec2(canvasWidth, 0), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    RenderCanvas();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("VSProps", ImVec2(propsWidth, 0), true);
    RenderProperties();
    ImGui::Separator();
    RenderBlackboard();
    ImGui::EndChild();
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
    if (ImGui::Button("New Graph"))
    {
        m_template      = TaskGraphTemplate();
        m_currentPath.clear();
        m_editorNodes.clear();
        m_editorLinks.clear();
        m_positionedNodes.clear();
        m_nextNodeID    = 1;
        m_nextLinkID    = 1;
        m_dirty         = false;
        m_template.GraphType = "VisualScript";
    }

    // Keyboard shortcuts
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        if (ImGui::IsKeyPressed(ImGuiKey_S) &&
            ImGui::GetIO().KeyCtrl)
        {
            SYSTEM_LOG << "[VisualScriptEditorPanel] Ctrl+S pressed. m_currentPath='"
                       << m_currentPath << "'\n";
            Save();
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
    if (m_needsPositionSync)
    {
        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            ImNodes::SetNodeEditorSpacePos(
                m_editorNodes[i].nodeID,
                ImVec2(m_editorNodes[i].posX, m_editorNodes[i].posY));
        }
        m_needsPositionSync = false;
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

        auto execIn  = GetExecInputPins(eNode.def.Type);
        auto execOut = GetExecOutputPins(eNode.def.Type);

        std::vector<std::pair<std::string, VariableType>> dataIn, dataOut;
        for (size_t p = 0; p < eNode.def.DataPins.size(); ++p)
        {
            const DataPinDefinition& pin = eNode.def.DataPins[p];
            if (pin.Dir == DataPinDir::Input)
                dataIn.push_back({pin.PinName, pin.PinType});
            else
                dataOut.push_back({pin.PinName, pin.PinType});
        }

        VisualScriptNodeRenderer::RenderNode(
            eNode.nodeID,
            eNode.nodeID,
            0 /* graphID placeholder */,
            eNode.def.NodeName,
            eNode.def.Type,
            hasBreakpoint,
            isActive,
            execIn, execOut,
            dataIn, dataOut);

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
            ImNodes::PushColorStyle(ImNodesCol_Link, IM_COL32(160, 160, 255, 200));
        ImNodes::Link(link.linkID, link.srcAttrID, link.dstAttrID);
        if (link.isData)
            ImNodes::PopColorStyle();
    }

    ImNodes::EndNodeEditor();

    // FIX: Eagerly sync node positions back into m_editorNodes immediately
    // after every canvas render.  This ensures that when Save() is invoked
    // from RenderToolbar() at the START of the next frame (before RenderCanvas
    // runs again), m_editorNodes already holds the latest drag positions.
    // The SyncNodePositionsFromImNodes() call inside Save()/SaveAs() then acts
    // as a lightweight double-check rather than the sole sync point.
    //
    // FIX 4: Skip position sync if undo/redo just executed.  SyncEditorNodesFromTemplate()
    // has already written the correct undo-target positions into m_editorNodes and
    // m_needsPositionSync will push them to ImNodes before BeginNodeEditor().
    // Reading them back here (before ImNodes has rendered the new positions once)
    // would overwrite the correct values with stale ImNodes state.
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

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
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
            else
            {
                // Canvas background — open the node-palette popup
                ImVec2 mp = ImGui::GetMousePos();
                ImVec2 wp = ImGui::GetWindowPos();
                m_contextMenuX = mp.x - wp.x;
                m_contextMenuY = mp.y - wp.y;
                ImGui::OpenPopup("VSNodePalette");
                SYSTEM_LOG << "[VSEditor] Opened context menu on CANVAS at ("
                           << m_contextMenuX << ", " << m_contextMenuY << ")\n";
            }
        }

        // Node context menu
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
                    // Pin definitions are preserved; the new node has no
                    // connections since those live in m_template.ExecConnections
                    // and m_template.DataConnections which only reference it by ID.

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

        // Link context menu
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
        int newNodeID = AddNode(m_pendingNodeType, m_pendingNodeX, m_pendingNodeY);

        // Pre-register the position so ImNodes places the node correctly
        // on the very first frame it is rendered (next frame).
        ImNodes::SetNodeEditorSpacePos(newNodeID, ImVec2(m_pendingNodeX, m_pendingNodeY));

        m_dirty           = true;
        m_pendingNodeDrop = false;

        std::cout << "[VisualScriptEditorPanel] Node created: ID=" << newNodeID
                  << " type=" << static_cast<int>(m_pendingNodeType)
                  << " at (" << m_pendingNodeX << ", " << m_pendingNodeY << ")"
                  << std::endl;
    }

    // Track node moves for undo/redo using MoveNodeCommand.
    // Strategy: record the drag-start position on the first frame a move is
    // detected while the mouse button is held, then push a single
    // MoveNodeCommand when the drag ends (mouse released).
    // This ensures one undoable command per drag gesture, not one per frame.
    //
    // Only query nodes that have been rendered at least once (present in
    // m_positionedNodes) to avoid ImNodes assertions for brand-new nodes.
    {
        const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        for (size_t i = 0; i < m_editorNodes.size(); ++i)
        {
            VSEditorNode& eNode = m_editorNodes[i];
            if (m_positionedNodes.count(eNode.nodeID) == 0)
                continue;  // Node was added this frame; position will sync next frame

            const ImVec2 pos = ImNodes::GetNodeEditorSpacePos(eNode.nodeID);
            const bool posChanged = (std::abs(pos.x - eNode.posX) > 0.5f ||
                                     std::abs(pos.y - eNode.posY) > 0.5f);

            if (mouseDown)
            {
                if (posChanged)
                {
                    // Drag started (or continuing): record start pos once
                    if (m_nodeDragStartPositions.find(eNode.nodeID) ==
                        m_nodeDragStartPositions.end())
                    {
                        m_nodeDragStartPositions[eNode.nodeID] =
                            std::make_pair(eNode.posX, eNode.posY);
                    }
                }
                // Always keep eNode.posX/Y current so Save() returns live positions
                eNode.posX = pos.x;
                eNode.posY = pos.y;
            }
            else
            {
                // Mouse released: commit any completed drag as an undoable command
                auto startIt = m_nodeDragStartPositions.find(eNode.nodeID);
                if (startIt != m_nodeDragStartPositions.end())
                {
                    const float startX = startIt->second.first;
                    const float startY = startIt->second.second;
                    if (std::abs(pos.x - startX) > 0.5f ||
                        std::abs(pos.y - startY) > 0.5f)
                    {
                        m_undoStack.PushCommand(
                            std::unique_ptr<ICommand>(
                                new MoveNodeCommand(eNode.nodeID,
                                                    startX, startY,
                                                    pos.x,  pos.y)),
                            m_template);
                        SYSTEM_LOG << "[VSEditor] Node #" << eNode.nodeID
                                   << " moved: (" << startX << "," << startY
                                   << ") -> (" << pos.x << "," << pos.y
                                   << ") [UNDOABLE]\n";
                        m_dirty = true;
                    }
                    m_nodeDragStartPositions.erase(startIt);
                }
                eNode.posX = pos.x;
                eNode.posY = pos.y;
            }
        }
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
                    auto outPins = GetExecOutputPins(srcIt->def.Type);
                    if (srcPinIndex < static_cast<int>(outPins.size()))
                        srcPinName = outPins[srcPinIndex];
                }

                ConnectExec(srcNodeID, srcPinName, dstNodeID, "In");
                std::cout << "[VisualScriptEditorPanel] Created exec link: node"
                          << srcNodeID << "." << srcPinName
                          << " -> node" << dstNodeID << ".In\n";
                m_dirty = true;
            }
            else if (isDataLink)
            {
                // Resolve source data-out and destination data-in pin names
                const int srcPinIndex = startOffset - 300;
                const int dstPinIndex = endOffset   - 200;
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

    // Node name — use a local (non-static) buffer initialized from the current def
    char nameBuf[128];
    strncpy_s(nameBuf, sizeof(nameBuf), def.NodeName.c_str(), _TRUNCATE);
    if (ImGui::InputText("Name##vsname", nameBuf, sizeof(nameBuf)))
    {
        def.NodeName = nameBuf;
        // Sync back to template
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

    // Type-specific fields — all buffers are local (non-static) to avoid
    // stale data when switching between selected nodes.
    switch (def.Type)
    {
        case TaskNodeType::AtomicTask:
        {
            char taskBuf[128];
            strncpy_s(taskBuf, sizeof(taskBuf), def.AtomicTaskID.c_str(), _TRUNCATE);
            if (ImGui::InputText("TaskType##vstask", taskBuf, sizeof(taskBuf)))
            {
                def.AtomicTaskID = taskBuf;
                m_dirty = true;
            }
            break;
        }
        case TaskNodeType::Delay:
        {
            float delay = def.DelaySeconds;
            if (ImGui::InputFloat("Delay (s)##vsdelay", &delay, 0.1f, 1.0f))
            {
                def.DelaySeconds = delay;
                m_dirty = true;
            }
            break;
        }
        case TaskNodeType::GetBBValue:
        case TaskNodeType::SetBBValue:
        {
            char bbKeyBuf[128];
            strncpy_s(bbKeyBuf, sizeof(bbKeyBuf), def.BBKey.c_str(), _TRUNCATE);
            if (ImGui::InputText("BB Key##vsbbkey", bbKeyBuf, sizeof(bbKeyBuf)))
            {
                def.BBKey = bbKeyBuf;
                m_dirty = true;
            }
            break;
        }
        case TaskNodeType::Branch:
        case TaskNodeType::While:
        {
            char condBuf[128];
            strncpy_s(condBuf, sizeof(condBuf), def.ConditionID.c_str(), _TRUNCATE);
            if (ImGui::InputText("ConditionID##vscond", condBuf, sizeof(condBuf)))
            {
                def.ConditionID = condBuf;
                m_dirty = true;
            }
            break;
        }
        case TaskNodeType::SubGraph:
        {
            char sgPathBuf[256];
            strncpy_s(sgPathBuf, sizeof(sgPathBuf), def.SubGraphPath.c_str(), _TRUNCATE);
            if (ImGui::InputText("SubGraph Path##vssg", sgPathBuf, sizeof(sgPathBuf)))
            {
                def.SubGraphPath = sgPathBuf;
                m_dirty = true;
            }
            break;
        }
        case TaskNodeType::MathOp:
        {
            char mathOpBuf[8];
            strncpy_s(mathOpBuf, sizeof(mathOpBuf), def.MathOperator.c_str(), _TRUNCATE);
            if (ImGui::InputText("Operator (+,-,*,/)##vsmath", mathOpBuf, sizeof(mathOpBuf)))
            {
                def.MathOperator = mathOpBuf;
                m_dirty = true;
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
}

void VisualScriptEditorPanel::RenderBlackboard()
{
    ImGui::TextDisabled("Local Blackboard");
    ImGui::Separator();

    // Add entry button
    if (ImGui::Button("+##vsbbAdd"))
    {
        BlackboardEntry entry;
        entry.Key      = "newKey";
        entry.Type     = VariableType::Float;
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

        // Type selector
        const char* typeLabels[] = {"None","Bool","Int","Float","Vector","EntityID","String"};
        int typeIdx = static_cast<int>(entry.Type);
        ImGui::SetNextItemWidth(80.0f);
        if (ImGui::Combo("##bbtype", &typeIdx, typeLabels, 7))
        {
            entry.Type = static_cast<VariableType>(typeIdx);
            m_dirty    = true;
        }
        ImGui::SameLine();

        // IsGlobal checkbox
        ImGui::Checkbox("G##bbglob", &entry.IsGlobal);
        ImGui::SameLine();

        // Remove button
        if (ImGui::SmallButton("x##bbdel"))
        {
            m_template.Blackboard.erase(m_template.Blackboard.begin() + idx);
            m_dirty = true;
            ImGui::PopID();
            continue;
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

} // namespace Olympe
