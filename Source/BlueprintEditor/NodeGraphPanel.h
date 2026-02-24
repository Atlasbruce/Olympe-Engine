/*
 * Olympe Blueprint Editor - Node Graph Panel
 * 
 * GUI panel for visual node graph editing using ImNodes
 * Supports Behavior Trees and HFSM editing
 */

#pragma once

#include <string>
#include <vector>
#include "BTNodeGraphManager.h"

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

        void RenderGraphTabs();
        void RenderGraph();
        void RenderContextMenu();
        void RenderNodeProperties();
        void RenderNodeEditModal();
        void HandleKeyboardShortcuts();
        void HandleNodeInteractions(int graphID);

        /**
         * Render a single node with a coloured title bar, icon, and typed pins.
         * Called from RenderGraph() for every node in the active graph.
         * @param node         Pointer to the graph node to render.
         * @param globalNodeUID ImNodes global unique ID for the node.
         * @param graphID      Active graph ID (used for debug-highlight check).
         */
        void RenderNodePinsAndContent(GraphNode* node, int globalNodeUID, int graphID);

        /**
         * Render a single typed attribute pin using ImDrawList shapes.
         * @param attrId  ImNodes attribute ID.
         * @param label   Text label shown next to the pin.
         * @param isInput True for input (left) attribute; false for output (right).
         * @param isExec  True to use the exec (triangle) pin shape; false for data (circle).
         */
        void RenderTypedPin(int attrId, const char* label, bool isInput, bool isExec);

        // Node creation helpers
        void CreateNewNode(const char* nodeType, float x, float y);
        
        // Helper for converting global UID to local node ID
        int GlobalUIDToLocalNodeID(int globalUID, int graphID) const
        {
            return globalUID - (graphID * 10000);  // 10000 is GRAPH_ID_MULTIPLIER
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

        // Context-menu fuzzy search buffer
        char m_ContextMenuSearch[128];

        // Runtime debug overlay: set to the local node ID that is currently
        // executing (-1 means none).  Highlighted with a coloured title bar tint.
        int m_ActiveDebugNodeId = -1;
    };
}
