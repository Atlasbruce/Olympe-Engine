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
#include <cstring>
#include "../../Source/third_party/imgui/imgui.h"

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
    BTNodePropertyPanel()
        : m_activeGraphId(-1)
        , m_selectedNodeId(-1)
    {
        std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
        std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
    }

    ~BTNodePropertyPanel() = default;

    /**
     * @brief Initialize the panel
     */
    void Initialize()
    {
        // Initialize property panel
    }

    /**
     * @brief Render the property panel
     */
    void Render()
    {
        if (m_activeGraphId < 0 || m_selectedNodeId < 0)
        {
            ImGui::TextDisabled("No node selected");
            return;
        }

        ImGui::TextDisabled("BTNodePropertyPanel - Phase 50.3");
        ImGui::TextDisabled("Graph ID: %d, Node ID: %d", m_activeGraphId, m_selectedNodeId);
    }

    /**
     * @brief Set the currently selected node ID
     * @param graphId Active graph ID in BTNodeGraphManager
     * @param nodeId Local node ID within the graph
     */
    void SetSelectedNode(int graphId, int nodeId)
    {
        m_activeGraphId = graphId;
        m_selectedNodeId = nodeId;
    }

    /**
     * @brief Clear the current selection
     */
    void ClearSelection()
    {
        m_activeGraphId = -1;
        m_selectedNodeId = -1;
        std::memset(m_nodeNameBuffer, 0, sizeof(m_nodeNameBuffer));
        std::memset(m_paramBuffer, 0, sizeof(m_paramBuffer));
    }

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
    void RenderSubGraphControls(GraphNode* node);       ///< Phase 39c: SubGraph node editor
    void RenderSubGraphBindingEditor(GraphNode* node);  ///< Phase 39c Step 4: Parameter binding editor
    void ApplyNodeChanges(GraphNode* node);
};

} // namespace Olympe
