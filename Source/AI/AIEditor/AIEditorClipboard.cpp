/**
 * @file AIEditorClipboard.cpp
 * @brief Implementation of AI Editor clipboard system
 * @author Olympe Engine
 * @date 2026-02-19
 */

#include "AIEditorClipboard.h"
#include "../../system/system_utils.h"
#include <cfloat>
#include <algorithm>

namespace Olympe {
namespace AI {

// ============================================================================
// Singleton Access
// ============================================================================

AIEditorClipboard& AIEditorClipboard::Get()
{
    static AIEditorClipboard instance;
    return instance;
}

// ============================================================================
// Copy
// ============================================================================

void AIEditorClipboard::Copy(
    const std::vector<NodeGraph::NodeId>& nodeIds,
    NodeGraph::GraphDocument* doc)
{
    Clear();
    
    if (nodeIds.empty() || doc == nullptr) {
        return;
    }
    
    // Build set of node IDs for fast lookup
    std::map<uint32_t, bool> nodeIdSet;
    for (size_t i = 0; i < nodeIds.size(); ++i) {
        nodeIdSet[nodeIds[i].value] = true;
    }
    
    // Find reference position (top-left corner)
    Vector refPos(FLT_MAX, FLT_MAX);
    for (auto it = nodeIdSet.begin(); it != nodeIdSet.end(); ++it) {
        NodeGraph::NodeId nodeId;
        nodeId.value = it->first;
        
        const NodeGraph::NodeData* nodeData = doc->GetNode(nodeId);
        if (nodeData != nullptr) {
            if (nodeData->position.x < refPos.x) {
                refPos.x = nodeData->position.x;
            }
            if (nodeData->position.y < refPos.y) {
                refPos.y = nodeData->position.y;
            }
        }
    }
    
    // Copy nodes
    for (auto it = nodeIdSet.begin(); it != nodeIdSet.end(); ++it) {
        NodeGraph::NodeId nodeId;
        nodeId.value = it->first;
        
        const NodeGraph::NodeData* nodeData = doc->GetNode(nodeId);
        if (nodeData == nullptr) {
            continue;
        }
        
        ClipboardNode cn;
        cn.type = nodeData->type;
        cn.name = nodeData->name;
        cn.position = Vector(
            nodeData->position.x - refPos.x,
            nodeData->position.y - refPos.y
        );
        cn.parameters = nodeData->parameters;
        cn.originalId = it->first;
        
        m_nodes.push_back(cn);
    }
    
    // Copy links (only between copied nodes)
    const std::vector<NodeGraph::LinkData>& allLinks = doc->GetLinks();
    for (size_t i = 0; i < allLinks.size(); ++i) {
        const NodeGraph::LinkData& link = allLinks[i];
        
        // Get source and target node IDs from pins
        // Pin ID convention: nodeId * PIN_ID_MULTIPLIER + offset
        uint32_t fromNodeId = link.fromPin.value / PIN_ID_MULTIPLIER;
        uint32_t toNodeId = link.toPin.value / PIN_ID_MULTIPLIER;
        
        // Check if both nodes are in the copied set
        auto fromIt = nodeIdSet.find(fromNodeId);
        auto toIt = nodeIdSet.find(toNodeId);
        
        if (fromIt != nodeIdSet.end() && toIt != nodeIdSet.end()) {
            ClipboardLink clipLink;
            clipLink.parentOriginalId = fromNodeId;
            clipLink.childOriginalId = toNodeId;
            m_links.push_back(clipLink);
        }
    }
    
    SYSTEM_LOG << "[Clipboard] Copied " << m_nodes.size() << " nodes, " 
               << m_links.size() << " links" << std::endl;
}

// ============================================================================
// Cut
// ============================================================================

void AIEditorClipboard::Cut(
    const std::vector<NodeGraph::NodeId>& nodeIds,
    NodeGraph::GraphDocument* doc)
{
    if (doc == nullptr) {
        return;
    }
    
    // Copy first
    Copy(nodeIds, doc);
    
    // Delete selected nodes
    for (size_t i = 0; i < nodeIds.size(); ++i) {
        doc->DeleteNode(nodeIds[i]);
    }
    
    SYSTEM_LOG << "[Clipboard] Cut " << nodeIds.size() << " nodes" << std::endl;
}

// ============================================================================
// Paste
// ============================================================================

std::vector<NodeGraph::NodeId> AIEditorClipboard::Paste(
    NodeGraph::GraphDocument* doc,
    Vector pasteOffset)
{
    std::vector<NodeGraph::NodeId> newNodeIds;
    
    if (IsEmpty() || doc == nullptr) {
        return newNodeIds;
    }
    
    // Map: originalId -> new NodeId
    std::map<uint32_t, NodeGraph::NodeId> idMap;
    
    // Create nodes
    for (size_t i = 0; i < m_nodes.size(); ++i)
    {        
		Vector pos ( m_nodes[i].position.x + pasteOffset.x, m_nodes[i].position.y + pasteOffset.y);
        
        
        NodeGraph::NodeId newId = doc->CreateNode(m_nodes[i].type, pos);
        
        // Set name and parameters
        const NodeGraph::NodeData* nodeData = doc->GetNode(newId);
        if (nodeData != nullptr) {
            NodeGraph::NodeData updatedData = *nodeData;
            updatedData.name = m_nodes[i].name;
            updatedData.parameters = m_nodes[i].parameters;
            doc->UpdateNode(newId, updatedData);
        }
        
        idMap[m_nodes[i].originalId] = newId;
        newNodeIds.push_back(newId);
    }
    
    // Recreate links
    for (size_t i = 0; i < m_links.size(); ++i) {
        auto parentIt = idMap.find(m_links[i].parentOriginalId);
        auto childIt = idMap.find(m_links[i].childOriginalId);
        
        if (parentIt != idMap.end() && childIt != idMap.end()) {
            // Create pins from node IDs using the pin ID convention
            // Output pin: nodeId * PIN_ID_MULTIPLIER + PIN_ID_OUTPUT_OFFSET
            // Input pin: nodeId * PIN_ID_MULTIPLIER + PIN_ID_INPUT_OFFSET
            NodeGraph::PinId fromPin;
            fromPin.value = parentIt->second.value * PIN_ID_MULTIPLIER + PIN_ID_OUTPUT_OFFSET;
            
            NodeGraph::PinId toPin;
            toPin.value = childIt->second.value * PIN_ID_MULTIPLIER + PIN_ID_INPUT_OFFSET;
            
            doc->ConnectPins(fromPin, toPin);
        }
    }
    
    SYSTEM_LOG << "[Clipboard] Pasted " << newNodeIds.size() << " nodes" << std::endl;
    
    return newNodeIds;
}

// ============================================================================
// Utility Methods
// ============================================================================

bool AIEditorClipboard::IsEmpty() const
{
    return m_nodes.empty();
}

void AIEditorClipboard::Clear()
{
    m_nodes.clear();
    m_links.clear();
}

} // namespace AI
} // namespace Olympe
