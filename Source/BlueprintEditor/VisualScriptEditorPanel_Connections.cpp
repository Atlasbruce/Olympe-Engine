/**
 * @file VisualScriptEditorPanel_Connections.cpp
 * @brief Link and pin management for VS Editor Panel (execution and data connections).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Part of the VisualScriptEditorPanel refactoring (Phase 1).
 * This file contains:
 * - Execution connection management (ConnectExec, RebuildLinks)
 * - Data connection management (ConnectData, RemoveLink)
 * - Pin query helpers (GetExecOutputPins, GetDataInputPins, etc.)
 * - Link validation and error handling
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "VSConnectionValidator.h"
#include "../system/system_utils.h"
#include "../system/system_consts.h"

#include <algorithm>

namespace Olympe {

// ============================================================================
// Pin Name Queries
// ============================================================================

std::vector<std::string> VisualScriptEditorPanel::GetExecInputPins(TaskNodeType type)
{
    switch (type)
    {
        case TaskNodeType::EntryPoint:
            return {};  // No exec-in on EntryPoint
        case TaskNodeType::GetBBValue:
            return {};  // Phase 24.2: Variable (GetBBValue) is data-pure (no execution pins)
        case TaskNodeType::MathOp:
            return {};  // Phase 24.2: MathOp is data-pure (no execution pins)
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
        case TaskNodeType::GetBBValue:  return {};  // Phase 24.2: Variable (GetBBValue) is data-pure (no execution pins)
        case TaskNodeType::SetBBValue:  return {"Completed"};  // SetBBValue needs exec-out for control flow
        case TaskNodeType::MathOp:      return {};  // Phase 24.2: MathOp is data-pure (no execution pins)
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
// Link Management: Connect, Disconnect
// ============================================================================

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

        ExecPinConnection conn;
        conn.SourceNodeID  = srcNodeID;
        conn.SourcePinName = srcPinName;
        conn.TargetNodeID  = dstNodeID;
        conn.TargetPinName = dstPinName;
        m_undoStack.PushCommand(
            std::unique_ptr<ICommand>(new DeleteLinkCommand(conn)),
            m_template);
    }

    m_dirty = true;
    m_verificationDone = false;
}

// ============================================================================
// Link Rebuilding
// ============================================================================

void VisualScriptEditorPanel::RebuildLinks()
{
    m_editorLinks.clear();

    // Rebuild from template exec connections
    for (size_t i = 0; i < m_template.ExecConnections.size(); ++i)
    {
        const ExecPinConnection& conn = m_template.ExecConnections[i];

        // Find exec-out pin index
        std::vector<std::string> outPins;
        const TaskNodeDefinition* srcNode = m_template.GetNode(conn.SourceNodeID);
        if (srcNode)
            outPins = GetExecOutputPinsForNode(*srcNode);

        int pinIdx = 0;
        for (int pi = 0; pi < static_cast<int>(outPins.size()); ++pi)
        {
            if (outPins[static_cast<size_t>(pi)] == conn.SourcePinName)
            {
                pinIdx = pi;
                break;
            }
        }

        VSEditorLink link;
        link.linkID    = AllocLinkID();
        link.srcAttrID = conn.SourceNodeID * 10000 + 100 + pinIdx;
        link.dstAttrID = conn.TargetNodeID * 10000 + 0;  // exec-in is always 0
        link.isData    = false;

        m_editorLinks.push_back(link);
    }

    // Rebuild from template data connections
    for (size_t i = 0; i < m_template.DataConnections.size(); ++i)
    {
        const DataPinConnection& conn = m_template.DataConnections[i];

        // Find data-out pin index
        int srcPinIdx = 0;
        const TaskNodeDefinition* srcNode = m_template.GetNode(conn.SourceNodeID);
        if (srcNode)
        {
            auto pins = GetDataOutputPins(srcNode->Type);
            bool found = false;
            for (int pi = 0; pi < static_cast<int>(pins.size()); ++pi)
            {
                if (pins[static_cast<size_t>(pi)] == conn.SourcePinName)
                {
                    srcPinIdx = pi;
                    found = true;
                    break;
                }
            }
            if (!found && srcNode->Type == TaskNodeType::Branch)
            {
                // Check dynamic pins for Branch
                for (size_t d = 0; d < srcNode->dynamicPins.size(); ++d)
                {
                    if (srcNode->dynamicPins[d].PinName == conn.SourcePinName &&
                        srcNode->dynamicPins[d].Dir == DataPinDir::Output)
                    {
                        srcPinIdx = static_cast<int>(d);
                        break;
                    }
                }
            }
        }

        // Find data-in pin index
        int dstPinIdx = 0;
        const TaskNodeDefinition* dstNode = m_template.GetNode(conn.TargetNodeID);
        if (dstNode)
        {
            auto pins = GetDataInputPins(dstNode->Type);
            bool foundDynamicPin = false;
            for (int pi = 0; pi < static_cast<int>(pins.size()); ++pi)
            {
                if (pins[static_cast<size_t>(pi)] == conn.TargetPinName)
                {
                    dstPinIdx = pi;
                    foundDynamicPin = true;
                    break;
                }
            }
            if (!foundDynamicPin && dstNode->Type == TaskNodeType::Branch)
            {
                for (size_t d = 0; d < dstNode->dynamicPins.size(); ++d)
                {
                    if (dstNode->dynamicPins[d].PinName == conn.TargetPinName &&
                        dstNode->dynamicPins[d].Dir == DataPinDir::Input)
                    {
                        dstPinIdx = static_cast<int>(d);
                        break;
                    }
                }
            }
        }

        VSEditorLink link;
        link.linkID    = AllocLinkID();
        link.srcAttrID = conn.SourceNodeID * 10000 + 300 + srcPinIdx;  // data-out range 300-399
        link.dstAttrID = conn.TargetNodeID * 10000 + 200 + dstPinIdx;  // data-in range 200-299
        link.isData    = true;

        m_editorLinks.push_back(link);
    }
}

} // namespace Olympe
