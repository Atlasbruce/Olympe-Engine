/*
Olympe Engine V2 2025
Animation System - Animation Bank

Purpose:
- Define animation data structures (spritesheets, animations, events)
- Load and parse animation bank JSON files
- Provide access to animation data for playback
*/

#pragma once

#include "AnimationTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace OlympeAnimation
{
    // ========================================================================
    // Spritesheet - Defines a texture atlas containing animation frames
    // ========================================================================
    struct SpriteSheet
    {
        std::string id;              // Unique identifier
        std::string path;            // Path to texture file
        TextureHandle texture = nullptr; // Loaded texture handle
        
        int frameWidth = 32;         // Width of each frame
        int frameHeight = 32;        // Height of each frame
        int columns = 1;             // Number of columns in the sheet
        int rows = 1;                // Number of rows in the sheet
        int totalFrames = 1;         // Total number of frames
        int spacing = 0;             // Spacing between frames (inner padding)
        int margin = 0;              // Margin around spritesheet edges (outer padding)
        
        Hotspot hotspot;             // Center point for rendering (default: center)

        SpriteSheet() = default;
    };

    // ========================================================================
    // Animation Event - Triggered at specific frames during playback
    // ========================================================================
    struct AnimationEvent
    {
        int frame = 0;               // Frame number to trigger on
        std::string type;            // Event type: "sound", "hitbox", "vfx", "gamelogic"
        std::string dataJson;        // JSON string with event-specific data

        AnimationEvent() = default;
        AnimationEvent(int f, const std::string& t, const std::string& d)
            : frame(f), type(t), dataJson(d) {}
    };

    // ========================================================================
    // Animation - Defines a single animation sequence
    // ========================================================================
    struct Animation
    {
        std::string name;            // Animation name (e.g., "Idle", "Walk")
        std::string spritesheetId;   // Reference to spritesheet ID
        
        int startFrame = 0;          // First frame index
        int endFrame = 0;            // Last frame index
        float framerate = 12.0f;     // Frames per second
        bool looping = true;         // Should animation loop?
        
        std::vector<AnimationEvent> events; // Events triggered during playback

        // Helper: Get total number of frames
        int GetFrameCount() const { return endFrame - startFrame + 1; }
        
        // Helper: Get duration in seconds
        float GetDuration() const { return GetFrameCount() / framerate; }

        Animation() = default;
    };

    // ========================================================================
    // AnimationBank - Container for all animation data
    // ========================================================================
    class AnimationBank
    {
    public:
        AnimationBank() = default;
        ~AnimationBank() = default;

        // Load animation bank from JSON file
        bool LoadFromFile(const std::string& filePath);

        // Get animation by name
        Animation* GetAnimation(const std::string& name);
        const Animation* GetAnimation(const std::string& name) const;

        // Get spritesheet by ID
        SpriteSheet* GetSpriteSheet(const std::string& id);
        const SpriteSheet* GetSpriteSheet(const std::string& id) const;

        // Get bank name
        const std::string& GetBankName() const { return m_bankName; }

        // Check if bank is valid
        bool IsValid() const { return m_isValid; }

    private:
        std::string m_bankName;
        std::string m_description;
        bool m_isValid = false;

        std::unordered_map<std::string, SpriteSheet> m_spritesheets;
        std::unordered_map<std::string, Animation> m_animations;

        // Helper: Parse JSON
        bool ParseJSON(const std::string& jsonContent);
    };

} // namespace OlympeAnimation
