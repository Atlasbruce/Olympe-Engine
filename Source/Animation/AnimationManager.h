/**
 * @file AnimationManager.h
 * @brief Singleton manager for animation banks and graphs
 * @author Nicolas Chereau
 * @date 2025
 * 
 * Manages loading and access to animation banks and animation graphs (FSMs).
 * Integrates with DataManager for texture loading to avoid duplication.
 */

#pragma once

#include "AnimationTypes.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Olympe
{

/**
 * @brief Singleton manager for animation resources
 * 
 * Handles loading animation banks and graphs from JSON files.
 * Provides thread-safe access to animation data.
 * Integrates with DataManager for texture management.
 */
class AnimationManager
{
public:
    // Singleton access
    static AnimationManager& Get()
    {
        static AnimationManager instance;
        return instance;
    }
    
    // Delete copy/move constructors
    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;
    AnimationManager(AnimationManager&&) = delete;
    AnimationManager& operator=(AnimationManager&&) = delete;
    
    /**
     * @brief Load a single animation bank from JSON file
     * @param bankJsonPath Path to JSON file
     * @return true if loaded successfully
     */
    bool LoadAnimationBank(const std::string& bankJsonPath);
    
    /**
     * @brief Load all animation banks from a directory
     * @param directoryPath Path to directory containing animation bank JSON files
     * @return Number of banks loaded
     */
    bool LoadAnimationBanksFromDirectory(const std::string& directoryPath);
    
    /**
     * @brief Load a single animation graph from JSON file
     * @param graphJsonPath Path to JSON file
     * @return true if loaded successfully
     */
    bool LoadAnimationGraph(const std::string& graphJsonPath);
    
    /**
     * @brief Load all animation graphs from a directory
     * @param directoryPath Path to directory containing animation graph JSON files
     * @return Number of graphs loaded
     */
    bool LoadAnimationGraphsFromDirectory(const std::string& directoryPath);
    
    /**
     * @brief Get animation bank by ID
     * @param id Bank identifier
     * @return Pointer to bank or nullptr if not found
     */
    const Olympe::AnimationBank* GetBank(const std::string& id) const;
    
    /**
     * @brief Get animation graph by ID
     * @param id Graph identifier
     * @return Pointer to graph or nullptr if not found
     */
    const Olympe::AnimationGraph* GetGraph(const std::string& id) const;
    
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


    /**
     * @brief List all loaded animation banks (debug)
     */
    void ListLoadedBanks() const;
    
    /**
     * @brief List all loaded animation graphs (debug)
     */
    void ListLoadedGraphs() const;

private:
    AnimationManager() = default;
    ~AnimationManager() = default;
    
    bool ParseAnimationBankJSON(const std::string& jsonPath, Olympe::AnimationBank& outBank);
    bool ParseAnimationGraphJSON(const std::string& jsonPath, Olympe::AnimationGraph& outGraph);
    
    std::unordered_map<std::string, Olympe::AnimationBank> m_banks;
    std::unordered_map<std::string, Olympe::AnimationGraph> m_graphs;
};

} // namespace Olympe
