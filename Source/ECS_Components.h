/**
 * @file ECS_Components.h
 * @brief Core ECS component definitions
 * @author Nicolas Chereau
 * @date 2025
 * 
 * This file contains all component data structures used in the
 * Entity Component System architecture.
 * 
 * Components purpose: Include all component definitions used in the ECS architecture.
 */

#pragma once

#include "Ecs_Entity.h"
#include <string>
#include <unordered_map>
#include "vector.h"
#include <SDL3/SDL.h>
#include "DataManager.h"
#include "SDL_rect.h"

// ========================================================================
// Entity Type Enumeration
// ========================================================================

/**
 * @brief Entity type classification
 * 
 * Used to categorize entities for gameplay and rendering purposes.
 */
enum class EntityType : int
{
    None = 0,
    Player,
    NPC,
    Enemy,
    Item,
    Collectible,
    Effect,
    Particle,
    UIElement,
    Background,
    Trigger,
    Waypoint,
    Static,
    Dynamic
};

// ========================================================================
// Render Layers (Z-Order) - Used for depth sorting
// ========================================================================

/**
 * @brief Render layer enumeration for Z-ordering
 * 
 * Defines rendering order with lower values rendered first (background)
 * and higher values rendered last (foreground).
 */
enum class RenderLayer : int
{
    Background_Far = -2,      // -2 * 10000 = -20000 (distant parallax backgrounds)
    Background_Near = -1,     // -1 * 10000 = -10000 (near backgrounds)
    Ground = 0,               //  0 * 10000 = 0      (floor tiles, terrain)
    Objects = 1,              //  1 * 10000 = 10000  (items, decorations, collectibles)
    Characters = 2,           //  2 * 10000 = 20000  (NPCs, players)
    Flying = 3,               //  3 * 10000 = 30000  (flying enemies, projectiles)
    Effects = 4,              //  4 * 10000 = 40000  (particles, VFX)
    UI_Near = 5,              //  5 * 10000 = 50000  (UI elements, HUD)
    Foreground_Near = 10,     // 10 * 10000 = 100000 (close foreground elements)
    Foreground_Far = 20       // 20 * 10000 = 200000 (very close overlay)
};

/// Convert layer enum to z-coordinate value
inline float LayerToZ(RenderLayer layer)
{
    return static_cast<float>(static_cast<int>(layer));
}

/// Convert z-coordinate to layer enum (rounds to nearest integer)
/// Note: z-coordinates should exactly match RenderLayer integer values for proper layer mapping
inline RenderLayer ZToLayer(float z)
{
    return static_cast<RenderLayer>(static_cast<int>(z + (z >= 0 ? 0.5f : -0.5f)));
}

// Component type definitions

/**
 * @brief Identity component for entity identification
 * 
 * Stores basic entity identification information including name, tag,
 * and type classification.
 */
struct Identity_data
{
	/** @brief Entity name identifier */
	std::string name = "Entity";
	
	/** @brief Entity tag/category for grouping */
	std::string tag = "Untagged";
	
	/** @brief Entity type string (for backward compatibility) */
	std::string type = "UnknownType";
	
	/** @brief Entity type enum (for layer management) */
	EntityType entityType = EntityType::None;
	
	/** @brief Should the entity persist across levels? */
	bool isPersistent = false;
	
	/** @brief Default constructor */
	Identity_data() = default;
	
	/**
	 * @brief Construct with name, tag, and type
	 * @param n Entity name
	 * @param t Entity tag
	 * @param et Entity type string
	 */
	Identity_data(std::string n, std::string t, std::string et)
		: name(std::move(n)), tag(std::move(t)), type(std::move(et)), entityType(EntityType::None) {}
	
	/** @brief Copy constructor */
	Identity_data(const Identity_data&) = default;
	
	/** @brief Copy assignment operator */
	Identity_data& operator=(const Identity_data&) = default;
};

/**
 * @brief Position component for spatial location
 * 
 * Stores entity position in world space.
 */
struct Position_data
{
	/** @brief 2D/3D position vector */
	Vector position;
	
	/** @brief Default constructor */
	Position_data() = default;
	
	/**
	 * @brief Construct with position
	 * @param pos Initial position vector
	 */
	Position_data(Vector pos) : position(pos) {}
	
	/** @brief Copy constructor */
	Position_data(const Position_data&) = default;
	
	/** @brief Copy assignment operator */
	Position_data& operator=(const Position_data&) = default;
};

/**
 * @brief Bounding box component for collision detection
 * 
 * Defines rectangular collision area for the entity.
 */
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
	float friction = 0.1f; // Friction coefficient
	bool useGravity = true; // Is gravity applied?
	bool rotation = false; // Is rotation allowed?
	
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
	bool visible = true;    // Is the entity visible
	
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
	bool isWalking = false; // Is the player walking?
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
	bool enabled = false;  // Disabled by default - toggle with TAB key
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
	
	// **NEW: Multi-layer overlay support**
	bool showCollisionOverlay = false;  // Hidden by default - toggle with C key
	bool showNavigationOverlay = false;  // Hidden by default - toggle with N key
	
	// **NEW: Layer selection for visualization**
	uint8_t activeCollisionLayer = 0;  // Which collision layer to display (0-7)
	uint8_t activeNavigationLayer = 0; // Which navigation layer to display (0-7)
	
	// **Updated: Overlay colors per layer (increased alpha for better visibility)**
	SDL_Color collisionColors[8] = {
		{ 150,  50, 200, 150 },  // Ground: purple
		{  50, 150, 255, 150 },  // Sky: cyan
		{ 100,  50,  50, 150 },  // Underground: dark red
		{ 255, 200,  50, 150 },  // Volume: orange
		{ 200, 200, 200, 150 },  // Custom1-4: gray variants
		{ 180, 180, 180, 150 },
		{ 160, 160, 160, 150 },
		{ 140, 140, 140, 150 }
	};
	
	SDL_Color navigationColors[8] = {
		{  50, 200, 100, 150 },  // Ground: green
		{ 100, 200, 255, 150 },  // Sky: light blue
		{ 200, 100,  50, 150 },  // Underground: brown
		{ 255, 255, 100, 150 },  // Volume: yellow
		{ 150, 255, 150, 150 },  // Custom1-4: green variants
		{ 120, 235, 120, 150 },
		{  90, 215,  90, 150 },
		{  60, 195,  60, 150 }
	};
	
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
	
	// Discrete zoom levels
	static constexpr float ZOOM_LEVELS[] = { 0.0125f, 0.025f, 0.05f, 0.1f, 0.15f, 0.25f, 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f };
	static constexpr size_t ZOOM_LEVEL_COUNT = sizeof(ZOOM_LEVELS) / sizeof(ZOOM_LEVELS[0]);
	int currentZoomLevelIndex = 3;           // Index 3 = 1.0 (default)
	
	// Ensure ZOOM_LEVELS array has at least 4 entries (for default index 3)
	static_assert(sizeof(ZOOM_LEVELS) / sizeof(ZOOM_LEVELS[0]) >= 4, "ZOOM_LEVELS must contain at least 4 entries for default index 3");

	// Rotation management (in degrees)        { "name": "speed", "type": "Float", "defaultValue": 100.0 },

	float rotation = 0.0f;                   // Current rotation angle
	float targetRotation = 0.0f;             // Target rotation angle for smooth transitions
	float rotationSpeed = 5.0f;              // Speed of rotation interpolation
	
	// Discrete rotation levels (15° increments)
	static constexpr float ROTATION_STEP = 15.0f;
	static constexpr int ROTATION_LEVELS = 24;  // 360° / 15° = 24 levels
	int currentRotationLevel = 0;  // Index in [0, 23], where 0 = 0°
	
	// Helper method to get rotation angle from level
	static constexpr float GetRotationFromLevel(int level)
	{
		return (level * ROTATION_STEP);  // level 0 = 0°, level 1 = 15°, etc.
	}

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
	
	// Diagonal movement keys
	SDL_Scancode key_up_left = SDL_SCANCODE_KP_7;     // Diagonal up-left
	SDL_Scancode key_up_right = SDL_SCANCODE_KP_9;    // Diagonal up-right
	SDL_Scancode key_down_left = SDL_SCANCODE_KP_1;   // Diagonal down-left
	SDL_Scancode key_down_right = SDL_SCANCODE_KP_3;  // Diagonal down-right
	
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
	
	// Previous trigger state for edge detection (discrete rotation)
	bool prevLeftTriggerPressed = false;     // Was left trigger pressed last frame
	bool prevRightTriggerPressed = false;    // Was right trigger pressed last frame
	
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

// --- Navigation Agent Component ---
// Navigation agent component (lightweight, entity-specific data only)
struct NavigationAgent_data
{
	// Agent properties
	float agentRadius = 16.0f;         // Collision radius for pathfinding
	float maxSpeed = 100.0f;           // Max movement speed
	float arrivalThreshold = 5.0f;     // Distance to consider "arrived"
	
	// Layer mask (which collision layers this agent can traverse)
	uint8_t layerMask = 0x01;          // Bit 0 = Ground layer by default
	
	// Pathfinding state
	std::vector<Vector> currentPath;   // Cached path (world coordinates)
	int currentWaypointIndex = 0;      // Current target waypoint in path
	Vector targetPosition = Vector(0, 0, 0);
	bool hasPath = false;
	bool needsRepath = false;
	
	// Optional: steering behaviors
	float steeringWeight = 1.0f;
	bool avoidObstacles = true;
	
	NavigationAgent_data() = default;
	NavigationAgent_data(const NavigationAgent_data&) = default;
	NavigationAgent_data& operator=(const NavigationAgent_data&) = default;
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