#include "BTNodeGraphManager.h"
#include <fstream>
#include "../../Source/system/system_utils.h"

namespace Olympe
{

NodeGraphManager& NodeGraphManager::Instance()
{
    static NodeGraphManager instance;
    return instance;
}

NodeGraphManager::NodeGraphManager() : m_Initialized(false), m_ActiveGraphId(-1), m_NextGraphId(1) {}

NodeGraphManager::~NodeGraphManager() {}

void NodeGraphManager::Initialize()
{
    m_Initialized = true;
}

void NodeGraphManager::Shutdown()
{
    m_Graphs.clear();
}

int NodeGraphManager::CreateGraph(const std::string& name, const std::string& type)
{
    // Phase 61 FIX: CreateGraph MUST store the GraphDocument in m_Graphs
    // Previously just returned a new ID without storing anything
    // This caused GetActiveGraph() to return nullptr (m_Graphs.find returned end)
    // Solution: Create GraphDocument, store it, then return the ID (like LoadGraph does)

    int newGraphId = m_NextGraphId++;
    auto graphDoc = std::make_unique<GraphDocument>();
    graphDoc->type = type;
    graphDoc->graphKind = name;

    m_Graphs[newGraphId] = std::move(graphDoc);

    SYSTEM_LOG << "[NodeGraphManager::CreateGraph] Created new graph: id=" << newGraphId 
               << " name=" << name << " type=" << type << "\n";

    return newGraphId;
}

bool NodeGraphManager::CloseGraph(int graphId)
{
    return m_Graphs.erase(graphId) > 0;
}

GraphDocument* NodeGraphManager::GetGraph(int graphId)
{
    auto it = m_Graphs.find(graphId);
    return it != m_Graphs.end() ? it->second.get() : nullptr;
}

const GraphDocument* NodeGraphManager::GetGraph(int graphId) const
{
    auto it = m_Graphs.find(graphId);
    return it != m_Graphs.end() ? it->second.get() : nullptr;
}

void NodeGraphManager::SetActiveGraph(int graphId)
{
    m_ActiveGraphId = graphId;
}

GraphDocument* NodeGraphManager::GetActiveGraph()
{
    return GetGraph(m_ActiveGraphId);
}

const GraphDocument* NodeGraphManager::GetActiveGraph() const
{
    return GetGraph(m_ActiveGraphId);
}

std::vector<int> NodeGraphManager::GetAllGraphIds() const
{
    std::vector<int> ids;
    for (auto& p : m_Graphs)
        ids.push_back(p.first);
    return ids;
}

std::string NodeGraphManager::GetGraphName(int graphId) const
{
    return "";
}

void NodeGraphManager::SetGraphOrder(const std::vector<int>& newOrder)
{
    m_GraphOrder = newOrder;
}

bool NodeGraphManager::SaveGraph(int graphId, const std::string& filepath)
{
    auto it = m_Graphs.find(graphId);
    if (it == m_Graphs.end() || !it->second)
        return false;

    // For now, return true to indicate the graph exists
    // Actual serialization would be implemented here
    return true;
}

int NodeGraphManager::LoadGraph(const std::string& filepath)
{
    if (filepath.empty())
        return -1;

    try
    {
        // Read JSON file
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            SYSTEM_LOG << "[NodeGraphManager::LoadGraph] Failed to open file: " << filepath << "\n";
            return -1;
        }

        json jsonData;
        file >> jsonData;
        file.close();

        // Parse JSON into GraphDocument
        auto graphDoc = std::make_unique<GraphDocument>();
        *graphDoc = GraphDocument::FromJson(jsonData);

        // Store the loaded document
        int newGraphId = m_NextGraphId++;
        m_Graphs[newGraphId] = std::move(graphDoc);

        SYSTEM_LOG << "[NodeGraphManager::LoadGraph] Successfully loaded graph from " << filepath << " (id=" << newGraphId << ")\n";
        return newGraphId;
    }
    catch (const std::exception& e)
    {
        SYSTEM_LOG << "[NodeGraphManager::LoadGraph] Exception loading " << filepath << ": " << e.what() << "\n";
        return -1;
    }
}

bool NodeGraphManager::IsGraphDirty(int graphId) const
{
    return false;
}

bool NodeGraphManager::HasUnsavedChanges() const
{
    return false;
}

}  // namespace Olympe
