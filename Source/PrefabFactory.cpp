/*
	Olympe Engine V2 - 2025
	Nicolas Chereau
	nchereau@gmail.com

	Purpose:
	- Implementation file for PrefabFactory class.
	- Component-agnostic instantiation system.
*/

#include "prefabfactory.h"
#include "ComponentRegistry.h"
#include "ComponentDefinition.h"
#include "PrefabScanner.h"
#include "ParameterResolver.h"
#include "World.h"
#include "DataManager.h"
#include "ECS_Components.h"
#include "ECS_Components_AI.h"
#include "ECS_Components_Camera.h"
#include "system/system_utils.h"
#include "AI/BehaviorTree.h"
#include <string>
#include <unordered_map>
#include "VideoGame.h"

// ========================================================================
// Component Factory Registry Implementation
// ========================================================================

void PrefabFactory::RegisterComponentFactory(const std::string& componentName, 
                                              std::function<bool(EntityID, const ComponentDefinition&)> factory)
{
    // Check if component is already registered (prevents duplicate registrations from multiple translation units)
    if (m_componentFactories.find(componentName) != m_componentFactories.end())
    {
        // Already registered, silently ignore duplicate
        return;
    }
    
    // First registration: store factory and log
    m_componentFactories[componentName] = factory;
    std::cout << "[ComponentRegistry] Registered: " << componentName << "\n";
}

bool PrefabFactory::IsComponentRegistered(const std::string& componentName) const
{
    return m_componentFactories.find(componentName) != m_componentFactories.end();
}

std::vector<std::string> PrefabFactory::GetRegisteredComponents() const
{
    std::vector<std::string> components;
    components.reserve(m_componentFactories.size());
    for (const auto& kv : m_componentFactories)
    {
        components.push_back(kv.first);
    }
    return components;
}

// Helper function for macro
void RegisterComponentFactory_Internal(const char* componentName, 
                                       std::function<bool(EntityID, const ComponentDefinition&)> factory)
{
    PrefabFactory::Get().RegisterComponentFactory(componentName, factory);
}

// ========================================================================
// Public API Implementation
// ========================================================================

void PrefabFactory::PreloadAllPrefabs(const std::string& prefabDirectory)
{
    if (m_prefabsPreloaded)
    {
        SYSTEM_LOG << "PrefabFactory::PreloadAllPrefabs: Already preloaded\n";
        return;
    }
    
    SYSTEM_LOG << "+===========================================================+\n";
    SYSTEM_LOG << "| PREFAB FACTORY: PRELOADING ALL PREFABS                   |\n";
    SYSTEM_LOG << "+===========================================================+\n";
    
    // Use new unified Initialize method from PrefabScanner
    PrefabScanner scanner;
    m_prefabRegistry = scanner.Initialize(prefabDirectory);
    
    // Store scanner for type normalization
    m_scanner = std::make_unique<PrefabScanner>(std::move(scanner));
    
    int prefabCount = static_cast<int>(m_prefabRegistry.GetCount());
    
    SYSTEM_LOG << "ok - Loaded " << prefabCount << " prefabs:\n";
    
    auto allNames = m_prefabRegistry.GetAllPrefabNames();
    for (const auto& name : allNames)
    {
        const PrefabBlueprint* bp = m_prefabRegistry.Find(name);
        if (bp && bp->isValid)
        {
            SYSTEM_LOG << "   +- " << name << " (" << bp->components.size() << " components)\n";
        }
    }
    
    SYSTEM_LOG << "\nok - PrefabFactory ready\n\n";
    m_prefabsPreloaded = true;
}

EntityID PrefabFactory::CreateEntityFromPrefabName(const std::string& prefabName)
{
    if (!m_prefabsPreloaded)
    {
        SYSTEM_LOG << "/!\\  PrefabFactory: Prefabs not preloaded! Call PreloadAllPrefabs() first\n";
        return INVALID_ENTITY_ID;
    }
    
    const PrefabBlueprint* blueprint = m_prefabRegistry.Find(prefabName);
    
    if (!blueprint || !blueprint->isValid)
    {
        SYSTEM_LOG << "x PrefabFactory: Prefab '" << prefabName << "' not found\n";
        return INVALID_ENTITY_ID;
    }
    
    return CreateEntityFromBlueprint(*blueprint);
}

EntityID PrefabFactory::CreateEntity(const std::string& prefabName)
{
    // Try legacy system first
    if (m_prefabs.find(prefabName) != m_prefabs.end())
    {
        EntityID newEntity = World::Get().CreateEntity();
        m_prefabs[prefabName](newEntity);
        SYSTEM_LOG << "PrefabFactory::CreateEntity '" << prefabName << "' created (ID: " << newEntity << ")\n";
        return newEntity;
    }

    // Fallback to modern system
    return CreateEntityFromPrefabName(prefabName);
}

void PrefabFactory::SetPrefabRegistry(const PrefabRegistry& registry)
{
    m_prefabRegistry = registry;
    SYSTEM_LOG << "PrefabFactory: Registry cached with " << registry.GetCount() << " prefabs\n";
    
    // Mark as preloaded if registry is non-empty
    if (registry.GetCount() > 0)
    {
        m_prefabsPreloaded = true;
    }
}

std::string PrefabFactory::NormalizeType(const std::string& type) const
{
    if (m_scanner)
    {
        return m_scanner->NormalizeType(type);
    }
    return type;
}

bool PrefabFactory::AreTypesEquivalent(const std::string& type1, const std::string& type2) const
{
    if (m_scanner)
    {
        return m_scanner->AreTypesEquivalent(type1, type2);
    }
    return type1 == type2;
}

bool PrefabFactory::IsTypeRegistered(const std::string& type) const
{
    if (m_scanner)
    {
        return m_scanner->IsTypeRegistered(type);
    }
    return false;
}

bool PrefabFactory::GetCanonicalInfo(const std::string& type, std::string& outCanonical, 
                                      std::string& outPrefabFile) const
{
    if (m_scanner)
    {
        return m_scanner->GetCanonicalInfo(type, outCanonical, outPrefabFile);
    }
    return false;
}

EntityID PrefabFactory::CreateEntityFromBlueprint(const PrefabBlueprint& blueprint, bool autoAssignLayer)
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
    
    // -> Auto-assign render layer based on entity type (if requested)
    if (autoAssignLayer && 
        world.HasComponent<Identity_data>(entity) && 
        world.HasComponent<Position_data>(entity))
    {
        const Identity_data& identity = world.GetComponent<Identity_data>(entity);
        RenderLayer defaultLayer = world.GetDefaultLayerForType(identity.entityType);
        world.SetEntityLayer(entity, defaultLayer);

    }
       
    return entity;
}

EntityID PrefabFactory::CreateEntityWithOverrides(
    const PrefabBlueprint& blueprint,
    const LevelInstanceParameters& instanceParams,
    bool autoAssignLayer)
{
    if (!blueprint.isValid)
    {
        SYSTEM_LOG << "PrefabFactory::CreateEntityWithOverrides: Invalid blueprint '" 
                   << blueprint.prefabName << "'\n";
        return INVALID_ENTITY_ID;
    }
    
    // Create entity
    World& world = World::Get();
    EntityID entity = world.CreateEntity();
    
    if (entity == INVALID_ENTITY_ID)
    {
        SYSTEM_LOG << "PrefabFactory::CreateEntityWithOverrides: Failed to create entity for '" 
                   << blueprint.prefabName << "'\n";
        return INVALID_ENTITY_ID;
    }
    
    // Use ParameterResolver to merge prefab defaults with instance parameters
    ParameterResolver resolver;
    std::vector<ResolvedComponentInstance> resolvedComponents = resolver.Resolve(blueprint, instanceParams);
    
    // Instantiate components with resolved parameters
    // NOTE: If some components fail to instantiate, the entity is still returned with partial state.
    // This is intentional - allows entities to be created even if some optional components fail.
    // Callers should check component existence before accessing them.
    int successCount = 0;
    int failCount = 0;
    
    for (const auto& resolved : resolvedComponents)
    {
        if (!resolved.isValid)
        {
            failCount++;
            SYSTEM_LOG << "    /!\\  Invalid resolved component: " << resolved.componentType << "\n";
            continue;
        }
        
        ComponentDefinition compDef;
        compDef.componentType = resolved.componentType;
        compDef.parameters = resolved.parameters;
        
        if (InstantiateComponent(entity, compDef))
        {
            successCount++;
        }
        else
        {
            failCount++;
            SYSTEM_LOG << "    /!\\  Failed to instantiate component: " << resolved.componentType << "\n";
        }
    }
    
    // Override position INCLUDING z component (zOrder) to preserve layer depth
    if (world.HasComponent<Position_data>(entity))
    {
        auto& pos = world.GetComponent<Position_data>(entity);
        pos.position = instanceParams.position;
    }
    
    // -> Auto-assign render layer based on entity type (if requested)
    if (autoAssignLayer && 
        world.HasComponent<Identity_data>(entity) && 
        world.HasComponent<Position_data>(entity))
    {
        const Identity_data& identity = world.GetComponent<Identity_data>(entity);
        RenderLayer defaultLayer = world.GetDefaultLayerForType(identity.entityType);
        world.SetEntityLayer(entity, defaultLayer);
    }
    
    SYSTEM_LOG << "    -> Created with " << successCount << " components";
    if (failCount > 0)
    {
        SYSTEM_LOG << " (" << failCount << " failed)";
    }
    SYSTEM_LOG << "\n";
    
    return entity;
}

bool PrefabFactory::InstantiateComponent(EntityID entity, const ComponentDefinition& componentDef)
{
    const std::string& type = componentDef.componentType;
    
    // Step 1: Try auto-registered components first
    auto it = m_componentFactories.find(type);
    if (it != m_componentFactories.end())
    {
        // Call the registered factory function to create component
        bool success = it->second(entity, componentDef);
        if (!success)
        {
            return false;
        }
        
        // For components that need specialized parameter handling, call the specialized function
        // Note: The specialized function applies parameters but does NOT recreate the component
        // This if-else chain is intentional for clarity and ease of modification.
        // Performance impact is negligible as this only runs once per component during entity creation.
        if (type == "BehaviorTreeRuntime_data")
            return InstantiateBehaviorTreeRuntime(entity, componentDef);
        else if (type == "Position_data")
            return InstantiatePosition(entity, componentDef);
        else if (type == "Identity_data")
            return InstantiateIdentity(entity, componentDef);
        else if (type == "PhysicsBody_data")
            return InstantiatePhysicsBody(entity, componentDef);
        else if (type == "VisualSprite_data")
            return InstantiateVisualSprite(entity, componentDef);
        else if (type == "AIBlackboard_data")
            return InstantiateAIBlackboard(entity, componentDef);
        else if (type == "AISenses_data")
            return InstantiateAISenses(entity, componentDef);
        else if (type == "MoveIntent_data")
            return InstantiateMoveIntent(entity, componentDef);
        
        // Component created successfully with default values
        // too soon return true;
    }

        SYSTEM_LOG << "[WARNING] PrefabFactory::InstantiateComponent: No factory registered for component type '" << type << "'\n";
        // Step 2: Fallback to legacy specialized functions (for backward compatibility)
        // This allows the system to work even if AUTO_REGISTER_COMPONENT was forgotten
        if (type == "Identity" || type == "Identity_data")
            return InstantiateIdentity(entity, componentDef);
        else if (type == "Position" || type == "Position_data")
            return InstantiatePosition(entity, componentDef);
        else if (type == "PhysicsBody" || type == "PhysicsBody_data")
            return InstantiatePhysicsBody(entity, componentDef);
        else if (type == "VisualSprite" || type == "VisualSprite_data")
            return InstantiateVisualSprite(entity, componentDef);
        else if (type == "VisualEditor" || type == "VisualEditor_data")
            return InstantiateVisualEditor(entity, componentDef);
        else if (type == "AIBehavior" || type == "AIBehavior_data")
            return InstantiateAIBehavior(entity, componentDef);
        else if (type == "AIBlackboard" || type == "AIBlackboard_data")
            return InstantiateAIBlackboard(entity, componentDef);
        else if (type == "AISenses" || type == "AISenses_data")
            return InstantiateAISenses(entity, componentDef);
        else if (type == "AIState" || type == "AIState_data")
            return InstantiateAIState(entity, componentDef);
        else if (type == "BehaviorTreeRuntime" || type == "BehaviorTreeRuntime_data")
            return InstantiateBehaviorTreeRuntime(entity, componentDef);
        else if (type == "MoveIntent" || type == "MoveIntent_data")
            return InstantiateMoveIntent(entity, componentDef);
        else if (type == "AttackIntent" || type == "AttackIntent_data")
            return InstantiateAttackIntent(entity, componentDef);
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
            SYSTEM_LOG << "  Available auto-registered components:\n";
            for (const auto& kv : m_componentFactories)
            {
                SYSTEM_LOG << "    - " << kv.first << "\n";
            }
            return false;
        }
    
}

// ========================================================================
// Component-specific instantiation helpers
// ========================================================================

// Helper function to convert string to EntityType enum
static EntityType StringToEntityType(const std::string& typeStr)
{
    static const std::unordered_map<std::string, EntityType> typeMap = {
        {"Player", EntityType::Player},
        {"NPC", EntityType::NPC},
        {"Enemy", EntityType::Enemy},
        {"Item", EntityType::Item},
        {"Collectible", EntityType::Collectible},
        {"Effect", EntityType::Effect},
        {"Particle", EntityType::Particle},
        {"UIElement", EntityType::UIElement},
        {"Background", EntityType::Background},
        {"Trigger", EntityType::Trigger},
        {"Waypoint", EntityType::Waypoint},
        {"Static", EntityType::Static},
        {"Dynamic", EntityType::Dynamic}
    };
    
    auto it = typeMap.find(typeStr);
    return (it != typeMap.end()) ? it->second : EntityType::None;
}

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
        // Store string type for backward compatibility
        identity.type = def.GetParameter("entityType")->AsString();
        
        // Map string to EntityType enum using helper function
        identity.entityType = StringToEntityType(identity.type);
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
    
    // REQUIREMENT E: Apply friction, useGravity, and rotation when present
    if (def.HasParameter("friction"))
        physics.friction = def.GetParameter("friction")->AsFloat();
    
    if (def.HasParameter("useGravity"))
        physics.useGravity = def.GetParameter("useGravity")->AsBool();
    
    if (def.HasParameter("rotation"))
        physics.rotation = def.GetParameter("rotation")->AsFloat();
    
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
    
    // REQUIREMENT E: Apply visible parameter when explicitly provided
    if (def.HasParameter("visible"))
    {
        visual.visible = def.GetParameter("visible")->AsBool();
    }
    
    // NOTE: width/height/layer fields don't exist in VisualSprite_data struct yet
    // These parameters are validated by schema but not applied until struct is updated
    // For now, srcRect.w and srcRect.h serve as the effective width/height
    
    World::Get().AddComponent<VisualSprite_data>(entity, visual);
    return true;
}

bool PrefabFactory::InstantiateVisualEditor(EntityID entity, const ComponentDefinition& def)
{
    VisualEditor_data editor;
    
    // Extract sprite path and load sprite
    if (def.HasParameter("spritePath"))
    {
        std::string spritePath = def.GetParameter("spritePath")->AsString();
        
        // Load sprite via DataManager
        editor.sprite = DataManager::Get().GetSprite(spritePath, spritePath, ResourceCategory::GameEntity);
        
        if (!editor.sprite)
        {
            SYSTEM_LOG << "PrefabFactory::InstantiateVisualEditor: Failed to load sprite '" 
                       << spritePath << "' - component will have null sprite\n";
            // Don't fail completely - create component with null sprite
        }
        else
        {
            // Get texture dimensions for srcRect
            float texW, texH;
            SDL_GetTextureSize(editor.sprite, &texW, &texH);
            
            // Set srcRect to full texture dimensions
            editor.srcRect = SDL_FRect{0, 0, texW, texH};
            
            // Set hotSpot to center of sprite
            editor.hotSpot = Vector(texW / 2.0f, texH / 2.0f, 0.0f);
        }
    }
    
    // Extract optional color
    if (def.HasParameter("color"))
    {
        editor.color = def.GetParameter("color")->AsColor();
    }
    else
    {
        // Default to white
        editor.color = SDL_Color{255, 255, 255, 255};
    }
    
    // Extract optional visibility flag
    if (def.HasParameter("visible"))
    {
        editor.isVisible = def.GetParameter("visible")->AsBool();
    }
    else
    {
        // Default to visible
        editor.isVisible = true;
    }
    
    // REQUIREMENT E: Apply layer when explicitly provided (if VisualEditor_data had the field)
    // NOTE: width/height/layer fields don't exist in VisualEditor_data struct yet
    // These parameters are validated by schema but not applied until struct is updated
    
    World::Get().AddComponent<VisualEditor_data>(entity, editor);
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
    else
    {
        controller.controllerID = -1;
    }

    
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

    if (def.HasParameter("isInteracting"))
        playerCtrl.isInteracting = def.GetParameter("isInteracting")->AsBool();

    if (def.HasParameter("isWalking"))
        playerCtrl.isWalking = def.GetParameter("isWalking")->AsBool();
    
    if (def.HasParameter("isUsingItem"))
        playerCtrl.isUsingItem = def.GetParameter("isUsingItem")->AsBool();

    if (def.HasParameter("isMenuOpen"))
        playerCtrl.isMenuOpen = def.GetParameter("isMenuOpen")->AsBool();

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

bool PrefabFactory::InstantiateAIState(EntityID entity, const ComponentDefinition& def)
{
    AIState_data aiState;
    
    // Extract AI state parameters
    if (def.HasParameter("currentState"))
    {
        std::string stateStr = def.GetParameter("currentState")->AsString();
        // Convert string to AIMode enum
        if (stateStr == "Idle") aiState.currentMode = AIMode::Idle;
        else if (stateStr == "Patrol") aiState.currentMode = AIMode::Patrol;
        else if (stateStr == "Combat") aiState.currentMode = AIMode::Combat;
        else if (stateStr == "Flee") aiState.currentMode = AIMode::Flee;
        else if (stateStr == "Investigate") aiState.currentMode = AIMode::Investigate;
        else if (stateStr == "Dead") aiState.currentMode = AIMode::Dead;
        else {
            SYSTEM_LOG << "PrefabFactory::InstantiateAIState: Warning - Unknown state '" 
                       << stateStr << "', defaulting to Idle\n";
        }
    }
    
    if (def.HasParameter("previousState"))
    {
        std::string stateStr = def.GetParameter("previousState")->AsString();
        if (stateStr == "Idle") aiState.previousMode = AIMode::Idle;
        else if (stateStr == "Patrol") aiState.previousMode = AIMode::Patrol;
        else if (stateStr == "Combat") aiState.previousMode = AIMode::Combat;
        else if (stateStr == "Flee") aiState.previousMode = AIMode::Flee;
        else if (stateStr == "Investigate") aiState.previousMode = AIMode::Investigate;
        else if (stateStr == "Dead") aiState.previousMode = AIMode::Dead;
        else {
            SYSTEM_LOG << "PrefabFactory::InstantiateAIState: Warning - Unknown previousState '" 
                       << stateStr << "', defaulting to Idle\n";
        }
    }
    
    if (def.HasParameter("combatEngageDistance"))
        aiState.combatEngageDistance = def.GetParameter("combatEngageDistance")->AsFloat();
    
    if (def.HasParameter("fleeHealthThreshold"))
        aiState.fleeHealthThreshold = def.GetParameter("fleeHealthThreshold")->AsFloat();
    
    if (def.HasParameter("investigateTimeout"))
        aiState.investigateTimeout = def.GetParameter("investigateTimeout")->AsFloat();
    
    World::Get().AddComponent<AIState_data>(entity, aiState);
    return true;
}

bool PrefabFactory::InstantiateBehaviorTreeRuntime(EntityID entity, const ComponentDefinition& def)
{
    BehaviorTreeRuntime_data btRuntime;
    
    // Extract behavior tree runtime parameters
    if (def.HasParameter("treeAssetId"))
        btRuntime.treeAssetId = static_cast<uint32_t>(def.GetParameter("treeAssetId")->AsInt());
    
    if (def.HasParameter("treePath"))
    {
        Identity_data* identity = nullptr;
		if (World::Get().HasComponent<Identity_data>(entity))
            identity = &World::Get().GetComponent<Identity_data>(entity);

        // Map treePath -> treeId using the registry
        std::string treePath = def.GetParameter("treePath")->AsString();
        btRuntime.treePath = treePath; 

        if (!treePath.empty())
        {
            uint32_t treeId = BehaviorTreeManager::Get().GetTreeIdFromPath(treePath);
            btRuntime.treeAssetId = treeId;
			if (identity != nullptr)
                std::cout << "[PrefabFactory] Mapped BehaviorTree: " << treePath << " -> ID " << treeId << " for entity " << identity->name << "\n";
            else
				std::cout << "[PrefabFactory] Mapped BehaviorTree: " << treePath << " -> ID " << treeId << " for entity " << entity << "\n";
            
            // Verify the tree is loaded
            const BehaviorTreeAsset* tree = BehaviorTreeManager::Get().GetTree(treeId);
            if (!tree)
            {
                std::cerr << "[PrefabFactory] WARNING: BehaviorTree not loaded: " << treePath 
                          << " (ID=" << treeId << ") - this should not happen if dependencies were loaded correctly\n";
            }
        }
    }
    
    if (def.HasParameter("active"))
        btRuntime.isActive = def.GetParameter("active")->AsBool();
    
    if (def.HasParameter("currentNodeIndex"))
        btRuntime.currentNodeIndex = static_cast<uint32_t>(def.GetParameter("currentNodeIndex")->AsInt());
    
    World::Get().AddComponent<BehaviorTreeRuntime_data>(entity, btRuntime);
    return true;
}

bool PrefabFactory::InstantiateMoveIntent(EntityID entity, const ComponentDefinition& def)
{
    MoveIntent_data moveIntent;
    
    // Extract move intent parameters
    if (def.HasParameter("targetX") && def.HasParameter("targetY"))
    {
        float x = def.GetParameter("targetX")->AsFloat();
        float y = def.GetParameter("targetY")->AsFloat();
        moveIntent.targetPosition = Vector(x, y, 0.0f);
    }
    
    if (def.HasParameter("targetPosition"))
        moveIntent.targetPosition = def.GetParameter("targetPosition")->AsVector();
    
    if (def.HasParameter("desiredSpeed"))
        moveIntent.desiredSpeed = def.GetParameter("desiredSpeed")->AsFloat();
    
    if (def.HasParameter("hasTarget"))
        moveIntent.hasIntent = def.GetParameter("hasTarget")->AsBool();
    
    if (def.HasParameter("hasIntent"))
        moveIntent.hasIntent = def.GetParameter("hasIntent")->AsBool();
    
    if (def.HasParameter("arrivalThreshold"))
        moveIntent.arrivalThreshold = def.GetParameter("arrivalThreshold")->AsFloat();
    
    if (def.HasParameter("usePathfinding"))
        moveIntent.usePathfinding = def.GetParameter("usePathfinding")->AsBool();
    
    if (def.HasParameter("avoidObstacles"))
        moveIntent.avoidObstacles = def.GetParameter("avoidObstacles")->AsBool();
    
    World::Get().AddComponent<MoveIntent_data>(entity, moveIntent);
    return true;
}

bool PrefabFactory::InstantiateAttackIntent(EntityID entity, const ComponentDefinition& def)
{
    AttackIntent_data attackIntent;
    
    // Extract attack intent parameters
    if (def.HasParameter("targetEntity"))
        attackIntent.targetEntity = def.GetParameter("targetEntity")->AsEntityRef();
    
    if (def.HasParameter("targetPosition"))
        attackIntent.targetPosition = def.GetParameter("targetPosition")->AsVector();
    
    if (def.HasParameter("damage"))
        attackIntent.damage = def.GetParameter("damage")->AsFloat();
    
    if (def.HasParameter("range"))
        attackIntent.range = def.GetParameter("range")->AsFloat();
    
    if (def.HasParameter("attackRange"))
        attackIntent.range = def.GetParameter("attackRange")->AsFloat();
    
    if (def.HasParameter("hasIntent"))
        attackIntent.hasIntent = def.GetParameter("hasIntent")->AsBool();
    
    if (def.HasParameter("cooldown"))
        attackIntent.cooldown = def.GetParameter("cooldown")->AsFloat();

    if (def.HasParameter("attackType"))
    {
        std::string typeStr = def.GetParameter("attackType")->AsString();
        if (typeStr == "Melee") attackIntent.attackType = AttackIntent_data::AttackType::Melee;
        else if (typeStr == "Ranged") attackIntent.attackType = AttackIntent_data::AttackType::Ranged;
        else if (typeStr == "Area") attackIntent.attackType = AttackIntent_data::AttackType::Area;
        else {
            SYSTEM_LOG << "PrefabFactory::InstantiateAttackIntent: Warning - Unknown attackType '" 
                       << typeStr << "', defaulting to Melee\n";
        }
    }
    
    World::Get().AddComponent<AttackIntent_data>(entity, attackIntent);
    return true;
}
