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
 * @brief Singleton manager for animation banks and state machines
 * @author Olympe Engine Team
 * @date 2026
 * 
 * AnimationManager loads and provides access to animation data from JSON files.
 * Integrates with DataManager for texture loading and caching.
 */

#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "AnimationTypes.h"
#include "Singleton.h"
#include <string>
#include <map>
#include <memory>

namespace Olympe {
namespace Animation {

/**
 * @class AnimationManager
 * @brief Singleton manager for animation banks and state machines
 * 
 * AnimationManager is responsible for:
 * - Loading animation banks from JSON files
 * - Loading animation graphs (FSM) from JSON files
 * - Caching and providing access to loaded resources
 * - Batch loading directories of animations
 * 
 * Usage pattern:
 * @code
 * // At startup
 * AnimationManager::Get().LoadAnimationBanksFromDirectory("Gamedata/Animations/AnimationBanks/");
 * AnimationManager::Get().LoadAnimationGraphsFromDirectory("Gamedata/Animations/AnimationGraphs/");
 * 
 * // In game code
 * auto bank = AnimationManager::Get().GetBank("player");
 * if (bank) {
 *     auto anim = bank->GetAnimation("walk");
 * }
 * @endcode
 * 
 * @see AnimationBank
 * @see AnimationGraph
 */
class AnimationManager : public Singleton<AnimationManager>
{
public:
    /**
     * @brief Load animation bank from JSON file
     * @param bankJsonPath Path to animation bank JSON file (relative to executable)
     * @return true if loaded successfully, false otherwise
     * 
     * Loads an animation bank definition from a JSON file. The bank must have a unique
     * ID. If a bank with the same ID is already loaded, it will be replaced.
     * 
     * Example:
     * @code
     * bool success = AnimationManager::Get().LoadAnimationBank(
     *     "Gamedata/Animations/AnimationBanks/player.json"
     * );
     * if (!success) {
     *     std::cerr << "Failed to load player animations" << std::endl;
     * }
     * @endcode
     * 
     * @note This function validates JSON structure and logs errors to console
     * @see LoadAnimationBanksFromDirectory
     */
    bool LoadAnimationBank(const std::string& bankJsonPath);
    
    /**
     * @brief Batch load all animation banks from a directory
     * @param directoryPath Path to directory containing .json files
     * @return Number of banks successfully loaded
     * 
     * Scans the directory for .json files and attempts to load each one as an
     * animation bank. Continues on error (doesn't stop at first failure).
     * 
     * Example:
     * @code
     * int count = AnimationManager::Get().LoadAnimationBanksFromDirectory(
     *     "Gamedata/Animations/AnimationBanks/"
     * );
     * std::cout << "Loaded " << count << " animation banks" << std::endl;
     * @endcode
     * 
     * @note Logs success/failure for each file to console
     * @see LoadAnimationBank
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
     * @brief Load animation graph (FSM) from JSON file
     * @param graphJsonPath Path to animation graph JSON file
     * @return true if loaded successfully, false otherwise
     * 
     * Loads an animation graph definition from a JSON file. Validates state
     * transitions and warns about unreachable states.
     * 
     * Example:
     * @code
     * bool success = AnimationManager::Get().LoadAnimationGraph(
     *     "Gamedata/Animations/AnimationGraphs/player_fsm.json"
     * );
     * @endcode
     * 
     * @note Validates that defaultState references a valid state
     * @note Validates that all transitions reference valid states
     * @see LoadAnimationGraphsFromDirectory
     */
    bool LoadAnimationGraph(const std::string& graphJsonPath);
    
    /**
     * @brief Batch load all animation graphs from a directory
     * @param directoryPath Path to directory containing .json files
     * @return Number of graphs successfully loaded
     * 
     * Example:
     * @code
     * int count = AnimationManager::Get().LoadAnimationGraphsFromDirectory(
     *     "Gamedata/Animations/AnimationGraphs/"
     * );
     * @endcode
     * 
     * @see LoadAnimationGraph
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
     * @param bankId Unique identifier of the bank
     * @return Shared pointer to AnimationBank if found, nullptr otherwise
     * 
     * Example:
     * @code
     * auto bank = AnimationManager::Get().GetBank("player");
     * if (bank) {
     *     std::cout << "Bank has " << bank->animations.size() << " animations" << std::endl;
     * } else {
     *     std::cerr << "Bank 'player' not found" << std::endl;
     * }
     * @endcode
     * 
     * @note Returns nullptr if bank not loaded
     * @see LoadAnimationBank
     */
    std::shared_ptr<AnimationBank> GetBank(const std::string& bankId) const;
    
    /**
     * @brief Get animation graph by ID
     * @param graphId Unique identifier of the graph
     * @return Shared pointer to AnimationGraph if found, nullptr otherwise
     * 
     * Example:
     * @code
     * auto graph = AnimationManager::Get().GetGraph("player_fsm");
     * if (graph) {
     *     std::cout << "Default state: " << graph->defaultState << std::endl;
     * }
     * @endcode
     * 
     * @see LoadAnimationGraph
     */
    std::shared_ptr<AnimationGraph> GetGraph(const std::string& graphId) const;
    
    /**
     * @brief List all loaded animation banks (debug)
     * 
     * Prints bank IDs and animation counts to console for debugging purposes.
     * 
     * Example output:
     * @code
     * [AnimationManager] Loaded Animation Banks:
     *   - player (4 animations)
     *   - enemy_goblin (6 animations)
     * @endcode
     * 
     * @see ListLoadedGraphs
     */
    void ListLoadedBanks() const;
    
    /**
     * @brief List all loaded animation graphs (debug)
     * 
     * Prints graph IDs, state counts, and default states to console.
     * 
     * Example output:
     * @code
     * [AnimationManager] Loaded Animation Graphs:
     *   - player_fsm (8 states, default: idle)
     *   - enemy_fsm (5 states, default: patrol)
     * @endcode
     * 
     * @see ListLoadedBanks
     */
    void ListLoadedGraphs() const;
    
    /**
     * @brief Unload an animation bank (free memory)
     * @param bankId ID of bank to unload
     * @return true if bank was found and unloaded, false otherwise
     * 
     * Warning: Entities referencing this bank will have invalid pointers.
     * Ensure no entities use the bank before unloading.
     * 
     * Example:
     * @code
     * // Unload level-specific animations
     * AnimationManager::Get().UnloadBank("level1_enemies");
     * @endcode
     * 
     * @see UnloadAllBanks
     */
    bool UnloadBank(const std::string& bankId);
    
    /**
     * @brief Unload all animation banks
     * 
     * Clears all loaded banks. Use when exiting game or switching to a new level
     * that uses completely different assets.
     * 
     * Example:
     * @code
     * // Clean up when exiting game
     * AnimationManager::Get().UnloadAllBanks();
     * @endcode
     * 
     * @see UnloadBank
     */
    void UnloadAllBanks();

private:
    /// Map of animation banks by ID
    std::map<std::string, std::shared_ptr<AnimationBank>> m_banks;
    
    /// Map of animation graphs by ID
    std::map<std::string, std::shared_ptr<AnimationGraph>> m_graphs;
};

} // namespace Animation
} // namespace Olympe

#endif // ANIMATION_MANAGER_H
