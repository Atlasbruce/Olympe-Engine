/**
 * @file AnimationTypes.h
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
