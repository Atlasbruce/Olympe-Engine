/*
 * Olympe Blueprint Editor - BT Connection Validator
 * 
 * Validates behavior tree node connection rules in real-time
 * Enforces proper graph structure (no cycles, parent limits, etc.)
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include "NodeGraphManager.h"

namespace Olympe
{
    /**
     * ConnectionValidationResult - Result of connection validation
     */
    struct ConnectionValidationResult
    {
        bool isValid = false;
        std::string errorMessage;
        
        ConnectionValidationResult() = default;
        ConnectionValidationResult(bool valid, const std::string& msg = "")
            : isValid(valid), errorMessage(msg) {}
        
        static ConnectionValidationResult Valid()
        {
            return ConnectionValidationResult(true, "");
        }
        
        static ConnectionValidationResult Invalid(const std::string& reason)
        {
            return ConnectionValidationResult(false, reason);
        }
    };
    
    /**
     * BTConnectionValidator - Validates BT node connections
     * 
     * Rules:
     * - Composite (Selector/Sequence): 1 parent max, N children, at least 1 child recommended
     * - Decorator (Repeater/Inverter): 1 parent max, 1 child mandatory
     * - Leaf (Action/Condition): 1 parent max, 0 children
     * - Root: 0 parent, at least 1 child
     * - A node can have only one parent
     * - No cycles (A→B→A)
     * - One root per tree
     */
    class BTConnectionValidator
    {
    public:
        BTConnectionValidator();
        ~BTConnectionValidator();
        
        /**
         * Check if a connection can be created between parent and child
         * @param graph The node graph
         * @param parentId Parent node ID
         * @param childId Child node ID
         * @return Validation result with error message if invalid
         */
        ConnectionValidationResult CanCreateConnection(
            const NodeGraph* graph, 
            int parentId, 
            int childId) const;
        
        /**
         * Check if a node can accept more children
         * @param graph The node graph
         * @param nodeId Node ID to check
         * @return Validation result
         */
        ConnectionValidationResult CanAcceptChild(
            const NodeGraph* graph, 
            int nodeId) const;
        
        /**
         * Check if a node can accept a parent
         * @param graph The node graph
         * @param nodeId Node ID to check
         * @return Validation result
         */
        ConnectionValidationResult CanAcceptParent(
            const NodeGraph* graph, 
            int nodeId) const;
        
        /**
         * Check if connecting parent->child would create a cycle
         * @param graph The node graph
         * @param parentId Parent node ID
         * @param childId Child node ID
         * @return true if connection would create a cycle
         */
        bool WouldCreateCycle(
            const NodeGraph* graph, 
            int parentId, 
            int childId) const;
        
        /**
         * Get the parent node ID of a given node
         * @param graph The node graph
         * @param nodeId Node ID to check
         * @return Parent node ID, or -1 if no parent
         */
        int GetParentNode(const NodeGraph* graph, int nodeId) const;
        
        /**
         * Get all parent nodes in the graph (for detecting multiple roots)
         * @param graph The node graph
         * @return Set of node IDs that have no parent
         */
        std::set<int> GetRootNodes(const NodeGraph* graph) const;
        
        /**
         * Get all orphan nodes (nodes with no parent and not root)
         * @param graph The node graph
         * @return Set of orphan node IDs
         */
        std::set<int> GetOrphanNodes(const NodeGraph* graph) const;
        
        /**
         * Get the maximum number of children allowed for a node type
         * @param nodeType Node type
         * @return Max children count (-1 for unlimited)
         */
        int GetMaxChildrenForType(NodeType nodeType) const;
        
        /**
         * Get the minimum number of children recommended for a node type
         * @param nodeType Node type
         * @return Min children count
         */
        int GetMinChildrenForType(NodeType nodeType) const;
        
        /**
         * Check if a node type can have children
         * @param nodeType Node type
         * @return true if node can have children
         */
        bool CanHaveChildren(NodeType nodeType) const;
        
        /**
         * Check if a node type can have a parent
         * @param nodeType Node type
         * @param isRoot Whether the node is marked as root
         * @return true if node can have a parent
         */
        bool CanHaveParent(NodeType nodeType, bool isRoot) const;
        
    private:
        /**
         * Recursive helper for cycle detection using DFS
         * @param graph The node graph
         * @param currentId Current node in traversal
         * @param targetId Target node to find
         * @param visited Set of visited nodes
         * @return true if cycle detected
         */
        bool HasPathTo(
            const NodeGraph* graph,
            int currentId,
            int targetId,
            std::set<int>& visited) const;
    };
}
