/**
 * @file BTGraphDocumentConverter.h
 * @brief Conversion BehaviorTreeAsset -> Olympe::NodeGraph (Blueprint Editor pipeline)
 *
 * @details
 * Bridge between the runtime BT (BehaviorTreeAsset) and the Blueprint Editor
 * rendering system (Olympe::NodeGraph + Olympe::NodeGraphManager).
 * Used exclusively by BehaviorTreeDebugWindow (F10) in debug mode.
 * Conversion READ-ONLY: no writes from the debugger back to the BT runtime.
 */

#pragma once

#include "../BlueprintEditor/BTNodeGraphManager.h"
#include "../AI/BehaviorTree.h"
#include <cstdint>

namespace Olympe {
namespace NodeGraphShared {

/**
 * @class BTGraphDocumentConverter
 * @brief Converts a BehaviorTreeAsset into an Olympe::NodeGraph for display
 *        in the NodeGraphPanel (Blueprint Editor pipeline).
 *
 * The resulting NodeGraph preserves the original BT node IDs so that
 * NodeGraphPanel::SetActiveDebugNode() works directly with BT node indices.
 */
class BTGraphDocumentConverter
{
public:
    /**
     * @brief Converts a BehaviorTreeAsset into a heap-allocated NodeGraph.
     *
     * @details Two-priority loading strategy:
     *  1. **JSON source file** (preferred): queries `BehaviorTreeManager::GetTreePathFromId()`
     *     to obtain the asset path, then calls `NodeGraphManager::LoadGraph()` to parse the
     *     file.  This preserves the visual positions saved in the JSON, making the debugger
     *     layout identical to the Blueprint Editor standalone view.  `ClearDirty()` is called
     *     on the resulting graph because the debugger is read-only.  The temporary graph slot
     *     in `NodeGraphManager` is closed immediately after cloning.
     *  2. **BFS fallback**: used when the path is unknown (prefixed `"TreeName:"`) or when
     *     `LoadGraph` returns -1.  Positions are computed by `BTGraphLayoutEngine`.
     *
     * @param tree  BehaviorTree asset to convert (non-null).
     * @return Newly allocated NodeGraph (caller takes ownership).
     *         Returns nullptr if @p tree is null.
     */
    static NodeGraph* FromBehaviorTree(const BehaviorTreeAsset* tree);

    /**
     * @brief Synchronises the active-node metadata in an existing NodeGraph.
     * @details No-op for the NodeGraph model; actual highlight is driven by
     *          NodeGraphPanel::SetActiveDebugNode().  Kept for API symmetry.
     * @param graph         Target graph (may be null — safely ignored).
     * @param currentNodeId BT node currently executing.
     */
    static void SyncActiveNode(NodeGraph* graph, uint32_t currentNodeId);

private:
    BTGraphDocumentConverter() = delete;
};

} // namespace NodeGraphShared
} // namespace Olympe
