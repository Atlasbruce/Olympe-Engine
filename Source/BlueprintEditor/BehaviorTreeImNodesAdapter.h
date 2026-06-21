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
            : m_imnodesContext(nullptr)
            , m_editorContext(nullptr)
            , m_canvasOffset(0, 0)
            , m_ownsImNodesContext(false)
        {
            // Robust context strategy:
            // - Prefer an existing global ImNodes context when available.
            // - If none exists, create one and mark ownership so we can destroy it.
            // - Always create a dedicated ImNodesEditorContext for this adapter
            //   and use EditorContextSet() before rendering. This matches the
            //   pattern used by VisualScript and the imnodes multi-editor example
            //   and avoids conflicts when multiple graph types are open.
            ImNodesContext* current = ImNodes::GetCurrentContext();
            if (current == nullptr)
            {
                m_imnodesContext = ImNodes::CreateContext();
                m_ownsImNodesContext = (m_imnodesContext != nullptr);
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Created global ImNodesContext\n";
            }
            else
            {
                m_imnodesContext = current;
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Reusing existing ImNodesContext\n";
            }

            // Create a per-adapter editor context (holds pan/selection state)
            m_editorContext = ImNodes::EditorContextCreate();
            if (m_editorContext)
            {
                // Do not call EditorContextSet here; will be set at render time
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Editor context created\n";
            }
        }

        ~BehaviorTreeImNodesAdapter()
        {
            // Free the editor context for this adapter
            if (m_editorContext)
            {
                ImNodes::EditorContextFree(m_editorContext);
                m_editorContext = nullptr;
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Editor context freed\n";
            }

            // Destroy global ImNodesContext only if we created it
            if (m_ownsImNodesContext && m_imnodesContext)
            {
                ImNodes::DestroyContext(m_imnodesContext);
                m_imnodesContext = nullptr;
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Owned ImNodesContext destroyed\n";
            }
        }

        /**
         * @brief Initialize adapter with graph document
         * @param graphId ID in NodeGraphManager
         */
        void Initialize(int graphId)
        {
            // Re-initialize only for a true graph switch/load.
            // The adapter caches which nodes have already had their positions
            // injected into ImNodes. Clearing the cache ensures a newly loaded
            // graph re-applies its stored positions, but ordinary tab activation
            // should not recreate the adapter.
            m_graphId = graphId;
            m_initializedNodes.clear();
            m_selectedNode = -1;
            m_hoveredNodeCache = -1;
            m_hoveredLinkCache = -1;
            m_isEditorHoveredCache = false;

            if (m_imnodesContext)
            {
                ImNodes::SetCurrentContext(m_imnodesContext);
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

        // Mapping between canonical NodeId (document) and ImNodes uids used for rendering
        // We keep a stable small uid space to avoid passing large or legacy-encoded ids
        std::map<uint32_t,int> m_nodeIdToUid; // NodeId.value -> uid
        std::map<int,uint32_t> m_uidToNodeId; // uid -> NodeId.value
        int m_nextUid = 1;
        // Registered attribute ids for the current render pass (input/output pin ids)
        // Used as a defensive check before calling ImNodes::Link to avoid passing
        // attribute ids that were never registered in this ImNodes editor context.
        std::set<int> m_registeredAttributes;
        // Attributes registered on the previous render frame - used to reduce log spam
        std::set<int> m_lastRegisteredAttributes;
        /**
         * @brief Convert an ImNodes uid to canonical NodeId (document id)
         * @param uid ImNodes uid
         * @return canonical NodeId as int, or -1 if no mapping
         */
        int GetCanonicalNodeIdFromUid(int uid) const
        {
            auto it = m_uidToNodeId.find(uid);
            if (it == m_uidToNodeId.end()) return -1;
            return static_cast<int>(it->second);
        }

        /**
         * @brief Convert canonical NodeId to ImNodes uid
         * @param canonicalId canonical NodeId value
         * @return uid or -1 if not mapped
         */
        int GetUidFromCanonicalNodeId(uint32_t canonicalId) const
        {
            auto it = m_nodeIdToUid.find(canonicalId);
            if (it == m_nodeIdToUid.end()) return -1;
            return it->second;
        }
        // Remember which links we've already warned about to avoid spamming the console
        std::set<int> m_reportedSkippedLinks;

        /**
         * @brief Main render call - draws entire BehaviorTree with ImNodes
         * @param minimapRenderCallback Optional callback to render minimap between RenderConnections and EndNodeEditor
         */
        void Render(std::function<void()> minimapRenderCallback = nullptr)
        {
            if (m_graphId < 0 || !m_imnodesContext)
            {
                ImGui::TextDisabled("BehaviorTree graph not loaded");
                return;
            }

            // Capture screen position for coordinate transformations
            m_canvasScreenPos = ImGui::GetCursorScreenPos();

            // Ensure the global ImNodes context is active and set this adapter's
            // editor context so pan/selection state is isolated per adapter.
            ImNodesContext* prevCtx = ImNodes::GetCurrentContext();

            if (m_imnodesContext)
                ImNodes::SetCurrentContext(m_imnodesContext);
            if (m_editorContext)
                ImNodes::EditorContextSet(m_editorContext);

            // Ensure mapping exists between NodeId and ImNodes uid before rendering
            // We prefer a stable mapping that preserves existing uids where possible
            // to avoid selection/interaction jitter. Only assign new uids for newly
            // discovered node ids and remove mappings for deleted nodes.
            GraphDocument* mappingDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
            std::set<uint32_t> currentNodeIds;
            if (mappingDoc)
            {
                const auto& nodes = mappingDoc->GetNodes();
                for (const auto& n : nodes) currentNodeIds.insert(n.id.value);
            }

            // Remove mappings for nodes that no longer exist
            std::vector<int> removedUids;
            for (auto it = m_nodeIdToUid.begin(); it != m_nodeIdToUid.end(); )
            {
                if (currentNodeIds.find(it->first) == currentNodeIds.end())
                {
                    removedUids.push_back(it->second);
                    m_uidToNodeId.erase(it->second);
                    it = m_nodeIdToUid.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // Assign uids to newly discovered nodes (preserve existing uids)
            for (uint32_t nid : currentNodeIds)
            {
                if (m_nodeIdToUid.find(nid) == m_nodeIdToUid.end())
                {
                    // Find next free uid starting from 1
                    int candidate = 1;
                    while (m_uidToNodeId.find(candidate) != m_uidToNodeId.end()) ++candidate;
                    m_nodeIdToUid[nid] = candidate;
                    m_uidToNodeId[candidate] = nid;
                    if (candidate >= m_nextUid) m_nextUid = candidate + 1;
                }
            }

            // Remove any initialized node entries that referenced removed uids
            for (int ru : removedUids) m_initializedNodes.erase(ru);

            // Clear registered attributes cache for this render pass (re-registered each frame)
            m_registeredAttributes.clear();
            // Reset last registered attributes only when mapping changed
            // If node count changed, mapping may have changed; update m_lastRegisteredAttributes
            if (m_nodeIdToUid.size() != m_lastRegisteredAttributes.size())
            {
                m_lastRegisteredAttributes.clear();
            }
            // Do not clear m_reportedSkippedLinks here; we want to avoid spamming logs across frames

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
                                // Map attribute ids back to node ids using uid mapping
                                int startUid = startPinId / 2;
                                int endUid = endPinId / 2;
                                uint32_t startNode = 0, endNode = 0;
                                auto itS = m_uidToNodeId.find(startUid);
                                if (itS != m_uidToNodeId.end()) startNode = itS->second;
                                auto itE = m_uidToNodeId.find(endUid);
                                if (itE != m_uidToNodeId.end()) endNode = itE->second;

                                if (startNode != 0 && endNode != 0)
                                {
                                    graphDoc->ConnectPins(NodeGraphTypes::PinId{startNode}, 
                                                         NodeGraphTypes::PinId{endNode});
                                    graphDoc->SetDirty(true);
                                    SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Link created (mapped): " << startNode << " -> " << endNode << "\n";
                                }
                                else
                                {
                                    SYSTEM_LOG << "[BehaviorTreeImNodesAdapter] Failed to map created link attrs to NodeIds: "
                                               << startPinId << " -> " << endPinId << "\n";
                                }
                            }
                }
            }

            // IMPORTANT: Add a dummy item to satisfy ImGui's layout contract.
            // ImNodes::EndNodeEditor() may call SetCursorScreenPos() internally for minimap positioning.
            // Without a subsequent layout item, ImGui will assert in ErrorCheckUsingSetCursorPosToExtendParentBoundaries().
            // This dummy ensures the boundary extension is properly validated.
            // See: ImGui v1.92.6 breaking change in imgui.cpp lines 10790-10801
            ImGui::Dummy(ImVec2(0, 0));

            // Restore previous ImNodes context (previous editor context is part of that ImNodesContext)
            if (prevCtx)
            {
                ImNodes::SetCurrentContext(prevCtx);
            }
            else
            {
                ImNodes::SetCurrentContext(nullptr);
            }
        }

        /**
         * @brief Get ImNodes context
         */
        ImNodesContext* GetContext() const { return m_imnodesContext; }

        /**
         * @brief Returns the ID of the currently selected node, or -1 if none.
         */
        int GetSelectedNodeId() const
        {
            if (!m_imnodesContext) return -1;

            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);

            int selectedNodeId = -1;
            if (ImNodes::NumSelectedNodes() > 0)
            {
                std::vector<int> selectedNodes(ImNodes::NumSelectedNodes());
                ImNodes::GetSelectedNodes(selectedNodes.data());
                selectedNodeId = selectedNodes[0];
            }

            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return selectedNodeId;
        }

        /**
         * @brief Returns all currently selected nodes.
         */
        std::vector<int> GetSelectedNodes() const
        {
            if (!m_imnodesContext) return {};
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);

            std::vector<int> selectedNodes;
            int numSelected = ImNodes::NumSelectedNodes();
            if (numSelected > 0)
            {
                selectedNodes.resize(numSelected);
                ImNodes::GetSelectedNodes(selectedNodes.data());
            }

            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return selectedNodes;
        }

        /**
         * @brief Returns the ID of the currently hovered link, or -1 if none.
         */
        int GetHoveredLink() const
        {
            if (!m_imnodesContext) return -1;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);
            int hoveredLink = -1;
            bool result = ImNodes::IsLinkHovered(&hoveredLink);
            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return result ? hoveredLink : -1;
        }

        /**
         * @brief Returns true if a node is currently hovered, and outputs its ID.
         */
        bool IsNodeHovered(int* nodeId) const
        {
            if (!m_imnodesContext) return false;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);
            bool result = ImNodes::IsNodeHovered(nodeId);
            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return result;
        }

        /**
         * @brief Returns true if the editor canvas is currently hovered.
         */
        bool IsEditorHovered() const
        {
            if (!m_imnodesContext) return false;
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);
            bool result = ImNodes::IsEditorHovered();
            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return result;
        }

        /**
         * @brief Returns IDs of all currently selected links.
         */
        std::vector<int> GetSelectedLinks() const
        {
            if (!m_imnodesContext) return {};
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);

            std::vector<int> selectedLinks;
            int numSelected = ImNodes::NumSelectedLinks();
            if (numSelected > 0)
            {
                selectedLinks.resize(numSelected);
                ImNodes::GetSelectedLinks(selectedLinks.data());
            }

            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return selectedLinks;
        }

        /**
         * @brief Centralized handler for context menu triggering.
         * Switches to the correct ImNodes context, detects what is hovered,
         * updates selection if necessary, and returns true if a popup should open.
         */
        bool HandleContextMenuTrigger(int* outHoveredNode, int* outHoveredLink)
        {
            if (!m_imnodesContext) return false;

            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);

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

            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return shouldOpen;
        }

        /**
         * @brief Returns the current panning offset of the canvas, safe to call outside Render()
         */
        ImVec2 GetPanning() const
        {
            if (!m_imnodesContext) return ImVec2(0, 0);
            ImNodesContext* oldContext = ImNodes::GetCurrentContext();
            ImNodes::SetCurrentContext(m_imnodesContext);
            ImNodes::EditorContextSet(m_editorContext);
            ImVec2 pan = ImNodes::EditorContextGetPanning();
            if (oldContext)
            {
                ImNodes::SetCurrentContext(oldContext);
            }
            return pan;
        }

    private:
        int m_graphId = -1;
        ImNodesContext* m_imnodesContext = nullptr;

        // Canvas state
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);
        ImVec2 m_canvasOffset;
        float m_canvasZoom = 1.0f;
        int m_nextLinkId = 0;

        // Selection state
        int m_selectedNode = -1;
        std::set<int> m_initializedNodes;

        // Editor/Context management
        ImNodesEditorContext* m_editorContext = nullptr;
        bool m_ownsImNodesContext = false;

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
                int canonicalNodeId = static_cast<int>(node.id.value);
                int mappedUid = canonicalNodeId;
                auto itMap = m_nodeIdToUid.find(node.id.value);
                if (itMap != m_nodeIdToUid.end()) mappedUid = itMap->second;

                // Phase 62 FIX: Only set position once for new nodes (use mapped uid)
                if (m_initializedNodes.find(mappedUid) == m_initializedNodes.end())
                {
                    ImNodes::SetNodeGridSpacePos(mappedUid, ImVec2(node.position.x, node.position.y));
                    m_initializedNodes.insert(mappedUid);
                }

                // LEGACY RESTORATION: Use NodeStyleRegistry for colors and icons
                NodeType type = StringToNodeType(node.type);
                const NodeStyle& style = NodeStyleRegistry::Get().GetStyle(type);

                ImNodes::PushColorStyle(ImNodesCol_TitleBar, style.headerColor);
                ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, style.headerHoveredColor);
                ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, style.headerSelectedColor);

                ImNodes::BeginNode(mappedUid);

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
                    ImVec2 nodePos = ImNodes::GetNodeScreenSpacePos(mappedUid);
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

                // Determine pin visibility based on node semantic
                NodeType ntype = StringToNodeType(node.type);
                bool hasInputPin = !(ntype == NodeType::BT_Root || ntype == NodeType::BT_OnEvent);
                bool hasOutputPin = !(ntype == NodeType::BT_Action || ntype == NodeType::BT_Condition);

                // LEFT COLUMN: Input pin (omit for Root / OnEvent entry nodes)
                {
                    if (hasInputPin)
                    {
                        int inputPinId = mappedUid * 2;
                        // Record attribute for this render pass
                        m_registeredAttributes.insert(inputPinId);
                        ImNodes::BeginInputAttribute(inputPinId);
                        ImGui::TextUnformatted("In");
                        ImNodes::EndInputAttribute();
                    }
                    else
                    {
                        // Keep column spacing consistent when no pin is present
                        ImGui::Dummy(ImVec2(minNodeWidth * 0.5f - 8.0f, 1.0f));
                    }
                }

                // RIGHT COLUMN: Output pin (omit for leaf Action/Condition nodes)
                ImGui::NextColumn();
                {
                    if (hasOutputPin)
                    {
                        int outputPinId = mappedUid * 2 + 1;
                        m_registeredAttributes.insert(outputPinId);
                        ImNodes::BeginOutputAttribute(outputPinId);
                        // Match VisualScript right-alignment for outputs
                        float textWidth = ImGui::CalcTextSize("Out").x;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (minNodeWidth * 0.5f) - textWidth - 25.0f);
                        ImGui::TextUnformatted("Out");
                        ImNodes::EndOutputAttribute();
                    }
                    else
                    {
                        ImGui::Dummy(ImVec2(minNodeWidth * 0.5f - 8.0f, 1.0f));
                    }
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
            
            // Build a map of parent node -> list of links from that parent. In GraphDocument
            // links.fromPin.value/toPin.value are canonical node IDs (not raw ImNodes attr ids).
            std::map<uint32_t, std::vector<size_t>> outputToLinks;
            for (size_t i = 0; i < links.size(); ++i)
            {
                outputToLinks[links[i].fromPin.value].push_back(i);
            }

            // Phase 85: Sort links for each output pin based on target node Y position
            for (auto& pair : outputToLinks)
            {
                std::sort(pair.second.begin(), pair.second.end(), [&](size_t a, size_t b) {
                    uint32_t nodeAId = links[a].toPin.value;
                    uint32_t nodeBId = links[b].toPin.value;
                    // Map canonical node ids to ImNodes uid
                    int uidA = nodeAId;
                    int uidB = nodeBId;
                    auto ita = m_nodeIdToUid.find(nodeAId);
                    if (ita != m_nodeIdToUid.end()) uidA = ita->second;
                    auto itb = m_nodeIdToUid.find(nodeBId);
                    if (itb != m_nodeIdToUid.end()) uidB = itb->second;
                    // Use Grid Space for stable sorting independent of panning
                    ImVec2 posA = ImNodes::GetNodeGridSpacePos(static_cast<int>(uidA));
                    ImVec2 posB = ImNodes::GetNodeGridSpacePos(static_cast<int>(uidB));
                    if (posA.y != posB.y) return posA.y < posB.y;
                    return posA.x < posB.x;
                });
            }

            auto computeAttributeId = [&](uint32_t nodeId, const std::string& pinName, bool preferOutput)->int {
                // Map canonical node id to mapped uid used by ImNodes
                int uid = static_cast<int>(nodeId);
                auto it = m_nodeIdToUid.find(nodeId);
                if (it != m_nodeIdToUid.end()) uid = it->second;
                // ImNodes attribute ids used when rendering: input = uid*2, output = uid*2+1
                if (!pinName.empty())
                {
                    if (pinName == "output" || pinName == "out") return static_cast<int>(uid * 2 + 1);
                    if (pinName == "input" || pinName == "in") return static_cast<int>(uid * 2);
                }
                // Fallback: prefer output for fromPin, input for toPin
                return static_cast<int>(uid * 2 + (preferOutput ? 1 : 0));
            };

            for (size_t i = 0; i < links.size(); ++i)
            {
                const auto& link = links[i];
                int linkUid = static_cast<int>(i);

                uint32_t fromNodeId = static_cast<uint32_t>(link.fromPin.value);
                uint32_t toNodeId = static_cast<uint32_t>(link.toPin.value);

                // Ensure both endpoints have mapping; skip rendering link if mapping missing
                if (m_nodeIdToUid.find(fromNodeId) == m_nodeIdToUid.end() || m_nodeIdToUid.find(toNodeId) == m_nodeIdToUid.end())
                {
                   
                    continue;
                }

                int fromAttr = computeAttributeId(fromNodeId, link.fromPinName, true);
                int toAttr = computeAttributeId(toNodeId, link.toPinName, false);
                // Link rendering is performance-sensitive and executed every frame;
                // avoid verbose per-frame logging here to prevent console spam.

                // Defensive check: ensure both attribute ids were registered in this render pass.
                // If an attribute id is missing, skip the link to avoid triggering asserts
                // inside imnodes (e.g., MiniMapDrawLink expects valid attribute types).
                if (m_registeredAttributes.find(fromAttr) == m_registeredAttributes.end() ||
                    m_registeredAttributes.find(toAttr) == m_registeredAttributes.end())
                {
                    if (m_reportedSkippedLinks.find(linkUid) == m_reportedSkippedLinks.end())
                    {
                        m_reportedSkippedLinks.insert(linkUid);
                    }
                    continue;
                }

                ImNodes::Link(linkUid, fromAttr, toAttr);
                // Update last-registered set after links drawn
                m_lastRegisteredAttributes.insert(fromAttr);
                m_lastRegisteredAttributes.insert(toAttr);

                // LEGACY RESTORATION: Draw child index on links if source is a Composite
                const auto& nodes = graphDoc->GetNodes();
                const NodeData* parentNode = nullptr;
                for (const auto& n : nodes) { if (n.id.value == fromNodeId) { parentNode = &n; break; } }

                if (parentNode)
                {
                    NodeType pType = StringToNodeType(parentNode->type);
                    bool isComposite = (pType == NodeType::BT_Selector || pType == NodeType::BT_Sequence || 
                                      pType == NodeType::BT_Parallel || pType == NodeType::BT_RandomSelector);

                    if (isComposite)
                    {
                        // Find index of this link among all links from this parent node
                        const auto& siblingLinks = outputToLinks[link.fromPin.value];
                        int childIndex = -1;
                        for (int idx = 0; idx < (int)siblingLinks.size(); ++idx)
                        {
                            if (siblingLinks[idx] == i) { childIndex = idx; break; }
                        }

                        // PHASE 85: Draw child index label EXACTLY on the connection line
                        // Use mapped ImNodes uids and per-node dimensions to compute true pin midpoint.
                        int parentUid = static_cast<int>(fromNodeId);
                        int childUid = static_cast<int>(toNodeId);
                        auto itParentUid = m_nodeIdToUid.find(fromNodeId);
                        if (itParentUid != m_nodeIdToUid.end()) parentUid = itParentUid->second;
                        auto itChildUid = m_nodeIdToUid.find(toNodeId);
                        if (itChildUid != m_nodeIdToUid.end()) childUid = itChildUid->second;

                        ImVec2 parentGridPos = ImNodes::GetNodeGridSpacePos(parentUid);
                        ImVec2 childGridPos = ImNodes::GetNodeGridSpacePos(childUid);

                        // Parent output X depends on parent visual width (mirrors RenderNodes min width logic)
                        float parentNodeWidth = 140.0f;
                        ImVec2 parentTitleSize = ImGui::CalcTextSize(parentNode->name.c_str());
                        if (parentTitleSize.x + 30.0f > parentNodeWidth) parentNodeWidth = parentTitleSize.x + 30.0f;

                        // Child-specific offset so midpoint uses each node's local pin row
                        const NodeData* childNode = nullptr;
                        for (const auto& n : nodes) { if (n.id.value == toNodeId) { childNode = &n; break; } }

                        float parentPinYOffset = parentTitleSize.y + 19.0f;
                        float childPinYOffset = parentPinYOffset;
                        if (childNode)
                        {
                            ImVec2 childTitleSize = ImGui::CalcTextSize(childNode->name.c_str());
                            childPinYOffset = childTitleSize.y + 19.0f;
                        }

                        ImVec2 pOut = ImVec2(parentGridPos.x + parentNodeWidth, parentGridPos.y + parentPinYOffset);
                        ImVec2 pIn = ImVec2(childGridPos.x, childGridPos.y + childPinYOffset);
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
                    // Map canonical NodeId -> ImNodes uid to fetch position
                    int uid = static_cast<int>(node.id.value);
                    auto it = m_nodeIdToUid.find(node.id.value);
                    if (it != m_nodeIdToUid.end()) uid = it->second;
                    ImVec2 pos = ImNodes::GetNodeGridSpacePos(uid);
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
