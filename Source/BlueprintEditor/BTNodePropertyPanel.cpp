/**
 * @file BTNodePropertyPanel.cpp
 * @brief Property editor panel implementation for BehaviorTree nodes
 * @author Olympe Engine
 * @date 2026-04-08
 */

#include "BTNodePropertyPanel.h"
#include "BTNodeGraphManager.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <cstring>

namespace Olympe {

BTNodePropertyPanel::BTNodePropertyPanel()
    : m_activeGraphId(-1)
    , m_selectedNodeId(-1)
{
    std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
    std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
}

void BTNodePropertyPanel::Initialize()
{
    // Nothing to initialize currently
}

void BTNodePropertyPanel::SetSelectedNode(int graphId, int nodeId)
{
    m_activeGraphId = graphId;
    m_selectedNodeId = nodeId;

    if (graphId >= 0 && nodeId >= 0)
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
        if (graph)
        {
            const GraphNode* node = graph->GetNode(nodeId);
            if (node)
            {
                strncpy_s(m_nodeNameBuffer, sizeof(m_nodeNameBuffer), node->name.c_str(), _TRUNCATE);
            }
        }
    }
}

void BTNodePropertyPanel::ClearSelection()
{
    m_activeGraphId = -1;
    m_selectedNodeId = -1;
    std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
    std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
}

void BTNodePropertyPanel::Render()
{
    if (m_activeGraphId < 0 || m_selectedNodeId < 0)
    {
        ImGui::TextDisabled("No node selected");
        return;
    }

    NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_activeGraphId);
    if (!graph)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Graph not found");
        ClearSelection();
        return;
    }

    GraphNode* node = graph->GetNode(m_selectedNodeId);
    if (!node)
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: Node not found");
        ClearSelection();
        return;
    }

    ImGui::Text("Node ID: %d", node->id);
    ImGui::Separator();

    RenderNodeBasicInfo(node);
    ImGui::Separator();

    RenderNodeParameters(node);
}

void BTNodePropertyPanel::RenderNodeBasicInfo(const GraphNode* node)
{
    ImGui::Text("Node Type: %s", NodeTypeToString(node->type));
    
    // Node name editing
    if (ImGui::InputText("##NodeName", m_nodeNameBuffer, sizeof(m_nodeNameBuffer)))
    {
        // Will apply on next ApplyNodeChanges call
    }
    
    if (ImGui::IsItemDeactivatedAfterEdit())
    {
        NodeGraph* graph = NodeGraphManager::Get().GetGraph(m_activeGraphId);
        if (graph)
        {
            GraphNode* mutableNode = graph->GetNode(m_selectedNodeId);
            if (mutableNode)
            {
                mutableNode->name = m_nodeNameBuffer;
                SYSTEM_LOG << "[BTNodePropertyPanel] Updated node name to: " << m_nodeNameBuffer << "\n";
            }
        }
    }

    // Position info (read-only for now)
    ImGui::Text("Position: (%.1f, %.1f)", node->posX, node->posY);
}

void BTNodePropertyPanel::RenderNodeParameters(const GraphNode* node)
{
    ImGui::PushID(node->id);

    // For BT nodes, parameters are stored in a generic map
    if (node->parameters.empty())
    {
        ImGui::TextDisabled("No parameters");
        ImGui::PopID();
        return;
    }

    ImGui::TextUnformatted("Parameters:");
    for (const auto& param : node->parameters)
    {
        ImGui::BulletText("%s: %s", param.first.c_str(), param.second.c_str());
    }

    ImGui::PopID();
}

void BTNodePropertyPanel::ApplyNodeChanges(GraphNode* node)
{
    if (!node)
        return;

    node->name = m_nodeNameBuffer;
    SYSTEM_LOG << "[BTNodePropertyPanel] Applied changes to node: " << node->id << "\n";
}

} // namespace Olympe
