/**
 * @file AIEditorClipboard.h
 * @brief Clipboard system for AI Editor (Cut/Copy/Paste)
 * @author Olympe Engine
 * @date 2026-02-19
 * 
 * @details
 * Provides clipboard functionality for copying, cutting, and pasting
 * nodes with their connections preserved. Uses singleton pattern for
 * global clipboard access.
 */

#pragma once

#include "../../NodeGraphCore/NodeGraphCore.h"
#include "../../NodeGraphCore/GraphDocument.h"
#include "../../vector.h"
#include <vector>
#include <map>
#include <string>

namespace Olympe {
namespace AI {

// Pin ID Convention Constants
// These match the convention used in AIEditorNodeRenderer.cpp
constexpr uint32_t PIN_ID_MULTIPLIER = 1000;  ///< Multiplier for pin IDs from node IDs
constexpr uint32_t PIN_ID_INPUT_OFFSET = 0;   ///< Offset for input pin: nodeId * 1000 + 0
constexpr uint32_t PIN_ID_OUTPUT_OFFSET = 1;  ///< Offset for output pin: nodeId * 1000 + 1

/**
 * @struct ClipboardNode
 * @brief Represents a node in the clipboard
 */
struct ClipboardNode {
    std::string type;                           ///< Node type (e.g., "BT_Selector")
    std::string name;                           ///< Node display name
    Vector position;                            ///< Position relative to first node
    std::map<std::string, std::string> parameters; ///< Node parameters
    uint32_t originalId = 0;                    ///< Original node ID (for link reconstruction)
};

/**
 * @struct ClipboardLink
 * @brief Represents a connection between nodes in the clipboard
 */
struct ClipboardLink {
    uint32_t parentOriginalId = 0;  ///< Parent node's original ID
    uint32_t childOriginalId = 0;   ///< Child node's original ID
};

/**
 * @class AIEditorClipboard
 * @brief Singleton clipboard manager for AI Editor
 * 
 * @details
 * Provides Cut/Copy/Paste functionality for nodes in the AI Editor.
 * Preserves node connections and maintains relative positions when pasting.
 */
class AIEditorClipboard {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the clipboard instance
     */
    static AIEditorClipboard& Get();
    
    /**
     * @brief Copy selected nodes to clipboard
     * @param nodeIds List of selected node IDs
     * @param doc Source graph document
     */
    void Copy(const std::vector<NodeGraph::NodeId>& nodeIds, NodeGraph::GraphDocument* doc);
    
    /**
     * @brief Cut selected nodes (copy + delete)
     * @param nodeIds List of selected node IDs
     * @param doc Source graph document
     */
    void Cut(const std::vector<NodeGraph::NodeId>& nodeIds, NodeGraph::GraphDocument* doc);
    
    /**
     * @brief Paste clipboard nodes into active graph
     * @param doc Target graph document
     * @param pasteOffset Offset from original position
     * @return List of new node IDs
     */
    std::vector<NodeGraph::NodeId> Paste(NodeGraph::GraphDocument* doc, Vector pasteOffset);
    
    /**
     * @brief Check if clipboard has data
     * @return true if clipboard contains nodes
     */
    bool IsEmpty() const;
    
    /**
     * @brief Clear clipboard
     */
    void Clear();
    
    /**
     * @brief Get number of nodes in clipboard
     * @return Number of nodes
     */
    size_t GetNodeCount() const { return m_nodes.size(); }
    
private:
    AIEditorClipboard() = default;
    ~AIEditorClipboard() = default;
    
    // Prevent copying
    AIEditorClipboard(const AIEditorClipboard&) = delete;
    AIEditorClipboard& operator=(const AIEditorClipboard&) = delete;
    
    std::vector<ClipboardNode> m_nodes;  ///< Copied nodes
    std::vector<ClipboardLink> m_links;  ///< Copied links
};

} // namespace AI
} // namespace Olympe
