/*
 * Olympe Blueprint Editor - Node Graph Panel
 * 
 * GUI panel for visual node graph editing using ImNodes
 * Supports Behavior Trees and HFSM editing
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include "BTNodeGraphManager.h"
#include "../EditorCommon/EditorAutosaveManager.h"

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

        /**
         * @brief Overlay glow-coloured lines on links that connect to/from the
         *        active debug node, giving a visual "active link" indicator.
         *
         * Must be called AFTER ImNodes::EndNodeEditor() so that node screen-space
         * positions are valid.
         *
         * @param graph    Active NodeGraph.
         * @param graphID  Active graph ID.
         */
        void RenderActiveLinks(NodeGraph* graph, int graphID);

        // Node creation helpers
        void CreateNewNode(const char* nodeType, float x, float y);
        
        // Helper for converting global UID to local node ID
        int GlobalUIDToLocalNodeID(int globalUID, int graphID) const
        {
            return globalUID - (graphID * 10000);  // 10000 is GRAPH_ID_MULTIPLIER
        }

        // -----------------------------------------------------------------------
        // Runtime debug overlay
        // -----------------------------------------------------------------------

        /**
         * @brief Set the local node ID that is currently executing.
         *        Pass -1 to clear the highlight.
         *        The value is stored in a static so it is visible to all panel
         *        instances and to the WorldBridge callback.
         */
        static void SetActiveDebugNode(int localNodeId);

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

        // -----------------------------------------------------------------------
        // Minimap
        // -----------------------------------------------------------------------

        /// When true the built-in ImNodes minimap is rendered in the bottom-right
        /// corner of the node editor canvas.
        bool m_ShowMinimap = true;

        // -----------------------------------------------------------------------
        // Snap-to-grid
        // -----------------------------------------------------------------------

        /// When true node positions are rounded to the nearest grid cell on move.
        bool m_SnapToGrid = false;

        /// Grid cell size in canvas units used when snap-to-grid is enabled.
        float m_SnapGridSize = 16.0f;

    private:
        /// Backing storage for SetActiveDebugNode: the local node ID currently
        /// executing (-1 = none).  Shared across all panel instances.
        static int s_ActiveDebugNodeId;

        /// Async autosave manager – persists node positions without blocking the UI.
        EditorAutosaveManager m_autosave;

        /// Tracks which global node UIDs have already had their ImNodes position
        /// initialised.  Prevents SetNodeGridSpacePos() from overriding user drags
        /// on subsequent frames.  Cleared whenever the active graph changes.
        std::unordered_set<int> m_positionedNodes;

        /// Graph ID that was active last frame; used to detect graph switches so
        /// m_positionedNodes can be cleared.
        int m_lastActiveGraphId = -1;
    };
}
