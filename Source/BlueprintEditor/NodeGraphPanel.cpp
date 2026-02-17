/*
 * Olympe Blueprint Editor - Node Graph Panel Implementation
 */

#include "NodeGraphPanel.h"
#include "BlueprintEditor.h"
#include "EditorContext.h"
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

// Use Blueprint namespace for command classes
using namespace Olympe::Blueprint;

namespace
{
    // UID generation constants for ImNodes
    // These ensure unique IDs across multiple open graphs
    constexpr int GRAPH_ID_MULTIPLIER = 10000;     // Multiplier for graph ID in node UID calculation
    constexpr int ATTR_ID_MULTIPLIER = 100;        // Multiplier for node UID in attribute UID calculation
    constexpr int LINK_ID_MULTIPLIER = 100000;     // Multiplier for graph ID in link UID calculation

    // Helper function to convert screen space coordinates to grid space coordinates
    // Screen space: origin at upper-left corner of the window
    // Grid space: origin at upper-left corner of the node editor, adjusted by panning
    ImVec2 ScreenSpaceToGridSpace(const ImVec2& screenPos)
    {
        // Get the editor's screen space position
        ImVec2 editorPos = ImGui::GetCursorScreenPos();
        
        // Get the current panning offset
        ImVec2 panning = ImNodes::EditorContextGetPanning();
        
        // Convert: subtract editor position to get editor space, then subtract panning to get grid space
        return ImVec2(screenPos.x - editorPos.x - panning.x, 
                      screenPos.y - editorPos.y - panning.y);
    }
}

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

        // Handle keyboard shortcuts
        HandleKeyboardShortcuts();

        // Show currently selected entity at the top (informational only, doesn't block rendering)
        uint64_t selectedEntity = BlueprintEditor::Get().GetSelectedEntity();
        if (selectedEntity != 0)
        {
            EntityInfo info = EntityInspectorManager::Get().GetEntityInfo(selectedEntity);
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), 
                "Editing for Entity: %s (ID: %llu)", info.name.c_str(), selectedEntity);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.9f, 1.0f), 
                "Editing BehaviorTree Asset (no entity context)");
        }
        ImGui::Separator();
        
        // Toolbar with Save/Save As buttons
        NodeGraph* activeGraph = NodeGraphManager::Get().GetActiveGraph();
        if (activeGraph)
        {
            // Save button
            bool canSave = activeGraph->HasFilepath();
            if (!canSave)
                ImGui::BeginDisabled();
                
            if (ImGui::Button("Save"))
            {
                // Validate before saving
                std::string validationError;
                if (!activeGraph->ValidateGraph(validationError))
                {
                    // Show validation error popup
                    ImGui::OpenPopup("ValidationError");
                }
                else
                {
                    int graphId = NodeGraphManager::Get().GetActiveGraphId();
                    const std::string& filepath = activeGraph->GetFilepath();
                    if (NodeGraphManager::Get().SaveGraph(graphId, filepath))
                    {
                        std::cout << "[NodeGraphPanel] Saved graph to: " << filepath << std::endl;
                    }
                    else
                    {
                        std::cout << "[NodeGraphPanel] Failed to save graph!" << std::endl;
                    }
                }
            }
            
            if (!canSave)
                ImGui::EndDisabled();
                
            if (!canSave && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip("No filepath set. Use 'Save As...' first.");
            }
            
            ImGui::SameLine();
            
            // Save As button
            if (ImGui::Button("Save As..."))
            {
                // TODO: Open file dialog to select save location
                // For now, show popup to enter filename
                ImGui::OpenPopup("SaveAsPopup");
            }
            
            // Show dirty indicator
            ImGui::SameLine();
            if (activeGraph->IsDirty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "*");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Unsaved changes");
                }
            }
            
            // Save As popup (simple text input for now)
            static bool saveAsPopupOpen = false;
            static char filepathBuffer[512] = "";
            
            if (ImGui::BeginPopup("SaveAsPopup"))
            {
                // Clear buffer when popup first opens
                if (!saveAsPopupOpen)
                {
                    filepathBuffer[0] = '\0';
                    saveAsPopupOpen = true;
                }
                
                ImGui::Text("Save graph as:");
                ImGui::InputText("Filepath", filepathBuffer, sizeof(filepathBuffer));
                
                if (ImGui::Button("Save", ImVec2(120, 0)))
                {
                    std::string filepath(filepathBuffer);
                    if (!filepath.empty())
                    {
                        // Validate before saving
                        std::string validationError;
                        if (!activeGraph->ValidateGraph(validationError))
                        {
                            // Show validation error
                            saveAsPopupOpen = false;
                            ImGui::CloseCurrentPopup();
                            ImGui::OpenPopup("ValidationError");
                        }
                        else
                        {
                            // Ensure .json extension (check that it ends with .json)
                            if (filepath.size() < 5 || filepath.substr(filepath.size() - 5) != ".json")
                                filepath += ".json";
                                
                            int graphId = NodeGraphManager::Get().GetActiveGraphId();
                            if (NodeGraphManager::Get().SaveGraph(graphId, filepath))
                            {
                                std::cout << "[NodeGraphPanel] Saved graph as: " << filepath << std::endl;
                                saveAsPopupOpen = false;
                                ImGui::CloseCurrentPopup();
                            }
                            else
                            {
                                std::cout << "[NodeGraphPanel] Failed to save graph!" << std::endl;
                            }
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    saveAsPopupOpen = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            else
            {
                saveAsPopupOpen = false;
            }
            
            // Validation error popup
            if (ImGui::BeginPopupModal("ValidationError", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Cannot save: Graph validation failed!");
                ImGui::Separator();
                
                std::string validationError;
                if (!activeGraph->ValidateGraph(validationError))
                {
                    ImGui::TextWrapped("%s", validationError.c_str());
                }
                
                ImGui::Separator();
                if (ImGui::Button("OK", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            
            ImGui::Separator();
        }

        // Render graph tabs
        RenderGraphTabs();

        ImGui::Separator();

        // Render the active graph
        activeGraph = NodeGraphManager::Get().GetActiveGraph();
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

        // Render node edit modal
        RenderNodeEditModal();

        ImGui::End();
    }

    void NodeGraphPanel::RenderGraphTabs()
    {
        auto graphIds = NodeGraphManager::Get().GetAllGraphIds();
        int currentActiveId = NodeGraphManager::Get().GetActiveGraphId();
        
        // Track which graph was requested to close
        // Using static but ensuring cleanup to prevent issues with multiple rapid closes
        static int graphToClose = -1;
        static bool confirmationOpen = false;

        if (ImGui::BeginTabBar("GraphTabs"))
        {
            for (int graphId : graphIds)
            {
                std::string graphName = NodeGraphManager::Get().GetGraphName(graphId);
                
                // Add dirty indicator to tab name
                NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphId);
                if (graph && graph->IsDirty())
                    graphName += " *";

                // Only set ImGuiTabItemFlags_SetSelected if this is the active graph
                // This ensures the tab is selected visually without forcing re-selection each frame
                ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
                if (graphId == currentActiveId)
                {
                    flags = ImGuiTabItemFlags_SetSelected;
                }
                
                // Enable close button for tabs
                bool tabOpen = true;
                if (ImGui::BeginTabItem(graphName.c_str(), &tabOpen, flags))
                {
                    // Only change active graph if user clicked this tab (and it's not already active)
                    // BeginTabItem returns true when the tab content should be shown
                    if (currentActiveId != graphId)
                    {
                        NodeGraphManager::Get().SetActiveGraph(graphId);
                    }
                    ImGui::EndTabItem();
                }
                
                // If tab was closed (X button clicked)
                if (!tabOpen)
                {
                    // Only process if no confirmation dialog is currently open
                    if (!confirmationOpen)
                    {
                        // Check if graph has unsaved changes
                        if (graph && graph->IsDirty())
                        {
                            graphToClose = graphId;
                            confirmationOpen = true;
                            ImGui::OpenPopup("ConfirmCloseUnsaved");
                        }
                        else
                        {
                            // Close immediately if no unsaved changes
                            NodeGraphManager::Get().CloseGraph(graphId);
                        }
                    }
                }
            }

            // Add "+" button for new graph
            if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            {
                ImGui::OpenPopup("CreateGraphPopup");
            }

            ImGui::EndTabBar();
        }
        
        // Confirmation popup for closing unsaved graph
        if (ImGui::BeginPopupModal("ConfirmCloseUnsaved", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Warning: Unsaved Changes!");
            ImGui::Separator();
            
            std::string graphName = NodeGraphManager::Get().GetGraphName(graphToClose);
            ImGui::TextWrapped("The graph '%s' has unsaved changes.", graphName.c_str());
            ImGui::TextWrapped("Do you want to save before closing?");
            
            ImGui::Separator();
            
            // Save and Close button
            if (ImGui::Button("Save and Close", ImVec2(120, 0)))
            {
                NodeGraph* graph = NodeGraphManager::Get().GetGraph(graphToClose);
                if (graph && graph->HasFilepath())
                {
                    // Validate before saving
                    std::string validationError;
                    if (!graph->ValidateGraph(validationError))
                    {
                        // Show validation error
                        ImGui::CloseCurrentPopup();
                        ImGui::OpenPopup("ValidationError");
                    }
                    else
                    {
                        // Save and close
                        if (NodeGraphManager::Get().SaveGraph(graphToClose, graph->GetFilepath()))
                        {
                            NodeGraphManager::Get().CloseGraph(graphToClose);
                            graphToClose = -1;
                            confirmationOpen = false;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                else
                {
                    // No filepath - need Save As
                    confirmationOpen = false;
                    ImGui::CloseCurrentPopup();
                    ImGui::OpenPopup("SaveAsPopup");
                }
            }
            
            ImGui::SameLine();
            
            // Close without saving button
            if (ImGui::Button("Close Without Saving", ImVec2(150, 0)))
            {
                NodeGraphManager::Get().CloseGraph(graphToClose);
                graphToClose = -1;
                confirmationOpen = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            
            // Cancel button
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                graphToClose = -1;
                confirmationOpen = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        else
        {
            // Popup closed without action - reset state
            if (confirmationOpen && graphToClose >= 0)
            {
                confirmationOpen = false;
                graphToClose = -1;
            }
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

        // Get the Graph ID for creating unique UIDs
        int graphID = NodeGraphManager::Get().GetActiveGraphId();
        if (graphID < 0)
        {
            std::cerr << "[NodeGraphPanel] Invalid graph ID" << std::endl;
            return;
        }

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
            // Generate a global unique UID for ImNodes
            // Format: graphID * GRAPH_ID_MULTIPLIER + nodeID
            // This ensures no node from different graphs has the same UID
            int globalNodeUID = (graphID * GRAPH_ID_MULTIPLIER) + node->id;

            // Set node position BEFORE rendering (ImNodes requirement)
            ImNodes::SetNodeGridSpacePos(globalNodeUID, ImVec2(node->posX, node->posY));

            ImNodes::BeginNode(globalNodeUID);

            // Title bar
            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted(node->name.c_str());
            ImNodes::EndNodeTitleBar();

            // Input attribute with UID based on globalNodeUID
            int inputAttrUID = globalNodeUID * ATTR_ID_MULTIPLIER + 1;
            ImNodes::BeginInputAttribute(inputAttrUID);
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

            // Output attribute with UID based on globalNodeUID
            int outputAttrUID = globalNodeUID * ATTR_ID_MULTIPLIER + 2;
            ImNodes::BeginOutputAttribute(outputAttrUID);
            ImGui::Text("Out");
            ImNodes::EndOutputAttribute();

            ImNodes::EndNode();
        }

        // Render all links with global UIDs
        auto links = graph->GetAllLinks();
        for (size_t i = 0; i < links.size(); ++i)
        {
            const GraphLink& link = links[i];
            
            // Generate global UIDs for the attributes
            int fromNodeUID = (graphID * GRAPH_ID_MULTIPLIER) + link.fromNode;
            int toNodeUID = (graphID * GRAPH_ID_MULTIPLIER) + link.toNode;
            
            int fromAttrUID = fromNodeUID * ATTR_ID_MULTIPLIER + 2;  // Output attribute
            int toAttrUID = toNodeUID * ATTR_ID_MULTIPLIER + 1;      // Input attribute
            
            // Link ID must also be unique globally
            int globalLinkUID = (graphID * LINK_ID_MULTIPLIER) + (int)i + 1;
            
            ImNodes::Link(globalLinkUID, fromAttrUID, toAttrUID);
        }

        ImNodes::EndNodeEditor();

        // Handle node interactions with UID mapping
        HandleNodeInteractions(graphID);

        // Handle link selection
        int numSelectedLinks = ImNodes::NumSelectedLinks();
        if (numSelectedLinks > 0)
        {
            std::vector<int> selectedLinks(numSelectedLinks);
            ImNodes::GetSelectedLinks(selectedLinks.data());
            if (selectedLinks.size() > 0)
                m_SelectedLinkId = selectedLinks[0];
        }

        // Handle Delete key for nodes and links (only if canDelete)
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && EditorContext::Get().CanDelete())
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
                // Extract the link index from the global link UID
                int linkIndex = (m_SelectedLinkId - (graphID * LINK_ID_MULTIPLIER)) - 1;
                
                auto links = graph->GetAllLinks();
                if (linkIndex >= 0 && linkIndex < (int)links.size())
                {
                    const GraphLink& link = links[linkIndex];
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<UnlinkNodesCommand>(graphId, link.fromNode, link.toNode);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                    m_SelectedLinkId = -1;
                }
            }
        }

        // Check for double-click on node to open edit modal
        int hoveredNodeUID = -1;
        if (ImNodes::IsNodeHovered(&hoveredNodeUID))
        {
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                // Convert global UID to local node ID
                int localNodeId = GlobalUIDToLocalNodeID(hoveredNodeUID, graphID);
                m_EditingNodeId = localNodeId;
                GraphNode* node = graph->GetNode(localNodeId);
                if (node)
                {
                    strncpy_s(m_NodeNameBuffer, node->name.c_str(), sizeof(m_NodeNameBuffer) - 1);
                    m_NodeNameBuffer[sizeof(m_NodeNameBuffer) - 1] = '\0';
                    m_ShowNodeEditModal = true;
                }
            }
        }

        // Right-click context menu on node
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && hoveredNodeUID != -1)
        {
            // Convert global UID to local node ID
            m_SelectedNodeId = GlobalUIDToLocalNodeID(hoveredNodeUID, graphID);
            ImGui::OpenPopup("NodeContextMenu");
        }

        // Handle right-click on canvas for node creation menu (only if canCreate)
        if (EditorContext::Get().CanCreate() &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            ImNodes::IsEditorHovered() &&
            !ImNodes::IsNodeHovered(&hoveredNodeUID))
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

            // Edit is always available for viewing
            if (ImGui::MenuItem("Edit", "Double-click"))
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

            // Duplicate and Delete only shown if allowed
            if (EditorContext::Get().CanEdit() && EditorContext::Get().CanCreate())
            {
                if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
                {
                    std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
                    auto cmd = std::make_unique<DuplicateNodeCommand>(graphId, m_SelectedNodeId);
                    BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
                }
            }

            ImGui::Separator();

            if (EditorContext::Get().CanDelete())
            {
                if (EditorContext::Get().CanDelete())
                {
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

            // Handle drag & drop from node palette
            if (ImGui::BeginDragDropTarget())
            {
                const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_TYPE");

                // Validate payload safety
                if (payload && payload->Data && payload->DataSize > 0)
                {
                    // Copy payload data to local string with bounds checking
                    size_t maxSize = 256;  // Reasonable max for node type strings
                    size_t dataSize = (payload->DataSize < maxSize) ? payload->DataSize : maxSize;

                    std::string nodeTypeData;
                    nodeTypeData.resize(dataSize);
                    std::memcpy(&nodeTypeData[0], payload->Data, dataSize);

                    // Ensure NUL-termination
                    if (nodeTypeData.find('\0') == std::string::npos)
                    {
                        // Truncate at first non-printable or add terminator
                        size_t validLen = 0;
                        for (size_t i = 0; i < nodeTypeData.size(); ++i)
                        {
                            if (nodeTypeData[i] == '\0' || nodeTypeData[i] < 32)
                                break;
                            validLen = i + 1;
                        }
                        nodeTypeData.resize(validLen);
                    }

                    // Remove any trailing null bytes
                    while (!nodeTypeData.empty() && nodeTypeData.back() == '\0')
                        nodeTypeData.pop_back();

                    // Convert screen space coordinates to grid space
                    ImVec2 mouseScreenPos = ImGui::GetMousePos();
                    ImVec2 canvasPos = ScreenSpaceToGridSpace(mouseScreenPos);

                    bool validNode = false;

                    // Parse the type and create appropriate node
                    if (nodeTypeData.find("Action:") == 0)
                    {
                        std::string actionType = nodeTypeData.substr(7);

                        // Validate action type exists in catalog
                        if (EnumCatalogManager::Get().IsValidActionType(actionType))
                        {
                            int nodeId = graph->CreateNode(NodeType::BT_Action, canvasPos.x, canvasPos.y, actionType);
                            GraphNode* node = graph->GetNode(nodeId);
                            if (node)
                            {
                                node->actionType = actionType;
                                validNode = true;
                                std::cout << "[NodeGraphPanel] Created Action node: " << actionType
                                    << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                            }
                        }
                        else
                        {
                            std::cerr << "[NodeGraphPanel] ERROR: Invalid ActionType: " << actionType << "\n";
                            ImGui::SetTooltip("Invalid ActionType: %s", actionType.c_str());
                        }
                    }
                    else if (nodeTypeData.find("Condition:") == 0)
                    {
                        std::string conditionType = nodeTypeData.substr(10);

                        // Validate condition type exists in catalog
                        if (EnumCatalogManager::Get().IsValidConditionType(conditionType))
                        {
                            int nodeId = graph->CreateNode(NodeType::BT_Condition, canvasPos.x, canvasPos.y, conditionType);
                            GraphNode* node = graph->GetNode(nodeId);
                            if (node)
                            {
                                node->conditionType = conditionType;
                                validNode = true;
                                std::cout << "[NodeGraphPanel] Created Condition node: " << conditionType
                                    << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                            }
                        }
                        else
                        {
                            std::cerr << "[NodeGraphPanel] ERROR: Invalid ConditionType: " << conditionType << "\n";
                            ImGui::SetTooltip("Invalid ConditionType: %s", conditionType.c_str());
                        }
                    }
                    else if (nodeTypeData.find("Decorator:") == 0)
                    {
                        std::string decoratorType = nodeTypeData.substr(10);

                        // Validate decorator type exists in catalog
                        if (EnumCatalogManager::Get().IsValidDecoratorType(decoratorType))
                        {
                            int nodeId = graph->CreateNode(NodeType::BT_Decorator, canvasPos.x, canvasPos.y, decoratorType);
                            GraphNode* node = graph->GetNode(nodeId);
                            if (node)
                            {
                                node->decoratorType = decoratorType;
                                validNode = true;
                                std::cout << "[NodeGraphPanel] Created Decorator node: " << decoratorType
                                    << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                            }
                        }
                        else
                        {
                            std::cerr << "[NodeGraphPanel] ERROR: Invalid DecoratorType: " << decoratorType << "\n";
                            ImGui::SetTooltip("Invalid DecoratorType: %s", decoratorType.c_str());
                        }
                    }
                    else if (nodeTypeData == "Sequence" || nodeTypeData == "Selector")
                    {
                        NodeType type = (nodeTypeData == "Sequence") ? NodeType::BT_Sequence : NodeType::BT_Selector;
                        int nodeId = graph->CreateNode(type, canvasPos.x, canvasPos.y, nodeTypeData);
                        if (nodeId > 0)
                        {
                            validNode = true;
                            std::cout << "[NodeGraphPanel] Created " << nodeTypeData << " node"
                                << " at canvas pos (" << canvasPos.x << ", " << canvasPos.y << ")\n";
                        }
                    }
                    else
                    {
                        std::cerr << "[NodeGraphPanel] ERROR: Unknown node type: " << nodeTypeData << "\n";
                        ImGui::SetTooltip("Unknown node type: %s", nodeTypeData.c_str());
                    }

                    if (!validNode)
                    {
                        std::cerr << "[NodeGraphPanel] Failed to create node from DnD payload\n";
                    }
                }
                else
                {
                    std::cerr << "[NodeGraphPanel] Invalid DnD payload received (null or empty)\n";
                }

                ImGui::EndDragDropTarget();
            }

            // Update node positions using global UIDs
            for (GraphNode* node : nodes)
            {
                int globalNodeUID = (graphID * GRAPH_ID_MULTIPLIER) + node->id;
                ImVec2 pos = ImNodes::GetNodeGridSpacePos(globalNodeUID);
                
                // Check if position changed
                if (node->posX != pos.x || node->posY != pos.y)
                {
                    node->posX = pos.x;
                    node->posY = pos.y;
                    
                    // Mark graph as dirty when node is moved
                    if (graph)
                        graph->MarkDirty();
                }
            }
        }
    }

    void NodeGraphPanel::HandleNodeInteractions(int graphID)
    {
        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
            return;

        // Handle node selection
        int numSelected = ImNodes::NumSelectedNodes();
        if (numSelected > 0)
        {
            std::vector<int> selectedUIDs(numSelected);
            ImNodes::GetSelectedNodes(selectedUIDs.data());
            
            // Convert the first global UID to local Node ID
            if (!selectedUIDs.empty())
            {
                int selectedLocalNodeID = GlobalUIDToLocalNodeID(selectedUIDs[0], graphID);
                m_SelectedNodeId = selectedLocalNodeID;
            }
        }

        // Handle link creation (only if canLink)
        int startAttrUID, endAttrUID;
        if (EditorContext::Get().CanLink() && ImNodes::IsLinkCreated(&startAttrUID, &endAttrUID))
        {
            // Extract the global UIDs of nodes
            int startNodeGlobalUID = startAttrUID / ATTR_ID_MULTIPLIER;
            int endNodeGlobalUID = endAttrUID / ATTR_ID_MULTIPLIER;
            
            // Convert to local IDs
            int startNodeLocalID = GlobalUIDToLocalNodeID(startNodeGlobalUID, graphID);
            int endNodeLocalID = GlobalUIDToLocalNodeID(endNodeGlobalUID, graphID);
            
            // Create the link with local IDs
            std::string graphId = std::to_string(graphID);
            auto cmd = std::make_unique<LinkNodesCommand>(graphId, startNodeLocalID, endNodeLocalID);
            BlueprintEditor::Get().GetCommandStack()->ExecuteCommand(std::move(cmd));
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
        ImVec2 canvasPos = ScreenSpaceToGridSpace(ImVec2(screenX, screenY));
        
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
                    auto cmd = std::make_unique<Olympe::Blueprint::DuplicateNodeCommand>(graphId, nodeId);
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

        ImGui::OpenPopup("Edit Node");
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        
        if (ImGui::BeginPopupModal("Edit Node", &m_ShowNodeEditModal, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // Node name
            if (ImGui::InputText("Name", m_NodeNameBuffer, sizeof(m_NodeNameBuffer)))
            {
                // Name will be saved on OK
            }
            
            ImGui::Text("Type: %s", NodeTypeToString(node->type));
            ImGui::Text("ID: %d", node->id);
            ImGui::Separator();
            
            std::string graphId = std::to_string(NodeGraphManager::Get().GetActiveGraphId());
            
            // Type-specific parameters
            if (node->type == NodeType::BT_Action)
            {
                // Action type dropdown
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
                        
                        char buffer[256];
                        strncpy_s(buffer, currentValue.c_str(), sizeof(buffer) - 1);
                        buffer[sizeof(buffer) - 1] = '\0';
                        
                        if (ImGui::InputText(paramDef.name.c_str(), buffer, sizeof(buffer)))
                        {
                            std::string oldValue = node->parameters[paramDef.name];
                            node->parameters[paramDef.name] = buffer;
                            // Could create SetParameterCommand here for undo support
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
                // Condition type dropdown
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
            else if (node->type == NodeType::BT_Decorator)
            {
                // Decorator type dropdown
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
            
            ImGui::Separator();
            
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                // Apply name change if different
                std::string newName(m_NodeNameBuffer);
                if (newName != node->name)
                {
                    node->name = newName;
                }
                
                // Mark graph as dirty since node was edited
                if (graph)
                    graph->MarkDirty();
                
                m_ShowNodeEditModal = false;
                m_EditingNodeId = -1;
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowNodeEditModal = false;
                m_EditingNodeId = -1;
            }
            
            ImGui::EndPopup();
        }
    }
}
