/**
 * @file BehaviorTree.h
 * @brief Data-driven behavior tree system for AI decision making
 * @author Nicolas Chereau
 * @date 2025
 * @version 2.0
 * 
 * @details
 * This file implements a behavior tree system for creating complex AI behaviors.
 * Behavior trees are hierarchical structures that make decisions based on conditions
 * and execute actions based on those decisions.
 * 
 * Key features:
 * - Composite nodes: Selector (OR), Sequence (AND)
 * - Decorator nodes: Inverter, Repeater
 * - Condition nodes: State checking (health, target, etc.)
 * - Action nodes: Behaviors (move, attack, patrol, etc.)
 * - JSON-based tree definitions
 * - Per-entity tree execution
 * 
 * @note Behavior Tree purpose: Data-driven behavior tree system for AI decision making.
 * 
 * @example
 * @code
 * // Load behavior tree from JSON
 * BehaviorTree::LoadTreeForEntity(npcEntity, "Blueprints/BehaviorTrees/Patrol.json");
 * 
 * // Update tree each frame
 * BehaviorTree::UpdateEntity(npcEntity, deltaTime);
 * @endcode
 */

#pragma once

#include "../ECS_Entity.h"
#include "../vector.h"
#include <vector>
#include <string>
#include <cstdint>
#include <map>

// Forward declarations
struct AIBlackboard_data;

/**
 * @enum BTNodeType
 * @brief Behavior tree node types
 * 
 * Defines the different types of nodes that can exist in a behavior tree.
 */
enum class BTNodeType : uint8_t
{
    Selector = 0,       ///< OR node - succeeds if any child succeeds
    Sequence,           ///< AND node - succeeds if all children succeed
    Condition,          ///< Leaf node - checks a condition
    Action,             ///< Leaf node - performs an action
    Inverter,           ///< Decorator - inverts child result
    Repeater            ///< Decorator - repeats child N times
};

/**
 * @enum BTStatus
 * @brief Behavior tree node execution status
 * 
 * Represents the current state of a behavior tree node.
 */
enum class BTStatus : uint8_t
{
    Running = 0,        ///< Node is still executing
    Success,            ///< Node completed successfully
    Failure             ///< Node failed
};

/**
 * @enum BTConditionType
 * @brief Built-in condition types for behavior trees
 * 
 * Predefined conditions that can be checked during tree execution.
 */
enum class BTConditionType : uint8_t
{
    TargetVisible = 0,              ///< Can see target entity
    TargetInRange,                  ///< Target within specified range
    HealthBelow,                    ///< Health below threshold
    HasMoveGoal,                    ///< Movement goal is set
    CanAttack,                      ///< Attack is available
    HeardNoise,                     ///< Detected noise
    // NEW: Wander behavior conditions
    IsWaitTimerExpired,             ///< Wait timer expired?
    HasNavigableDestination,        ///< Navigable destination chosen?
    HasValidPath,                   ///< Valid path calculated?
    HasReachedDestination,          ///< Reached destination?
    // Catalog aliases for better readability
    HasTarget = TargetVisible,          ///< Alias for HasTarget condition
    IsTargetInAttackRange = TargetInRange  ///< Alias for range check
};

/**
 * @enum BTActionType
 * @brief Built-in action types for behavior trees
 * 
 * Predefined actions that can be executed during tree execution.
 */
enum class BTActionType : uint8_t
{
    SetMoveGoalToLastKnownTargetPos = 0, ///< Move to last seen target position
    SetMoveGoalToTarget,                  ///< Move towards current target
    SetMoveGoalToPatrolPoint,             ///< Move to next patrol waypoint
    MoveToGoal,                           ///< Execute movement to goal
    AttackIfClose,                        ///< Attack if in range
    PatrolPickNextPoint,                  ///< Select next patrol point
    ClearTarget,                          ///< Clear current target
    Idle,                                 ///< Do nothing
    // NEW: Wander behavior actions
    WaitRandomTime,                       ///< Initialize random timer (param1=min, param2=max)
    ChooseRandomNavigablePoint,           ///< Choose navigable point (param1=searchRadius, param2=maxAttempts)
    RequestPathfinding,                   ///< Request pathfinding to moveGoal via MoveIntent
    FollowPath,                           ///< Follow the path (check progression)
    // Catalog aliases for better readability
    MoveTo = MoveToGoal,                  ///< Alias for MoveTo action
    AttackMelee = AttackIfClose           ///< Alias for melee attack
};

/**
 * @struct BTNode
 * @brief Represents a single node in a behavior tree
 * 
 * Can be a composite, decorator, condition, or action node.
 * Stores node type, parameters, and child references.
 */
struct BTNode
{
    BTNodeType type = BTNodeType::Action;   ///< Node type
    uint32_t id = 0;                        ///< Unique node ID within tree
    
    // For composite nodes (Selector, Sequence)
    std::vector<uint32_t> childIds;         ///< IDs of child nodes
    
    // For condition nodes
    BTConditionType conditionType = BTConditionType::TargetVisible;  ///< Condition type
    float conditionParam = 0.0f;            ///< Generic parameter for conditions
    
    // For action nodes
    BTActionType actionType = BTActionType::Idle;  ///< Action type
    float actionParam1 = 0.0f;              ///< Generic parameter 1 for actions
    float actionParam2 = 0.0f;              ///< Generic parameter 2 for actions
    
    // For decorator nodes
    uint32_t decoratorChildId = 0;
    int repeatCount = 1;                    // For Repeater decorator
    
    // Debug info
    std::string name;
};

// --- Behavior Tree Asset ---
struct BehaviorTreeAsset
{
    uint32_t id = 0;                        // Unique tree ID
    std::string name;
    std::vector<BTNode> nodes;
    uint32_t rootNodeId = 0;
    
    // Helper: get node by ID
    BTNode* GetNode(uint32_t nodeId)
    {
        for (auto& node : nodes)
        {
            if (node.id == nodeId)
                return &node;
        }
        return nullptr;
    }
    
    const BTNode* GetNode(uint32_t nodeId) const
    {
        for (const auto& node : nodes)
        {
            if (node.id == nodeId)
                return &node;
        }
        return nullptr;
    }
};

// --- Behavior Tree Manager ---
// Singleton manager for loading and caching behavior tree assets
class BehaviorTreeManager
{
public:
    static BehaviorTreeManager& Get()
    {
        static BehaviorTreeManager instance;
        return instance;
    }
    
    // Load a behavior tree from JSON file
    bool LoadTreeFromFile(const std::string& filepath, uint32_t treeId);
    
    // Reload a behavior tree from JSON file (hot-reload support)
    bool ReloadTree(uint32_t treeId);
    
    // Validate a behavior tree structure
    bool ValidateTree(const BehaviorTreeAsset& tree, std::string& errorMessage) const;
    
    // Get a loaded tree by ID
    const BehaviorTreeAsset* GetTree(uint32_t treeId) const;
    
    // Clear all loaded trees
    void Clear();
    
    // NEW: Get tree ID from path (for prefab instantiation)
    uint32_t GetTreeIdFromPath(const std::string& treePath) const;
    
    // NEW: Check if tree is already loaded by path
    bool IsTreeLoadedByPath(const std::string& treePath) const;
    
    // NEW: Get loaded tree by path
    const BehaviorTreeAsset* GetTreeByPath(const std::string& treePath) const;
    
    // NEW: Enhanced lookup that tries multiple strategies
    const BehaviorTreeAsset* GetTreeByAnyId(uint32_t treeId) const;
    
    // NEW: Get tree path from ID (reverse lookup)
    std::string GetTreePathFromId(uint32_t treeId) const;
    
    // NEW: Debug method to list all loaded trees
    void DebugPrintLoadedTrees() const;
    
private:
    BehaviorTreeManager() = default;
    std::vector<BehaviorTreeAsset> m_trees;
    
    // NEW: Registry to map file paths to tree IDs
    std::map<std::string, uint32_t> m_pathToIdMap;
};

// --- Behavior Tree Execution ---
// Execute a single node of a behavior tree
BTStatus ExecuteBTNode(const BTNode& node, EntityID entity, AIBlackboard_data& blackboard, const BehaviorTreeAsset& tree);

// Execute built-in condition nodes
BTStatus ExecuteBTCondition(BTConditionType condType, float param, EntityID entity, const AIBlackboard_data& blackboard);

// Execute built-in action nodes
BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, EntityID entity, AIBlackboard_data& blackboard);
