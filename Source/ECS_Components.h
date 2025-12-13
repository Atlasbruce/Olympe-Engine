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
	float speed = 50.0f;	// Movement speed in pixels/second
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
	short controllerIndex = -1; // Index of the controller
	bool isConnected = false; // Is the controller connected?
	bool isVibrating = false; // Is the controller vibrating?
	bool buttonStates[30]; // States of buttons
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
};
// --- Component Player Binding Controller --- JoystickID, KeyboardID, etc.
struct PlayerBinding_data
{
	int playerIndex = 0; // Index of the player (e.g., Player 1, Player 2)
	short joystickID = -1; // ID of the joystick/controller
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