/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- PanelManager is a singleton responsible for creating and managing
  lightweight tool windows (log window, object inspector, tree view)
  using the Win32 API when available. The actual rendering/content of
  these windows is implemented later; this class provides creation,
  visibility and lifetime management.

Notes:
- On non-Windows platforms the implementation is a no-op (stubs), so
  the engine remains portable.
*/

#pragma once

#include "object.h"
//#include "system/system_utils.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <SDL3/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <SDL3/SDL.h>

class PanelManager 
{
public:
    PanelManager();
    virtual ~PanelManager();

    static PanelManager& GetInstance();
    static PanelManager& Get() { return GetInstance(); }

    // Initialize/shutdown the manager (register window class on Windows)
    void Initialize();
    void Shutdown();

    // Create predefined panels (id is used to reference)
    void CreateLogWindow();
    void CreateObjectInspectorWindow();
    void CreateTreeViewWindow();
    void CreateInputsInspectorWindow();
    void CreateMainMenuWindow();

    // Show/hide panels
    void ShowPanel(const std::string& id);
    void HidePanel(const std::string& id);
    bool IsPanelVisible(const std::string& id) const;

    // Append text to the log window (thread-safe)
    void AppendLog(const std::string& text);

    // Attach a menu to the SDL main window (Windows only)
    void AttachToSDLWindow(SDL_Window* sdlWindow);

    // Process internal tasks (on Windows: pump messages for tool windows)
    void HandleEvent(const SDL_Event* ev);

    //OnEvent
	virtual void OnEvent(const Message& msg) ;

	static int LogPanelWidth;
	static int LogPanelHeight;
	static int LogPanelPosX;
    static int LogPanelPosY;

	static int InspectorPanelWidth;
	static int InspectorPanelHeight;
	static int InspectorPanelPosX;
    static int InspectorPanelPosY;

	static int TreeViewPanelWidth;
	static int TreeViewPanelHeight;
	static int TreeViewPanelPosX;
    static int TreeViewPanelPosY;

    private:
        string name;

    struct Panel
    {
        std::string id;
        std::string title;
        bool visible = false;
#ifdef _WIN32
        HWND hwnd = nullptr;
        HWND hwndChild = nullptr; // child control (e.g. edit for log)
        HMENU hMenu = nullptr; // optional per-panel menu
#else
        void* hwnd = nullptr; // stub
#endif
    };

    void CreatePanel(const std::string& id, const std::string& title);
    void UpdateInputsInspectorList();

    std::unordered_map<std::string, Panel> m_panels_;
    std::vector<std::string> m_pendingLogs_; // logs received before the UI control exists
    mutable std::mutex m_mutex_;

#ifdef _WIN32
    ATOM m_wndClassAtom = 0;
    HWND m_mainHwnd = nullptr;
    HMENU m_mainMenu = nullptr;

    static LRESULT CALLBACK PanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // menu command IDs
    enum MenuIDs
    {
        IDM_PANEL_LOG = 40001,
        IDM_PANEL_INSPECTOR = 40002,
        IDM_PANEL_TREE = 40003,
        IDM_PANEL_INPUTS = 40004,
        IDM_FILE_NEW = 40100,
        IDM_FILE_LOAD = 40101,
        IDM_FILE_SAVE = 40102,
        IDM_WINDOW_OBJECT_INSPECTOR = 40110,
        IDM_WINDOW_OBJECT_HIERARCHY = 40111,
        IDM_WINDOW_LOG = 40112,
        IDM_WINDOW_INPUTS = 40113,
        IDM_ABOUT = 40200
    };
#endif
};
