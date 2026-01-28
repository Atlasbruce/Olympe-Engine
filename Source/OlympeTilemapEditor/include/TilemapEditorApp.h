/*
 * Olympe Tilemap Editor - Main Application
 * 
 * ImGui-based tilemap/level editor application.
 */

#pragma once

#include "LevelManager.h"
#include "EditorState.h"
#include <memory>
#include <string>

namespace Olympe {
namespace Editor {

    class TilemapEditorApp
    {
    public:
        TilemapEditorApp();
        ~TilemapEditorApp();

        // Initialization and shutdown
        bool Initialize();
        void Shutdown();

        // Main render loop
        void Render();

        // File operations
        void NewLevel();
        void OpenLevel();
        void SaveLevel();
        void SaveLevelAs();

        // Editor state
        bool HasUnsavedChanges() const;
        const std::string& GetCurrentLevelName() const;

    private:
        // UI Rendering methods
        void RenderMenuBar();
        void RenderToolbar();
        void RenderLevelViewport();
        void RenderEntityList();
        void RenderPropertiesPanel();
        void RenderHistoryPanel();
        void RenderStatusBar();

        // Helper methods
        void ShowNewLevelDialog();
        void ShowOpenLevelDialog();
        void ShowSaveLevelDialog();
        void ShowAboutDialog();

        // Editor state
        std::unique_ptr<LevelManager> m_levelManager;
        std::unique_ptr<EditorState> m_editorState;

        // UI state
        bool m_showNewLevelDialog;
        bool m_showOpenLevelDialog;
        bool m_showSaveLevelDialog;
        bool m_showAboutDialog;
        
        char m_newLevelNameBuffer[256];
        char m_filePathBuffer[512];

        // Editor settings
        float m_viewportZoom;
        Vector m_viewportOffset;
        
        // Selection state
        std::string m_selectedEntityId;
        int m_selectedTileId;
        
        bool m_initialized;
    };

} // namespace Editor
} // namespace Olympe
