#pragma once
#include <vector>
#include <string>
#include "system_utils.h"

// GameMenu is an instance-based menu attached to a VideoGame object.
// It receives events (via OnEvent(Message& msg) forwarding) and can be used
// to navigate a set of menu entries. It is also a GameEntity-like
// entity so it can be added to the engine's object list if desired.

class GameMenu 
{
public:
    enum MenuOption
    {
        Resume = 0,
        Restart = 1,
        Quit = 2,
        Count = 3
    };
    
    GameMenu()
    {
        name = "GameMenu";
		SYSTEM_LOG << "GameMenu Initialized\n";
	}
    virtual ~GameMenu()
    {
        Deactivate();
		SYSTEM_LOG << "GameMenu Destroyed\n";
    }

    static GameMenu& GetInstance()
    {
        static GameMenu instance;
        return instance;
	}
	static GameMenu& Get() { return GetInstance(); }

    void Activate();
    void Deactivate();
    bool IsActive() const { return m_active; }

    void AddEntry(const std::string& e) { m_entries.push_back(e); }
    
    // Menu navigation
    void SelectPrevious();
    void SelectNext();
    void ValidateSelection();
    
    int GetSelectedOption() const { return m_selected; }

    // Render simplified text-based menu (placeholder)
    virtual void Render();
    
    // Render ImGui F2 menu for loading Tiled levels
    void RenderF2Menu();
    
    // Check if F2 menu should be shown
    void Update();
    
    bool IsF2MenuOpen() const { return m_f2MenuOpen; }
    void ToggleF2Menu();
    void SetF2MenuOpen(bool open);

private:
    std::string name;
    bool m_active = false;
    std::vector<std::string> m_entries;
    int m_selected = MenuOption::Resume;  // Default to Resume
    
    // F2 menu state
    bool m_f2MenuOpen = false;
    std::vector<std::string> m_tiledMapPaths;
    int m_selectedMapIndex = -1;
    bool m_hasScannedTiledMaps = false;
    
    // Helper to scan for .tmj files
    void ScanForTiledMaps(const std::string& directory);
    void RefreshTiledMapList();
};
