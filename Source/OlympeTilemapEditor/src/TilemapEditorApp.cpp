/*
 * Olympe Tilemap Editor - Main Application Implementation
 */

#include "../include/TilemapEditorApp.h"
#include "../../third_party/imgui/imgui.h"
#include <iostream>
#include <cstring>

namespace Olympe {
namespace Editor {

    TilemapEditorApp::TilemapEditorApp()
        : m_showNewLevelDialog(false)
        , m_showOpenLevelDialog(false)
        , m_showSaveLevelDialog(false)
        , m_showAboutDialog(false)
        , m_viewportZoom(1.0f)
        , m_viewportOffset(0, 0)
        , m_selectedTileId(0)
        , m_initialized(false)
    {
        std::memset(m_newLevelNameBuffer, 0, sizeof(m_newLevelNameBuffer));
        std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
    }

    TilemapEditorApp::~TilemapEditorApp()
    {
        Shutdown();
    }

    bool TilemapEditorApp::Initialize()
    {
        if (m_initialized)
        {
            std::cerr << "[TilemapEditorApp] Already initialized" << std::endl;
            return true;
        }

        std::cout << "[TilemapEditorApp] Initializing..." << std::endl;

        // Create core systems
        m_levelManager = std::make_unique<LevelManager>();
        m_editorState = std::make_unique<EditorState>();

        // Create a default new level
        m_levelManager->NewLevel("UntitledLevel");

        m_initialized = true;
        std::cout << "[TilemapEditorApp] Initialization complete" << std::endl;

        return true;
    }

    void TilemapEditorApp::Shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        std::cout << "[TilemapEditorApp] Shutting down..." << std::endl;

        m_editorState.reset();
        m_levelManager.reset();

        m_initialized = false;
    }

    void TilemapEditorApp::Render()
    {
        if (!m_initialized)
        {
            return;
        }

        // Main menu bar
        RenderMenuBar();

        // Render UI panels directly without dockspace
        RenderToolbar();
        RenderLevelViewport();
        RenderEntityList();
        RenderPropertiesPanel();
        RenderHistoryPanel();
        RenderStatusBar();

        // Render dialogs
        if (m_showNewLevelDialog)
            ShowNewLevelDialog();
        if (m_showOpenLevelDialog)
            ShowOpenLevelDialog();
        if (m_showSaveLevelDialog)
            ShowSaveLevelDialog();
        if (m_showAboutDialog)
            ShowAboutDialog();
    }

    // ========================================================================
    // File Operations
    // ========================================================================

    void TilemapEditorApp::NewLevel()
    {
        m_showNewLevelDialog = true;
    }

    void TilemapEditorApp::OpenLevel()
    {
        m_showOpenLevelDialog = true;
    }

    void TilemapEditorApp::SaveLevel()
    {
        const std::string& currentPath = m_levelManager->GetCurrentLevelPath();
        if (currentPath.empty())
        {
            SaveLevelAs();
        }
        else
        {
            m_levelManager->SaveLevel(currentPath);
        }
    }

    void TilemapEditorApp::SaveLevelAs()
    {
        m_showSaveLevelDialog = true;
    }

    bool TilemapEditorApp::HasUnsavedChanges() const
    {
        return m_levelManager && m_levelManager->HasUnsavedChanges();
    }

    const std::string& TilemapEditorApp::GetCurrentLevelName() const
    {
        static std::string defaultName = "Untitled";
        if (m_levelManager)
        {
            return m_levelManager->GetLevelDefinition().levelName;
        }
        return defaultName;
    }

    // ========================================================================
    // UI Rendering
    // ========================================================================

    void TilemapEditorApp::RenderMenuBar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Level", "Ctrl+N"))
                {
                    NewLevel();
                }
                if (ImGui::MenuItem("Open Level...", "Ctrl+O"))
                {
                    OpenLevel();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    SaveLevel();
                }
                if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                {
                    SaveLevelAs();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    // Exit will be handled by main loop
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                bool canUndo = m_editorState->CanUndo();
                bool canRedo = m_editorState->CanRedo();

                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo))
                {
                    m_editorState->Undo(*m_levelManager);
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo))
                {
                    m_editorState->Redo(*m_levelManager);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Reset Zoom"))
                {
                    m_viewportZoom = 1.0f;
                }
                if (ImGui::MenuItem("Reset Pan"))
                {
                    m_viewportOffset = Vec2(0, 0);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About"))
                {
                    m_showAboutDialog = true;
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void TilemapEditorApp::RenderToolbar()
    {
        ImGui::Begin("Toolbar");

        ImGui::Text("Tools");
        ImGui::Separator();

        if (ImGui::Button("Select", ImVec2(80, 30)))
        {
            // Activate select tool
        }
        ImGui::SameLine();

        if (ImGui::Button("Paint Tile", ImVec2(80, 30)))
        {
            // Activate tile painting tool
        }
        ImGui::SameLine();

        if (ImGui::Button("Entity", ImVec2(80, 30)))
        {
            // Activate entity placement tool
        }

        ImGui::Separator();
        ImGui::Text("Selected Tile ID: %d", m_selectedTileId);
        ImGui::SliderInt("Tile ID", &m_selectedTileId, 0, 255);

        ImGui::End();
    }

    void TilemapEditorApp::RenderLevelViewport()
    {
        ImGui::Begin("Level Viewport");

        ImGui::Text("Level: %s", GetCurrentLevelName().c_str());
        ImGui::SameLine();
        if (HasUnsavedChanges())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "*");
        }

        ImGui::Separator();

        // Viewport controls
        ImGui::Text("Zoom: %.2f", m_viewportZoom);
        ImGui::SameLine();
        if (ImGui::Button("-"))
        {
            m_viewportZoom = std::max(0.1f, m_viewportZoom - 0.1f);
        }
        ImGui::SameLine();
        if (ImGui::Button("+"))
        {
            m_viewportZoom = std::min(5.0f, m_viewportZoom + 0.1f);
        }

        ImGui::Separator();

        // Placeholder for actual viewport rendering
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("ViewportCanvas", viewport_size, true);

        ImGui::Text("Canvas Area");
        ImGui::Text("Size: %.0f x %.0f", viewport_size.x, viewport_size.y);
        ImGui::Text("Entities: %zu", m_levelManager->GetAllEntities().size());

        // TODO: Render actual level content here
        // - Grid
        // - Tiles
        // - Entities
        // - Selection highlights

        ImGui::EndChild();

        ImGui::End();
    }

    void TilemapEditorApp::RenderEntityList()
    {
        ImGui::Begin("Entity List");

        ImGui::Text("Entities");
        ImGui::Separator();

        if (ImGui::Button("Add Entity"))
        {
            auto cmd = std::make_unique<PlaceEntityCommand>(
                "Blueprints/DefaultEntity.json",
                Vec2(0, 0)
            );
            m_editorState->ExecuteCommand(std::move(cmd), *m_levelManager);
        }

        ImGui::Separator();

        // List all entities
        auto entities = m_levelManager->GetAllEntities();
        for (auto* entity : entities)
        {
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (m_selectedEntityId == entity->id)
            {
                node_flags |= ImGuiTreeNodeFlags_Selected;
            }

            ImGui::TreeNodeEx(entity->id.c_str(), node_flags, "%s", entity->name.c_str());
            if (ImGui::IsItemClicked())
            {
                m_selectedEntityId = entity->id;
            }

            // Context menu
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    auto cmd = std::make_unique<DeleteEntityCommand>(entity->id);
                    m_editorState->ExecuteCommand(std::move(cmd), *m_levelManager);
                    m_selectedEntityId = "";
                }
                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void TilemapEditorApp::RenderPropertiesPanel()
    {
        ImGui::Begin("Properties");

        if (!m_selectedEntityId.empty())
        {
            EntityInstance* entity = m_levelManager->GetEntity(m_selectedEntityId);
            if (entity)
            {
                ImGui::Text("Entity Properties");
                ImGui::Separator();

                // Name
                char nameBuffer[256];
#ifdef _MSC_VER
                strncpy_s(nameBuffer, sizeof(nameBuffer), entity->name.c_str(), _TRUNCATE);
#else
                std::strncpy(nameBuffer, entity->name.c_str(), sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
#endif
                if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
                {
                    entity->name = nameBuffer;
                    m_levelManager->MarkDirty();
                }

                // Prefab path (read-only for now)
                ImGui::Text("Prefab: %s", entity->prefabPath.c_str());

                // Position
                float posX = static_cast<float>(entity->position.x);
                float posY = static_cast<float>(entity->position.y);
                if (ImGui::DragFloat("Position X", &posX, 1.0f))
                {
                    entity->position.x = posX;
                    m_levelManager->MarkDirty();
                }
                if (ImGui::DragFloat("Position Y", &posY, 1.0f))
                {
                    entity->position.y = posY;
                    m_levelManager->MarkDirty();
                }
            }
            else
            {
                ImGui::Text("Selected entity not found");
                m_selectedEntityId = "";
            }
        }
        else
        {
            ImGui::Text("No entity selected");
        }

        ImGui::End();
    }

    void TilemapEditorApp::RenderHistoryPanel()
    {
        ImGui::Begin("History");

        ImGui::Text("Command History");
        ImGui::Separator();

        if (ImGui::Button("Undo") && m_editorState->CanUndo())
        {
            m_editorState->Undo(*m_levelManager);
        }
        ImGui::SameLine();
        if (ImGui::Button("Redo") && m_editorState->CanRedo())
        {
            m_editorState->Redo(*m_levelManager);
        }

        ImGui::Separator();

        ImGui::Text("History Size: %zu", m_editorState->GetHistorySize());
        ImGui::Text("Current Index: %zu", m_editorState->GetHistoryIndex());

        if (m_editorState->CanUndo())
        {
            ImGui::Text("Next Undo: %s", m_editorState->GetUndoDescription().c_str());
        }
        if (m_editorState->CanRedo())
        {
            ImGui::Text("Next Redo: %s", m_editorState->GetRedoDescription().c_str());
        }

        ImGui::End();
    }

    void TilemapEditorApp::RenderStatusBar()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 25));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 25));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
        ImGui::Begin("StatusBar", nullptr, window_flags);

        ImGui::Text("Olympe Tilemap Editor - Phase 1");
        ImGui::SameLine(viewport->WorkSize.x - 200);
        ImGui::Text("Entities: %zu", m_levelManager->GetAllEntities().size());

        ImGui::End();
    }

    // ========================================================================
    // Dialogs
    // ========================================================================

    void TilemapEditorApp::ShowNewLevelDialog()
    {
        ImGui::OpenPopup("New Level");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("New Level", &m_showNewLevelDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Create a new level");
            ImGui::Separator();

            ImGui::InputText("Level Name", m_newLevelNameBuffer, sizeof(m_newLevelNameBuffer));

            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::string levelName = m_newLevelNameBuffer;
                if (levelName.empty())
                {
                    levelName = "UntitledLevel";
                }
                m_levelManager->NewLevel(levelName);
                m_editorState->ClearHistory();
                m_showNewLevelDialog = false;
                std::memset(m_newLevelNameBuffer, 0, sizeof(m_newLevelNameBuffer));
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_showNewLevelDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    void TilemapEditorApp::ShowOpenLevelDialog()
    {
        ImGui::OpenPopup("Open Level");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Open Level", &m_showOpenLevelDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Open an existing level");
            ImGui::Separator();

            ImGui::InputText("File Path", m_filePathBuffer, sizeof(m_filePathBuffer));
            ImGui::Text("Note: Use full or relative path to JSON file");

            ImGui::Separator();

            if (ImGui::Button("Open", ImVec2(120, 0)))
            {
                std::string filePath = m_filePathBuffer;
                if (!filePath.empty())
                {
                    if (m_levelManager->LoadLevel(filePath))
                    {
                        m_editorState->ClearHistory();
                        m_showOpenLevelDialog = false;
                        std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
                    }
                    else
                    {
                        // Show error in console
                        std::cerr << "[TilemapEditorApp] Failed to open level: " << filePath << std::endl;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_showOpenLevelDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    void TilemapEditorApp::ShowSaveLevelDialog()
    {
        ImGui::OpenPopup("Save Level As");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Save Level As", &m_showSaveLevelDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Save level to file");
            ImGui::Separator();

            ImGui::InputText("File Path", m_filePathBuffer, sizeof(m_filePathBuffer));
            ImGui::Text("Note: Use full or relative path (e.g., Levels/my_level.json)");

            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                std::string filePath = m_filePathBuffer;
                if (!filePath.empty())
                {
                    if (m_levelManager->SaveLevel(filePath))
                    {
                        m_showSaveLevelDialog = false;
                        std::memset(m_filePathBuffer, 0, sizeof(m_filePathBuffer));
                    }
                    else
                    {
                        std::cerr << "[TilemapEditorApp] Failed to save level: " << filePath << std::endl;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_showSaveLevelDialog = false;
            }

            ImGui::EndPopup();
        }
    }

    void TilemapEditorApp::ShowAboutDialog()
    {
        ImGui::OpenPopup("About");

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("About", &m_showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Olympe Tilemap Editor");
            ImGui::Separator();
            ImGui::Text("Version: 1.0.0 (Phase 1)");
            ImGui::Text("Author: Atlasbruce");
            ImGui::Separator();
            ImGui::Text("A tilemap/level editor for Olympe Engine");
            ImGui::Text("Built with SDL3 + ImGui");
            ImGui::Separator();

            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                m_showAboutDialog = false;
            }

            ImGui::EndPopup();
        }
    }

} // namespace Editor
} // namespace Olympe
