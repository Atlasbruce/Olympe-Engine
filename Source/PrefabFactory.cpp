/*
	Olympe Engine V2 - 2025
	Nicolas Chereau
	nchereau@gmail.com

	Purpose:
	- Implementation file for PrefabFactory class.
	- Component-agnostic instantiation system.
*/

#include "prefabfactory.h"
#include "ComponentDefinition.h"
#include "PrefabScanner.h"
#include "World.h"
#include "DataManager.h"
#include "ECS_Components.h"
#include "ECS_Components_AI.h"
#include "ECS_Components_Camera.h"
#include "system/system_utils.h"
#include <string>

// ========================================================================
// Public API Implementation
// ========================================================================

void PrefabFactory::SetPrefabRegistry(const PrefabRegistry& registry)
{
    m_prefabRegistry = registry;
    SYSTEM_LOG << "PrefabFactory: Registry cached with " << registry.GetCount() << " prefabs\n";
}

EntityID PrefabFactory::CreateEntityFromBlueprint(const PrefabBlueprint& blueprint)
{
    if (!blueprint.isValid)
    {
        SYSTEM_LOG << "PrefabFactory::CreateEntityFromBlueprint: Invalid blueprint '" 
                   << blueprint.prefabName << "'\n";
        return INVALID_ENTITY_ID;
    }
    
    // Create entity
    World& world = World::Get();
    EntityID entity = world.CreateEntity();
    
    if (entity == INVALID_ENTITY_ID)
    {
        SYSTEM_LOG << "PrefabFactory::CreateEntityFromBlueprint: Failed to create entity for '" 
                   << blueprint.prefabName << "'\n";
        return INVALID_ENTITY_ID;
    }
    
    // Instantiate all components
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& componentDef : blueprint.components)
    {
        if (InstantiateComponent(entity, componentDef))
        {
            successCount++;
        }
        else
        {
            failCount++;
            SYSTEM_LOG << "PrefabFactory::CreateEntityFromBlueprint: Failed to instantiate component '" 
                       << componentDef.componentType << "' on entity " << entity << "\n";
        }
    }
    
    SYSTEM_LOG << "PrefabFactory::CreateEntityFromBlueprint: Created entity " << entity 
               << " from blueprint '" << blueprint.prefabName 
               << "' (" << successCount << " components, " << failCount << " failed)\n";
    
    return entity;
}

bool PrefabFactory::InstantiateComponent(EntityID entity, const ComponentDefinition& componentDef)
{
    const std::string& type = componentDef.componentType;
    
    // Dispatch to appropriate helper based on component type
    if (type == "Identity" || type == "Identity_data")
        return InstantiateIdentity(entity, componentDef);
    else if (type == "Position" || type == "Position_data")
        return InstantiatePosition(entity, componentDef);
    else if (type == "PhysicsBody" || type == "PhysicsBody_data")
        return InstantiatePhysicsBody(entity, componentDef);
    else if (type == "VisualSprite" || type == "VisualSprite_data")
        return InstantiateVisualSprite(entity, componentDef);
    else if (type == "AIBehavior" || type == "AIBehavior_data")
        return InstantiateAIBehavior(entity, componentDef);
    else if (type == "AIBlackboard" || type == "AIBlackboard_data")
        return InstantiateAIBlackboard(entity, componentDef);
    else if (type == "AISenses" || type == "AISenses_data")
        return InstantiateAISenses(entity, componentDef);
    else if (type == "BoundingBox" || type == "BoundingBox_data")
        return InstantiateBoundingBox(entity, componentDef);
    else if (type == "Movement" || type == "Movement_data")
        return InstantiateMovement(entity, componentDef);
    else if (type == "Health" || type == "Health_data")
        return InstantiateHealth(entity, componentDef);
    else if (type == "TriggerZone" || type == "TriggerZone_data")
        return InstantiateTriggerZone(entity, componentDef);
    else if (type == "CollisionZone" || type == "CollisionZone_data")
        return InstantiateCollisionZone(entity, componentDef);
    else if (type == "Animation" || type == "Animation_data")
        return InstantiateAnimation(entity, componentDef);
    else if (type == "FX" || type == "FX_data")
        return InstantiateFX(entity, componentDef);
    else if (type == "AudioSource" || type == "AudioSource_data")
        return InstantiateAudioSource(entity, componentDef);
    else if (type == "Controller" || type == "Controller_data")
        return InstantiateController(entity, componentDef);
    else if (type == "PlayerController" || type == "PlayerController_data")
        return InstantiatePlayerController(entity, componentDef);
    else if (type == "PlayerBinding" || type == "PlayerBinding_data")
        return InstantiatePlayerBinding(entity, componentDef);
    else if (type == "NPC" || type == "NPC_data")
        return InstantiateNPC(entity, componentDef);
    else if (type == "Inventory" || type == "Inventory_data")
        return InstantiateInventory(entity, componentDef);
    else if (type == "Camera" || type == "Camera_data")
        return InstantiateCamera(entity, componentDef);
    else if (type == "CameraTarget" || type == "CameraTarget_data")
        return InstantiateCameraTarget(entity, componentDef);
    else if (type == "CameraEffects" || type == "CameraEffects_data")
        return InstantiateCameraEffects(entity, componentDef);
    else if (type == "CameraBounds" || type == "CameraBounds_data")
        return InstantiateCameraBounds(entity, componentDef);
    else if (type == "CameraInputBinding" || type == "CameraInputBinding_data")
        return InstantiateCameraInputBinding(entity, componentDef);
    else if (type == "InputMapping" || type == "InputMapping_data")
        return InstantiateInputMapping(entity, componentDef);
    else
    {
        SYSTEM_LOG << "PrefabFactory::InstantiateComponent: Unknown component type '" 
                   << type << "'\n";
        return false;
    }
}

// ========================================================================
// Component-specific instantiation helpers
// ========================================================================

bool PrefabFactory::InstantiateIdentity(EntityID entity, const ComponentDefinition& def)
{
    Identity_data identity;
    
    // Extract parameters
    if (def.HasParameter("name"))
        identity.name = def.GetParameter("name")->AsString();
    
    if (def.HasParameter("tag"))
        identity.tag = def.GetParameter("tag")->AsString();
    
    if (def.HasParameter("entityType"))
    {
        // Map string to EntityType enum
        identity.type = def.GetParameter("entityType")->AsString();
    }
    
    World::Get().AddComponent<Identity_data>(entity, identity);
    return true;
}

bool PrefabFactory::InstantiatePosition(EntityID entity, const ComponentDefinition& def)
{
    Position_data position;
    
    // Extract position vector
    if (def.HasParameter("position"))
    {
        position.position = def.GetParameter("position")->AsVector();
    }
    else if (def.HasParameter("x") && def.HasParameter("y"))
    {
        float x = def.GetParameter("x")->AsFloat();
        float y = def.GetParameter("y")->AsFloat();
        float z = def.HasParameter("z") ? def.GetParameter("z")->AsFloat() : 0.0f;
        position.position = Vector(x, y, z);
    }
    
    World::Get().AddComponent<Position_data>(entity, position);
    return true;
}

bool PrefabFactory::InstantiatePhysicsBody(EntityID entity, const ComponentDefinition& def)
{
    PhysicsBody_data physics;
    
    // Extract parameters
    if (def.HasParameter("mass"))
        physics.mass = def.GetParameter("mass")->AsFloat();
    
    if (def.HasParameter("speed"))
        physics.speed = def.GetParameter("speed")->AsFloat();
    
    // Note: friction, restitution, useGravity are not in PhysicsBody_data struct currently
    // These would need to be added to the struct definition in ECS_Components.h
    // For now, we only support mass and speed
    
    World::Get().AddComponent<PhysicsBody_data>(entity, physics);
    return true;
}

bool PrefabFactory::InstantiateVisualSprite(EntityID entity, const ComponentDefinition& def)
{
    VisualSprite_data visual;
    
    // Extract sprite path and load sprite
    if (def.HasParameter("spritePath"))
    {
        std::string spritePath = def.GetParameter("spritePath")->AsString();
        
        // Load sprite via DataManager
        visual.sprite = DataManager::Get().GetSprite(spritePath, spritePath, ResourceCategory::GameEntity);
        
        if (!visual.sprite)
        {
            SYSTEM_LOG << "PrefabFactory::InstantiateVisualSprite: Failed to load sprite '" 
                       << spritePath << "' - component will have null sprite\n";
            // Don't fail completely - create component with null sprite
        }
        else
        {
            // Get texture dimensions for srcRect
            float texW, texH;
            SDL_GetTextureSize(visual.sprite, &texW, &texH);
            
            // Extract optional srcRect
            if (def.HasParameter("srcX") && def.HasParameter("srcY") && 
                def.HasParameter("srcW") && def.HasParameter("srcH"))
            {
                visual.srcRect.x = def.GetParameter("srcX")->AsFloat();
                visual.srcRect.y = def.GetParameter("srcY")->AsFloat();
                visual.srcRect.w = def.GetParameter("srcW")->AsFloat();
                visual.srcRect.h = def.GetParameter("srcH")->AsFloat();
            }
            else
            {
                // Default to full texture
                visual.srcRect = SDL_FRect{0, 0, texW, texH};
            }
            
            // Extract optional hotSpot
            if (def.HasParameter("hotSpot"))
            {
                visual.hotSpot = def.GetParameter("hotSpot")->AsVector();
            }
            else if (def.HasParameter("hotSpotX") && def.HasParameter("hotSpotY"))
            {
                float x = def.GetParameter("hotSpotX")->AsFloat();
                float y = def.GetParameter("hotSpotY")->AsFloat();
                visual.hotSpot = Vector(x, y, 0.0f);
            }
            else
            {
                // Default to center of sprite
                visual.hotSpot = Vector(texW / 2.0f, texH / 2.0f, 0.0f);
            }
        }
    }
    
    // Extract optional color
    if (def.HasParameter("color"))
    {
        visual.color = def.GetParameter("color")->AsColor();
    }
    
    World::Get().AddComponent<VisualSprite_data>(entity, visual);
    return true;
}

bool PrefabFactory::InstantiateAIBehavior(EntityID entity, const ComponentDefinition& def)
{
    AIBehavior_data ai;
    
    // Extract behavior type
    if (def.HasParameter("behaviorType"))
        ai.behaviorType = def.GetParameter("behaviorType")->AsString();
    
    World::Get().AddComponent<AIBehavior_data>(entity, ai);
    return true;
}

bool PrefabFactory::InstantiateAIBlackboard(EntityID entity, const ComponentDefinition& def)
{
    AIBlackboard_data blackboard;
    
    // Extract blackboard parameters
    if (def.HasParameter("targetEntity"))
        blackboard.targetEntity = def.GetParameter("targetEntity")->AsEntityRef();
    
    if (def.HasParameter("hasTarget"))
        blackboard.hasTarget = def.GetParameter("hasTarget")->AsBool();
    
    if (def.HasParameter("attackCooldown"))
        blackboard.attackCooldown = def.GetParameter("attackCooldown")->AsFloat();
    
    // Add support for additional AI parameters
    if (def.HasParameter("distanceToTarget"))
        blackboard.distanceToTarget = def.GetParameter("distanceToTarget")->AsFloat();
    
    if (def.HasParameter("targetVisible"))
        blackboard.targetVisible = def.GetParameter("targetVisible")->AsBool();
    
    if (def.HasParameter("targetInRange"))
        blackboard.targetInRange = def.GetParameter("targetInRange")->AsBool();
    
    World::Get().AddComponent<AIBlackboard_data>(entity, blackboard);
    return true;
}

bool PrefabFactory::InstantiateAISenses(EntityID entity, const ComponentDefinition& def)
{
    AISenses_data senses;
    
    // Extract senses parameters
    if (def.HasParameter("visionRadius"))
        senses.visionRadius = def.GetParameter("visionRadius")->AsFloat();
    else if (def.HasParameter("visionRange"))
        senses.visionRadius = def.GetParameter("visionRange")->AsFloat();
    
    if (def.HasParameter("visionAngle"))
        senses.visionAngle = def.GetParameter("visionAngle")->AsFloat();
    
    if (def.HasParameter("hearingRadius"))
        senses.hearingRadius = def.GetParameter("hearingRadius")->AsFloat();
    else if (def.HasParameter("hearingRange"))
        senses.hearingRadius = def.GetParameter("hearingRange")->AsFloat();
    
    if (def.HasParameter("perceptionHz"))
        senses.perceptionHz = def.GetParameter("perceptionHz")->AsFloat();
    
    if (def.HasParameter("thinkHz"))
        senses.thinkHz = def.GetParameter("thinkHz")->AsFloat();
    
    World::Get().AddComponent<AISenses_data>(entity, senses);
    return true;
}

bool PrefabFactory::InstantiateBoundingBox(EntityID entity, const ComponentDefinition& def)
{
    BoundingBox_data bbox;
    
    // Extract width and height
    if (def.HasParameter("width") && def.HasParameter("height"))
    {
        bbox.boundingBox.w = def.GetParameter("width")->AsFloat();
        bbox.boundingBox.h = def.GetParameter("height")->AsFloat();
        
        // Optional x, y offset
        if (def.HasParameter("x"))
            bbox.boundingBox.x = def.GetParameter("x")->AsFloat();
        if (def.HasParameter("y"))
            bbox.boundingBox.y = def.GetParameter("y")->AsFloat();
        
        // Optional offsetX, offsetY (alternative to x,y)
        if (def.HasParameter("offsetX"))
            bbox.boundingBox.x = def.GetParameter("offsetX")->AsFloat();
        if (def.HasParameter("offsetY"))
            bbox.boundingBox.y = def.GetParameter("offsetY")->AsFloat();
    }
    
    World::Get().AddComponent<BoundingBox_data>(entity, bbox);
    return true;
}

bool PrefabFactory::InstantiateMovement(EntityID entity, const ComponentDefinition& def)
{
    Movement_data movement;
    
    // Extract direction
    if (def.HasParameter("direction"))
    {
        movement.direction = def.GetParameter("direction")->AsVector();
    }
    else if (def.HasParameter("directionX") && def.HasParameter("directionY"))
    {
        float x = def.GetParameter("directionX")->AsFloat();
        float y = def.GetParameter("directionY")->AsFloat();
        movement.direction = Vector(x, y, 0.0f);
    }
    
    // Extract velocity
    if (def.HasParameter("velocity"))
    {
        movement.velocity = def.GetParameter("velocity")->AsVector();
    }
    else if (def.HasParameter("velocityX") && def.HasParameter("velocityY"))
    {
        float x = def.GetParameter("velocityX")->AsFloat();
        float y = def.GetParameter("velocityY")->AsFloat();
        movement.velocity = Vector(x, y, 0.0f);
    }
    
    World::Get().AddComponent<Movement_data>(entity, movement);
    return true;
}

bool PrefabFactory::InstantiateHealth(EntityID entity, const ComponentDefinition& def)
{
    Health_data health;
    
    // Extract health parameters
    if (def.HasParameter("currentHealth"))
        health.currentHealth = def.GetParameter("currentHealth")->AsInt();
    
    if (def.HasParameter("maxHealth"))
        health.maxHealth = def.GetParameter("maxHealth")->AsInt();
    
    World::Get().AddComponent<Health_data>(entity, health);
    return true;
}

bool PrefabFactory::InstantiateTriggerZone(EntityID entity, const ComponentDefinition& def)
{
    TriggerZone_data trigger;
    
    // Extract radius
    if (def.HasParameter("radius"))
        trigger.radius = def.GetParameter("radius")->AsFloat();
    
    if (def.HasParameter("triggered"))
        trigger.triggered = def.GetParameter("triggered")->AsBool();
    
    World::Get().AddComponent<TriggerZone_data>(entity, trigger);
    return true;
}

bool PrefabFactory::InstantiateCollisionZone(EntityID entity, const ComponentDefinition& def)
{
    CollisionZone_data collision;
    
    // Extract bounds
    if (def.HasParameter("x") && def.HasParameter("y") && 
        def.HasParameter("width") && def.HasParameter("height"))
    {
        collision.bounds.x = def.GetParameter("x")->AsFloat();
        collision.bounds.y = def.GetParameter("y")->AsFloat();
        collision.bounds.w = def.GetParameter("width")->AsFloat();
        collision.bounds.h = def.GetParameter("height")->AsFloat();
    }
    
    if (def.HasParameter("isStatic"))
        collision.isStatic = def.GetParameter("isStatic")->AsBool();
    
    World::Get().AddComponent<CollisionZone_data>(entity, collision);
    return true;
}

bool PrefabFactory::InstantiateAnimation(EntityID entity, const ComponentDefinition& def)
{
    Animation_data animation;
    
    // Extract animation parameters
    if (def.HasParameter("animationID"))
        animation.animationID = def.GetParameter("animationID")->AsString();
    
    if (def.HasParameter("frameDuration"))
        animation.frameDuration = def.GetParameter("frameDuration")->AsFloat();
    
    if (def.HasParameter("currentFrame"))
        animation.currentFrame = def.GetParameter("currentFrame")->AsInt();
    
    World::Get().AddComponent<Animation_data>(entity, animation);
    return true;
}

bool PrefabFactory::InstantiateFX(EntityID entity, const ComponentDefinition& def)
{
    FX_data fx;
    
    // Extract FX parameters
    if (def.HasParameter("effectType"))
        fx.effectType = def.GetParameter("effectType")->AsString();
    
    if (def.HasParameter("duration"))
        fx.duration = def.GetParameter("duration")->AsFloat();
    
    World::Get().AddComponent<FX_data>(entity, fx);
    return true;
}

bool PrefabFactory::InstantiateAudioSource(EntityID entity, const ComponentDefinition& def)
{
    AudioSource_data audio;
    
    // Extract audio parameters
    if (def.HasParameter("soundEffectID"))
        audio.soundEffectID = def.GetParameter("soundEffectID")->AsString();
    
    if (def.HasParameter("volume"))
        audio.volume = def.GetParameter("volume")->AsFloat();
    
    World::Get().AddComponent<AudioSource_data>(entity, audio);
    return true;
}

bool PrefabFactory::InstantiateController(EntityID entity, const ComponentDefinition& def)
{
    Controller_data controller;
    
    // Extract controller parameters
    if (def.HasParameter("controllerID"))
        controller.controllerID = static_cast<short>(def.GetParameter("controllerID")->AsInt());
    
    if (def.HasParameter("isConnected"))
        controller.isConnected = def.GetParameter("isConnected")->AsBool();
    
    World::Get().AddComponent<Controller_data>(entity, controller);
    return true;
}

bool PrefabFactory::InstantiatePlayerController(EntityID entity, const ComponentDefinition& def)
{
    PlayerController_data playerCtrl;
    
    // Extract player controller parameters
    if (def.HasParameter("isJumping"))
        playerCtrl.isJumping = def.GetParameter("isJumping")->AsBool();
    
    if (def.HasParameter("isShooting"))
        playerCtrl.isShooting = def.GetParameter("isShooting")->AsBool();
    
    if (def.HasParameter("isRunning"))
        playerCtrl.isRunning = def.GetParameter("isRunning")->AsBool();
    
    World::Get().AddComponent<PlayerController_data>(entity, playerCtrl);
    return true;
}

bool PrefabFactory::InstantiatePlayerBinding(EntityID entity, const ComponentDefinition& def)
{
    PlayerBinding_data binding;
    
    // Extract binding parameters
    if (def.HasParameter("playerIndex"))
        binding.playerIndex = static_cast<short>(def.GetParameter("playerIndex")->AsInt());
    
    if (def.HasParameter("controllerID"))
        binding.controllerID = static_cast<short>(def.GetParameter("controllerID")->AsInt());
    
    World::Get().AddComponent<PlayerBinding_data>(entity, binding);
    return true;
}

bool PrefabFactory::InstantiateNPC(EntityID entity, const ComponentDefinition& def)
{
    NPC_data npc;
    
    // Extract NPC type
    if (def.HasParameter("npcType"))
        npc.npcType = def.GetParameter("npcType")->AsString();
    
    World::Get().AddComponent<NPC_data>(entity, npc);
    return true;
}

bool PrefabFactory::InstantiateInventory(EntityID entity, const ComponentDefinition& def)
{
    Inventory_data inventory;
    
    // Note: Items are typically added dynamically during gameplay
    // We don't extract items from the definition here
    
    World::Get().AddComponent<Inventory_data>(entity, inventory);
    return true;
}

bool PrefabFactory::InstantiateCamera(EntityID entity, const ComponentDefinition& def)
{
    Camera_data camera;
    
    // Extract camera parameters
    if (def.HasParameter("playerId"))
        camera.playerId = static_cast<short>(def.GetParameter("playerId")->AsInt());
    
    if (def.HasParameter("zoom"))
        camera.zoom = def.GetParameter("zoom")->AsFloat();
    
    if (def.HasParameter("targetZoom"))
        camera.targetZoom = def.GetParameter("targetZoom")->AsFloat();
    
    if (def.HasParameter("position"))
        camera.position = def.GetParameter("position")->AsVector();
    
    World::Get().AddComponent<Camera_data>(entity, camera);
    return true;
}

bool PrefabFactory::InstantiateCameraTarget(EntityID entity, const ComponentDefinition& def)
{
    CameraTarget_data target;
    
    // Extract target parameters
    if (def.HasParameter("targetEntityID"))
        target.targetEntityID = def.GetParameter("targetEntityID")->AsEntityRef();
    
    if (def.HasParameter("followTarget"))
        target.followTarget = def.GetParameter("followTarget")->AsBool();
    
    if (def.HasParameter("smoothFactor"))
        target.smoothFactor = def.GetParameter("smoothFactor")->AsFloat();
    
    World::Get().AddComponent<CameraTarget_data>(entity, target);
    return true;
}

bool PrefabFactory::InstantiateCameraEffects(EntityID entity, const ComponentDefinition& def)
{
    CameraEffects_data effects;
    
    // Extract effects parameters
    if (def.HasParameter("isShaking"))
        effects.isShaking = def.GetParameter("isShaking")->AsBool();
    
    if (def.HasParameter("shakeIntensity"))
        effects.shakeIntensity = def.GetParameter("shakeIntensity")->AsFloat();
    
    if (def.HasParameter("shakeDuration"))
        effects.shakeDuration = def.GetParameter("shakeDuration")->AsFloat();
    
    World::Get().AddComponent<CameraEffects_data>(entity, effects);
    return true;
}

bool PrefabFactory::InstantiateCameraBounds(EntityID entity, const ComponentDefinition& def)
{
    CameraBounds_data bounds;
    
    // Extract bounds parameters
    if (def.HasParameter("useBounds"))
        bounds.useBounds = def.GetParameter("useBounds")->AsBool();
    
    if (def.HasParameter("x") && def.HasParameter("y") && 
        def.HasParameter("width") && def.HasParameter("height"))
    {
        bounds.boundingBox.x = def.GetParameter("x")->AsFloat();
        bounds.boundingBox.y = def.GetParameter("y")->AsFloat();
        bounds.boundingBox.w = def.GetParameter("width")->AsFloat();
        bounds.boundingBox.h = def.GetParameter("height")->AsFloat();
    }
    
    World::Get().AddComponent<CameraBounds_data>(entity, bounds);
    return true;
}

bool PrefabFactory::InstantiateCameraInputBinding(EntityID entity, const ComponentDefinition& def)
{
    CameraInputBinding_data binding;
    
    // Extract input binding parameters
    if (def.HasParameter("playerId"))
        binding.playerId = static_cast<short>(def.GetParameter("playerId")->AsInt());
    
    if (def.HasParameter("useKeyboard"))
        binding.useKeyboard = def.GetParameter("useKeyboard")->AsBool();
    
    if (def.HasParameter("deadzone"))
        binding.deadzone = def.GetParameter("deadzone")->AsFloat();
    
    World::Get().AddComponent<CameraInputBinding_data>(entity, binding);
    return true;
}

bool PrefabFactory::InstantiateInputMapping(EntityID entity, const ComponentDefinition& def)
{
    InputMapping_data mapping;
    
    // Initialize with defaults
    mapping.InitializeDefaults();
    
    // Extract custom parameters if provided
    if (def.HasParameter("deadzone"))
        mapping.deadzone = def.GetParameter("deadzone")->AsFloat();
    
    if (def.HasParameter("sensitivity"))
        mapping.sensitivity = def.GetParameter("sensitivity")->AsFloat();
    
    World::Get().AddComponent<InputMapping_data>(entity, mapping);
    return true;
}
