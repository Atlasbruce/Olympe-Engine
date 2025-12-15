/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

RenderBackendSystem: Manages RenderTarget entities and coordinates multi-pass rendering
Data-driven approach for split-screen and multi-window support
*/

#include "ECS_Systems.h"
#include "ECS_Components.h"
#include "World.h"
#include "GameEngine.h"
#include "system/CameraManager.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <utility>

//-------------------------------------------------------------
// Implementation structure (pimpl pattern)
//-------------------------------------------------------------
struct RenderBackendSystem::Implementation
{
    // Cache of render target and viewport entities for efficient iteration
    std::vector<EntityID> renderTargetEntities;
    std::vector<EntityID> viewportEntities;
    
    // Track whether we need to rebuild caches
    bool needsRebuild;
    
    Implementation()
        : needsRebuild(true)
    {}
    
    void RebuildCaches();
    void RenderViewport(EntityID viewportEntity);
    void SetupViewportForRendering(const Viewport_data& viewport, const RenderTarget_data& target);
};

//-------------------------------------------------------------
RenderBackendSystem::RenderBackendSystem()
    : pImpl(new Implementation())
{
    // This system doesn't require entities with specific signature
    // It manages RenderTarget and Viewport entities directly
    requiredSignature.reset(); // Empty signature - autonomous system
}

//-------------------------------------------------------------
RenderBackendSystem::~RenderBackendSystem()
{
    delete pImpl;
}

//-------------------------------------------------------------
void RenderBackendSystem::Initialize()
{
    SYSTEM_LOG << "RenderBackendSystem initialized\n";
    pImpl->needsRebuild = true;
}

//-------------------------------------------------------------
void RenderBackendSystem::Process()
{
    // Process phase: Update render target states, handle window events, etc.
    // For now, mainly used to detect changes requiring cache rebuild
    
    if (pImpl->needsRebuild)
    {
        pImpl->RebuildCaches();
    }
}

//-------------------------------------------------------------
void RenderBackendSystem::Render()
{
    // Main multi-pass rendering logic
    // Iterate through all render targets and their viewports
    
    if (pImpl->needsRebuild)
    {
        pImpl->RebuildCaches();
    }
    
    // Render each render target
    for (EntityID rtEntity : pImpl->renderTargetEntities)
    {
        if (!World::Get().IsEntityValid(rtEntity))
            continue;
            
        RenderTarget_data& target = World::Get().GetComponent<RenderTarget_data>(rtEntity);
        
        if (!target.isActive || !target.renderer)
            continue;
        
        // Clear the render target
        SDL_SetRenderDrawColor(target.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(target.renderer);
        
        // Get viewports for this render target
        auto viewports = GetViewportsForRenderTarget(rtEntity);
        
        if (viewports.empty())
        {
            // No viewports defined - render full screen with default camera
            SDL_SetRenderViewport(target.renderer, nullptr);
            
            // Render all ECS systems to this target
            // For now, delegate to World's existing render systems
            // Note: This is a transitional approach - ideally each system
            // would be viewport-aware
            World::Get().Render_ECS_Systems();
        }
        else
        {
            // Sort viewports by render order
            // Pre-fetch World reference for efficiency
            World& world = World::Get();
            std::sort(viewports.begin(), viewports.end(), 
                [&world](EntityID a, EntityID b) {
                    const Viewport_data& va = world.GetComponent<Viewport_data>(a);
                    const Viewport_data& vb = world.GetComponent<Viewport_data>(b);
                    return va.renderOrder < vb.renderOrder;
                });
            
            // Render each viewport
            for (EntityID vpEntity : viewports)
            {
                pImpl->RenderViewport(vpEntity);
            }
        }
        
        // Present the render target
        SDL_RenderPresent(target.renderer);
    }
}

//-------------------------------------------------------------
void RenderBackendSystem::Implementation::RebuildCaches()
{
    // Find all RenderTarget and Viewport entities using World helper
    renderTargetEntities = World::Get().GetEntitiesWithComponent<RenderTarget_data>();
    viewportEntities = World::Get().GetEntitiesWithComponent<Viewport_data>();
    
    needsRebuild = false;
}

//-------------------------------------------------------------
void RenderBackendSystem::Implementation::RenderViewport(EntityID viewportEntity)
{
    // Cache World reference for efficiency
    World& world = World::Get();
    
    if (!world.IsEntityValid(viewportEntity))
        return;
        
    const Viewport_data& viewport = world.GetComponent<Viewport_data>(viewportEntity);
    
    if (!viewport.isActive)
        return;
    
    // Get the render target for this viewport
    if (!world.IsEntityValid(viewport.renderTargetEntity))
        return;
        
    const RenderTarget_data& target = world.GetComponent<RenderTarget_data>(viewport.renderTargetEntity);
    
    if (!target.renderer)
        return;
    
    // Setup viewport
    SetupViewportForRendering(viewport, target);
    
    // Setup camera if specified
    if (world.IsEntityValid(viewport.cameraEntity))
    {
        // Apply camera transform for this viewport
        // For now, use legacy CameraManager with player index
        CameraManager::Get().SetActivePlayerID(viewport.playerIndex);
        CameraManager::Get().Apply(target.renderer, viewport.playerIndex);
    }
    
    // Render all ECS systems for this viewport
    // TODO: Make render systems viewport-aware by passing viewport context
    // For now, systems use CameraManager's active player to determine camera
    world.Render_ECS_Systems();
}

//-------------------------------------------------------------
void RenderBackendSystem::Implementation::SetupViewportForRendering(
    const Viewport_data& viewport, 
    const RenderTarget_data& target)
{
    // Convert FRect to Rect for SDL
    SDL_Rect viewportRect;
    viewportRect.x = static_cast<int>(viewport.rect.x);
    viewportRect.y = static_cast<int>(viewport.rect.y);
    viewportRect.w = static_cast<int>(viewport.rect.w);
    viewportRect.h = static_cast<int>(viewport.rect.h);
    
    // Set the viewport on the renderer
    SDL_SetRenderViewport(target.renderer, &viewportRect);
}

//-------------------------------------------------------------
EntityID RenderBackendSystem::CreatePrimaryRenderTarget(SDL_Window* window, SDL_Renderer* renderer)
{
    EntityID entity = World::Get().CreateEntity();
    
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    
    RenderTarget_data& rt = World::Get().AddComponent<RenderTarget_data>(entity);
    rt.window = window;
    rt.renderer = renderer;
    rt.type = RenderTargetType::Primary;
    rt.index = 0;
    rt.isActive = true;
    rt.width = w;
    rt.height = h;
    
    pImpl->needsRebuild = true;
    
    return entity;
}

//-------------------------------------------------------------
EntityID RenderBackendSystem::CreateSecondaryRenderTarget(
    const char* title, int width, int height, int index)
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    
    if (!SDL_CreateWindowAndRenderer(title, width, height, 
        SDL_WINDOW_RESIZABLE, &window, &renderer))
    {
        SYSTEM_LOG << "Failed to create secondary window '" << title 
                   << "' (" << width << "x" << height << "): " 
                   << SDL_GetError() << "\n";
        return INVALID_ENTITY_ID;
    }
    
    SDL_SetRenderLogicalPresentation(renderer, width, height, 
        SDL_LOGICAL_PRESENTATION_LETTERBOX);
    
    EntityID entity = World::Get().CreateEntity();
    
    RenderTarget_data& rt = World::Get().AddComponent<RenderTarget_data>(entity);
    rt.window = window;
    rt.renderer = renderer;
    rt.type = RenderTargetType::Secondary;
    rt.index = index;
    rt.isActive = true;
    rt.width = width;
    rt.height = height;
    
    pImpl->needsRebuild = true;
    
    return entity;
}

//-------------------------------------------------------------
EntityID RenderBackendSystem::CreateViewport(
    short playerIndex, const SDL_FRect& rect, EntityID renderTargetEntity)
{
    EntityID entity = World::Get().CreateEntity();
    
    Viewport_data& vp = World::Get().AddComponent<Viewport_data>(entity);
    vp.rect = rect;
    vp.playerIndex = playerIndex;
    vp.renderTargetEntity = renderTargetEntity;
    vp.isActive = true;
    vp.renderOrder = playerIndex; // Default order by player index
    
    pImpl->needsRebuild = true;
    
    return entity;
}

//-------------------------------------------------------------
void RenderBackendSystem::UpdateViewportLayouts()
{
    // Update viewport rectangles based on current configuration
    // This would be called when players are added/removed in split-screen mode
    
    // For now, this is a placeholder that would integrate with ViewportManager
    // or implement the layout logic directly
    
    pImpl->needsRebuild = true;
}

//-------------------------------------------------------------
std::vector<EntityID> RenderBackendSystem::GetActiveRenderTargets() const
{
    std::vector<EntityID> result;
    
    for (EntityID entity : pImpl->renderTargetEntities)
    {
        if (World::Get().IsEntityValid(entity))
        {
            const RenderTarget_data& rt = World::Get().GetComponent<RenderTarget_data>(entity);
            if (rt.isActive)
            {
                result.push_back(entity);
            }
        }
    }
    
    return result;
}

//-------------------------------------------------------------
std::vector<EntityID> RenderBackendSystem::GetViewportsForRenderTarget(
    EntityID renderTargetEntity) const
{
    std::vector<EntityID> result;
    
    for (EntityID entity : pImpl->viewportEntities)
    {
        if (World::Get().IsEntityValid(entity))
        {
            const Viewport_data& vp = World::Get().GetComponent<Viewport_data>(entity);
            if (vp.renderTargetEntity == renderTargetEntity && vp.isActive)
            {
                result.push_back(entity);
            }
        }
    }
    
    return result;
}

//-------------------------------------------------------------
// High-level configuration helpers
//-------------------------------------------------------------

std::vector<EntityID> RenderBackendSystem::SetupSplitScreen(int numPlayers)
{
    std::vector<EntityID> viewports;
    
    if (numPlayers < 1 || numPlayers > 8)
    {
        SYSTEM_LOG << "SetupSplitScreen: Invalid number of players (" << numPlayers << "). Must be 1-8.\n";
        return viewports;
    }
    
    // Get the primary render target
    auto renderTargets = GetActiveRenderTargets();
    if (renderTargets.empty())
    {
        SYSTEM_LOG << "SetupSplitScreen: No active render target found.\n";
        return viewports;
    }
    
    EntityID primaryTarget = renderTargets[0];
    const RenderTarget_data& target = World::Get().GetComponent<RenderTarget_data>(primaryTarget);
    
    float w = static_cast<float>(target.width);
    float h = static_cast<float>(target.height);
    
    // Calculate viewport rectangles based on number of players
    // Layout logic similar to ViewportManager
    std::vector<SDL_FRect> rects;
    
    switch (numPlayers)
    {
        case 1:
            rects.push_back({0.f, 0.f, w, h});
            break;
        case 2:
            rects.push_back({0.f, 0.f, w/2.f, h});
            rects.push_back({w/2.f, 0.f, w/2.f, h});
            break;
        case 3:
            rects.push_back({0.f, 0.f, w/3.f, h});
            rects.push_back({w/3.f, 0.f, w/3.f, h});
            rects.push_back({2.f*w/3.f, 0.f, w/3.f, h});
            break;
        case 4:
            rects.push_back({0.f, 0.f, w/2.f, h/2.f});
            rects.push_back({w/2.f, 0.f, w/2.f, h/2.f});
            rects.push_back({0.f, h/2.f, w/2.f, h/2.f});
            rects.push_back({w/2.f, h/2.f, w/2.f, h/2.f});
            break;
        case 5:
        case 6:
            // 3x2 grid
            for (int i = 0; i < numPlayers; ++i)
            {
                int col = i % 3;
                int row = i / 3;
                rects.push_back({col*w/3.f, row*h/2.f, w/3.f, h/2.f});
            }
            break;
        case 7:
        case 8:
            // 4x2 grid
            for (int i = 0; i < numPlayers; ++i)
            {
                int col = i % 4;
                int row = i / 4;
                rects.push_back({col*w/4.f, row*h/2.f, w/4.f, h/2.f});
            }
            break;
    }
    
    // Create viewport entities
    for (int i = 0; i < numPlayers && i < static_cast<int>(rects.size()); ++i)
    {
        EntityID vp = CreateViewport(static_cast<short>(i), rects[i], primaryTarget);
        viewports.push_back(vp);
    }
    
    SYSTEM_LOG << "SetupSplitScreen: Created " << viewports.size() << " viewports for " << numPlayers << " players\n";
    
    return viewports;
}

//-------------------------------------------------------------
std::vector<std::pair<EntityID, EntityID>> RenderBackendSystem::SetupMultiWindow(
    int numPlayers, int width, int height)
{
    std::vector<std::pair<EntityID, EntityID>> result;
    
    if (numPlayers < 1 || numPlayers > 4)
    {
        SYSTEM_LOG << "SetupMultiWindow: Invalid number of players (" << numPlayers << "). Must be 1-4.\n";
        return result;
    }
    
    // Create a separate window for each player
    const int TITLE_BUFFER_SIZE = 64;
    for (int i = 0; i < numPlayers; ++i)
    {
        char title[TITLE_BUFFER_SIZE];
        snprintf(title, TITLE_BUFFER_SIZE, "Player %d - Olympe Engine", i + 1);
        
        EntityID rtEntity = CreateSecondaryRenderTarget(title, width, height, i + 1);
        
        if (rtEntity == INVALID_ENTITY_ID)
        {
            SYSTEM_LOG << "SetupMultiWindow: Failed to create window for player " << i << "\n";
            continue;
        }
        
        // Create a full-screen viewport for this window
        SDL_FRect fullRect = {0.f, 0.f, static_cast<float>(width), static_cast<float>(height)};
        EntityID vpEntity = CreateViewport(static_cast<short>(i), fullRect, rtEntity);
        
        result.push_back(std::make_pair(rtEntity, vpEntity));
    }
    
    SYSTEM_LOG << "SetupMultiWindow: Created " << result.size() << " windows for " << numPlayers << " players\n";
    
    return result;
}

//-------------------------------------------------------------
void RenderBackendSystem::ClearAllViewportsAndTargets()
{
    // Get all render targets except primary
    auto renderTargets = World::Get().GetEntitiesWithComponent<RenderTarget_data>();
    
    for (EntityID entity : renderTargets)
    {
        const RenderTarget_data& rt = World::Get().GetComponent<RenderTarget_data>(entity);
        
        // Skip primary render target
        if (rt.type == RenderTargetType::Primary)
            continue;
        
        // Clean up secondary windows
        if (rt.window)
        {
            SDL_DestroyWindow(rt.window);
        }
        
        // Destroy the entity
        World::Get().DestroyEntity(entity);
    }
    
    // Destroy all viewport entities
    auto viewports = World::Get().GetEntitiesWithComponent<Viewport_data>();
    for (EntityID entity : viewports)
    {
        World::Get().DestroyEntity(entity);
    }
    
    pImpl->needsRebuild = true;
    
    SYSTEM_LOG << "ClearAllViewportsAndTargets: Cleaned up all non-primary render targets and viewports\n";
}

//-------------------------------------------------------------
void RenderBackendSystem::SwitchToSplitScreen(int numPlayers)
{
    SYSTEM_LOG << "Switching to split-screen mode with " << numPlayers << " players\n";
    
    ClearAllViewportsAndTargets();
    SetupSplitScreen(numPlayers);
}

//-------------------------------------------------------------
void RenderBackendSystem::SwitchToMultiWindow(int numPlayers, int width, int height)
{
    SYSTEM_LOG << "Switching to multi-window mode with " << numPlayers << " players\n";
    
    ClearAllViewportsAndTargets();
    SetupMultiWindow(numPlayers, width, height);
}
