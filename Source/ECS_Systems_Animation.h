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
 * @brief ECS system for updating sprite animations
 * @author Olympe Engine Team
 * @date 2026
 * 
 * AnimationSystem processes entities with VisualAnimation_data and updates
 * their animation frames based on elapsed time. Integrates with AnimationManager
 * for animation data and updates VisualSprite_data for rendering.
 */

#ifndef ECS_SYSTEMS_ANIMATION_H
#define ECS_SYSTEMS_ANIMATION_H

#include "ECS_Register.h"
#include "ECS_Components.h"
#include "Singleton.h"
#include <string>

/**
 * @class AnimationSystem
 * @brief ECS system for updating sprite animations
 * 
 * AnimationSystem is responsible for:
 * - Updating animation frames each tick based on delta time
 * - Managing animation playback (play, pause, stop)
 * - Handling animation transitions and FSM state changes
 * - Updating VisualSprite_data srcRect to display current frame
 * 
 * The system processes all entities with both VisualAnimation_data and
 * VisualSprite_data components. It uses delta-time accumulation for
 * frame-rate independent animation playback.
 * 
 * Architecture:
 * - Reads animation data from AnimationManager (banks, graphs)
 * - Updates VisualAnimation_data component (frame, elapsed time, state)
 * - Writes to VisualSprite_data component (srcRect for rendering)
 * 
 * Usage pattern:
 * @code
 * // System is automatically registered and updated by World
 * // Control animations via API:
 * AnimationSystem::Get().PlayAnimation(entity, "walk", false);
 * AnimationSystem::Get().PauseAnimation(entity);
 * AnimationSystem::Get().SetPlaybackSpeed(entity, 2.0f);
 * @endcode
 * 
 * @see VisualAnimation_data
 * @see VisualSprite_data
 * @see AnimationManager
 */
class AnimationSystem : public ECS_System, public Singleton<AnimationSystem>
{
public:
	/**
	 * @brief Process all animated entities
	 * @param deltaTime Time elapsed since last frame (seconds)
	 * 
	 * Called automatically by World each frame. Updates animation state for
	 * all entities with VisualAnimation_data components.
	 * 
	 * Processing steps:
	 * 1. Accumulate elapsed time with playback speed
	 * 2. Check if frame should advance
	 * 3. Update current frame index
	 * 4. Handle loop/non-loop behavior
	 * 5. Update VisualSprite_data srcRect
	 * 
	 * @note Uses GameEngine::fDt for delta time in actual implementation
	 */
	void Process(float deltaTime);
	
	/**
	 * @brief Play or switch to a different animation
	 * @param entity Entity with VisualAnimation_data component
	 * @param animationName Name of animation to play (from bank)
	 * @param restart If true, restart animation even if already playing
	 * @return true if animation started successfully, false otherwise
	 * 
	 * Switches to the specified animation and starts playback. If the entity
	 * is already playing this animation and restart is false, does nothing.
	 * 
	 * Example:
	 * @code
	 * // Switch to walk animation (doesn't restart if already walking)
	 * AnimationSystem::Get().PlayAnimation(player, "walk", false);
	 * 
	 * // Always restart attack animation from beginning
	 * AnimationSystem::Get().PlayAnimation(player, "attack", true);
	 * @endcode
	 * 
	 * @note Validates that animation exists in bank
	 * @see PauseAnimation, StopAnimation
	 */
	bool PlayAnimation(ECS_Entity entity, const std::string& animationName, bool restart = false);
	
	/**
	 * @brief Pause animation playback
	 * @param entity Entity to pause
	 * 
	 * Pauses animation on current frame. Can be resumed with ResumeAnimation().
	 * 
	 * Example:
	 * @code
	 * // Pause game
	 * for (ECS_Entity entity : allEntities) {
	 *     AnimationSystem::Get().PauseAnimation(entity);
	 * }
	 * @endcode
	 * 
	 * @see ResumeAnimation, StopAnimation
	 */
	void PauseAnimation(ECS_Entity entity);
	
	/**
	 * @brief Resume paused animation playback
	 * @param entity Entity to resume
	 * 
	 * Resumes animation from where it was paused. Does not reset frame.
	 * 
	 * Example:
	 * @code
	 * // Unpause game
	 * for (ECS_Entity entity : allEntities) {
	 *     AnimationSystem::Get().ResumeAnimation(entity);
	 * }
	 * @endcode
	 * 
	 * @see PauseAnimation
	 */
	void ResumeAnimation(ECS_Entity entity);
	
	/**
	 * @brief Stop animation and reset to first frame
	 * @param entity Entity to stop
	 * 
	 * Stops animation playback and resets to frame 0. Different from pause,
	 * which preserves the current frame.
	 * 
	 * Example:
	 * @code
	 * void OnEntityDeath(ECS_Entity entity)
	 * {
	 *     AnimationSystem::Get().StopAnimation(entity);
	 *     // Entity displays first frame of last animation
	 * }
	 * @endcode
	 * 
	 * @see PauseAnimation
	 */
	void StopAnimation(ECS_Entity entity);
	
	/**
	 * @brief Change animation playback speed
	 * @param entity Entity to modify
	 * @param speed Speed multiplier (1.0 = normal, 2.0 = double, 0.5 = half)
	 * 
	 * Changes playback speed without changing the animation. Useful for
	 * slow-motion effects, time manipulation, or status effects.
	 * 
	 * Example:
	 * @code
	 * // Apply slow motion effect
	 * AnimationSystem::Get().SetPlaybackSpeed(entity, 0.5f);
	 * 
	 * // Apply haste buff
	 * AnimationSystem::Get().SetPlaybackSpeed(entity, 2.0f);
	 * 
	 * // Reset to normal
	 * AnimationSystem::Get().SetPlaybackSpeed(entity, 1.0f);
	 * @endcode
	 * 
	 * @note Multiplies the animation's base speed from JSON
	 */
	void SetPlaybackSpeed(ECS_Entity entity, float speed);
	
	/**
	 * @brief Transition to a new state in animation graph (FSM)
	 * @param entity Entity with VisualAnimation_data component
	 * @param stateName Name of target state
	 * @return true if transition succeeded, false if invalid
	 * 
	 * Attempts to transition to a new state in the animation graph. Only
	 * succeeds if the transition is valid according to the FSM definition.
	 * 
	 * Example:
	 * @code
	 * void HandleCombatInput(ECS_Entity player)
	 * {
	 *     if (InputManager::Get().IsActionJustPressed("attack"))
	 *     {
	 *         if (AnimationSystem::Get().TransitionToState(player, "attack"))
	 *         {
	 *             std::cout << "Started attack" << std::endl;
	 *         }
	 *         else
	 *         {
	 *             std::cout << "Cannot attack from current state" << std::endl;
	 *         }
	 *     }
	 * }
	 * @endcode
	 * 
	 * @note Requires entity to have graphId set in VisualAnimation_data
	 * @see AnimationGraph
	 */
	bool TransitionToState(ECS_Entity entity, const std::string& stateName);
	
	/**
	 * @brief Get current frame index of entity's animation
	 * @param entity Entity to query
	 * @return Current frame index (0-based), or -1 if invalid
	 * 
	 * Example:
	 * @code
	 * int frame = AnimationSystem::Get().GetCurrentFrame(player);
	 * std::cout << "Player on frame " << frame << std::endl;
	 * 
	 * // Check if on specific frame for events
	 * if (frame == 3) {
	 *     PlaySound("footstep.wav");
	 * }
	 * @endcode
	 */
	int GetCurrentFrame(ECS_Entity entity) const;
	
	/**
	 * @brief Check if non-looping animation has finished
	 * @param entity Entity to query
	 * @return true if animation finished, false if still playing or looping
	 * 
	 * Useful for waiting for one-shot animations to complete before
	 * transitioning to a new state.
	 * 
	 * Example:
	 * @code
	 * void CheckAttackComplete(ECS_Entity entity)
	 * {
	 *     if (AnimationSystem::Get().IsAnimationComplete(entity))
	 *     {
	 *         std::cout << "Attack animation finished" << std::endl;
	 *         AnimationSystem::Get().PlayAnimation(entity, "idle", true);
	 *     }
	 * }
	 * @endcode
	 */
	bool IsAnimationComplete(ECS_Entity entity) const;

private:
	/**
	 * @brief Update animation frame for a single entity
	 * @param entity Entity to update
	 * @param deltaTime Time elapsed since last frame
	 * 
	 * Internal method called by Process() for each animated entity.
	 * Handles frame advancement, looping, and next animation transitions.
	 */
	void UpdateFrame(ECS_Entity entity, float deltaTime);
	
	/**
	 * @brief Apply current frame's srcRect to VisualSprite_data
	 * @param entity Entity to update
	 * 
	 * Internal method that updates the sprite's source rectangle to display
	 * the current animation frame.
	 */
	void ApplySrcRect(ECS_Entity entity);
	
	/**
	 * @brief Trigger frame events for current frame
	 * @param entity Entity triggering events
	 * @param frameIndex Frame index with events
	 * 
	 * Internal method that triggers any events attached to the current frame
	 * (e.g., footstep sounds, hit detection).
	 */
	void TriggerFrameEvents(ECS_Entity entity, int frameIndex);
};

#endif // ECS_SYSTEMS_ANIMATION_H
