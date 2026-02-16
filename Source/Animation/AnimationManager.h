/*
Olympe Engine V2 2025
Animation System - Animation Manager

Purpose:
- Singleton manager for loading and caching animation banks and graphs
- Provides centralized access to animation resources
- Integrates with DataManager for texture loading
*/

#pragma once

#include "AnimationBank.h"
#include "AnimationGraph.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationManager - Singleton for managing animation resources
    // ========================================================================
    class AnimationManager
    {
    public:
        // Singleton access
        static AnimationManager& GetInstance()
        {
            static AnimationManager instance;
            return instance;
        }
        static AnimationManager& Get() { return GetInstance(); }

        // Initialize the manager
        void Init();

        // Load animation banks from a directory
        void LoadAnimationBanks(const std::string& directoryPath);

        // Load animation graphs from a directory
        void LoadAnimationGraphs(const std::string& directoryPath);

        // Load a specific animation bank
        bool LoadAnimationBank(const std::string& filePath);

        // Load a specific animation graph
        bool LoadAnimationGraph(const std::string& filePath);

        // Get animation bank by name
        AnimationBank* GetBank(const std::string& bankName);
        const AnimationBank* GetBank(const std::string& bankName) const;

        // Get animation graph by name
        AnimationGraph* GetGraph(const std::string& graphName);
        const AnimationGraph* GetGraph(const std::string& graphName) const;

        /**
        * @brief Get animation sequence from a bank by name
        * @param bankId Animation bank identifier
        * @param animName Animation name within the bank
        * @return Pointer to AnimationSequence if found, nullptr otherwise
        *
        * Helper function to avoid manual bank lookup + animation lookup.
        *
        * Example:
        * @code
        * auto* seq = AnimationManager::Get().GetAnimationSequence("player", "walk");
        * if (seq) {
        *     // Use sequence
        * }
        * @endcode
        */
        const Olympe::AnimationSequence* GetAnimationSequence(const std::string& bankId, const std::string& animName) const;

        /**
         * @brief Check if animation exists in bank
         * @param bankId Bank identifier
         * @param animName Animation name
         * @return true if animation exists
         */
        bool HasAnimation(const std::string& bankId, const std::string& animName) const;

        // Shutdown and cleanup
        void Shutdown();

    private:
        AnimationManager() = default;
        ~AnimationManager() = default;

        // Delete copy constructor and assignment operator (singleton)
        AnimationManager(const AnimationManager&) = delete;
        AnimationManager& operator=(const AnimationManager&) = delete;

        bool m_initialized = false;

        std::unordered_map<std::string, std::unique_ptr<AnimationBank>> m_banks;
        std::unordered_map<std::string, std::unique_ptr<AnimationGraph>> m_graphs;

        // Helper: Scan directory for JSON files
        std::vector<std::string> ScanDirectory(const std::string& directoryPath);
    };

} // namespace OlympeAnimation
