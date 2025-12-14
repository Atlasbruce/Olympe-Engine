/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Components purpose: Include all component definitions used in the ECS architecture.

*/

#pragma once

#include "Ecs_Entity.h"
#include <string>
#include <unordered_map>
#include "vector.h"
#include <SDL3/SDL.h>
#include "DataManager.h"

using namespace ::std;

// --- Component Position Data ---
struct Position_data
{
	Vector position; // 2D/3D position
};
// --- Component BoundingBox Data ---
struct BoundingBox_data
{
	SDL_FRect boundingBox = {0.f, 0.f, 25.f, 25.f}; // Collision rectangle
};
// --- Component Detection Data ---
struct TriggerZone_data
{
	float radius = 15.f;  // Detection radius
	bool triggered = false; // Is something detected?
};
// --- Component Movement Data ---
struct Movement_data
{
	Vector direction; // Movement direction vector
	Vector velocity; // Velocity vector
};
// --- Component Physics Data ---
struct PhysicsBody_data
{
	float mass = 1.0f;		// Mass of the body
	float speed = 150.0f;	// Movement speed in pixels/second
};
// --- Component Health Data ---
struct Health_data
{
	int currentHealth = 100; // Current health points
	int maxHealth = 100;     // Maximum health points
};
// --- Component AI Data ---
struct AIBehavior_data
{
	std::string behaviorType = "idle"; // Type of AI behavior (e.g., "patrol", "chase")
};
// --- Component Inventory Data ---
struct Inventory_data
{
	std::vector<std::string> items; // List of item IDs in the inventory
};
// --- Component Render Data --- Sprite, Animation, FX, GUI, etc.
struct VisualSprite_data
{
	SDL_FRect srcRect = { 0, 0, 25, 25 }; // Source rectangle for texture atlas
	Sprite* sprite = nullptr; // Pointer to the sprite/texture
	Vector hotSpot;        // Hotspot offset for rendering
	SDL_Color color = { 0, 0, 0, 255 }; // Color (RGBA)
};
// --- Component Animation Data ---
struct Animation_data
{
	std::string animationID; // ID of the animation
	int currentFrame = 0;    // Current frame index
	float frameDuration = 0.1f; // Duration of each frame in seconds
	float elapsedTime = 0.0f;   // Time elapsed since last frame change
};
// --- Component FX Data --- Visual effects like particles, explosions, etc.
struct FX_data
{
	std::string effectType; // Type of effect (e.g., "explosion", "smoke")
	float duration = 1.0f;  // Duration of the effect in seconds
	float elapsedTime = 0.0f; // Time elapsed since effect started
};
// --- Component Audio Data ---
struct AudioSource_data
{
	std::string soundEffectID; // ID of the sound effect to play
	float volume = 1.0f;       // Volume level (0.0 to 1.0)
};
// --- Component Controller Data ---
struct Controller_data
{
	static constexpr int MAX_BUTTONS = 16;
	
	short controllerID = -1; // Index of the controller (-1 = keyboard)
	bool isConnected = false; // Is the controller connected?
	
	// Normalized axes (deadzone applied)
	Vector leftStick;   // -1..1
	Vector rightStick;  // -1..1
	float leftTrigger = 0.f;  // 0..1
	float rightTrigger = 0.f; // 0..1
	
	// Buttons
	bool buttons[MAX_BUTTONS] = {false};
	
	// Haptics (optional)
	bool isVibrating = false;
	float vibrateStrength = 0.f;
};
// --- Component PlayerConroller Data ---
struct PlayerController_data
{
	Vector Joydirection;        // Joystick direction vector
	bool isJumping = false; // Is the player jumping?
	bool isShooting = false; // Is the player shooting?
	bool isRunning = false; // Is the player running?
	bool isInteracting = false; // Is the player interacting?
	bool isUsingItem = false; // Is the player using an item?
	bool isMenuOpen = false; // Is the game menu open?
};
// --- Component Player Binding Controller --- JoystickID, KeyboardID, etc.
struct PlayerBinding_data
{
	short playerIndex = 0; // Index of the player (e.g., Player 1, Player 2)
	short controllerID = -1; // ID of the joystick/controller
};
// --- Component Camera Data ---
struct Camera_data
{
	float zoomLevel = 1.0f; // Zoom level of the camera
	float rotation = 0.0f;  // Rotation angle of the camera in radians
	EntityID targetEntity = INVALID_ENTITY_ID; // Entity the camera is following
};

// --- Component NPC Data ---
struct NPC_data
{
	std::string npcType; // Type of NPC (e.g., "vendor", "quest_giver")
};

// --- Component Input Mapping Data ---
struct InputMapping_data
{
	// Map action name → scancode/button
	std::unordered_map<std::string, SDL_Scancode> keyboardBindings;
	std::unordered_map<std::string, int> gamepadBindings;
	float deadzone = 0.15f;
	float sensitivity = 1.0f;
	
	// Helper to initialize default bindings
	void InitializeDefaults()
	{
		// Keyboard default bindings (WASD + Arrows)
		keyboardBindings["up_alt"] = SDL_SCANCODE_W;
		keyboardBindings["down_alt"] = SDL_SCANCODE_S;
		keyboardBindings["left_alt"] = SDL_SCANCODE_A;
		keyboardBindings["right_alt"] = SDL_SCANCODE_D;
		keyboardBindings["up"] = SDL_SCANCODE_UP;
		keyboardBindings["down"] = SDL_SCANCODE_DOWN;
		keyboardBindings["left"] = SDL_SCANCODE_LEFT;
		keyboardBindings["right"] = SDL_SCANCODE_RIGHT;
		keyboardBindings["jump"] = SDL_SCANCODE_SPACE;
		keyboardBindings["shoot"] = SDL_SCANCODE_LCTRL;
		keyboardBindings["interact"] = SDL_SCANCODE_E;
		keyboardBindings["menu"] = SDL_SCANCODE_TAB;
		
		// Gamepad default bindings
		gamepadBindings["jump"] = 0;  // A button
		gamepadBindings["shoot"] = 1; // B button
		gamepadBindings["interact"] = 2; // X button
		gamepadBindings["menu"] = 7; // Start button
	}
};