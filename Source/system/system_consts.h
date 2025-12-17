#pragma once

constexpr double k_PI = 3.14159265358979323846;

// Configuration: defaults
static const int DEFAULT_WINDOW_WIDTH = 800;
static const int DEFAULT_WINDOW_HEIGHT = 600;

// Event structure types
// Used to identify the source/type of event messages
enum class EventStructType
{
	EventStructType_Default = -1,
	EventStructType_System_Windows = 0,
	EventStructType_SDL,
	EventStructType_Olympe,
	EventStructType_All
};

// Olympe engine message identifiers
enum class EventType
{
    Olympe_EventType_Default = -1,
    // -------- GAME OBJECTS MESSAGES ----------
	Olympe_EventType_Object_Activate = 0, // object activation 
    Olympe_EventType_Object_Deactivate,
    Olympe_EventType_Object_CollideEvent,
    Olympe_EventType_Object_UncollideEvent,
    Olympe_EventType_Object_CollideNav,
    Olympe_EventType_Object_UnCollideNav,
    Olympe_EventType_Object_CollideDeathZone, // collision with death zone

	//-------- MENU SYSTEM EVENTS ----------
	Olympe_EventType_Menu_Enter,
	Olympe_EventType_Menu_Exit,
	Olympe_EventType_Menu_Validate,

    EventType_Hit,

    // -------- OBBJECT EVENTS ----------
    Olympe_EventType_Object_Create, 
	Olympe_EventType_Object_Destroy,
	Olympe_EventType_Property_Add,
	Olympe_EventType_Property_Remove,

	// -------- LEVEL EVENTS ----------
	Olympe_EventType_Level_Load,
	Olympe_EventType_Level_Unload,

	// -------- SECTOR EVENTS ----------    
    Olympe_EventType_SectorToActivate,        // Sector to activate
    Olympe_EventType_SectorToDeactivate,      // Sector to deactivate

	// -------- INPUTS EVENTS ----------
	Olympe_EventType_Joystick_AxisMotion,
	Olympe_EventType_Joystick_ButtonDown,
	Olympe_EventType_Joystick_ButtonUp,
	Olympe_EventType_Joystick_Connected,
	Olympe_EventType_Joystick_Disconnected,

	Olympe_EventType_Keyboard_KeyDown,
	Olympe_EventType_Keyboard_KeyUp,
	Olympe_EventType_Keyboard_Connected,
	Olympe_EventType_Keyboard_Disconnected,


	Olympe_EventType_Mouse_ButtonDown,
	Olympe_EventType_Mouse_ButtonUp,
	Olympe_EventType_Mouse_Motion,
	Olympe_EventType_Mouse_Wheel,
	Olympe_EventType_Mouse_Connected,
	Olympe_EventType_Mouse_Disconnected,

	// -------- CAMERA EVENT ----------
	Olympe_EventType_Camera_Shake, // shake camera with given parameters intensity, duration (milliseconds)
	Olympe_EventType_Camera_Shake_Stop, // stop camera shake immediately
	Olympe_EventType_Camera_Teleport, // instantly move camera to position of the renderer scene
	Olympe_EventType_Camera_MoveToPosition, // smoothly move camera to position over time using blending (velocity)
	Olympe_EventType_Camera_ZoomTo, // smoothly zoom camera to level over time using blending (velocity)
	Olympe_EventType_Camera_RotateTo, // smoothly rotate camera to angle over time using blending (velocity)
	Olympe_EventType_Camera_Reset, // reset camera to default position/zoom
	Olympe_EventType_Camera_SetBounds, // set camera movement bounds
	Olympe_EventType_Camera_ClearBounds, // clear camera movement bounds
	Olympe_EventType_Camera_Mode_2D, // set camera to free 2D mode
	Olympe_EventType_Camera_Mode_2_5D, // set camera to 2.5D mode (follow target on X axis only)
	Olympe_EventType_Camera_Mode_Isometric, // set camera to isometric mode
	Olympe_EventType_Camera_Target_Follow, // follow an object (given by UID or object pointer or object name)
	Olympe_EventType_Camera_Target_Unfollow, // stop following object, return to free mode

	// -------- GAME EVENTS ----------
	Olympe_EventType_Game_Pause, // pause the game from GameMenu
	Olympe_EventType_Game_Resume, // resume the game from GameMenu
	Olympe_EventType_Game_Quit, // quit the game from GameMenu
	Olympe_EventType_Game_Restart, // restart the current level from GameMenu
	Olympe_EventType_Game_AddPlayer, // add a new player (up to 4) param: player id (0..3)
	Olympe_EventType_Game_RemovePlayer, // remove a player param: player id (0..3)
	Olympe_EventType_Game_TakeScreenshot, // take a screenshot of the current frame
	Olympe_EventType_Game_SaveState, // save game state to slot (param: slot id)
	Olympe_EventType_Game_LoadState, // load game state from slot (param: slot id)

	// -------- SYSTEM EVENTS ----------
	Olympe_EventType_System_Any, // Any system event registration
	
    Olympe_EventType_Any,
    Olympe_EventType_MAX
};
