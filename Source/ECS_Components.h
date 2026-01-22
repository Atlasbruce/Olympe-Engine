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
#include "SDL_rect.h"



// Component type definitions
struct Identity_data
{
	std::string name = "Entity"; // Entity name
	std::string tag = "Untagged"; // Entity tag/category
	std::string type = "UnknownType"; // Entity type
	
	// Constructors
	Identity_data() = default;
	Identity_data(std::string n, std::string t, std::string et)
		: name(std::move(n)), tag(std::move(t)), type(std::move(et)) {}
	Identity_data(const Identity_data&) = default;
	Identity_data& operator=(const Identity_data&) = default;
};

// --- Component Position Data ---
struct Position_data
{
	Vector position; // 2D/3D position
	
	// Constructors
	Position_data() = default;
	Position_data(Vector pos) : position(pos) {}
	Position_data(const Position_data&) = default;
	Position_data& operator=(const Position_data&) = default;
};
// --- Component BoundingBox Data ---
struct BoundingBox_data
{
	SDL_FRect boundingBox = {0.f, 0.f, 25.f, 25.f}; // Collision rectangle
	
	// Constructors
	BoundingBox_data() = default;
	BoundingBox_data(SDL_FRect rect) : boundingBox(rect) {}
	BoundingBox_data(const BoundingBox_data&) = default;
	BoundingBox_data& operator=(const BoundingBox_data&) = default;
};
// --- Component Detection Data ---
struct TriggerZone_data
{
	float radius = 15.f;  // Detection radius
	bool triggered = false; // Is something detected?
	
	// Constructors
	TriggerZone_data() = default;
	TriggerZone_data(const TriggerZone_data&) = default;
	TriggerZone_data& operator=(const TriggerZone_data&) = default;
};
// --- Component Movement Data ---
struct Movement_data
{
	Vector direction; // Movement direction vector
	Vector velocity; // Velocity vector
	
	// Constructors
	Movement_data() = default;
	Movement_data(const Movement_data&) = default;
	Movement_data& operator=(const Movement_data&) = default;
};
// --- Component Physics Data ---
struct PhysicsBody_data
{
	float mass = 1.0f;		// Mass of the body
	float speed = 150.0f;	// Movement speed in pixels/second
	
	// Constructors
	PhysicsBody_data() = default;
	PhysicsBody_data(float m, float s) : mass(m), speed(s) {}
	PhysicsBody_data(const PhysicsBody_data&) = default;
	PhysicsBody_data& operator=(const PhysicsBody_data&) = default;
};
// --- Component Health Data ---
struct Health_data
{
	int currentHealth = 100; // Current health points
	int maxHealth = 100;     // Maximum health points
	
	// Constructors
	Health_data() = default;
	Health_data(int current, int max) : currentHealth(current), maxHealth(max) {}
	Health_data(const Health_data&) = default;
	Health_data& operator=(const Health_data&) = default;
};
// --- Component AI Data ---
struct AIBehavior_data
{
	std::string behaviorType = "idle"; // Type of AI behavior (e.g., "patrol", "chase")
	
	// Constructors
	AIBehavior_data() = default;
	AIBehavior_data(const AIBehavior_data&) = default;
	AIBehavior_data& operator=(const AIBehavior_data&) = default;
};
// --- Component Inventory Data ---
struct Inventory_data
{
	std::vector<std::string> items; // List of item IDs in the inventory
	
	// Constructors
	Inventory_data() = default;
	Inventory_data(const Inventory_data&) = default;
	Inventory_data& operator=(const Inventory_data&) = default;
};
// --- Component Render Data --- Sprite, Animation, FX, GUI, etc.
struct VisualSprite_data
{
	SDL_FRect srcRect = { 0, 0, 25, 25 }; // Source rectangle for texture atlas
	Sprite* sprite = nullptr; // Pointer to the sprite/texture
	Vector hotSpot;        // Hotspot offset for rendering
	SDL_Color color = { 255, 255, 255, 255 }; // Color (RGBA)
	
	// Constructors
	VisualSprite_data() = default;
	VisualSprite_data(SDL_FRect rect, Sprite* spr, Vector hotspot)
		: srcRect(rect), sprite(spr), hotSpot(hotspot) {}
	VisualSprite_data(const VisualSprite_data&) = default;
	VisualSprite_data& operator=(const VisualSprite_data&) = default;
	void UpdateRect()
	{
		if (sprite)
		{
			srcRect.w = static_cast<float>(sprite->w);
			srcRect.h = static_cast<float>(sprite->h);
			hotSpot.x = srcRect.w / 2.f;
			hotSpot.y = srcRect.h / 2.f;
		}
	}
};
// --- Component visual Editor Data ---
struct VisualEditor_data
{
	SDL_FRect srcRect = { 0, 0, 25, 25 }; // Source rectangle for texture atlas
	Sprite* sprite = nullptr; // Pointer to the sprite/texture
	Vector hotSpot;        // Hotspot offset for rendering
	SDL_Color color = { 255, 255, 255, 255 }; // Color (RGBA)
	bool isSelected = false; // Is the entity selected in the editor?
	bool isVisible = true;   // Is the entity visible in the editor?
	
	// Constructors
	VisualEditor_data() = default;
	VisualEditor_data(SDL_FRect rect, Sprite* spr, Vector hotspot)
		: srcRect(rect), sprite(spr), hotSpot(hotspot) {}
	VisualEditor_data(const VisualEditor_data&) = default;
	VisualEditor_data& operator=(const VisualEditor_data&) = default;
	void UpdateRect()
	{
		if (sprite)
		{
			srcRect.w = static_cast<float>(sprite->w);
			srcRect.h = static_cast<float>(sprite->h);
			hotSpot.x = srcRect.w / 2.f;
			hotSpot.y = srcRect.h / 2.f;
		}
	}
};
// --- Component Animation Data ---
struct Animation_data
{
	std::string animationID; // ID of the animation
	int currentFrame = 0;    // Current frame index
	float frameDuration = 0.1f; // Duration of each frame in seconds
	float elapsedTime = 0.0f;   // Time elapsed since last frame change
	
	// Constructors
	Animation_data() = default;
	Animation_data(const Animation_data&) = default;
	Animation_data& operator=(const Animation_data&) = default;
};
// --- Component FX Data --- Visual effects like particles, explosions, etc.
struct FX_data
{
	std::string effectType; // Type of effect (e.g., "explosion", "smoke")
	float duration = 1.0f;  // Duration of the effect in seconds
	float elapsedTime = 0.0f; // Time elapsed since effect started
	
	// Constructors
	FX_data() = default;
	FX_data(const FX_data&) = default;
	FX_data& operator=(const FX_data&) = default;
};
// --- Component Audio Data ---
struct AudioSource_data
{
	std::string soundEffectID; // ID of the sound effect to play
	float volume = 1.0f;       // Volume level (0.0 to 1.0)
	
	// Constructors
	AudioSource_data() = default;
	AudioSource_data(const AudioSource_data&) = default;
	AudioSource_data& operator=(const AudioSource_data&) = default;
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
	
	// Constructors
	Controller_data() = default;
	Controller_data(const Controller_data&) = default;
	Controller_data& operator=(const Controller_data&) = default;
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
	
	// Constructors
	PlayerController_data() = default;
	PlayerController_data(const PlayerController_data&) = default;
	PlayerController_data& operator=(const PlayerController_data&) = default;
};
// --- Component Player Binding Controller --- JoystickID, KeyboardID, etc.
struct PlayerBinding_data
{
	short playerIndex = 0; // Index of the player (e.g., Player 1, Player 2)
	short controllerID = -1; // ID of the joystick/controller
	
	// Constructors
	PlayerBinding_data() = default;
	PlayerBinding_data(const PlayerBinding_data&) = default;
	PlayerBinding_data& operator=(const PlayerBinding_data&) = default;
};

// --- Component NPC Data ---
struct NPC_data
{
	std::string npcType; // Type of NPC (e.g., "vendor", "quest_giver")
	
	// Constructors
	NPC_data() = default;
	NPC_data(const NPC_data&) = default;
	NPC_data& operator=(const NPC_data&) = default;
};

// --- Component Input Mapping Data ---
struct InputMapping_data
{
	// Map action name -> scancode/button
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
	
	// Constructors
	InputMapping_data() = default;
	InputMapping_data(const InputMapping_data&) = default;
	InputMapping_data& operator=(const InputMapping_data&) = default;
};
// --- Grid settings (singleton component) ---
enum class GridProjection : uint8_t
{
	Ortho = 0,
	Iso = 1,
	HexAxial = 2
};

struct GridSettings_data
{
	bool enabled = true;
	GridProjection projection = GridProjection::Ortho;

	// Ortho / Iso: taille cellule en unités "world"
	Vector cellSize = Vector(32.f, 32.f, 0.f);

	// Hex axial (pointy-top): rayon en unités "world"
	float hexRadius = 16.f;

	// Render
	SDL_Color color = { 180, 180, 180, 255 };
	int maxLines = 1200; // budget perf
	
	// LOD: skip lines based on zoom to avoid visual clutter
	float lodZoomThreshold = 0.5f;  // Below this zoom, apply LOD
	int lodSkipFactor = 10;          // Draw 1 line every N when LOD active
	
	// Constructors
	GridSettings_data() = default;
	GridSettings_data(const GridSettings_data&) = default;
	GridSettings_data& operator=(const GridSettings_data&) = default;
};

// Camera type enumeration
enum class CameraType : uint8_t {
	CameraType_2D = 0,        // Standard 2D camera
	CameraType_2_5D = 1,      // 2.5D camera (follows on X axis only)
	CameraType_Isometric = 2  // Isometric camera
};

// Camera control mode enumeration
enum class CameraControlMode : uint8_t {
	Mode_Free,              // Free camera movement
	Mode_Follow,            // Camera follows target strictly
	Mode_FollowWithControl  // Camera follows target + allows manual control
};

// --- Main Camera Component ---
// Contains all core camera properties including position, zoom, rotation, and control settings
struct Camera_data
{
	short playerId = -1;                      // ID of the player who owns this camera
	//EntityID playerEntityID = INVALID_ENTITY_ID;	// EntityID of the player entity
	CameraType type = CameraType::CameraType_2D; // Type of camera projection

	// Position and offset
	Vector position = { 0.f, 0.f, 0.f };       // World position of the camera
	Vector baseOffset = { 0.f, 0.f, 0.f };     // Base offset for viewport centering
	Vector controlOffset = { 0.f, 0.f, 0.f };  // Manual control offset from player

	// Zoom management
	float zoom = 1.0f;                       // Current zoom level
	float targetZoom = 1.0f;                 // Target zoom level for smooth transitions
	float zoomSpeed = 5.0f;                  // Speed of zoom interpolation
	float minZoom = 0.1f;                    // Minimum allowed zoom
	float maxZoom = 5.0f;                    // Maximum allowed zoom

	// Rotation management (in degrees)
	float rotation = 0.0f;                   // Current rotation angle
	float targetRotation = 0.0f;             // Target rotation angle for smooth transitions
	float rotationSpeed = 5.0f;              // Speed of rotation interpolation

	// Control settings
	CameraControlMode controlMode = CameraControlMode::Mode_Free; // Control mode
	SDL_FRect viewportRect = { 0.f, 0.f, 800.f, 600.f }; // Viewport rectangle

	// Control parameters
	float manualMoveSpeed = 200.0f;          // Speed of manual camera movement (pixels/sec)
	float zoomStep = 0.1f;                   // Zoom increment per input
	float rotationStep = 15.0f;              // Rotation increment per input (degrees)

	bool isActive = true;                    // Is this camera active for rendering
	
	// Constructors
	Camera_data() = default;
	Camera_data(const Camera_data&) = default;
	Camera_data& operator=(const Camera_data&) = default;
};

// --- Camera Target Component ---
// Handles target following for both ECS entities and legacy GameObjects
struct CameraTarget_data
{
	EntityID targetEntityID = INVALID_ENTITY_ID; // ECS entity to follow

	bool followTarget = false;               // Enable/disable target following
	float smoothFactor = 5.0f;               // Smoothing factor for following (0-10, higher = smoother)
	Vector followOffset = { 0.f, 0.f, 0.f };   // Additional offset when following target

	bool allowManualControl = true;          // Allow manual control while following
	float manualControlDecay = 2.0f;         // Speed at which manual offset decays back to target
	
	// Constructors
	CameraTarget_data() = default;
	CameraTarget_data(const CameraTarget_data&) = default;
	CameraTarget_data& operator=(const CameraTarget_data&) = default;
};

// --- Camera Effects Component ---
// Visual effects such as camera shake
struct CameraEffects_data
{
	bool isShaking = false;                  // Is camera shake active
	float shakeIntensity = 0.0f;             // Intensity of the shake effect
	float shakeDuration = 0.0f;              // Total duration of shake in seconds
	float shakeTimeRemaining = 0.0f;         // Time remaining for shake effect
	Vector shakeOffset = { 0.f, 0.f, 0.f };    // Current shake offset applied to position
	
	// Constructors
	CameraEffects_data() = default;
	CameraEffects_data(const CameraEffects_data&) = default;
	CameraEffects_data& operator=(const CameraEffects_data&) = default;
};

// --- Camera Bounds Component ---
// Constrains camera movement to a specific area
struct CameraBounds_data
{
	bool useBounds = false;                  // Enable/disable boundary constraints
	SDL_FRect boundingBox = { 0.f, 0.f, 0.f, 0.f }; // World space bounding box
	bool clampToViewport = true;             // Clamp camera so viewport stays within bounds
	
	// Constructors
	CameraBounds_data() = default;
	CameraBounds_data(const CameraBounds_data&) = default;
	CameraBounds_data& operator=(const CameraBounds_data&) = default;
};

// --- Camera Input Binding Component ---
// Configures input controls for the camera (keyboard or joystick)
struct CameraInputBinding_data
{
	// Player/device identification
	short playerId = -1;                     // Player ID (-1 = keyboard, >= 0 = joystick)
	SDL_JoystickID joystickId = 0;           // Joystick device ID
	bool useKeyboard = false;                // Use keyboard controls

	// Keyboard bindings (using numpad scancodes - hardware-based, layout-independent)
	SDL_Scancode key_up = SDL_SCANCODE_KP_8;          // Move camera up
	SDL_Scancode key_down = SDL_SCANCODE_KP_2;        // Move camera down
	SDL_Scancode key_left = SDL_SCANCODE_KP_4;        // Move camera left
	SDL_Scancode key_right = SDL_SCANCODE_KP_6;       // Move camera right
	SDL_Scancode key_reset = SDL_SCANCODE_KP_5;       // Reset camera controls
	SDL_Scancode key_rotate_left = SDL_SCANCODE_KP_DIVIDE;   // Rotate camera left
	SDL_Scancode key_rotate_right = SDL_SCANCODE_KP_MULTIPLY; // Rotate camera right
	SDL_Scancode key_zoom_in = SDL_SCANCODE_KP_PLUS;  // Zoom in
	SDL_Scancode key_zoom_out = SDL_SCANCODE_KP_MINUS; // Zoom out

	// Joystick bindings
	int axis_horizontal = 2;                 // Right stick horizontal axis
	int axis_vertical = 3;                   // Right stick vertical axis
	int trigger_left = 4;                    // Left trigger for rotation
	int trigger_right = 5;                   // Right trigger for rotation
	int button_reset = 10;                   // Button to reset camera

	// Input thresholds
	float deadzone = 0.15f;                  // Deadzone for analog sticks
	float triggerThreshold = 0.3f;           // Threshold for trigger activation

	// Current input state (updated each frame)
	Vector inputDirection = { 0.f, 0.f, 0.f }; // Normalized input direction
	float rotationInput = 0.0f;              // Rotation input value (-1 to 1)
	float zoomInput = 0.0f;                  // Zoom input value (-1 to 1)
	bool resetRequested = false;             // Reset button pressed this frame
	
	// Constructors
	CameraInputBinding_data() = default;
	CameraInputBinding_data(const CameraInputBinding_data&) = default;
	CameraInputBinding_data& operator=(const CameraInputBinding_data&) = default;
};
// --- Collision Zone Component ---
// Represents a static collision area (e.g., from Tiled object layer)
struct CollisionZone_data
{
	SDL_FRect bounds = { 0.f, 0.f, 100.f, 100.f };  // Collision rectangle
	bool isStatic = true;                            // Is this a static collision zone
	
	// Constructors
	CollisionZone_data() = default;
	CollisionZone_data(SDL_FRect rect, bool isStatic_) : bounds(rect), isStatic(isStatic_) {}
	CollisionZone_data(const CollisionZone_data&) = default;
	CollisionZone_data& operator=(const CollisionZone_data&) = default;
};

// Editor context for plugins
struct EditorContext_st
{
	bool isDirty = false;
	float deltaTime = 0.0f;
	
	// Constructors
	EditorContext_st() = default;
	EditorContext_st(const EditorContext_st&) = default;
	EditorContext_st& operator=(const EditorContext_st&) = default;
};