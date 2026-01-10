/*
 * Olympe Blueprint Editor - Node Graph Panel Implementation
 */

#include "NodeGraphPanel.h"
#include "BlueprintEditor.h"
#include "EntityInspectorManager.h"
#include "NodeGraphManager.h"
#include "EnumCatalogManager.h"
#include "CommandSystem.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imnodes/imnodes.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

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
        // Set window title based on editor mode
        const char* windowTitle = (m_EditorMode == BlueprintEditorMode::Runtime) 
            ? "Node Graph Viewer (Runtime)" 
            : "Node Graph Editor (Standalone)";
        ImGui::Begin(windowTitle);

        // Display editor mode badge
        if (m_EditorMode == BlueprintEditorMode::Runtime)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Runtime Mode - Read Only]");
            ImGui::SameLine();
            if (ImGui::SmallButton("Switch to Standalone"))
            {
                m_EditorMode = BlueprintEditorMode::Standalone;
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), "[Standalone Mode - Full Editing]");
            ImGui::SameLine();
            if (ImGui::SmallButton("Switch to Runtime"))
            {
                m_EditorMode = BlueprintEditorMode::Runtime;
            }
        }

        // Handle keyboard shortcuts (only in Standalone mode)
        if (!IsReadOnly())
        {
            HandleKeyboardShortcuts();
        }

        // Show currently selected entity at the top (informational only, doesn't block rendering)
        uint64_t selectedEntity = BlueprintEditor::Get().GetSelectedEntity();
        if (selectedEntity != 0)
        {
            EntityInfo info = EntityInspectorManager::Get().GetEntityInfo(selectedEntity);
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                "Viewing Entity: %s (ID: %llu)", info.name.c_str(), selectedEntity);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), 
                m_EditorMode == BlueprintEditorMode::Runtime 
                    ? "Runtime Blueprint Viewer (no entity context)"
                    : "Standalone Blueprint Editor (no entity context)");
        }
        ImGui::Separator();

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
            
            // Only show creation buttons in Standalone mode
            if (!IsReadOnly())
            {
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
            else
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                    "(Runtime mode: switch to Standalone to create graphs)");
            }
        }

        // Render node edit modal (only in Standalone mode)
        if (!IsReadOnly())
        {
            RenderNodeEditModal();
        }

        ImGui::End();
    }

    void NodeGraphPanel::RenderGraphTabs()
    {
        auto graphIds = NodeGraphManager::Get().GetAllGraphIds();
        int activeGraphId = NodeGraphManager::Get().GetActiveGraphId();

        if (ImGui::BeginTabBar("GraphTabs"))
        {
            for (int graphId : graphIds)
            {
                std::string graphName = NodeGraphManager::Get().GetGraphName(graphId);
                
                // Use SetSelected flag to ensure the active tab is visually selected
                ImGuiTabItemFlags flags = (graphId == activeGraphId) 
                    ? ImGuiTabItemFlags_SetSelected 
                    : ImGuiTabItemFlags_None;

                if (ImGui::BeginTabItem(graphName.c_str(), nullptr, flags))
                {
                    // Only set active if user actually clicked this tab (not the already active one)
                    if (activeGraphId != graphId)
                    {
                        NodeGraphManager::Get().SetActiveGraph(graphId);
                    }
                    ImGui::EndTabItem();
                }
            }

            // Add "+" button for new graph (only in Standalone mode)
            if (!IsReadOnly())
            {
                if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
                {
                    ImGui::OpenPopup("CreateGraphPopup");
                }
            }

            ImGui::EndTabBar();
        }

        // Create graph popup (only in Standalone mode)
        if (!IsReadOnly() && ImGui::BeginPopup("CreateGraphPopup"))
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

        // Ensure canvas has valid size (minimum 1px to render)
        constexpr float MIN_CANVAS_SIZE = 1.0f;
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < MIN_CANVAS_SIZE || canvasSize.y < MIN_CANVAS_SIZE)
        {
            ImGui::Text("Canvas too small to render graph");
            return;
        }

        ImNodes::BeginNodeEditor();

        // Render all nodes
        auto nodes = graph->GetAllNodes();
        for (GraphNode* node : nodes)
        {
            // Set node position BEFORE rendering (ImNodes requirement)
            ImNodes::SetNodeGridSpacePos(node->id, ImVec2(node->posX, node->posY));

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
        }

        // Render all links
        auto links = graph->GetAllLinks();
        for (size_t i = 0; i < links.size(); ++i)
        {
            const GraphLink& link = links[i];
            int linkId = (int)i + 1;  // Link IDs start from 1
            ImNodes::Link(linkId, link.fromNode * 100 + 2, link.toNode * 100 + 1);
        }

        // Handle drag & drop from node palette BEFORE ending editor (only in Standalone mode)
        // This must be done inside the ImNodes context for proper coordinate conversion
        if (!IsReadOnly() && ImNodes::IsEditorHovered())
        {
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE"))
                {
                    std::string nodeTypeData((const char*)payload->Data);
                    
                    // Convert screen space coordinates to grid space
                    ImVec2 mouseScreenPos = ImGui::GetMousePos();
                    ImVec2 canvasPos = ImNodes::ScreenSpaceToGridSpace(mouseScreenPos);
                    
                    // Parse the type and create appropriate node
                    if (nodeTypeData.find("Action:") == 0)
                    {
                        std::string actionType = nodeTypeData.substr(7);
                        int nodeId = graph->CreateNode(NodeType::BT_Action, canvasPos.x, canvasPos.y, actionType);
                        GraphNode* node = graph->GetNode(nodeId);
                        if (node)
                        {
                            node->actionType = actionType;
                            std::cout << "[NodeGraphPanel] Created Action node: " << actionType 
                                      << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                        }
                    }
                    else if (nodeTypeData.find("Condition:") == 0)
                    {
                        std::string conditionType = nodeTypeData.substr(10);
                        int nodeId = graph->CreateNode(NodeType::BT_Condition, canvasPos.x, canvasPos.y, conditionType);
                        GraphNode* node = graph->GetNode(nodeId);
                        if (node)
                        {
                            node->conditionType = conditionType;
                            std::cout << "[NodeGraphPanel] Created Condition node: " << conditionType 
                                      << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                        }
                    }
                    else if (nodeTypeData.find("Decorator:") == 0)
                    {
                        std::string decoratorType = nodeTypeData.substr(10);
                        int nodeId = graph->CreateNode(NodeType::BT_Decorator, canvasPos.x, canvasPos.y, decoratorType);
                        GraphNode* node = graph->GetNode(nodeId);
                        if (node)
                        {
                            node->decoratorType = decoratorType;
                            std::cout << "[NodeGraphPanel] Created Decorator node: " << decoratorType 
                                      << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                        }
                    }
                    else if (nodeTypeData == "Sequence" || nodeTypeData == "Selector")
                    {
                        NodeType type = (nodeTypeData == "Sequence") ? NodeType::BT_Sequence : NodeType::BT_Selector;
                        graph->CreateNode(type, canvasPos.x, canvasPos.y, nodeTypeData);
                        std::cout << "[NodeGraphPanel] Created " << nodeTypeData << " node"
                                  << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }

        ImNodes::EndNodeEditor();

        // Handle link selection
        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0)
        {
            std::vector<int> selectedLinks(numSelectedLinks);
            ImNodes::GetSelectedLinks(selectedLinks.data());
            if (selectedLinks.size() > 0)
                m_SelectedLinkId = selectedLinks[0];
        }

        // Handle Delete key for nodes and links (only in Standalone mode)
        if (!IsReadOnly() && ImGui::IsKeyPressed(ImGuiKey_Delete))
        {
            if (m_SelectedNodeId != -1)
            {
                // Delete selected node
                std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                auto cmd = std::make_unique<DeleteNodeCommand>(graphId, m_SelectedNodeId);
                BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                m_SelectedNodeId = -1;
            }
            else if (m_SelectedLinkId != -1)
            {
                // Delete selected link
                // Note: Link IDs start from 1, array indices start from 0
                auto links = graph->GetAllLinks();
                if (m_SelectedLinkId >= 1 && m_SelectedLinkId <= (int)links.size())
                {
                    const GraphLink& link = links[m_SelectedLinkId - 1];
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<UnlinkNodesCommand>(graphId, link.fromNode, link.toNode);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                    m_SelectedLinkId = -1;
                }
            }
        }

        // Check for double-click on node to open edit modal (or view in Runtime mode)
        int hoveredNodeId = -1;
        if (ImNodes::IsNodeHovered(&hoveredNodeId))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                m_EditingNodeId = hoveredNodeId;
                GraphNode* node = graph->GetNode(hoveredNodeId);
                if (node)
                {
                    strncpy_s(m_NodeNameBuffer, node->name.c_str(), sizeof(m_NodeNameBuffer) - 1);
                    m_NodeNameBuffer[sizeof(m_NodeNameBuffer) - 1] = '\0';
                    m_ShowNodeEditModal = true;
                }
            }
        }
        
        // Right-click context menu on node
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && hoveredNodeId != -1)
        {
            m_SelectedNodeId = hoveredNodeId;
            ImGui::OpenPopup("NodeContextMenu");
        }

        // Handle right-click on canvas for node creation menu (only in Standalone mode)
        if (!IsReadOnly() && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImNodes::IsEditorHovered() && !ImNodes::IsNodeHovered(&hoveredNodeId))
        {
            ImGui::OpenPopup("NodeCreationMenu");
            ImVec2 mousePos = ImGui::GetMousePos();
            m_ContextMenuPosX = mousePos.x;
            m_ContextMenuPosY = mousePos.y;
        }

        // Context menu on node
        if (ImGui::BeginPopup("NodeContextMenu"))
        {
            ImGui::Text("Node: %d", m_SelectedNodeId);
            ImGui::Separator();
            
            // Show View option in Runtime mode, Edit in Standalone mode
            const char* viewEditLabel = IsReadOnly() ? "View" : "Edit";
            const char* viewEditShortcut = IsReadOnly() ? "Double-click" : "Double-click";
            
            if (ImGui::MenuItem(viewEditLabel, viewEditShortcut))
            {
                m_EditingNodeId = m_SelectedNodeId;
                GraphNode* node = graph->GetNode(m_SelectedNodeId);
                if (node)
                {
                    strncpy_s(m_NodeNameBuffer, node->name.c_str(), sizeof(m_NodeNameBuffer) - 1);
                    m_NodeNameBuffer[sizeof(m_NodeNameBuffer) - 1] = '\0';
                    m_ShowNodeEditModal = true;
                }
            }
            
            // Editing operations only in Standalone mode
            if (!IsReadOnly())
            {
                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<DuplicateNodeCommand>(graphId, m_SelectedNodeId);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Delete", "Del"))
                {
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<DeleteNodeCommand>(graphId, m_SelectedNodeId);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                }
            }
            
            ImGui::EndPopup();
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

        // Handle link creation (only in Standalone mode)
        if (!IsReadOnly())
        {
            int startAttr, endAttr;
            if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
            {
                int fromNode = startAttr / 100;
                int toNode = endAttr / 100;
                
                std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                auto cmd = std::make_unique<LinkNodesCommand>(graphId, fromNode, toNode);
                BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
            }
        }

        // Update node positions (only in Standalone mode to prevent accidental moves in Runtime mode)
        if (!IsReadOnly())
        {
            for (GraphNode* node : nodes)
            {
                ImVec2 pos = ImNodes::GetNodeGridSpacePos(node->id);
                node->posX = pos.x;
                node->posY = pos.y;
            }
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

    void NodeGraphPanel::CreateNewNode(const char* nodeType, float screenX, float screenY)
    {
        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
        {
            std::cerr << "[NodeGraphPanel] Cannot create node: No active graph\n";
            return;
        }
        
        // Convert screen coordinates to canvas coordinates
        ImVec2 canvasPos = ImNodes::ScreenSpaceToGridSpace(ImVec2(screenX, screenY));
        
        // Validate coordinates are finite (not NaN or infinity)
        if (!std::isfinite(canvasPos.x) || !std::isfinite(canvasPos.y))
        {
            std::cerr << "[NodeGraphPanel] Invalid coordinates for node creation\n";
            return;
        }
        
        std::cout << "[NodeGraphPanel] Creating " << nodeType << " at canvas pos (" 
                  << canvasPos.x << ", " << canvasPos.y << ")\n";

        NodeType type = StringToNodeType(nodeType);
        int nodeId = graph->CreateNode(type, canvasPos.x, canvasPos.y, nodeType);

        std::cout << "[NodeGraphPanel] Created node " << nodeId << " of type " << nodeType << "\n";
    }

    void NodeGraphPanel::RenderNodeProperties()
    {
        // This would show properties of the selected node
        // Can be integrated into inspector panel
    }

    void NodeGraphPanel::HandleKeyboardShortcuts()
    {
        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
            return;

        ImGuiIO& io = ImGui::GetIO();
        
        // Ctrl+Z: Undo
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift)
        {
            BlueprintEditor::Get().Undo();
        }
        
        // Ctrl+Y or Ctrl+Shift+Z: Redo
        if ((io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) ||
            (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))
        {
            BlueprintEditor::Get().Redo();
        }
        
        // Ctrl+D: Duplicate selected node
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D))
        {
            int selectedNodeCount = ImNodes::NumSelectedNodes();
            if (selectedNodeCount > 0)
            {
                std::vector<int> selectedNodes(selectedNodeCount);
                ImNodes::GetSelectedNodes(selectedNodes.data());
                if (selectedNodes.size() > 0)
                {
                    int nodeId = selectedNodes[0];
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<DuplicateNodeCommand>(graphId, nodeId);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                }
            }
        }
    }

    void NodeGraphPanel::RenderNodeEditModal()
    {
        if (!m_ShowNodeEditModal || m_EditingNodeId < 0)
            return;

        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
        {
            m_ShowNodeEditModal = false;
            return;
        }

        GraphNode* node = graph->GetNode(m_EditingNodeId);
        if (!node)
        {
            m_ShowNodeEditModal = false;
            return;
        }

        // Different title for Runtime vs Standalone mode
        const char* modalTitle = IsReadOnly() ? "View Node" : "Edit Node";
        ImGui::OpenPopup(modalTitle);
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal(modalTitle, &m_ShowNodeEditModal, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // Node name (read-only in Runtime mode)
            if (IsReadOnly())
            {
                ImGui::Text("Name: %s", node->name.c_str());
            }
            else
            {
                if (ImGui::InputText("Name", m_NodeNameBuffer, sizeof(m_NodeNameBuffer)))
                {
                    // Name will be saved on OK
                }
            }
            
            ImGui::Text("Type: %s", NodeTypeToString(node->type));
            ImGui::Text("ID: %d", node->id);
            ImGui::Separator();
            
            std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
            
            // Type-specific parameters
            if (node->type == NodeType::BT_Action)
            {
                // Action type dropdown (read-only in Runtime mode)
                if (IsReadOnly())
                {
                    ImGui::Text("Action Type: %s", node->actionType.c_str());
                }
                else
                {
                    ImGui::Text("Action Type:");
                    auto actionTypes = EnumCatalogManager::Get().GetActionTypes();
                    if (ImGui::BeginCombo("##actiontype", node->actionType.c_str()))
                    {
                        for (const auto& actionType : actionTypes)
                        {
                            bool isSelected = (node->actionType == actionType);
                            if (ImGui::Selectable(actionType.c_str(), isSelected))
                            {
                                std::string oldType = node->actionType;
                                node->actionType = actionType;
                                // Could create EditNodeCommand here
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                
                // Show and edit parameters
                ImGui::Separator();
                ImGui::Text("Parameters:");
                
                // Get parameter definitions from catalog
                const CatalogType* actionDef = EnumCatalogManager::Get().FindActionType(node->actionType);
                if (actionDef)
                {
                    for (const auto& paramDef : actionDef->parameters)
                    {
                        std::string currentValue = node->parameters[paramDef.name];
                        if (currentValue.empty())
                            currentValue = paramDef.defaultValue;
                        
                        if (IsReadOnly())
                        {
                            // Display as read-only text in Runtime mode
                            ImGui::Text("%s: %s", paramDef.name.c_str(), currentValue.c_str());
                        }
                        else
                        {
                            // Allow editing in Standalone mode
                            char buffer[256];
                            strncpy_s(buffer, currentValue.c_str(), sizeof(buffer) - 1);
                            buffer[sizeof(buffer) - 1] = '\0';
                            
                            if (ImGui::InputText(paramDef.name.c_str(), buffer, sizeof(buffer)))
                            {
                                std::string oldValue = node->parameters[paramDef.name];
                                node->parameters[paramDef.name] = buffer;
                                // Could create SetParameterCommand here for undo support
                            }
                        }
                        
                        if (!actionDef->tooltip.empty() && ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("%s", actionDef->tooltip.c_str());
                        }
                    }
                }
            }
            else if (node->type == NodeType::BT_Condition)
            {
                // Condition type dropdown (read-only in Runtime mode)
                if (IsReadOnly())
                {
                    ImGui::Text("Condition Type: %s", node->conditionType.c_str());
                }
                else
                {
                    ImGui::Text("Condition Type:");
                    auto conditionTypes = EnumCatalogManager::Get().GetConditionTypes();
                    if (ImGui::BeginCombo("##conditiontype", node->conditionType.c_str()))
                    {
                        for (const auto& conditionType : conditionTypes)
                        {
                            bool isSelected = (node->conditionType == conditionType);
                            if (ImGui::Selectable(conditionType.c_str(), isSelected))
                            {
                                node->conditionType = conditionType;
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
                
                // Show and edit parameters
                ImGui::Separator();
                ImGui::Text("Parameters:");
                
                const CatalogType* conditionDef = EnumCatalogManager::Get().FindConditionType(node->conditionType);
                if (conditionDef)
                {
                    for (const auto& paramDef : conditionDef->parameters)
                    {
                        std::string currentValue = node->parameters[paramDef.name];
                        if (currentValue.empty())
                            currentValue = paramDef.defaultValue;
                        
                        if (IsReadOnly())
                        {
                            // Display as read-only text in Runtime mode
                            ImGui::Text("%s: %s", paramDef.name.c_str(), currentValue.c_str());
                        }
                        else
                        {
                            // Allow editing in Standalone mode
                            char buffer[256];
                            strncpy_s(buffer, currentValue.c_str(), sizeof(buffer) - 1);
                            buffer[sizeof(buffer) - 1] = '\0';
                            
                            if (ImGui::InputText(paramDef.name.c_str(), buffer, sizeof(buffer)))
                            {
                                node->parameters[paramDef.name] = buffer;
                            }
                        }
                    }
                }
            }
            else if (node->type == NodeType::BT_Decorator)
            {
                // Decorator type dropdown (read-only in Runtime mode)
                if (IsReadOnly())
                {
                    ImGui::Text("Decorator Type: %s", node->decoratorType.c_str());
                }
                else
                {
                    ImGui::Text("Decorator Type:");
                    auto decoratorTypes = EnumCatalogManager::Get().GetDecoratorTypes();
                    if (ImGui::BeginCombo("##decoratortype", node->decoratorType.c_str()))
                    {
                        for (const auto& decoratorType : decoratorTypes)
                        {
                            bool isSelected = (node->decoratorType == decoratorType);
                            if (ImGui::Selectable(decoratorType.c_str(), isSelected))
                            {
                                node->decoratorType = decoratorType;
                            }
                            if (isSelected)
                                ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                }
            }
            
            ImGui::Separator();
            
            // In Runtime mode, only show Close button
            if (IsReadOnly())
            {
                if (ImGui::Button("Close", ImVec2(120, 0)))
                {
                    m_ShowNodeEditModal = false;
                    m_EditingNodeId = -1;
                }
            }
            else
            {
                // In Standalone mode, show OK and Cancel buttons
                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    // Apply name change if different
                    std::string newName(m_NodeNameBuffer);
                    if (newName != node->name)
                    {
                        node->name = newName;
                    }
                    
                    m_ShowNodeEditModal = false;
                    m_EditingNodeId = -1;
                    
                    // Auto-save the graph
                    int graphId = NodeGraphManager::Get().GetActiveGraphId();
                    std::string filename = "graph_" + std::to_string(graphId) + ".json";
                    NodeGraphManager::Get().SaveGraph(graphId, filename);
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_ShowNodeEditModal = false;
                    m_EditingNodeId = -1;
                }
            }
            
            ImGui::EndPopup();
        }
    }
}
