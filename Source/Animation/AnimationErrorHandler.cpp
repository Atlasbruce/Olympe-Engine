/*
Olympe Engine V2 2025
Animation System - Error Handler Implementation
*/

#include "Animation/AnimationErrorHandler.h"
#include "system/system_utils.h"

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationErrorHandler Implementation
    // ========================================================================

    void AnimationErrorHandler::LogMissingAnimation(
        const std::string& entityName,
        const std::string& graphPath,
        const std::string& stateName,
        const std::string& animName)
    {
        std::string errorKey = "MISSING_ANIM:" + entityName + ":" + graphPath + ":" + stateName + ":" + animName;
        
        if (m_loggedErrors.find(errorKey) == m_loggedErrors.end())
        {
            m_loggedErrors.insert(errorKey);
            SYSTEM_LOG << "[ANIMATION ERROR] Missing animation:\n"
                      << "  Entity: " << entityName << "\n"
                      << "  Graph: " << graphPath << "\n"
                      << "  State: " << stateName << "\n"
                      << "  Animation: " << animName << "\n"
                      << "  Using fallback placeholder.\n";
        }
    }

    void AnimationErrorHandler::LogMissingSpritesheet(
        const std::string& entityName,
        const std::string& spritesheetId,
        const std::string& animName)
    {
        std::string errorKey = "MISSING_SPRITE:" + entityName + ":" + spritesheetId + ":" + animName;
        
        if (m_loggedErrors.find(errorKey) == m_loggedErrors.end())
        {
            m_loggedErrors.insert(errorKey);
            SYSTEM_LOG << "[ANIMATION ERROR] Missing spritesheet:\n"
                      << "  Entity: " << entityName << "\n"
                      << "  Spritesheet ID: " << spritesheetId << "\n"
                      << "  Animation: " << animName << "\n"
                      << "  Using fallback placeholder.\n";
        }
    }

    void AnimationErrorHandler::LogError(const std::string& message)
    {
        std::string errorKey = GenerateErrorKey(message);
        
        if (m_loggedErrors.find(errorKey) == m_loggedErrors.end())
        {
            m_loggedErrors.insert(errorKey);
            SYSTEM_LOG << "[ANIMATION ERROR] " << message << "\n";
        }
    }

    void AnimationErrorHandler::ClearCache()
    {
        m_loggedErrors.clear();
    }

    std::string AnimationErrorHandler::GenerateErrorKey(const std::string& error)
    {
        return "GENERIC:" + error;
    }

} // namespace OlympeAnimation
