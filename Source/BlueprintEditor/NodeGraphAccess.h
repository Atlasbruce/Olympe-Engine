/**
 * @file NodeGraphAccess.h
 * @brief Safe wrapper for NodeGraphManager access without NodeGraph namespace/class conflicts
 * @author Olympe Engine
 * @date 2026-03-09
 * 
 * @details
 * This header provides forward declarations and safe access patterns to NodeGraphManager
 * without including NodeGraphCore.h, which has a namespace/class naming conflict
 * (namespace NodeGraph + class NodeGraph in BTNodeGraphManager).
 * 
 * Phase 50.3: Created to unblock graph rendering without deep architecture refactoring.
 */

#pragma once

#include <vector>
#include <string>

// Forward declarations - these types are defined elsewhere
// but we don't need to include their full headers
namespace Olympe {
    // Forward declare the inner NodeGraph class from BTNodeGraphManager
    // This is NOT the namespace NodeGraph::NodeGraph, but rather
    // the BTNodeGraphManager::NodeGraph inner class
    class BTNodeGraphManager;
    struct GraphNode;
    struct GraphLink;
    
    /**
     * @class GraphAccessor
     * @brief Safe wrapper for accessing graph data through BTNodeGraphManager
     * 
     * Avoids the namespace/class conflict by using opaque handles and
     * BTNodeGraphManager as the data provider
     */
    class GraphAccessor
    {
    public:
        /**
         * Get the active graph's nodes
         * @return Vector of GraphNode pointers from active graph
         */
        static std::vector<GraphNode*> GetActiveGraphNodes();
        
        /**
         * Get the active graph's links/connections
         * @return Vector of GraphLink from active graph
         */
        static std::vector<GraphLink> GetActiveGraphLinks();
        
        /**
         * Get the active graph ID
         * @return Graph ID, or -1 if no active graph
         */
        static int GetActiveGraphId();
        
        /**
         * Check if a graph is currently active
         * @return True if an active graph exists
         */
        static bool HasActiveGraph();
    };
}
