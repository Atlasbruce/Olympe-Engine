/**
 * @file BTDebugAdapter.h
 * @brief Debug adapter for BehaviorTree runtime visualization
 * @author Olympe Engine - NodeGraphShared
 * @date 2025-02-19
 *
 * @details
 * Provides a unified interface to visualize BehaviorTree runtime state.
 * 
 * NOTE: This adapter is part of the planned unified architecture migration.
 * Currently not actively used - see BehaviorTreeDebugWindow_UnifiedMigration_Plan.md
 * 
 * Future integration will enable:
 * - Read-only visualization of runtime trees via NodeGraphRenderer
 * - Active node highlighting with pulse animation
 * - Consistent rendering with standalone editor
 * - Zero-copy lazy conversion (BT → GraphDocument on demand)
 */

#pragma once

// Forward declarations to avoid circular dependencies
namespace Olympe { namespace NodeGraph { class NodeGraphRenderer; class GraphDocument; struct RenderConfig; } }
class BehaviorTreeAsset;
class BTGraphLayoutEngine;
struct BTNodeLayout;

#include <memory>
#include <cstdint>
#include <vector>

namespace Olympe
{
namespace NodeGraphShared
{

/**
 * @class BTDebugAdapter
 * @brief Adapter for visualizing BehaviorTree runtime using unified renderer
 *
 * Usage in BehaviorTreeDebugWindow:
 * @code
 * // Setup (once per tree load)
 * BTDebugAdapter adapter(&tree, &layoutEngine);
 * adapter.Initialize(&renderer);
 * 
 * // Update active node each frame
 * adapter.SetActiveNode(btRuntime.AICurrentNodeIndex);
 * 
 * // Render
 * adapter.Render(deltaTime);
 * @endcode
 */
class BTDebugAdapter
{
public:
    /**
     * @brief Construct a debug adapter for a behavior tree
     * @param tree BehaviorTree runtime asset to visualize
     * @param layoutEngine Layout engine for node positioning
     */
    BTDebugAdapter(
        const BehaviorTreeAsset* tree,
        BTGraphLayoutEngine* layoutEngine
    );
    
    ~BTDebugAdapter();
    
    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    /**
     * @brief Initialize the adapter with a renderer instance
     * @param renderer NodeGraphRenderer to use (must outlive adapter)
     * @param config Optional render configuration (nullptr = use defaults)
     */
    void Initialize(
        NodeGraph::NodeGraphRenderer* renderer,
        const NodeGraph::RenderConfig* config = nullptr
    );
    
    /**
     * @brief Shutdown and cleanup resources
     */
    void Shutdown();
    
    /**
     * @brief Check if adapter is ready to render
     */
    bool IsInitialized() const { return m_renderer != nullptr; }
    
    // ========================================================================
    // Runtime State
    // ========================================================================
    
    /**
     * @brief Set the currently executing node (for highlighting)
     * @param nodeId ID of active node (0 = none)
     */
    void SetActiveNode(uint32_t nodeId);
    
    /**
     * @brief Clear all runtime highlighting
     */
    void ClearRuntimeState();
    
    /**
     * @brief Get current active node ID
     */
    uint32_t GetActiveNodeId() const { return m_activeNodeId; }
    
    // ========================================================================
    // Layout & View Control
    // ========================================================================
    
    /**
     * @brief Recompute layout (call after tree structure changes)
     * @param nodeSpacingX Horizontal spacing between nodes
     * @param nodeSpacingY Vertical spacing between nodes
     * @param zoomFactor Current zoom level
     */
    void RecomputeLayout(float nodeSpacingX, float nodeSpacingY, float zoomFactor);
    
    /**
     * @brief Update a single node position (user drag in editor mode)
     * @param nodeId Node to move
     * @param x New X position
     * @param y New Y position
     * @return true if position updated successfully
     */
    bool UpdateNodePosition(uint32_t nodeId, float x, float y);
    
    /**
     * @brief Fit graph to view (auto-zoom)
     */
    void FitToView();
    
    /**
     * @brief Center view on active node
     */
    void CenterOnActiveNode();
    
    // ========================================================================
    // Rendering
    // ========================================================================
    
    /**
     * @brief Render the behavior tree using unified pipeline
     * @param deltaTime Time since last frame (for animations)
     */
    void Render(float deltaTime);
    
    // ========================================================================
    // Editor Mode (Optional)
    // ========================================================================
    
    /**
     * @brief Enable editor mode (allows modifications via commands)
     * @param commandStack Command stack for undo/redo (nullptr = read-only)
     */
    void SetEditorMode(bool enabled);
    
    /**
     * @brief Check if adapter is in editor mode
     */
    bool IsEditorMode() const { return m_editorMode; }
    
private:
    // Data conversion (lazy)
    void EnsureGraphDocumentConverted();
    void ConvertBehaviorTreeToDocument();
    void SyncLayoutToDocument();
    
    // Rendering helpers
    void ApplyRuntimeHighlighting();
    
    // State
    const BehaviorTreeAsset* m_tree;
    BTGraphLayoutEngine* m_layoutEngine;
    NodeGraph::NodeGraphRenderer* m_renderer;
    
    // Converted document (lazy-initialized)
    std::unique_ptr<NodeGraph::GraphDocument> m_document;
    bool m_documentDirty;
    
    // Runtime state
    uint32_t m_activeNodeId;
    bool m_editorMode;
    
    // Layout cache
    std::vector<BTNodeLayout> m_currentLayout;
    float m_lastNodeSpacingX;
    float m_lastNodeSpacingY;
    float m_lastZoomFactor;
};

} // namespace NodeGraphShared
} // namespace Olympe
