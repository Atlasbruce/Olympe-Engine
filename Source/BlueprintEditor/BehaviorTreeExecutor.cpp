/**
 * @file BehaviorTreeExecutor.cpp
 * @brief Native BehaviorTree execution implementation
 * @author Olympe Engine
 * @date 2026-03-24
 */

#include "BehaviorTreeExecutor.h"
#include "../system/system_utils.h"

namespace Olympe {

BehaviorTreeExecutor::BehaviorTreeExecutor()
    : m_maxDepth(0), m_executedNodes(0)
{
}

BehaviorTreeExecutor::~BehaviorTreeExecutor()
{
}

BTStatus BehaviorTreeExecutor::ExecuteTree(const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer)
{
    outTracer.Reset();
    m_maxDepth = 0;
    m_executedNodes = 0;

    if (btAsset.rootNodeId == 0)
    {
        SYSTEM_LOG << "[BehaviorTreeExecutor] Tree has no root node\n";
        outTracer.RecordError(-1, "", "Tree has no root node", "Critical");
        outTracer.RecordExecutionCompleted(false, "No root node");
        return BTStatus::Failure;
    }

    SYSTEM_LOG << "[BehaviorTreeExecutor] Starting execution of tree: " << btAsset.name << "\n";

    // Execute the root node
    BTStatus result = ExecuteNode(btAsset.rootNodeId, btAsset, outTracer);

    bool success = (result == BTStatus::Success);
    outTracer.RecordExecutionCompleted(success, 
        success ? "Tree executed successfully" : "Tree execution failed");

    SYSTEM_LOG << "[BehaviorTreeExecutor] Tree execution completed: " 
               << StatusToString(result) 
               << " (executed " << m_executedNodes << " nodes)\n";

    return result;
}

BTStatus BehaviorTreeExecutor::ExecuteNode(uint32_t nodeId, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer)
{
    m_maxDepth++;
    if (m_maxDepth > 100)
    {
        SYSTEM_LOG << "[BehaviorTreeExecutor] Recursion limit exceeded (cycle detected?)\n";
        return BTStatus::Failure;
    }

    m_executedNodes++;

    const BTNode* node = btAsset.GetNode(nodeId);
    if (!node)
    {
        SYSTEM_LOG << "[BehaviorTreeExecutor] Node " << nodeId << " not found in tree\n";
        return BTStatus::Failure;
    }

    // Record node entry
    std::string nodeTypeName;
    switch (node->type)
    {
        case BTNodeType::Selector:    nodeTypeName = "Selector"; break;
        case BTNodeType::Sequence:    nodeTypeName = "Sequence"; break;
        case BTNodeType::Condition:   nodeTypeName = "Condition"; break;
        case BTNodeType::Action:      nodeTypeName = "Action"; break;
        case BTNodeType::Inverter:    nodeTypeName = "Inverter"; break;
        case BTNodeType::Repeater:    nodeTypeName = "Repeater"; break;
        case BTNodeType::Root:        nodeTypeName = "Root"; break;
        case BTNodeType::OnEvent:     nodeTypeName = "OnEvent"; break;
        default:                      nodeTypeName = "Unknown"; break;
    }

    outTracer.RecordNodeEntered(static_cast<int32_t>(nodeId), node->name, nodeTypeName);

    BTStatus status = BTStatus::Failure;

    switch (node->type)
    {
        case BTNodeType::Selector:
            status = ExecuteSelector(*node, btAsset, outTracer);
            break;

        case BTNodeType::Sequence:
            status = ExecuteSequence(*node, btAsset, outTracer);
            break;

        case BTNodeType::Condition:
            status = ExecuteCondition(*node, outTracer);
            break;

        case BTNodeType::Action:
            status = ExecuteAction(*node, outTracer);
            break;

        case BTNodeType::Inverter:
        case BTNodeType::Repeater:
            status = ExecuteDecorator(*node, btAsset, outTracer);
            break;

        case BTNodeType::Root:
            // Root node: execute first child (usually a Selector or Sequence)
            if (!node->childIds.empty())
            {
                status = ExecuteNode(node->childIds[0], btAsset, outTracer);
            }
            else
            {
                status = BTStatus::Success;  // Empty root = success
            }
            break;

        case BTNodeType::OnEvent:
            // OnEvent nodes only execute when triggered by events
            // For simulation, treat as success (no event triggered in offline sim)
            status = BTStatus::Success;
            break;

        default:
            status = BTStatus::Failure;
            break;
    }

    m_maxDepth--;
    return status;
}

BTStatus BehaviorTreeExecutor::ExecuteSelector(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer)
{
    // Selector (OR): tries children in order, returns SUCCESS on first success
    // Records which branch was taken
    int childIndex = 0;
    for (uint32_t childId : node.childIds)
    {
        BTStatus childStatus = ExecuteNode(childId, btAsset, outTracer);
        
        if (childStatus == BTStatus::Success)
        {
            // Record branch taken
            outTracer.RecordBranchTaken(static_cast<int32_t>(node.id), 
                                       "Selector child " + std::to_string(childIndex), 
                                       static_cast<int32_t>(childId));
            return BTStatus::Success;
        }
        
        childIndex++;
    }

    // All children failed
    outTracer.RecordBranchTaken(static_cast<int32_t>(node.id), "Selector failed", -1);
    return BTStatus::Failure;
}

BTStatus BehaviorTreeExecutor::ExecuteSequence(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer)
{
    // Sequence (AND): runs all children, returns FAILURE on first failure
    // Records which child failed (if any)
    int childIndex = 0;
    for (uint32_t childId : node.childIds)
    {
        BTStatus childStatus = ExecuteNode(childId, btAsset, outTracer);
        
        if (childStatus != BTStatus::Success)
        {
            // Record failure
            outTracer.RecordBranchTaken(static_cast<int32_t>(node.id), 
                                       "Sequence failed at child " + std::to_string(childIndex), 
                                       static_cast<int32_t>(childId));
            return BTStatus::Failure;
        }
        
        childIndex++;
    }

    // All children succeeded
    outTracer.RecordBranchTaken(static_cast<int32_t>(node.id), "Sequence succeeded", -1);
    return BTStatus::Success;
}

BTStatus BehaviorTreeExecutor::ExecuteCondition(const BTNode& node, GraphExecutionTracer& outTracer)
{
    // Simulate condition evaluation
    // In offline simulation, we make reasonable assumptions:
    // - Most conditions succeed (optimistic)
    // - Some conditions based on entity state fail

    std::string conditionName = node.conditionTypeString;
    if (conditionName.empty())
    {
        switch (node.conditionType)
        {
            case BTConditionType::TargetVisible:     conditionName = "TargetVisible"; break;
            case BTConditionType::TargetInRange:     conditionName = "TargetInRange"; break;
            case BTConditionType::HealthBelow:       conditionName = "HealthBelow"; break;
            case BTConditionType::HasMoveGoal:       conditionName = "HasMoveGoal"; break;
            case BTConditionType::CanAttack:         conditionName = "CanAttack"; break;
            case BTConditionType::HeardNoise:        conditionName = "HeardNoise"; break;
            case BTConditionType::IsWaitTimerExpired: conditionName = "IsWaitTimerExpired"; break;
            case BTConditionType::HasNavigableDestination: conditionName = "HasNavigableDestination"; break;
            case BTConditionType::HasValidPath:      conditionName = "HasValidPath"; break;
            case BTConditionType::HasReachedDestination: conditionName = "HasReachedDestination"; break;
            default: conditionName = "Unknown"; break;
        }
    }

    // In simulation: conditions optimistically succeed
    bool result = true;  // Assume condition passes
    
    outTracer.RecordConditionEvaluated(static_cast<int32_t>(node.id), 
                                      conditionName, result, 
                                      "Parameter: " + std::to_string(node.conditionParam));

    return result ? BTStatus::Success : BTStatus::Failure;
}

BTStatus BehaviorTreeExecutor::ExecuteAction(const BTNode& node, GraphExecutionTracer& outTracer)
{
    // Simulate action execution
    // In offline simulation, actions always succeed (no runtime effects)

    std::string actionName = "Action";
    switch (node.actionType)
    {
        case BTActionType::SetMoveGoalToLastKnownTargetPos: actionName = "SetMoveGoalToLastKnownTargetPos"; break;
        case BTActionType::SetMoveGoalToTarget:      actionName = "SetMoveGoalToTarget"; break;
        case BTActionType::SetMoveGoalToPatrolPoint: actionName = "SetMoveGoalToPatrolPoint"; break;
        case BTActionType::MoveToGoal:               actionName = "MoveToGoal"; break;
        case BTActionType::AttackIfClose:            actionName = "AttackIfClose"; break;
        case BTActionType::PatrolPickNextPoint:      actionName = "PatrolPickNextPoint"; break;
        case BTActionType::ClearTarget:              actionName = "ClearTarget"; break;
        case BTActionType::Idle:                     actionName = "Idle"; break;
        case BTActionType::WaitRandomTime:           actionName = "WaitRandomTime"; break;
        case BTActionType::ChooseRandomNavigablePoint: actionName = "ChooseRandomNavigablePoint"; break;
        case BTActionType::RequestPathfinding:       actionName = "RequestPathfinding"; break;
        case BTActionType::FollowPath:               actionName = "FollowPath"; break;
        case BTActionType::SendMessage:              actionName = "SendMessage"; break;
        default: actionName = "Unknown"; break;
    }

    outTracer.RecordDataPinResolved(static_cast<int32_t>(node.id), actionName, 
                                   "Params: " + std::to_string(node.actionParam1) + 
                                   ", " + std::to_string(node.actionParam2));

    // Actions always succeed in simulation
    return BTStatus::Success;
}

BTStatus BehaviorTreeExecutor::ExecuteDecorator(const BTNode& node, const BehaviorTreeAsset& btAsset, GraphExecutionTracer& outTracer)
{
    if (node.decoratorChildId == 0)
    {
        return BTStatus::Failure;  // Decorator with no child = failure
    }

    BTStatus childStatus = ExecuteNode(node.decoratorChildId, btAsset, outTracer);

    if (node.type == BTNodeType::Inverter)
    {
        // Inverter: flip the result
        return (childStatus == BTStatus::Success) ? BTStatus::Failure : BTStatus::Success;
    }
    else if (node.type == BTNodeType::Repeater)
    {
        // Repeater: repeat child N times, return success if last iteration succeeds
        for (int i = 1; i < node.repeatCount; ++i)
        {
            childStatus = ExecuteNode(node.decoratorChildId, btAsset, outTracer);
        }
        return childStatus;
    }

    return childStatus;
}

const char* BehaviorTreeExecutor::StatusToString(BTStatus status)
{
    switch (status)
    {
        case BTStatus::Idle:    return "Idle";
        case BTStatus::Running: return "Running";
        case BTStatus::Success: return "Success";
        case BTStatus::Failure: return "Failure";
        case BTStatus::Aborted: return "Aborted";
        default:                return "Unknown";
    }
}

} // namespace Olympe
