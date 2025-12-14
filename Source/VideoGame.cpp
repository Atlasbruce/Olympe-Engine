/*
Purpose: Implementation of the VideoGame class, which represents a video game with attributes such as title, genre, and platform.
*/

#include "VideoGame.h"
#include "DataManager.h"
#include "GameObject.h"
#include "Player.h"
#include <sstream>
#include <string>
#include "InputsManager.h"
#include "prefabfactory.h"
#include "engine_utils.h"

short VideoGame::m_playerIdCounter = 0;
using namespace std;
using IM = InputsManager;

VideoGame::VideoGame()
{
	name = "VideoGame";

	// Register to EventManager for game events
	using EM = EventManager;
	/*EM::Get().Register(this, EventType::Olympe_EventType_Game_Pause, [this](const Message& m) { this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_Resume, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_Quit, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_Restart, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_AddPlayer, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_RemovePlayer, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_TakeScreenshot, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_SaveState, [this](const Message& m){ this->OnEvent(m); });
	EM::Get().Register(this, EventType::Olympe_EventType_Game_LoadState, [this](const Message& m){ this->OnEvent(m); });/**/
	EM::Get().Register(this, EventType::Olympe_EventType_Game_AddPlayer);
	EM::Get().Register(this, EventType::Olympe_EventType_Game_RemovePlayer);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyDown);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyUp);
        
	// Initialize viewport manager
	viewport.Initialize(GameEngine::screenWidth, GameEngine::screenHeight);

	// Ensure default state is running
	GameStateManager::SetState(GameState::GameState_Running);
	
	// OLD GameObject Legacy - - TO BE MIGRATED AND REMOVED
	testGao = (GameObject*)ObjectFactory::Get().CreateObject("GameObject");
	testGao->name = "OlympeSystem";
	ObjectFactory::Get().AddComponent("OlympeSystem", testGao);

	// Register all prefab items for the game
    RegisterPrefabItems();

	SYSTEM_LOG << "VideoGame created\n";
}
//-------------------------------------------------------------
VideoGame::~VideoGame()
{
	SYSTEM_LOG << "VideoGame destroyed\n";
	// Unregister from EventManager
	EventManager::Get().UnregisterAll(this);
}
//-------------------------------------------------------------
// Player management: supports up to 4 players
// Returns assigned player ID  -1 on failure
short VideoGame::AddPlayerObject(string _playerclassname)
{
	// check if there is some input devices available
    if (IM::Get().GetAvailableJoystickCount() <= 0)
    {
        if (! IM::Get().IsKeyboardAssigned())
        {
            SYSTEM_LOG << "VideoGame::AddPlayer: No joysticks available, but keyboard is free to use\n";
        }
		else
            {
                SYSTEM_LOG << "VideoGame::AddPlayer: No input devices available to add a new player\n";
                return -1;
            }
	}


	// check if class name is valid and registered in Factory
    if (_playerclassname.empty() || ! ObjectFactory::Get().IsRegistered(_playerclassname))
    {
        SYSTEM_LOG << "VideoGame::AddPlayer: Player class name '" << _playerclassname << "' not found in Factory, using default 'Player'\n";
        _playerclassname = "Player";
    }

    GameObject* player = (GameObject*)ObjectFactory::Get().CreateObject(_playerclassname);
	player->name = "Player_" + std::to_string(m_playerIdCounter);
	m_playersObject.push_back(player);

	//Send message to ViewportManager to add a new player viewport
	Message msg;
	msg.sender = this;
	msg.targetUid = player->GetUID();
	msg.objectParamPtr = player;
	msg.param1 = ((Player*)player)->m_PlayerID; // new player ID
	msg.struct_type = EventStructType::EventStructType_Olympe;

	msg.msg_type = EventType::Olympe_EventType_Camera_Target_Follow;
	EventManager::Get().AddMessage(msg);

	// Vieport mode enabled? 

    SetViewportLayout(((Player*)player)->m_PlayerID);


	return m_playerIdCounter; // return the new player ID

}
//-------------------------------------------------------------
bool VideoGame::RemovePlayerObject(const short PlayerID)
{
	return false;
	/*auto it = std::find(m_players.begin(), m_players.end(), PlayerID);
    if (it == m_players.end()) return false;
    // free controller mapping
    auto pit = m_playerToJoystick.find(PlayerID);
    if (pit != m_playerToJoystick.end())
    {
        if (pit->second == SDL_JoystickID(-1)) m_keyboardAssigned = false;
        m_playerToJoystick.erase(pit);
    }
    m_players.erase(it);
    Viewport::Get().RemovePlayer(PlayerID);
    Camera::Get().RemoveCameraForPlayer(PlayerID);
    SYSTEM_LOG << "VideoGame: Removed player " << PlayerID << "\n";
    return true;/**/
}
//-------------------------------------------------------------
EntityID VideoGame::AddPlayerEntity(string _playerPrefabName)
{
    EntityID eID = PrefabFactory::Get().CreateEntity("PlayerEntity");
	m_playersEntity.push_back(eID);

    if (eID == INVALID_ENTITY_ID)
    {
        SYSTEM_LOG << "VideoGame::AddPlayerEntity: Failed to create player entity from prefab '" << _playerPrefabName << "'\n";
        return INVALID_ENTITY_ID;
	}

	// bind input components with player ID
	PlayerBinding_data &binding = world.GetComponent<PlayerBinding_data>(eID);
	Controller_data& controller = world.GetComponent<Controller_data>(eID);

	binding.playerIndex = ++m_playerIdCounter;
	binding.controllerID = -1; // keyboard by default

	// assign controller (if available)
	if (IM::Get().GetAvailableJoystickCount() > 0)
	{
        binding.controllerID = IM::Get().AutoBindControllerToPlayer(binding.playerIndex);
		controller.controllerID = binding.controllerID;
		IM::Get().AddPlayerEntityIndex(binding.playerIndex, eID);
		SYSTEM_LOG << "VideoGame::AddPlayerEntity: Player " << binding.playerIndex << " bound to controller " << binding.controllerID << "\n";
	}
    else
    {
        SYSTEM_LOG << "VideoGame::AddPlayerEntity: No available controllers to bind to player " << binding.playerIndex << "\n";
	}

    //Send message to ViewportManager to add a new player viewport
    Message msg;
    msg.targetUid = eID;
    msg.param1 = binding.playerIndex;
    msg.struct_type = EventStructType::EventStructType_Olympe;

    msg.msg_type = EventType::Olympe_EventType_Camera_Target_Follow;
    EventManager::Get().AddMessage(msg);

    SetViewportLayout(binding.playerIndex);

    return eID;
}
//-------------------------------------------------------------
bool VideoGame::RemovePlayerEntity(const EntityID eid)
{
    return false;
}
//-------------------------------------------------------------
void VideoGame::SetViewportLayout(short playerID)
{
	short nbPLayer = static_cast<short>(m_playersEntity.size());

	switch (nbPLayer)
    {
    case 1:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid1x1);
        break;
    case 2:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid2x1);
        break;
    case 3:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid3x1);
        break;
    case 4:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid2x2);
        break;
    case 5:
    case 6:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid3x2);
        break;
    case 7:
    case 8:
        viewport.AddPlayer(playerID, ViewportLayout::ViewportLayout_Grid4x2);
        break;

    default:
        break;
    }
}
//-------------------------------------------------------------
// Event handler for EventManager messages registered in ctor
void VideoGame::OnEvent(const Message& msg)
{
    switch (msg.msg_type)
    {
    case EventType::Olympe_EventType_Game_Pause:
        Pause();
        SYSTEM_LOG << "VideoGame: Paused via event\n";
        break;
    case EventType::Olympe_EventType_Game_Resume:
        Resume();
        SYSTEM_LOG << "VideoGame: Resumed via event\n";
        break;
    case EventType::Olympe_EventType_Game_Quit:
        RequestQuit();
        SYSTEM_LOG << "VideoGame: Quit requested via event\n";
        break;
    case EventType::Olympe_EventType_Game_Restart:
        // Placeholder: restart current level (not implemented fully)
        SYSTEM_LOG << "VideoGame: Restart requested via event (not implemented)\n";
        break;
    case EventType::Olympe_EventType_Game_AddPlayer:
    {
        bool ok = (AddPlayerObject() != -2);
        SYSTEM_LOG << "VideoGame: AddPlayer event -> " << (ok ? "success" : "failed") << "\n";
        break;
    }
    case EventType::Olympe_EventType_Game_RemovePlayer:
    {
        if (msg.controlId >= 0)
        {
            short pid = static_cast<short>(msg.controlId);
            bool ok = RemovePlayerObject(pid);
            SYSTEM_LOG << "VideoGame: RemovePlayer event for " << pid << " -> " << (ok ? "removed" : "not found") << "\n";
        }
        break;
    }
    case EventType::Olympe_EventType_Game_TakeScreenshot:
        // Not implemented: placeholder
        SYSTEM_LOG << "VideoGame: TakeScreenshot event (not implemented)\n";
        break;
    case EventType::Olympe_EventType_Game_SaveState:
    {
        int slot = msg.controlId;
        SYSTEM_LOG << "VideoGame: SaveState event slot=" << slot << "\n";
        SaveGame(slot);
        break;
    }
    case EventType::Olympe_EventType_Game_LoadState:
    {
        int slot = msg.controlId;
        SYSTEM_LOG << "VideoGame: LoadState event slot=" << slot << "\n";
        LoadGame(slot);
        break;
    }
    case EventType::Olympe_EventType_Keyboard_KeyDown:
    {
        // msg.controlId contains SDL_Scancode
        auto sc = static_cast<SDL_Scancode>(msg.controlId);
        if (sc == SDL_SCANCODE_KP_PLUS)
        {
            // debounce: only act on initial press
            if (!m_kpPlusPressed && msg.state == 1)
            {
                m_kpPlusPressed = true;
                short added = AddPlayerEntity(); //AddPlayerObject();
                SYSTEM_LOG << "VideoGame: Numpad + pressed -> add viewport (AddPlayer returned " << added << ")\n";
            }
        }
        else if (sc == SDL_SCANCODE_KP_MINUS)
        {
            if (!m_kpMinusPressed && msg.state == 1)
            {
                m_kpMinusPressed = true;
                if (!m_playersEntity.empty())
                {
                    //short pid = ((Player*)(m_playersObject.back()))->m_PlayerID;
                    //bool ok = RemovePlayerObject(pid);

                    EntityID pid = m_playersEntity.back();
					bool ok = RemovePlayerEntity(pid);
                    SYSTEM_LOG << "VideoGame: Numpad - pressed -> remove viewport/player " << pid << " -> " << (ok ? "removed" : "failed") << "\n";
                }
            }
        }
        break;
    }
    case EventType::Olympe_EventType_Keyboard_KeyUp:
    {
        auto sc = static_cast<SDL_Scancode>(msg.controlId);
        if (sc == SDL_SCANCODE_KP_PLUS) m_kpPlusPressed = false;
        if (sc == SDL_SCANCODE_KP_MINUS) m_kpMinusPressed = false;
        break;
    }
    default:
        break;
    }
}
//-------------------------------------------------------------
void VideoGame::RegisterPrefabItems()
{
	// PLAYER PREFAB
    PrefabFactory::Get().RegisterPrefab("PlayerEntity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "PlayerEntity";
        VisualSprite_data* st_vsprite_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/entity_" + to_string(Random_Int(1, 15)) + ".png");
        if (!st_vsprite_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vsprite_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        world.AddComponent<PlayerBinding_data>(id);// , (short)++m_playerIdCounter, (short)-1); // default to keyboard
        world.AddComponent<Controller_data>(id);// , (short)-1/*controller index*/, false, false);
        world.AddComponent<PlayerController_data>(id);// , Vector(), false, false, false, false, false);
        // Add other components as needed
		});

	//TRIGGER PREFAB
    PrefabFactory::Get().RegisterPrefab("TriggerEntity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "TriggerEntity";
        VisualSprite_data* st_vsprite_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/trigger.png");
        if (!st_vsprite_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vsprite_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        // Add other components as needed
		});
    
}
