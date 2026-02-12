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
