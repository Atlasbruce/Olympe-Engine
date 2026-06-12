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
#include <algorithm>
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

            // Phase 86: Cache hover state AFTER closing scope to satisfy assertions
            // IsNodeHovered, IsLinkHovered, etc. assert (GImNodes->CurrentScope == ImNodesScope_None)
            m_isEditorHoveredCache = ImNodes::IsEditorHovered();
            m_hoveredNodeCache = -1;
            m_hoveredLinkCache = -1;
            ImNodes::IsNodeHovered(&m_hoveredNodeCache);
            ImNodes::IsLinkHovered(&m_hoveredLinkCache);

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
         * @brief Returns all currently selected nodes.
         */
        std::vector<int> GetSelectedNodes() const
        {
            if (!m_context) return {};
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);

            std::vector<int> selectedNodes;
            int numSelected = ImNodes::NumSelectedNodes();
            if (numSelected > 0)
            {
                selectedNodes.resize(numSelected);
                ImNodes::GetSelectedNodes(selectedNodes.data());
            }

            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return selectedNodes;
        }

        /**
         * @brief Returns the ID of the currently hovered link, or -1 if none.
         */
        int GetHoveredLink() const
        {
            if (!m_context) return -1;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);
            int hoveredLink = -1;
            bool result = ImNodes::IsLinkHovered(&hoveredLink);
            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return result ? hoveredLink : -1;
        }

        /**
         * @brief Returns true if a node is currently hovered, and outputs its ID.
         */
        bool IsNodeHovered(int* nodeId) const
        {
            if (!m_context) return false;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);
            bool result = ImNodes::IsNodeHovered(nodeId);
            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return result;
        }

        /**
         * @brief Returns true if the editor canvas is currently hovered.
         */
        bool IsEditorHovered() const
        {
            if (!m_context) return false;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);
            bool result = ImNodes::IsEditorHovered();
            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return result;
        }

        /**
         * @brief Returns IDs of all currently selected links.
         */
        std::vector<int> GetSelectedLinks() const
        {
            if (!m_context) return {};
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);

            std::vector<int> selectedLinks;
            int numSelected = ImNodes::NumSelectedLinks();
            if (numSelected > 0)
            {
                selectedLinks.resize(numSelected);
                ImNodes::GetSelectedLinks(selectedLinks.data());
            }

            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return selectedLinks;
        }

        /**
         * @brief Centralized handler for context menu triggering.
         * Switches to the correct ImNodes context, detects what is hovered,
         * updates selection if necessary, and returns true if a popup should open.
         */
        bool HandleContextMenuTrigger(int* outHoveredNode, int* outHoveredLink)
        {
            if (!m_context) return false;

            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_context);

            bool shouldOpen = false;

            // Phase 86: Use cached values from last Render() call
            if (m_hoveredNodeCache != -1)
            {
                int node = m_hoveredNodeCache;
                if (outHoveredNode) *outHoveredNode = node;
                // Auto-select node if not already selected
                if (!ImNodes::IsNodeSelected(node))
                {
                    ImNodes::ClearNodeSelection();
                    ImNodes::ClearLinkSelection();
                    ImNodes::SelectNode(node);
                }
                shouldOpen = true;
            }
            else if (m_hoveredLinkCache != -1)
            {
                int link = m_hoveredLinkCache;
                if (outHoveredLink) *outHoveredLink = link;
                ImNodes::ClearNodeSelection();
                ImNodes::ClearLinkSelection();
                ImNodes::SelectLink(link);
                shouldOpen = true;
            }
            else if (m_isEditorHoveredCache)
            {
                // PHASE 87: Allow context menu on empty editor space
                if (outHoveredNode) *outHoveredNode = -1;
                if (outHoveredLink) *outHoveredLink = -1;
                shouldOpen = true;
            }

            if (oldContext) ImNodes::SetCurrentContext(oldContext);
            return shouldOpen;
        }

        /**
         * @brief Returns the current panning offset of the canvas, safe to call outside Render()
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

        // Phase 86: Hover state cache to avoid ImNodes scope assertions
        int m_hoveredNodeCache = -1;
        int m_hoveredLinkCache = -1;
        bool m_isEditorHoveredCache = false;

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

                // PHASE 84: Node Width Spacer (Parity with VisualScript)
                // Ensures nodes have enough width for horizontal pin layout
                float minNodeWidth = 140.0f;
                ImVec2 titleSize = ImGui::CalcTextSize(node.name.c_str());
                if (titleSize.x + 30.0f > minNodeWidth) minNodeWidth = titleSize.x + 30.0f;
                ImGui::InvisibleButton("##node_width_spacer", ImVec2(minNodeWidth, 1.0f));

                // LEGACY RESTORATION: Breakpoint Indicator (visual-only in editor)
                if (node.parameters.count("breakpoint") && node.parameters.at("breakpoint") == "true")
                {
                    // Draw red dot for breakpoint
                    ImVec2 nodePos = ImNodes::GetNodeScreenSpacePos(nodeUid);
                    ImGui::GetWindowDrawList()->AddCircleFilled(
                        ImVec2(nodePos.x - 10, nodePos.y - 10), 6.0f, IM_COL32(255, 50, 50, 255));
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

                // PHASE 84: Horizontal Pin Layout (VisualScript Style)
                // Col 0: Input pins (left) | Col 1: Output pins (right)
                ImGui::Columns(2, "bt_node_pins", false);
                ImGui::SetColumnWidth(0, minNodeWidth * 0.5f);

                // LEFT COLUMN: Input pin
                {
                    int inputPinId = nodeUid * 2;
                    ImNodes::BeginInputAttribute(inputPinId);
                    ImGui::TextUnformatted("In");
                    ImNodes::EndInputAttribute();
                }

                // RIGHT COLUMN: Output pin
                ImGui::NextColumn();
                {
                    int outputPinId = nodeUid * 2 + 1;
                    ImNodes::BeginOutputAttribute(outputPinId);
                    // Match VisualScript right-alignment for outputs
                    float textWidth = ImGui::CalcTextSize("Out").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (minNodeWidth * 0.5f) - textWidth - 25.0f);
                    ImGui::TextUnformatted("Out");
                    ImNodes::EndOutputAttribute();
                }
                ImGui::Columns(1); // End columns

                // Node Content (below pins)
                ImGui::Spacing();
                ImGui::Indent(5.0f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", node.type.c_str());

                // Show specific subtype if available (Legacy feature)
                if (type == NodeType::BT_Action && node.parameters.count("actionType"))
                    ImGui::TextDisabled("Action: %s", node.parameters.at("actionType").c_str());
                else if (type == NodeType::BT_Condition && node.parameters.count("conditionType"))
                    ImGui::TextDisabled("Cond: %s", node.parameters.at("conditionType").c_str());

                // LEGACY RESTORATION: Execution status indicator 
                if (node.parameters.count("lastStatus"))
                {
                    std::string status = node.parameters.at("lastStatus");
                    ImVec4 statusColor = ImVec4(1, 1, 1, 1);
                    const char* statusIcon = "?";

                    if (status == "SUCCESS") { statusColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); statusIcon = "v"; }
                    else if (status == "FAILURE") { statusColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); statusIcon = "x"; }
                    else if (status == "RUNNING") { statusColor = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); statusIcon = "O"; }

                    ImGui::TextColored(statusColor, "Status: %s [%s]", status.c_str(), statusIcon);
                }
                ImGui::Unindent(5.0f);

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

            // Phase 85: Sort links for each output pin based on target node Y position (Legacy Parity)
            for (auto& pair : outputToLinks)
            {
                std::sort(pair.second.begin(), pair.second.end(), [&](size_t a, size_t b) {
                    uint32_t nodeAId = links[a].toPin.value / 2;
                    uint32_t nodeBId = links[b].toPin.value / 2;
                    // Use Grid Space for stable sorting independent of panning
                    ImVec2 posA = ImNodes::GetNodeGridSpacePos(static_cast<int>(nodeAId));
                    ImVec2 posB = ImNodes::GetNodeGridSpacePos(static_cast<int>(nodeBId));
                    if (posA.y != posB.y) return posA.y < posB.y;
                    return posA.x < posB.x;
                });
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

                                                                                 // PHASE 85: Draw child index label EXACTLY on the connection line
                                                                                 // Calculate precise pin positions in grid space for the midpoint
                                                                                 int childNodeId = static_cast<int>(link.toPin.value / 2);

                                                                                 ImVec2 parentGridPos = ImNodes::GetNodeGridSpacePos(static_cast<int>(parentNodeId));
                                                                                 ImVec2 childGridPos = ImNodes::GetNodeGridSpacePos(childNodeId);

                                                                                 // Retrieve node width to find output pin X (Mirroring RenderNodes)
                                                                                 float nodeWidth = 140.0f;
                                                                                 ImVec2 titleSize = ImGui::CalcTextSize(parentNode->name.c_str());
                                                                                 if (titleSize.x + 30.0f > nodeWidth) nodeWidth = titleSize.x + 30.0f;

                                                                                 // Pin vertical offset: TitleBar + half-row
                                                                                 float pinYOffset = 35.0f; 

                                                                                 // Pin positions in grid space
                                                                                 ImVec2 pOut = ImVec2(parentGridPos.x + nodeWidth, parentGridPos.y + pinYOffset);
                                                                                 ImVec2 pIn = ImVec2(childGridPos.x, childGridPos.y + pinYOffset);

                                                                                 // Midpoint calculation on the link
                                                                                 ImVec2 midPointGrid = ImVec2((pOut.x + pIn.x) * 0.5f, (pOut.y + pIn.y) * 0.5f);

                                                                                 char idxBuf[16];
                                                     #ifdef _MSC_VER
                                                                                 sprintf_s(idxBuf, sizeof(idxBuf), "%d", childIndex);
                                                     #else
                                                                                 sprintf(idxBuf, "%d", childIndex);
                                                     #endif

                                                                                 // Convert Grid Space to Screen Space
                                                                                 ImVec2 pan = ImNodes::EditorContextGetPanning();
                                                                                 ImVec2 screenMidPoint = ImVec2(
                                                                                     midPointGrid.x + pan.x + m_canvasScreenPos.x,
                                                                                     midPointGrid.y + pan.y + m_canvasScreenPos.y
                                                                                 );

                                                                                 // Draw background circle for readability
                                                                                 ImDrawList* drawList = ImGui::GetWindowDrawList();
                                                                                 drawList->AddCircleFilled(screenMidPoint, 10.0f, IM_COL32(20, 20, 20, 240));
                                                                                 drawList->AddCircle(screenMidPoint, 10.0f, IM_COL32(230, 230, 230, 180), 0, 1.5f);

                                                                                 // Draw text BOLD (triple-strike for visibility) - White color
                                                                                 ImVec2 textPos = ImVec2(screenMidPoint.x - 4, screenMidPoint.y - 7);
                                                                                 ImU32 textColor = IM_COL32(255, 255, 255, 255);
                                                                                 drawList->AddText(textPos, textColor, idxBuf);
                                                                                 drawList->AddText(ImVec2(textPos.x + 1, textPos.y), textColor, idxBuf);
                                                                                 drawList->AddText(ImVec2(textPos.x, textPos.y + 1), textColor, idxBuf);
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
