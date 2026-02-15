/**
 * @file ECS_Systems_Animation.cpp
 * @brief Implementation of AnimationSystem
 * @author Nicolas Chereau
 * @date 2025
 */

#include "ECS_Systems_Animation.h"
#include "World.h"
#include "GameEngine.h"
#include "Animation/AnimationManager.h"
#include "Animation/AnimationTypes.h"
#include "system/system_utils.h"

// ========================================================================
// Constructor
// ========================================================================

AnimationSystem::AnimationSystem()
{
    // Require both VisualAnimation_data and VisualSprite_data
    ComponentSignature signature;
    signature.set(World::Get().GetComponentID<VisualAnimation_data>());
    signature.set(World::Get().GetComponentID<VisualSprite_data>());
    requiredSignature = signature;
}

// ========================================================================
// Main Update Loop
// ========================================================================

void AnimationSystem::Process()
{
    World& world = World::Get();
    
    // Iterate through all entities with animation components
    for (EntityID entity : m_entities)
    {
        auto& animData = world.GetComponent<VisualAnimation_data>(entity);
        auto& spriteData = world.GetComponent<VisualSprite_data>(entity);
        
        UpdateEntity(entity, animData, spriteData);
    }
}

// ========================================================================
// Update Single Entity
// ========================================================================

void AnimationSystem::UpdateEntity(EntityID entity, VisualAnimation_data& animData, VisualSprite_data& spriteData)
{
    // Clear animation finished flag (it's only set for one frame)
    animData.animationJustFinished = false;
    
    // Check if animation is playing
    if (!animData.isPlaying || animData.isPaused)
        return;
    
    // Resolve animation sequence pointer if needed
    if (!animData.currentSequence)
    {
        if (!ResolveAnimationSequence(animData))
        {
            // Failed to resolve - stop trying to avoid spam
            animData.isPlaying = false;
            return;
        }
    }
    
    const Olympe::AnimationSequence* sequence = animData.currentSequence;
    if (!sequence || sequence->frames.empty())
        return;
    
    // Accumulate frame timer
    float deltaTime = GameEngine::fDt * animData.playbackSpeed * sequence->speed;
    animData.frameTimer += deltaTime;
    
    // Get current frame data
    if (animData.currentFrame < 0 || animData.currentFrame >= static_cast<int>(sequence->frames.size()))
    {
        animData.currentFrame = 0;
    }
    
    const Olympe::AnimationFrame& currentFrame = sequence->frames[animData.currentFrame];
    
    // Check if frame duration has elapsed
    if (animData.frameTimer >= currentFrame.duration)
    {
        animData.frameTimer = 0.0f;
        animData.currentFrame++;
        
        // Check if animation has finished
        if (animData.currentFrame >= static_cast<int>(sequence->frames.size()))
        {
            if (sequence->loop || animData.loop)
            {
                // Loop back to start
                animData.currentFrame = 0;
                animData.loopCount++;
            }
            else
            {
                // Animation finished
                animData.currentFrame = static_cast<int>(sequence->frames.size()) - 1;
                animData.animationJustFinished = true;
                
                // Check for next animation
                if (!sequence->nextAnimation.empty())
                {
                    PlayAnimation(entity, sequence->nextAnimation, true);
                    return;
                }
                else
                {
                    // Stay on last frame
                    animData.isPlaying = false;
                }
            }
        }
    }
    
    // Update sprite srcRect with current frame
    if (animData.currentFrame >= 0 && animData.currentFrame < static_cast<int>(sequence->frames.size()))
    {
        const Olympe::AnimationFrame& frame = sequence->frames[animData.currentFrame];
        spriteData.srcRect = frame.srcRect;
        
        // Update hotspot if specified
        if (frame.hotSpot.x != 0.0f || frame.hotSpot.y != 0.0f)
        {
            spriteData.hotSpot.x = frame.hotSpot.x;
            spriteData.hotSpot.y = frame.hotSpot.y;
        }
    }
    
    // Load sprite texture if needed
    if (!spriteData.sprite && !sequence->spritesheetPath.empty())
    {
        std::string textureId = animData.bankId + "_" + animData.currentAnimName;
        spriteData.sprite = DataManager::Get().GetSprite(textureId, sequence->spritesheetPath);
    }
}

// ========================================================================
// Animation Resolution
// ========================================================================

bool AnimationSystem::ResolveAnimationSequence(VisualAnimation_data& animData)
{
    if (animData.bankId.empty() || animData.currentAnimName.empty())
    {
        SYSTEM_LOG << "[AnimationSystem] ERROR: Empty bankId or animName for entity\n";
        return false;
    }
    
    // Get animation sequence from AnimationManager
    const Olympe::AnimationSequence* sequence = 
        Olympe::AnimationManager::Get().GetAnimationSequence(animData.bankId, animData.currentAnimName);
    
    if (!sequence)
    {
        SYSTEM_LOG << "[AnimationSystem] ERROR: Animation '" << animData.currentAnimName 
                   << "' not found in bank '" << animData.bankId << "'\n";
        return false;
    }
    
    animData.currentSequence = sequence;
    return true;
}

// ========================================================================
// Public API Methods
// ========================================================================

void AnimationSystem::PlayAnimation(EntityID entity, const std::string& animName, bool restart)
{
    World& world = World::Get();
    
    if (!world.HasComponent<VisualAnimation_data>(entity))
        return;
    
    auto& animData = world.GetComponent<VisualAnimation_data>(entity);
    
    // Check if animation is already playing
    if (animData.currentAnimName == animName && !restart)
        return;
    
    // Set new animation
    animData.currentAnimName = animName;
    animData.currentFrame = 0;
    animData.frameTimer = 0.0f;
    animData.isPlaying = true;
    animData.isPaused = false;
    animData.loopCount = 0;
    animData.animationJustFinished = false;
    
    // Clear sequence pointer to force re-resolution
    animData.currentSequence = nullptr;
    
    // Resolve immediately
    ResolveAnimationSequence(animData);
}

void AnimationSystem::PauseAnimation(EntityID entity)
{
    World& world = World::Get();
    
    if (!world.HasComponent<VisualAnimation_data>(entity))
        return;
    
    auto& animData = world.GetComponent<VisualAnimation_data>(entity);
    animData.isPaused = true;
}

void AnimationSystem::ResumeAnimation(EntityID entity)
{
    World& world = World::Get();
    
    if (!world.HasComponent<VisualAnimation_data>(entity))
        return;
    
    auto& animData = world.GetComponent<VisualAnimation_data>(entity);
    animData.isPaused = false;
}

void AnimationSystem::StopAnimation(EntityID entity)
{
    World& world = World::Get();
    
    if (!world.HasComponent<VisualAnimation_data>(entity))
        return;
    
    auto& animData = world.GetComponent<VisualAnimation_data>(entity);
    animData.isPlaying = false;
    animData.isPaused = false;
    animData.currentFrame = 0;
    animData.frameTimer = 0.0f;
}

void AnimationSystem::SetPlaybackSpeed(EntityID entity, float speed)
{
    World& world = World::Get();
    
    if (!world.HasComponent<VisualAnimation_data>(entity))
        return;
    
    auto& animData = world.GetComponent<VisualAnimation_data>(entity);
    animData.playbackSpeed = speed;
}
