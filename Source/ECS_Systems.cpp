/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ECS Systems purpose: Define systems that operate on entities with specific components.

*/

#include "ECS_Systems.h"
#include "ECS_Components.h"
#include "ECS_Register.h"
#include "ECS_Entity.h"
#include "World.h" 
#include "GameEngine.h" // For delta time (fDt)
#include "InputsManager.h"
#include "system/KeyboardManager.h"
#include "system/JoystickManager.h"
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

    // Get the active camera position for current viewport
    // This is set by the main render loop when iterating viewports
    Vector cameraPosition = CameraManager::Get().GetCameraPositionForActivePlayer();
    
    // Iterate ONLY over the relevant entities stored in m_entities
    for (EntityID entity : m_entities)
    {
        try
        {
            // Direct and fast access to Component data from the Pools
            Position_data& pos = World::Get().GetComponent<Position_data>(entity);
            VisualSprite_data& visual = World::Get().GetComponent<VisualSprite_data>(entity);
            BoundingBox_data& boxComp = World::Get().GetComponent<BoundingBox_data>(entity);

            if (visual.sprite)
            {
                Vector vRenderPos = pos.position - visual.hotSpot - cameraPosition;
                SDL_FRect box = boxComp.boundingBox;
                box.x = vRenderPos.x;
                box.y = vRenderPos.y;
				
				SDL_SetTextureColorMod(visual.sprite, visual.color.r, visual.color.g, visual.color.b);
                SDL_RenderTexture(GameEngine::renderer, visual.sprite, nullptr, &box);

				// Debug: draw bounding box
				SDL_SetRenderDrawColor(GameEngine::renderer, 255, 0, 0, 255);
				Draw_Circle(GameEngine::renderer, (int)( box.x + box.w / 2.f) , (int) (box.y + box.h / 2.f), 35);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "RenderingSystem Error for Entity " << entity << ": " << e.what() << "\n";
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
CameraSystem::CameraSystem()
{
    // Required components: Camera_data
    requiredSignature.set(GetComponentTypeID_Static<Camera_data>(), true);
}

void CameraSystem::Process()
{
    // Iterate over all entities with Camera_data
    for (EntityID entity : m_entities)
    {
        try
        {
            Camera_data& camera = World::Get().GetComponent<Camera_data>(entity);
            
            // Update camera position if following a target
            if (camera.followTarget && camera.targetEntity != INVALID_ENTITY_ID)
            {
                // Check if target entity is valid
                if (World::Get().IsEntityValid(camera.targetEntity))
                {
                    // Get target entity position
                    Position_data& targetPos = World::Get().GetComponent<Position_data>(camera.targetEntity);
                    
                    // Calculate desired camera position (target + offset)
                    Vector desiredPos = targetPos.position + camera.offset;
                    
                    // Smooth follow using vBlend (consistent with CameraManager)
                    camera.position = vBlend(camera.position, desiredPos, camera.followSpeed);
                    
                    // Apply camera bounds if set
                    if (camera.bounds.w != INT_MAX && camera.bounds.h != INT_MAX)
                    {
                        if (camera.position.x < camera.bounds.x)
                            camera.position.x = static_cast<float>(camera.bounds.x);
                        if (camera.position.y < camera.bounds.y)
                            camera.position.y = static_cast<float>(camera.bounds.y);
                        if (camera.position.x > camera.bounds.x + camera.bounds.w)
                            camera.position.x = static_cast<float>(camera.bounds.x + camera.bounds.w);
                        if (camera.position.y > camera.bounds.y + camera.bounds.h)
                            camera.position.y = static_cast<float>(camera.bounds.y + camera.bounds.h);
                    }
                }
                else
                {
                    // Target entity no longer valid, disable following
                    camera.followTarget = false;
                    camera.targetEntity = INVALID_ENTITY_ID;
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "CameraSystem Error for Entity " << entity << ": " << e.what() << "\n";
        }
    }
}
//-------------------------------------------------------------
