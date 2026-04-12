/**
 * @file BTNodePropertyPanel.h
 * @brief Property editor panel for BehaviorTree nodes
 * @author Olympe Engine
 * @date 2026-04-08
 *
 * @details
 * Displays and allows editing of properties for the currently selected BT node.
 * Supports string, int, and float parameter types.
 * Mirrors EntityPrefab's PropertyEditorPanel pattern for consistency.
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe {

// Forward declarations
struct GraphNode;
class BTNodeGraphManager;

/**
 * @class BTNodePropertyPanel
 * @brief UI panel for editing BehaviorTree node properties
 *
 * Renders property controls for the currently selected node.
 * Updates the BTNodeGraphManager when properties are changed.
 */
class BTNodePropertyPanel
{
public:
    BTNodePropertyPanel();
    ~BTNodePropertyPanel() = default;

    /**
     * @brief Initialize the panel
     */
    void Initialize();

    /**
     * @brief Render the property panel
     */
    void Render();

    /**
     * @brief Set the currently selected node ID
     * @param graphId Active graph ID in BTNodeGraphManager
     * @param nodeId Local node ID within the graph
     */
    void SetSelectedNode(int graphId, int nodeId);

    /**
     * @brief Clear the current selection
     */
    void ClearSelection();

    /**
     * @brief Check if a node is currently selected
     * @return true if a node is selected
     */
    bool HasSelectedNode() const { return m_selectedNodeId >= 0; }

    // Public access to selected node ID for synchronization
    int m_selectedNodeId = -1;

private:
    int m_activeGraphId = -1;          ///< Current graph ID in BTNodeGraphManager
    char m_nodeNameBuffer[256] = {0}; ///< Buffer for node name editing
    char m_paramBuffer[512] = {0};    ///< Buffer for parameter editing

    // Rendering helpers
    void RenderNodeBasicInfo(const GraphNode* node);
    void RenderNodeParameters(const GraphNode* node);
    void RenderSubGraphControls(GraphNode* node);  ///< Phase 39c: SubGraph node editor
    void ApplyNodeChanges(GraphNode* node);
};

} // namespace Olympe
