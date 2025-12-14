#pragma once
#include "object.h"
#include "system/JoystickManager.h"
#include "system/KeyboardManager.h"
#include "system/MouseManager.h"
#include <unordered_map>
#include <vector>
#include "Player.h"
#include "Ecs_Entity.h"

// Input context types for context stack
enum class InputContext { Gameplay, UI, Editor };

class InputsManager : public Object
{
public:
    InputsManager();
    virtual ~InputsManager();

    virtual ObjectType GetObjectType() const { return ObjectType::Singleton; }

    static InputsManager& GetInstance()
    {
        static InputsManager instance;
        return instance;
    }
    static InputsManager& Get() { return GetInstance(); }

    void Shutdown()
    {
        JoystickManager::Get().Shutdown();
        KeyboardManager::Get().Shutdown();
        MouseManager::Get().Shutdown();
        m_playerBindings.clear();
		m_playerIndex.clear();
        m_keyboardAssigned = false;
    }

    virtual void HandleEvent(const SDL_Event* ev);
    virtual void OnEvent(const Message& msg) override;

	bool IsKeyboardAssigned() const { return m_keyboardAssigned; }

    string GetDevicesStatusUpdate();
    
	//--------------------------------------------------------------
    int GetConnectedJoysticksCount() const
    {
        return static_cast<int>(JoystickManager::Get().GetConnectedJoysticks().size());
	}
	//--------------------------------------------------------------
    int GetConnectedKeyboardsCount() const
    {
		return m_keyboardAssigned ? 1 : 0;
	}
	//--------------------------------------------------------------
    int GetMaxDevices() const
    {
        // Max players = number of connected joysticks + 1 (keyboard)
		return static_cast<int>(GetConnectedJoysticksCount() + GetConnectedKeyboardsCount());
	}
   	//--------------------------------------------------------------
    int GetAvailableJoystickCount() const
    {
        int count = 0;
        auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
        for (auto jid : joysticks)
        {
            bool used = false;
            for (auto& kv : m_playerBindings) 
                if (kv.second == jid) 
                {
                    used = true; break;
                }
            if (!used) ++count;
        }
        return count;
    }
	//--------------------------------------------------------------
	// Automatically bind first available controller (joystick or keyboard) to a player
    bool AutoBindControllerToPlayer(short playerID)
    {
        // try to bind first available joystick
        auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
        for (auto jid : joysticks)
        {
            if (BindControllerToPlayer(playerID, jid)) return true;
        }

		// Failled to bind joystick,
		SYSTEM_LOG << "No available joystick to bind to player " << playerID << ". Try to bind keyboard\n";

        // try to bind keyboard if no joystick available
        if (BindControllerToPlayer(playerID, SDL_JoystickID(-1))) 
        {
			SYSTEM_LOG << "Player " << playerID << " bound to keyboard\n";
            return true;
        }
        else
        {
			SYSTEM_LOG << "Failed to bind keyboard to player " << playerID << " (already assigned?)\n";
            return false;
        }
	}
	//--------------------------------------------------------------
    bool AddPlayerIndex(short playerID, Player* playerPtr)
    {
        if (m_playerIndex.find(playerID) != m_playerIndex.end()) return false;
        m_playerIndex[playerID] = playerPtr;
        return true;
    }
	//--------------------------------------------------------------
    // Bind a controller (joystick id) or keyboard (-1) to a player
    bool BindControllerToPlayer(short playerID, SDL_JoystickID controller)
    {
        // if controller == -1 -> keyboard
        if (controller == SDL_JoystickID(-1))
        {
            if (m_keyboardAssigned) return false;
            m_keyboardAssigned = true;
            m_playerBindings[playerID] = controller;
            m_playerIndex[playerID]->m_ControllerID = controller;
            return true;
        }
        // ensure joystick exists
        if (!JoystickManager::Get().IsJoystickConnected(controller)) return false;
        // ensure not already used
        for (auto &kv : m_playerBindings) if (kv.second == controller) return false;
        m_playerBindings[playerID] = controller;
        m_playerIndex[playerID]->m_ControllerID = controller;
		SYSTEM_LOG << "Player " << playerID << " named "+ ((Player*)m_playerIndex[playerID])->name +" bound to joystick " << controller << "\n";
        return true;
    }

    bool UnbindControllerFromPlayer(short playerID)
    {
        auto it = m_playerBindings.find(playerID);
        if (it == m_playerBindings.end()) return false;
        if (it->second == SDL_JoystickID(-1)) m_keyboardAssigned = false;
        m_playerBindings.erase(it);
		SYSTEM_LOG << "Player " << playerID << " unbound from controller\n";
        return true;
    }
	//--------------------------------------------------------------
	// Manage disconnected players
    bool AddDisconnectedPlayer(short playerID, SDL_JoystickID old_controller)
    {
        if (m_playerDisconnected.find(playerID) != m_playerDisconnected.end()) return false;
        m_playerDisconnected[playerID] = old_controller;
        return true;
	}
    bool RemoveDisconnectedPlayer(short playerID)
    {
        auto it = m_playerDisconnected.find(playerID);
        if (it == m_playerDisconnected.end()) return false;
        m_playerDisconnected.erase(it);
        return true;
    }
    bool IsPlayerDisconnected(short playerID) const
    {
        return m_playerDisconnected.find(playerID) != m_playerDisconnected.end();
	}
    short GetDisconnectedPlayersCount() const
    {
        return static_cast<int>(m_playerDisconnected.size());
    }
    short GetFirstDisconnectedPlayerID() const
    {
        if (m_playerDisconnected.empty()) return -1;
        return m_playerDisconnected.begin()->first;
	}


    // Query
    bool IsPlayerBound(short playerID) const { return m_playerBindings.find(playerID) != m_playerBindings.end(); }
    SDL_JoystickID GetPlayerBinding(short playerID) const
    {
        auto it = m_playerBindings.find(playerID);
        if (it == m_playerBindings.end()) return SDL_JoystickID(0);
        return it->second;
    }
    short GetPlayerForController(SDL_JoystickID controller) const
    {
        for (auto &kv : m_playerBindings)
        {
            if (kv.second == controller) return kv.first;
        }
        return -1;
	}

    // Input Context Stack
    void PushContext(InputContext ctx);
    void PopContext();
    InputContext GetActiveContext() const;

    // Input Entity Cache
    void RegisterInputEntity(EntityID e);
    void UnregisterInputEntity(EntityID e);
    const std::vector<EntityID>& GetInputEntities() const;

private:
    std::unordered_map<short, SDL_JoystickID> m_playerBindings;
	std::unordered_map<short, SDL_JoystickID> m_playerDisconnected;
	std::unordered_map<short, Player*> m_playerIndex;
    bool m_keyboardAssigned = false;
	//JoystickManager& joystickmanager = JoystickManager::GetInstance();
	//KeyboardManager& keyboardmanager = KeyboardManager::GetInstance();
	//MouseManager& mousemanager = MouseManager::GetInstance();
	std::ostringstream m_devicesStatus;

    // Context stack for input handling (Gameplay, UI, Editor)
    std::vector<InputContext> m_contextStack = { InputContext::Gameplay };

    // Cache of entities with input components for optimized iteration
    std::vector<EntityID> m_inputEntities;
};

