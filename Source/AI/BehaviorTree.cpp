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
#include "../third_party/nlohmann/json.hpp"
#include <fstream>
#include <cmath>

using json = nlohmann::json;

// --- BehaviorTreeManager Implementation ---

bool BehaviorTreeManager::LoadTreeFromFile(const std::string& filepath, uint32_t treeId)
{
    try
    {
        // Read JSON file
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            SYSTEM_LOG << "BehaviorTreeManager: Failed to open file: " << filepath << "\n";
            return false;
        }
        
        json j;
        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        j = json::parse(jsonStr);
        file.close();
        
        // Parse behavior tree
        BehaviorTreeAsset tree;
        tree.id = treeId;
        tree.name = j.contains("name") && j["name"].is_string() ? j["name"].get<std::string>() : "Unnamed Tree";
        
        if (j.contains("rootNodeId") && j["rootNodeId"].is_number())
            tree.rootNodeId = static_cast<uint32_t>(j["rootNodeId"].get<int>());
        else
            tree.rootNodeId = 0;
        
        // Parse nodes
        if (j.contains("nodes") && j["nodes"].is_array())
        {
            for (size_t i = 0; i < j["nodes"].size(); ++i)
            {
                const auto& nodeJson = j["nodes"][i];
                BTNode node;
                node.id = nodeJson.contains("id") ? nodeJson["id"].get<int>() : 0;
                
                if (nodeJson.contains("name") && nodeJson["name"].is_string())
                    node.name = nodeJson["name"].get<std::string>();
                else
                    node.name = "";
                
                std::string typeStr = nodeJson.contains("type") && nodeJson["type"].is_string() ? nodeJson["type"].get<std::string>() : "Action";
                if (typeStr == "Selector") node.type = BTNodeType::Selector;
                else if (typeStr == "Sequence") node.type = BTNodeType::Sequence;
                else if (typeStr == "Condition") node.type = BTNodeType::Condition;
                else if (typeStr == "Action") node.type = BTNodeType::Action;
                else if (typeStr == "Inverter") node.type = BTNodeType::Inverter;
                else if (typeStr == "Repeater") node.type = BTNodeType::Repeater;
                
                // Parse child IDs for composite nodes
                if (nodeJson.contains("children") && nodeJson["children"].is_array())
                {
                    size_t childCount = nodeJson["children"].size();
                    for (size_t j = 0; j < childCount; ++j)
                    {
                        const auto& childId = nodeJson["children"][j];
                        node.childIds.push_back(childId.get<uint32_t>());
                    }
                }
                
                // Parse condition type
                if (node.type == BTNodeType::Condition && nodeJson.contains("conditionType"))
                {
                    std::string condStr = nodeJson["conditionType"].get<std::string>();
                    if (condStr == "TargetVisible") node.conditionType = BTConditionType::TargetVisible;
                    else if (condStr == "TargetInRange") node.conditionType = BTConditionType::TargetInRange;
                    else if (condStr == "HealthBelow") node.conditionType = BTConditionType::HealthBelow;
                    else if (condStr == "HasMoveGoal") node.conditionType = BTConditionType::HasMoveGoal;
                    else if (condStr == "CanAttack") node.conditionType = BTConditionType::CanAttack;
                    else if (condStr == "HeardNoise") node.conditionType = BTConditionType::HeardNoise;
                    
                    if (nodeJson.contains("param") && nodeJson["param"].is_number())
                        node.conditionParam = static_cast<float>(nodeJson["param"].get<double>());
                    else
                        node.conditionParam = 0.0f;
                }
                
                // Parse action type
                if (node.type == BTNodeType::Action && nodeJson.contains("actionType"))
                {
                    std::string actStr = nodeJson["actionType"].get<std::string>();
                    if (actStr == "SetMoveGoalToLastKnownTargetPos") 
                        node.actionType = BTActionType::SetMoveGoalToLastKnownTargetPos;
                    else if (actStr == "SetMoveGoalToTarget") 
                        node.actionType = BTActionType::SetMoveGoalToTarget;
                    else if (actStr == "SetMoveGoalToPatrolPoint") 
                        node.actionType = BTActionType::SetMoveGoalToPatrolPoint;
                    else if (actStr == "MoveToGoal") 
                        node.actionType = BTActionType::MoveToGoal;
                    else if (actStr == "AttackIfClose") 
                        node.actionType = BTActionType::AttackIfClose;
                    else if (actStr == "PatrolPickNextPoint") 
                        node.actionType = BTActionType::PatrolPickNextPoint;
                    else if (actStr == "ClearTarget") 
                        node.actionType = BTActionType::ClearTarget;
                    else if (actStr == "Idle") 
                        node.actionType = BTActionType::Idle;
                    
                    node.actionParam1 = (nodeJson.contains("param1") && nodeJson["param1"].is_number())
                        ? static_cast<float>(nodeJson["param1"].get<double>()): 0.0f;
                    
                    if (nodeJson.contains("param2") && nodeJson["param2"].is_number())
                        node.actionParam2 = static_cast<float>(nodeJson["param2"].get<double>());
                    else
                        node.actionParam2 = 0.0f;
                }
                
                // Parse decorator child
                if ((node.type == BTNodeType::Inverter || node.type == BTNodeType::Repeater) && 
                    nodeJson.contains("child"))
                {
                    node.decoratorChildId = nodeJson["child"].get<int>();
                }
                
                if (node.type == BTNodeType::Repeater && nodeJson.contains("repeatCount"))
                {
                    node.repeatCount = nodeJson["repeatCount"].get<int>();
                }
                
                tree.nodes.push_back(node);
            }
        }
        
        // Store the tree
        m_trees.push_back(tree);
        
        SYSTEM_LOG << "BehaviorTreeManager: Loaded tree '" << tree.name << "' (ID=" << treeId 
                   << ") with " << tree.nodes.size() << " nodes\n";
        
        return true;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "BehaviorTreeManager: Error loading tree from " << filepath << ": " << e.what() << "\n";
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
