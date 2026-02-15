/**
 * @file AnimationTypes.h
 * @brief Core animation data structures for 2D sprite animation system
 * @author Nicolas Chereau
 * @date 2025
 * 
 * Defines data structures for frame-based sprite animation including:
 * - AnimationFrame: Individual frame data
 * - AnimationSequence: Complete animation with frames and playback settings
 * - AnimationBank: Collection of animations for an entity
 * - AnimationGraph: State machine for animation transitions
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <SDL3/SDL.h>

namespace Olympe
{

/**
 * @brief Represents a single frame in an animation
 */
struct AnimationFrame
{
    SDL_FRect srcRect;          ///< Source rectangle in spritesheet (x, y, w, h)
    float duration;             ///< Duration of this frame in seconds
    SDL_FPoint hotSpot;         ///< Render offset (pivot point)
    std::string eventName;      ///< Optional event triggered when this frame starts (empty if none)
    
    AnimationFrame()
        : srcRect{0, 0, 0, 0}
        , duration(0.1f)
        , hotSpot{0, 0}
        , eventName("")
    {}
};

/**
 * @brief Defines a complete animation sequence
 */
struct AnimationSequence
{
    std::string name;           ///< Animation identifier
    std::string spritesheetPath;///< Path to spritesheet texture
    std::vector<AnimationFrame> frames; ///< Frame data
    
    // Playback settings
    bool loop;                  ///< Whether animation loops
    float speed;                ///< Playback speed multiplier (1.0 = normal)
    std::string nextAnimation;  ///< Animation to play after completion (if not looping)
    
    AnimationSequence()
        : name("")
        , spritesheetPath("")
        , loop(true)
        , speed(1.0f)
        , nextAnimation("")
    {}
};

/**
 * @brief Collection of animations for an entity with spritesheet metadata
 */
struct AnimationBank
{
    std::string bankId;         ///< Unique identifier for this animation bank
    
    // Spritesheet metadata (for automatic frame calculation)
    int frameWidth;             ///< Width of each frame in pixels
    int frameHeight;            ///< Height of each frame in pixels
    int columns;                ///< Number of columns in spritesheet
    int spacing;                ///< Spacing between frames in pixels
    int margin;                 ///< Margin around spritesheet edges in pixels
    
    // Animation sequences
    std::unordered_map<std::string, AnimationSequence> animations;
    
    AnimationBank()
        : bankId("")
        , frameWidth(32)
        , frameHeight(32)
        , columns(1)
        , spacing(0)
        , margin(0)
    {}
    
    /**
     * @brief Calculate source rectangle for a frame based on spritesheet layout
     * @param frameIndex Frame index (0-based)
     * @return Source rectangle in spritesheet coordinates
     */
    SDL_FRect CalculateFrameRect(int frameIndex) const
    {
        int row = frameIndex / columns;
        int col = frameIndex % columns;
        
        SDL_FRect rect;
        rect.x = static_cast<float>(margin + col * (frameWidth + spacing));
        rect.y = static_cast<float>(margin + row * (frameHeight + spacing));
        rect.w = static_cast<float>(frameWidth);
        rect.h = static_cast<float>(frameHeight);
        
        return rect;
    }
};

/**
 * @brief State machine transition definition
 */
struct AnimationTransition
{
    std::string fromState;      ///< Source animation state
    std::string toState;        ///< Target animation state
    std::string condition;      ///< Optional condition for transition (empty = always allowed)
    
    AnimationTransition()
        : fromState("")
        , toState("")
        , condition("")
    {}
};

/**
 * @brief Animation state machine (FSM) for managing transitions
 */
struct AnimationGraph
{
    std::string graphId;        ///< Unique identifier for this graph
    std::string defaultState;   ///< Initial animation state
    
    // State machine data
    std::vector<std::string> states;                     ///< All valid animation states
    std::vector<AnimationTransition> transitions;        ///< Valid state transitions
    
    AnimationGraph()
        : graphId("")
        , defaultState("")
    {}
    
    /**
     * @brief Check if transition from one state to another is valid
     * @param from Source animation state
     * @param to Target animation state
     * @return true if transition is allowed
     */
    bool CanTransition(const std::string& from, const std::string& to) const
    {
        // Always allow transitioning to the same state (restart)
        if (from == to)
            return true;
        
        // Check if explicit transition exists
        for (const auto& transition : transitions)
        {
            if (transition.fromState == from && transition.toState == to)
                return true;
        }
        
        return false;
    }
};

} // namespace Olympe
 * @brief Core data structures for 2D sprite animation system
 * @author Olympe Engine Team
 * @date 2026
 * 
 * Defines animation frames, sequences, banks, and state machines.
 * These structures are used by AnimationManager to load animation data from JSON files.
 */

#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H

#include <string>
#include <vector>
#include <memory>
#include "SDL3/SDL.h"
#include "vector.h"

namespace Olympe {
namespace Animation {

/**
 * @struct AnimationFrame
 * @brief Single frame in an animation sequence
 * 
 * Represents one frame with source rectangle, duration, and optional metadata.
 * 
 * Example usage:
 * @code
 * AnimationFrame frame;
 * frame.srcRect = {0, 0, 64, 64};
 * frame.duration = 0.1f;  // 100ms per frame
 * frame.hotSpot = {32, 32};  // Center pivot
 * frame.events.push_back("footstep");
 * @endcode
 */
struct AnimationFrame
{
    SDL_Rect srcRect;       ///< Source rectangle in spritesheet (pixels)
    float duration;         ///< Frame duration in seconds
    Vector hotSpot;         ///< Pivot point for rendering (relative to top-left)
    std::vector<std::string> events; ///< Frame events (footstep, hit, etc.)
};

/**
 * @struct AnimationSequence
 * @brief A named sequence of frames that make up an animation
 * 
 * Defines a complete animation with timing, looping, and transition properties.
 * Multiple entities can share the same AnimationSequence (flyweight pattern).
 * 
 * Example usage:
 * @code
 * AnimationSequence walk;
 * walk.name = "walk";
 * walk.loop = true;
 * walk.speed = 1.0f;
 * // Add frames...
 * @endcode
 */
struct AnimationSequence
{
    std::string name;              ///< Animation name (e.g., "walk", "idle", "attack")
    bool loop;                     ///< Whether animation repeats or plays once
    float speed;                   ///< Playback speed multiplier (1.0 = normal)
    std::string nextAnimation;     ///< Auto-transition to this animation when done (optional)
    std::vector<AnimationFrame> frames; ///< Sequence of frames
};

/**
 * @struct AnimationBank
 * @brief Collection of animation sequences for a character or object
 * 
 * An animation bank defines all animations for a single entity type, along with
 * spritesheet layout information. Banks are loaded from JSON files and cached.
 * 
 * Example JSON structure:
 * @code{.json}
 * {
 *   "id": "player",
 *   "spritesheetPath": "Gamedata/Sprites/player.png",
 *   "frameWidth": 64,
 *   "frameHeight": 64,
 *   "columns": 8,
 *   "spacing": 0,
 *   "margin": 0,
 *   "animations": [
 *     {
 *       "name": "idle",
 *       "loop": true,
 *       "frameRange": {
 *         "start": 0,
 *         "end": 7,
 *         "frameDuration": 0.1
 *       }
 *     }
 *   ]
 * }
 * @endcode
 */
struct AnimationBank
{
    std::string id;                ///< Unique identifier for this bank
    std::string spritesheetPath;   ///< Path to spritesheet PNG file
    int frameWidth;                ///< Width of each frame in pixels
    int frameHeight;               ///< Height of each frame in pixels
    int columns;                   ///< Number of frames per row
    int spacing;                   ///< Pixels between frames
    int margin;                    ///< Border pixels around grid
    
    /// Collection of animation sequences in this bank
    std::vector<std::shared_ptr<AnimationSequence>> animations;
    
    /**
     * @brief Get animation by name
     * @param name Animation name to search for
     * @return Shared pointer to animation sequence, or nullptr if not found
     */
    std::shared_ptr<AnimationSequence> GetAnimation(const std::string& name) const;
};

/**
 * @struct AnimationTransition
 * @brief Defines a valid state transition in an animation graph
 * 
 * Specifies which states can be entered from the current state, with optional conditions.
 */
struct AnimationTransition
{
    std::string to;                ///< Target state name
    std::string condition;         ///< Optional condition string (for documentation/tools)
};

/**
 * @struct AnimationState
 * @brief Single state in a finite state machine (FSM)
 * 
 * Each state plays a specific animation and defines valid transitions to other states.
 * 
 * Example:
 * @code
 * AnimationState idle;
 * idle.name = "idle";
 * idle.animation = "idle";
 * idle.transitions.push_back({"walk", "velocity > 0"});
 * idle.transitions.push_back({"attack", "input.attack"});
 * @endcode
 */
struct AnimationState
{
    std::string name;              ///< State name (e.g., "idle", "walking")
    std::string animation;         ///< Animation to play in this state
    std::vector<AnimationTransition> transitions; ///< Valid state transitions
};

/**
 * @struct AnimationGraph
 * @brief Finite state machine (FSM) for controlling animation flow
 * 
 * Animation graphs define valid state transitions and ensure only legal animation
 * changes occur. This prevents bugs like jumping while dead or attacking while stunned.
 * 
 * Example JSON structure:
 * @code{.json}
 * {
 *   "id": "player_fsm",
 *   "defaultState": "idle",
 *   "states": [
 *     {
 *       "name": "idle",
 *       "animation": "idle",
 *       "transitions": [
 *         {"to": "walk"},
 *         {"to": "attack"}
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 */
struct AnimationGraph
{
    std::string id;                ///< Unique identifier for this graph
    std::string defaultState;      ///< Starting state when entity is created
    std::vector<AnimationState> states; ///< All states in the FSM
    
    /**
     * @brief Get state by name
     * @param name State name to search for
     * @return Pointer to state, or nullptr if not found
     */
    const AnimationState* GetState(const std::string& name) const;
    
    /**
     * @brief Check if transition is valid
     * @param fromState Current state name
     * @param toState Target state name
     * @return True if transition is allowed, false otherwise
     */
    bool IsTransitionValid(const std::string& fromState, const std::string& toState) const;
};

} // namespace Animation
} // namespace Olympe

#endif // ANIMATION_TYPES_H
