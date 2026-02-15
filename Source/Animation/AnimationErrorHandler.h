/*
Olympe Engine V2 2025
Animation System - Error Handler

Purpose:
- Utility class for logging animation errors
- Prevents spam by logging each unique error only once
- Provides fallback behavior for missing animations
*/

#pragma once

#include <string>
#include <unordered_set>

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationErrorHandler - Manages error logging and fallbacks
    // ========================================================================
    class AnimationErrorHandler
    {
    public:
        static AnimationErrorHandler& GetInstance()
        {
            static AnimationErrorHandler instance;
            return instance;
        }
        static AnimationErrorHandler& Get() { return GetInstance(); }

        // Log a missing animation error (only once per unique combination)
        void LogMissingAnimation(
            const std::string& entityName,
            const std::string& graphPath,
            const std::string& stateName,
            const std::string& animName
        );

        // Log a missing spritesheet error (only once per unique combination)
        void LogMissingSpritesheet(
            const std::string& entityName,
            const std::string& spritesheetId,
            const std::string& animName
        );

        // Log a general animation error (only once per unique message)
        void LogError(const std::string& message);

        // Clear the error cache (useful for testing)
        void ClearCache();

    private:
        AnimationErrorHandler() = default;
        ~AnimationErrorHandler() = default;

        // Delete copy constructor and assignment operator (singleton)
        AnimationErrorHandler(const AnimationErrorHandler&) = delete;
        AnimationErrorHandler& operator=(const AnimationErrorHandler&) = delete;

        std::unordered_set<std::string> m_loggedErrors;

        // Helper: Generate unique key for error
        std::string GenerateErrorKey(const std::string& error);
    };

} // namespace OlympeAnimation
