/**
 * @file GameEngine.h
 * @brief Core game engine class
 * @author Nicolas Chereau
 * @date 2025
 * 
 * Purpose:
 * - Class representing the core game engine
 * - GameEngine stores the list of all game objects
 */
#pragma once
#include <SDL3/SDL.h>
#include "system/system_utils.h"
#include <string>

using namespace std;

// Forward declarations to avoid circular includes
class InputsManager;
class SystemMenu;
class VideoGame;
class OptionsManager;
class DataManager;

/**
 * @brief Main game engine singleton
 * 
 * Manages all subsystems including input, rendering, and world management.
 * Handles the main game loop timing via delta time calculation.
 */
class GameEngine
{
	public:
		/**
		 * @brief Constructor for GameEngine
		 */
		GameEngine()
		{
			name = "GameEngine";
			SYSTEM_LOG << "GameEngine created and Initialized\n";
		}
		
		/**
		 * @brief Destructor for GameEngine
		 */
		virtual ~GameEngine() 	
		{
			SYSTEM_LOG << "GameEngine destroyed\n";
		}

		//-------------------------------------------------------------
		/**
		 * @brief Get singleton instance
		 * @return Reference to GameEngine instance
		 */
		static GameEngine& GetInstance()
		{
			static GameEngine instance;
			return instance;
		}
		
		/**
		 * @brief Get singleton instance (short form)
		 * @return Reference to GameEngine instance
		 */
		static GameEngine& Get() { return GetInstance(); }

		/**
		 * @brief Initialize all engine subsystems
		 * 
		 * Must be called once before using the engine.
		 * Initializes InputManager, DataManager, CameraSystem, etc.
		 */
		void Initialize();

		//-------------------------------------------------------------
		/**
		 * @brief Process one frame
		 * 
		 * Calculates delta time and updates all systems.
		 */
		void Process();

		//-------------------------------------------------------------
	public:
		/** @brief Engine name identifier */
		string name = "";
		
		/** @brief Pointer to input manager subsystem */
		InputsManager* ptr_inputsmanager = nullptr;
		
		/** @brief Pointer to system menu subsystem */
		SystemMenu* ptr_systemmenu = nullptr;
		
		/** @brief Pointer to video game subsystem */
		VideoGame* ptr_videogame = nullptr;
		
		/** @brief Pointer to options manager subsystem */
		OptionsManager* ptr_optionsmanager = nullptr;
		
		/** @brief Pointer to data manager subsystem */
		DataManager* ptr_datamanager = nullptr;

		/** @brief Reference to input manager */
		InputsManager & inputsmanager = *ptr_inputsmanager;
		
		/** @brief Reference to system menu */
		SystemMenu& systemmenu = *ptr_systemmenu;
		
		/** @brief Reference to video game */
		VideoGame& videogame = *ptr_videogame;
		
		/** @brief Reference to options manager */
		OptionsManager& optionsmanager = *ptr_optionsmanager;
		
		/** @brief Reference to data manager */
		DataManager& datamanager = *ptr_datamanager;

		/** @brief Delta time between frames in seconds */
		static float fDt;
		
		/** @brief Screen width in pixels */
		static int screenWidth;
		
		/** @brief Screen height in pixels */
		static int screenHeight;

		/** @brief Main SDL renderer */
		static SDL_Renderer* renderer;
		
		/**
		 * @brief Get the main SDL renderer
		 * @return Pointer to SDL_Renderer
		 */
		SDL_Renderer* GetMainRenderer() const {  return renderer; }
};