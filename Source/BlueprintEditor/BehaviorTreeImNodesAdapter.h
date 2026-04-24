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
#include "../NodeGraphCore/NodeGraphManager.h"
#include <vector>
#include <string>
#include <map>

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
         */
        void Render()
        {
            if (m_graphId < 0 || !m_context)
            {
                ImGui::TextDisabled("BehaviorTree graph not loaded");
                return;
            }

            // Set current context for ImNodes
            ImNodes::SetCurrentContext(m_context);

            // Begin node editor
            ImNodes::BeginNodeEditor();

            // Render nodes
            RenderNodes();

            // Render connections
            RenderConnections();

            // Handle canvas interaction (pan/zoom)
            UpdateCanvasInteraction();

                 // End node editor
                ImNodes::EndNodeEditor();

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

    private:
        int m_graphId = -1;
        ImNodesContext* m_context = nullptr;

        // Canvas state
        ImVec2 m_canvasOffset;
        float m_canvasZoom = 1.0f;
        int m_nextLinkId = 0;

        // Selection state
        int m_selectedNode = -1;

        // Rendering helpers
        void RenderNodes()
        {
            // Phase 61 FIX: Query CORRECT NodeGraphManager (NodeGraph namespace)
            // BehaviorTreeRenderer uses: NodeGraph::NodeGraphManager::Get().GetActiveGraph()
            // RenderNodes must use the same manager to access stored graphs
            GraphDocument* graphDoc = NodeGraph::NodeGraphManager::Get().GetActiveGraph();
            if (!graphDoc)
            {
                // PHASE 61 DIAGNOSTIC: If we get here, active graph was not set or cleared
                // This should not happen if Initialize() called SetActiveGraph properly
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter::RenderNodes] ERROR: GetActiveGraph returned nullptr "
                           << "(m_graphId=" << m_graphId << ") - querying CORRECT NodeGraph manager\n";
                return;
            }

            // PHASE 61 DIAGNOSTIC: Log graph pointer and node count (ONE TIME after drop)
            // Only log when count changes to avoid spam
            static size_t s_lastNodeCount = 0;
            const auto& nodes = graphDoc->GetNodes();
            if (nodes.size() != s_lastNodeCount)
            {
                SYSTEM_LOG << "[BehaviorTreeImNodesAdapter::RenderNodes] DIAGNOSTIC: graphDoc=" 
                           << static_cast<void*>(graphDoc) << " nodeCount=" << nodes.size() 
                           << " m_graphId=" << m_graphId << " (NodeGraph manager)\n";
                s_lastNodeCount = nodes.size();
            }

            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const auto& node = nodes[i];
                int nodeUid = static_cast<int>(node.id.value);

                ImNodes::BeginNode(nodeUid);

                // Input pin (every node can accept input)
                {
                    int inputPinId = nodeUid * 1000;
                    ImNodes::BeginInputAttribute(inputPinId);
                    ImGui::TextUnformatted("In");
                    ImNodes::EndInputAttribute();
                }

                ImGui::Separator();

                // Node title
                ImGui::TextUnformatted(node.name.c_str());
                ImGui::Separator();
                ImGui::TextUnformatted(node.type.c_str());

                ImGui::Separator();

                // Output pin (every node can have output)
                {
                    int outputPinId = nodeUid * 1000 + 1;
                    ImNodes::BeginOutputAttribute(outputPinId);
                    ImGui::TextUnformatted("Out");
                    ImNodes::EndOutputAttribute();
                }

                // Set node position
                ImNodes::SetNodeGridSpacePos(nodeUid, ImVec2(node.position.x, node.position.y));

                ImNodes::EndNode();
            }
        }

        void RenderConnections()
        {
            // Get active graph from NodeGraphManager
            GraphDocument* graphDoc = NodeGraphManager::Get().GetActiveGraph();
            if (!graphDoc)
            {
                // Phase 59 FIX: Removed frame-by-frame logging (violates COPILOT_INSTRUCTIONS)
                // This method is called 60 times per second, so logging here creates console spam
                return;
            }

            const auto& links = graphDoc->GetLinks();

            for (size_t i = 0; i < links.size(); ++i)
            {
                const auto& link = links[i];
                int linkUid = static_cast<int>(i);  // Use index as link ID

                // Convert pin IDs to attribute IDs in our mapping
                // Pin ID formula: nodeId * 1000 + pinOffset (0 for input, 1 for output)
                int fromPinValue = static_cast<int>(link.fromPin.value);
                int toPinValue = static_cast<int>(link.toPin.value);

                ImNodes::Link(linkUid, fromPinValue, toPinValue);
            }
        }

        void UpdateCanvasInteraction()
        {
            // ImNodes handles pan/zoom/selection internally via EditorContext
            // When called within BeginNodeEditor/EndNodeEditor scope,
            // ImNodes automatically manages canvas interaction.
            // No explicit implementation required for basic functionality.

            // Note: For custom interactions (pan limits, zoom constraints),
            // retrieve state via ImNodes::EditorContextGet() / ImNodes::EditorContextSet()
        }
    };
}
