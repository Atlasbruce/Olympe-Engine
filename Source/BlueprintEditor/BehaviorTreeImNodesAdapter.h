/**
 * @file BehaviorTreeImNodesAdapter.h
 * @brief Minimal ImNodes integration for BehaviorTree rendering (Phase 50.3)
 * @author Olympe Engine
 * @date 2026-04-16
 * 
 * Provides direct ImNodes rendering for BehaviorTree graphs without NodeGraphPanel overhead.
 * Pragmatic minimal implementation: nodes, connections, pan/zoom/select.
 */

#pragma once

#include "../third_party/imnodes/imnodes.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include "BTNodeGraphManager.h"
#include "NodeStyleRegistry.h"
#include "../NodeGraphCore/NodeGraphManager.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <functional>

namespace Olympe
{
    // Forward declarations
    namespace NodeGraphTypes {
        class GraphDocument;
    }

    class BehaviorTreeImNodesAdapter
    {
    public:
        BehaviorTreeImNodesAdapter()
            : m_context(nullptr), m_canvasOffset(0, 0)
        {
            // Create ImNodes context
            m_context = ImNodes::CreateContext();
            if (m_context)
            {
                ImNodes::SetCurrentContext(m_context);
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] ImNodes context created\n";
            }
        }

        ~BehaviorTreeImNodesAdapter()
        {
            if (m_context)
            {
                ImNodes::SetCurrentContext(m_context);
                ImNodes::DestroyContext(m_context);
                m_context = nullptr;
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] ImNodes context destroyed\n";
            }
        }

        /**
         * @brief Initialize adapter with graph document
         * @param graphId ID in NodeGraphManager
         */
        void Initialize(int graphId)
        {
            m_graphId = graphId;
            if (m_context)
            {
                ImNodes::SetCurrentContext(m_context);
            }

            // Phase 61 FIX: Route to CORRECT NodeGraphManager (NodeGraph namespace)
            // NOT the Olympe::NodeGraphManager from BTNodeGraphManager.h
            // BehaviorTreeRenderer successfully uses NodeGraph::NodeGraphManager::Get()
            // RenderNodes must query the same manager that stores the graphs
            if (graphId > 0)
            {
                // Convert int to GraphId struct expected by correct manager
                NodeGraph::NodeGraphManager::Get().SetActiveGraph(
                    NodeGraphTypes::GraphId{static_cast<uint32_t>(graphId)}
                );
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Set active graph (NodeGraph manager): " 
                           << graphId << "\n";
            }

            SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Initialized with graph ID: " << graphId << "\n";
        }

        /**
         * @brief Main render call - draws entire BehaviorTree with ImNodes
         * @param minimapRenderCallback Optional callback to render minimap between RenderConnections and EndNodeEditor
         */
        void Render(std::function<void()> minimapRenderCallback = nullptr)
        {
            if (m_graphId < 0 || !m_context)
            {
                ImGui::TextDisabled("BehaviorTree graph not loaded");
                return;
            }

            // Capture screen position for coordinate transformations
            m_canvasScreenPos = ImGui::GetCursorScreenPos();

            // Set current context for ImNodes
            ImNodes::SetCurrentContext(m_context);

            // Begin node editor
            ImNodes::BeginNodeEditor();

            // Render nodes
            RenderNodes();

            // Render connections
            RenderConnections();

            // Render minimap if callback provided (MUST be before EndNodeEditor)
            if (minimapRenderCallback)
            {
                minimapRenderCallback();
            }

            // Handle canvas interaction (pan/zoom)
            UpdateCanvasInteraction();

            // End node editor
            ImNodes::EndNodeEditor();

            // PHASE 82: Handle link creation via UI
            // MOVED outside BeginNodeEditor/EndNodeEditor scope to fix assertion 
            // "GImNodes->CurrentScope == ImNodesScope_None" in ImNodes::IsLinkCreated
            int startPinId, endPinId;
            if (ImNodes::IsLinkCreated(&startPinId, &endPinId))
            {
                GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
                if (graphDoc)
                {
                    // Convert back from internal mapping:
                    // inputPinId = nodeUid * 2
                    // outputPinId = nodeUid * 2 + 1
                    // ImNodes.Link(linkUid, fromPinValue, toPinValue) expects output -> input
                    
                    // Standard BT flow: Parent Output -> Child Input
                    // Ensure startPin is output (odd) and endPin is input (even)
                    if (startPinId % 2 == 0 && endPinId % 2 != 0)
                    {
                        // Swap to ensure output -> input
                        std::swap(startPinId, endPinId);
                    }

                    if (startPinId % 2 != 0 && endPinId % 2 == 0)
                    {
                        graphDoc->ConnectPins(NodeGraphTypes::PinId{static_cast<uint32_t>(startPinId)}, 
                                             NodeGraphTypes::PinId{static_cast<uint32_t>(endPinId)});
                        graphDoc->SetDirty(true);
                        SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Link created: " << startPinId << " -> " << endPinId << "\n";
                    }
                }
            }

            // IMPORTANT: Add a dummy item to satisfy ImGui's layout contract.
            // ImNodes::EndNodeEditor() may call SetCursorScreenPos() internally for minimap positioning.
            // Without a subsequent layout item, ImGui will assert in ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
            // This dummy ensures the boundary extension is properly validated.
            // See: ImGui v1.92.6 breaking change in imgui.cpp lines 10790-10801
            ImGui::Dummy(ImVec2(0, 0));

            // Restore ImGui default context
            ImNodes::SetCurrentContext(nullptr);
        }

        /**
         * @brief Get ImNodes context
         */
        ImNodesContext* GetContext() const { return m_context; }

        /**
         * @brief Returns the ID of the currently selected node, or -1 if none.
         */
        int GetSelectedNodeId() const
        {
            if (!m_context) return -1;
            
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);
            
            int selectedNodeId = -1;
            if (ImNodes::NumSelectedNodes() > 0)
            {
                std::vector<int> selectedNodes(ImNodes::NumSelectedNodes());
                ImNodes::GetSelectedNodes(selectedNodes.data());
                selectedNodeId = selectedNodes[0];
            }
            
            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return selectedNodeId;
        }

        /**
         * @brief Returns current panning offset, safe to call outside Render()
         */
        ImVec2 GetPanning() const
        {
            if (!m_context) return ImVec2(0, 0);
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);
            ImVec2 pan = ImNodes::EditorContextGetPanning();
            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return pan;
        }

    private:
        int m_graphId = -1;
        ImNodesContext* m_context = nullptr;

        // Canvas state
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);
        ImVec2 m_canvasOffset;
        float m_canvasZoom = 1.0f;
        int m_nextLinkId = 0;

        // Selection state
        int m_selectedNode = -1;
        std::set<int> m_initializedNodes;

        // Rendering helpers
        void RenderNodes()
        {
            GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
            if (!graphDoc)
            {
                return;
            }

            const auto& nodes = graphDoc->GetNodes();
            
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const auto& node = nodes[i];
                int nodeUid = static_cast<int>(node.id.value);

                // Phase 62 FIX: Only set position once for new nodes
                if (m_initializedNodes.find(nodeUid) == m_initializedNodes.end())
                {
                    ImNodes::SetNodeGridSpacePos(nodeUid, ImVec2(node.position.x, node.position.y));
                    m_initializedNodes.insert(nodeUid);
                }

                // LEGACY RESTORATION: Use NodeStyleRegistry for colors and icons
                NodeType type = StringToNodeType(node.type);
                const NodeStyle& style = NodeStyleRegistry::Get().GetStyle(type);

                ImNodes::PushColorStyle(ImNodesCol_TitleBar, style.headerColor);
                ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, style.headerHoveredColor);
                ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, style.headerSelectedColor);

                ImNodes::BeginNode(nodeUid);

                // LEGACY RESTORATION: Breakpoint Indicator (visual-only in editor)
                if (node.parameters.count("breakpoint") && node.parameters.at("breakpoint") == "true")
                {
                    ImVec2 nodePos = ImNodes::GetNodeScreenSpacePos(nodeUid);
                    ImGui::GetWindowDrawList()->AddCircleFilled(
                        ImVec2(nodePos.x - 10, nodePos.y - 10), 6.0f, IM_COL32(255, 50, 50, 255));
                    ImGui::GetWindowDrawList()->AddCircle(
                        ImVec2(nodePos.x - 10, nodePos.y - 10), 7.0f, IM_COL32(255, 255, 255, 200));
                }

                // Node Header with Icon
                ImNodes::BeginNodeTitleBar();
                if (style.icon && style.icon[0] != '\0')
                {
                    ImGui::Text("[%s] %s", style.icon, node.name.c_str());
                }
                else
                {
                    ImGui::TextUnformatted(node.name.c_str());
                }
                ImNodes::EndNodeTitleBar();

                // LEGACY RESTORATION: Execution status indicator (from simulation)
                if (node.parameters.count("lastStatus"))
                {
                    std::string status = node.parameters.at("lastStatus");
                    ImVec4 statusColor = ImVec4(1, 1, 1, 1);
                    const char* statusIcon = "?";

                    if (status == "SUCCESS") { statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); statusIcon = "v"; }
                    else if (status == "FAILURE") { statusColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); statusIcon = "x"; }
                    else if (status == "RUNNING") { statusColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); statusIcon = "O"; }

                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
                    ImGui::TextColored(statusColor, "[%s]", statusIcon);
                }

                // Input pin
                {
                    int inputPinId = nodeUid * 2;
                    ImNodes::BeginInputAttribute(inputPinId);
                    ImGui::TextUnformatted("In");
                    ImNodes::EndInputAttribute();
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "%s", node.type.c_str());

                // Show specific subtype if available (Legacy feature)
                if (type == NodeType::BT_Action && node.parameters.count("actionType"))
                    ImGui::TextDisabled("Action: %s", node.parameters.at("actionType").c_str());
                else if (type == NodeType::BT_Condition && node.parameters.count("conditionType"))
                    ImGui::TextDisabled("Cond: %s", node.parameters.at("conditionType").c_str());

                ImGui::Spacing();

                // Output pin
                {
                    int outputPinId = nodeUid * 2 + 1;
                    ImNodes::BeginOutputAttribute(outputPinId);
                    ImGui::Indent(60.0f); 
                    ImGui::TextUnformatted("Out");
                    ImNodes::EndOutputAttribute();
                }

                ImNodes::EndNode();
                
                ImNodes::PopColorStyle();
                ImNodes::PopColorStyle();
                ImNodes::PopColorStyle();
            }
        }

        void RenderConnections()
        {
            // Get active graph from NodeGraphManager (NodeGraph namespace)
            GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
            if (!graphDoc)
            {
                // Phase 59 FIX: Removed frame-by-frame logging (violates COPILOT_INSTRUCTIONS)
                // This method is called 60 times per second, so logging here creates console spam
                return;
            }

            const auto& links = graphDoc->GetLinks();
            
            // Build a map of output pin -> list of links to find indices for child nodes
            // In BT, the order of children (links from an output pin) is vital
            std::map<uint32_t, std::vector<size_t>> outputToLinks;
            for (size_t i = 0; i < links.size(); ++i)
            {
                outputToLinks[links[i].fromPin.value].push_back(i);
            }

            for (size_t i = 0; i < links.size(); ++i)
            {
                const auto& link = links[i];
                int linkUid = static_cast<int>(i);

                int fromPinValue = static_cast<int>(link.fromPin.value);
                int toPinValue = static_cast<int>(link.toPin.value);

                ImNodes::Link(linkUid, fromPinValue, toPinValue);

                // LEGACY RESTORATION: Draw child index on links if source is a Composite
                // We identify composite nodes (Selector, Sequence, etc.) and number their links
                uint32_t parentNodeId = link.fromPin.value / 2;
                const auto& nodes = graphDoc->GetNodes();
                const NodeData* parentNode = nullptr;
                for (const auto& n : nodes) { if (n.id.value == parentNodeId) { parentNode = &n; break; } }

                if (parentNode)
                {
                    NodeType pType = StringToNodeType(parentNode->type);
                    bool isComposite = (pType == NodeType::BT_Selector || pType == NodeType::BT_Sequence || 
                                      pType == NodeType::BT_Parallel || pType == NodeType::BT_RandomSelector);

                    if (isComposite)
                    {
                        // Find index of this link among all links from this output pin
                        const auto& siblingLinks = outputToLinks[link.fromPin.value];
                        int childIndex = -1;
                        for (int idx = 0; idx < (int)siblingLinks.size(); ++idx)
                        {
                            if (siblingLinks[idx] == i) { childIndex = idx; break; }
                        }

                        if (childIndex != -1)
                        {
                            // LEGACY RESTORATION: Draw child index label above the connection
                            // We use a simplified mid-point calculation between input and output pins
                            // For a more precise Bezier mid-point, we'd need internal ImNodes geometry.
                            int childNodeId = static_cast<int>(link.toPin.value / 2);

                            ImVec2 parentPos = ImNodes::GetNodeEditorSpacePos(static_cast<int>(parentNodeId));
                            ImVec2 childPos = ImNodes::GetNodeEditorSpacePos(childNodeId);

                            // Estimate pin positions (assuming standard node size and pin offsets)
                            // In a real ImNodes session, we could use GetPinScreenPos if we were inside BeginNode/EndNode
                            // Since we are in RenderConnections, we approximate.
                            ImVec2 midPoint = ImVec2(
                                (parentPos.x + childPos.x) * 0.5f + 50.0f, 
                                (parentPos.y + childPos.y) * 0.5f
                            );

                            char idxBuf[16];
#ifdef _MSC_VER
                            sprintf_s(idxBuf, sizeof(idxBuf), "%d", childIndex);
#else
                            sprintf(idxBuf, "%d", childIndex);
#endif

                            // Draw background circle for readability
                            // Convert Editor Space to Screen Space manually: EditorPos + Pan + CanvasPos
                            ImVec2 pan = ImNodes::EditorContextGetPanning();
                            ImVec2 screenMidPoint = ImVec2(
                                midPoint.x + pan.x + m_canvasScreenPos.x,
                                midPoint.y + pan.y + m_canvasScreenPos.y
                            );

                            ImGui::GetWindowDrawList()->AddCircleFilled(screenMidPoint, 10.0f, IM_COL32(40, 40, 40, 220));
                            ImGui::GetWindowDrawList()->AddText(
                                ImVec2(screenMidPoint.x - 4, screenMidPoint.y - 7), 
                                IM_COL32(255, 255, 100, 255), idxBuf);
                        }
                    }
                }
            }
        }

        void UpdateCanvasInteraction()
        {
            // Sync ImNodes positions back to document
            GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
            if (graphDoc)
            {
                auto& nodes = graphDoc->GetNodesRef();
                for (auto& node : nodes)
                {
                    ImVec2 pos = ImNodes::GetNodeGridSpacePos(static_cast<int>(node.id.value));
                    if (pos.x != node.position.x || pos.y != node.position.y)
                    {
                        node.position.x = pos.x;
                        node.position.y = pos.y;
                        graphDoc->SetDirty(true);
                    }
                }
            }
        }
    };
}
