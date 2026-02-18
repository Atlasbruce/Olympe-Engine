/**
 * @file GraphDocument.h
 * @brief Document class for managing node graphs
 * @author Olympe Engine
 * @date 2026-02-18
 *
 * @details
 * Manages a single node graph with CRUD operations, validation, and JSON serialization.
 * Supports generic node graphs for any editor type.
 */

#pragma once

#include "NodeGraphCore.h"
#include "../json_helper.h"
#include <memory>

namespace Olympe {
namespace NodeGraph {

// Forward declaration
class GraphDocument;

/**
 * @class GraphDocument
 * @brief Main document class for a node graph
 */
class GraphDocument {
public:
    GraphDocument();
    ~GraphDocument();
    
    // ========================================================================
    // Document Properties
    // ========================================================================
    
    std::string type;
    std::string graphKind;
    NodeId rootNodeId;
    json metadata;
    EditorState editorState;
    
    // ========================================================================
    // CRUD Operations - Nodes
    // ========================================================================
    
    /**
     * @brief Create a new node in the graph
     * @param nodeType Type of the node (e.g., "BT_Selector", "BT_Action")
     * @param pos Position of the node
     * @return ID of the created node
     */
    NodeId CreateNode(const std::string& nodeType, Vector2 pos);
    
    /**
     * @brief Delete a node from the graph
     * @param id ID of the node to delete
     * @return true if deleted, false if not found
     */
    bool DeleteNode(NodeId id);
    
    /**
     * @brief Update node position
     * @param id Node ID
     * @param newPos New position
     * @return true if updated, false if not found
     */
    bool UpdateNodePosition(NodeId id, Vector2 newPos);
    
    /**
     * @brief Update node parameters
     * @param id Node ID
     * @param params New parameters
     * @return true if updated, false if not found
     */
    bool UpdateNodeParameters(NodeId id, const std::map<std::string, std::string>& params);
    
    /**
     * @brief Get a node by ID
     * @param id Node ID
     * @return Pointer to node data or nullptr if not found
     */
    NodeData* GetNode(NodeId id);
    const NodeData* GetNode(NodeId id) const;
    
    // ========================================================================
    // CRUD Operations - Links
    // ========================================================================
    
    /**
     * @brief Connect two pins with a link
     * @param fromPin Source pin
     * @param toPin Target pin
     * @return ID of created link
     */
    LinkId ConnectPins(PinId fromPin, PinId toPin);
    
    /**
     * @brief Disconnect a link
     * @param id Link ID
     * @return true if deleted, false if not found
     */
    bool DisconnectLink(LinkId id);
    
    /**
     * @brief Get a link by ID
     * @param id Link ID
     * @return Pointer to link data or nullptr if not found
     */
    LinkData* GetLink(LinkId id);
    const LinkData* GetLink(LinkId id) const;
    
    // ========================================================================
    // Validation
    // ========================================================================
    
    /**
     * @brief Validate the graph structure
     * @param errorMessage Output error message if validation fails
     * @return true if valid, false otherwise
     */
    bool ValidateGraph(std::string& errorMessage) const;
    
    /**
     * @brief Check if the graph has cycles
     * @return true if cycles detected, false otherwise
     */
    bool HasCycles() const;
    
    // ========================================================================
    // Serialization
    // ========================================================================
    
    /**
     * @brief Convert graph to JSON format (v2 schema)
     * @return JSON representation
     */
    json ToJson() const;
    
    /**
     * @brief Create graph from JSON
     * @param j JSON object
     * @return GraphDocument instance
     */
    static GraphDocument FromJson(const json& j);
    
    // ========================================================================
    // Data Access
    // ========================================================================
    
    const std::vector<NodeData>& GetNodes() const { return m_nodes; }
    const std::vector<LinkData>& GetLinks() const { return m_links; }
    
    std::vector<NodeData>& GetNodesRef() { return m_nodes; }
    std::vector<LinkData>& GetLinksRef() { return m_links; }
    
    bool IsDirty() const { return m_isDirty; }
    void SetDirty(bool dirty) { m_isDirty = dirty; }
    
private:
    // Data members
    std::vector<NodeData> m_nodes;
    std::vector<LinkData> m_links;
    
    uint32_t m_nextNodeId = 1;
    uint32_t m_nextLinkId = 1;
    bool m_isDirty = false;
    
    // Helper methods
    bool HasCyclesHelper(NodeId nodeId, std::vector<NodeId>& visited, std::vector<NodeId>& recursionStack) const;
};

} // namespace NodeGraph
} // namespace Olympe
