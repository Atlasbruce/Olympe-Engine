/*
 * Olympe Blueprint Editor - BT Connection Validator Implementation
 */

#include "BTConnectionValidator.h"
#include <iostream>
#include <algorithm>

namespace Olympe
{
    BTConnectionValidator::BTConnectionValidator()
    {
    }
    
    BTConnectionValidator::~BTConnectionValidator()
    {
    }
    
    ConnectionValidationResult BTConnectionValidator::CanCreateConnection(
        const NodeGraph* graph, 
        int parentId, 
        int childId) const
    {
        if (!graph)
            return ConnectionValidationResult::Invalid("Graph is null");
        
        const GraphNode* parentNode = graph->GetNode(parentId);
        const GraphNode* childNode = graph->GetNode(childId);
        
        if (!parentNode)
            return ConnectionValidationResult::Invalid("Parent node not found");
        if (!childNode)
            return ConnectionValidationResult::Invalid("Child node not found");
        
        // Check 1: Cannot connect node to itself
        if (parentId == childId)
            return ConnectionValidationResult::Invalid("Cannot connect node to itself");
        
        // Check 2: Parent must be able to have children
        if (!CanHaveChildren(parentNode->type))
            return ConnectionValidationResult::Invalid(
                "Node type '" + std::string(NodeTypeToString(parentNode->type)) + 
                "' cannot have children (leaf node)");
        
        // Check 3: Child must be able to have a parent
        bool isChildRoot = (graph->rootNodeId == childId);
        if (!CanHaveParent(childNode->type, isChildRoot))
            return ConnectionValidationResult::Invalid(
                "Node is marked as root and cannot have a parent");
        
        // Check 4: Check if parent already has maximum children
        auto parentCapacity = CanAcceptChild(graph, parentId);
        if (!parentCapacity.isValid)
            return parentCapacity;
        
        // Check 5: Check if child already has a parent
        auto childCapacity = CanAcceptParent(graph, childId);
        if (!childCapacity.isValid)
            return childCapacity;
        
        // Check 6: Check for cycles
        if (WouldCreateCycle(graph, parentId, childId))
            return ConnectionValidationResult::Invalid(
                "Connection would create a cycle in the tree");
        
        // Check 7: Already connected
        auto links = graph->GetAllLinks();
        for (const auto& link : links)
        {
            if (link.fromNode == parentId && link.toNode == childId)
                return ConnectionValidationResult::Invalid(
                    "Nodes are already connected");
        }
        
        return ConnectionValidationResult::Valid();
    }
    
    ConnectionValidationResult BTConnectionValidator::CanAcceptChild(
        const NodeGraph* graph, 
        int nodeId) const
    {
        if (!graph)
            return ConnectionValidationResult::Invalid("Graph is null");
        
        const GraphNode* node = graph->GetNode(nodeId);
        if (!node)
            return ConnectionValidationResult::Invalid("Node not found");
        
        // Check if node type can have children
        if (!CanHaveChildren(node->type))
            return ConnectionValidationResult::Invalid(
                "Node type cannot have children");
        
        int maxChildren = GetMaxChildrenForType(node->type);
        
        // Unlimited children
        if (maxChildren == -1)
            return ConnectionValidationResult::Valid();
        
        // Count current children
        int childCount = 0;
        if (node->type == NodeType::BT_Decorator)
        {
            childCount = (node->decoratorChildId >= 0) ? 1 : 0;
        }
        else
        {
            childCount = static_cast<int>(node->childIds.size());
        }
        
        if (childCount >= maxChildren)
        {
            return ConnectionValidationResult::Invalid(
                "Node already has maximum number of children (" + 
                std::to_string(maxChildren) + ")");
        }
        
        return ConnectionValidationResult::Valid();
    }
    
    ConnectionValidationResult BTConnectionValidator::CanAcceptParent(
        const NodeGraph* graph, 
        int nodeId) const
    {
        if (!graph)
            return ConnectionValidationResult::Invalid("Graph is null");
        
        const GraphNode* node = graph->GetNode(nodeId);
        if (!node)
            return ConnectionValidationResult::Invalid("Node not found");
        
        // Check if node is root
        bool isRoot = (graph->rootNodeId == nodeId);
        if (!CanHaveParent(node->type, isRoot))
            return ConnectionValidationResult::Invalid(
                "Root node cannot have a parent");
        
        // Check if node already has a parent
        int existingParent = GetParentNode(graph, nodeId);
        if (existingParent >= 0)
        {
            return ConnectionValidationResult::Invalid(
                "Node already has a parent (node " + std::to_string(existingParent) + ")");
        }
        
        return ConnectionValidationResult::Valid();
    }
    
    bool BTConnectionValidator::WouldCreateCycle(
        const NodeGraph* graph, 
        int parentId, 
        int childId) const
    {
        if (!graph)
            return false;
        
        // Check if there's a path from child to parent
        // If yes, then connecting parent->child would create a cycle
        std::set<int> visited;
        return HasPathTo(graph, childId, parentId, visited);
    }
    
    bool BTConnectionValidator::HasPathTo(
        const NodeGraph* graph,
        int currentId,
        int targetId,
        std::set<int>& visited) const
    {
        // Already visited this node
        if (visited.find(currentId) != visited.end())
            return false;
        
        visited.insert(currentId);
        
        // Found target
        if (currentId == targetId)
            return true;
        
        const GraphNode* currentNode = graph->GetNode(currentId);
        if (!currentNode)
            return false;
        
        // Check all children
        for (int childId : currentNode->childIds)
        {
            if (HasPathTo(graph, childId, targetId, visited))
                return true;
        }
        
        // Check decorator child
        if (currentNode->decoratorChildId >= 0)
        {
            if (HasPathTo(graph, currentNode->decoratorChildId, targetId, visited))
                return true;
        }
        
        return false;
    }
    
    int BTConnectionValidator::GetParentNode(const NodeGraph* graph, int nodeId) const
    {
        if (!graph)
            return -1;
        
        auto nodes = graph->GetAllNodes();
        for (const GraphNode* node : nodes)
        {
            // Check in childIds
            if (std::find(node->childIds.begin(), node->childIds.end(), nodeId) != node->childIds.end())
                return node->id;
            
            // Check decorator child
            if (node->decoratorChildId == nodeId)
                return node->id;
        }
        
        return -1;
    }
    
    std::set<int> BTConnectionValidator::GetRootNodes(const NodeGraph* graph) const
    {
        std::set<int> roots;
        
        if (!graph)
            return roots;
        
        auto nodes = graph->GetAllNodes();
        for (const GraphNode* node : nodes)
        {
            // A node is a root if it has no parent
            if (GetParentNode(graph, node->id) == -1)
            {
                roots.insert(node->id);
            }
        }
        
        return roots;
    }
    
    std::set<int> BTConnectionValidator::GetOrphanNodes(const NodeGraph* graph) const
    {
        std::set<int> orphans;
        
        if (!graph)
            return orphans;
        
        std::set<int> roots = GetRootNodes(graph);
        
        // Orphans are nodes with no parent that are NOT the root node
        for (int nodeId : roots)
        {
            if (nodeId != graph->rootNodeId)
            {
                orphans.insert(nodeId);
            }
        }
        
        return orphans;
    }
    
    int BTConnectionValidator::GetMaxChildrenForType(NodeType nodeType) const
    {
        switch (nodeType)
        {
            case NodeType::BT_Sequence:
            case NodeType::BT_Selector:
                return -1;  // Unlimited children
            
            case NodeType::BT_Decorator:
                return 1;   // Single child
            
            case NodeType::BT_Action:
            case NodeType::BT_Condition:
                return 0;   // No children (leaf nodes)
            
            default:
                return 0;
        }
    }
    
    int BTConnectionValidator::GetMinChildrenForType(NodeType nodeType) const
    {
        switch (nodeType)
        {
            case NodeType::BT_Sequence:
            case NodeType::BT_Selector:
                return 1;   // At least 1 child recommended
            
            case NodeType::BT_Decorator:
                return 1;   // 1 child mandatory
            
            case NodeType::BT_Action:
            case NodeType::BT_Condition:
                return 0;   // No children
            
            default:
                return 0;
        }
    }
    
    bool BTConnectionValidator::CanHaveChildren(NodeType nodeType) const
    {
        return GetMaxChildrenForType(nodeType) != 0;
    }
    
    bool BTConnectionValidator::CanHaveParent(NodeType nodeType, bool isRoot) const
    {
        // Root nodes cannot have parents
        if (isRoot)
            return false;
        
        // All node types can have parents (except root)
        return true;
    }
}
