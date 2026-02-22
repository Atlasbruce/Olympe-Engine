/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#pragma once

#include "ECS_Entity.h"
#include "ECS_Components.h"
#include <set>
#include <SDL3/SDL.h>
#include "vector.h"
#include <unordered_map>
#include "system/message.h"

// Forward declaration
struct CameraTransform;
struct GridSettings_data;
struct CameraInputBinding_data;

// Prototype function to render entities for a given camera
void RenderEntitiesForCamera(const CameraTransform& cam);
void RenderMultiLayerForCamera(const CameraTransform& cam);
void RenderSingleEntity(const CameraTransform& cam, EntityID entity);
// Get the active camera transform for a specific player
CameraTransform GetActiveCameraTransform(short playerID);

// The System class handles game logic over entities with specific components.
class ECS_System
{
public:
    // The signature required for an Entity to be processed by this System
    ComponentSignature requiredSignature;

    // The set of Entities this System processes in its Update loop
    std::set<EntityID> m_entities;

    ECS_System() : requiredSignature() {}

    // The core logic of the System
    virtual void Process() {}
	virtual void Render() {}
    virtual void RenderDebug() {}

    void AddEntity(EntityID entity) { m_entities.insert(entity); }
    void RemoveEntity(EntityID entity) { m_entities.erase(entity); }
};


// Input System: processes entities with Input_data
class InputSystem : public ECS_System
{
    public:
    InputSystem();
	virtual void Process() override;
 };

// InputEventConsumeSystem: Consumes Input domain events from EventQueue
// and updates ECS components (Controller_data, PlayerBinding_data)
// Replaces the old UpdateECSInputFromMessage callback-based approach
class InputEventConsumeSystem : public ECS_System
{
public:
    InputEventConsumeSystem();
    virtual void Process() override;
};

// GameEventConsumeSystem: Consumes Gameplay domain events from EventQueue
// Handles game state changes (pause/resume/quit), player add/remove via keyboard
// Replaces VideoGame::OnEvent callback-based approach
class GameEventConsumeSystem : public ECS_System
{
public:
    GameEventConsumeSystem();
    virtual void Process() override;
};

// UIEventConsumeSystem: Consumes UI domain events from EventQueue
// Handles menu activation/deactivation and validation
// Replaces GameMenu::OnEvent callback-based approach
class UIEventConsumeSystem : public ECS_System
{
public:
    UIEventConsumeSystem();
    virtual void Process() override;
};

// CameraEventConsumeSystem: Consumes Camera domain events from EventQueue
// Handles camera control events (shake, zoom, follow, etc.)
// Replaces CameraSystem::OnEvent callback-based approach
class CameraEventConsumeSystem : public ECS_System
{
public:
    CameraEventConsumeSystem();
    virtual void Process() override;
};

// AI System: processes entities with AIBehavior_data and Movement_data
class AISystem : public ECS_System
{
    public:
    AISystem();
    virtual void Process() override;
};
// Detection System: processes entities with TriggerZone_data and Position_data
class DetectionSystem : public ECS_System
{
    public:
    DetectionSystem();
    virtual void Process() override;
};
// Physics System: processes entities with Transform_data and PhysicsBody_data
class PhysicsSystem : public ECS_System
{
    public:
    PhysicsSystem();
    virtual void Process() override;
};
// Collision System: processes entities with BoundingBox_data and Position_data
class CollisionSystem : public ECS_System
{
    public:
    CollisionSystem();
    virtual void Process() override;
};
// Trigger System: processes entities with TriggerZone_data and Position_data
class TriggerSystem : public ECS_System
{
    public:
    TriggerSystem();
    virtual void Process() override;
};
// Navigation System: processes entities with NavigationAgent_data
class NavigationSystem : public ECS_System
{
public:
	NavigationSystem();
	virtual void Process() override;
	
	// Request pathfinding for entity
	void RequestPath(EntityID entity, const Vector& targetPos);
	
	// Follow current path
	void FollowPath(EntityID entity, float deltaTime);
	
	// Check if repath needed (obstacle detected)
	bool NeedsRepath(EntityID entity);
};
// Movement System: processes entities with Transform_data and Movement_data
class MovementSystem : public ECS_System
{
    public:
    MovementSystem();
    virtual void Process() override;
};
// Rendering System: processes entities with Transform_data and VisualSprite_data
class RenderingSystem : public ECS_System
{
public:
    RenderingSystem();
	virtual void Render() override;
};
// Rendering Editor System: displays editor-specific visuals (grid, gizmos, etc.)
class RenderingEditorSystem : public ECS_System
{
public:
    RenderingEditorSystem();
    virtual void Render() override;
};
//-------------------------------------------------------------
class GridSystem : public ECS_System
{
public:
    GridSystem();
    virtual void Render() override;
    void RenderForCamera(const CameraTransform& cam);

    const GridSettings_data* FindSettings() const;

    void DrawLineWorld(const CameraTransform& cam, const Vector& aWorld, const Vector& bWorld, const SDL_Color& c);
    
    // **NEW: Draw filled rectangle in world space (for overlays)**
    void DrawFilledRectWorld(const CameraTransform& cam, const Vector& worldPos, float width, float height, const SDL_Color& c);

    SDL_FRect GetWorldVisibleBounds(const CameraTransform& cam) ;

    void RenderOrtho(const CameraTransform& cam, const GridSettings_data& s);
    void RenderIso(const CameraTransform& cam, const GridSettings_data& s);
    void RenderHex(const CameraTransform& cam, const GridSettings_data& s);
    
    // **NEW: Overlay rendering methods**
    void RenderCollisionOverlay(const CameraTransform& cam, const GridSettings_data& s);
    void RenderNavigationOverlay(const CameraTransform& cam, const GridSettings_data& s);
    
    // **NEW: Helper functions for drawing overlay shapes**
    void DrawIsometricTileOverlay(float centerX, float centerY, float width, float height, const SDL_Color& color);
	void DrawHexagonOverlay(float centerX, float centerY, float radius, const SDL_Color& color);
};
//-------------------------------------------------------------
// UI Rendering System: Pass 2 rendering for UI/HUD/Menu (always on top)
// Renders UI elements independently of world depth sorting
class UIRenderingSystem : public ECS_System
{
public:
    UIRenderingSystem();
    virtual void Render() override;
    
    void RenderHUD(const CameraTransform& cam);
    void RenderInGameMenu(const CameraTransform& cam);
    void RenderDebugOverlay(const CameraTransform& cam);
};
//-------------------------------------------------------------
// Player Control System: processes entities with PlayerBinding_data and Controller_data
class PlayerControlSystem : public ECS_System
{
    public:
    PlayerControlSystem();
    virtual void Process() override;
};
// Input Mapping System: maps hardware input to gameplay actions
class InputMappingSystem : public ECS_System
{
    public:
    InputMappingSystem();
    virtual void Process() override;
};

//-------------------------------------------------------------
// Olympe Effect System: Plasma bloom background with glowing orbs
// No entity required - autonomous visual effect system
class OlympeEffectSystem : public ECS_System
{
public:
    OlympeEffectSystem();
    virtual ~OlympeEffectSystem();
    
    virtual void Process() override;
    virtual void Render() override;
    
    void Initialize();

private:
    // Implementation details hidden - defined in OlympeEffectSystem.cpp
    struct Implementation;
    Implementation* pImpl;
};
//-------------------------------------------------------------
// CameraSystem - Manages all camera entities in the ECS
class CameraSystem : public ECS_System
{
public:
    CameraSystem();
    virtual ~CameraSystem();

    // Core ECS system methods
    virtual void Process() override;  // Update cameras each frame
    virtual void Render() override;   // Update viewport settings

    // Camera creation and management
    EntityID CreateCameraForPlayer(short playerID, bool bindToKeyboard = false);
    void RemoveCameraForPlayer(short playerID);
    EntityID GetCameraEntityForPlayer(short playerID);

    // Input binding
    void BindCameraToKeyboard(EntityID cameraEntity);
    void BindCameraToJoystick(EntityID cameraEntity, short playerID, SDL_JoystickID joystickId);
    void UnbindCameraKeyboard(EntityID cameraEntity);

    // Target setting
    void SetCameraTarget_ECS(EntityID cameraEntity, EntityID targetEntity);
    void ClearCameraTarget(EntityID cameraEntity);

    // Rendering support
    void ApplyCameraToRenderer(SDL_Renderer* renderer, short playerID);

    // Event handling
    void OnEvent(const Message& msg);

    // Update methods called during Process()
    void UpdateCameraInput(EntityID entity, float dt);
    void UpdateCameraFollow(EntityID entity, float dt);
    void UpdateCameraZoom(EntityID entity, float dt);
    void UpdateCameraRotation(EntityID entity, float dt);
    void UpdateCameraShake(EntityID entity, float dt);
    void ApplyCameraBounds(EntityID entity);

    // Input processing
    void ProcessKeyboardInput(EntityID entity, CameraInputBinding_data& binding);
    void ProcessJoystickInput(EntityID entity, CameraInputBinding_data& binding);
    float ApplyDeadzone(float value, float deadzone);

    // Camera control
    void ResetCameraControls(EntityID entity);
    void SyncZoomLevelIndex(Camera_data& cam);  // Helper to sync currentZoomLevelIndex with targetZoom

    // Player camera mapping
    std::unordered_map<short, EntityID> m_playerCameras; // Map player ID to camera entity
    EntityID m_defaultKeyboardCamera = INVALID_ENTITY_ID; // Default keyboard-controlled camera
};
//-------------------------------------------------------------

// Structure that holds camera transformation data for rendering
struct CameraTransform
{
    Vector worldPosition;        // Camera position in world space
    Vector screenOffset;         // Screen offset (control + shake)
    float zoom = 0.f;                  // Zoom level
    float rotation = 0.f;              // Rotation angle in degrees
    SDL_FRect viewport = {0.f, 0.f, 0.f, 0.f};          // Viewport rectangle
    bool isActive = false;               // Is this camera active

    // Transform a world position to screen coordinates
    Vector WorldToScreen(const Vector& worldPos) const
    {
        if (!isActive)
            return worldPos;

        // 1. Calculate position relative to camera
        Vector relative = worldPos;
        relative = relative - worldPosition;

        // 2. Apply rotation (convert degrees to radians)
        if (rotation != 0.0f)
        {
            float rotRad = rotation * (float)(k_PI / 180.0);
            float cosRot = std::cos(rotRad);
            float sinRot = std::sin(rotRad);

            float rotatedX = relative.x * cosRot - relative.y * sinRot;
            float rotatedY = relative.x * sinRot + relative.y * cosRot;

            relative.x = rotatedX;
            relative.y = rotatedY;
        }

        // 3. Apply zoom
        relative.x *= zoom;
        relative.y *= zoom;

        // 4. Apply screen offset (control offset + shake)
        relative.x -= screenOffset.x;
        relative.y -= screenOffset.y;

        // 5. Center in viewport
        // SDL viewport is already set; coordinates are viewport-local
        // Add half viewport dimensions to center within viewport bounds
        relative.x += viewport.w / 2.0f;
        relative.y += viewport.h / 2.0f;

        return relative;
    }

    // Transform a world size to screen size
    Vector WorldSizeToScreenSize(const Vector& worldSize) const
    {
        Vector size = worldSize;
        return size * zoom;
    }

    // Transform a screen position to world coordinates (inverse of WorldToScreen)
    Vector ScreenToWorld(const Vector& screenPos) const
    {
        if (!isActive)
            return screenPos;

        // 1. Remove viewport offset
        // screenPos is viewport-local when SDL viewport is set
        // Coordinates are relative to viewport's top-left corner (0,0)
        Vector relative;
        relative.x = screenPos.x - viewport.w / 2.0f;
        relative.y = screenPos.y - viewport.h / 2.0f;

        // 2. Remove screen offset (shake + control)
        relative.x += screenOffset.x;
        relative.y += screenOffset.y;

        // 3. Inverse zoom
        relative.x /= zoom;
        relative.y /= zoom;

        // 4. Inverse rotation (if any)
        if (rotation != 0.0f)
        {
            float rotRad = -rotation * (float)(k_PI / 180.0);
            float cosRot = std::cos(rotRad);
            float sinRot = std::sin(rotRad);

            float rotatedX = relative.x * cosRot - relative.y * sinRot;
            float rotatedY = relative.x * sinRot + relative.y * cosRot;

            relative.x = rotatedX;
            relative.y = rotatedY;
        }

        // 5. Add world position
        relative.x += worldPosition.x;
        relative.y += worldPosition.y;

        return relative;
    }

    // Check if a world-space bounding box is visible in this camera
    bool IsVisible(const SDL_FRect& worldBounds) const
    {
        if (!isActive)
            return false;

        // Transform all four corners of the bounding box
        Vector corners[4] = {
            Vector(worldBounds.x, worldBounds.y, 0.f),
            Vector(worldBounds.x + worldBounds.w, worldBounds.y, 0.f),
            Vector(worldBounds.x, worldBounds.y + worldBounds.h, 0.f),
            Vector(worldBounds.x + worldBounds.w, worldBounds.y + worldBounds.h, 0.f)
        };

        // Transform all corners to screen space
        Vector screenCorners[4];
        for (int i = 0; i < 4; i++)
        {
            screenCorners[i] = WorldToScreen(corners[i]);
        }

        // Calculate screen bounding box
        float minX = screenCorners[0].x;
        float maxX = screenCorners[0].x;
        float minY = screenCorners[0].y;
        float maxY = screenCorners[0].y;

        for (int i = 1; i < 4; i++)
        {
            if (screenCorners[i].x < minX) minX = screenCorners[i].x;
            if (screenCorners[i].x > maxX) maxX = screenCorners[i].x;
            if (screenCorners[i].y < minY) minY = screenCorners[i].y;
            if (screenCorners[i].y > maxY) maxY = screenCorners[i].y;
        }

        // Check if screen bounding box intersects viewport
        // WorldToScreen returns viewport-local coordinates (since SDL_SetRenderViewport is used)
        // So we check against viewport-local bounds [0, viewport.w] x [0, viewport.h]
        bool intersects = !(maxX < 0.f ||
            minX > viewport.w ||
            maxY < 0.f ||
            minY > viewport.h);

        return intersects;
    }
};