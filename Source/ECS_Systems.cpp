/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#include "ECS_Systems.h"
#include "ECS_Components.h"
#include "ECS_Entity.h"
#include "World.h" 
#include "GameEngine.h" // For delta time (fDt)
#include "InputsManager.h"
#include "system/KeyboardManager.h"
#include "system/JoystickManager.h"
#include "system/ViewportManager.h"
#include "system/EventQueue.h"
#include "VideoGame.h"
#include "system/GameMenu.h"
#include "TiledLevelLoader/include/ParallaxLayerManager.h"
#include "Rendering/IsometricRenderer.h"
#include <iostream>
#include <bitset>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cfloat>
#include "drawing.h"
#include "RenderContext.h"

#undef min
#undef max


//-------------------------------------------------------------
InputSystem::InputSystem()
{
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
}
void InputSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Input processing logic here
}
//-------------------------------------------------------------
// InputEventConsumeSystem: Consumes Input domain events and updates ECS components
InputEventConsumeSystem::InputEventConsumeSystem()
{
    // No specific component signature required - operates on cached input entities
}

void InputEventConsumeSystem::Process()
{
    // Get all Input domain events from the EventQueue
    const EventQueue& queue = EventQueue::Get();
    
    // Static flags for debouncing
    static bool s_key_TabPressed = false;
    
    // Process each input event
    queue.ForEachDomainEvent(EventDomain::Input, [](const Message& msg) {
        
        // Handle TAB key for grid toggle (global, not player-specific)
        if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyDown)
        {
            auto sc = static_cast<SDL_Scancode>(msg.controlId);
            
            if (sc == SDL_SCANCODE_TAB && !s_key_TabPressed)
            {
                s_key_TabPressed = true;
                World::Get().ToggleGrid();
            }
        }
        else if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyUp)
        {
            auto sc = static_cast<SDL_Scancode>(msg.controlId);
            
            if (sc == SDL_SCANCODE_TAB)
            {
                s_key_TabPressed = false;
            }
        }
        
        // Handle joystick connect/disconnect for InputsManager auto-rebind logic
        if (msg.msg_type == EventType::Olympe_EventType_Joystick_Connected)
        {
            // Auto reconnect joystick to any player that was disconnected if any
            if (InputsManager::Get().GetDisconnectedPlayersCount() > 0)
            {
                // get 1st Disconnected player
                short disconnectedPlayerID = InputsManager::Get().GetFirstDisconnectedPlayerID();
                if (disconnectedPlayerID >= 0)
                {
                    SYSTEM_LOG << "InputEventConsumeSystem: try rebinding joystick ID=" << msg.deviceId << " to disconnected player " << disconnectedPlayerID << "\n";
                    if (InputsManager::Get().AutoBindControllerToPlayer(disconnectedPlayerID))
                    {
                        // we can remove the disconnected player now, since he is rebound
                        InputsManager::Get().RemoveDisconnectedPlayer(disconnectedPlayerID);
                        SYSTEM_LOG << "InputEventConsumeSystem: Joystick ID=" << msg.deviceId << " rebound to player " << disconnectedPlayerID << "\n";
                    }
                    else
                        SYSTEM_LOG << "InputEventConsumeSystem: Failed to rebind joystick ID=" << msg.deviceId << " to disconnected player " << disconnectedPlayerID << "\n";
                }
            }
        }
        
        // Use optimized input entity cache instead of iterating all entities
        const auto& inputEntities = InputsManager::Get().GetInputEntities();
        
        for (EntityID entity : inputEntities)
        {
            try
            {
                if (!World::Get().HasComponent<PlayerBinding_data>(entity)) continue;
                PlayerBinding_data &binding = World::Get().GetComponent<PlayerBinding_data>(entity);

                // Match device id: for keyboard, joystickID may be -1
                if (binding.controllerID != msg.deviceId) continue;

                // Ensure Controller_data exists
                if (!World::Get().HasComponent<Controller_data>(entity))
                {
                    continue;
                }
                Controller_data &ctrl = World::Get().GetComponent<Controller_data>(entity);
                ctrl.controllerID = static_cast<short>(msg.deviceId);

                // Update connection state
                if (msg.msg_type == EventType::Olympe_EventType_Joystick_Connected || 
                    msg.msg_type == EventType::Olympe_EventType_Keyboard_Connected)
                    ctrl.isConnected = true;
                if (msg.msg_type == EventType::Olympe_EventType_Joystick_Disconnected || 
                    msg.msg_type == EventType::Olympe_EventType_Keyboard_Disconnected)
                    ctrl.isConnected = false;

                // Button events
                if (msg.msg_type == EventType::Olympe_EventType_Joystick_ButtonDown || 
                    msg.msg_type == EventType::Olympe_EventType_Joystick_ButtonUp)
                {
                    int button = msg.controlId;
                    if (button >= 0 && button < Controller_data::MAX_BUTTONS) 
                        ctrl.buttons[button] = (msg.state != 0);
                }

                // Axis motion: update Controller_data axes
                if (msg.msg_type == EventType::Olympe_EventType_Joystick_AxisMotion)
                {
                    int axis = msg.controlId;
                    float value = msg.param1; // normalized [-1,1]
                    // Map axes to Controller_data structure
                    // axis 0 -> leftStick.x, axis 1 -> leftStick.y
                    // axis 2 -> rightStick.x, axis 3 -> rightStick.y
                    // axis 4 -> leftTrigger, axis 5 -> rightTrigger
                    if (axis == 0) ctrl.leftStick.x = value;
                    else if (axis == 1) ctrl.leftStick.y = value;
                    else if (axis == 2) ctrl.rightStick.x = value;
                    else if (axis == 3) ctrl.rightStick.y = value;
                    else if (axis == 4) ctrl.leftTrigger = (value + 1.0f) * 0.5f; // normalize -1..1 to 0..1
                    else if (axis == 5) ctrl.rightTrigger = (value + 1.0f) * 0.5f;
                }

                // Keyboard events are now handled by InputMappingSystem via Pull API
                // No longer need to process keyboard events here
            }
            catch (const std::exception&)
            {
                // ignore per-entity errors
            }
        }
    });
}
//-------------------------------------------------------------
// GameEventConsumeSystem: Consumes Gameplay domain events
GameEventConsumeSystem::GameEventConsumeSystem()
{
    // No specific component signature required - operates on global game state
}

void GameEventConsumeSystem::Process()
{
    // Get all Gameplay domain events from the EventQueue
    const EventQueue& queue = EventQueue::Get();
    
    // Forward declaration to access VideoGame
    extern class VideoGame;
    
    // Process each gameplay event
    queue.ForEachDomainEvent(EventDomain::Gameplay, [](const Message& msg) {
        
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Game_Pause:
                VideoGame::Get().Pause();
                SYSTEM_LOG << "GameEventConsumeSystem: Paused via event\n";
                break;
            case EventType::Olympe_EventType_Game_Resume:
                VideoGame::Get().Resume();
                SYSTEM_LOG << "GameEventConsumeSystem: Resumed via event\n";
                break;
            case EventType::Olympe_EventType_Game_Quit:
                VideoGame::Get().RequestQuit();
                SYSTEM_LOG << "GameEventConsumeSystem: Quit requested via event\n";
                break;
            case EventType::Olympe_EventType_Game_Restart:
                SYSTEM_LOG << "GameEventConsumeSystem: Restart requested via event (not implemented)\n";
                break;
            case EventType::Olympe_EventType_Game_TakeScreenshot:
                SYSTEM_LOG << "GameEventConsumeSystem: TakeScreenshot event (not implemented)\n";
                break;
            case EventType::Olympe_EventType_Game_SaveState:
            {
                int slot = msg.controlId;
                SYSTEM_LOG << "GameEventConsumeSystem: SaveState event slot=" << slot << "\n";
                VideoGame::Get().SaveGame(slot);
                break;
            }
            case EventType::Olympe_EventType_Game_LoadState:
            {
                int slot = msg.controlId;
                SYSTEM_LOG << "GameEventConsumeSystem: LoadState event slot=" << slot << "\n";
                VideoGame::Get().LoadGame(slot);
                break;
            }
            default:
                break;
        }
    });
    
    // Also process keyboard events for add/remove player (Input domain, but game-related)
    // Static state for debouncing
    static bool s_key_AddPlayerPressed = false;
    static bool s_key_RemovePlayerPressed = false;
    
    queue.ForEachDomainEvent(EventDomain::Input, [](const Message& msg) {
        
        if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyDown)
        {
            auto sc = static_cast<SDL_Scancode>(msg.controlId);
            if (sc == SDL_SCANCODE_RETURN)
            {
                // debounce: only act on initial press
                if (!s_key_AddPlayerPressed && msg.state == 1)
                {
                    s_key_AddPlayerPressed = true;
                    EntityID added = VideoGame::Get().AddPlayerEntity();
                    SYSTEM_LOG << "GameEventConsumeSystem: Enter pressed -> add player (returned " << added << ")\n";
                }
            }
            else if (sc == SDL_SCANCODE_BACKSPACE)
            {
                if (!s_key_RemovePlayerPressed && msg.state == 1)
                {
                    s_key_RemovePlayerPressed = true;
                    auto& players = VideoGame::Get().m_playersEntity;
                    if (!players.empty())
                    {
                        EntityID pid = players.back();
                        bool ok = VideoGame::Get().RemovePlayerEntity(pid);
                        SYSTEM_LOG << "GameEventConsumeSystem: Backspace pressed -> remove player " << pid << " -> " << (ok ? "removed" : "failed") << "\n";
                    }
                }
            }
        }
        else if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyUp)
        {
            auto sc = static_cast<SDL_Scancode>(msg.controlId);
            if (sc == SDL_SCANCODE_RETURN) s_key_AddPlayerPressed = false;
            if (sc == SDL_SCANCODE_BACKSPACE) s_key_RemovePlayerPressed = false;
        }
    });
}
//-------------------------------------------------------------
// UIEventConsumeSystem: Consumes UI domain events
UIEventConsumeSystem::UIEventConsumeSystem()
{
    // No specific component signature required - operates on UI state
}

void UIEventConsumeSystem::Process()
{
    // Get all UI domain events from the EventQueue
    const EventQueue& queue = EventQueue::Get();
    
    // Forward declaration to access GameMenu
    extern class GameMenu;
    
    // Process keyboard events from Input domain for menu toggle (ESC key)
    queue.ForEachDomainEvent(EventDomain::Input, [](const Message& msg) {
        // Toggle menu with ESC key
        if (msg.msg_type == EventType::Olympe_EventType_Keyboard_KeyDown)
        {
            auto sc = static_cast<SDL_Scancode>(msg.controlId);
            
            if (sc == SDL_SCANCODE_ESCAPE)
            {
                if (GameMenu::Get().IsActive())
                {
                    GameMenu::Get().Deactivate();  // Close menu, resume game
                    SYSTEM_LOG << "UIEventConsumeSystem: ESC pressed - menu deactivated\n";
                }
                else
                {
                    GameMenu::Get().Activate();    // Open menu, pause game
                    SYSTEM_LOG << "UIEventConsumeSystem: ESC pressed - menu activated\n";
                }
            }
            
            // Handle menu navigation (if menu is active)
            if (GameMenu::Get().IsActive())
            {
                switch (sc)
                {
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_W:
                        GameMenu::Get().SelectPrevious();
                        SYSTEM_LOG << "UIEventConsumeSystem: Menu selection moved up\n";
                        break;
                    case SDL_SCANCODE_DOWN:
                    case SDL_SCANCODE_S:
                        GameMenu::Get().SelectNext();
                        SYSTEM_LOG << "UIEventConsumeSystem: Menu selection moved down\n";
                        break;
                    case SDL_SCANCODE_RETURN:
                    case SDL_SCANCODE_SPACE:
                        GameMenu::Get().ValidateSelection();
                        SYSTEM_LOG << "UIEventConsumeSystem: Menu selection validated\n";
                        break;
                    default:
                        break;
                }
            }
        }
    });
    
    // Process each UI event
    queue.ForEachDomainEvent(EventDomain::UI, [](const Message& msg) {
        
        switch (msg.msg_type)
        {
            case EventType::Olympe_EventType_Menu_Enter:
                GameMenu::Get().Activate();
                SYSTEM_LOG << "UIEventConsumeSystem: Menu Enter event - activated\n";
                break;
            case EventType::Olympe_EventType_Menu_Exit:
                GameMenu::Get().Deactivate();
                SYSTEM_LOG << "UIEventConsumeSystem: Menu Exit event - deactivated\n";
                break;
            case EventType::Olympe_EventType_Menu_Validate:
                if (GameMenu::Get().IsActive())
                {
                    SYSTEM_LOG << "UIEventConsumeSystem: Menu Validate event\n";
                    GameMenu::Get().ValidateSelection();
                }
                break;
            default:
                break;
        }
    });
}
//-------------------------------------------------------------
// CameraEventConsumeSystem: Consumes Camera domain events
CameraEventConsumeSystem::CameraEventConsumeSystem()
{
    // No specific component signature required - operates on camera entities
}

void CameraEventConsumeSystem::Process()
{
    // Get all Camera domain events from the EventQueue
    const EventQueue& queue = EventQueue::Get();
    
    // Get the CameraSystem instance
    CameraSystem* camSys = World::Get().GetSystem<CameraSystem>();
    if (!camSys) return;
    
    // Process each camera event by forwarding to CameraSystem::OnEvent
    // This is a transitional approach - eventually CameraSystem should consume events directly
    queue.ForEachDomainEvent(EventDomain::Camera, [camSys](const Message& msg) {
        camSys->OnEvent(msg);
    });
}
//-------------------------------------------------------------
AISystem::AISystem()
{
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<AIBehavior_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<Movement_data>(), true);
}
void AISystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // AI processing logic here
}
//-------------------------------------------------------------
DetectionSystem::DetectionSystem()
{
}
void DetectionSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Detection processing logic here
}
//-------------------------------------------------------------
PhysicsSystem::PhysicsSystem()
{
}
void PhysicsSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Physics processing logic here
}
//-------------------------------------------------------------
CollisionSystem::CollisionSystem()
{
}
void CollisionSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Collision processing logic here
}
//-------------------------------------------------------------
TriggerSystem::TriggerSystem()
{
}
void TriggerSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Trigger processing logic here
}
//-------------------------------------------------------------
MovementSystem::MovementSystem()
{
    // Define the required components: Position AND AI_Player
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<Movement_data>(), true);
}

void MovementSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Iterate ONLY over the relevant entities stored in m_entities
    for (EntityID entity : m_entities)
    {
        try
        {
            // Direct and fast access to Component data from the Pools
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Movement_data& move = World::Get().GetComponent<Movement_data>(entity);
            // Game logic: simple movement based on speed and delta time
            pos.position += move.velocity * GameEngine::fDt;
        }
        catch (const std::exception& e)
        {
            std::cerr << "MovementSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}
//-------------------------------------------------------------
RenderingSystem::RenderingSystem()
{
	// Define the required components: Identity, Position, VisualSprite and BoundingBox
	requiredSignature.set(GetComponentTypeID_Static<Identity_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<VisualSprite_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<BoundingBox_data>(), true);
}
void RenderingSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    // Get actual players from ViewportManager
    const auto& players = ViewportManager::Get().GetPlayers();
    
    // Check if we have any ECS cameras active
    // For multi-player: check player cameras
    // For no-players: check default camera (playerId=-1)
    bool hasECSCameras = false;
    if (!players.empty())
    {
        // Multi-player case: check if any player has an active camera
        for (short playerID : players)
        {
            CameraTransform camTransform = GetActiveCameraTransform(playerID);
            if (camTransform.isActive)
            {
                hasECSCameras = true;
                break;
            }
        }
    }
    else
    {
        // No-players case: check if default camera exists
        CameraTransform defaultCam = GetActiveCameraTransform(-1);
        hasECSCameras = defaultCam.isActive;
    }
    
    // Use ECS camera system if available, otherwise fall back to legacy
    if (hasECSCameras)
    {
        if (!players.empty())
        {
            // Multi-camera rendering with ECS camera system
            for (short playerID : players)
            {
                CameraTransform camTransform = GetActiveCameraTransform(playerID);
                
                if (!camTransform.isActive)
                    continue;
                
                // Set active camera for drawing functions
                RenderContext::Get().SetActiveCamera(camTransform);
                
                // Set viewport and clip rect
                SDL_Rect viewportRect = {
                    (int)camTransform.viewport.x,
                    (int)camTransform.viewport.y,
                    (int)camTransform.viewport.w,
                    (int)camTransform.viewport.h
                };

                SDL_SetRenderViewport(renderer, &viewportRect);
                //SDL_SetRenderClipRect(renderer, &viewportRect);
                
                GridSystem* grid = World::Get().GetSystem<GridSystem>();
                if (grid)
                    grid->RenderForCamera(camTransform);

                // Render with parallax layers
                RenderMultiLayerForCamera(camTransform);
                
                // Clear active camera after rendering this player
                RenderContext::Get().ClearActiveCamera();
                
                // Reset viewport-specific state
                //SDL_SetRenderClipRect(renderer, nullptr);

            }
        }
        else
        {
            // Single-view rendering with default camera (playerId=-1)
            CameraTransform camTransform = GetActiveCameraTransform(-1);
            if (camTransform.isActive)
            {
                // Set active camera for drawing functions
                RenderContext::Get().SetActiveCamera(camTransform);
                
                // Set viewport and clip rect
                SDL_Rect viewportRect = {
                    (int)camTransform.viewport.x,
                    (int)camTransform.viewport.y,
                    (int)camTransform.viewport.w,
                    (int)camTransform.viewport.h
                };

                SDL_SetRenderViewport(renderer, &viewportRect);
                //SDL_SetRenderClipRect(renderer, &viewportRect);
                
                // Render with parallax layers
                RenderMultiLayerForCamera(camTransform);
                
                // Clear active camera after rendering
                RenderContext::Get().ClearActiveCamera();
                
                // Reset viewport-specific state
                //SDL_SetRenderClipRect(renderer, nullptr);
            }
        }
        
        // Final reset
        //SDL_SetRenderClipRect(renderer, nullptr);
        SDL_SetRenderViewport(renderer, nullptr);
    }
}

// ========================================================================
// Tile Rendering Helper Functions
// ========================================================================

// Rendering constants
namespace {
    // Depth calculation constants
    constexpr float DEPTH_LAYER_SCALE = 10000.0f;  // Scale for layer separation
    constexpr float DEPTH_DIAGONAL_SCALE = 100.0f; // Scale for isometric diagonal (X+Y)
    constexpr float DEPTH_X_SCALE = 0.1f;          // Scale for X coordinate tie-breaking
    
    // Frustum culling padding
    constexpr int ISO_TILE_PADDING = 5;    // Padding for isometric tall tiles
    constexpr int ORTHO_TILE_PADDING = 2;  // Padding for orthogonal tiles
}

// -> Helper: Extract flip flags from GID
void ExtractFlipFlags(uint32_t gid, bool& flipH, bool& flipV, bool& flipD)
{
    constexpr uint32_t FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    constexpr uint32_t FLIPPED_VERTICALLY_FLAG = 0x40000000;
    constexpr uint32_t FLIPPED_DIAGONALLY_FLAG = 0x20000000;
    
    flipH = (gid & FLIPPED_HORIZONTALLY_FLAG) != 0;
    flipV = (gid & FLIPPED_VERTICALLY_FLAG) != 0;
    flipD = (gid & FLIPPED_DIAGONALLY_FLAG) != 0;
}

// -> Helper: Convert flip flags to SDL flip mode
// Note: SDL3 only supports horizontal and vertical flips
// Diagonal flip (flipD) is extracted but not applied - requires rotation
SDL_FlipMode GetSDLFlip(bool flipH, bool flipV, bool /*flipD*/)
{
    int flip = SDL_FLIP_NONE;
    if (flipH) flip |= SDL_FLIP_HORIZONTAL;
    if (flipV) flip |= SDL_FLIP_VERTICAL;
    return static_cast<SDL_FlipMode>(flip);
}

// -> NEW: Calculate visible tile range with frustum culling
void GetVisibleTileRange(const CameraTransform& cam,
                        const std::string& orientation,
                        int tileWidth, int tileHeight,
                        int& minX, int& minY, int& maxX, int& maxY)
{
    if (orientation == "isometric") {
        // Convert screen corners to world coordinates using CameraTransform::ScreenToWorld()
        Vector topLeftWorld = cam.ScreenToWorld(Vector(0, 0, 0));
        Vector topRightWorld = cam.ScreenToWorld(Vector(cam.viewport.w, 0, 0));
        Vector bottomLeftWorld = cam.ScreenToWorld(Vector(0, cam.viewport.h, 0));
        Vector bottomRightWorld = cam.ScreenToWorld(Vector(cam.viewport.w, cam.viewport.h, 0));
        
        // Convert world coordinates to isometric tile coordinates
        // Inverse of: isoX = (worldX - worldY) * (tileWidth / 2.0f)
        //             isoY = (worldX + worldY) * (tileHeight / 2.0f)
        // Solution:   worldX = (isoX / (tileWidth/2) + isoY / (tileHeight/2)) / 2
        //             worldY = (isoY / (tileHeight/2) - isoX / (tileWidth/2)) / 2
        float halfTileW = tileWidth / 2.0f;
        float halfTileH = tileHeight / 2.0f;
        
        auto worldToTile = [halfTileW, halfTileH](const Vector& world) {
            float tileX = (world.x / halfTileW + world.y / halfTileH) / 2.0f;
            float tileY = (world.y / halfTileH - world.x / halfTileW) / 2.0f;
            return Vector(tileX, tileY, 0);
        };
        
        Vector topLeft = worldToTile(topLeftWorld);
        Vector topRight = worldToTile(topRightWorld);
        Vector bottomLeft = worldToTile(bottomLeftWorld);
        Vector bottomRight = worldToTile(bottomRightWorld);
        
        // Find bounding box in tile coordinates
        float tileMinX = std::min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
        float tileMaxX = std::max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
        float tileMinY = std::min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});
        float tileMaxY = std::max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});
        
        // Bounding box with padding for tall tiles
        minX = static_cast<int>(std::floor(tileMinX)) - ISO_TILE_PADDING;
        minY = static_cast<int>(std::floor(tileMinY)) - ISO_TILE_PADDING;
        maxX = static_cast<int>(std::ceil(tileMaxX)) + ISO_TILE_PADDING;
        maxY = static_cast<int>(std::ceil(tileMaxY)) + ISO_TILE_PADDING;
    }
    else {
        // Orthogonal/hex: Use CameraTransform::ScreenToWorld() for consistency
        // When camera is rotated, all four corners need to be checked to find min/max
        Vector topLeftWorld = cam.ScreenToWorld(Vector(0, 0, 0));
        Vector topRightWorld = cam.ScreenToWorld(Vector(cam.viewport.w, 0, 0));
        Vector bottomLeftWorld = cam.ScreenToWorld(Vector(0, cam.viewport.h, 0));
        Vector bottomRightWorld = cam.ScreenToWorld(Vector(cam.viewport.w, cam.viewport.h, 0));
        
        // Find bounding box in world space (handles rotation correctly)
        float worldMinX = std::min({topLeftWorld.x, topRightWorld.x, bottomLeftWorld.x, bottomRightWorld.x});
        float worldMaxX = std::max({topLeftWorld.x, topRightWorld.x, bottomLeftWorld.x, bottomRightWorld.x});
        float worldMinY = std::min({topLeftWorld.y, topRightWorld.y, bottomLeftWorld.y, bottomRightWorld.y});
        float worldMaxY = std::max({topLeftWorld.y, topRightWorld.y, bottomLeftWorld.y, bottomRightWorld.y});
        
        minX = static_cast<int>(std::floor(worldMinX / tileWidth)) - ORTHO_TILE_PADDING;
        minY = static_cast<int>(std::floor(worldMinY / tileHeight)) - ORTHO_TILE_PADDING;
        maxX = static_cast<int>(std::ceil(worldMaxX / tileWidth)) + ORTHO_TILE_PADDING;
        maxY = static_cast<int>(std::ceil(worldMaxY / tileHeight)) + ORTHO_TILE_PADDING;
    }
}

// -> NEW: Calculate depth for a tile
float CalculateTileDepth(const std::string& orientation,
                        int worldX, int worldY,
                        int layerZOrder,
                        int tileWidth, int tileHeight)
{
    float baseDepth = static_cast<float>(layerZOrder) * DEPTH_LAYER_SCALE;
    
    if (orientation == "isometric") {
        // Isometric diagonal sort: X+Y then X
        int diagonalSum = worldX + worldY;
        return baseDepth + diagonalSum * DEPTH_DIAGONAL_SCALE + worldX * DEPTH_X_SCALE;
    }
    else {
        // Orthogonal/hex: Y position
        return baseDepth + static_cast<float>(worldY * tileHeight);
    }
}

// -> NEW: Calculate depth for an entity
float CalculateEntityDepth(const std::string& orientation,
                          const Vector& position,
                          int tileWidth, int tileHeight)
{
    float baseDepth = position.z * DEPTH_LAYER_SCALE;  // Layer zOrder
    
    if (orientation == "isometric") {
        // Use exact worldPosY for fine sorting
        // This matches the tile depth calculation for proper integration
        return baseDepth + position.y;
    }
    else {
        // Orthogonal: Y position
        return baseDepth + position.y;
    }
}

// -> NEW: Render individual tile immediately (unified for all orientations)
void RenderTileImmediate(SDL_Texture* texture, const SDL_Rect& srcRect,
                        int worldX, int worldY, uint32_t gid,
                        int tileoffsetX, int tileoffsetY,
                        const CameraTransform& cam,
                        const std::string& orientation,
                        int tileWidth, int tileHeight)
{
    if (!texture) return;
    
    // Extract flip flags
    bool flipH, flipV, flipD;
    ExtractFlipFlags(gid, flipH, flipV, flipD);
    SDL_FlipMode flip = GetSDLFlip(flipH, flipV, flipD);
    
    SDL_FRect srcFRect = {(float)srcRect.x, (float)srcRect.y, 
                         (float)srcRect.w, (float)srcRect.h};
    
    // Calculate world position for this tile (orientation-specific)
    Vector worldPos;
    
    if (orientation == "isometric") {
        // Convert tile coordinates to isometric world coordinates
        float isoX = (worldX - worldY) * (tileWidth / 2.0f);
        float isoY = (worldX + worldY) * (tileHeight / 2.0f);
        
        // Apply isometric origin offset to align tiles and entities in the same world space
        // This offset is computed from the map bounds (minTileX, minTileY) in World
        float originOffsetX = World::Get().GetIsometricOriginX();
        float originOffsetY = World::Get().GetIsometricOriginY();
        
        worldPos = Vector(isoX + originOffsetX, isoY + originOffsetY, 0.0f);
    }
    else if (orientation == "hexagonal") {
        // Hexagonal axial to world (pointy-top)
        float hexRadius = tileWidth / 2.0f;
        float worldXPos = hexRadius * (sqrtf(3.0f) * worldX + sqrtf(3.0f) / 2.0f * worldY);
        float worldYPos = hexRadius * (3.0f / 2.0f * worldY);
        worldPos = Vector(worldXPos, worldYPos, 0.0f);
    }
    else {
        // Orthogonal
        worldPos = Vector(worldX * tileWidth, worldY * tileHeight, 0.0f);
    }
    
    // Calculate screen position with zoom only (no rotation)
    // Rotation is applied separately via SDL_RenderTextureRotated with viewport-centered pivot
    float screenX = (worldPos.x - cam.worldPosition.x) * cam.zoom - cam.screenOffset.x + cam.viewport.w / 2.0f;
    float screenY = (worldPos.y - cam.worldPosition.y) * cam.zoom - cam.screenOffset.y + cam.viewport.h / 2.0f;
    
    // Tile offsets are in pixel/texture space, scale by zoom
    float offsetScreenX = tileoffsetX * cam.zoom;
    float offsetScreenY = tileoffsetY * cam.zoom;
    
    // Calculate destination rectangle
    SDL_FRect destRect;
    destRect.w = srcRect.w * cam.zoom;
    destRect.h = srcRect.h * cam.zoom;
    
    if (orientation == "isometric") {
        // Isometric: center tile horizontally, anchor at bottom
        destRect.x = screenX + offsetScreenX - destRect.w / 2.0f;
        destRect.y = screenY + offsetScreenY - destRect.h + (tileHeight * cam.zoom);
    }
    else {
        // Orthogonal/hex: top-left anchor
        // NOTE: Hexagonal tiles may require centered anchoring in the future
        // depending on the specific hex tileset and map configuration
        destRect.x = screenX + offsetScreenX;
        destRect.y = screenY + offsetScreenY;
    }
    
    // Rotate around viewport center (not tile center!)
    // Pivot point is the viewport center expressed in the tile's local coordinate system
    SDL_FPoint pivotInTileSpace = {
        cam.viewport.w / 2.0f - destRect.x,
        cam.viewport.h / 2.0f - destRect.y
    };
    
    SDL_RenderTextureRotated(GameEngine::renderer, texture, 
                            &srcFRect, &destRect, 
                            cam.rotation,        // Camera rotation angle
                            &pivotInTileSpace,   // Pivot = viewport center in tile space
                            flip);               // Flip flags
}

// -> UNIFIED RENDERING PIPELINE - Single-pass sorting with frustum culling
// Multi-layer rendering with parallax support
void RenderMultiLayerForCamera(const CameraTransform& cam)
{
    Olympe::Tiled::ParallaxLayerManager& parallaxMgr = Olympe::Tiled::ParallaxLayerManager::Get();
    const std::string& mapOrientation = World::Get().GetMapOrientation();
    int tileWidth = World::Get().GetTileWidth();
    int tileHeight = World::Get().GetTileHeight();
    
    // Unified RenderItem structure for all renderable elements
    struct RenderItem
    {
        enum Type { 
            ParallaxLayer,    // Image layers (backgrounds/foregrounds)
            IndividualTile,   // -> NEW: Individual tile with full data
            Entity            // Game objects
        } type;
        
        float depth;          // Unified sorting key (lower = background)
        
        // Type-specific data
        union {
            struct {
                int layerIndex;
            } parallax;
            
            struct {
                // -> Complete tile data for immediate rendering
                SDL_Texture* texture;
                SDL_Rect srcRect;
                int worldX, worldY;
                uint32_t gid;
                int tileoffsetX, tileoffsetY;
                int zOrder;
            } tile;
            
            struct {
                EntityID entityId;
            } entity;
        };
        
        // Factory methods
        static RenderItem MakeParallax(float depth, int layerIndex) {
            RenderItem item;
            item.type = ParallaxLayer;
            item.depth = depth;
            item.parallax.layerIndex = layerIndex;
            return item;
        }
        
        static RenderItem MakeTile(float depth, 
                                   SDL_Texture* tex, SDL_Rect src,
                                   int wx, int wy, uint32_t gid,
                                   int offX, int offY, int z) {
            RenderItem item;
            item.type = IndividualTile;
            item.depth = depth;
            item.tile.texture = tex;
            item.tile.srcRect = src;
            item.tile.worldX = wx;
            item.tile.worldY = wy;
            item.tile.gid = gid;
            item.tile.tileoffsetX = offX;
            item.tile.tileoffsetY = offY;
            item.tile.zOrder = z;
            return item;
        }
        
        static RenderItem MakeEntity(float depth, EntityID id) {
            RenderItem item;
            item.type = Entity;
            item.depth = depth;
            item.entity.entityId = id;
            return item;
        }
    };
    
    std::vector<RenderItem> renderBatch;
    renderBatch.reserve(1000);  // Reserve for typical visible items (parallax + ~200-400 tiles + entities)
    
    // ================================================================
    // PHASE 1: FRUSTUM CULLING + POPULATION
    // ================================================================
    
    // 1.1 Parallax Layers (always visible)
    const auto& parallaxLayers = parallaxMgr.GetLayers();
    for (size_t i = 0; i < parallaxLayers.size(); ++i) {
        const auto& layer = parallaxLayers[i];
        if (!layer.visible) continue;
        
        float depth;
        if (layer.scrollFactorX < 1.0f || layer.zOrder < 0) {
            // Background (distant)
            depth = -1000.0f + layer.zOrder;
        } else if (layer.scrollFactorX > 1.0f || layer.zOrder > 100) {
            // Foreground (close)
            depth = 10000.0f + layer.zOrder;
        } else {
            // Middle layers
            depth = static_cast<float>(layer.zOrder);
        }
        
        renderBatch.push_back(RenderItem::MakeParallax(depth, static_cast<int>(i)));
    }
    
    // 1.2 Tiles (with -> FRUSTUM CULLING)
    const auto& tileChunks = World::Get().GetTileChunks();
    auto& tilesetMgr = World::Get().GetTilesetManager();
    
    // -> Calculate visible tile range
    int minX, minY, maxX, maxY;
    GetVisibleTileRange(cam, mapOrientation, tileWidth, tileHeight, 
                        minX, minY, maxX, maxY);
    
    for (const auto& chunk : tileChunks) {
        for (int y = 0; y < chunk.height; ++y) {
            for (int x = 0; x < chunk.width; ++x) {
                int worldX = chunk.x + x;
                int worldY = chunk.y + y;
                
                // -> FRUSTUM CULLING
                if (worldX < minX || worldX > maxX || 
                    worldY < minY || worldY > maxY) {
                    continue;
                }
                
                int tileIndex = y * chunk.width + x;
                if (tileIndex >= chunk.tileGIDs.size()) continue;
                
                uint32_t gid = chunk.tileGIDs[tileIndex];
                if (gid == 0) continue;
                
                // Get texture
                SDL_Texture* texture = nullptr;
                SDL_Rect srcRect;
                const TilesetManager::TilesetInfo* tileset = nullptr;
                if (!tilesetMgr.GetTileTexture(gid, texture, srcRect, tileset)) {
                    continue;
                }
                
                // -> Calculate depth
                float depth = CalculateTileDepth(mapOrientation, 
                                                worldX, worldY, 
                                                chunk.zOrder,
                                                tileWidth, tileHeight);
                
                // -> Add to batch
                renderBatch.push_back(RenderItem::MakeTile(
                    depth, texture, srcRect, 
                    worldX, worldY, gid,
                    tileset ? tileset->tileoffsetX : 0,
                    tileset ? tileset->tileoffsetY : 0,
                    chunk.zOrder
                ));
            }
        }
    }
    
    // 1.3 Entities (with -> FRUSTUM CULLING)
    for (EntityID entity : World::Get().GetSystem<RenderingSystem>()->m_entities) {
        try {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
            BoundingBox_data& bbox = World::Get().GetComponent<BoundingBox_data>(entity);
            
            // -> NEW: Skip UI entities (rendered in Pass 2)
            if (World::Get().HasComponent<Identity_data>(entity))
            {
                Identity_data& id = World::Get().GetComponent<Identity_data>(entity);
                if (id.type == "UIElement")
                    continue;  // Skip UI, will be rendered in UIRenderingSystem
            }
            
            if (!visual.visible) continue;
            
            // -> FRUSTUM CULLING
            SDL_FRect worldBounds = {
                pos.position.x - visual.hotSpot.x,
                pos.position.y - visual.hotSpot.y,
                bbox.boundingBox.w,
                bbox.boundingBox.h
            };
            if (!cam.IsVisible(worldBounds)) continue;
            
            // -> Calculate depth
            float depth = CalculateEntityDepth(mapOrientation, 
                                              pos.position, 
                                              tileWidth, tileHeight);
            
            renderBatch.push_back(RenderItem::MakeEntity(depth, entity));
            
        } catch (...) {}
    }
    
    // ================================================================
    // PHASE 2: -> UNIFIED SORT (SINGLE PASS!)
    // ================================================================
    std::sort(renderBatch.begin(), renderBatch.end(),
        [](const RenderItem& a, const RenderItem& b) {
            return a.depth < b.depth;
        });
    
    // ================================================================
    // PHASE 3: BATCH RENDER
    // ================================================================
    for (const auto& item : renderBatch) {
        switch (item.type) {
            case RenderItem::ParallaxLayer:
                parallaxMgr.RenderLayer(parallaxLayers[item.parallax.layerIndex], cam);
                break;
                
            case RenderItem::IndividualTile:
                RenderTileImmediate(
                    item.tile.texture, item.tile.srcRect,
                    item.tile.worldX, item.tile.worldY, item.tile.gid,
                    item.tile.tileoffsetX, item.tile.tileoffsetY,
                    cam, mapOrientation, tileWidth, tileHeight
                );
                break;
                
            case RenderItem::Entity:
                RenderSingleEntity(cam, item.entity.entityId);
                break;
        }
    }
}

// Render a single entity
void RenderSingleEntity(const CameraTransform& cam, EntityID entity)
{
    try
    {
        Identity_data& id = World::Get().GetComponent<Identity_data>(entity);
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
        BoundingBox_data& boxComp = World::Get().GetComponent<BoundingBox_data>(entity);
        
        // Create world bounds for frustum culling
        SDL_FRect worldBounds = {
            pos.position.x - visual.hotSpot.x,
            pos.position.y - visual.hotSpot.y,
            boxComp.boundingBox.w,
            boxComp.boundingBox.h
        };
        
        // Frustum culling - skip if not visible
        if (!cam.IsVisible(worldBounds))
            return;
        
        // Transform position to screen space
        Vector centerScreen = cam.WorldToScreen(pos.position);
        
        // Transform size to screen space
        Vector screenSize = cam.WorldSizeToScreenSize(
            Vector(boxComp.boundingBox.w, boxComp.boundingBox.h, 0.f)
        );
        
        // Create destination rectangle
        SDL_FRect destRect = {
            centerScreen.x - visual.hotSpot.x * cam.zoom,
            centerScreen.y - visual.hotSpot.y * cam.zoom,
            screenSize.x,
            screenSize.y
        };
        
		SDL_FPoint fpoint = { visual.hotSpot.x * cam.zoom, visual.hotSpot.y * cam.zoom };

        // Render based on sprite type
        if (visual.sprite)
        {
            // Apply color modulation
            SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
            
            
            SDL_RenderTextureRotated(GameEngine::renderer, visual.sprite, nullptr, &destRect, cam.rotation, &fpoint, SDL_FLIP_NONE);
            
            // Debug: draw position & bounding box
            /*switch (id.type)
            {
                case EntityType::UIElement:
                case EntityType::Background:
                    SDL_SetRenderDrawColor(GameEngine::renderer, 0, 0, 255, 255); // blue
                    break;
                case EntityType::Player:
                    SDL_SetRenderDrawColor(GameEngine::renderer, 0, 255, 0, 255); // green
                    break;
                case EntityType::Ennemy:
                case EntityType::NPC:
                    SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 0, 255); // red
                    break;
                default:
                    SDL_SetRenderDrawColor(GameEngine::renderer, 255, 255, 0, 255); // yellow
                    break;
            }/**/
        }

        SDL_SetRenderDrawColor(GameEngine::renderer, 255, 255, 0, 255); // yellow

		destRect = { pos.position.x - visual.hotSpot.x, pos.position.y - visual.hotSpot.y, boxComp.boundingBox.w, boxComp.boundingBox.h };

        Draw_FilledCircle((int)(destRect.x + destRect.w / 2), (int)(destRect.y + destRect.h / 2), 3); // draw pivot/centre
        Draw_Rectangle(&destRect, SDL_Color{ 0, 255, 255, 255 }); // draw bounding box



    }
    catch (const std::exception& e)
    {
        std::cerr << "RenderSingleEntity Error for Entity " << entity << ": " << e.what() << "\n";
    }
}

// Render entities for a specific camera with frustum culling
void RenderEntitiesForCamera(const CameraTransform& cam)
{
    //SDL_Renderer* renderer = GameEngine::renderer;
    //if (!renderer) return;
    
    // Get all entities with Position, VisualSprite, and BoundingBox
    for (EntityID entity : World::Get().GetSystem<RenderingSystem>()->m_entities)
    {
        try
        {
			Identity_data& id = World::Get().GetComponent<Identity_data>(entity);
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
            BoundingBox_data& boxComp = World::Get().GetComponent<BoundingBox_data>(entity);
            
            // Create world bounds for frustum culling
            SDL_FRect worldBounds = {
                pos.position.x - visual.hotSpot.x,
                pos.position.y - visual.hotSpot.y,
                boxComp.boundingBox.w,
                boxComp.boundingBox.h
            };
            
            // Frustum culling - skip if not visible
            if (!cam.IsVisible(worldBounds))
                continue;
            
            // Transform position to screen space
            //Vector screenPos = cam.WorldToScreen(pos.position - visual.hotSpot);
            Vector centerScreen = cam.WorldToScreen(pos.position);
            
            // Transform size to screen space
            Vector screenSize = cam.WorldSizeToScreenSize(
                Vector(boxComp.boundingBox.w, boxComp.boundingBox.h, 0.f)
            );
            
            // Create destination rectangle
            SDL_FRect destRect = {
                //screenPos.x,
                //screenPos.y,
                centerScreen.x - visual.hotSpot.x * cam.zoom,
                centerScreen.y - visual.hotSpot.y * cam.zoom,
                screenSize.x,
                screenSize.y
            };
            
            // Render based on sprite type
            if (visual.sprite)
            {
                // Apply color modulation
                SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
                
                // Render sprite WITHOUT rotation - camera rotation is already applied via WorldToScreen
                // The "paper" (viewport) rotates, but sprites stay upright like stamps
                //SDL_RenderTexture(renderer, visual.sprite, nullptr, &destRect);
                SDL_FPoint fpoint = { visual.hotSpot.x * cam.zoom, visual.hotSpot.y * cam.zoom };
                SDL_RenderTextureRotated(GameEngine::renderer, visual.sprite, nullptr, &destRect, cam.rotation, &fpoint, SDL_FLIP_NONE);
                
                // Debug: draw position & bounding box
                /*switch (id.type)
                {
                    case EntityType::UIElement:
					case EntityType::Background:

                        SDL_SetRenderDrawColor(GameEngine::renderer, 0, 0, 255, 255); // blue
						break;
                    case EntityType::Player:
					    SDL_SetRenderDrawColor(GameEngine::renderer, 0, 255, 0, 255); // green
                        break;
                    case EntityType::Ennemy:
                    case EntityType::NPC:
                        SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 0, 255); // red
                        break;
                    default:
                        SDL_SetRenderDrawColor(GameEngine::renderer, 255, 255, 0, 255); // yellow
						break;

                }/**/
            }

			SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 255, 255); // magenta

            Draw_FilledCircle((int)(destRect.x + destRect.w / 2), (int)(destRect.y + destRect.h / 2), 3); // draw pivot/centre
            Draw_Rectangle(&destRect, SDL_Color{ 0, 255, 255, 255 }); // draw bounding box
        }
        catch (const std::exception& e)
        {
            std::cerr << "RenderEntitiesForCamera Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}
//-------------------------------------------------------------
PlayerControlSystem::PlayerControlSystem()
{
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<Controller_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<PhysicsBody_data>(), false); // optional

}
void PlayerControlSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Iterate ONLY over the relevant entities stored in m_entities
    for (EntityID entity : m_entities)
    {
        try
        {
            // Direct and fast access to Component data from the Pools
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
			PlayerController_data& controller = World::Get().GetComponent<PlayerController_data>(entity);
			PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
			Controller_data& ctrlData = World::Get().GetComponent<Controller_data>(entity);
			PhysicsBody_data& physBody = World::Get().GetComponent<PhysicsBody_data>(entity);

            // check if the controller is bound with right player id
			if (binding.controllerID != ctrlData.controllerID )
				continue; // skip if not bound

			// Game logic: simple movement based on joystick direction and delta time
            pos.position += controller.Joydirection * physBody.speed * GameEngine::fDt;
        }
        catch (const std::exception& e)
        {
            std::cerr << "MovementSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}
//-------------------------------------------------------------
InputMappingSystem::InputMappingSystem()
{
    // Required components: PlayerBinding_data + PlayerController_data + Controller_data
	requiredSignature.set(GetComponentTypeID_Static<PlayerBinding_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<PlayerController_data>(), true);
	requiredSignature.set(GetComponentTypeID_Static<Controller_data>(), true);
}

void InputMappingSystem::Process()
{
    // Early return if no entities
    if (m_entities.empty())
        return;

    // Check if we should process gameplay input
    InputContext activeContext = InputsManager::Get().GetActiveContext();
    if (activeContext != InputContext::Gameplay)
    {
        // Don't process gameplay input when in UI or Editor context
        return;
    }

    // Iterate over all entities with input components
    for (EntityID entity : m_entities)
    {
        try
        {
            PlayerBinding_data& binding = World::Get().GetComponent<PlayerBinding_data>(entity);
            PlayerController_data& pctrl = World::Get().GetComponent<PlayerController_data>(entity);
            Controller_data& ctrl = World::Get().GetComponent<Controller_data>(entity);

            // Reset direction each frame
            pctrl.Joydirection.x = 0.0f;
            pctrl.Joydirection.y = 0.0f;

            // Get or create InputMapping_data (optional component)
            InputMapping_data* mapping = nullptr;
            if (World::Get().HasComponent<InputMapping_data>(entity))
            {
                mapping = &World::Get().GetComponent<InputMapping_data>(entity);
            }

            // Keyboard input (controllerID == -1)
            if (binding.controllerID == -1)
            {
                // Use Pull API to read keyboard state
                KeyboardManager& km = KeyboardManager::Get();

                if (mapping)
                {
                    // Use custom bindings
                    if (km.IsKeyHeld(mapping->keyboardBindings["up"]) || km.IsKeyHeld(mapping->keyboardBindings["up_alt"]))
                        pctrl.Joydirection.y = -1.0f;
                    if (km.IsKeyHeld(mapping->keyboardBindings["down"]) || km.IsKeyHeld(mapping->keyboardBindings["down_alt"]))
                        pctrl.Joydirection.y = 1.0f;
                    if (km.IsKeyHeld(mapping->keyboardBindings["left"]) || km.IsKeyHeld(mapping->keyboardBindings["left_alt"]))
                        pctrl.Joydirection.x = -1.0f;
                    if (km.IsKeyHeld(mapping->keyboardBindings["right"]) || km.IsKeyHeld(mapping->keyboardBindings["right_alt"]))
                        pctrl.Joydirection.x = 1.0f;

                    // Action buttons
                    pctrl.isJumping = km.IsKeyHeld(mapping->keyboardBindings["jump"]);
                    pctrl.isShooting = km.IsKeyHeld(mapping->keyboardBindings["shoot"]);
                    pctrl.isInteracting = km.IsKeyPressed(mapping->keyboardBindings["interact"]);
                }
                else
                {
                    // Default WASD + Arrows bindings
                    if (km.IsKeyHeld(SDL_SCANCODE_W) || km.IsKeyHeld(SDL_SCANCODE_UP))
                        pctrl.Joydirection.y = -1.0f;
                    if (km.IsKeyHeld(SDL_SCANCODE_S) || km.IsKeyHeld(SDL_SCANCODE_DOWN))
                        pctrl.Joydirection.y = 1.0f;
                    if (km.IsKeyHeld(SDL_SCANCODE_A) || km.IsKeyHeld(SDL_SCANCODE_LEFT))
                        pctrl.Joydirection.x = -1.0f;
                    if (km.IsKeyHeld(SDL_SCANCODE_D) || km.IsKeyHeld(SDL_SCANCODE_RIGHT))
                        pctrl.Joydirection.x = 1.0f;

                    // Default action buttons
                    pctrl.isJumping = km.IsKeyHeld(SDL_SCANCODE_SPACE);
                    pctrl.isShooting = km.IsKeyHeld(SDL_SCANCODE_LCTRL);
                    pctrl.isInteracting = km.IsKeyPressed(SDL_SCANCODE_E);
                }
            }
            // Gamepad input
            else if (binding.controllerID >= 0)
            {
                SDL_JoystickID joyID = static_cast<SDL_JoystickID>(binding.controllerID);
                JoystickManager& jm = JoystickManager::Get();

                // Read left stick from Controller_data (already populated by event system)
                pctrl.Joydirection.x = ctrl.leftStick.x;
                pctrl.Joydirection.y = ctrl.leftStick.y;

                // Apply deadzone
                float deadzone = mapping ? mapping->deadzone : 0.15f;
                float magnitude = std::sqrt(pctrl.Joydirection.x * pctrl.Joydirection.x + 
                                           pctrl.Joydirection.y * pctrl.Joydirection.y);
                if (magnitude < deadzone)
                {
                    pctrl.Joydirection.x = 0.0f;
                    pctrl.Joydirection.y = 0.0f;
                }

                // Read action buttons
                if (mapping)
                {
                    pctrl.isJumping = jm.GetButton(joyID, mapping->gamepadBindings["jump"]);
                    pctrl.isShooting = jm.GetButton(joyID, mapping->gamepadBindings["shoot"]);
                    pctrl.isInteracting = jm.IsButtonPressed(joyID, mapping->gamepadBindings["interact"]);
                }
                else
                {
                    // Default gamepad buttons
                    pctrl.isJumping = jm.GetButton(joyID, 0);  // A button
                    pctrl.isShooting = jm.GetButton(joyID, 1); // B button
                    pctrl.isInteracting = jm.IsButtonPressed(joyID, 2); // X button
                }
            }

            // Normalize diagonal movement
            float magnitude = std::sqrt(pctrl.Joydirection.x * pctrl.Joydirection.x + 
                                       pctrl.Joydirection.y * pctrl.Joydirection.y);
            if (magnitude > 1.0f)
            {
                pctrl.Joydirection.x /= magnitude;
                pctrl.Joydirection.y /= magnitude;
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "InputMappingSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}
//-------------------------------------------------------------
GridSystem::GridSystem() {}

const GridSettings_data* GridSystem::FindSettings() const
{
    // Singleton simple: on prend la 1�re entit� qui a GridSettings_data
    for (const auto& kv : World::Get().m_entitySignatures)
    {
        EntityID e = kv.first;
        if (World::Get().HasComponent<GridSettings_data>(e))
            return &World::Get().GetComponent<GridSettings_data>(e);
    }
    return nullptr;
}

void GridSystem::DrawLineWorld(const CameraTransform& cam, const Vector& aWorld, const Vector& bWorld, const SDL_Color& c)
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    Vector a = cam.WorldToScreen(aWorld);
    Vector b = cam.WorldToScreen(bWorld);

    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderLine(renderer, a.x, a.y, b.x, b.y);
}

// Calculate world-space AABB visible in camera viewport
SDL_FRect GridSystem::GetWorldVisibleBounds(const CameraTransform& cam)
{
    // Get viewport corners in screen space (viewport-local coordinates)
    Vector topLeft     = cam.ScreenToWorld(Vector(0.f, 0.f, 0.f));
    Vector topRight    = cam.ScreenToWorld(Vector(cam.viewport.w, 0.f, 0.f));
    Vector bottomLeft  = cam.ScreenToWorld(Vector(0.f, cam.viewport.h, 0.f));
    Vector bottomRight = cam.ScreenToWorld(Vector(cam.viewport.w, cam.viewport.h, 0.f));

    // Compute AABB (min/max) in world space
    float minX = std::min({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
    float maxX = std::max({topLeft.x, topRight.x, bottomLeft.x, bottomRight.x});
    float minY = std::min({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});
    float maxY = std::max({topLeft.y, topRight.y, bottomLeft.y, bottomRight.y});

    return SDL_FRect{minX, minY, maxX - minX, maxY - minY};
}

void GridSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    const GridSettings_data* s = FindSettings();
    if (!s || !s->enabled) return;

    // Get actual players from ViewportManager
    const auto& players = ViewportManager::Get().GetPlayers();
    
    if (!players.empty())
    {
        // Multi-player case: render grid for each player's camera
        for (short playerID : players)
        {
            CameraTransform cam = GetActiveCameraTransform(playerID);
            if (!cam.isActive) continue;

            SDL_Rect viewportRect = {
                (int)cam.viewport.x,
                (int)cam.viewport.y,
                (int)cam.viewport.w,
                (int)cam.viewport.h
            };
            SDL_SetRenderViewport(renderer, &viewportRect);
            SDL_SetRenderClipRect(renderer, &viewportRect);

            switch (s->projection)
            {
            case GridProjection::Ortho:    RenderOrtho(cam, *s); break;
            case GridProjection::Iso:      RenderIso(cam, *s); break;
            case GridProjection::HexAxial: RenderHex(cam, *s); break;
            default: RenderOrtho(cam, *s); break;
            }

            // Reset viewport and clip rect after each viewport
            SDL_SetRenderClipRect(renderer, nullptr);
            SDL_SetRenderViewport(renderer, nullptr);
        }
    }
    else
    {
        // No-players case: render grid with default camera (playerId=-1)
        CameraTransform cam = GetActiveCameraTransform(-1);
        if (cam.isActive)
        {
            SDL_Rect viewportRect = {
                (int)cam.viewport.x,
                (int)cam.viewport.y,
                (int)cam.viewport.w,
                (int)cam.viewport.h
            };
            SDL_SetRenderViewport(renderer, &viewportRect);
            SDL_SetRenderClipRect(renderer, &viewportRect);

            switch (s->projection)
            {
            case GridProjection::Ortho:    RenderOrtho(cam, *s); break;
            case GridProjection::Iso:      RenderIso(cam, *s); break;
            case GridProjection::HexAxial: RenderHex(cam, *s); break;
            default: RenderOrtho(cam, *s); break;
            }

            // Reset viewport and clip rect
            SDL_SetRenderClipRect(renderer, nullptr);
            SDL_SetRenderViewport(renderer, nullptr);
        }
    }

    // Final reset
    SDL_SetRenderClipRect(renderer, nullptr);
    SDL_SetRenderViewport(renderer, nullptr);
}

void GridSystem::RenderForCamera(const CameraTransform& cam)
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    const GridSettings_data* s = FindSettings();
    if (!s || !s->enabled) return;

    // on n'utilise PLUS de boucle sur les players ici !
    switch (s->projection)
    {
        case GridProjection::Ortho:    RenderOrtho(cam, *s); break;
        case GridProjection::Iso:      RenderIso(cam, *s); break;
        case GridProjection::HexAxial: RenderHex(cam, *s); break;
        default: RenderOrtho(cam, *s); break;
    }
}

void GridSystem::RenderOrtho(const CameraTransform& cam, const GridSettings_data& s)
{
    const float csx = std::max(1.f, s.cellSize.x);
    const float csy = std::max(1.f, s.cellSize.y);

    // Get actual world bounds visible in viewport
    SDL_FRect bounds = GetWorldVisibleBounds(cam);
    float minX = bounds.x;
    float maxX = bounds.x + bounds.w;
    float minY = bounds.y;
    float maxY = bounds.y + bounds.h;

    // LOD: if zoomed out, skip lines to avoid visual clutter
    int skipFactor = 1;
    if (cam.zoom < s.lodZoomThreshold)
    {
        skipFactor = s.lodSkipFactor;
    }
    
    float stepX = csx * skipFactor;
    float stepY = csy * skipFactor;

    int lines = 0;

    // Vertical lines (constant X)
    float startX = std::floor(minX / stepX) * stepX;
    float endX   = std::ceil(maxX / stepX) * stepX;
    for (float x = startX; x <= endX && lines < s.maxLines; x += stepX)
    {
        DrawLineWorld(cam, Vector(x, minY, 0.f), Vector(x, maxY, 0.f), s.color);
        ++lines;
    }

    // Horizontal lines (constant Y)
    float startY = std::floor(minY / stepY) * stepY;
    float endY   = std::ceil(maxY / stepY) * stepY;
    for (float y = startY; y <= endY && lines < s.maxLines; y += stepY)
    {
        DrawLineWorld(cam, Vector(minX, y, 0.f), Vector(maxX, y, 0.f), s.color);
        ++lines;
    }
}

void GridSystem::RenderIso(const CameraTransform& cam, const GridSettings_data& s)
{
    const float w = std::max(1.f, s.cellSize.x);
    const float h = std::max(1.f, s.cellSize.y);

    // Iso basis vectors (diamond)
    // World position = i*u + j*v
    Vector u(w * 0.5f, -h * 0.5f, 0.f);  // Right-down diagonal
    Vector v(w * 0.5f,  h * 0.5f, 0.f);  // Right-up diagonal

    // Get world bounds from camera (like RenderOrtho)
    SDL_FRect bounds = GetWorldVisibleBounds(cam);
    
    // Convert world bounds to isometric grid coordinates (i, j)
    // Helper: convert world position to grid coordinates using matrix inversion
    auto worldToGrid = [&](float wx, float wy) -> std::pair<float, float>
    {
        // Solve linear system:
        //   wx = i * u.x + j * v.x
        //   wy = i * u.y + j * v.y
        // Matrix inversion (2x2):
        //   det = u.x * v.y - u.y * v.x
        float det = u.x * v.y - u.y * v.x;
        if (std::abs(det) < 0.0001f) return {0.f, 0.f};
        
        float i = (wx * v.y - wy * v.x) / det;
        float j = (wy * u.x - wx * u.y) / det;
        return {i, j};
    };
    
    // Sample bounds corners to find grid range
    float minI = FLT_MAX, maxI = -FLT_MAX;
    float minJ = FLT_MAX, maxJ = -FLT_MAX;
    
    std::vector<std::pair<float, float>> samples = {
        {bounds.x, bounds.y},                                      // Top-left
        {bounds.x + bounds.w, bounds.y},                          // Top-right
        {bounds.x, bounds.y + bounds.h},                          // Bottom-left
        {bounds.x + bounds.w, bounds.y + bounds.h},              // Bottom-right
        {bounds.x + bounds.w * 0.5f, bounds.y + bounds.h * 0.5f} // Center
    };
    
    for (const auto &sample : samples)
    {
        std::pair<float, float> gridCoords = worldToGrid(sample.first, sample.second);
        float i = gridCoords.first;
        float j = gridCoords.second;
        minI = std::min(minI, i);
        maxI = std::max(maxI, i);
        minJ = std::min(minJ, j);
        maxJ = std::max(maxJ, j);
    }
    
    // Add padding to ensure full coverage
    int iMin = (int)std::floor(minI) - 2;
    int iMax = (int)std::ceil(maxI) + 2;
    int jMin = (int)std::floor(minJ) - 2;
    int jMax = (int)std::ceil(maxJ) + 2;
    
    // LOD: skip lines when zoomed out
    int skipFactor = 1;
    if (cam.zoom < s.lodZoomThreshold)
    {
        skipFactor = s.lodSkipFactor;
    }
    
    int lines = 0;
    
    // Draw lines parallel to u (constant j)
    for (int j = jMin; j <= jMax && lines < s.maxLines; j += skipFactor)
    {
        Vector p0 = v * (float)j + u * (float)iMin;
        Vector p1 = v * (float)j + u * (float)iMax;
        DrawLineWorld(cam, p0, p1, s.color);
        ++lines;
    }

    // Draw lines parallel to v (constant i)
    for (int i = iMin; i <= iMax && lines < s.maxLines; i += skipFactor)
    {
        Vector p0 = u * (float)i + v * (float)jMin;
        Vector p1 = u * (float)i + v * (float)jMax;
        DrawLineWorld(cam, p0, p1, s.color);
        ++lines;
    }
}

void GridSystem::RenderHex(const CameraTransform& cam, const GridSettings_data& s)
{
    const float r = std::max(1.f, s.hexRadius);

    // Pointy-top axial layout
    const float dx = 1.5f * r;
    const float dy = std::sqrt(3.0f) * r; // sqrt(3) * r

    // Get world bounds
    SDL_FRect bounds = GetWorldVisibleBounds(cam);
    float minX = bounds.x;
    float maxX = bounds.x + bounds.w;
    float minY = bounds.y;
    float maxY = bounds.y + bounds.h;

    // Convert bounds to axial hex coords (q, r)
    int qMin = (int)std::floor(minX / dx) - 2;
    int qMax = (int)std::ceil(maxX / dx) + 2;
    int rMin = (int)std::floor(minY / dy) - 2;
    int rMax = (int)std::ceil(maxY / dy) + 2;

    // LOD: skip hexes when zoomed out
    int skipFactor = 1;
    if (cam.zoom < s.lodZoomThreshold)
    {
        skipFactor = s.lodSkipFactor;
    }

    auto hexCenter = [&](int q, int rr) -> Vector
    {
        float x = dx * (float)q;
        float y = dy * ((float)rr + 0.5f * (float)(q & 1));
        return Vector(x, y, 0.f);
    };

    auto drawHex = [&](const Vector& c)
    {
        Vector pts[6];
        for (int i = 0; i < 6; ++i)
        {
            float a = (60.f * (float)i + 30.f) * (float)k_PI / 180.f;
            pts[i] = Vector(c.x + std::cos(a) * r, c.y + std::sin(a) * r, 0.f);
        }
        for (int i = 0; i < 6; ++i)
            DrawLineWorld(cam, pts[i], pts[(i + 1) % 6], s.color);
    };

    int lines = 0;
    for (int q = qMin; q <= qMax && lines < s.maxLines; q += skipFactor)
    {
        for (int rr = rMin; rr <= rMax && lines < s.maxLines; rr += skipFactor)
        {
            Vector c = hexCenter(q, rr);

            // Simple AABB culling (hex center ± radius)
            if (c.x + r < minX || c.x - r > maxX || c.y + r < minY || c.y - r > maxY)
                continue;

            drawHex(c);
            lines += 6;
        }
    }
}
//-------------------------------------------------------------
// UIRenderingSystem: Pass 2 rendering for UI/HUD/Menu (always on top)
//-------------------------------------------------------------
UIRenderingSystem::UIRenderingSystem()
{
    // Require UI components
    requiredSignature.set(GetComponentTypeID_Static<Identity_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<VisualSprite_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<BoundingBox_data>(), true);
}

void UIRenderingSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    const auto& players = ViewportManager::Get().GetPlayers();
    
    if (!players.empty())
    {
        // Multi-player: render UI for each player's viewport
        for (short playerID : players)
        {
            CameraTransform cam = GetActiveCameraTransform(playerID);
            if (!cam.isActive) continue;
            
            // Set viewport
            SDL_Rect viewportRect = {
                (int)cam.viewport.x, (int)cam.viewport.y,
                (int)cam.viewport.w, (int)cam.viewport.h
            };
            SDL_SetRenderViewport(renderer, &viewportRect);
            SDL_SetRenderClipRect(renderer, &viewportRect);
            
            // Render UI layers
            RenderHUD(cam);
            RenderInGameMenu(cam);
            RenderDebugOverlay(cam);
        }
        
        // Reset viewport
        SDL_SetRenderClipRect(renderer, nullptr);
        SDL_SetRenderViewport(renderer, nullptr);
    }
    else
    {
        // Single camera fallback
        CameraTransform cam = GetActiveCameraTransform(-1);
        if (cam.isActive)
        {
            RenderHUD(cam);
            RenderInGameMenu(cam);
            RenderDebugOverlay(cam);
        }
    }
}

void UIRenderingSystem::RenderHUD(const CameraTransform& cam)
{
    // Render HUD entities (health bars, score, etc.)
    for (EntityID entity : m_entities)
    {
        if (!World::Get().HasComponent<Identity_data>(entity))
            continue;
        
        Identity_data& id = World::Get().GetComponent<Identity_data>(entity);
        
        // Only render UI elements
        if (id.type != "UIElement")
            continue;
        
        // Skip menu-specific elements
        if (id.tag == "MenuElement")
            continue;
        
        Position_data& pos = World::Get().GetComponent<Position_data>(entity);
        VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
        BoundingBox_data& bbox = World::Get().GetComponent<BoundingBox_data>(entity);
        
        if (!visual.visible)
            continue;
        
        // Render in screen space (no camera transform)
        SDL_FRect destRect = {
            pos.position.x,  // Already in screen coordinates
            pos.position.y,
            bbox.boundingBox.w,
            bbox.boundingBox.h
        };
        
        if (visual.sprite)
        {
            SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
            SDL_RenderTexture(GameEngine::renderer, visual.sprite, nullptr, &destRect);
        }
    }
}

void UIRenderingSystem::RenderInGameMenu(const CameraTransform& cam)
{
    // Only render if menu is active
    if (!GameMenu::Get().IsActive())
        return;
    
    SDL_Renderer* renderer = GameEngine::renderer;
    
    // Semi-transparent background overlay
    SDL_FRect overlay = {
        0, 0,
        (float)GameEngine::screenWidth,
        (float)GameEngine::screenHeight
    };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);  // Black, 50% alpha
    SDL_RenderFillRect(renderer, &overlay);
    
    // Menu panel (centered)
    float panelWidth = 400;
    float panelHeight = 300;
    float panelX = (GameEngine::screenWidth - panelWidth) / 2.0f;
    float panelY = (GameEngine::screenHeight - panelHeight) / 2.0f;
    
    SDL_FRect panel = { panelX, panelY, panelWidth, panelHeight };
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);  // Dark gray
    SDL_RenderFillRect(renderer, &panel);
    
    // Panel border
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);  // Light gray
    SDL_RenderRect(renderer, &panel);
    
    // TODO: Render menu text with SDL_ttf or ImGui
    // For now, render placeholder rectangles for buttons
    
    float buttonWidth = 300;
    float buttonHeight = 50;
    float buttonX = panelX + (panelWidth - buttonWidth) / 2.0f;
    float buttonY = panelY + 80;
    float buttonSpacing = 70;
    
    int selectedOption = GameMenu::Get().GetSelectedOption();
    
    // Resume button
    SDL_FRect resumeButton = { buttonX, buttonY, buttonWidth, buttonHeight };
    SDL_SetRenderDrawColor(renderer, 80, 120, 180, 255);  // Blue
    SDL_RenderFillRect(renderer, &resumeButton);
    if (selectedOption == GameMenu::Resume)
    {
        // Selected - draw thick yellow border
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderRect(renderer, &resumeButton);
        SDL_FRect innerRect = { buttonX + 2, buttonY + 2, buttonWidth - 4, buttonHeight - 4 };
        SDL_RenderRect(renderer, &innerRect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &resumeButton);
    }
    
    // Restart button
    SDL_FRect restartButton = { buttonX, buttonY + buttonSpacing, buttonWidth, buttonHeight };
    SDL_SetRenderDrawColor(renderer, 180, 120, 80, 255);  // Orange
    SDL_RenderFillRect(renderer, &restartButton);
    if (selectedOption == GameMenu::Restart)
    {
        // Selected - draw thick yellow border
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderRect(renderer, &restartButton);
        SDL_FRect innerRect = { buttonX + 2, buttonY + buttonSpacing + 2, buttonWidth - 4, buttonHeight - 4 };
        SDL_RenderRect(renderer, &innerRect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &restartButton);
    }
    
    // Quit button
    SDL_FRect quitButton = { buttonX, buttonY + buttonSpacing * 2, buttonWidth, buttonHeight };
    SDL_SetRenderDrawColor(renderer, 180, 80, 80, 255);  // Red
    SDL_RenderFillRect(renderer, &quitButton);
    if (selectedOption == GameMenu::Quit)
    {
        // Selected - draw thick yellow border
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderRect(renderer, &quitButton);
        SDL_FRect innerRect = { buttonX + 2, buttonY + buttonSpacing * 2 + 2, buttonWidth - 4, buttonHeight - 4 };
        SDL_RenderRect(renderer, &innerRect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &quitButton);
    }
    
    // TODO: Add text rendering with SDL_ttf
    // For now, logs indicate menu is active
    static bool loggedOnce = false;
    if (!loggedOnce)
    {
        SYSTEM_LOG << "UIRenderingSystem: In-game menu rendered (Resume/Restart/Quit)\n";
        loggedOnce = true;
    }
}

void UIRenderingSystem::RenderDebugOverlay(const CameraTransform& cam)
{
    // Render debug information (FPS, entity count, etc.)
    // This is always on top of everything
    
    // TODO: Use SDL_ttf for text rendering
    // For now, just a placeholder
}
//-------------------------------------------------------------
RenderingEditorSystem::RenderingEditorSystem()
{
}
void RenderingEditorSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;
    // Iterate over all entities with Position and VisualSprite
    for (EntityID entity : World::Get().GetSystem<RenderingEditorSystem>()->m_entities)
    {
        try
        {
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
            // Render sprite at position (no camera transform)
            SDL_FRect destRect = {
                pos.position.x - visual.hotSpot.x,
                pos.position.y - visual.hotSpot.y,
                visual.srcRect.w,
                visual.srcRect.h
            };
            // Apply color modulation
            SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
            // Render sprite
            SDL_RenderTexture(renderer, visual.sprite, nullptr, &destRect);
        }
        catch (const std::exception& e)
        {
            std::cerr << "RenderingEditorSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}