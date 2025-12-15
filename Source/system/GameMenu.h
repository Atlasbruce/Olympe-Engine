#pragma once
#include <vector>
#include <string>
#include "system_utils.h"
#include "../object.h"

// GameMenu is an instance-based menu attached to a VideoGame object.
// It receives events (via OnEvent(Message& msg) forwarding) and can be used
// to navigate a set of menu entries. It is also a GameEntity-like
// entity so it can be added to the engine's object list if desired.

class GameMenu 
{
public:
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

    // Called by EventManager when messages arrive (overrides Object::OnEvent)
    virtual void OnEvent(const Message& msg) ;

    // Render simplified text-based menu (placeholder)
    virtual void Render() ;

private:
    string name;
    bool m_active = false;
    std::vector<std::string> m_entries;
    int m_selected = 0;
};
