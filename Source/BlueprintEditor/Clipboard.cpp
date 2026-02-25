/**
 * @file Clipboard.cpp
 * @brief Implementation of NodeGraphClipboard.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Serialisation format (JSON, one object per copied node):
 * {
 *   "nodes": [
 *     {
 *       "type": "BT_Action",
 *       "name": "Move",
 *       "actionType": "MoveToLocation",
 *       "conditionType": "",
 *       "decoratorType": "",
 *       "params": { "speed": "5.0" },
 *       "dx": 0.0,
 *       "dy": 0.0
 *     }
 *   ]
 * }
 *
 * "dx"/"dy" are position offsets relative to the top-left bounding corner of
 * the copied selection, so pasted nodes reproduce the original spatial layout.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Clipboard.h"
#include "../system/system_utils.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include "../third_party/nlohmann/json.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <cfloat>
#include <algorithm>

using json = nlohmann::json;

namespace Olympe
{

// ============================================================================
// Constants
// ============================================================================

const char* NodeGraphClipboard::k_ClipPrefix = "OLYMPE_NG_CLIP:";

// Multiplier used to convert local node IDs to ImNodes global UIDs.
// Must match the value in NodeGraphPanel.cpp.
static constexpr int CLIP_GRAPH_ID_MULTIPLIER = 10000;

// ============================================================================
// Singleton
// ============================================================================

NodeGraphClipboard& NodeGraphClipboard::Get()
{
    static NodeGraphClipboard s_instance;
    return s_instance;
}

// ============================================================================
// CopySelectedNodes
// ============================================================================

void NodeGraphClipboard::CopySelectedNodes(NodeGraph* graph, int graphID)
{
    if (graph == nullptr)
        return;

    int numSelected = ImNodes::NumSelectedNodes();
    if (numSelected <= 0)
        return;

    std::vector<int> selectedUIDs(numSelected);
    ImNodes::GetSelectedNodes(selectedUIDs.data());

    // Collect local node IDs and find bounding box for relative offsets.
    float minX = FLT_MAX;
    float minY = FLT_MAX;

    std::vector<GraphNode*> selectedNodes;
    selectedNodes.reserve(static_cast<size_t>(numSelected));

    for (int globalUID : selectedUIDs)
    {
        int localId = globalUID - (graphID * CLIP_GRAPH_ID_MULTIPLIER);
        GraphNode* node = graph->GetNode(localId);
        if (node == nullptr)
            continue;

        selectedNodes.push_back(node);
        if (node->posX < minX) minX = node->posX;
        if (node->posY < minY) minY = node->posY;
    }

    if (selectedNodes.empty())
        return;

    // Build JSON payload.
    json payload = json::object();
    json nodesArr = json::array();

    for (GraphNode* node : selectedNodes)
    {
        json nodeObj = json::object();
        nodeObj["type"]          = NodeTypeToString(node->type);
        nodeObj["name"]          = node->name;
        nodeObj["actionType"]    = node->actionType;
        nodeObj["conditionType"] = node->conditionType;
        nodeObj["decoratorType"] = node->decoratorType;
        nodeObj["dx"]            = node->posX - minX;
        nodeObj["dy"]            = node->posY - minY;

        json paramsObj = json::object();
        for (auto it = node->parameters.begin(); it != node->parameters.end(); ++it)
        {
            paramsObj[it->first] = it->second;
        }
        nodeObj["params"] = paramsObj;

        nodesArr.push_back(nodeObj);
    }

    payload["nodes"] = nodesArr;

    // Write to system clipboard with prefix.
    std::string clipStr = k_ClipPrefix;
    clipStr += payload.dump();
    ImGui::SetClipboardText(clipStr.c_str());

    SYSTEM_LOG << "[NodeGraphClipboard] Copied " << numSelected << " node(s) to clipboard.\n";
}

// ============================================================================
// PasteNodes
// ============================================================================

void NodeGraphClipboard::PasteNodes(NodeGraph* graph, int graphID,
                                     float mousePosX, float mousePosY,
                                     bool snapToGrid, float snapGridSize)
{
    if (graph == nullptr)
        return;

    const char* rawClip = ImGui::GetClipboardText();
    if (rawClip == nullptr)
        return;

    // Validate prefix.
    size_t prefixLen = std::strlen(k_ClipPrefix);
    if (std::strncmp(rawClip, k_ClipPrefix, prefixLen) != 0)
        return;

    const char* jsonStart = rawClip + prefixLen;

    json payload;
    try
    {
        payload = json::parse(jsonStart);
    }
    catch (...)
    {
        SYSTEM_LOG << "[NodeGraphClipboard] Paste failed: invalid JSON payload.\n";
        return;
    }

    if (!payload.is_object() || !payload.contains("nodes") || !payload["nodes"].is_array())
    {
        SYSTEM_LOG << "[NodeGraphClipboard] Paste failed: missing 'nodes' array.\n";
        return;
    }

    const json& nodesArr = payload["nodes"];
    int pastedCount = 0;

    for (size_t i = 0; i < nodesArr.size(); ++i)
    {
        const json& nodeObj = nodesArr[i];
        if (!nodeObj.is_object())
            continue;

        std::string typeStr;
        std::string nodeName;
        std::string actionType;
        std::string conditionType;
        std::string decoratorType;
        float dx = 0.0f;
        float dy = 0.0f;

        if (nodeObj.contains("type") && nodeObj["type"].is_string())
            typeStr = nodeObj["type"].get<std::string>();
        if (nodeObj.contains("name") && nodeObj["name"].is_string())
            nodeName = nodeObj["name"].get<std::string>();
        if (nodeObj.contains("actionType") && nodeObj["actionType"].is_string())
            actionType = nodeObj["actionType"].get<std::string>();
        if (nodeObj.contains("conditionType") && nodeObj["conditionType"].is_string())
            conditionType = nodeObj["conditionType"].get<std::string>();
        if (nodeObj.contains("decoratorType") && nodeObj["decoratorType"].is_string())
            decoratorType = nodeObj["decoratorType"].get<std::string>();
        if (nodeObj.contains("dx") && nodeObj["dx"].is_number())
            dx = nodeObj["dx"].get<float>();
        if (nodeObj.contains("dy") && nodeObj["dy"].is_number())
            dy = nodeObj["dy"].get<float>();

        NodeType ntype = StringToNodeType(typeStr);
        float pasteX   = mousePosX + dx;
        float pasteY   = mousePosY + dy;

        // Apply snap-to-grid to each pasted node individually.
        if (snapToGrid && snapGridSize > 0.0f)
        {
            pasteX = std::roundf(pasteX / snapGridSize) * snapGridSize;
            pasteY = std::roundf(pasteY / snapGridSize) * snapGridSize;
        }

        int newId = graph->CreateNode(ntype, pasteX, pasteY, nodeName.empty() ? typeStr : nodeName);
        GraphNode* newNode = graph->GetNode(newId);
        if (newNode == nullptr)
            continue;

        // Apply immediate ImNodes position so the node appears at the correct
        // grid location this frame (before the next RenderGraph pass).
        int globalUID = graphID * CLIP_GRAPH_ID_MULTIPLIER + newId;
        ImNodes::SetNodeGridSpacePos(globalUID, ImVec2(pasteX, pasteY));

        newNode->actionType    = actionType;
        newNode->conditionType = conditionType;
        newNode->decoratorType = decoratorType;

        // Restore parameters.
        if (nodeObj.contains("params") && nodeObj["params"].is_object())
        {
            const json& paramsObj = nodeObj["params"];
            for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it)
            {
                if (it.value().is_string())
                    newNode->parameters[it.key()] = it.value().get<std::string>();
            }
        }

        ++pastedCount;
    }

    if (pastedCount > 0)
    {
        graph->MarkDirty();
        SYSTEM_LOG << "[NodeGraphClipboard] Pasted " << pastedCount << " node(s).\n";
    }
}

} // namespace Olympe
