/**
 * @file AnimationHelpers.h
 * @brief Helper functions for manipulating VisualAnimation_data components
 * @author Olympe Engine - Animation System
 * @date 2025
 * 
 * Provides utility functions to set/get animation graph parameters
 * without violating ECS principles (components remain pure data).
 */

#pragma once

#include "../ECS_Components.h"
#include <string>

namespace Olympe
{
namespace AnimationHelpers
{
    // ========================================================================
    // Parameter Setters (overloaded)
    // ========================================================================
    
    /**
     * @brief Set float parameter value
     */
    inline void SetParameter(VisualAnimation_data& animData, const std::string& name, float value)
    {
        animData.floatParams[name] = value;
    }
    
    /**
     * @brief Set bool parameter value
     */
    inline void SetParameter(VisualAnimation_data& animData, const std::string& name, bool value)
    {
        animData.boolParams[name] = value;
    }
    
    /**
     * @brief Set int parameter value
     */
    inline void SetParameter(VisualAnimation_data& animData, const std::string& name, int value)
    {
        animData.intParams[name] = value;
    }
    
    // ========================================================================
    // Parameter Getters
    // ========================================================================
    
    /**
     * @brief Get float parameter value with default
     */
    inline float GetFloatParameter(const VisualAnimation_data& animData, const std::string& name, float defaultValue = 0.0f)
    {
        auto it = animData.floatParams.find(name);
        return (it != animData.floatParams.end()) ? it->second : defaultValue;
    }
    
    /**
     * @brief Get bool parameter value with default
     */
    inline bool GetBoolParameter(const VisualAnimation_data& animData, const std::string& name, bool defaultValue = false)
    {
        auto it = animData.boolParams.find(name);
        return (it != animData.boolParams.end()) ? it->second : defaultValue;
    }
    
    /**
     * @brief Get int parameter value with default
     */
    inline int GetIntParameter(const VisualAnimation_data& animData, const std::string& name, int defaultValue = 0)
    {
        auto it = animData.intParams.find(name);
        return (it != animData.intParams.end()) ? it->second : defaultValue;
    }
    
    // ========================================================================
    // Parameter Management
    // ========================================================================
    
    /**
     * @brief Check if parameter exists (any type)
     */
    inline bool HasParameter(const VisualAnimation_data& animData, const std::string& name)
    {
        return animData.floatParams.count(name) > 0 || 
               animData.boolParams.count(name) > 0 || 
               animData.intParams.count(name) > 0;
    }
    
    /**
     * @brief Clear all parameters
     */
    inline void ClearParameters(VisualAnimation_data& animData)
    {
        animData.floatParams.clear();
        animData.boolParams.clear();
        animData.intParams.clear();
    }
    
} // namespace AnimationHelpers
} // namespace Olympe
