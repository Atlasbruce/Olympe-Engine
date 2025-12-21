/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing the core game engine
- Gamengine stores the list of all game objects

*/
#pragma once
#include <vector>
#include <memory>
#include "system/system_utils.h"
#include <SDL3/SDL.h>

using namespace std;

// Forward declarations to avoid circular includes
class InputsManager;
class SystemMenu;
class VideoGame;
class OptionsManager;
class DataManager;

class GameEngine
{
	public:
		//GameEngine properties and methods
		GameEngine()
		{
			name = "GameEngine";
			SYSTEM_LOG << "GameEngine created and Initialized\n";
		}
		virtual ~GameEngine() 	
		{
			SYSTEM_LOG << "GameEngine destroyed\n";
		}

		//-------------------------------------------------------------
		// Per-class singleton accessors
		static GameEngine& GetInstance()
		{
			static GameEngine instance;
			return instance;
		}
		static GameEngine& Get() { return GetInstance(); }

		void Initialize();

		//-------------------------------------------------------------
		void Process();

		//-------------------------------------------------------------
	public:
		string name = "";
		InputsManager* ptr_inputsmanager = nullptr;
		SystemMenu* ptr_systemmenu = nullptr;
		VideoGame* ptr_videogame = nullptr;
		OptionsManager* ptr_optionsmanager = nullptr;
		DataManager* ptr_datamanager = nullptr;

		InputsManager & inputsmanager = *ptr_inputsmanager;
		SystemMenu& systemmenu = *ptr_systemmenu;
		VideoGame& videogame = *ptr_videogame;
		OptionsManager& optionsmanager = *ptr_optionsmanager;
		DataManager& datamanager = *ptr_datamanager;

		static float fDt; // Delta Time between frames
		static int screenWidth;
		static int screenHeight;

		static SDL_Renderer* renderer;
		SDL_Renderer* GetMainRenderer() const {  return renderer; }
};