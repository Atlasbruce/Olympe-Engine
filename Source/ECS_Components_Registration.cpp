/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

ECS Component Registration

This file contains all AUTO_REGISTER_COMPONENT calls for ECS components.
Separated from ECS_Components.h to break circular dependency:
  ECS_Components.h -> ComponentRegistry.h -> World.h -> ECS_Components.h

The registrations still execute at static initialization time (before main()).
*/

#include "ECS_Components.h"
#include "ComponentRegistry.h"

// ========================================================================
// COMPONENT AUTO-REGISTRATION
// ========================================================================

// Identity and Position
AUTO_REGISTER_COMPONENT(Identity_data);
AUTO_REGISTER_COMPONENT(Position_data);
AUTO_REGISTER_COMPONENT(BoundingBox_data);

// Triggers and Detection
AUTO_REGISTER_COMPONENT(TriggerZone_data);

// Movement and Physics
AUTO_REGISTER_COMPONENT(Movement_data);
AUTO_REGISTER_COMPONENT(PhysicsBody_data);

// Gameplay
AUTO_REGISTER_COMPONENT(Health_data);
AUTO_REGISTER_COMPONENT(AIBehavior_data);
AUTO_REGISTER_COMPONENT(Inventory_data);

// Visuals and Animation
AUTO_REGISTER_COMPONENT(VisualSprite_data);
AUTO_REGISTER_COMPONENT(VisualEditor_data);
AUTO_REGISTER_COMPONENT(Animation_data);
AUTO_REGISTER_COMPONENT(VisualAnimation_data);
AUTO_REGISTER_COMPONENT(FX_data);

// Audio
AUTO_REGISTER_COMPONENT(AudioSource_data);

// Input and Control
AUTO_REGISTER_COMPONENT(Controller_data);
AUTO_REGISTER_COMPONENT(PlayerController_data);
AUTO_REGISTER_COMPONENT(PlayerBinding_data);
AUTO_REGISTER_COMPONENT(NPC_data);
AUTO_REGISTER_COMPONENT(InputMapping_data);

// Grid and World Settings
AUTO_REGISTER_COMPONENT(GridSettings_data);

// Camera System
AUTO_REGISTER_COMPONENT(Camera_data);
AUTO_REGISTER_COMPONENT(CameraTarget_data);
AUTO_REGISTER_COMPONENT(CameraEffects_data);
AUTO_REGISTER_COMPONENT(CameraBounds_data);
AUTO_REGISTER_COMPONENT(CameraInputBinding_data);

// Collision and Navigation
AUTO_REGISTER_COMPONENT(CollisionZone_data);
AUTO_REGISTER_COMPONENT(NavigationAgent_data);
