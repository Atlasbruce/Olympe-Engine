/**
 * @file BehaviorTreeGraphAdapter.cpp
 * @brief Implementation of BehaviorTree to TaskGraphTemplate conversion adapter
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * Implements the BehaviorTreeGraphAdapter class methods for converting hierarchical
 * BehaviorTree structures into flat TaskGraphTemplate format for generic simulation.
 */

#include "BehaviorTreeGraphAdapter.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Olympe {

// ============================================================================
// PUBLIC: Main Conversion Methods
// ============================================================================

std::unique_ptr<TaskGraphTemplate> BehaviorTreeGraphAdapter::AdaptToTaskGraph(
    const BehaviorTreeAsset& btAsset)
{
    // Step 1: Validate tree structure before conversion
    if (!ValidateTreeStructure(btAsset)) {
        std::cerr << "[BehaviorTreeGraphAdapter] Failed to validate BehaviorTree structure. Conversion aborted.\n";
        return nullptr;
    }

    // Step 2: Create output graph template
    auto taskGraph = std::make_unique<TaskGraphTemplate>();
    taskGraph->Name = btAsset.name;
    taskGraph->Description = "Adapted from BehaviorTree: " + btAsset.name;
    taskGraph->GraphType = "BehaviorTree";

    // For BehaviorTree, the root node is the entry point (not EntryPointID)
    // EntryPointID is only for VisualScript graphs
    taskGraph->RootNodeID = static_cast<int32_t>(btAsset.rootNodeId);
    taskGraph->EntryPointID = -1;  // BehaviorTree doesn't use EntryPointID

    // Step 3: Convert all BT nodes recursively, starting from root
    std::map<uint32_t, int32_t> btToGraphIdMap;  // BT nodeId -> Graph nodeId mapping

    if (btAsset.rootNodeId > 0) {
        const BTNode* rootNode = btAsset.GetNode(btAsset.rootNodeId);
        if (rootNode) {
            AddNodeToGraph(*rootNode, btAsset, *taskGraph, btToGraphIdMap);
        }
    }

    std::cout << "[BehaviorTreeGraphAdapter] Successfully adapted BehaviorTree '" << btAsset.name 
              << "' to TaskGraphTemplate (" << taskGraph->Nodes.size() << " nodes)\n";

    return taskGraph;
}

std::string BehaviorTreeGraphAdapter::FormatTraceForBehaviorTree(
    const GraphExecutionTracer& tracer,
    const BehaviorTreeAsset& btAsset)
{
    std::ostringstream output;
    output << "BehaviorTree Simulation Trace: " << btAsset.name << "\n";
    output << "==================================================\n\n";

    const auto& events = tracer.GetEvents();
    
    if (events.empty()) {
        output << "[No execution events recorded]\n";
        return output.str();
    }

    // Build map of depth for each node (for indentation)
    std::map<int32_t, int32_t> nodeDepthMap;
    for (const auto& event : events) {
        if (nodeDepthMap.find(event.nodeId) == nodeDepthMap.end()) {
            int32_t depth = CalculateNodeDepth(static_cast<uint32_t>(event.nodeId), btAsset);
            nodeDepthMap[event.nodeId] = depth;
        }
    }

    // Process each event
    for (const auto& event : events) {
        // Find corresponding BT node for context
        const BTNode* btNode = btAsset.GetNode(static_cast<uint32_t>(event.nodeId));
        if (!btNode) {
            continue;  // Skip events for nodes we can't find
        }

        // Determine status symbol from event message/type
        std::string statusSymbol = "  ";  // Default: neutral
        if (event.message.find("SUCCESS") != std::string::npos) {
            statusSymbol = "✓ ";
        } else if (event.message.find("FAILURE") != std::string::npos) {
            statusSymbol = "✗ ";
        } else if (event.message.find("RUNNING") != std::string::npos || 
                   event.message.find("Running") != std::string::npos) {
            statusSymbol = "⊙ ";
        }

        // Get indentation from node depth
        int32_t depth = nodeDepthMap[event.nodeId];
        std::string indent(depth * 2, ' ');

        // Get BT-specific type name
        std::string typeName = GetBTNodeTypeName(static_cast<uint8_t>(btNode->type));

        // Build trace line
        output << indent << statusSymbol << typeName << ": " << btNode->name;
        
        // Add event message if present
        if (!event.message.empty()) {
            output << " → " << event.message;
        }
        
        output << "\n";
    }

    output << "\n==================================================\n";
    output << "Total Events: " << events.size() << "\n";

    return output.str();
}

bool BehaviorTreeGraphAdapter::ValidateTreeStructure(
    const BehaviorTreeAsset& btAsset)
{
    // Check 1: Empty tree
    if (btAsset.nodes.empty()) {
        std::cout << "[BehaviorTreeGraphAdapter] Warning: BehaviorTree is empty (no nodes)\n";
        return btAsset.rootNodeId == 0;  // Valid if root is also 0
    }

    // Check 2: Node ID uniqueness
    std::map<uint32_t, bool> seenIds;
    for (const auto& node : btAsset.nodes) {
        if (seenIds.find(node.id) != seenIds.end()) {
            std::cerr << "[BehaviorTreeGraphAdapter] Error: Duplicate node ID: " << node.id << "\n";
            return false;
        }
        seenIds[node.id] = true;
    }

    // Check 3: Root node exists
    if (btAsset.rootNodeId != 0) {
        if (btAsset.GetNode(btAsset.rootNodeId) == nullptr) {
            std::cerr << "[BehaviorTreeGraphAdapter] Error: Root node ID " << btAsset.rootNodeId << " does not exist in tree\n";
            return false;
        }
    }

    // Check 4: Child references validity (all referenced children must exist)
    for (const auto& node : btAsset.nodes) {
        for (uint32_t childId : node.childIds) {
            if (btAsset.GetNode(childId) == nullptr) {
                std::cerr << "[BehaviorTreeGraphAdapter] Error: Node " << node.id << " references non-existent child " << childId << "\n";
                return false;
            }
        }
    }

    // Check 5: Cycle detection
    if (btAsset.rootNodeId != 0) {
        if (btAsset.DetectCycle(btAsset.rootNodeId)) {
            std::cerr << "[BehaviorTreeGraphAdapter] Error: Cycle detected in BehaviorTree\n";
            return false;
        }
    }

    return true;
}

// ============================================================================
// PRIVATE: Helper Methods
// ============================================================================

void BehaviorTreeGraphAdapter::AddNodeToGraph(
    const BTNode& btNode,
    const BehaviorTreeAsset& btAsset,
    TaskGraphTemplate& outGraph,
    std::map<uint32_t, int32_t>& btToGraphIdMap)
{
    // Check if we've already processed this node (cycle safety)
    if (btToGraphIdMap.find(btNode.id) != btToGraphIdMap.end()) {
        return;  // Already processed
    }

    // Step 1: Create graph node from BT node
    TaskNodeDefinition graphNode;
    graphNode.NodeID = static_cast<int32_t>(outGraph.Nodes.size());
    graphNode.NodeName = btNode.name;
    
    // Map BTNodeType to TaskNodeType
    switch (btNode.type) {
        case BTNodeType::Selector:
            graphNode.Type = TaskNodeType::Selector;
            break;
        case BTNodeType::Sequence:
            graphNode.Type = TaskNodeType::Sequence;
            break;
        case BTNodeType::Condition:
            graphNode.Type = TaskNodeType::AtomicTask;
            graphNode.AtomicTaskID = "BT_Condition_" + std::to_string(static_cast<uint8_t>(btNode.conditionType));
            break;
        case BTNodeType::Action:
            graphNode.Type = TaskNodeType::AtomicTask;
            graphNode.AtomicTaskID = "BT_Action_" + std::to_string(static_cast<uint8_t>(btNode.actionType));
            break;
        case BTNodeType::Inverter:
        case BTNodeType::Repeater:
            graphNode.Type = TaskNodeType::Decorator;
            break;
        case BTNodeType::Root:
        case BTNodeType::OnEvent:
            graphNode.Type = TaskNodeType::Root;
            break;
        default:
            graphNode.Type = TaskNodeType::AtomicTask;
    }

    // Store editor position
    graphNode.EditorPosX = btNode.editorPosX;
    graphNode.EditorPosY = btNode.editorPosY;
    graphNode.HasEditorPos = true;

    // Track the mapping
    btToGraphIdMap[btNode.id] = graphNode.NodeID;
    const int32_t currentGraphNodeId = graphNode.NodeID;

    // Step 2: Add to graph
    outGraph.Nodes.push_back(graphNode);

    // Step 3: Recursively add children and create connections
    for (uint32_t childBtId : btNode.childIds) {
        const BTNode* childNode = btAsset.GetNode(childBtId);
        if (!childNode) continue;

        // Recursively process child (if not already done)
        AddNodeToGraph(*childNode, btAsset, outGraph, btToGraphIdMap);

        // Find the graph node ID for this child
        int32_t childGraphId = btToGraphIdMap[childBtId];

        // Create explicit connection (parent control-flow → child)
         ExecPinConnection connection;
         connection.SourceNodeID = currentGraphNodeId;
         connection.SourcePinName = "Control";  // Output pin name
         connection.TargetNodeID = childGraphId;
         connection.TargetPinName = "In";       // Input pin name

         outGraph.ExecConnections.push_back(connection);
    }
}

int32_t BehaviorTreeGraphAdapter::CalculateNodeDepth(
    uint32_t nodeId,
    const BehaviorTreeAsset& btAsset)
{
    // Base case: root node
    if (nodeId == btAsset.rootNodeId) {
        return 0;
    }

    // Find parent node
    uint32_t parentId = FindParentNodeId(nodeId, btAsset);
    if (parentId == 0) {
        return 0;  // Root or orphaned node
    }

    // Recursive case: depth = parent_depth + 1
    return CalculateNodeDepth(parentId, btAsset) + 1;
}

uint32_t BehaviorTreeGraphAdapter::FindParentNodeId(
    uint32_t childNodeId,
    const BehaviorTreeAsset& btAsset)
{
    // Search all nodes to find one that has childNodeId in its children
    for (const auto& node : btAsset.nodes) {
        for (uint32_t childId : node.childIds) {
            if (childId == childNodeId) {
                return node.id;  // Found parent
            }
        }
    }
    return 0;  // No parent found (root or orphaned)
}

std::string BehaviorTreeGraphAdapter::GetBTNodeTypeName(uint8_t type)
{
    // Cast to BTNodeType for safety
    BTNodeType nodeType = static_cast<BTNodeType>(type);
    
    switch (nodeType) {
        case BTNodeType::Selector:    return "Selector";
        case BTNodeType::Sequence:    return "Sequence";
        case BTNodeType::Condition:   return "Condition";
        case BTNodeType::Action:      return "Action";
        case BTNodeType::Inverter:    return "Inverter";
        case BTNodeType::Repeater:    return "Repeater";
        case BTNodeType::Root:        return "Root";
        case BTNodeType::OnEvent:     return "OnEvent";
        default:                      return "Unknown";
    }
}

}  // namespace Olympe
