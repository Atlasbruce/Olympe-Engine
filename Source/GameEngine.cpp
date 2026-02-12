/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing the core game engine
- Gamengine stores the list of all game objects
- Initializes core subsystems
- Manages the main game loop timing (delta time)

*/
#pragma once

#include "GameEngine.h"
#include "InputsManager.h"
#include "system/SystemMenu.h"
#include "VideoGame.h"
#include "OptionsManager.h"
#include "DataManager.h"
#include "PanelManager.h"
#include "World.h"
#include "system/CameraEventHandler.h"
#include "AI/BehaviorTree.h"
#include "Animation/AnimationManager.h"

float GameEngine::fDt = 0.0f;
SDL_Renderer* GameEngine::renderer = nullptr;
int GameEngine::screenWidth = 1280;
int GameEngine::screenHeight = 720;
//-------------------------------------------------------------
void GameEngine::Initialize()
{
	ptr_inputsmanager = &InputsManager::GetInstance();
	ptr_systemmenu = &SystemMenu::GetInstance();
	ptr_videogame = &VideoGame::GetInstance();
	ptr_optionsmanager = &OptionsManager::GetInstance();
	ptr_datamanager = &DataManager::GetInstance();

	// Preload system resources from olymp.ini
	DataManager::Get().PreloadSystemResources("./olympe.ini");

	// Create and initialize panel manager
	//PanelManager::Get().Initialize();
	//PanelManager::Get().CreateMainMenuWindow();
	//PanelManager::Get().CreateLogWindow();
	//PanelManager::Get().CreateObjectInspectorWindow();
	//PanelManager::Get().CreateTreeViewWindow();	
	//PanelManager::Get().CreateInputsInspectorWindow();
	// By default keep them hidden; can be shown by the UI later
	
	// Initialize camera event handler
	CameraEventHandler::Get().Initialize();
	
	// Load AI Behavior Trees
	SYSTEM_LOG << "Loading AI Behavior Trees...\n";
	BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/idle.json", 1);
	BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_patrol.json", 2);
	BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/guard_combat.json", 3);
	BehaviorTreeManager::Get().LoadTreeFromFile("Blueprints/AI/investigate.json", 5);
	SYSTEM_LOG << "AI Behavior Trees loaded.\n";
	
	// Initialize Animation System
	SYSTEM_LOG << "Initializing Animation System...\n";
	OlympeAnimation::AnimationManager::Get().Init();
	OlympeAnimation::AnimationManager::Get().LoadAnimationBanks("Gamedata/Animations/AnimationBanks");
	OlympeAnimation::AnimationManager::Get().LoadAnimationGraphs("Gamedata/Animations/AnimationGraphs");
	SYSTEM_LOG << "Animation System initialized.\n";
	
	// Create default camera for player 0 with keyboard controls
	CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
	if (camSys)
	{
		EntityID defaultCamera = camSys->CreateCameraForPlayer(-1, true); // true = bind to keyboard
		SYSTEM_LOG << "Created default ECS camera for player -1 (Entity " << defaultCamera << ")\n";
	}
}
//-------------------------------------------------------------
void GameEngine::Process()
{
	// Calculate Delta Time for th entire engine cycle
	const Uint64 now = SDL_GetTicks();
	static Uint64 last_time = 0;
	fDt = ((float)(now - last_time)) / 1000.0f;  /* seconds since last iteration */
	last_time = now;
}
//-------------------------------------------------------------