/*
Olympe Engine V2 2025
Animation System - Common Types

Purpose:
- Define common types, enums, and structures used throughout the animation system
- Provides shared data structures for AnimationBank, AnimationGraph, and AnimationManager
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
