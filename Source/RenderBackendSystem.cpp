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
            std::sort(viewports.begin(), viewports.end(), 
                [](EntityID a, EntityID b) {
                    const Viewport_data& va = World::Get().GetComponent<Viewport_data>(a);
                    const Viewport_data& vb = World::Get().GetComponent<Viewport_data>(b);
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
    if (!World::Get().IsEntityValid(viewportEntity))
        return;
        
    const Viewport_data& viewport = World::Get().GetComponent<Viewport_data>(viewportEntity);
    
    if (!viewport.isActive)
        return;
    
    // Get the render target for this viewport
    if (!World::Get().IsEntityValid(viewport.renderTargetEntity))
        return;
        
    const RenderTarget_data& target = World::Get().GetComponent<RenderTarget_data>(viewport.renderTargetEntity);
    
    if (!target.renderer)
        return;
    
    // Setup viewport
    SetupViewportForRendering(viewport, target);
    
    // Setup camera if specified
    if (World::Get().IsEntityValid(viewport.cameraEntity))
    {
        // Apply camera transform for this viewport
        // For now, use legacy CameraManager with player index
        CameraManager::Get().SetActivePlayerID(viewport.playerIndex);
        CameraManager::Get().Apply(target.renderer, viewport.playerIndex);
    }
    
    // Render all ECS systems for this viewport
    // Note: This calls the existing render systems which will use the active viewport
    World::Get().Render_ECS_Systems();
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
        SYSTEM_LOG << "Failed to create secondary window: " << SDL_GetError() << "\n";
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
