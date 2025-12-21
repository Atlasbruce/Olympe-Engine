/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Behavior Tree purpose: Data-driven behavior tree system for AI decision making.

*/

#pragma once

#include "../Ecs_Entity.h"
#include "../vector.h"
#include <vector>
#include <string>
#include <cstdint>

// Forward declarations
struct AIBlackboard_data;

// --- Behavior Tree Node Types ---
enum class BTNodeType : uint8_t
{
    Selector = 0,       // OR node - succeeds if any child succeeds
    Sequence,           // AND node - succeeds if all children succeed
    Condition,          // Leaf node - checks a condition
    Action,             // Leaf node - performs an action
    Inverter,           // Decorator - inverts child result
    Repeater            // Decorator - repeats child N times
};

// --- Behavior Tree Node Status ---
enum class BTStatus : uint8_t
{
    Running = 0,        // Node is still executing
    Success,            // Node completed successfully
    Failure             // Node failed
};

// --- Built-in Condition Types ---
enum class BTConditionType : uint8_t
{
    TargetVisible = 0,
    TargetInRange,
    HealthBelow,
    HasMoveGoal,
    CanAttack,
    HeardNoise
};

// --- Built-in Action Types ---
enum class BTActionType : uint8_t
{
    SetMoveGoalToLastKnownTargetPos = 0,
    SetMoveGoalToTarget,
    SetMoveGoalToPatrolPoint,
    MoveToGoal,
    AttackIfClose,
    PatrolPickNextPoint,
    ClearTarget,
    Idle
};

// --- Behavior Tree Node ---
struct BTNode
{
    BTNodeType type = BTNodeType::Action;
    uint32_t id = 0;                        // Unique node ID within tree
    
    // For composite nodes (Selector, Sequence)
    std::vector<uint32_t> childIds;
    
    // For condition nodes
    BTConditionType conditionType = BTConditionType::TargetVisible;
    float conditionParam = 0.0f;            // Generic parameter for conditions
    
    // For action nodes
    BTActionType actionType = BTActionType::Idle;
    float actionParam1 = 0.0f;              // Generic parameter 1 for actions
    float actionParam2 = 0.0f;              // Generic parameter 2 for actions
    
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
    
    // Get a loaded tree by ID
    const BehaviorTreeAsset* GetTree(uint32_t treeId) const;
    
    // Clear all loaded trees
    void Clear();
    
private:
    BehaviorTreeManager() = default;
    std::vector<BehaviorTreeAsset> m_trees;
};

// --- Behavior Tree Execution ---
// Execute a single node of a behavior tree
BTStatus ExecuteBTNode(const BTNode& node, EntityID entity, AIBlackboard_data& blackboard, const BehaviorTreeAsset& tree);

// Execute built-in condition nodes
BTStatus ExecuteBTCondition(BTConditionType condType, float param, EntityID entity, const AIBlackboard_data& blackboard);

// Execute built-in action nodes
BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, EntityID entity, AIBlackboard_data& blackboard);
