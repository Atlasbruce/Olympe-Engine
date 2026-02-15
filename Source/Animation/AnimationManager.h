/**
 * @file AnimationManager.h
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
