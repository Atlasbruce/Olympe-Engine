/**
 * @file ECS_Systems_Animation.h
 * @brief Animation system for 2D sprite animation
 * @author Nicolas Chereau
 * @date 2025
 * 
 * ECS System that updates sprite animations frame-by-frame based on elapsed time.
 * Integrates with AnimationManager and updates VisualSprite_data srcRect.
 */

#pragma once

#include "ECS_Systems.h"
#include "ECS_Components.h"

/**
 * @brief Animation system for sprite-based 2D animations
 * 
 * Requires: VisualAnimation_data + VisualSprite_data
 * 
 * Responsibilities:
 * - Update animation frames based on elapsed time (GameEngine::fDt)
 * - Resolve animation pointers from AnimationManager
 * - Update VisualSprite_data::srcRect to current frame
 * - Handle looping and non-looping animations
 * - Support animation transitions via API calls
 */
class AnimationSystem : public ECS_System
{
public:
    AnimationSystem();
    virtual void Process() override;
    
    /**
     * @brief Play a specific animation on an entity
     * @param entity Entity ID
     * @param animName Animation name to play
     * @param restart If true, restart animation even if already playing
     */
    void PlayAnimation(EntityID entity, const std::string& animName, bool restart = false);
    
    /**
     * @brief Pause animation playback
     * @param entity Entity ID
     */
    void PauseAnimation(EntityID entity);
    
    /**
     * @brief Resume paused animation
     * @param entity Entity ID
     */
    void ResumeAnimation(EntityID entity);
    
    /**
     * @brief Stop animation playback
     * @param entity Entity ID
     */
    void StopAnimation(EntityID entity);
    
    /**
     * @brief Set playback speed multiplier
     * @param entity Entity ID
     * @param speed Speed multiplier (1.0 = normal, 2.0 = double speed, etc.)
     */
    void SetPlaybackSpeed(EntityID entity, float speed);
    
private:
    /**
     * @brief Update a single entity's animation
     * @param entity Entity ID
     * @param animData Animation component
     * @param spriteData Sprite component
     */
    void UpdateEntity(EntityID entity, VisualAnimation_data& animData, VisualSprite_data& spriteData);
    
    /**
     * @brief Resolve animation sequence pointer from AnimationManager
     * @param animData Animation component
     * @return true if successfully resolved
     */
    bool ResolveAnimationSequence(VisualAnimation_data& animData);
};
