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

	static int s_debugGraphId = -1;
	static uint32_t s_lastDebugTreeId = 0;

	void BehaviorTreeDebugWindow::InitNodeGraphDebugMode()
	{
		// Initialize the NodeGraphPanel instance used for rendering the read-only graph
		m_nodeGraphPanel.Initialize();
		m_nodeGraphPanel.m_SuppressGraphTabs = true; // single-view read-only
		m_nodeGraphPanel.m_SuppressLegacyModals = true;
		// Put panel into read-only mode so nodes/links cannot be edited
		m_nodeGraphPanel.SetReadOnly(true);

		// Clear any previous debug graph id
		s_debugGraphId = -1;

		std::cout << "[BTDebugger] NodeGraph debug mode initialized" << std::endl;
	}

	void BehaviorTreeDebugWindow::ShutdownNodeGraphDebugMode()
	{
		// Close any loaded debug graph
		if (s_debugGraphId != -1)
		{
			NodeGraph::GraphId gidClose; gidClose.value = static_cast<uint32_t>(s_debugGraphId);
			NodeGraph::NodeGraphManager::Get().CloseGraph(gidClose);
			s_debugGraphId = -1;
		}

		m_nodeGraphPanel.Shutdown();

		std::cout << "[BTDebugger] NodeGraph debug mode shutdown" << std::endl;
	}

	void BehaviorTreeDebugWindow::RenderNodeGraphDebugPanel()
	{
		if (m_nextGraphRefreshTime > 0.0f)
		{
			ImGui::Text("Graph refresh throttled");
			return;
		}

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
		if (treeId != s_lastDebugTreeId)
		{
			s_lastDebugTreeId = treeId;

			// Close previous graph
			if (s_debugGraphId != -1)
			{
				NodeGraph::GraphId gidClose; gidClose.value = static_cast<uint32_t>(s_debugGraphId);
				NodeGraph::NodeGraphManager::Get().CloseGraph(gidClose);
				s_debugGraphId = -1;
			}

			std::string path = BehaviorTreeManager::Get().GetTreePathFromId(treeId);
			if (!path.empty())
			{
				auto gid = NodeGraph::NodeGraphManager::Get().LoadGraph(path);
				if (gid.value != 0)
				{
					s_debugGraphId = gid.value;
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
			s_debugGraphId = -1;
			}
		}

		// Render the graph if available
		if (s_debugGraphId != -1)
		{
			// Render content-only so it fits in the central child
			m_nodeGraphPanel.m_SuppressGraphTabs = true;

			NodeGraph::GraphId gid{ 0 }; gid.value = static_cast<uint32_t>(s_debugGraphId);
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
		if (s_debugGraphId != -1)
		{
			NodeGraph::GraphId gid{ 0 }; gid.value = static_cast<uint32_t>(s_debugGraphId);
			GraphDocument* doc = NodeGraph::NodeGraphManager::Get().GetGraph(gid);
			if (doc && m_selectedEntity != 0)
			{
				auto& world = World::Get();
				if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
					return;

				const auto& rt = world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);
				if (rt.debugNodeTrace.empty())
					return;

				bool needOverlayRebuild =
					m_cachedOverlayPoints.empty() ||
					m_cachedOverlayGraphId != s_debugGraphId ||
					m_cachedOverlayEntity != m_selectedEntity ||
					(m_nextOverlayRefreshTime <= 0.0f);

				if (needOverlayRebuild)
				{
					m_cachedOverlayPoints.clear();
					m_cachedNodeCenters.clear();
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
					// Build a cache nodeId -> screen center
					for (const auto& nd : doc->GetNodes())
					{
						int uid = gid.value * 10000 + nd.id.value;
						ImVec2 pos = ImNodes::GetNodeScreenSpacePos(uid);
						ImVec2 dim = ImNodes::GetNodeDimensions(uid);
						m_cachedNodeCenters[nd.id.value] = std::make_pair(pos.x + dim.x * 0.5f, pos.y + dim.y * 0.5f);
					}

					// Cache history as compact draw primitives.
					int idx = 0;
					bool havePrev = false;
					float prevX = 0.0f;
					float prevY = 0.0f;
					for (size_t i = 0; i < rt.debugNodeTrace.size(); ++i)
					{
						uint32_t nid = rt.debugNodeTrace[i];
						auto ncIt = m_cachedNodeCenters.find(nid);
						if (ncIt == m_cachedNodeCenters.end()) continue;
						float px = ncIt->second.first;
						float py = ncIt->second.second;

						float t = static_cast<float>(idx + 1) / static_cast<float>(MAX_EXEC_HISTORY);
						t = std::min(1.0f, std::max(0.05f, t));
						int alpha = static_cast<int>(t * 200.0f) + 55;
						CachedOverlayPoint point;
						point.x = px;
						point.y = py;
						point.radius = 2.0f + 6.0f * t;
						point.color = IM_COL32(255, 100, 30, alpha);
						point.lineThickness = 1.5f + 2.0f * t;
						point.connectFromPrevious = havePrev;
						m_cachedOverlayPoints.push_back(point);

						havePrev = true;
						++idx;
					}
					m_cachedOverlayGraphId = s_debugGraphId;
					m_cachedOverlayEntity = m_selectedEntity;
					m_overlayCacheDirty = false;
					m_nextOverlayRefreshTime = m_overlayRefreshInterval;
				}

				ImDrawList* dl = ImGui::GetWindowDrawList();
				if (dl)
				{
					// Render the cached primitives.
					ImVec2 prevPoint = ImVec2(0,0);
					bool havePrev = false;
						ImU32 highlightColor = IM_COL32(255, 170, 60, 120);
					for (const auto& point : m_cachedOverlayPoints)
					{
						ImVec2 p(point.x, point.y);
						dl->AddCircleFilled(p, point.radius, point.color);
						if (havePrev && point.connectFromPrevious)
						{
								dl->AddLine(prevPoint, p, highlightColor, point.lineThickness);
						}
						prevPoint = p;
						havePrev = true;
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
