#pragma once
#include "system/JoystickManager.h"
#include "system/KeyboardManager.h"
#include "system/MouseManager.h"
#include "InputDevice.h"
#include "InputConfigLoader.h"
#include <unordered_map>
#include <vector>
#include "Ecs_Entity.h"
#include "ECS_Components.h"
#include "World.h"

// Input context types for context stack (backward compatibility)
enum class InputContext { Gameplay, UI, Editor };

class InputsManager 
{
public:
    InputsManager();
    virtual ~InputsManager();

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
        //m_playerObjectIndex.clear();
        m_keyboardAssigned = false;
    }
    
    // Initialize new input system
    void InitializeInputSystem(const std::string& configPath = "Config/olympe-config.json");
    
    // Get new input system components (for advanced usage)
    InputDeviceManager& GetDeviceManager() { return InputDeviceManager::Get(); }
    InputContextManager& GetContextManager() { return InputContextManager::Get(); }

    virtual void HandleEvent(const SDL_Event* ev);

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
    short AutoBindControllerToPlayer(short playerID)
    {
        // try to bind first available joystick
        auto joysticks = JoystickManager::Get().GetConnectedJoysticks();
        for (auto jid : joysticks)
        {
            if (BindControllerToPlayer(playerID, jid)) 
                return jid;
        }

		// Failled to bind joystick,
		SYSTEM_LOG << "No available joystick to bind to player " << playerID << ". Try to bind keyboard\n";

        // try to bind keyboard if no joystick available
        if (BindControllerToPlayer(playerID, SDL_JoystickID(-1))) 
        {
			SYSTEM_LOG << "Player " << playerID << " bound to keyboard\n";
            return -1;
        }
        else
        {
            SYSTEM_LOG << "Failed to bind keyboard to player " << playerID << " keyboard already assigned to playerID :" << GetPlayerForController( SDL_JoystickID(- 1) ) << "\n";
            return -2;
        }
	}
	////--------------------------------------------------------------
 //   bool AddPlayerObjectIndex(short playerID, Player* playerPtr)
 //   {
 //       if (m_playerObjectIndex.find(playerID) != m_playerObjectIndex.end()) return false;
 //       m_playerObjectIndex[playerID] = playerPtr;
 //       return true;
 //   }
	//--------------------------------------------------------------
    bool AddPlayerEntityIndex(short playerID, EntityID eID)
    {
        if (m_playerEntityIndex.find(playerID) != m_playerEntityIndex.end()) return false;
        m_playerEntityIndex[playerID] = eID;
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
            //m_playerObjectIndex[playerID]->m_ControllerID = controller;
            //return true;
        }
        else
            {
                // ensure joystick exists
                if (!JoystickManager::Get().IsJoystickConnected(controller)) return false;
                // ensure not already used
                for (auto& kv : m_playerBindings) if (kv.second == controller) return false;
                m_playerBindings[playerID] = controller;
            }

        auto entityPlayer = m_playerEntityIndex.find(playerID);
        if (entityPlayer != m_playerEntityIndex.end())
        {
            // Access to entity controllerdata to remove controllerID binding
            EntityID eID = entityPlayer->second;
            Controller_data& ctrl = World::Get().GetComponent<Controller_data>(eID);
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(eID);
            ctrl.controllerID = controller; // set new controller ID
            binding.controllerID = controller; // set new controller ID

            SYSTEM_LOG << "Player " << playerID << " bound to joystick " << controller << "\n";
            return true;
        }
        else
            {
                SYSTEM_LOG << "Player " << playerID << " bound to joystick " << controller << " but no entity found to update controllerID\n";
                return true;
		    }
    }
	//---------------------------------------------------------------------------------------------
    bool UnbindControllerFromPlayer(short playerID)
    {
        auto it = m_playerBindings.find(playerID);
        if (it == m_playerBindings.end()) return false;
        if (it->second == SDL_JoystickID(-1)) m_keyboardAssigned = false;
        m_playerBindings.erase(it);

		auto entityPlayer =  m_playerEntityIndex.find(playerID);
        if ( entityPlayer != m_playerEntityIndex.end() )
        {
			// Access to entity controllerdata to remove controllerID binding
			EntityID eID = entityPlayer->second;
			Controller_data& controller = World::Get().GetComponent<Controller_data>(eID);
			PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(eID);
			controller.controllerID = -2; // unbound
			binding.controllerID = -2; // unbound
            //m_playerEntityIndex.erase(entityPlayer);
		}
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
    std::string name;
    std::unordered_map<short, SDL_JoystickID> m_playerBindings;
	std::unordered_map<short, SDL_JoystickID> m_playerDisconnected;
	//std::unordered_map<short, Player*> m_playerObjectIndex;
	std::unordered_map<short, EntityID> m_playerEntityIndex;
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

