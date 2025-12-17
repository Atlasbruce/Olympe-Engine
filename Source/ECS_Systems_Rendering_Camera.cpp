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

// Structure that holds camera transformation data for rendering
struct CameraTransform
{
    Vector worldPosition;        // Camera position in world space
    Vector screenOffset;         // Screen offset (control + shake)
    float zoom;                  // Zoom level
    float rotation;              // Rotation angle in degrees
    SDL_FRect viewport;          // Viewport rectangle
    bool isActive;               // Is this camera active
    
    // Transform a world position to screen coordinates
    Vector WorldToScreen(const Vector& worldPos) const
    {
        if (!isActive)
            return worldPos;
        
        // 1. Calculate position relative to camera
        Vector relative = worldPos - worldPosition;
        
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
        relative.x += viewport.x + viewport.w / 2.0f;
        relative.y += viewport.y + viewport.h / 2.0f;
        
        return relative;
    }
    
    // Transform a world size to screen size
    Vector WorldSizeToScreenSize(const Vector& worldSize) const
    {
        return worldSize * zoom;
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
        bool intersects = !(maxX < viewport.x || 
                           minX > viewport.x + viewport.w ||
                           maxY < viewport.y || 
                           minY > viewport.y + viewport.h);
        
        return intersects;
    }
};

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
        
        // Check if this is the camera we're looking for
        if (!world.HasComponent<Camera_data>(entity))
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
