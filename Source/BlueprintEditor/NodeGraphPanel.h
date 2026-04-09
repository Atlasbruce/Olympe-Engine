/*
 * Olympe Blueprint Editor - Node Graph Panel
 * 
 * GUI panel for visual node graph editing using ImNodes
 * Supports Behavior Trees and HFSM editing
 *
 * @deprecated (Phase 7) NodeGraphPanel is the legacy BT v2 editor.
 *   For new VS v4 graphs use VisualScriptEditorPanel.
 *   NodeGraphPanel is retained for BehaviorTreeDebugWindow (BT debug visualisation).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include "BTNodeGraphManager.h"
#include "../EditorCommon/EditorAutosaveManager.h"
#include "../third_party/imnodes/imnodes.h"
#include "Utilities/ImNodesCanvasEditor.h"

namespace Olympe
{
    // -------------------------------------------------------------------------
    // Phase 8: Subgraph tab descriptor
    // -------------------------------------------------------------------------

    /**
     * @struct GraphTab
     * @brief Represents one open tab in the NodeGraphPanel tab bar.
     *
     * The root graph always has graphPath == "root".
     * A subgraph tab has graphPath == "subgraphs/<uuid>".
     */
    struct GraphTab
    {
        std::string tabID;        ///< Unique identifier (UUID or "root")
        std::string displayName;  ///< Label shown on the tab ("Root" or subgraph name)
        std::string graphPath;    ///< "root" or "subgraphs/<uuid>"
        bool        isDirty;      ///< True when the graph has unsaved changes

        GraphTab() : isDirty(false) {}
        GraphTab(const std::string& id,
                 const std::string& name,
                 const std::string& path)
            : tabID(id), displayName(name), graphPath(path), isDirty(false) {}
    };

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
        void RenderContent();  // Render without window wrapper - for fixed layout

        void RenderGraphTabs();
        void RenderGraph();
        void RenderContextMenu();
        void RenderNodeProperties();
        void RenderNodeEditModal();
        void HandleKeyboardShortcuts();
        void HandleNodeInteractions(int graphID);

        // -----------------------------------------------------------------------
        // Phase 8: Subgraph tab system
        // -----------------------------------------------------------------------

        /**
         * @brief Renders the subgraph-aware tab bar above the graph canvas.
         *
         * Shows one tab per open subgraph plus the root graph.  A "+ New SubGraph"
         * button at the end creates an empty subgraph and opens it.
         */
        void RenderSubgraphTabBar();

        /**
         * @brief Opens (or focuses) the subgraph identified by @p subgraphUUID
         *        in a new tab.  Called on double-click of a BT_SubGraph node.
         *
         * @param subgraphUUID  UUID key in data.subgraphs.
         * @param displayName   Name shown on the tab.
         */
        void OpenSubgraphTab(const std::string& subgraphUUID,
                             const std::string& displayName);

        /**
         * @brief Closes the tab at @p index.  The root tab (index 0) cannot be closed.
         */
        void CloseSubgraphTab(int index);

        /**
         * @brief Creates an empty subgraph, inserts it into the active blueprint's
         *        data.subgraphs dict, and opens it in a new tab.
         *
         * @param name  Human-readable name for the new subgraph.
         */
        void CreateEmptySubgraph(const std::string& name);

        /**
         * @brief Returns the GraphTab for the currently active tab.
         */
        const GraphTab* GetActiveTab() const;

        /**
         * @brief Returns the subgraph UUID for the active tab, or empty string
         *        if the root graph is active.
         */
        std::string GetActiveSubgraphUUID() const;

        /**
         * @brief Render a single node with a coloured title bar, icon, and typed pins.
         * Called from RenderGraph() for every node in the active graph.
         * @param node             Pointer to the graph node to render.
         * @param globalNodeUID    ImNodes global unique ID for the node.
         * @param graphID          Active graph ID (used for debug-highlight check).
         * @param connectedAttrIDs Set of connected attribute IDs (for filled/outlined shapes).
         */
        void RenderNodePinsAndContent(GraphNode* node, int globalNodeUID, int graphID,
                                      const std::unordered_set<int>& connectedAttrIDs = {});

        /**
         * @brief Render a single typed attribute pin using ImDrawList shapes.
         * @param attrId           ImNodes attribute ID.
         * @param label            Text label shown next to the pin.
         * @param isInput          True for input (left) attribute; false for output (right).
         * @param isExec           True to use the exec (triangle) pin shape; false for data (circle).
         * @param connectedAttrIDs Set of connected attribute IDs (for filled/outlined shapes).
         */
        void RenderTypedPin(int attrId, const char* label, bool isInput, bool isExec,
                            const std::unordered_set<int>& connectedAttrIDs = {});

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

        // Synchronize node positions from ImNodes to the graph data
        void SyncNodePositionsFromImNodes(int graphID);

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

        // Control rendering behavior when embedded in other renderers
        bool m_SuppressGraphTabs = false;  ///< When true, RenderGraphTabs() is skipped (used by BehaviorTreeRenderer)

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

        // -----------------------------------------------------------------------
        // Phase 8: Subgraph tab state
        // -----------------------------------------------------------------------

        /// Ordered list of open subgraph tabs.  Index 0 is always the root graph.
        std::vector<GraphTab> m_SubgraphTabs;

                /// Index into m_SubgraphTabs of the currently visible tab.
                int m_ActiveSubgraphTabIndex = 0;

                /// Buffer used by the "New SubGraph" name input popup.
                char m_NewSubgraphNameBuffer[128];

                        // -----------------------------------------------------------------------
                        // Phase 35.0: imnodes context management
                        // -----------------------------------------------------------------------

                        /// Dedicated imnodes rendering context for this panel instance.
                        /// Prevents viewport state collision with other graph renderers.
                        ImNodesEditorContext* m_imnodesContext = nullptr;

                        // -----------------------------------------------------------------------
                        // Phase 36: Canvas editor minimap framework
                        // -----------------------------------------------------------------------

                        /// Canvas editor adapter for minimap support (Phase 36)
                        /// Abstracts imnodes minimap rendering through ICanvasEditor interface
                        std::unique_ptr<class ImNodesCanvasEditor> m_canvasEditor;
                    };

                } // namespace Olympe
