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
        
        if (cam.playerId == playerID && cam.isActive)
        {
            // Found the active camera for this player
            transform.worldPosition = cam.position;
            transform.zoom = cam.zoom;
            transform.rotation = cam.rotation;
            transform.viewport = cam.viewportRect;
            transform.isActive = true;
            
            // Calculate screen offset (control + base + shake)
            transform.screenOffset = cam.controlOffset + cam.baseOffset;
            
            // Add shake offset if present
            if (world.HasComponent<CameraEffects_data>(entity))
            {
                CameraEffects_data& effects = world.GetComponent<CameraEffects_data>(entity);
                if (effects.isShaking)
                {
                    transform.screenOffset += effects.shakeOffset;
                }
            }
            
            break;
        }
    }
    
    return transform;
}
