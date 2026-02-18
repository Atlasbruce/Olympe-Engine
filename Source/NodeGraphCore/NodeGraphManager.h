/**
 * @file NodeGraphManager.h
 * @brief Singleton manager for multiple node graphs
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Manages multiple graph documents with tab-like interface.
 * Handles graph lifecycle (create, load, save, close) and active graph selection.
 */

#pragma once

#include "GraphDocument.h"
#include <map>
#include <memory>

namespace Olympe {
namespace NodeGraph {

/**
 * @class NodeGraphManager
 * @brief Singleton manager for multiple node graphs
 */
class NodeGraphManager {
public:
    /**
     * @brief Get singleton instance
     */
    static NodeGraphManager& Get();
    
    // Prevent copying
    NodeGraphManager(const NodeGraphManager&) = delete;
    NodeGraphManager& operator=(const NodeGraphManager&) = delete;
    
    // ========================================================================
    // Graph Lifecycle
    // ========================================================================
    
    /**
     * @brief Create a new graph
     * @param graphType Type of graph (e.g., "AIGraph")
     * @param graphKind Kind of graph (e.g., "BehaviorTree", "HFSM")
     * @return ID of created graph
     */
    GraphId CreateGraph(const std::string& graphType, const std::string& graphKind);
    
    /**
     * @brief Load a graph from file
     * @param filepath Path to JSON file
     * @return ID of loaded graph, or {0} if failed
     */
    GraphId LoadGraph(const std::string& filepath);
    
    /**
     * @brief Save a graph to file
     * @param id Graph ID
     * @param filepath Path to save file
     * @return true if saved successfully
     */
    bool SaveGraph(GraphId id, const std::string& filepath);
    
    /**
     * @brief Close a graph
     * @param id Graph ID
     * @return true if closed successfully
     */
    bool CloseGraph(GraphId id);
    
    // ========================================================================
    // Active Graph Management
    // ========================================================================
    
    /**
     * @brief Set active graph
     * @param id Graph ID
     */
    void SetActiveGraph(GraphId id);
    
    /**
     * @brief Get active graph
     * @return Pointer to active graph or nullptr
     */
    GraphDocument* GetActiveGraph();
    
    /**
     * @brief Get active graph ID
     * @return Active graph ID
     */
    GraphId GetActiveGraphId() const { return m_activeGraphId; }
    
    // ========================================================================
    // Queries
    // ========================================================================
    
    /**
     * @brief Get graph by ID
     * @param id Graph ID
     * @return Pointer to graph or nullptr
     */
    GraphDocument* GetGraph(GraphId id);
    
    /**
     * @brief Get all graph IDs
     * @return Vector of graph IDs
     */
    std::vector<GraphId> GetAllGraphIds() const;
    
    /**
     * @brief Get graph name (for tab display)
     * @param id Graph ID
     * @return Graph name or empty string
     */
    std::string GetGraphName(GraphId id) const;
    
    /**
     * @brief Get graph order for tabs
     * @return Vector of graph IDs in insertion order
     */
    std::vector<GraphId> GetGraphOrderForTabs() const { return m_graphOrder; }
    
private:
    NodeGraphManager();
    ~NodeGraphManager();
    
    std::map<GraphId, std::unique_ptr<GraphDocument>> m_graphs;
    std::map<GraphId, std::string> m_graphNames;
    std::vector<GraphId> m_graphOrder;
    GraphId m_activeGraphId;
    uint32_t m_nextGraphId = 1;
};

} // namespace NodeGraph
} // namespace Olympe
