#pragma once

#include "../object.h"
#include <vector>
#include <string>
#include "system_utils.h"

// Lightweight System menu singleton used by GameEngine to provide global
// application-level menu controls (show/hide, basic commands).
// This implementation is intentionally minimal and independent from the
// event system so it can be activated simply by calling SystemMenu::Get().

class SystemMenu 
{
public:
    SystemMenu()
    {
        // default entries
        m_items.push_back("About");
        m_items.push_back("Settings");
        m_items.push_back("Exit");
		name = "SystemMenu";
        SYSTEM_LOG << "SystemMenu Initialized\n";
    }
    virtual ~SystemMenu()
    {
        SYSTEM_LOG << "SystemMenu Shutdown\n";
	}

    static SystemMenu& GetInstance();
    static SystemMenu& Get() { return GetInstance(); }

    void Activate();    // make menu active / visible
    void Deactivate();  // hide menu
    void Toggle();
    bool IsActive() const { return m_active; }

    // Basic operations that a system menu might expose
    void AddItem(const std::string& item);
    void RemoveItem(const std::string& item);
    const std::vector<std::string>& Items() const { return m_items; }

private:
    string name;
    bool m_active = false;
    std::vector<std::string> m_items;
};
