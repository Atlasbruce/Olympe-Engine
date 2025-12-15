/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#pragma once

#include "ECS_Entity.h"
#include <set>


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
// RenderBackend System: Manages RenderTarget entities (windows/renderers)
// Coordinates multi-pass rendering through viewports
class RenderBackendSystem : public ECS_System
{
public:
    RenderBackendSystem();
    virtual ~RenderBackendSystem();
    
    virtual void Process() override;
    virtual void Render() override;
    
    void Initialize();
    
    // Create a primary render target from existing window/renderer
    EntityID CreatePrimaryRenderTarget(SDL_Window* window, SDL_Renderer* renderer);
    
    // Create a secondary window and its render target
    EntityID CreateSecondaryRenderTarget(const char* title, int width, int height, int index);
    
    // Create a viewport for a player
    EntityID CreateViewport(short playerIndex, const SDL_FRect& rect, EntityID renderTargetEntity);
    
    // Update viewport layouts (for split-screen)
    void UpdateViewportLayouts();
    
    // Get all active render targets
    std::vector<EntityID> GetActiveRenderTargets() const;
    
    // Get all viewports for a specific render target
    std::vector<EntityID> GetViewportsForRenderTarget(EntityID renderTargetEntity) const;
    
    // ============ High-level Configuration Helpers ============
    
    // Setup split-screen mode: creates viewports within the primary render target
    // numPlayers: number of players (1-8)
    // Returns vector of viewport entities (one per player)
    std::vector<EntityID> SetupSplitScreen(int numPlayers);
    
    // Setup multi-window mode: creates separate windows for each player
    // numPlayers: number of players (1-4 recommended)
    // width, height: dimensions for each window
    // Returns vector of pairs (renderTarget entity, viewport entity) for each player
    std::vector<std::pair<EntityID, EntityID>> SetupMultiWindow(int numPlayers, int width, int height);
    
    // Clear all viewports and render targets (except primary)
    void ClearAllViewportsAndTargets();
    
    // Switch between split-screen and multi-window modes
    void SwitchToSplitScreen(int numPlayers);
    void SwitchToMultiWindow(int numPlayers, int width, int height);

private:
    // Implementation details hidden
    struct Implementation;
    Implementation* pImpl;
};