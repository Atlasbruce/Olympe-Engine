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

#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../json_helper.h"
#include "../TaskSystem/TaskGraphLoader.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace Olympe {

// ============================================================================
// Constructor / Destructor
// ============================================================================

VisualScriptEditorPanel::VisualScriptEditorPanel()
{
}

VisualScriptEditorPanel::~VisualScriptEditorPanel()
{
}

void VisualScriptEditorPanel::Initialize()
{
    // Nothing to initialize beyond the constructor.
}

void VisualScriptEditorPanel::Shutdown()
{
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

// ============================================================================
// Node management
// ============================================================================

int VisualScriptEditorPanel::AddNode(TaskNodeType type, float x, float y)
{
    int newID = AllocNodeID();

    VSEditorNode eNode;
    eNode.nodeID = newID;
    eNode.posX   = x;
    eNode.posY   = y;
    // Note: ImNodes will position the node at (0,0) on first render.
    // User must manually drag it to desired position, or we implement
    // a custom positioning system in VisualScriptNodeRenderer.

    eNode.def.NodeID   = newID;
    eNode.def.Type     = type;
    eNode.def.NodeName = GetNodeTypeLabel(type);

    // EntryPoint is special
    if (type == TaskNodeType::EntryPoint && m_template.EntryPointID == NODE_INDEX_NONE)
    {
        m_template.EntryPointID = newID;
        m_template.RootNodeID   = newID;
    }

    m_editorNodes.push_back(eNode);
    m_template.Nodes.push_back(eNode.def);
    m_template.BuildLookupCache();
    m_dirty = true;
    return newID;
}

void VisualScriptEditorPanel::RemoveNode(int nodeID)
{
    // Remove from editor nodes
    m_editorNodes.erase(
        std::remove_if(m_editorNodes.begin(), m_editorNodes.end(),
                       [nodeID](const VSEditorNode& n) { return n.nodeID == nodeID; }),
        m_editorNodes.end());

    // Remove from template nodes
    m_template.Nodes.erase(
        std::remove_if(m_template.Nodes.begin(), m_template.Nodes.end(),
                       [nodeID](const TaskNodeDefinition& n) { return n.NodeID == nodeID; }),
        m_template.Nodes.end());

    // Remove associated exec connections
    m_template.ExecConnections.erase(
        std::remove_if(m_template.ExecConnections.begin(),
                       m_template.ExecConnections.end(),
                       [nodeID](const ExecPinConnection& c) {
                           return c.SourceNodeID == nodeID || c.TargetNodeID == nodeID;
                       }),
        m_template.ExecConnections.end());

    // Remove associated data connections
    m_template.DataConnections.erase(
        std::remove_if(m_template.DataConnections.begin(),
                       m_template.DataConnections.end(),
                       [nodeID](const DataPinConnection& c) {
                           return c.SourceNodeID == nodeID || c.TargetNodeID == nodeID;
                       }),
        m_template.DataConnections.end());

    m_template.BuildLookupCache();
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
    m_template.ExecConnections.push_back(conn);
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
    m_template.DataConnections.push_back(conn);
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
        eNode.posX   = 200.0f * static_cast<float>(i);  // Default auto-layout
        eNode.posY   = 100.0f;

        if (def.NodeID >= m_nextNodeID)
            m_nextNodeID = def.NodeID + 1;

        m_editorNodes.push_back(eNode);
    }

    RebuildLinks();
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

    // Data links (simplified — pin index 0 for now)
    for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = m_template.DataConnections[i];
        VSEditorLink link;
        link.linkID    = AllocLinkID();
        link.srcAttrID = DataOutAttrUID(conn.SourceNodeID, 0);
        link.dstAttrID = DataInAttrUID(conn.TargetNodeID, 0);
        link.isData    = true;
        m_editorLinks.push_back(link);
    }
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

    SyncCanvasFromTemplate();
}

bool VisualScriptEditorPanel::Save()
{
    if (m_currentPath.empty())
        return false;
    return SerializeAndWrite(m_currentPath);
}

bool VisualScriptEditorPanel::SaveAs(const std::string& path)
{
    if (path.empty())
        return false;
    bool ok = SerializeAndWrite(path);
    if (ok)
    {
        m_currentPath = path;
        m_dirty       = false;
    }
    return ok;
}

bool VisualScriptEditorPanel::SerializeAndWrite(const std::string& path)
{
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
    std::ofstream ofs(path);
    if (!ofs.is_open())
    {
        std::cerr << "[VisualScriptEditorPanel] Cannot open file for write: " << path << std::endl;
        return false;
    }
    ofs << root.dump(2);
    ofs.close();
    m_dirty = false;
    return true;
}

// ============================================================================
// Rendering
// ============================================================================

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
        if (!Save())
            ImGui::OpenPopup("SaveError");
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
            Save();
        }
    }

    if (ImGui::BeginPopup("SaveError"))
    {
        ImGui::TextColored(ImVec4(1,0,0,1), "Save failed — check file path.");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void VisualScriptEditorPanel::RenderCanvas()
{
    ImNodes::BeginNodeEditor();

    // Context menu for node palette
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup("VSNodePalette");
        ImVec2 mp = ImGui::GetMousePos();
        ImVec2 wp = ImGui::GetWindowPos();
        m_contextMenuX = mp.x - wp.x;
        m_contextMenuY = mp.y - wp.y;
    }
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
            dataIn, dataOut,
            GetNodeTypeLabel(eNode.def.Type));

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

    // Handle drag & drop from Asset Browser node palette
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VS_NODE_TYPE_ENUM");
        if (payload && payload->Data && payload->DataSize == sizeof(uint8_t))
        {
            uint8_t enumValue = *static_cast<const uint8_t*>(payload->Data);
            TaskNodeType nodeType = static_cast<TaskNodeType>(enumValue);

            // Get mouse position in canvas space
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 canvasPos = ImNodes::EditorContextGetPanning();
            float zoom = 1.0f;  // ImNodes doesn't expose zoom yet, assume 1.0

            // Convert screen space to canvas space
            ImVec2 windowPos = ImGui::GetWindowPos();
            float canvasX = (mousePos.x - windowPos.x - canvasPos.x) / zoom;
            float canvasY = (mousePos.y - windowPos.y - canvasPos.y) / zoom;

            // Add the node at drop position and pre-register its position with
            // ImNodes so the position-sync loop this frame does not assert on
            // a node that has never been through BeginNode/EndNode yet.
            int newNodeID = AddNode(nodeType, canvasX, canvasY);
            ImNodes::SetNodeEditorSpacePos(newNodeID, ImVec2(canvasX, canvasY));
            m_dirty = true;

            std::cout << "[VisualScriptEditorPanel] Node dropped: type=" 
                     << static_cast<int>(nodeType) << " at (" 
                     << canvasX << ", " << canvasY << ")" << std::endl;
        }
        ImGui::EndDragDropTarget();
    }

    // Sync positions back from ImNodes after rendering.
    // Only sync nodes that were actually rendered this frame (i.e., went through
    // ImNodes BeginNode/EndNode) to avoid the "node_idx != -1" assertion for
    // nodes that were just added via drag-and-drop in the same frame.
    for (size_t i = 0; i < m_editorNodes.size(); ++i)
    {
        VSEditorNode& eNode = m_editorNodes[i];
        if (m_positionedNodes.count(eNode.nodeID) == 0)
            continue;  // Node was added this frame; position will sync next frame
        ImVec2 pos = ImNodes::GetNodeEditorSpacePos(eNode.nodeID);
        eNode.posX = pos.x;
        eNode.posY = pos.y;
    }

    // Handle new link creation
    int startAttr = -1, endAttr = -1;
    if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
    {
        // Determine if exec or data link from attribute UID pattern
        // (just create an exec link for simplicity — full routing is in sync)
        int srcNodeID = startAttr / 10000;
        int dstNodeID = endAttr   / 10000;
        ConnectExec(srcNodeID, "Out", dstNodeID, "In");
        m_dirty = true;
    }

    // Handle link deletion
    int destroyedLink = -1;
    if (ImNodes::IsLinkDestroyed(&destroyedLink))
    {
        m_editorLinks.erase(
            std::remove_if(m_editorLinks.begin(), m_editorLinks.end(),
                           [destroyedLink](const VSEditorLink& l) {
                               return l.linkID == destroyedLink;
                           }),
            m_editorLinks.end());
        m_dirty = true;
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
