/**
 * @file BTNodePropertyPanel.h
 * @brief Property editor panel for BehaviorTree nodes
 * @author Olympe Engine
 * @date 2026-04-08
 *
 * @details
 * Displays and allows editing of properties for the currently selected BT node.
 * Supports string, int, and float parameter types.
 * Mirrors EntityPrefab's PropertyEditorPanel pattern for consistency.
 */

#pragma once

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <string>
#include <vector>
#include <cstring>
#include <map>
#include "../third_party/imgui/imgui.h"
#include "../NodeGraphCore/NodeGraphManager.h"
#include "BTNodeGraphManager.h"
#include "../system/system_utils.h"

namespace Olympe {

// Forward declarations
struct GraphNode;
class BTNodeGraphManager;

/**
 * @class BTNodePropertyPanel
 * @brief UI panel for editing BehaviorTree node properties
 *
 * Renders property controls for the currently selected node.
 * Updates the BTNodeGraphManager when properties are changed.
 */
class BTNodePropertyPanel
{
public:
    BTNodePropertyPanel()
        : m_activeGraphId(-1)
        , m_selectedNodeId(-1)
    {
        std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
        std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
    }

    ~BTNodePropertyPanel() = default;

    /**
     * @brief Initialize the panel
     */
    void Initialize()
    {
        // Initialize property panel
    }

    /**
     * @brief Render the property panel
     */
    void Render()
    {
        if (m_activeGraphId < 0 || m_selectedNodeId < 0)
        {
            ImGui::TextDisabled("No node selected");
            return;
        }

        // Fetch the modern graph document and node data via NodeGraphManager
        auto& manager = NodeGraph::NodeGraphManager::Get();
        NodeGraphTypes::GraphDocument* graphDoc = manager.GetGraph(NodeGraphTypes::GraphId{static_cast<uint32_t>(m_activeGraphId)});
        if (!graphDoc)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Graph %d not found", m_activeGraphId);
            return;
        }

        NodeGraphTypes::NodeData* node = graphDoc->GetNode(NodeGraphTypes::NodeId{static_cast<uint32_t>(m_selectedNodeId)});
        if (!node)
        {
            ImGui::TextDisabled("Node %d not found in graph", m_selectedNodeId);
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 8));

        // Get node type for color coding
        NodeType nodeType = StringToNodeType(node->type);
        ImVec4 headerColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f); // Default Blue

        switch(nodeType)
        {
            case NodeType::BT_Root:
            case NodeType::BT_OnEvent: headerColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f); break; // Green
            case NodeType::BT_Sequence:
            case NodeType::BT_Selector:
            case NodeType::BT_Parallel: headerColor = ImVec4(0.2f, 0.4f, 0.8f, 1.0f); break; // Blue
            case NodeType::BT_Condition: headerColor = ImVec4(0.7f, 0.4f, 0.8f, 1.0f); break; // Purple
            case NodeType::BT_Decorator: headerColor = ImVec4(0.8f, 0.5f, 0.7f, 1.0f); break; // Plum
            case NodeType::BT_Action: headerColor = ImVec4(1.0f, 0.6f, 0.2f, 1.0f); break; // Orange
            default: break;
        }

        ImGui::TextColored(headerColor, "%s Node", NodeTypeToString(nodeType));
        ImGui::Separator();

        // LEGACY RESTORATION: Specific BT Fields
        if (nodeType == NodeType::BT_Action || nodeType == NodeType::BT_Condition || nodeType == NodeType::BT_Decorator)
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Internal Type");

            std::string key = "";
            if (nodeType == NodeType::BT_Action) key = "actionType";
            else if (nodeType == NodeType::BT_Condition) key = "conditionType";
            else if (nodeType == NodeType::BT_Decorator) key = "decoratorType";

            std::string value = node->parameters.count(key) ? node->parameters.at(key) : "";

            char typeBuf[128];
            memset(typeBuf, 0, sizeof(typeBuf));
#ifdef _MSC_VER
            strncpy_s(typeBuf, sizeof(typeBuf), value.c_str(), _TRUNCATE);
#else
            strncpy(typeBuf, value.c_str(), sizeof(typeBuf) - 1);
#endif

            if (ImGui::InputText("##BTInternalType", typeBuf, sizeof(typeBuf)))
            {
                node->parameters[key] = typeBuf;
                graphDoc->SetDirty(true);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(%s)", key.c_str());
        }

        ImGui::Spacing();

        // Node Identification
        ImGui::TextDisabled("ID: %d", node->id.value);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 100);
        ImGui::TextDisabled("Type: %s", node->type.c_str());

        // Name Editing
        ImGui::TextUnformatted("Name");
        ImGui::SetNextItemWidth(-1);
        char nameBuf[256];
        memset(nameBuf, 0, sizeof(nameBuf));
#ifdef _MSC_VER
        strncpy_s(nameBuf, sizeof(nameBuf), node->name.c_str(), _TRUNCATE);
#else
        strncpy(nameBuf, node->name.c_str(), sizeof(nameBuf) - 1);
#endif
        if (ImGui::InputText("##NodeName", nameBuf, sizeof(nameBuf)))
        {
            node->name = nameBuf;
            graphDoc->SetDirty(true);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rename node for better organization");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Parameter Editing
        if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool changed = false;
            
            // Standard parameters based on BT type
            if (node->parameters.empty())
            {
                ImGui::TextDisabled("No parameters available");
            }
            else
            {
                for (auto& param : node->parameters)
                {
                    ImGui::TextUnformatted(param.first.c_str());
                    ImGui::SetNextItemWidth(-1);
                    
                    char paramBuf[512];
                    memset(paramBuf, 0, sizeof(paramBuf));
#ifdef _MSC_VER
                    strncpy_s(paramBuf, sizeof(paramBuf), param.second.c_str(), _TRUNCATE);
#else
                    strncpy(paramBuf, param.second.c_str(), sizeof(paramBuf) - 1);
#endif
                    std::string label = "##param_" + param.first;
                    
                    if (ImGui::InputText(label.c_str(), paramBuf, sizeof(paramBuf)))
                    {
                        param.second = paramBuf;
                        changed = true;
                    }
                }
            }
            
            if (changed) graphDoc->SetDirty(true);

            // LEGACY RESTORATION: SubGraph Controls & Bindings
            if (nodeType == NodeType::BT_SubGraph)
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0, 1, 1, 1), "SubGraph Configuration");

                std::string path = node->parameters.count("subgraphPath") ? node->parameters.at("subgraphPath") : "";
                char pathBuf[512];
                memset(pathBuf, 0, sizeof(pathBuf));
#ifdef _MSC_VER
                strncpy_s(pathBuf, sizeof(pathBuf), path.c_str(), _TRUNCATE);
#else
                strncpy(pathBuf, path.c_str(), sizeof(pathBuf) - 1);
#endif
                if (ImGui::InputText("Path", pathBuf, sizeof(pathBuf)))
                {
                    node->parameters["subgraphPath"] = pathBuf;
                    graphDoc->SetDirty(true);
                }

                if (ImGui::Button("Open SubGraph (Ctrl+Double Click)"))
                {
                    // Logic to open tab will be handled by TabManager but signaled here
                    SYSTEM_LOG << "[BTNodePropertyPanel] Signaling to open SubGraph: " << pathBuf << "\n";
                }

                if (ImGui::CollapsingHeader("Parameter Bindings", ImGuiTreeNodeFlags_None))
                {
                    ImGui::TextDisabled("Map parent variables to subgraph inputs:");
                    // Simplified binding editor (Phase 39c legacy)
                    for (int i = 0; i < 3; ++i) // Example 3 slots
                    {
                        std::string bindKey = "bind_in_" + std::to_string(i);
                        std::string bindVal = node->parameters.count(bindKey) ? node->parameters.at(bindKey) : "";

                        char bindBuf[64];
                        memset(bindBuf, 0, sizeof(bindBuf));
#ifdef _MSC_VER
                        strncpy_s(bindBuf, sizeof(bindBuf), bindVal.c_str(), _TRUNCATE);
#else
                        strncpy(bindBuf, bindVal.c_str(), sizeof(bindBuf) - 1);
#endif

                        if (ImGui::InputText((std::string("Input ") + std::to_string(i)).c_str(), bindBuf, sizeof(bindBuf)))
                        {
                            node->parameters[bindKey] = bindBuf;
                            graphDoc->SetDirty(true);
                        }
                    }
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Add Parameter", ImVec2(-1, 0)))
            {
                ImGui::OpenPopup("AddBTParamPopup");
            }

            if (ImGui::BeginPopup("AddBTParamPopup"))
            {
                static char newKey[64] = "";
                ImGui::InputText("Parameter Name", newKey, sizeof(newKey));
                if (ImGui::Button("Add") && strlen(newKey) > 0)
                {
                    node->parameters[newKey] = "";
                    graphDoc->SetDirty(true);
                    newKey[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        ImGui::PopStyleVar();
    }

    /**
     * @brief Set the currently selected node ID
     * @param graphId Active graph ID in BTNodeGraphManager
     * @param nodeId Local node ID within the graph
     */
    void SetSelectedNode(int graphId, int nodeId)
    {
        // Avoid noisy logs when selection hasn't changed
        if (m_activeGraphId == graphId && m_selectedNodeId == nodeId)
            return;

        m_activeGraphId = graphId;
        m_selectedNodeId = nodeId;
        SYSTEM_LOG << "[BTNodePropertyPanel] Selected node set: graph=" << graphId << " node=" << nodeId << "\n";
    }

    /**
     * @brief Clear the current selection
     */
    void ClearSelection()
    {
        m_activeGraphId = -1;
        m_selectedNodeId = -1;
        std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
        std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
    }

    /**
     * @brief Check if a node is currently selected
     * @return true if a node is selected
     */
    bool HasSelectedNode() const { return m_selectedNodeId >= 0; }

    // Public access to selected node ID for synchronization
    int m_selectedNodeId = -1;

private:
    int m_activeGraphId = -1;          ///< Current graph ID in BTNodeGraphManager
    char m_nodeNameBuffer[256] = {0}; ///< Buffer for node name editing
    char m_paramBuffer[512] = {0};    ///< Buffer for parameter editing

    // Rendering helpers
    void RenderNodeBasicInfo(const GraphNode* node);
    void RenderNodeParameters(const GraphNode* node);
    void RenderSubGraphControls(GraphNode* node);       ///< Phase 39c: SubGraph node editor
    void RenderSubGraphBindingEditor(GraphNode* node);  ///< Phase 39c Step 4: Parameter binding editor
    void ApplyNodeChanges(GraphNode* node);
};

} // namespace Olympe
