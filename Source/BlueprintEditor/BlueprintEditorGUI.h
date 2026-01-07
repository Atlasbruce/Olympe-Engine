/*
 * Olympe Blueprint Editor GUI - Frontend (User Interface)
 * 
 * Visual node-based editor interface using ImGui
 * All data is retrieved from and modified through BlueprintEditor backend
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "AssetBrowser.h"
#include "AssetInfoPanel.h"
#include "NodeGraphPanel.h"
#include "EntitiesPanel.h"
#include "InspectorPanel.h"

// Forward declare ImVec2
struct ImVec2;

namespace Olympe
{
    // Forward declaration of backend
    class BlueprintEditor;

    struct NodeConnection
    {
        int from_node;
        int to_node;
        int from_attr;
        int to_attr;
    };

    /**
     * BlueprintEditorGUI - Frontend UI for Blueprint Editor
     * Renders ImGui interface and interacts with BlueprintEditor backend
     * Contains NO business logic - all data operations go through backend
     */
    class BlueprintEditorGUI
    {
    public:
        BlueprintEditorGUI();
        ~BlueprintEditorGUI();

        void Initialize();
        void Shutdown();
        
        // Render the editor UI
        // Only renders if BlueprintEditor backend is active
        void Render();

    private:
        void RenderMenuBar();
        void RenderEntityPanel();
        void RenderNodeEditor();
        void RenderPropertyPanel();
        void RenderStatusBar();
        void RenderComponentAddDialog();
        
        // D) Additional dialog methods
        void RenderPreferencesDialog();
        void RenderShortcutsDialog();
        
        // Phase 6: Keyboard shortcuts
        void HandleKeyboardShortcuts();

        // File operations (delegate to backend)
        void NewBlueprint();
        void LoadBlueprint(const std::string& filepath);
        void SaveBlueprint();
        void SaveBlueprintAs();

        // Component operations (delegate to backend)
        void AddComponent(const std::string& type);
        void RemoveComponent(int index);

        // UI state (not business logic)
        int m_SelectedComponentIndex;
        
        // Node editor state (UI-only)
        std::map<int, ImVec2> m_NodePositions;
        std::vector<NodeConnection> m_Connections;
        int m_NextNodeId;

        // UI dialog state
        bool m_ShowDemoWindow;
        bool m_ShowAddComponentDialog;
        bool m_ShowAboutDialog;
        char m_NewBlueprintNameBuffer[256];
        char m_FilepathBuffer[512];
        int m_SelectedComponentType;
        
        // D) Panel visibility flags for View menu
        bool m_ShowAssetBrowser;
        bool m_ShowAssetInfo;
        bool m_ShowInspector;
        bool m_ShowNodeGraph;
        bool m_ShowEntities;
        bool m_ShowEntityProperties;
        bool m_ShowComponentGraph;
        bool m_ShowPropertyPanel;
        bool m_ShowTemplateBrowser;  // Phase 5
        bool m_ShowHistory;          // Phase 6
        
        // D) Additional dialog flags
        bool m_ShowPreferences;
        bool m_ShowShortcuts;
        
        // Asset management UI (uses backend for data)
        AssetBrowser m_AssetBrowser;
        AssetInfoPanel m_AssetInfoPanel;
        
        // New panels for Phase 2, 3, 4
        NodeGraphPanel m_NodeGraphPanel;
        EntitiesPanel m_EntitiesPanel;
        InspectorPanel m_InspectorPanel;
        
        // Phase 5: Template management UI
        class TemplateBrowserPanel* m_TemplateBrowserPanel;
        
        // Phase 6: History panel UI
        class HistoryPanel* m_HistoryPanel;
    };
}
