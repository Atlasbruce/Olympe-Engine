/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- PanelManager implementation. Creates simple Win32 child/floating windows
  to host debug panels (log window, object inspector, tree view). The
  actual rendering/content of windows will be implemented later.

Notes:
- This file uses minimal Win32 APIs guarded by #ifdef _WIN32 so it remains
  portable on other platforms (stubs will be used).
*/

#include "PanelManager.h"
#include "GameEngine.h"
#include "system/EventManager.h"
#include <mutex>
#include "inputsmanager.h"

#ifdef _WIN32
#include <strsafe.h>
#if defined(__has_include)
  #if __has_include(<SDL3/SDL_syswm.h>)
    #include <SDL3/SDL_syswm.h>
    #define HAVE_SDL_SYSWM 1
  #endif
#endif
#endif

//----------------------------------------------------------------------
// initialize static vairables for panel sizes and positions
int PanelManager::LogPanelWidth = 400;
int PanelManager::LogPanelHeight = 200;
int PanelManager::LogPanelPosX = 50;
int PanelManager::LogPanelPosY = 50;
int PanelManager::InspectorPanelWidth = 300;
int PanelManager::InspectorPanelHeight = 400;
int PanelManager::InspectorPanelPosX = 100;
int PanelManager::InspectorPanelPosY = 100;
int PanelManager::TreeViewPanelWidth = 300;
int PanelManager::TreeViewPanelHeight = 400;
int PanelManager::TreeViewPanelPosX = 150;
int PanelManager::TreeViewPanelPosY = 150;

//----------------------------------------------------------------------
PanelManager::PanelManager()
{
    name = "PanelManager";
}

PanelManager::~PanelManager()
{
    SYSTEM_LOG << "PanelManager destroyed\n";
    Shutdown();
}

PanelManager& PanelManager::GetInstance()
{
    static PanelManager instance;
    return instance;
}

void PanelManager::Initialize()
{
    EventManager::Get().Register(this, EventType::Olympe_EventType_Joystick_Connected);
	EventManager::Get().Register(this, EventType::Olympe_EventType_Joystick_Disconnected);
	EventManager::Get().Register(this, EventType::Olympe_EventType_Keyboard_Connected);
	EventManager::Get().Register(this, EventType::Olympe_EventType_Keyboard_Disconnected);
	EventManager::Get().Register(this, EventType::Olympe_EventType_Mouse_Connected);
	EventManager::Get().Register(this, EventType::Olympe_EventType_Mouse_Disconnected);

	EventManager::Get().Register(this, (EventType) IDM_PANEL_LOG);

#ifdef _WIN32
    // Register a small window class for panels
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = PanelWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = TEXT("OlympePanelClass");
    m_wndClassAtom = RegisterClassEx(&wc);
    if (!m_wndClassAtom) SYSTEM_LOG << "PanelManager: RegisterClassEx failed\n";
#endif
}

void PanelManager::Shutdown()
{
#ifdef _WIN32
    for (auto &kv : m_panels_)
    {
        if (kv.second.hwnd) DestroyWindow(kv.second.hwnd);
        kv.second.hwnd = nullptr;
        kv.second.hwndChild = nullptr;
        if (kv.second.hMenu) {
            DestroyMenu(kv.second.hMenu);
            kv.second.hMenu = nullptr;
        }
    }
    if (m_mainMenu && m_mainHwnd) {
        SetMenu(m_mainHwnd, nullptr);
        DestroyMenu(m_mainMenu);
        m_mainMenu = nullptr;
    }
    if (m_wndClassAtom) UnregisterClass(TEXT("OlympePanelClass"), GetModuleHandle(nullptr));
    m_wndClassAtom = 0;
#else
    m_panels_.clear();
#endif
}

void PanelManager::CreatePanel(const std::string& id, const std::string& title)
{
    auto it = m_panels_.find(id);
    if (it != m_panels_.end()) return; // already created

    Panel p;
    p.id = id;
    p.title = title;
    p.visible = true;

#ifdef _WIN32
    HWND parent = nullptr;
    HWND hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE, TEXT("OlympePanelClass"), TEXT(""),
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);
    if (hwnd)
    {
        SetWindowTextA(hwnd, title.c_str());
        p.hwnd = hwnd;

        if (id == "log_window")
        {
            HWND hEdit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0, 0, 400, 300, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            if (hEdit)
            {
                HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
                p.hwndChild = hEdit;
            }
        }
        else if (id == "inputs_inspector")
        {
            // create a multiline edit control to display joystick list
            HWND hEdit = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0, 0, 400, 300, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
            if (hEdit)
            {
                HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE,0));
                p.hwndChild = hEdit;
                // populate initial list
                std::string text = "(initializing)\r\n";
                SetWindowTextA(hEdit, text.c_str());
            }
        }

        ShowWindow(hwnd, SW_SHOW);
        if (p.hwndChild) ShowWindow(p.hwndChild, SW_SHOW);
    }
#endif

    m_panels_.emplace(id, std::move(p));
}

void PanelManager::CreateLogWindow()
{
    CreatePanel("log_window", "Log Window");
#ifdef _WIN32
    auto it = m_panels_.find("log_window");
    if (it != m_panels_.end() && it->second.hwnd)
    {
        HWND hwnd = it->second.hwnd;
        SetWindowPos(hwnd, nullptr, 0, 0, LogPanelWidth, LogPanelHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
        if (it->second.hwndChild)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            MoveWindow(it->second.hwndChild, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            SetWindowPos(hwnd, nullptr, LogPanelPosX, LogPanelPosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
        }
    }
#endif
}

void PanelManager::CreateObjectInspectorWindow()
{
    CreatePanel("object_inspector", "Object Inspector");
}

void PanelManager::CreateTreeViewWindow()
{
    CreatePanel("tree_view", "Tree View");
}

void PanelManager::CreateMainMenuWindow()
{
    CreatePanel("main_menu", "Olympe Main Menu");
#ifdef _WIN32
    auto it = m_panels_.find("main_menu");
    if (it == m_panels_.end() || !it->second.hwnd) return;

    HWND hwnd = it->second.hwnd;

    HMENU hFile = CreatePopupMenu();
    AppendMenuA(hFile, MF_STRING, IDM_FILE_NEW, "New\tCtrl+N");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_LOAD, "Load\tCtrl+L");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_SAVE, "Save\tCtrl+S");

    HMENU hOptions = CreatePopupMenu();
    UINT fl = IsPanelVisible("log_window") ? MF_CHECKED : MF_UNCHECKED;
    UINT fi = IsPanelVisible("object_inspector") ? MF_CHECKED : MF_UNCHECKED;
    UINT ft = IsPanelVisible("tree_view") ? MF_CHECKED : MF_UNCHECKED;
    UINT fi2 = IsPanelVisible("inputs_inspector") ? MF_CHECKED : MF_UNCHECKED;
    AppendMenuA(hOptions, MF_STRING | fl, IDM_WINDOW_LOG, "Display Log Window");
    AppendMenuA(hOptions, MF_STRING | fi, IDM_WINDOW_OBJECT_INSPECTOR, "Display Object Inspector");
    AppendMenuA(hOptions, MF_STRING | ft, IDM_WINDOW_OBJECT_HIERARCHY, "Display Objects Hierarchy");
    AppendMenuA(hOptions, MF_STRING | fi2, IDM_WINDOW_INPUTS, "Display Inputs Inspector");

    HMENU hAbout = CreatePopupMenu();
    AppendMenuA(hAbout, MF_STRING, IDM_ABOUT, "About Olympe Engine");

    HMENU hMenu = CreateMenu();
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFile, "File");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hOptions, "Options");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hAbout, "About");

    SetMenu(hwnd, hMenu);
    it->second.hMenu = hMenu;
    m_mainMenu = hMenu;
    m_mainHwnd = hwnd;
#endif
}

void PanelManager::ShowPanel(const std::string& id)
{
    auto it = m_panels_.find(id);
    if (it == m_panels_.end()) return;
    it->second.visible = true;
#ifdef _WIN32
    if (it->second.hwnd) ShowWindow(it->second.hwnd, SW_SHOW);
    if (it->second.hwndChild) ShowWindow(it->second.hwndChild, SW_SHOW);
#endif
}

void PanelManager::HidePanel(const std::string& id)
{
    auto it = m_panels_.find(id);
    if (it == m_panels_.end()) return;
    it->second.visible = false;
#ifdef _WIN32
    if (it->second.hwnd) ShowWindow(it->second.hwnd, SW_HIDE);
    if (it->second.hwndChild) ShowWindow(it->second.hwndChild, SW_HIDE);
#endif
}

bool PanelManager::IsPanelVisible(const std::string& id) const
{
    auto it = m_panels_.find(id);
    if (it == m_panels_.end()) return false;
    return it->second.visible;
}

void PanelManager::AppendLog(const std::string& text)
{
    auto it = m_panels_.find("log_window");
    if (it == m_panels_.end()) return;
#ifdef _WIN32
    HWND hEdit = it->second.hwndChild;
    if (!hEdit) return;
    int len = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    std::string s = text + "\r\n";
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
    SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
#endif
}

void PanelManager::HandleEvent(const SDL_Event* ev)
{
#ifdef _WIN32
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_COMMAND)
        {
            int id = LOWORD(msg.wParam);
            switch (id)
            {
            case IDM_PANEL_LOG:
            case IDM_WINDOW_LOG:
                if (IsPanelVisible("log_window")) HidePanel("log_window"); else ShowPanel("log_window");
                if (m_mainMenu) CheckMenuItem(m_mainMenu, IDM_WINDOW_LOG, MF_BYCOMMAND | (IsPanelVisible("log_window") ? MF_CHECKED : MF_UNCHECKED));
                continue;
            case IDM_PANEL_INSPECTOR:
            case IDM_WINDOW_OBJECT_INSPECTOR:
                if (IsPanelVisible("object_inspector")) HidePanel("object_inspector"); else ShowPanel("object_inspector");
                if (m_mainMenu) CheckMenuItem(m_mainMenu, IDM_WINDOW_OBJECT_INSPECTOR, MF_BYCOMMAND | (IsPanelVisible("object_inspector") ? MF_CHECKED : MF_UNCHECKED));
                continue;
            case IDM_PANEL_TREE:
            case IDM_WINDOW_OBJECT_HIERARCHY:
                if (IsPanelVisible("tree_view")) HidePanel("tree_view"); else ShowPanel("tree_view");
                if (m_mainMenu) CheckMenuItem(m_mainMenu, IDM_WINDOW_OBJECT_HIERARCHY, MF_BYCOMMAND | (IsPanelVisible("tree_view") ? MF_CHECKED : MF_UNCHECKED));
                continue;
            case IDM_PANEL_INPUTS:
            case IDM_WINDOW_INPUTS:
                if (IsPanelVisible("inputs_inspector")) HidePanel("inputs_inspector"); else ShowPanel("inputs_inspector");
                if (m_mainMenu) CheckMenuItem(m_mainMenu, IDM_WINDOW_INPUTS, MF_BYCOMMAND | (IsPanelVisible("inputs_inspector") ? MF_CHECKED : MF_UNCHECKED));
                // Update list when shown
                if (IsPanelVisible("inputs_inspector")) {
                    auto pit = m_panels_.find("inputs_inspector");
                    if (pit != m_panels_.end() && pit->second.hwndChild) {
						UpdateInputsInspectorList();
                    }
                }
                continue;
            case IDM_FILE_NEW:
                SYSTEM_LOG << "Menu: File->New selected (not implemented)\n";
                continue;
            case IDM_FILE_LOAD:
                SYSTEM_LOG << "Menu: File->Load selected (not implemented)\n";
                continue;
            case IDM_FILE_SAVE:
                SYSTEM_LOG << "Menu: File->Save selected (not implemented)\n";
                continue;
            case IDM_ABOUT:
                MessageBoxA(m_mainHwnd, "Olympe Engine V2\n\nNicolas Chereau - 2025-2026\n\n n chereau@gmail.com\nhttps://github.com/Atlasbruce/Olympe-Engine/", "About Olympe Engine", MB_OK | MB_ICONINFORMATION);
                continue;
            default:
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    // Handle SDL_Event Message

}
//----------------------------------------------------------------------
void PanelManager::OnEvent(const Message& msg)
{
    switch (msg.msg_type)
    {
        case EventType::Olympe_EventType_Keyboard_Connected:
        case EventType::Olympe_EventType_Keyboard_Disconnected:
        case EventType::Olympe_EventType_Mouse_Connected:
        case EventType::Olympe_EventType_Mouse_Disconnected:
        case EventType::Olympe_EventType_Joystick_Connected:
        case EventType::Olympe_EventType_Joystick_Disconnected:
        {
            if (IsPanelVisible("inputs_inspector"))
            {
                auto pit = m_panels_.find("inputs_inspector");
                if (pit != m_panels_.end() && pit->second.hwndChild)
                {
                    UpdateInputsInspectorList();
                }
            }
            break;
        }
        default:
            break;
    }
}
#ifdef _WIN32
void PanelManager::AttachToSDLWindow(SDL_Window* sdlWindow)
{
#ifdef HAVE_SDL_SYSWM
    if (!sdlWindow) return;
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(sdlWindow, &wmi)) {
        return;
    }

    HWND hwnd = (HWND)wmi.info.win.window;
    if (!hwnd) return;

    m_mainHwnd = hwnd;

    HMENU hFile = CreatePopupMenu();
    AppendMenuA(hFile, MF_STRING, IDM_FILE_NEW, "New\tCtrl+N");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_LOAD, "Load\tCtrl+L");
    AppendMenuA(hFile, MF_STRING, IDM_FILE_SAVE, "Save\tCtrl+S");

    HMENU hWindow = CreatePopupMenu();
    AppendMenuA(hWindow, MF_STRING, IDM_WINDOW_OBJECT_INSPECTOR, "Object Inspector\tF2");
    AppendMenuA(hWindow, MF_STRING, IDM_WINDOW_OBJECT_HIERARCHY, "Object Hierarchy\tF3");
    AppendMenuA(hWindow, MF_STRING, IDM_WINDOW_LOG, "Log\tF1");

    HMENU hAbout = CreatePopupMenu();
    AppendMenuA(hAbout, MF_STRING, IDM_ABOUT, "About Olympe Engine");

    HMENU hMenu = CreateMenu();
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hFile, "File");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hWindow, "Window");
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR)hAbout, "About");

    SetMenu(hwnd, hMenu);
    m_mainMenu = hMenu;
#else
    (void)sdlWindow;
#endif
}
#endif

#if defined(_WIN32)
LRESULT CALLBACK PanelManager::PanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_SIZE:
    {
        PanelManager& mgr = PanelManager::GetInstance();
        std::lock_guard<std::mutex> lock(mgr.m_mutex_);
        for (auto &kv : mgr.m_panels_)
        {
            if (kv.second.hwnd == hwnd)
            {
                HWND hChild = kv.second.hwndChild;
                if (hChild)
                {
                    RECT rc; GetClientRect(hwnd, &rc);
                    MoveWindow(hChild, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
                }
                break;
            }
        }
        return 0;
    }
    case WM_DESTROY:
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
#endif

namespace SystemLogSink
{
    void AppendToLogWindow(const std::string& text)
    {
        PanelManager::GetInstance().AppendLog(text);
    }
}

void PanelManager::CreateInputsInspectorWindow()
{
    CreatePanel("inputs_inspector", "Inputs Inspector");
#ifdef _WIN32
    auto it = m_panels_.find("inputs_inspector");
    if (it != m_panels_.end() && it->second.hwnd)
    {
        HWND hwnd = it->second.hwnd;
        // default size similar to inspector
        SetWindowPos(hwnd, nullptr, 0, 0, InspectorPanelWidth, InspectorPanelHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
        if (it->second.hwndChild)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            MoveWindow(it->second.hwndChild, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            // Position the panel near inspector by default
            SetWindowPos(hwnd, nullptr, InspectorPanelPosX + 30, InspectorPanelPosY + 30, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
        }
        // populate initial list
        UpdateInputsInspectorList();
    }
#endif
}

void PanelManager::UpdateInputsInspectorList()
{
#ifdef _WIN32
    auto it = m_panels_.find("inputs_inspector");
    if (it == m_panels_.end()) return;
    if (!it->second.hwndChild) return;
    std::string text = InputsManager::Get().GetDevicesStatusUpdate();
    SetWindowTextA(it->second.hwndChild, text.c_str());
#endif
}
