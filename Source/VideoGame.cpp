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
#include "system/EventQueue.h"
#include "ECS_Components_AI.h"

short VideoGame::m_playerIdCounter = 0;
using namespace std;
using IM = InputsManager;

VideoGame::VideoGame()
{
	name = "VideoGame";
        
	// Initialize viewport manager
	viewport.Initialize(GameEngine::screenWidth, GameEngine::screenHeight);

	// Ensure default state is running
	GameStateManager::SetState(GameState::GameState_Running);
	
	// Register all prefab items for the game
    RegisterPrefabItems();

    PrefabFactory::Get().CreateEntity("OlympeIdentity");
    
    // Initialize AI test scene (NPC "garde" with patrol)
    InitializeAITestScene();

	SYSTEM_LOG << "VideoGame created\n";
}
//-------------------------------------------------------------
VideoGame::~VideoGame()
{
	SYSTEM_LOG << "VideoGame destroyed\n";
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
    Message msg = Message::Create(
        EventType::Olympe_EventType_Camera_Target_Follow,
        EventDomain::Camera,
        -1,
        -1,
        eID
    );
    msg.param1 = binding.playerIndex;

    EventQueue::Get().Push(msg);

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
            // Safe cast: controllerID validated as >= 0, matches SDL_JoystickID range
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
void VideoGame::RegisterPrefabItems()
{
	// PLAYER PREFAB
    PrefabFactory::Get().RegisterPrefab("PlayerEntity", [](EntityID id) {
        World& world = World::Get();
		world.AddComponent<Identity_data>(id, string("Player_" + to_string(id)), "Player", EntityType::Player);

        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "PlayerEntity";
        static VisualSprite_data* st_vspriteData_ptr = nullptr;
        string str_index = to_string(Random_Int(1, 15));
        st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName + str_index, "Resources/SpriteEntities/entity_" + str_index + ".png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        st_vspriteData_ptr->color = Random_Color(50, 255);
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite);// .srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        world.AddComponent<PlayerBinding_data>(id);// , (short)++m_playerIdCounter, (short)-1); // default to keyboard
        world.AddComponent<Controller_data>(id);// , (short)-1/*controller index*/, false, false);
        world.AddComponent<PlayerController_data>(id);// , Vector(), false, false, false, false, false);
		world.AddComponent<Health_data>(id, 100, 100);
        world.AddComponent<PhysicsBody_data>(id);// , Vector(), Vector(), false, 0.f, 0.f,
        // Add other components as needed
		});

	//TRIGGER PREFAB
    PrefabFactory::Get().RegisterPrefab("Trigger", [](EntityID id) {
        World& world = World::Get();
		world.AddComponent<Identity_data>(id, string("Trigger_" + to_string(id)), "Trigger", EntityType::Trigger);
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "Trigger";
        static VisualSprite_data* st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/trigger-50.png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
		});

	// WAYPOINT PREFAB
    PrefabFactory::Get().RegisterPrefab("Waypoint", [](EntityID id) {
        World& world = World::Get();
		world.AddComponent<Identity_data>(id, string("Waypoint_" + to_string(id)), "Waypoint", EntityType::Waypoint);
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "Waypoint";
        static VisualSprite_data* st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/waypoint-64.png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
		world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
		});

	//NPC PREFAB
    PrefabFactory::Get().RegisterPrefab("NPCEntity", [](EntityID id) {
        World& world = World::Get();
		world.AddComponent<Identity_data>(id, string("Npc_" + to_string(id)), "Npc", EntityType::NPC);
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "NPCEntity";
        static VisualSprite_data * st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/entity_" + to_string(Random_Int(1, 15)) + ".png");
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
    
	//GUARD NPC PREFAB (AI-enabled)
    PrefabFactory::Get().RegisterPrefab("GuardNPC", [](EntityID id) {
        World& world = World::Get();
		world.AddComponent<Identity_data>(id, string("GuardNPC_" + to_string(id)), "GuardNPC", EntityType::NPC);
        world.AddComponent<Position_data>(id, Vector(0, 0, 0));
        string prefabName = "GuardNPC";
        static VisualSprite_data* st_vspriteData_ptr = DataManager::Get().GetSprite_data(prefabName, "Resources/SpriteEntities/entity_4.png");
        if (!st_vspriteData_ptr)
        {
            SYSTEM_LOG << "PrefabFactory: Failed to load sprite data for " + prefabName + " \n";
            return;
        }
        st_vspriteData_ptr->color = SDL_Color{255, 0, 0, 255}; // Red color for guards
        VisualSprite_data st_vsprite = *st_vspriteData_ptr;
        world.AddComponent<VisualSprite_data>(id, st_vsprite.srcRect, st_vsprite.sprite, st_vsprite.hotSpot);
        world.AddComponent<BoundingBox_data>(id, SDL_FRect{ 0.f, 0.f, st_vsprite.srcRect.w, st_vsprite.srcRect.h });
        world.AddComponent<Health_data>(id, 100, 100);
        world.AddComponent<Movement_data>(id);
        world.AddComponent<PhysicsBody_data>(id, 1.0f, 120.0f);
        
        // AI components
        world.AddComponent<AIBlackboard_data>(id);
        world.AddComponent<AISenses_data>(id);
        world.AddComponent<AIState_data>(id);
        world.AddComponent<BehaviorTreeRuntime_data>(id);
        world.AddComponent<MoveIntent_data>(id);
        world.AddComponent<AttackIntent_data>(id);
        
        // Configure AI senses
        AISenses_data& senses = world.GetComponent<AISenses_data>(id);
        senses.visionRadius = 200.0f; // 2m detection range (assuming 100 units = 1m)
        senses.hearingRadius = 600.0f;
        senses.perceptionHz = 5.0f;
        senses.thinkHz = 10.0f;
        
        // Start in patrol mode
        AIState_data& state = world.GetComponent<AIState_data>(id);
        state.currentMode = AIMode::Patrol;
        state.combatEngageDistance = 200.0f; // 2m
        
        // Activate behavior tree (Patrol tree = ID 2)
        BehaviorTreeRuntime_data& btRuntime = world.GetComponent<BehaviorTreeRuntime_data>(id);
        btRuntime.treeAssetId = 2; // Patrol tree
        btRuntime.isActive = true;
		});
    
	//OLYMPE LOGO PREFAB
    PrefabFactory::Get().RegisterPrefab("OlympeIdentity", [](EntityID id) {
        World& world = World::Get();
        world.AddComponent<Identity_data>(id, string("OlympeIdentity_" + to_string(id)), "Logo", EntityType::UIElement);
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
//-------------------------------------------------------------
void VideoGame::InitializeAITestScene()
{
    SYSTEM_LOG << "VideoGame: Initializing AI Test Scene...\n";
    
    // Create a guard NPC "garde" at position (400, 300)
    EntityID garde = PrefabFactory::Get().CreateEntity("GuardNPC");
    
    if (garde != INVALID_ENTITY_ID)
    {
        // Set initial position
        Position_data& pos = world.GetComponent<Position_data>(garde);
        pos.position = Vector(400.0f, 300.0f, 0.0f);
        
        // Configure patrol waypoints (square patrol pattern)
        AIBlackboard_data& blackboard = world.GetComponent<AIBlackboard_data>(garde);
        blackboard.patrolPoints[0] = Vector(300.0f, 200.0f, 0.0f);
        blackboard.patrolPoints[1] = Vector(500.0f, 200.0f, 0.0f);
        blackboard.patrolPoints[2] = Vector(500.0f, 400.0f, 0.0f);
        blackboard.patrolPoints[3] = Vector(300.0f, 400.0f, 0.0f);
        blackboard.patrolPointCount = 4;
        blackboard.currentPatrolPoint = 0;

		//create waypoints entities for visualization
		for (int i = 0; i < blackboard.patrolPointCount; ++i)
		{
			EntityID waypoint = PrefabFactory::Get().CreateEntity("Waypoint");

			if (waypoint != INVALID_ENTITY_ID)
			{
				Position_data& wpPos = world.GetComponent<Position_data>(waypoint);
				wpPos.position = blackboard.patrolPoints[i];
			}
		}

        SYSTEM_LOG << "VideoGame: Created guard NPC 'garde' (Entity " << garde << ") with 4 waypoints\n";
        SYSTEM_LOG << "  - Patrol waypoints: (300,200), (500,200), (500,400), (300,400)\n";
        SYSTEM_LOG << "  - Detection range: 200 units (~2m)\n";
        SYSTEM_LOG << "  - Will attack player if within 2m, otherwise patrol\n";
    }
    else
    {
        SYSTEM_LOG << "VideoGame: ERROR - Failed to create guard NPC\n";
    }
}

