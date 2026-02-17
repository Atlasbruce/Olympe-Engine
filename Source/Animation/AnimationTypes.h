/*
Olympe Engine V2 2025
Animation System - Common Types

Purpose:
- Define common types, enums, and structures used throughout the animation system
- Provides shared data structures for AnimationBank, AnimationGraph, and AnimationManager
*/
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
#include <SDL3/SDL.h>

// Forward declarations
using TextureHandle = SDL_Texture*;

namespace OlympeAnimation
{
    // ========================================================================
    // Hotspot - 2D position for sprite anchor points
    // Note: We don't reuse the engine's Vector class here to keep the
    // animation system decoupled and use a simple POD structure
    // ========================================================================
    struct Hotspot
    {
        float x = 0.0f;
        float y = 0.0f;

        Hotspot() = default;
        Hotspot(float _x, float _y) : x(_x), y(_y) {}
    };

    // ========================================================================
    // Blend Mode - How animations blend together
    // ========================================================================
    enum class BlendMode
    {
        Override,  // Replace current animation completely
        Additive,  // Add to current animation
        Blend      // Smooth blend between animations
    };

    // ========================================================================
    // Transition Type - How transitions are evaluated
    // ========================================================================
    enum class TransitionType
    {
        Immediate,     // Instant transition
        Smooth,        // Smooth blend over time
        AfterComplete  // Wait for current animation to complete
    };

    // ========================================================================
    // Parameter Type - Types of parameters in animation graph
    // ========================================================================
    enum class ParameterType
    {
        Bool,
        Float,
        Int,
        String
    };

    // ========================================================================
    // Comparison Operator - For condition evaluation
    // ========================================================================
    enum class ComparisonOperator
    {
        Equal,              // ==
        NotEqual,           // !=
        Greater,            // >
        GreaterOrEqual,     // >=
        Less,               // <
        LessOrEqual         // <=
    };

    // ========================================================================
    // Parameter Value - Union for storing different parameter types
    // ========================================================================
    struct ParameterValue
    {
        ParameterType type = ParameterType::Float;
        union
        {
            bool boolValue;
            float floatValue;
            int intValue;
        };
        std::string stringValue;

        ParameterValue() : floatValue(0.0f) {}
        ParameterValue(bool val) : type(ParameterType::Bool), boolValue(val) {}
        ParameterValue(float val) : type(ParameterType::Float), floatValue(val) {}
        ParameterValue(int val) : type(ParameterType::Int), intValue(val) {}
        ParameterValue(const std::string& val) : type(ParameterType::String), stringValue(val) {}
    };

    // ========================================================================
    // Animation Event Data - Events triggered during animation playback
    // ========================================================================
    struct AnimationEventData
    {
        std::string type;           // Event type: "sound", "hitbox", "vfx", "gamelogic"
        int frame = 0;              // Frame number to trigger on
        std::string dataJson;       // JSON string with event-specific data
    };

} // namespace OlympeAnimation
#include <unordered_map>
#include <SDL3/SDL.h>

namespace Olympe
{

/**
 * @struct SpritesheetInfo
 * @brief Metadata for a single spritesheet within an animation bank
 */
struct SpritesheetInfo
{
    std::string id;                 ///< Unique identifier (e.g., "thesee_idle")
    std::string path;               ///< Relative path to image
    std::string description;
    
    int frameWidth = 0;             ///< Width of each frame in pixels
    int frameHeight = 0;            ///< Height of each frame in pixels
    int columns = 0;                ///< Number of columns in spritesheet
    int rows = 0;                   ///< Number of rows in spritesheet
    int totalFrames = 0;            ///< Total number of frames available
    int spacing = 0;                ///< Spacing between frames in pixels
    int margin = 0;                 ///< Margin around spritesheet edges
    
    SDL_FPoint hotspot = {0.0f, 0.0f}; ///< Default hotspot/pivot point
    
    SpritesheetInfo() = default;
};

/**
 * @brief Represents a single frame in an animation (DEPRECATED - use SpritesheetInfo + frame ranges)
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
    std::string name;                   ///< Animation name (e.g., "idle", "walk")
    std::string spritesheetId;          ///< References SpritesheetInfo.id
    
    int startFrame = 0;                 ///< Starting frame index (0-based)
    int frameCount = 1;                 ///< Number of frames in sequence
    
    float frameDuration = 0.1f;         ///< Duration of each frame in seconds
    bool loop = true;                   ///< Whether animation loops
    float speed = 1.0f;                 ///< Speed multiplier
    std::string nextAnimation;          ///< Animation to play after this one
    
    // DEPRECATED: Old frame-by-frame approach (kept for backward compatibility)
    std::string spritesheetPath;        ///< Path to spritesheet texture
    std::vector<AnimationFrame> frames; ///< Frame data
    
    AnimationSequence()
        : name("")
        , spritesheetId("")
        , startFrame(0)
        , frameCount(1)
        , frameDuration(0.1f)
        , loop(true)
        , speed(1.0f)
        , nextAnimation("")
        , spritesheetPath("")
    {}
    
    float GetTotalDuration() const {
        return frameCount * frameDuration / speed;
    }
    
    float GetEffectiveFPS() const {
        return (frameDuration > 0.0f) ? (1.0f / frameDuration) * speed : 0.0f;
    }
    
    int GetFrameCount() const { 
        // Support both old and new formats
        return frames.empty() ? frameCount : static_cast<int>(frames.size());
    }
};

/**
 * @brief Collection of animations for an entity with multi-spritesheet support
 */
struct AnimationBank
{
    std::string bankId;         ///< Unique identifier for this animation bank
    std::string description;
    std::string author;
    std::string createdDate;
    std::string lastModifiedDate;
    std::vector<std::string> tags;
    
    // Multi-spritesheet support (NEW)
    std::vector<SpritesheetInfo> spritesheets;
    
    // DEPRECATED: Old single-spritesheet metadata (kept for backward compatibility)
    int frameWidth;             ///< Width of each frame in pixels
    int frameHeight;            ///< Height of each frame in pixels
    int columns;                ///< Number of columns in spritesheet
    int spacing;                ///< Spacing between frames in pixels
    int margin;                 ///< Margin around spritesheet edges in pixels
    
    // Animation sequences
    std::unordered_map<std::string, AnimationSequence> animations;
    
    AnimationBank()
        : bankId("")
        , description("")
        , author("")
        , createdDate("")
        , lastModifiedDate("")
        , frameWidth(32)
        , frameHeight(32)
        , columns(1)
        , spacing(0)
        , margin(0)
    {}
    
    /**
     * @brief Get spritesheet by ID
     * @param id Spritesheet identifier
     * @return Pointer to SpritesheetInfo if found, nullptr otherwise
     */
    const SpritesheetInfo* GetSpritesheet(const std::string& id) const
    {
        for (const auto& sheet : spritesheets)
        {
            if (sheet.id == id)
                return &sheet;
        }
        return nullptr;
    }
    
    SpritesheetInfo* GetSpritesheet(const std::string& id)
    {
        for (auto& sheet : spritesheets)
        {
            if (sheet.id == id)
                return &sheet;
        }
        return nullptr;
    }
    
    /**
     * @brief Get animation sequence by name
     * @param name Animation name
     * @return Pointer to AnimationSequence if found, nullptr otherwise
     */
    const AnimationSequence* GetSequence(const std::string& name) const
    {
        auto it = animations.find(name);
        return (it != animations.end()) ? &it->second : nullptr;
    }
    
    AnimationSequence* GetSequence(const std::string& name)
    {
        auto it = animations.find(name);
        return (it != animations.end()) ? &it->second : nullptr;
    }
    
    /**
     * @brief DEPRECATED: Calculate source rectangle for a frame based on spritesheet layout
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
    
    /**
     * @brief DEPRECATED: Get animation sequence by name (use GetSequence() instead)
     * @param name Animation name
     * @return Pointer to AnimationSequence if found, nullptr otherwise
     * @deprecated Use GetSequence() for consistency with new API
     */
    const AnimationSequence* GetAnimation(const std::string& name) const
    {
        return GetSequence(name);
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
