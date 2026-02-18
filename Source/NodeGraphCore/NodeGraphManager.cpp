/**
 * @file NodeGraphManager.cpp
 * @brief Implementation of NodeGraphManager
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "NodeGraphManager.h"
#include "GraphMigrator.h"
#include "../system/system_utils.h"
#include <fstream>

using json = nlohmann::json;

namespace Olympe {
namespace NodeGraph {

// ============================================================================
// Singleton
// ============================================================================

NodeGraphManager& NodeGraphManager::Get()
{
    static NodeGraphManager instance;
    return instance;
}

NodeGraphManager::NodeGraphManager()
    : m_nextGraphId(1)
{
}

NodeGraphManager::~NodeGraphManager()
{
}

// ============================================================================
// Graph Lifecycle
// ============================================================================

GraphId NodeGraphManager::CreateGraph(const std::string& graphType, const std::string& graphKind)
{
    GraphId newId;
    newId.value = m_nextGraphId++;
    
    auto doc = std::unique_ptr<GraphDocument>(new GraphDocument());
    doc->type = graphType;
    doc->graphKind = graphKind;
    
    std::string name = graphKind + " " + std::to_string(newId.value);
    
    m_graphs[newId] = std::move(doc);
    m_graphNames[newId] = name;
    m_graphOrder.push_back(newId);
    m_activeGraphId = newId;
    
    SYSTEM_LOG << "[NodeGraphManager] Created graph " << newId.value << " (" << graphKind << ")" << std::endl;
    
    return newId;
}

GraphId NodeGraphManager::LoadGraph(const std::string& filepath)
{
    json j;
    if (!JsonHelper::LoadJsonFromFile(filepath, j))
    {
        SYSTEM_LOG << "[NodeGraphManager] Failed to load graph from " << filepath << std::endl;
        return GraphId{0};
    }
    
    // Use migrator to handle version detection and migration
    GraphDocument doc = GraphMigrator::LoadWithMigration(j);
    
    GraphId newId;
    newId.value = m_nextGraphId++;
    
    // Extract name from filepath
    std::string name = filepath;
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        name = filepath.substr(lastSlash + 1);
    }
    
    m_graphs[newId] = std::unique_ptr<GraphDocument>(new GraphDocument(doc));
    m_graphNames[newId] = name;
    m_graphOrder.push_back(newId);
    m_activeGraphId = newId;
    
    SYSTEM_LOG << "[NodeGraphManager] Loaded graph " << newId.value << " from " << filepath << std::endl;
    
    return newId;
}

bool NodeGraphManager::SaveGraph(GraphId id, const std::string& filepath)
{
    auto it = m_graphs.find(id);
    if (it == m_graphs.end())
    {
        SYSTEM_LOG << "[NodeGraphManager] Cannot save: graph " << id.value << " not found" << std::endl;
        return false;
    }
    
    json j = it->second->ToJson();
    
    if (!JsonHelper::SaveJsonToFile(filepath, j, 2))
    {
        SYSTEM_LOG << "[NodeGraphManager] Failed to save graph to " << filepath << std::endl;
        return false;
    }
    
    it->second->SetDirty(false);
    
    SYSTEM_LOG << "[NodeGraphManager] Saved graph " << id.value << " to " << filepath << std::endl;
    
    return true;
}

bool NodeGraphManager::CloseGraph(GraphId id)
{
    auto it = m_graphs.find(id);
    if (it == m_graphs.end())
    {
        return false;
    }
    
    m_graphs.erase(it);
    m_graphNames.erase(id);
    
    // Remove from order
    auto orderIt = std::find(m_graphOrder.begin(), m_graphOrder.end(), id);
    if (orderIt != m_graphOrder.end())
    {
        m_graphOrder.erase(orderIt);
    }
    
    // Update active graph if closed
    if (m_activeGraphId == id)
    {
        if (!m_graphOrder.empty())
        {
            m_activeGraphId = m_graphOrder.back();
        }
        else
        {
            m_activeGraphId = GraphId{0};
        }
    }
    
    SYSTEM_LOG << "[NodeGraphManager] Closed graph " << id.value << std::endl;
    
    return true;
}

// ========================================================================
// Active Graph Management
// ========================================================================

void NodeGraphManager::SetActiveGraph(GraphId id)
{
    auto it = m_graphs.find(id);
    if (it != m_graphs.end())
    {
        m_activeGraphId = id;
    }
}

GraphDocument* NodeGraphManager::GetActiveGraph()
{
    if (m_activeGraphId.value == 0)
        return nullptr;
    
    return GetGraph(m_activeGraphId);
}

// ========================================================================
// Queries
// ========================================================================

GraphDocument* NodeGraphManager::GetGraph(GraphId id)
{
    auto it = m_graphs.find(id);
    if (it != m_graphs.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::vector<GraphId> NodeGraphManager::GetAllGraphIds() const
{
    std::vector<GraphId> ids;
    for (auto it = m_graphs.begin(); it != m_graphs.end(); ++it)
    {
        ids.push_back(it->first);
    }
    return ids;
}

std::string NodeGraphManager::GetGraphName(GraphId id) const
{
    auto it = m_graphNames.find(id);
    if (it != m_graphNames.end())
    {
        return it->second;
    }
    return "";
}

} // namespace NodeGraph
} // namespace Olympe
