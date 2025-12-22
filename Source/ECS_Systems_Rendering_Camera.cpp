/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Camera Rendering Integration: Provides camera transformation utilities for the
RenderingSystem, including world-to-screen coordinate conversion and frustum culling.

*/

#include "ECS_Components_Camera.h"
#include "ECS_Components.h"
#include "ECS_Register.h"
#include "World.h"
#include "vector.h"
#include "system/system_consts.h"
#include "system/ViewportManager.h"
#include <cmath>
#include <SDL3/SDL.h>
#include "ECS_Systems_Rendering_Camera.h"


// Get the active camera transform for a specific player
CameraTransform GetActiveCameraTransform(short playerID)
{
    CameraTransform transform;
    transform.isActive = false;
    
    // Iterate through all entities to find the camera for this player
    World& world = World::Get();
    
    for (const auto& pair : world.m_entitySignatures)
    {
        EntityID entity = pair.first;
        const ComponentSignature& signature = pair.second;
        
        // Check if entity has Camera_data component
        ComponentTypeID camTypeID = GetComponentTypeID_Static<Camera_data>();
        if (!signature.test(camTypeID))
            continue;
        
        Camera_data& cam = world.GetComponent<Camera_data>(entity);
        
        // For player cameras (playerID >= 0), only consider cameras with matching playerID
        // Never fall back to default camera (playerId=-1) for player rendering
        if (playerID >= 0)
        {
            if (cam.playerId == playerID && cam.isActive)
            {
                // Found the active camera for this player
                // Apply all world-space offsets to worldPosition (control + base + shake)
                transform.worldPosition = cam.position + cam.controlOffset + cam.baseOffset;
                
                // Add shake offset if present
                if (world.HasComponent<CameraEffects_data>(entity))
                {
                    CameraEffects_data& effects = world.GetComponent<CameraEffects_data>(entity);
                    if (effects.isShaking)
                    {
                        transform.worldPosition += effects.shakeOffset;
                    }
                }
                
                transform.zoom = cam.zoom;
                transform.rotation = cam.rotation;
                
                // Get the actual current viewport for this player from ViewportManager
                SDL_FRect playerViewport;
                if (ViewportManager::Get().GetViewRectForPlayer(playerID, playerViewport))
                {
                    transform.viewport = playerViewport;
                }
                else
                {
                    // Fallback: use stored viewport if player not found (shouldn't happen for active players)
                    transform.viewport = cam.viewportRect;
                }
                
                transform.isActive = true;
                
                // Screen offset is zero for world rendering (offsets are applied in world space)
                transform.screenOffset = Vector(0.f, 0.f, 0.f);
                
                break;
            }
        }
        else
        {
            // For single-view/no-players case, allow default camera (playerId=-1)

            if (cam.playerId == -1 && cam.isActive)
            {
                // Found the default camera
                // Apply all world-space offsets to worldPosition (control + base + shake)
                transform.worldPosition = cam.position + cam.controlOffset + cam.baseOffset;
                
                // Add shake offset if present
                if (world.HasComponent<CameraEffects_data>(entity))
                {
                    CameraEffects_data& effects = world.GetComponent<CameraEffects_data>(entity);
                    if (effects.isShaking)
                    {
                        transform.worldPosition += effects.shakeOffset;
                    }
                }
                
                transform.zoom = cam.zoom;
                transform.rotation = cam.rotation;
                transform.viewport = cam.viewportRect;
                transform.isActive = true;
                
                // Screen offset is zero for world rendering (offsets are applied in world space)
                transform.screenOffset = Vector(0.f, 0.f, 0.f);
                
                break;
            }
        }
    }
    
    return transform;
}
