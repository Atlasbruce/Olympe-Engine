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

// Type aliases for backward compatibility
using GraphDocument = Olympe::NodeGraphTypes::GraphDocument;
using NodeData = Olympe::NodeGraphTypes::NodeData;

bool BTGraphCompiler::Compile(
    const GraphDocument* graph,
    BehaviorTreeAsset& outAsset,
    std::string& errorMsg)
{
    // TODO: Reimplement with modern NodeGraphTypes schema
    // - Old validator expects int, new GraphDocument* signature incompatible
    // - Node compilation logic uses removed decoratorChild/children fields
    // - Parameter mapping logic deprecated with new schema
    // For now: Return false with error message
    (void)graph;
    (void)errorMsg;

    outAsset.nodes.clear();
    outAsset.rootNodeId = 0;

    errorMsg = "[BTGraphCompiler::Compile] DEPRECATED - awaiting reimplementation (Phase 50.4)";
    SYSTEM_LOG << errorMsg << std::endl;
    return false;
}

bool BTGraphCompiler::CompileNode(
    const NodeData& graphNode,
    BTNode& outNode)
{
    // TODO: Reimplement with modern NodeGraphTypes schema
    // - graphNode.children field removed from modern schema
    // - graphNode.decoratorChild field removed from modern schema
    // - graphNode.parameters field structure changed
    // For now: Minimal stub to satisfy interface
    (void)graphNode;

    outNode.id = 0;
    outNode.name = "DEPRECATED";
    outNode.type = BTNodeType::Action;
    outNode.actionType = BTActionType::Idle;
    outNode.childIds.clear();
    outNode.decoratorChildId = 0;
    outNode.stringParams.clear();
    outNode.intParams.clear();
    outNode.floatParams.clear();

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
