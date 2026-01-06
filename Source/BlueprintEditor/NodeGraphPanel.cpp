/*
 * Olympe Blueprint Editor - Node Graph Panel Implementation
 */

#include "NodeGraphPanel.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "NodeGraphManager.h"
#include "EnumCatalogManager.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include <iostream>
#include <vector>

namespace Olympe
{
    NodeGraphPanel::NodeGraphPanel()
    {
    }

    NodeGraphPanel::~NodeGraphPanel()
    {
    }

    void NodeGraphPanel::Initialize()
    {
        std::cout << "[NodeGraphPanel] Initialized\n";
    }

    void NodeGraphPanel::Shutdown()
    {
        std::cout << "[NodeGraphPanel] Shutdown\n";
    }

    void NodeGraphPanel::Render()
    {
        ImGui::Begin("Node Graph Editor");

        // C) Show currently selected entity at the top
        uint64_t selectedEntity = BlueprintEditor::Get().GetSelectedEntity();
        if (selectedEntity != 0)
        {
            EntityInfo info = EntityInspectorManager::Get().GetEntityInfo(selectedEntity);
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                "Selected Entity: %s (ID: %llu)", info.name.c_str(), selectedEntity);
            ImGui::Separator();
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                "No entity selected. Select an entity to view its Behavior Tree / HFSM.");
            ImGui::Separator();
        }

        // Render graph tabs
        RenderGraphTabs();

        ImGui::Separator();

        // Render the active graph
        NodeGraph* activeGraph = NodeGraphManager::Get().GetActiveGraph();
        if (activeGraph)
        {
            RenderGraph();
        }
        else
        {
            ImGui::Text("No graph open. Create or load a graph to begin.");
            if (ImGui::Button("Create New Behavior Tree"))
            {
                NodeGraphManager::Get().CreateGraph("New Behavior Tree", "BehaviorTree");
            }
            ImGui::SameLine();
            if (ImGui::Button("Create New HFSM"))
            {
                NodeGraphManager::Get().CreateGraph("New HFSM", "HFSM");
            }
        }

        ImGui::End();
    }

    void NodeGraphPanel::RenderGraphTabs()
    {
        auto graphIds = NodeGraphManager::Get().GetAllGraphIds();

        if (ImGui::BeginTabBar("GraphTabs"))
        {
            for (int graphId : graphIds)
            {
                std::string graphName = NodeGraphManager::Get().GetGraphName(graphId);
                bool isActive = (NodeGraphManager::Get().GetActiveGraphId() == graphId);

                if (ImGui::BeginTabItem(graphName.c_str(), nullptr, isActive ? ImGuiTabItemFlags_SetSelected : 0))
                {
                    NodeGraphManager::Get().SetActiveGraph(graphId);
                    ImGui::EndTabItem();
                }
            }

            // Add "+" button for new graph
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                ImGui::OpenPopup("CreateGraphPopup");
            }

            ImGui::EndTabBar();
        }

        // Create graph popup
        if (ImGui::BeginPopup("CreateGraphPopup"))
        {
            if (ImGui::MenuItem("New Behavior Tree"))
            {
                NodeGraphManager::Get().CreateGraph("New Behavior Tree", "BehaviorTree");
            }
            if (ImGui::MenuItem("New HFSM"))
            {
                NodeGraphManager::Get().CreateGraph("New HFSM", "HFSM");
            }
            ImGui::EndPopup();
        }
    }

    void NodeGraphPanel::RenderGraph()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
            return;

        ImNodes::BeginNodeEditor();

        // Render all nodes
        auto nodes = graph->GetAllNodes();
        for (GraphNode* node : nodes)
        {
            ImNodes::BeginNode(node->id);

            // Title bar
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node->name.c_str());
            ImNodes::EndNodeTitleBar();

            // Input attribute (for child connections)
            ImNodes::BeginInputAttribute(node->id * 100 + 1);
            ImGui::Text("In");
            ImNodes::EndInputAttribute();

            // Node content based on type
            ImGui::Text("Type: %s", NodeTypeToString(node->type));
            
            if (node->type == NodeType::BT_Action && !node->actionType.empty())
            {
                ImGui::Text("Action: %s", node->actionType.c_str());
            }
            else if (node->type == NodeType::BT_Condition && !node->conditionType.empty())
            {
                ImGui::Text("Condition: %s", node->conditionType.c_str());
            }
            else if (node->type == NodeType::BT_Decorator && !node->decoratorType.empty())
            {
                ImGui::Text("Decorator: %s", node->decoratorType.c_str());
            }

            // Output attribute (for parent connections)
            ImNodes::BeginOutputAttribute(node->id * 100 + 2);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();

            ImNodes::EndNode();

            // Set node position
            ImNodes::SetNodeGridSpacePos(node->id, ImVec2(node->posX, node->posY));
        }

        // Render all links
        auto links = graph->GetAllLinks();
        for (size_t i = 0; i < links.size(); ++i)
        {
            const GraphLink& link = links[i];
            int linkId = i + 1;  // Link IDs start from 1
            ImNodes::Link(linkId, link.fromNode * 100 + 2, link.toNode * 100 + 1);
        }

        ImNodes::EndNodeEditor();

        // Handle node creation (right-click on canvas)
        if (ImGui::IsMouseReleased(1) && ImNodes::IsEditorHovered())
        {
            ImGui::OpenPopup("NodeCreationMenu");
            ImVec2 mousePos = ImGui::GetMousePos();
            m_ContextMenuPosX = mousePos.x;
            m_ContextMenuPosY = mousePos.y;
        }

        RenderContextMenu();

        // Handle node selection
        int selectedNodeCount = ImNodes::NumSelectedNodes();
        if (selectedNodeCount > 0)
        {
            std::vector<int> selectedNodes(selectedNodeCount);
            ImNodes::GetSelectedNodes(selectedNodes.data());
            if (selectedNodes.size() > 0)
                m_SelectedNodeId = selectedNodes[0];
        }

        // Handle link creation
        int startAttr, endAttr;
        if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
        {
            int fromNode = startAttr / 100;
            int toNode = endAttr / 100;
            graph->LinkNodes(fromNode, toNode);
        }

        // Update node positions
        for (GraphNode* node : nodes)
        {
            ImVec2 pos = ImNodes::GetNodeGridSpacePos(node->id);
            node->posX = pos.x;
            node->posY = pos.y;
        }
    }

    void NodeGraphPanel::RenderContextMenu()
    {
        if (ImGui::BeginPopup("NodeCreationMenu"))
        {
            ImGui::Text("Create Node");
            ImGui::Separator();

            if (ImGui::BeginMenu("Composite"))
            {
                if (ImGui::MenuItem("Sequence"))
                    CreateNewNode("Sequence", m_ContextMenuPosX, m_ContextMenuPosY);
                if (ImGui::MenuItem("Selector"))
                    CreateNewNode("Selector", m_ContextMenuPosX, m_ContextMenuPosY);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Action"))
            {
                auto actionTypes = EnumCatalogManager::Get().GetActionTypes();
                for (const auto& actionType : actionTypes)
                {
                    if (ImGui::MenuItem(actionType.c_str()))
                    {
                        CreateNewNode("Action", m_ContextMenuPosX, m_ContextMenuPosY);
                        // Set action type on the created node
                        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
                        if (graph)
                        {
                            auto nodes = graph->GetAllNodes();
                            if (!nodes.empty())
                            {
                                nodes.back()->actionType = actionType;
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Condition"))
            {
                auto conditionTypes = EnumCatalogManager::Get().GetConditionTypes();
                for (const auto& conditionType : conditionTypes)
                {
                    if (ImGui::MenuItem(conditionType.c_str()))
                    {
                        CreateNewNode("Condition", m_ContextMenuPosX, m_ContextMenuPosY);
                        // Set condition type on the created node
                        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
                        if (graph)
                        {
                            auto nodes = graph->GetAllNodes();
                            if (!nodes.empty())
                            {
                                nodes.back()->conditionType = conditionType;
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Decorator"))
            {
                auto decoratorTypes = EnumCatalogManager::Get().GetDecoratorTypes();
                for (const auto& decoratorType : decoratorTypes)
                {
                    if (ImGui::MenuItem(decoratorType.c_str()))
                    {
                        CreateNewNode("Decorator", m_ContextMenuPosX, m_ContextMenuPosY);
                        // Set decorator type on the created node
                        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
                        if (graph)
                        {
                            auto nodes = graph->GetAllNodes();
                            if (!nodes.empty())
                            {
                                nodes.back()->decoratorType = decoratorType;
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }
    }

    void NodeGraphPanel::CreateNewNode(const char* nodeType, float x, float y)
    {
        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
            return;

        NodeType type = StringToNodeType(nodeType);
        int nodeId = graph->CreateNode(type, x, y, nodeType);

        std::cout << "[NodeGraphPanel] Created node " << nodeId << " of type " << nodeType << "\n";
    }

    void NodeGraphPanel::RenderNodeProperties()
    {
        // This would show properties of the selected node
        // Can be integrated into inspector panel
    }
}
