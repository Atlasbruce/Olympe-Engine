/**
 * @file BehaviorTreeDebugWindow_NodeGraph.cpp
 * @brief NodeGraph debug panel methods for BehaviorTreeDebugWindow (F10)
 *
 * @details
 * Implements the three new methods added to BehaviorTreeDebugWindow:
 *   - InitNodeGraphDebugMode()
 *   - ShutdownNodeGraphDebugMode()
 *   - RenderNodeGraphDebugPanel()
 *
 * These replace the legacy RenderNodeGraphPanel() / RenderBehaviorTreeGraph()
 * rendering path with the full Blueprint Editor pipeline:
 * BTGraphDocumentConverter -> Olympe::NodeGraph -> NodeGraphPanel::RenderGraph()
 *
 * The NodeGraph registered here is READ-ONLY (Runtime EditorContext mode).
 * No modifications are written back to the BehaviorTreeAsset.
 */

#include "BehaviorTreeDebugWindow.h"
#include "../NodeGraphShared/BTGraphDocumentConverter.h"
#include "../BlueprintEditor/BTNodeGraphManager.h"
#include "../BlueprintEditor/NodeGraphPanel.h"
#include "../BlueprintEditor/EditorContext.h"
#include "BehaviorTree.h"
#include "../World.h"
#include "../ECS_Components_AI.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>

namespace Olympe {

// ---------------------------------------------------------------------------
// InitNodeGraphDebugMode
// ---------------------------------------------------------------------------

void BehaviorTreeDebugWindow::InitNodeGraphDebugMode()
{
    // Set EditorContext to Runtime mode (read-only — no create/edit/delete).
    EditorContext::Get().InitializeRuntime();

    m_nodeGraphPanel.Initialize();

    m_debugGraphId     = -1;
    m_lastDebugTreeId  = 0;

    SYSTEM_LOG << "[BTDebugWindow] NodeGraph debug mode initialized (Runtime/Read-Only)"
               << std::endl;
}

// ---------------------------------------------------------------------------
// ShutdownNodeGraphDebugMode
// ---------------------------------------------------------------------------

void BehaviorTreeDebugWindow::ShutdownNodeGraphDebugMode()
{
    if (m_debugGraphId >= 0)
    {
        NodeGraphManager::Get().CloseGraph(m_debugGraphId);
        m_debugGraphId = -1;
    }

    m_nodeGraphPanel.Shutdown();

    SYSTEM_LOG << "[BTDebugWindow] NodeGraph debug mode shutdown" << std::endl;
}

// ---------------------------------------------------------------------------
// RenderNodeGraphDebugPanel
// ---------------------------------------------------------------------------

void BehaviorTreeDebugWindow::RenderNodeGraphDebugPanel()
{
    // 1. Guard: entity must be selected
    if (m_selectedEntity == 0)
    {
        ImGui::TextDisabled("Select an entity from the list to view its behavior tree");
        return;
    }

    auto& world = World::Get();
    if (!world.HasComponent<BehaviorTreeRuntime_data>(m_selectedEntity))
    {
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f),
            "Selected entity has no BehaviorTreeRuntime_data");
        m_selectedEntity = 0;
        return;
    }

    const auto& btRuntime =
        world.GetComponent<BehaviorTreeRuntime_data>(m_selectedEntity);

    const BehaviorTreeAsset* tree =
        BehaviorTreeManager::Get().GetTreeByAnyId(btRuntime.AITreeAssetId);

    if (!tree)
    {
        ImGui::TextColored(ImVec4(1.f, 0.5f, 0.f, 1.f),
            "BehaviorTree asset not found (ID: %u)", btRuntime.AITreeAssetId);
        return;
    }

    // 2. Reload graph when the tree changes
    if (btRuntime.AITreeAssetId != m_lastDebugTreeId)
    {
        if (m_debugGraphId >= 0)
        {
            NodeGraphManager::Get().CloseGraph(m_debugGraphId);
            m_debugGraphId = -1;
        }

        NodeGraph* converted =
            NodeGraphShared::BTGraphDocumentConverter::FromBehaviorTree(tree);

        if (!converted)
        {
            ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                "Failed to convert BT graph");
            return;
        }

        // Register the converted graph with the Blueprint Editor's NodeGraphManager
        m_debugGraphId = NodeGraphManager::Get().CreateGraph(tree->name, "BehaviorTree");

        NodeGraph* managed = NodeGraphManager::Get().GetGraph(m_debugGraphId);
        if (managed)
            *managed = std::move(*converted);

        delete converted;
        converted = nullptr;

        NodeGraphManager::Get().SetActiveGraph(m_debugGraphId);
        m_lastDebugTreeId = btRuntime.AITreeAssetId;

        SYSTEM_LOG << "[BTDebugWindow] Loaded graph for tree '" << tree->name
                   << "' (entity " << m_selectedEntity << ")" << std::endl;
    }
    else
    {
        // Keep the graph active across frames
        NodeGraphManager::Get().SetActiveGraph(m_debugGraphId);
    }

    // 3. Sync active node highlight each frame (no-op on NodeGraph side;
    //    SetActiveDebugNode drives the visual highlight in NodeGraphPanel)
    NodeGraphShared::BTGraphDocumentConverter::SyncActiveNode(
        NodeGraphManager::Get().GetGraph(m_debugGraphId),
        btRuntime.AICurrentNodeIndex);

    NodeGraphPanel::SetActiveDebugNode(
        static_cast<int>(btRuntime.AICurrentNodeIndex));

    // 4. Runtime status banner
    const char* statusStr =
        btRuntime.lastStatus == 0 ? "Running" :
        btRuntime.lastStatus == 1 ? "Success" : "Failure";

    ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f),
        "Tree: %s  |  Active node: #%u  |  Status: %s",
        tree->name.c_str(),
        btRuntime.AICurrentNodeIndex,
        statusStr);
    ImGui::Separator();

    // 5. Render via NodeGraphPanel (Blueprint Editor pipeline)
    //    RenderGraph() embeds directly in the current child panel without
    //    creating a floating window (unlike Render()).
    m_nodeGraphPanel.RenderGraph();
}

} // namespace Olympe
