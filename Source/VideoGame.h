/* VideoGame.h
 High-level videogame class that composes World, GameRules, QuestManager and GameMenu.
*/
#pragma once

#include "object.h"
#include "World.h"
#include "GameRules.h"
#include "QuestManager.h"
#include "system/GameMenu.h"
#include "system/CameraManager.h"
#include "system/ViewportManager.h"
#include "system/JoystickManager.h"
#include "system/EventManager.h"
#include "system/message.h"
#include "GameState.h"
#include <memory>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "system/system_utils.h"
#include "GameEngine.h"

class VideoGame 
{
public:
    VideoGame();

    virtual ~VideoGame();

    // Per-class singleton accessors
    static VideoGame& GetInstance()
    {
       static VideoGame instance;
       return instance;
    }
    static VideoGame& Get() { return GetInstance(); }


     // Game state helpers (front-end to GameStateManager)
     void SetState(GameState s)
     {
         GameStateManager::SetState(s);
         m_state = s;
     }
     GameState GetState() const { return GameStateManager::GetState(); }
     bool IsPaused() const { return GameStateManager::IsPaused(); }

     void Pause() { SetState(GameState::GameState_Paused); }
     void Resume() { SetState(GameState::GameState_Running); }
     void RequestQuit() { SetState(GameState::GameState_Quit); }

     // Player management
     void SetViewportLayout(short playerID);
     EntityID AddPlayerEntity(string _playerPrefabName = "PlayerEntity");
	 bool RemovePlayerEntity(const EntityID eid);

     // Event handler for EventManager messages registered in ctor
     void OnEvent(const Message& msg);

     // Save / Load game state (slot optional)
     bool SaveGame(int slot = 0) { return false; }
     bool LoadGame(int slot = 0) { return false; }

     virtual void RegisterPrefabItems();
     //{
         // Example prefab registration
         /*
         PrefabFactory::Get().RegisterPrefab("MiliceGuard", [](EntityID id) {
             World& world = World::Get();
             world.AddComponent<TransformComponent>(id, Vector(0, 0, 0), 100.0f, 150.0f);
             world.AddComponent<SpriteComponent>(id, "milice_guard_sprite.png");
             world.AddComponent<AIComponent>(id, AIType::Guard);
             // Add other components as needed
         });
         /**/
	//}

public:
	World& world = World::GetInstance();
	GameRules& gamerules = GameRules::GetInstance();
	GameMenu& gamemenu = GameMenu::GetInstance();
	QuestManager& questmanager = QuestManager::GetInstance();
	CameraManager& cameramanager = CameraManager::GetInstance();
    ViewportManager& viewport = ViewportManager::GetInstance();

    // Players & Entities
	std::vector<EntityID> m_playersEntity;
    static short m_playerIdCounter;

private:
    string name;
     // cached state for quick local reads (authoritative value lives in GameStateManager)
     GameState m_state = GameState::GameState_Running;

     bool m_keyboardAssigned = false;

     // key debounce flags for numpad +/-
     bool m_kpPlusPressed = false;
     bool m_kpMinusPressed = false;
};
