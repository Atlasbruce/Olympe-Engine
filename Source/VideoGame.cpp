/*
Purpose: Implementation of the VideoGame class, which represents a video game with attributes such as title, genre, and platform.
*/

#include "VideoGame.h"
#include "DataManager.h"
#include <sstream>
#include <string>
#include "InputsManager.h"
#include "prefabfactory.h"
#include "engine_utils.h"
#include "ECS_Systems.h"

short VideoGame::m_playerIdCounter = 0;
using namespace std;
using IM = InputsManager;

VideoGame::VideoGame()
{
	name = "VideoGame";

	// Register to EventManager for game events
	using EM = EventManager;
	EM::Get().Register(this, EventType::Olympe_EventType_Game_AddPlayer);
	EM::Get().Register(this, EventType::Olympe_EventType_Game_RemovePlayer);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyDown);
	EM::Get().Register(this, EventType::Olympe_EventType_Keyboard_KeyUp);
        
	// Initialize viewport manager
	viewport.Initialize(GameEngine::screenWidth, GameEngine::screenHeight);

	// Ensure default state is running
	GameStateManager::SetState(GameState::GameState_Running);
	
	// Register all prefab items for the game
    RegisterPrefabItems();

    PrefabFactory::Get().CreateEntity("OlympeIdentity");

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
// Player management: supports up to 8 players
// Returns assigned player ID  -1 on failure
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
        IM::Get().AddPlayerEntityIndex(binding.playerIndex, eID);
        binding.controllerID = IM::Get().AutoBindControllerToPlayer(binding.playerIndex);
		controller.controllerID = binding.controllerID;		
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

    // Bind camera input to the same device as the player
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (camSys)
    {
        // Get or create camera for this player
        EntityID cameraEntity = camSys->GetCameraEntityForPlayer(binding.playerIndex);
        if (cameraEntity == INVALID_ENTITY_ID)
        {
            // Camera doesn't exist yet, create it
            cameraEntity = camSys->CreateCameraForPlayer(binding.playerIndex, false);
            SYSTEM_LOG << "VideoGame::AddPlayerEntity: Created camera " << cameraEntity << " for player " << binding.playerIndex << "\n";
        }
        
        // Bind camera to the same input device as the player
        if (binding.controllerID == -1)
        {
            // Keyboard-bound player: bind camera to keyboard
            camSys->BindCameraToKeyboard(cameraEntity);
            SYSTEM_LOG << "VideoGame::AddPlayerEntity: Bound camera to keyboard for player " << binding.playerIndex << "\n";
            
            // Disable keyboard binding on default camera (player -1) if it exists
            EntityID defaultCamera = camSys->GetCameraEntityForPlayer(-1);
            if (defaultCamera != INVALID_ENTITY_ID)
            {
                camSys->UnbindCameraKeyboard(defaultCamera);
                SYSTEM_LOG << "VideoGame::AddPlayerEntity: Disabled keyboard binding on default camera\n";
            }
        }
        else if (binding.controllerID >= 0)
        {
            // Joystick-bound player: bind camera to joystick
            camSys->BindCameraToJoystick(cameraEntity, binding.playerIndex, (SDL_JoystickID)binding.controllerID);
            SYSTEM_LOG << "VideoGame::AddPlayerEntity: Bound camera to joystick " << binding.controllerID << " for player " << binding.playerIndex << "\n";
        }
        else
        {
            SYSTEM_LOG << "VideoGame::AddPlayerEntity: Invalid controllerID " << binding.controllerID << " for player " << binding.playerIndex << "\n";
        }
    }

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
	// FOR TESTS : Add/Remove player with Enter and Backspace keys -----------------------------
    case EventType::Olympe_EventType_Keyboard_KeyDown:
    {
        // msg.controlId contains SDL_Scancode
        auto sc = static_cast<SDL_Scancode>(msg.controlId);
        if (sc == SDL_SCANCODE_RETURN)
        {
            // debounce: only act on initial press
            if (!m_key_AddPlayerPressed && msg.state == 1)
            {
                m_key_AddPlayerPressed = true;
                EntityID added = AddPlayerEntity(); 
                SYSTEM_LOG << "VideoGame: Enter + pressed -> add viewport (AddPlayer returned " << added << ")\n";
            }
        }
        else if (sc == SDL_SCANCODE_BACKSPACE)
        {
            if (!m_key_RemovePlayerPressed && msg.state == 1)
            {
                m_key_RemovePlayerPressed = true;
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
        if (sc == SDL_SCANCODE_RETURN) m_key_AddPlayerPressed = false;
        if (sc == SDL_SCANCODE_BACKSPACE) m_key_RemovePlayerPressed = false;
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
        static VisualSprite_data *st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/entity_" + to_string(Random_Int(1, 15)) + ".png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        st_vspriteData_ptr->color = Random_Color(50, 255);
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        world.AddComponent<PlayerBinding_data>(id);// , (short)++m_playerIdCounter, (short)-1); // default to keyboard
        world.AddComponent<Controller_data>(id);// , (short)-1/*controller index*/, false, false);
        world.AddComponent<PlayerController_data>(id);// , Vector(), false, false, false, false, false);
		world.AddComponent<Health_data>(id, 100, 100);
        world.AddComponent<PhysicsBody_data>(id);// , Vector(), Vector(), false, 0.f, 0.f,
        // Add other components as needed
		});

	//TRIGGER PREFAB
    PrefabFactory::Get().RegisterPrefab("TriggerEntity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "TriggerEntity";
        static VisualSprite_data* st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/trigger.png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        // Add other components as needed
		});

	//NPC PREFAB
    PrefabFactory::Get().RegisterPrefab("NPCEntity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "NPCEntity";
        static VisualSprite_data * st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/npc_" + to_string(Random_Int(1, 10)) + ".png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        st_vspriteData_ptr->color = Random_Color(50, 255);
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        world.AddComponent<Health_data>(id, 100, 100);
        // Add other components as needed
		});
    
	//OLYMPE LOGO PREFAB
    PrefabFactory::Get().RegisterPrefab("OlympeIdentity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "OlympeIdentity";
        static VisualSprite_data* st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/olympe_logo.png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        // Add other components as needed
		});
}
