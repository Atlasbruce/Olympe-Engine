/**
 * @file BTGraphCompiler.cpp
 * @brief Implementation of BTGraphCompiler
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTGraphCompiler.h"
#include "BTGraphValidator.h"
#include "BTNodeRegistry.h"
#include "../../system/system_utils.h"

namespace Olympe {
namespace AI {

bool BTGraphCompiler::Compile(
    const NodeGraph::GraphDocument* graph,
    BehaviorTreeAsset& outAsset,
    std::string& errorMsg)
{
    if (graph == nullptr) {
        errorMsg = "Graph is null";
        return false;
    }
    
    // Validate graph first
    auto validationMessages = BTGraphValidator::ValidateGraph(graph);
    for (auto msgIt = validationMessages.begin(); msgIt != validationMessages.end(); ++msgIt) {
        const auto& msg = *msgIt;
        if (msg.severity == BTValidationSeverity::Error) {
            errorMsg = msg.message;
            return false;
        }
    }
    
    // Clear asset
    outAsset.nodes.clear();
    outAsset.rootNodeId = graph->rootNodeId.value;
    
    // Compile each node
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& graphNode = *nodeIt;
        
        BTNode btNode;
        if (!CompileNode(graphNode, btNode)) {
            errorMsg = "Failed to compile node " + std::to_string(graphNode.id.value);
            return false;
        }
        
        outAsset.nodes.push_back(btNode);
    }
    
    SYSTEM_LOG << "[BTGraphCompiler] Compiled " << outAsset.nodes.size() << " nodes" << std::endl;
    return true;
}

bool BTGraphCompiler::CompileNode(
    const NodeGraph::NodeData& graphNode,
    BTNode& outNode)
{
    outNode.id = graphNode.id.value;
    outNode.name = graphNode.name;
    
    // Map type
    outNode.type = MapNodeType(graphNode.type);
    
    // Copy children
    outNode.childIds.clear();
    for (auto childIt = graphNode.children.begin(); childIt != graphNode.children.end(); ++childIt) {
        outNode.childIds.push_back(childIt->value);
    }
    
    // Copy decorator child
    outNode.decoratorChildId = graphNode.decoratorChild.value;
    
    // Copy parameters to flexible parameter maps
    for (auto paramIt = graphNode.parameters.begin(); paramIt != graphNode.parameters.end(); ++paramIt) {
        const std::string& key = paramIt->first;
        const std::string& value = paramIt->second;
        
        // Store as string by default
        outNode.stringParams[key] = value;
        
        // Try to parse as int or float
        try {
            size_t pos = 0;
            int intVal = std::stoi(value, &pos);
            if (pos == value.length()) {
                outNode.intParams[key] = intVal;
            }
        } catch (...) {
            // Not an int
        }
        
        try {
            size_t pos = 0;
            float floatVal = std::stof(value, &pos);
            if (pos == value.length()) {
                outNode.floatParams[key] = floatVal;
            }
        } catch (...) {
            // Not a float
        }
    }
    
    // Set legacy fields for backward compatibility
    if (outNode.type == BTNodeType::Action) {
        // Try to map action type
        if (graphNode.type == "BT_Wait") {
            outNode.actionType = BTActionType::Idle;
            outNode.actionParam1 = outNode.GetParameterFloat("duration", 1.0f);
        } else if (graphNode.type == "BT_MoveToTarget") {
            outNode.actionType = BTActionType::SetMoveGoalToTarget;
            outNode.actionParam1 = outNode.GetParameterFloat("speed", 100.0f);
        } else if (graphNode.type == "BT_AttackTarget") {
            outNode.actionType = BTActionType::AttackIfClose;
        }
    } else if (outNode.type == BTNodeType::Condition) {
        // Try to map condition type
        if (graphNode.type == "BT_HasTarget") {
            outNode.conditionType = BTConditionType::TargetVisible;
        } else if (graphNode.type == "BT_IsTargetInRange") {
            outNode.conditionType = BTConditionType::TargetInRange;
            outNode.conditionParam = outNode.GetParameterFloat("distance", 100.0f);
        }
    } else if (outNode.type == BTNodeType::Repeater) {
        outNode.repeatCount = outNode.GetParameterInt("repeatCount", 1);
    }
    
    return true;
}

BTNodeType BTGraphCompiler::MapNodeType(const std::string& typeString) {
    // Composites
    if (typeString == "BT_Selector") {
        return BTNodeType::Selector;
    }
    if (typeString == "BT_Sequence") {
        return BTNodeType::Sequence;
    }
    if (typeString == "BT_Parallel") {
        return BTNodeType::Selector; // Map to Selector for now (no Parallel in BTNodeType)
    }
    
    // Decorators
    if (typeString == "BT_Inverter") {
        return BTNodeType::Inverter;
    }
    if (typeString == "BT_Repeater" || 
        typeString == "BT_UntilSuccess" || 
        typeString == "BT_UntilFailure" || 
        typeString == "BT_Cooldown") {
        return BTNodeType::Repeater;
    }
    
    // Conditions
    if (typeString == "BT_CheckBlackboardValue" ||
        typeString == "BT_HasTarget" ||
        typeString == "BT_IsTargetInRange" ||
        typeString == "BT_CanSeeTarget") {
        return BTNodeType::Condition;
    }
    
    // Actions (everything else defaults to Action)
    return BTNodeType::Action;
}

} // namespace AI
} // namespace Olympe
