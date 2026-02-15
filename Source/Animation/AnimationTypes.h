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
    
    int GetFrameCount() const { return static_cast<int>(frames.size()); }
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
    
    const AnimationSequence* GetAnimation(const std::string& name) const
    {
        auto it = animations.find(name);
        return (it != animations.end()) ? &it->second : nullptr;
    }
};

/**
 * @struct AnimationState
 * @brief Single state in FSM
 */
struct AnimationState
{
    std::string name;
    std::string animation;
    std::vector<std::string> transitions;
    
    AnimationState() : name(""), animation("") {}
};

/**
 * @brief Animation state machine (FSM) for managing transitions
 */
struct AnimationGraph
{
    std::string graphId;        ///< Unique identifier for this graph
    std::string defaultState;   ///< Initial animation state
    
    // State machine data
    std::unordered_map<std::string, AnimationState> states;
    
    AnimationGraph()
        : graphId("")
        , defaultState("")
    {}
    
    /**
     * @brief Get state by name
     * @param name State name to search for
     * @return Pointer to state, or nullptr if not found
     */
    const AnimationState* GetState(const std::string& name) const
    {
        auto it = states.find(name);
        return (it != states.end()) ? &it->second : nullptr;
    }
    
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
        auto it = states.find(from);
        if (it == states.end())
            return false;
        
        for (const auto& target : it->second.transitions)
        {
            if (target == to)
                return true;
        }
        
        return false;
    }
};

} // namespace Olympe
