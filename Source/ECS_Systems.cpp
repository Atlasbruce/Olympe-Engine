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
#include <iostream>
#include <bitset>
#include <cmath>
#include "drawing.h"


//-------------------------------------------------------------
InputSystem::InputSystem()
{
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
}
void InputSystem::Process()
{
    // Input processing logic here
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
    // AI processing logic here
}
//-------------------------------------------------------------
DetectionSystem::DetectionSystem()
{
}
void DetectionSystem::Process()
{
    // Detection processing logic here
}
//-------------------------------------------------------------
PhysicsSystem::PhysicsSystem()
{
}
void PhysicsSystem::Process()
{
    // Physics processing logic here
}
//-------------------------------------------------------------
CollisionSystem::CollisionSystem()
{
}
void CollisionSystem::Process()
{
    // Collision processing logic here
}
//-------------------------------------------------------------
TriggerSystem::TriggerSystem()
{
}
void TriggerSystem::Process()
{
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
    // Iterate ONLY over the relevant entities stored in m_entities
    for (EntityID entity : m_entities)
    {
        try
        {
            // Direct and fast access to Component data from the Pools
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            Movement_data& move = World::Get().GetComponent<Movement_data>(entity);
            // Game logic: simple movement based on speed and delta time
            pos.position += move.direction * GameEngine::fDt;
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
    // Define the required components: Position AND VisualSprite
    requiredSignature.set(GetComponentTypeID_Static<Position_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<VisualSprite_data>(), true);
    requiredSignature.set(GetComponentTypeID_Static<BoundingBox_data>(), true);
}
void RenderingSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    // Get player count from ViewportManager
    int playerCount = ViewportManager::Get().GetPlayerCount();
    
    // Check if we have any ECS cameras active
    bool hasECSCameras = false;
    for (short playerID = 0; playerID < playerCount; playerID++)
    {
        CameraTransform camTransform = GetActiveCameraTransform(playerID);
        if (camTransform.isActive)
        {
            hasECSCameras = true;
            break;
        }
    }
    
    // Use ECS camera system if available, otherwise fall back to legacy
    if (hasECSCameras)
    {
        // Multi-camera rendering with ECS camera system
        for (short playerID = 0; playerID < playerCount; playerID++)
        {
            CameraTransform camTransform = GetActiveCameraTransform(playerID);
            

            SYSTEM_LOG << "P" << playerID << " / nb" << playerCount
                << " active=" << camTransform.isActive
                << " cam.vp=("
                << camTransform.viewport.x << ","
                << camTransform.viewport.y << ","
                << camTransform.viewport.w << ","
				<< camTransform.viewport.h << "'" << ")\n";

            if (!camTransform.isActive)
                continue;
            
            // Set viewport and clip rect
            SDL_Rect viewportRect = {
                (int)camTransform.viewport.x,
                (int)camTransform.viewport.y,
                (int)camTransform.viewport.w,
                (int)camTransform.viewport.h
            };

            SDL_SetRenderViewport(renderer, &viewportRect);
            SDL_SetRenderClipRect(renderer, &viewportRect);
            
            // Render entities for this camera
            RenderEntitiesForCamera(camTransform);

            // Clear clip rect
            SDL_SetRenderViewport(renderer, nullptr);
            SDL_SetRenderClipRect(renderer, nullptr);
        }
        
        // Reset viewport
        SDL_SetRenderViewport(renderer, nullptr);
    }
	else // Legacy single-camera rendering TO BE REMOVED LATER
    {
        // Legacy rendering path using CameraManager
        for (EntityID entity : m_entities)
        {
            try
            {
                Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
                BoundingBox_data& boxComp = World::Get().GetComponent<BoundingBox_data>(entity);

                if (visual.sprite)
                {
                    Vector vRenderPos = pos.position - visual.hotSpot - CameraManager::Get().GetCameraPositionForActivePlayer();
                    SDL_FRect box = boxComp.boundingBox;
                    box.x = vRenderPos.x;
                    box.y = vRenderPos.y;
                    
                    SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
                    SDL_RenderTexture(GameEngine::renderer, visual.sprite, nullptr, &box);

                    // Debug: draw bounding box
                    SDL_SetRenderDrawColor(GameEngine::renderer, 0, 255, 255, 255);
                    Draw_Circle(GameEngine::renderer, (int)( box.x + box.w / 2.f) , (int) (box.y + box.h / 2.f), 35);
					SDL_RenderRect(GameEngine::renderer, &boxComp.boundingBox);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "RenderingSystem Error for Entity " << entity << ": " << e.what() << "\n";
            }
        }/**/
    }
}

// Render entities for a specific camera with frustum culling
void RenderEntitiesForCamera(const CameraTransform& cam)
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;
    
    // Get all entities with Position, VisualSprite, and BoundingBox
    for (EntityID entity : World::Get().GetSystem<RenderingSystem>()->m_entities)
    {
        try
        {
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
            Vector screenPos = cam.WorldToScreen(pos.position - visual.hotSpot);
            
            // Transform size to screen space
            Vector screenSize = cam.WorldSizeToScreenSize(
                Vector(boxComp.boundingBox.w, boxComp.boundingBox.h, 0.f)
            );
            
            // Create destination rectangle
            SDL_FRect destRect = {
                screenPos.x,
                screenPos.y,
                screenSize.x,
                screenSize.y
            };
            
            // Render based on sprite type
            if (visual.sprite)
            {
                // Apply color modulation
                SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
                
                // Render with rotation if camera is rotated
                if (cam.rotation != 0.0f)
                {
                    SDL_RenderTextureRotated(renderer, visual.sprite, nullptr, &destRect, 
                                            cam.rotation, nullptr, SDL_FLIP_NONE);
                }
                else
                {
                    SDL_RenderTexture(renderer, visual.sprite, nullptr, &destRect);
                }
                
                // Debug: draw bounding box
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                Draw_Circle(renderer, (int)(destRect.x + destRect.w / 2.f), 
                          (int)(destRect.y + destRect.h / 2.f), 5);
            }
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
    // Singleton simple: on prend la 1ère entité qui a GridSettings_data
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

void GridSystem::Render()
{
    SDL_Renderer* renderer = GameEngine::renderer;
    if (!renderer) return;

    const GridSettings_data* s = FindSettings();
    if (!s || !s->enabled) return;

    int playerCount = ViewportManager::Get().GetPlayerCount();
    for (short playerID = 0; playerID < playerCount; playerID++)
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

        SDL_SetRenderClipRect(renderer, nullptr);
    }

    SDL_SetRenderViewport(renderer, nullptr);
}

void GridSystem::RenderOrtho(const CameraTransform& cam, const GridSettings_data& s)
{
    const float csx = max(1.f, s.cellSize.x);
    const float csy = max(1.f, s.cellSize.y);

    // Approx bounds around camera based on viewport size (simple & stable)
    float halfW = cam.viewport.w * 0.5f;
    float halfH = cam.viewport.h * 0.5f;

    float minX = cam.worldPosition.x - halfW;
    float maxX = cam.worldPosition.x + halfW;
    float minY = cam.worldPosition.y - halfH;
    float maxY = cam.worldPosition.y + halfH;

    int lines = 0;

    float startX = std::floor(minX / csx) * csx;
    float endX = std::ceil(maxX / csx) * csx;
    for (float x = startX; x <= endX && lines < s.maxLines; x += csx)
    {
        DrawLineWorld(cam, Vector(x, minY, 0.f), Vector(x, maxY, 0.f), s.color);
        ++lines;
    }

    float startY = std::floor(minY / csy) * csy;
    float endY = std::ceil(maxY / csy) * csy;
    for (float y = startY; y <= endY && lines < s.maxLines; y += csy)
    {
        DrawLineWorld(cam, Vector(minX, y, 0.f), Vector(maxX, y, 0.f), s.color);
        ++lines;
    }
}

void GridSystem::RenderIso(const CameraTransform& cam, const GridSettings_data& s)
{
    const float w = max(1.f, s.cellSize.x);
    const float h = max(1.f, s.cellSize.y);

    Vector u(w * 0.5f, -h * 0.5f, 0.f);
    Vector v(w * 0.5f, h * 0.5f, 0.f);

    float span = max(cam.viewport.w, cam.viewport.h);
    int range = (int)std::ceil((span / min(w, h))) + 8;

    int lines = 0;
    Vector origin(cam.worldPosition.x, cam.worldPosition.y, 0.f);

    for (int i = -range; i <= range && lines < s.maxLines; ++i)
    {
        Vector p0 = origin + v * (float)i - u * (float)range;
        Vector p1 = origin + v * (float)i + u * (float)range;
        DrawLineWorld(cam, p0, p1, s.color);
        ++lines;

        if (lines >= s.maxLines) break;

        p0 = origin + u * (float)i - v * (float)range;
        p1 = origin + u * (float)i + v * (float)range;
        DrawLineWorld(cam, p0, p1, s.color);
        ++lines;
    }
}

void GridSystem::RenderHex(const CameraTransform& cam, const GridSettings_data& s)
{
    const float r = max(1.f, s.hexRadius);

    // pointy-top axial layout
    const float dx = 1.5f * r;
    const float dy = 1.73205080757f * r; // sqrt(3) * r

    float halfW = cam.viewport.w * 0.5f;
    float halfH = cam.viewport.h * 0.5f;

    float minX = cam.worldPosition.x - halfW;
    float maxX = cam.worldPosition.x + halfW;
    float minY = cam.worldPosition.y - halfH;
    float maxY = cam.worldPosition.y + halfH;

    int qMin = (int)std::floor(minX / dx) - 2;
    int qMax = (int)std::ceil(maxX / dx) + 2;
    int rMin = (int)std::floor(minY / dy) - 2;
    int rMax = (int)std::ceil(maxY / dy) + 2;

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
                float a = (60.f * (float)i + 30.f) * 3.1415926535f / 180.f;
                pts[i] = Vector(c.x + std::cos(a) * r, c.y + std::sin(a) * r, 0.f);
            }
            for (int i = 0; i < 6; ++i)
                DrawLineWorld(cam, pts[i], pts[(i + 1) % 6], s.color);
        };

    int lines = 0;
    for (int q = qMin; q <= qMax && lines < s.maxLines; ++q)
    {
        for (int rr = rMin; rr <= rMax && lines < s.maxLines; ++rr)
        {
            Vector c = hexCenter(q, rr);

            // simple cull
            if (c.x + r < minX || c.x - r > maxX || c.y + r < minY || c.y - r > maxY)
                continue;

            drawHex(c);
            lines += 6;
        }
    }
}
