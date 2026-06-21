/**
 * @file BehaviorTreeDebugWindow_NodeGraph.cpp
 * @brief NodeGraph debug helpers for BehaviorTreeDebugWindow
 *
 * Restores a read-only node graph view in the center panel of the BT Debugger.
 * This file provides three methods used by BehaviorTreeDebugWindow:
 *   - InitNodeGraphDebugMode()
 *   - ShutdownNodeGraphDebugMode()
 *   - RenderNodeGraphDebugPanel()
 *
 * The implementation uses NodeGraphManager to load the graph from the BehaviorTree
 * path (if available) into a GraphId and displays it via NodeGraphPanel in a
 * non-editable/read-only mode (suppressing tabs and modals).
 */

#include "BehaviorTreeDebugWindow.h"
#include "../World.h"
#include "../ECS_Components.h"
#include "../ECS_Components_AI.h"
#include "../NodeGraphCore/NodeGraphManager.h"
#include "../BlueprintEditor/BTNodeGraphManager.h"
#include <iostream>

namespace Olympe
{

	void BehaviorTreeDebugWindow::InitNodeGraphDebugMode()
	{
		// Initialize the NodeGraphPanel instance used for rendering the read-only graph
		m_nodeGraphPanel.Initialize();
		m_nodeGraphPanel.m_SuppressGraphTabs = true; // single-view read-only
		m_nodeGraphPanel.m_SuppressLegacyModals = true;
		// Put panel into read-only mode so nodes/links cannot be edited
		m_nodeGraphPanel.SetReadOnly(true);

		// Clear any previous debug graph id
		m_debugGraphId = -1;

		std::cout << "[BTDebugger] NodeGraph debug mode initialized" << std::endl;
	}

	void BehaviorTreeDebugWindow::ShutdownNodeGraphDebugMode()
	{
		// Close any loaded debug graph
		if (m_debugGraphId != -1)
		{
			NodeGraph::GraphId gidClose; gidClose.value = static_cast<uint32_t>(m_debugGraphId);
			NodeGraph::NodeGraphManager::Get().CloseGraph(gidClose);
			m_debugGraphId = -1;
		}

		m_nodeGraphPanel.Shutdown();

		std::cout << "[BTDebugger] NodeGraph debug mode shutdown" << std::endl;
	}

	void BehaviorTreeDebugWindow::RenderNodeGraphDebugPanel()
	{
		// If no entity selected or no tree loaded, show placeholder
		if (m_selectedEntity == 0)
		{
			ImGui::Text("No entity selected");
			return;
		}

		auto& world = World::Get();
		if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
		{
			ImGui::Text("Selected entity has no BehaviorTree runtime");
			return;
		}

		const auto& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
		uint32_t treeId = btRuntime.AITreeAssetId;

		// Ensure we have a graph loaded for this treeId. If different from last, reload.
		if (treeId != m_lastDebugTreeId)
		{
			m_lastDebugTreeId = treeId;

			// Close previous graph
			if (m_debugGraphId != -1)
			{
				NodeGraph::GraphId gidClose; gidClose.value = static_cast<uint32_t>(m_debugGraphId);
				NodeGraph::NodeGraphManager::Get().CloseGraph(gidClose);
				m_debugGraphId = -1;
			}

			std::string path = BehaviorTreeManager::Get().GetTreePathFromId(treeId);
			if (!path.empty())
			{
				auto gid = NodeGraph::NodeGraphManager::Get().LoadGraph(path);
				if (gid.value != 0)
				{
					m_debugGraphId = gid.value;
					// Set active graph to ensure NodeGraphPanel will render it
					NodeGraph::NodeGraphManager::Get().SetActiveGraph(gid);
				}
				else
				{
					std::cout << "[BTDebugger] Failed to load debug graph from " << path << std::endl;
				}
			}
			else
			{
				// No path found for this tree id
				m_debugGraphId = -1;
			}
		}

		// Render the graph if available
		if (m_debugGraphId != -1)
		{
			// Render content-only so it fits in the central child
			m_nodeGraphPanel.m_SuppressGraphTabs = true;

			NodeGraph::GraphId gid{ 0 }; gid.value = static_cast<uint32_t>(m_debugGraphId);
			// Only set active graph when it differs to avoid spamming logs
			if (NodeGraph::NodeGraphManager::Get().GetActiveGraphId().value != gid.value)
			{
				NodeGraph::NodeGraphManager::Get().SetActiveGraph(gid);
			}

			// Update highlighted/active node in NodeGraphPanel each frame for realtime visualization
			// Get current node index from runtime component if entity still valid
			if (m_selectedEntity != 0)
			{
				auto& world2 = World::Get();
				if (world2.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
				{
					const auto& rt = world2.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
					int activeLocalId = static_cast<int>(rt.AICurrentNodeIndex);
					m_nodeGraphPanel.SetActiveDebugNode(activeLocalId);
				}
			}

			m_nodeGraphPanel.RenderContent();
		}

		// Draw execution history overlay if graph is visible
		if (m_debugGraphId != -1 && !m_execHistory.empty())
		{
			NodeGraph::GraphId gid{ 0 }; gid.value = static_cast<uint32_t>(m_debugGraphId);
			GraphDocument* doc = NodeGraph::NodeGraphManager::Get().GetGraph(gid);
			if (doc)
			{
				ImDrawList* dl = ImGui::GetWindowDrawList();
				if (dl)
				{
					// Ensure we have a valid ImNodes editor context for the panel before
					// calling ImNodes APIs. If missing, skip overlay to avoid asserts.
					ImNodesEditorContext* editorCtx = m_nodeGraphPanel.GetImNodesEditorContext();
					if (!editorCtx)
					{
						std::cout << "[BTDebugger] Node overlay skipped: ImNodes editor context missing" << std::endl;
						return;
					}
					// Set the editor context so ImNodes internal calls use the correct editor
					ImNodes::EditorContextSet(editorCtx);
					// Build a map nodeId -> screen center
					std::unordered_map<uint32_t, ImVec2> nodeCenter;
					for (const auto& nd : doc->GetNodes())
					{
						int uid = gid.value * 10000 + nd.id.value;
						ImVec2 pos = ImNodes::GetNodeScreenSpacePos(uid);
						ImVec2 dim = ImNodes::GetNodeDimensions(uid);
						nodeCenter[nd.id.value] = ImVec2(pos.x + dim.x * 0.5f, pos.y + dim.y * 0.5f);
					}

					// Draw history as gradient dots and connecting lines
					int idx = 0;
					for (auto it = m_execHistory.rbegin(); it != m_execHistory.rend(); ++it)
					{
						uint32_t nid = it->second;
						auto ncIt = nodeCenter.find(nid);
						if (ncIt == nodeCenter.end()) continue;
						ImVec2 p = ncIt->second;

						float t = 1.0f - (static_cast<float>(idx) / static_cast<float>(MAX_EXEC_HISTORY));
						int alpha = static_cast<int>(t * 200.0f) + 55;
						ImU32 col = IM_COL32(255, 100, 30, alpha);

						// Draw small circle
						dl->AddCircleFilled(p, 6.0f * t + 2.0f, col);

						// Connect to previous point
						if (idx > 0)
						{
							// previous point is the last drawn point's center
							// find prev
							auto pit = std::next(it);
							if (pit != m_execHistory.rend())
							{
								auto pncIt = nodeCenter.find(pit->second);
								if (pncIt != nodeCenter.end())
								{
									dl->AddLine(p, pncIt->second, IM_COL32(255,150,60, alpha/2), 2.0f * t);
								}
							}
						}

						idx++;
					}
				}
			}
		}
		else
		{
			ImGui::Text("Behavior tree graph not available for this entity");
		}
	}

} // namespace Olympe
