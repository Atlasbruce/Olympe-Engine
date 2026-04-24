/**
 * @file BTGraphDocumentConverter.h
 * @brief Conversion BehaviorTreeAsset -> Olympe::NodeGraph (Blueprint Editor pipeline)
 *
 * @details
 * Bridge between the runtime BT (BehaviorTreeAsset) and the Blueprint Editor
 * rendering system (Olympe::NodeGraph + Olympe::NodeGraphManager).
 * Used exclusively by BehaviorTreeDebugWindow (F10) in debug mode.
 * Conversion READ-ONLY: no writes from the debugger back to the BT runtime.
 *
 * LEGACY: BTGraphDocumentConverter is not used by Phase 50.3
 * Disabled to resolve namespace conflicts (NodeGraph used as both class and namespace)
 *
 * Two-priority loading strategy (kept for reference):
 *  1. **JSON source file** (preferred): queries `BehaviorTreeManager::GetTreePathFromId()`
 *     to obtain the asset path, then calls `NodeGraphManager::LoadGraph()` to parse the
 *     file.  This preserves the visual positions saved in the JSON, making the debugger
 *     layout identical to the Blueprint Editor standalone view.  `ClearDirty()` is called
 *     on the resulting graph because the debugger is read-only.  The temporary graph slot
 *     in `NodeGraphManager` is closed immediately after cloning.
 *  2. **BFS fallback**: used when the path is unknown (prefixed `"TreeName:"`) or when
 *     `LoadGraph` returns -1.  Positions are computed by `BTGraphLayoutEngine`.
 */

namespace Olympe {
namespace NodeGraphShared {

class BTGraphDocumentConverter
{
public:
    // DISABLED - LEGACY methods:
    // static NodeGraph* FromBehaviorTree(const BehaviorTreeAsset* tree);
    // static void SyncActiveNode(NodeGraph* graph, uint32_t currentNodeId);

private:
    BTGraphDocumentConverter() = delete;
};

} // namespace NodeGraphShared
} // namespace Olympe
