/*
 * Olympe Blueprint Editor - Node Graph Panel
 * 
 * GUI panel for visual node graph editing using ImNodes
 * Supports Behavior Trees and HFSM editing
 */

#pragma once

#include <string>
#include <vector>
#include "NodeGraphManager.h"

namespace Olympe
{
    /**
     * NodeGraphPanel - ImGui/ImNodes panel for node graph editing
     * Provides visual editor for behavior trees and state machines
     */
    class NodeGraphPanel
    {
    public:
        NodeGraphPanel();
        ~NodeGraphPanel();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        void RenderGraphTabs();
        void RenderGraph();
        void RenderContextMenu();
        void RenderNodeProperties();
        void RenderNodeEditModal();
        void HandleKeyboardShortcuts();

        // Node creation helpers
        void CreateNewNode(const char* nodeType, float x, float y);
        
        // Helper to generate unique ImGui IDs per graph to avoid conflicts
        static int GetUniqueImGuiID(int graphId, int nodeId) {
            // Offset unique per graph to avoid conflicts when multiple graphs are open
            return graphId * 1000000 + nodeId;
        }
        
        static int GetOriginalNodeId(int uniqueId) {
            // Extract original node ID from unique ID
            return uniqueId % 1000000;
        }

        // ImNodes state
        int m_SelectedNodeId = -1;
        int m_SelectedLinkId = -1;
        bool m_ShowContextMenu = false;
        float m_ContextMenuPosX = 0.0f;
        float m_ContextMenuPosY = 0.0f;
        
        // Node editing modal
        bool m_ShowNodeEditModal = false;
        GraphNode* m_EditingNode = nullptr;
        int m_EditingNodeId = -1;
        char m_NodeNameBuffer[256];
        
        // For tracking node movement (undo/redo)
        bool m_NodeDragStarted = false;
        int m_DraggedNodeId = -1;
        float m_DragStartX = 0.0f;
        float m_DragStartY = 0.0f;
    };
}
