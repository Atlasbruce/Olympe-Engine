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

    // Add Camera_data component - each player has a camera that follows its position
    Camera_data camera;
    camera.targetEntity = eID; // Camera tracks this player entity's position
    camera.followTarget = true;
    // Initial offset will be updated by UpdateAllPlayerViewports() based on viewport size
    camera.offset = Vector(-GameEngine::screenWidth / 2.f, -GameEngine::screenHeight / 2.f, 0.0f);
    camera.mode = Camera_data::Mode::Viewport_Follow;
    world.AddComponent<Camera_data>(eID, camera);

    // Add Viewport_data component
    Viewport_data viewport;
    viewport.viewportIndex = binding.playerIndex - 1; // 0-based index
    viewport.enabled = true;
    world.AddComponent<Viewport_data>(eID, viewport);

    // Update viewport layout and compute viewport rectangles
    SetViewportLayout(binding.playerIndex);
    
    // Update viewport rectangles for all players
    UpdateAllPlayerViewports();

    //Send message to ViewportManager to add a new player viewport (for backward compatibility)
    Message msg;
    msg.targetUid = eID;
    msg.param1 = binding.playerIndex;
    msg.struct_type = EventStructType::EventStructType_Olympe;

    msg.msg_type = EventType::Olympe_EventType_Camera_Target_Follow;
    EventManager::Get().AddMessage(msg);

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
void VideoGame::UpdateAllPlayerViewports()
{
    // Calculate viewport rectangles based on number of players
    int numPlayers = static_cast<int>(m_playersEntity.size());
    if (numPlayers == 0) return;

    int screenWidth = GameEngine::screenWidth;
    int screenHeight = GameEngine::screenHeight;

    // Calculate viewport layout based on player count
    std::vector<SDL_FRect> viewportRects;
    
    switch (numPlayers)
    {
    case 1:
        viewportRects.push_back({0.f, 0.f, static_cast<float>(screenWidth), static_cast<float>(screenHeight)});
        break;
    case 2:
        {
            float w = screenWidth / 2.f;
            viewportRects.push_back({0.f, 0.f, w, static_cast<float>(screenHeight)});
            viewportRects.push_back({w, 0.f, static_cast<float>(screenWidth) - w, static_cast<float>(screenHeight)});
        }
        break;
    case 3:
        {
            float w = screenWidth / 3.f;
            viewportRects.push_back({0.f, 0.f, w, static_cast<float>(screenHeight)});
            viewportRects.push_back({w, 0.f, w, static_cast<float>(screenHeight)});
            viewportRects.push_back({2.f * w, 0.f, static_cast<float>(screenWidth) - 2.f * w, static_cast<float>(screenHeight)});
        }
        break;
    case 4:
        {
            float w = screenWidth / 2.f;
            float h = screenHeight / 2.f;
            viewportRects.push_back({0.f, 0.f, w, h});
            viewportRects.push_back({w, 0.f, static_cast<float>(screenWidth) - w, h});
            viewportRects.push_back({0.f, h, w, static_cast<float>(screenHeight) - h});
            viewportRects.push_back({w, h, static_cast<float>(screenWidth) - w, static_cast<float>(screenHeight) - h});
        }
        break;
    default:
        // 5-8 players: 3x2 or 4x2 grid
        {
            int cols = (numPlayers <= 6) ? 3 : 4;
            int rows = 2;
            float w = screenWidth / static_cast<float>(cols);
            float h = screenHeight / static_cast<float>(rows);
            for (int i = 0; i < numPlayers; ++i)
            {
                int col = i % cols;
                int row = i / cols;
                viewportRects.push_back({col * w, row * h, w, h});
            }
        }
        break;
    }

    // Update Viewport_data for each player entity
    for (size_t i = 0; i < m_playersEntity.size() && i < viewportRects.size(); ++i)
    {
        EntityID playerEntity = m_playersEntity[i];
        if (world.HasComponent<Viewport_data>(playerEntity))
        {
            Viewport_data& viewport = world.GetComponent<Viewport_data>(playerEntity);
            viewport.viewRect = viewportRects[i];
            viewport.viewportIndex = static_cast<int>(i);
        }

        // Update camera offset based on viewport size
        if (world.HasComponent<Camera_data>(playerEntity))
        {
            Camera_data& camera = world.GetComponent<Camera_data>(playerEntity);
            camera.offset = Vector(-viewportRects[i].w / 2.f, -viewportRects[i].h / 2.f, 0.0f);
        }
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
	// FOR TESTS : Add/Remove player with Numpad +/- keys -----------------------------
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
                EntityID added = AddPlayerEntity(); //AddPlayerObject();
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
        VisualSprite_data st_vsprite_ptr = *DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/entity_" + to_string(Random_Int(1, 15)) + ".png");
        if (!st_vsprite_ptr.sprite)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        st_vsprite_ptr.color = Random_Color(50, 255);
        VisualSprite_data st_vsprite = st_vsprite_ptr;
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
