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
     * Blueprint Editor Mode
     * Defines the operational mode of the blueprint editor
     */
    enum class BlueprintEditorMode
    {
        Standalone,     // Full CRUD editing, no entity binding
        Runtime         // Read-only or light operations, for runtime entity inspection
    };

    /**
     * NodeGraphPanel - ImGui/ImNodes panel for node graph editing
     * Provides visual editor for behavior trees and state machines
     * Supports both Standalone (full CRUD) and Runtime (visualization) modes
     */
    class NodeGraphPanel
    {
    public:
        NodeGraphPanel();
        ~NodeGraphPanel();

        void Initialize();
        void Shutdown();
        void Render();
        
        // Set editor mode
        void SetEditorMode(BlueprintEditorMode mode) { m_EditorMode = mode; }
        BlueprintEditorMode GetEditorMode() const { return m_EditorMode; }

    private:
        void RenderGraphTabs();
        void RenderGraph();
        void RenderContextMenu();
        void RenderNodeProperties();
        void RenderNodeEditModal();
        void HandleKeyboardShortcuts();

        // Node creation helpers
        void CreateNewNode(const char* nodeType, float x, float y);
        
        // Check if editor is in read-only mode
        bool IsReadOnly() const { return m_EditorMode == BlueprintEditorMode::Runtime; }

        // ImNodes state
        int m_SelectedNodeId = -1;
        int m_SelectedLinkId = -1;
        bool m_ShowContextMenu = false;
        float m_ContextMenuPosX = 0.0f;
        float m_ContextMenuPosY = 0.0f;
        
        // Editor mode
        BlueprintEditorMode m_EditorMode = BlueprintEditorMode::Standalone;
        
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
