/**
 * @file BTGraphValidator.cpp
 * @brief Implementation of BTGraphValidator
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTGraphValidator.h"
#include "BTNodeRegistry.h"
#include "../../system/system_utils.h"
#include <set>
#include <functional>

namespace Olympe {
namespace AI {

std::vector<BTValidationMessage> BTGraphValidator::ValidateGraph(const NodeGraph::GraphDocument* graph) {
    std::vector<BTValidationMessage> messages;
    
    if (graph == nullptr) {
        messages.push_back({
            BTValidationSeverity::Error,
            0,
            "Graph is null",
            "Create a valid graph document"
        });
        return messages;
    }
    
    // Execute all validation rules
    ValidateRootNode(graph, messages);
    ValidateCycles(graph, messages);
    ValidateChildrenCount(graph, messages);
    ValidateOrphans(graph, messages);
    ValidateNodeTypes(graph, messages);
    
    return messages;
}

void BTGraphValidator::ValidateRootNode(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    // Count nodes without parent
    int rootCount = 0;
    uint32_t lastRootId = 0;
    
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        bool hasParent = false;
        
        // Check if this node is a child of any other node
        for (auto otherIt = graph->GetNodes().begin(); otherIt != graph->GetNodes().end(); ++otherIt) {
            const auto& otherNode = *otherIt;
            
            // Check in children array
            for (auto childIt = otherNode.children.begin(); childIt != otherNode.children.end(); ++childIt) {
                if (childIt->value == node.id.value) {
                    hasParent = true;
                    break;
                }
            }
            
            // Check decorator child
            if (otherNode.decoratorChild.value == node.id.value) {
                hasParent = true;
            }
            
            if (hasParent) break;
        }
        
        if (!hasParent) {
            rootCount++;
            lastRootId = node.id.value;
        }
    }
    
    if (rootCount == 0) {
        messages.push_back({
            BTValidationSeverity::Error,
            0,
            "No root node found",
            "Add a root node (Selector or Sequence)"
        });
    } else if (rootCount > 1) {
        messages.push_back({
            BTValidationSeverity::Error,
            0,
            "Multiple root nodes detected",
            "Connect all nodes to a single root"
        });
    }
}

void BTGraphValidator::ValidateCycles(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    // DFS-based cycle detection
    std::set<uint32_t> visited;
    std::set<uint32_t> recursionStack;
    
    // Recursive DFS function
    std::function<bool(uint32_t)> hasCycleDFS = [&](uint32_t nodeId) -> bool {
        visited.insert(nodeId);
        recursionStack.insert(nodeId);
        
        const auto* node = graph->GetNode(NodeGraph::NodeId{nodeId});
        if (node != nullptr) {
            // Check all children
            for (auto childIt = node->children.begin(); childIt != node->children.end(); ++childIt) {
                uint32_t childId = childIt->value;
                
                if (visited.find(childId) == visited.end()) {
                    // Not visited yet, recurse
                    if (hasCycleDFS(childId)) {
                        return true;
                    }
                } else if (recursionStack.find(childId) != recursionStack.end()) {
                    // Found a back edge (cycle)
                    messages.push_back({
                        BTValidationSeverity::Error,
                        nodeId,
                        "Cycle detected in graph",
                        "Remove circular connections"
                    });
                    return true;
                }
            }
            
            // Check decorator child
            if (node->decoratorChild.value != 0) {
                uint32_t childId = node->decoratorChild.value;
                
                if (visited.find(childId) == visited.end()) {
                    if (hasCycleDFS(childId)) {
                        return true;
                    }
                } else if (recursionStack.find(childId) != recursionStack.end()) {
                    messages.push_back({
                        BTValidationSeverity::Error,
                        nodeId,
                        "Cycle detected in decorator chain",
                        "Remove circular connections"
                    });
                    return true;
                }
            }
        }
        
        recursionStack.erase(nodeId);
        return false;
    };
    
    // Launch DFS from each unvisited node
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        if (visited.find(node.id.value) == visited.end()) {
            hasCycleDFS(node.id.value);
        }
    }
}

void BTGraphValidator::ValidateChildrenCount(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    auto& registry = BTNodeRegistry::Get();
    
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(node.type);
        
        if (typeInfo == nullptr) {
            continue; // Type validation handled in ValidateNodeTypes
        }
        
        int childCount = static_cast<int>(node.children.size());
        
        // Add decorator child to count if present
        if (node.decoratorChild.value != 0) {
            childCount++;
        }
        
        // Check minimum
        if (typeInfo->minChildren >= 0 && childCount < typeInfo->minChildren) {
            messages.push_back({
                BTValidationSeverity::Error,
                node.id.value,
                "Too few children (" + std::to_string(childCount) + " < " + std::to_string(typeInfo->minChildren) + ")",
                "Add at least " + std::to_string(typeInfo->minChildren - childCount) + " children"
            });
        }
        
        // Check maximum
        if (typeInfo->maxChildren >= 0 && childCount > typeInfo->maxChildren) {
            messages.push_back({
                BTValidationSeverity::Error,
                node.id.value,
                "Too many children (" + std::to_string(childCount) + " > " + std::to_string(typeInfo->maxChildren) + ")",
                "Remove " + std::to_string(childCount - typeInfo->maxChildren) + " children"
            });
        }
    }
}

void BTGraphValidator::ValidateOrphans(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    if (graph->GetNodes().empty()) {
        return;
    }
    
    // Find the root node(s)
    std::set<uint32_t> roots;
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        bool hasParent = false;
        
        for (auto otherIt = graph->GetNodes().begin(); otherIt != graph->GetNodes().end(); ++otherIt) {
            const auto& otherNode = *otherIt;
            
            for (auto childIt = otherNode.children.begin(); childIt != otherNode.children.end(); ++childIt) {
                if (childIt->value == node.id.value) {
                    hasParent = true;
                    break;
                }
            }
            
            if (otherNode.decoratorChild.value == node.id.value) {
                hasParent = true;
            }
            
            if (hasParent) break;
        }
        
        if (!hasParent) {
            roots.insert(node.id.value);
        }
    }
    
    if (roots.empty() || roots.size() > 1) {
        return; // Root validation handles this
    }
    
    // BFS from root to find all reachable nodes
    std::set<uint32_t> reachable;
    std::vector<uint32_t> queue;
    
    uint32_t rootId = *roots.begin();
    queue.push_back(rootId);
    reachable.insert(rootId);
    
    while (!queue.empty()) {
        uint32_t currentId = queue.front();
        queue.erase(queue.begin());
        
        const auto* node = graph->GetNode(NodeGraph::NodeId{currentId});
        if (node != nullptr) {
            // Add all children to queue
            for (auto childIt = node->children.begin(); childIt != node->children.end(); ++childIt) {
                uint32_t childId = childIt->value;
                if (reachable.find(childId) == reachable.end()) {
                    reachable.insert(childId);
                    queue.push_back(childId);
                }
            }
            
            // Add decorator child
            if (node->decoratorChild.value != 0) {
                uint32_t childId = node->decoratorChild.value;
                if (reachable.find(childId) == reachable.end()) {
                    reachable.insert(childId);
                    queue.push_back(childId);
                }
            }
        }
    }
    
    // Check for orphans
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        if (reachable.find(node.id.value) == reachable.end()) {
            messages.push_back({
                BTValidationSeverity::Warning,
                node.id.value,
                "Orphan node detected (not connected to root)",
                "Connect this node to the tree or delete it"
            });
        }
    }
}

void BTGraphValidator::ValidateNodeTypes(const NodeGraph::GraphDocument* graph, std::vector<BTValidationMessage>& messages) {
    auto& registry = BTNodeRegistry::Get();
    
    for (auto nodeIt = graph->GetNodes().begin(); nodeIt != graph->GetNodes().end(); ++nodeIt) {
        const auto& node = *nodeIt;
        
        if (!registry.IsValidNodeType(node.type)) {
            messages.push_back({
                BTValidationSeverity::Error,
                node.id.value,
                "Unknown node type: " + node.type,
                "Change to valid BT node type"
            });
        }
    }
}

} // namespace AI
} // namespace Olympe
