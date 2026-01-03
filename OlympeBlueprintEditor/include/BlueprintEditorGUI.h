/*
 * Olympe Blueprint Editor GUI - Phase 2
 * Visual node-based editor interface
 */

#pragma once

#include "EntityBlueprint.h"
#include "AssetBrowser.h"
#include "AssetInfoPanel.h"
#include <string>
#include <vector>
#include <map>

// Forward declare ImVec2
struct ImVec2;

namespace Olympe
{
    struct NodeConnection
    {
        int from_node;
        int to_node;
        int from_attr;
        int to_attr;
    };

    class BlueprintEditorGUI
    {
    public:
        BlueprintEditorGUI();
        ~BlueprintEditorGUI();

        void Initialize();
        void Shutdown();
        bool Render();  // Returns false if should exit

    private:
        void RenderMenuBar();
        void RenderEntityPanel();
        void RenderNodeEditor();
        void RenderPropertyPanel();
        void RenderStatusBar();
        void RenderComponentAddDialog();
        void RenderAssetBrowser();
        void RenderAssetInfoPanel();

        // File operations
        void NewBlueprint();
        void LoadBlueprint(const std::string& filepath);
        void SaveBlueprint();
        void SaveBlueprintAs();

        // Component operations
        void AddComponent(const std::string& type);
        void RemoveComponent(int index);

        // State
        Blueprint::EntityBlueprint m_CurrentBlueprint;
        std::string m_CurrentFilepath;
        bool m_HasUnsavedChanges;
        int m_SelectedComponentIndex;
        
        // Node editor state
        std::map<int, ImVec2> m_NodePositions;
        std::vector<NodeConnection> m_Connections;
        int m_NextNodeId;

        // UI state
        bool m_ShowDemoWindow;
        bool m_ShowAddComponentDialog;
        bool m_ShowAboutDialog;
        bool m_RequestExit;
        char m_NewBlueprintNameBuffer[256];
        char m_FilepathBuffer[512];
        int m_SelectedComponentType;

        // Asset browser and info panel
        AssetBrowser m_AssetBrowser;
        AssetInfoPanel m_AssetInfoPanel;
    };
}
