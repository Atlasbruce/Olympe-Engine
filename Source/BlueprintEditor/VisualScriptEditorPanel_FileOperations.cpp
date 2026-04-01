/**
 * @file VisualScriptEditorPanel_FileOperations.cpp
 * @brief File operations for VisualScriptEditorPanel (Phase 7 refactoring).
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details Extracted methods for file I/O operations:
 *   - LoadTemplate() — Load blueprint from file/memory with preset loading (Phase 24)
 *   - Save() — Save current graph to m_currentPath
 *   - SaveAs() — Save graph to new path
 *   - SyncNodePositionsFromImNodes() — Sync grid-space positions (BUG-003 Fix)
 *   - SyncPresetsFromRegistryToTemplate() — Phase 24 preset synchronization
 *   - SerializeAndWrite() — Complete JSON v4 serialization with all Phase 24 features
 *
 * @note C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "VisualScriptEditorPanel.h"
#include "DebugController.h"
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

namespace Olympe {

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

    // Phase 24 CRITICAL FIX: Sync both SubGraphPath and Parameters["subgraph_path"]
    // After loading, both storage locations should be synchronized:
    // 1. If SubGraphPath has a value, ensure Parameters["subgraph_path"] exists
    // 2. If Parameters["subgraph_path"] has a value, sync to SubGraphPath
    // This handles both old (subGraphPath field only) and new (params field) JSON formats
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].Type == TaskNodeType::SubGraph)
        {
            auto pathParamIt = m_template.Nodes[i].Parameters.find("subgraph_path");

            // Case 1: SubGraphPath has value, Parameters["subgraph_path"] doesn't
            if (!m_template.Nodes[i].SubGraphPath.empty() && pathParamIt == m_template.Nodes[i].Parameters.end())
            {
                ParameterBinding pathBinding;
                pathBinding.Type = ParameterBindingType::Literal;
                pathBinding.LiteralValue = TaskValue(m_template.Nodes[i].SubGraphPath);
                m_template.Nodes[i].Parameters["subgraph_path"] = pathBinding;
                SYSTEM_LOG << "[VSEditor] LoadTemplate: initialized Parameters[subgraph_path] from SubGraphPath = '"
                           << m_template.Nodes[i].SubGraphPath << "' for node " 
                           << m_template.Nodes[i].NodeID << "\n";
            }
            // Case 2: Parameters["subgraph_path"] exists, sync to SubGraphPath if needed
            else if (pathParamIt != m_template.Nodes[i].Parameters.end() &&
                     pathParamIt->second.Type == ParameterBindingType::Literal)
            {
                std::string paramPath = pathParamIt->second.LiteralValue.to_string();
                if (!paramPath.empty())
                {
                    // Use parameter value as canonical
                    if (m_template.Nodes[i].SubGraphPath != paramPath)
                    {
                        m_template.Nodes[i].SubGraphPath = paramPath;
                        SYSTEM_LOG << "[VSEditor] LoadTemplate: synced SubGraphPath from Parameters[subgraph_path] = '"
                                   << paramPath << "' for node " 
                                   << m_template.Nodes[i].NodeID << "\n";
                    }
                }
                else if (!m_template.Nodes[i].SubGraphPath.empty())
                {
                    // Parameters is empty, sync SubGraphPath to it
                    pathParamIt->second.LiteralValue = TaskValue(m_template.Nodes[i].SubGraphPath);
                    SYSTEM_LOG << "[VSEditor] LoadTemplate: synced Parameters[subgraph_path] from SubGraphPath = '"
                               << m_template.Nodes[i].SubGraphPath << "' for node " 
                               << m_template.Nodes[i].NodeID << "\n";
                }
            }
        }
    }

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

    // Phase 24 CRITICAL FIX: Sync SubGraph paths bidirectionally before save
    // This ensures that whether the path is in SubGraphPath OR Parameters["subgraph_path"],
    // both will be synchronized before serialization.
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].Type == TaskNodeType::SubGraph)
        {
            auto pathIt = m_template.Nodes[i].Parameters.find("subgraph_path");

            // Case 1: Parameters["subgraph_path"] exists with value → sync to SubGraphPath
            if (pathIt != m_template.Nodes[i].Parameters.end() &&
                pathIt->second.Type == ParameterBindingType::Literal)
            {
                std::string paramPath = pathIt->second.LiteralValue.to_string();
                if (!paramPath.empty())
                {
                    m_template.Nodes[i].SubGraphPath = paramPath;
                    SYSTEM_LOG << "[VisualScriptEditorPanel::Save] Synced SubGraphPath from Parameters[subgraph_path] = '"
                               << paramPath << "' for node " << m_template.Nodes[i].NodeID << "\n";
                }
            }

            // Case 2: SubGraphPath has value but Parameters["subgraph_path"] doesn't → create it
            if (!m_template.Nodes[i].SubGraphPath.empty() && 
                (pathIt == m_template.Nodes[i].Parameters.end() ||
                 (pathIt->second.Type == ParameterBindingType::Literal && 
                  pathIt->second.LiteralValue.to_string().empty())))
            {
                ParameterBinding pathBinding;
                pathBinding.Type = ParameterBindingType::Literal;
                pathBinding.LiteralValue = TaskValue(m_template.Nodes[i].SubGraphPath);
                m_template.Nodes[i].Parameters["subgraph_path"] = pathBinding;
                SYSTEM_LOG << "[VisualScriptEditorPanel::Save] Created Parameters[subgraph_path] = '"
                           << m_template.Nodes[i].SubGraphPath << "' for node " 
                           << m_template.Nodes[i].NodeID << "\n";
            }
        }
    }

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

    // Phase 24 CRITICAL FIX: Sync SubGraph paths bidirectionally before save
    // This ensures that whether the path is in SubGraphPath OR Parameters["subgraph_path"],
    // both will be synchronized before serialization.
    for (size_t i = 0; i < m_template.Nodes.size(); ++i)
    {
        if (m_template.Nodes[i].Type == TaskNodeType::SubGraph)
        {
            auto pathIt = m_template.Nodes[i].Parameters.find("subgraph_path");

            // Case 1: Parameters["subgraph_path"] exists with value → sync to SubGraphPath
            if (pathIt != m_template.Nodes[i].Parameters.end() &&
                pathIt->second.Type == ParameterBindingType::Literal)
            {
                std::string paramPath = pathIt->second.LiteralValue.to_string();
                if (!paramPath.empty())
                {
                    m_template.Nodes[i].SubGraphPath = paramPath;
                    SYSTEM_LOG << "[VisualScriptEditorPanel::SaveAs] Synced SubGraphPath from Parameters[subgraph_path] = '"
                               << paramPath << "' for node " << m_template.Nodes[i].NodeID << "\n";
                }
            }

            // Case 2: SubGraphPath has value but Parameters["subgraph_path"] doesn't → create it
            if (!m_template.Nodes[i].SubGraphPath.empty() && 
                (pathIt == m_template.Nodes[i].Parameters.end() ||
                 (pathIt->second.Type == ParameterBindingType::Literal && 
                  pathIt->second.LiteralValue.to_string().empty())))
            {
                ParameterBinding pathBinding;
                pathBinding.Type = ParameterBindingType::Literal;
                pathBinding.LiteralValue = TaskValue(m_template.Nodes[i].SubGraphPath);
                m_template.Nodes[i].Parameters["subgraph_path"] = pathBinding;
                SYSTEM_LOG << "[VisualScriptEditorPanel::SaveAs] Created Parameters[subgraph_path] = '"
                           << m_template.Nodes[i].SubGraphPath << "' for node " 
                           << m_template.Nodes[i].NodeID << "\n";
            }
        }
    }

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
            // Input parameters: map of name -> ParameterBinding
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

            // Output parameters: map of name -> blackboard key
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

} // namespace Olympe
