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
     * @param bankId Unique identifier for the bank
     * @param filePath Path to JSON file
     * @return true if loaded successfully
     */
    bool LoadAnimationBank(const std::string& bankId, const std::string& filePath);
    
    /**
     * @brief Load all animation banks from a directory
     * @param directoryPath Path to directory containing animation bank JSON files
     * @return Number of banks loaded
     */
    int LoadAnimationBanksFromDirectory(const std::string& directoryPath);
    
    /**
     * @brief Load a single animation graph from JSON file
     * @param graphId Unique identifier for the graph
     * @param filePath Path to JSON file
     * @return true if loaded successfully
     */
    bool LoadAnimationGraph(const std::string& graphId, const std::string& filePath);
    
    /**
     * @brief Load all animation graphs from a directory
     * @param directoryPath Path to directory containing animation graph JSON files
     * @return Number of graphs loaded
     */
    int LoadAnimationGraphsFromDirectory(const std::string& directoryPath);
    
    /**
     * @brief Get animation bank by ID
     * @param bankId Bank identifier
     * @return Pointer to bank or nullptr if not found
     */
    const AnimationBank* GetAnimationBank(const std::string& bankId) const;
    
    /**
     * @brief Get animation graph by ID
     * @param graphId Graph identifier
     * @return Pointer to graph or nullptr if not found
     */
    const AnimationGraph* GetAnimationGraph(const std::string& graphId) const;
    
    /**
     * @brief Get animation sequence from a bank
     * @param bankId Bank identifier
     * @param animName Animation name
     * @return Pointer to sequence or nullptr if not found
     */
    const AnimationSequence* GetAnimationSequence(const std::string& bankId, 
                                                   const std::string& animName) const;
    
    /**
     * @brief Check if animation bank is loaded
     * @param bankId Bank identifier
     * @return true if bank exists
     */
    bool HasAnimationBank(const std::string& bankId) const;
    
    /**
     * @brief Check if animation graph is loaded
     * @param graphId Graph identifier
     * @return true if graph exists
     */
    bool HasAnimationGraph(const std::string& graphId) const;
    
    /**
     * @brief Clear all loaded animation data
     */
    void Clear();
    
    /**
     * @brief Debug: List all loaded animation banks
     */
    void ListLoadedBanks() const;
    
    /**
     * @brief Debug: List all loaded animation graphs
     */
    void ListLoadedGraphs() const;
    
private:
    AnimationManager() = default;
    ~AnimationManager() = default;
    
    // Storage
    std::unordered_map<std::string, std::unique_ptr<AnimationBank>> m_banks;
    std::unordered_map<std::string, std::unique_ptr<AnimationGraph>> m_graphs;
};

} // namespace Olympe
