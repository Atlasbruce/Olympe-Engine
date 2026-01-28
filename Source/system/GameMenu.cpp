#include "GameMenu.h"
#include "../VideoGame.h"
#include "../World.h"
#include <iostream>
#include <fstream>
#include "system_utils.h"

#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
#include "../third_party/imgui/imgui.h"
#endif

// C++14 compatible directory traversal (no std::filesystem)
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif


void GameMenu::Activate()
{
    if (m_active) return;
    m_active = true;

    // Pause the game when the menu is activated
    VideoGame::Get().Pause();

    SYSTEM_LOG << "GameMenu: activated\n";
}

void GameMenu::Deactivate()
{
    if (!m_active) return;
    m_active = false;

    // Resume the game when the menu is closed
    VideoGame::Get().Resume();

    SYSTEM_LOG << "GameMenu: deactivated\n";
}

void GameMenu::SelectPrevious()
{
    m_selected = (m_selected - 1 + MenuOption::Count) % MenuOption::Count;
}

void GameMenu::SelectNext()
{
    m_selected = (m_selected + 1) % MenuOption::Count;
}

void GameMenu::ValidateSelection()
{
    switch (m_selected)
    {
        case MenuOption::Resume:
            Deactivate();  // Resume game
            break;
        
        case MenuOption::Restart:
            // Restart current level
            // For now, just log - full implementation depends on level loading system
            SYSTEM_LOG << "GameMenu: Restart selected (not yet implemented)\n";
            Deactivate();
            break;
        
        case MenuOption::Quit:
            // Quit to main menu or exit
            VideoGame::Get().RequestQuit();
            break;
    }
}

void GameMenu::Render()
{
    if (!m_active) return;
    SYSTEM_LOG << "--- GameMenu ---\n";
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        if (static_cast<int>(i) == m_selected) SYSTEM_LOG << "> "; else SYSTEM_LOG << "  ";
        SYSTEM_LOG << m_entries[i] << "\n";
    }
}

void GameMenu::Update()
{
    // Check for F2 key press to toggle F2 menu
    // This will be called from the game loop
    // For now, this is a placeholder - actual key handling is done elsewhere
}

void GameMenu::ScanForTiledMaps(const std::string& directory)
{ 
    #ifdef _WIN32
    // Windows implementation
    std::string searchPath = directory + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string fileName = findData.cFileName;
            if (fileName == "." || fileName == "..") continue;
            
            std::string fullPath = directory + "\\" + fileName;
            
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Recursively scan subdirectories
                ScanForTiledMaps(fullPath);
            }
            else
            {
                // Check if file has .tmj extension
                if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".tmj")
                {
                    m_tiledMapPaths.push_back(fullPath);
                }
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
    #else
    // Unix/Linux implementation
    DIR* dir = opendir(directory.c_str());
    if (dir != nullptr)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string fileName = entry->d_name;
            if (fileName == "." || fileName == "..") continue;
            
            std::string fullPath = directory + "/" + fileName;
            
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0)
            {
                if (S_ISDIR(statbuf.st_mode))
                {
                    // Recursively scan subdirectories
                    ScanForTiledMaps(fullPath);
                }
                else if (S_ISREG(statbuf.st_mode))
                {
                    // Check if file has .tmj extension
                    if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == ".tmj")
                    {
                        m_tiledMapPaths.push_back(fullPath);
                    }
                }
            }
        }
        closedir(dir);
    }
    #endif
}

void GameMenu::RenderF2Menu()
{
#ifdef OLYMPE_BLUEPRINT_EDITOR_ENABLED
    if (!m_f2MenuOpen) return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Load Tiled Level (F2)", &m_f2MenuOpen))
    {
        ImGui::Text("Select a Tiled map (.tmj) to load:");
        ImGui::Separator();
        
        // Scan button
        if (ImGui::Button("Refresh List"))
        {
            m_tiledMapPaths.clear();

            //ScanForTiledMaps("Blueprints");
            //ScanForTiledMaps("Levels");
            //ScanForTiledMaps("gamedata");
            //ScanForTiledMaps("gamedata\\levels");
            ScanForTiledMaps(".");
            SYSTEM_LOG << "GameMenu: Found " << m_tiledMapPaths.size() << " Tiled maps\n";
        }
        
        ImGui::SameLine();
        ImGui::Text("Found %d maps", static_cast<int>(m_tiledMapPaths.size()));
        
        ImGui::Separator();
        
        // Map list
        ImGui::BeginChild("MapList", ImVec2(0, -30), true);
        
        for (size_t i = 0; i < m_tiledMapPaths.size(); ++i)
        {
            const std::string& mapPath = m_tiledMapPaths[i];
            
            // Extract just the filename for display
            std::string displayName = mapPath;
            size_t lastSlash = displayName.find_last_of("/\\");
            if (lastSlash != std::string::npos)
            {
                displayName = displayName.substr(lastSlash + 1);
            }
            
            bool isSelected = (static_cast<int>(i) == m_selectedMapIndex);
            if (ImGui::Selectable(displayName.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_selectedMapIndex = static_cast<int>(i);
                
                // Double-click to load
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    SYSTEM_LOG << "GameMenu: Loading map: " << mapPath << "\n";
                    World::Get().LoadLevelFromTiled(mapPath);
                    m_f2MenuOpen = false;
                }
            }
            
            // Show full path as tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", mapPath.c_str());
            }
        }
        
        ImGui::EndChild();
        
        // Load button
        ImGui::Separator();
        if (ImGui::Button("Load Selected", ImVec2(120, 0)))
        {
            if (m_selectedMapIndex >= 0 && m_selectedMapIndex < static_cast<int>(m_tiledMapPaths.size()))
            {
                const std::string& mapPath = m_tiledMapPaths[m_selectedMapIndex];
                SYSTEM_LOG << "GameMenu: Loading map: " << mapPath << "\n";
                World::Get().LoadLevelFromTiled(mapPath);
                m_f2MenuOpen = false;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            m_f2MenuOpen = false;
        }
    }
    ImGui::End();
#endif
}
