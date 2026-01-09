/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

Behavior Tree implementation: JSON loading and built-in node execution.

*/

#include "BehaviorTree.h"
#include "../ECS_Components_AI.h"
#include "../ECS_Components.h"
#include "../World.h"
#include "../system/system_utils.h"
#include "../json_helper.h"
#include <cmath>

using json = nlohmann::json;

// --- BehaviorTreeManager Implementation ---

bool BehaviorTreeManager::LoadTreeFromFile(const std::string& filepath, uint32_t treeId)
{
    std::cout << "[BehaviorTreeManager] ========================================" << std::endl;
    std::cout << "[BehaviorTreeManager] Loading: " << filepath << std::endl;
    
    try
    {
        // Read JSON file
        json j;
        if (!JsonHelper::LoadJsonFromFile(filepath, j))
        {
            std::cerr << "[BehaviorTreeManager] ERROR: Cannot open " << filepath << std::endl;
            std::cout << "[BehaviorTreeManager] ========================================" << std::endl;
            return false;
        }
        
        std::cout << "[BehaviorTreeManager] File loaded and parsed" << std::endl;
        
        // Detect version
        bool isV2 = j.contains("schema_version") && j["schema_version"].get<int>() == 2;
        std::cout << "[BehaviorTreeManager] Version: " << (isV2 ? "v2" : "v1") << std::endl;
        
        // Parse behavior tree
        BehaviorTreeAsset tree;
        tree.id = treeId;
        
        // Extract data section based on version
        const json* dataSection = &j;
        if (isV2) {
            tree.name = JsonHelper::GetString(j, "name", "Unnamed Tree");
            if (j.contains("data")) {
                dataSection = &j["data"];
                std::cout << "[BehaviorTreeManager] Using v2 data section" << std::endl;
            }
        } else {
            tree.name = JsonHelper::GetString(j, "name", "Unnamed Tree");
            std::cout << "[BehaviorTreeManager] Using v1 flat structure" << std::endl;
        }
        
        tree.rootNodeId = JsonHelper::GetUInt(*dataSection, "rootNodeId", 0);
        std::cout << "[BehaviorTreeManager] Tree name: " << tree.name << ", Root node ID: " << tree.rootNodeId << std::endl;
        
        // Parse nodes
        if (JsonHelper::IsArray(*dataSection, "nodes"))
        {
            const auto& nodesArray = (*dataSection)["nodes"];
            std::cout << "[BehaviorTreeManager] Found " << nodesArray.size() << " nodes" << std::endl;
            
            JsonHelper::ForEachInArray(*dataSection, "nodes", [&tree, isV2](const json& nodeJson, size_t i)
            {
                BTNode node;
                node.id = JsonHelper::GetInt(nodeJson, "id", 0);
                node.name = JsonHelper::GetString(nodeJson, "name", "");
                
                std::string typeStr = JsonHelper::GetString(nodeJson, "type", "Action");
                if (typeStr == "Selector") node.type = BTNodeType::Selector;
                else if (typeStr == "Sequence") node.type = BTNodeType::Sequence;
                else if (typeStr == "Condition") node.type = BTNodeType::Condition;
                else if (typeStr == "Action") node.type = BTNodeType::Action;
                else if (typeStr == "Inverter") node.type = BTNodeType::Inverter;
                else if (typeStr == "Repeater") node.type = BTNodeType::Repeater;
                
                // Parse child IDs for composite nodes
                if (JsonHelper::IsArray(nodeJson, "children"))
                {
                    JsonHelper::ForEachInArray(nodeJson, "children", [&node](const json& childId, size_t j)
                    {
                        node.childIds.push_back(childId.get<uint32_t>());
                    });
                }
                
                // Parse condition type
                if (node.type == BTNodeType::Condition && nodeJson.contains("conditionType"))
                {
                    std::string condStr = JsonHelper::GetString(nodeJson, "conditionType", "");
                    if (condStr == "TargetVisible" || condStr == "HasTarget") 
                        node.conditionType = BTConditionType::TargetVisible;
                    else if (condStr == "TargetInRange" || condStr == "IsTargetInAttackRange") 
                        node.conditionType = BTConditionType::TargetInRange;
                    else if (condStr == "HealthBelow") 
                        node.conditionType = BTConditionType::HealthBelow;
                    else if (condStr == "HasMoveGoal") 
                        node.conditionType = BTConditionType::HasMoveGoal;
                    else if (condStr == "CanAttack") 
                        node.conditionType = BTConditionType::CanAttack;
                    else if (condStr == "HeardNoise") 
                        node.conditionType = BTConditionType::HeardNoise;
                    
                    // For v2 format, check parameters object first
                    if (isV2 && nodeJson.contains("parameters") && nodeJson["parameters"].is_object()) {
                        node.conditionParam = JsonHelper::GetFloat(nodeJson["parameters"], "param", 0.0f);
                    } else {
                        node.conditionParam = JsonHelper::GetFloat(nodeJson, "param", 0.0f);
                    }
                }
                
                // Parse action type
                if (node.type == BTNodeType::Action && nodeJson.contains("actionType"))
                {
                    std::string actStr = JsonHelper::GetString(nodeJson, "actionType", "");
                    if (actStr == "SetMoveGoalToLastKnownTargetPos") 
                        node.actionType = BTActionType::SetMoveGoalToLastKnownTargetPos;
                    else if (actStr == "SetMoveGoalToTarget") 
                        node.actionType = BTActionType::SetMoveGoalToTarget;
                    else if (actStr == "SetMoveGoalToPatrolPoint") 
                        node.actionType = BTActionType::SetMoveGoalToPatrolPoint;
                    else if (actStr == "MoveToGoal" || actStr == "MoveTo") 
                        node.actionType = BTActionType::MoveToGoal;
                    else if (actStr == "AttackIfClose" || actStr == "AttackMelee") 
                        node.actionType = BTActionType::AttackIfClose;
                    else if (actStr == "PatrolPickNextPoint") 
                        node.actionType = BTActionType::PatrolPickNextPoint;
                    else if (actStr == "ClearTarget") 
                        node.actionType = BTActionType::ClearTarget;
                    else if (actStr == "Idle") 
                        node.actionType = BTActionType::Idle;
                    
                    // For v2 format, check parameters object first
                    if (isV2 && nodeJson.contains("parameters") && nodeJson["parameters"].is_object()) {
                        const auto& params = nodeJson["parameters"];
                        node.actionParam1 = JsonHelper::GetFloat(params, "param1", 0.0f);
                        node.actionParam2 = JsonHelper::GetFloat(params, "param2", 0.0f);
                    } else {
                        node.actionParam1 = JsonHelper::GetFloat(nodeJson, "param1", 0.0f);
                        node.actionParam2 = JsonHelper::GetFloat(nodeJson, "param2", 0.0f);
                    }
                }
                
                // Parse decorator child
                if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
                {
                    node.decoratorChildId = JsonHelper::GetInt(nodeJson, "child", 0);
                }
                
                if (node.type == BTNodeType::Repeater)
                {
                    node.repeatCount = JsonHelper::GetInt(nodeJson, "repeatCount", 0);
                }
                
                std::cout << "[BehaviorTreeManager]   Node " << node.id << ": " << node.name << " (" << typeStr << ")" << std::endl;
                tree.nodes.push_back(node);
            });
        }
        else
        {
            std::cerr << "[BehaviorTreeManager] ERROR: No 'nodes' array found in " << filepath << std::endl;
            std::cout << "[BehaviorTreeManager] ========================================" << std::endl;
            return false;
        }
        
        // Store the tree
        m_trees.push_back(tree);
        
        // Validate the tree structure
        std::string validationError;
        if (!ValidateTree(tree, validationError))
        {
            std::cerr << "[BehaviorTreeManager] WARNING: Tree validation failed for '" << tree.name 
                       << "': " << validationError << std::endl;
            // Don't fail loading - allow hot-reload to fix issues
        }
        
        std::cout << "[BehaviorTreeManager] SUCCESS: Loaded '" << tree.name << "' (ID=" << treeId 
                   << ") with " << tree.nodes.size() << " nodes" << std::endl;
        std::cout << "[BehaviorTreeManager] ========================================" << std::endl;
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[BehaviorTreeManager] EXCEPTION: " << e.what() << std::endl;
        std::cout << "[BehaviorTreeManager] ========================================" << std::endl;
        return false;
    }
}

const BehaviorTreeAsset* BehaviorTreeManager::GetTree(uint32_t treeId) const
{
    for (const auto& tree : m_trees)
    {
        if (tree.id == treeId)
            return &tree;
    }
    return nullptr;
}

void BehaviorTreeManager::Clear()
{
    m_trees.clear();
}

bool BehaviorTreeManager::ReloadTree(uint32_t treeId)
{
    // Find the tree
    for (auto& tree : m_trees)
    {
        if (tree.id == treeId)
        {
            // Get the original filepath (we need to store it)
            // For now, reconstruct it from tree name
            std::string filepath = "Blueprints/AI/" + tree.name + ".json";
            
            // Remove old tree
            m_trees.erase(std::remove_if(m_trees.begin(), m_trees.end(),
                [treeId](const BehaviorTreeAsset& t) { return t.id == treeId; }), 
                m_trees.end());
            
            // Load new version
            bool success = LoadTreeFromFile(filepath, treeId);
            if (success)
            {
                SYSTEM_LOG << "BehaviorTreeManager: Hot-reloaded tree ID=" << treeId << "\n";
            }
            return success;
        }
    }
    
    SYSTEM_LOG << "BehaviorTreeManager: Cannot reload tree ID=" << treeId << " (not found)\n";
    return false;
}

bool BehaviorTreeManager::ValidateTree(const BehaviorTreeAsset& tree, std::string& errorMessage) const
{
    errorMessage.clear();
    
    // Check if tree has nodes
    if (tree.nodes.empty())
    {
        errorMessage = "Tree has no nodes";
        return false;
    }
    
    // Check if root node exists
    const BTNode* root = tree.GetNode(tree.rootNodeId);
    if (!root)
    {
        errorMessage = "Root node ID " + std::to_string(tree.rootNodeId) + " not found";
        return false;
    }
    
    // Validate each node
    for (const auto& node : tree.nodes)
    {
        // Check composite nodes have children
        if (node.type == BTNodeType::Selector || node.type == BTNodeType::Sequence)
        {
            if (node.childIds.empty())
            {
                errorMessage = "Composite node '" + node.name + "' (ID=" + std::to_string(node.id) + ") has no children";
                return false;
            }
            
            // Validate all children exist
            for (uint32_t childId : node.childIds)
            {
                if (!tree.GetNode(childId))
                {
                    errorMessage = "Node '" + node.name + "' references missing child ID " + std::to_string(childId);
                    return false;
                }
            }
        }
        
        // Check decorator nodes have a child
        if (node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater)
        {
            if (!tree.GetNode(node.decoratorChildId))
            {
                errorMessage = "Decorator node '" + node.name + "' references missing child ID " + std::to_string(node.decoratorChildId);
                return false;
            }
        }
        
        // Check for duplicate node IDs
        int count = 0;
        for (const auto& other : tree.nodes)
        {
            if (other.id == node.id)
                count++;
        }
        if (count > 1)
        {
            errorMessage = "Duplicate node ID " + std::to_string(node.id);
            return false;
        }
    }
    
    return true;
}

// --- Behavior Tree Execution ---

BTStatus ExecuteBTNode(const BTNode& node, EntityID entity, AIBlackboard_data& blackboard, const BehaviorTreeAsset& tree)
{
    switch (node.type)
    {
        case BTNodeType::Selector:
        {
            // OR node: succeeds if any child succeeds
            for (uint32_t childId : node.childIds)
            {
                const BTNode* child = tree.GetNode(childId);
                if (!child) continue;
                
                BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
                if (status == BTStatus::Success)
                    return BTStatus::Success;
                if (status == BTStatus::Running)
                    return BTStatus::Running;
            }
            return BTStatus::Failure;
        }
        
        case BTNodeType::Sequence:
        {
            // AND node: succeeds if all children succeed
            for (uint32_t childId : node.childIds)
            {
                const BTNode* child = tree.GetNode(childId);
                if (!child) continue;
                
                BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
                if (status == BTStatus::Failure)
                    return BTStatus::Failure;
                if (status == BTStatus::Running)
                    return BTStatus::Running;
            }
            return BTStatus::Success;
        }
        
        case BTNodeType::Condition:
        {
            return ExecuteBTCondition(node.conditionType, node.conditionParam, entity, blackboard);
        }
        
        case BTNodeType::Action:
        {
            return ExecuteBTAction(node.actionType, node.actionParam1, node.actionParam2, entity, blackboard);
        }
        
        case BTNodeType::Inverter:
        {
            const BTNode* child = tree.GetNode(node.decoratorChildId);
            if (!child) return BTStatus::Failure;
            
            BTStatus status = ExecuteBTNode(*child, entity, blackboard, tree);
            if (status == BTStatus::Success)
                return BTStatus::Failure;
            if (status == BTStatus::Failure)
                return BTStatus::Success;
            return status;
        }
        
        case BTNodeType::Repeater:
        {
            // Simplified repeater: just execute once per tick
            const BTNode* child = tree.GetNode(node.decoratorChildId);
            if (!child) return BTStatus::Failure;
            
            return ExecuteBTNode(*child, entity, blackboard, tree);
        }
    }
    
    return BTStatus::Failure;
}

BTStatus ExecuteBTCondition(BTConditionType condType, float param, EntityID entity, const AIBlackboard_data& blackboard)
{
    switch (condType)
    {
        case BTConditionType::TargetVisible:
            return blackboard.targetVisible ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::TargetInRange:
            if (!blackboard.hasTarget) return BTStatus::Failure;
            return (blackboard.distanceToTarget <= param) ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HealthBelow:
            if (World::Get().HasComponent<Health_data>(entity))
            {
                const Health_data& health = World::Get().GetComponent<Health_data>(entity);
                float healthPercent = static_cast<float>(health.currentHealth) / static_cast<float>(health.maxHealth);
                return (healthPercent < param) ? BTStatus::Success : BTStatus::Failure;
            }
            return BTStatus::Failure;
        
        case BTConditionType::HasMoveGoal:
            return blackboard.hasMoveGoal ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::CanAttack:
            return blackboard.canAttack ? BTStatus::Success : BTStatus::Failure;
        
        case BTConditionType::HeardNoise:
            return blackboard.heardNoise ? BTStatus::Success : BTStatus::Failure;
    }
    
    return BTStatus::Failure;
}

BTStatus ExecuteBTAction(BTActionType actionType, float param1, float param2, EntityID entity, AIBlackboard_data& blackboard)
{
    switch (actionType)
    {
        case BTActionType::SetMoveGoalToLastKnownTargetPos:
            blackboard.moveGoal = blackboard.lastKnownTargetPosition;
            blackboard.hasMoveGoal = true;
            return BTStatus::Success;
        
        case BTActionType::SetMoveGoalToTarget:
            if (blackboard.hasTarget && blackboard.targetEntity != INVALID_ENTITY_ID)
            {
                if (World::Get().HasComponent<Position_data>(blackboard.targetEntity))
                {
                    const Position_data& targetPos = World::Get().GetComponent<Position_data>(blackboard.targetEntity);
                    blackboard.moveGoal = targetPos.position;
                    blackboard.hasMoveGoal = true;
                    return BTStatus::Success;
                }
            }
            return BTStatus::Failure;
        
        case BTActionType::SetMoveGoalToPatrolPoint:
            if (blackboard.patrolPointCount > 0)
            {
                int index = static_cast<int>(param1);
                if (index < 0 || index >= blackboard.patrolPointCount)
                    index = blackboard.currentPatrolPoint;
                
                blackboard.moveGoal = blackboard.patrolPoints[index];
                blackboard.hasMoveGoal = true;
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        
        case BTActionType::MoveToGoal:
            if (!blackboard.hasMoveGoal) return BTStatus::Failure;
            
            // Set MoveIntent if component exists
            if (World::Get().HasComponent<MoveIntent_data>(entity))
            {
                MoveIntent_data& intent = World::Get().GetComponent<MoveIntent_data>(entity);
                intent.targetPosition = blackboard.moveGoal;
                intent.desiredSpeed = (param1 > 0.0f) ? param1 : 1.0f;
                intent.hasIntent = true;
                
                // Check if we've arrived
                if (World::Get().HasComponent<Position_data>(entity))
                {
                    Position_data& pos = World::Get().GetComponent<Position_data>(entity);
                    float dist = (pos.position - blackboard.moveGoal).Magnitude();
                    if (dist < intent.arrivalThreshold)
                    {
                        blackboard.hasMoveGoal = false;
                        intent.hasIntent = false;
                        return BTStatus::Success;
                    }
                }
                
                return BTStatus::Running;
            }
            return BTStatus::Failure;
        
        case BTActionType::AttackIfClose:
        {
            float range = (param1 > 0.0f) ? param1 : 50.0f;
            if (blackboard.hasTarget && blackboard.distanceToTarget <= range && blackboard.canAttack)
            {
                // Set AttackIntent if component exists
                if (World::Get().HasComponent<AttackIntent_data>(entity))
                {
                    AttackIntent_data& intent = World::Get().GetComponent<AttackIntent_data>(entity);
                    intent.targetEntity = blackboard.targetEntity;
                    intent.targetPosition = blackboard.lastKnownTargetPosition;
                    intent.range = range;
                    intent.damage = (param2 > 0.0f) ? param2 : 10.0f;
                    intent.hasIntent = true;
                    
                    blackboard.canAttack = false;
                    return BTStatus::Success;
                }
            }
            return BTStatus::Failure;
        }
        
        case BTActionType::PatrolPickNextPoint:
            if (blackboard.patrolPointCount > 0)
            {
                blackboard.currentPatrolPoint = (blackboard.currentPatrolPoint + 1) % blackboard.patrolPointCount;
                blackboard.moveGoal = blackboard.patrolPoints[blackboard.currentPatrolPoint];
                blackboard.hasMoveGoal = true;
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        
        case BTActionType::ClearTarget:
            blackboard.hasTarget = false;
            blackboard.targetEntity = INVALID_ENTITY_ID;
            blackboard.targetVisible = false;
            return BTStatus::Success;
        
        case BTActionType::Idle:
            // Do nothing
            return BTStatus::Success;
    }
    
    return BTStatus::Failure;
}
