#include "PlaceholderGraphDocument.h"
#include <iostream>
#include <algorithm>
#include <fstream>

namespace Olympe {

PlaceholderGraphDocument::PlaceholderGraphDocument()
    : m_documentPath(""), m_isDirty(false), m_nextNodeId(1)
{
}

PlaceholderGraphDocument::~PlaceholderGraphDocument()
{
    Clear();
}

bool PlaceholderGraphDocument::Load(const std::string& filePath)
{
    std::cout << "[PlaceholderGraphDocument] Loading from: " << filePath << std::endl;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cout << "[PlaceholderGraphDocument] ERROR: Cannot open file: " << filePath << std::endl;
        return false;
    }

    m_documentPath = filePath;
    m_isDirty = false;

    // For now, just mark as loaded (stub implementation)
    // Full JSON parsing will be added when nlohmann/json is available
    std::cout << "[PlaceholderGraphDocument] Loaded successfully" << std::endl;
    return true;
}

bool PlaceholderGraphDocument::Save(const std::string& filePath)
{
    std::cout << "[PlaceholderGraphDocument] Saving to: " << filePath << std::endl;

    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cout << "[PlaceholderGraphDocument] ERROR: Cannot write to file: " << filePath << std::endl;
        return false;
    }

    // Write basic structure
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"nodeCount\": " << m_nodes.size() << ",\n";
    file << "  \"connectionCount\": " << m_connections.size() << "\n";
    file << "}\n";

    file.close();
    m_documentPath = filePath;
    m_isDirty = false;

    std::cout << "[PlaceholderGraphDocument] Saved successfully" << std::endl;
    return true;
}

std::string PlaceholderGraphDocument::GetName() const
{
    // Extract filename from path
    size_t lastSlash = m_documentPath.find_last_of("/\\");
    if (lastSlash == std::string::npos)
    {
        return m_documentPath.empty() ? "Untitled" : m_documentPath;
    }
    return m_documentPath.substr(lastSlash + 1);
}

int PlaceholderGraphDocument::CreateNode(PlaceholderNodeType type, const std::string& title, float posX, float posY)
{
    PlaceholderNode node(m_nextNodeId, type, title);
    node.posX = posX;
    node.posY = posY;

    m_nodes.push_back(node);
    int nodeId = m_nextNodeId;
    m_nextNodeId++;
    m_isDirty = true;

    std::cout << "[PlaceholderGraphDocument] Created node " << nodeId << " (" << title << ")" << std::endl;
    return nodeId;
}

bool PlaceholderGraphDocument::DeleteNode(int nodeId)
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [nodeId](const PlaceholderNode& n) { return n.nodeId == nodeId; });

    if (it == m_nodes.end())
    {
        std::cout << "[PlaceholderGraphDocument] WARNING: Node " << nodeId << " not found" << std::endl;
        return false;
    }

    // Remove all connections involving this node
    auto connIt = std::remove_if(m_connections.begin(), m_connections.end(),
        [nodeId](const PlaceholderConnection& c) {
            return c.fromNodeId == nodeId || c.toNodeId == nodeId;
        });
    m_connections.erase(connIt, m_connections.end());

    m_nodes.erase(it);
    m_isDirty = true;

    std::cout << "[PlaceholderGraphDocument] Deleted node " << nodeId << std::endl;
    return true;
}

PlaceholderNode* PlaceholderGraphDocument::GetNode(int nodeId)
{
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [nodeId](PlaceholderNode& n) { return n.nodeId == nodeId; });

    if (it == m_nodes.end())
    {
        return nullptr;
    }
    return &(*it);
}

int PlaceholderGraphDocument::CreateConnection(int fromNodeId, int toNodeId, int fromPort, int toPort)
{
    // Validate nodes exist
    if (!GetNode(fromNodeId) || !GetNode(toNodeId))
    {
        std::cout << "[PlaceholderGraphDocument] ERROR: Cannot create connection - node not found" << std::endl;
        return -1;
    }

    // Prevent self-connections
    if (fromNodeId == toNodeId)
    {
        std::cout << "[PlaceholderGraphDocument] ERROR: Cannot create self-connection" << std::endl;
        return -1;
    }

    // Check for duplicate connection
    for (const auto& conn : m_connections)
    {
        if (conn.fromNodeId == fromNodeId && conn.toNodeId == toNodeId)
        {
            std::cout << "[PlaceholderGraphDocument] WARNING: Connection already exists" << std::endl;
            return -1;
        }
    }

    PlaceholderConnection conn(fromNodeId, toNodeId, fromPort, toPort);
    m_connections.push_back(conn);
    m_isDirty = true;

    std::cout << "[PlaceholderGraphDocument] Created connection: " << fromNodeId << " -> " << toNodeId << std::endl;
    return static_cast<int>(m_connections.size()) - 1;
}

bool PlaceholderGraphDocument::DeleteConnection(int fromNodeId, int toNodeId)
{
    auto it = std::find_if(m_connections.begin(), m_connections.end(),
        [fromNodeId, toNodeId](const PlaceholderConnection& c) {
            return c.fromNodeId == fromNodeId && c.toNodeId == toNodeId;
        });

    if (it == m_connections.end())
    {
        std::cout << "[PlaceholderGraphDocument] WARNING: Connection not found" << std::endl;
        return false;
    }

    m_connections.erase(it);
    m_isDirty = true;

    std::cout << "[PlaceholderGraphDocument] Deleted connection: " << fromNodeId << " -> " << toNodeId << std::endl;
    return true;
}

void PlaceholderGraphDocument::SetNodeTitle(int nodeId, const std::string& title)
{
    PlaceholderNode* node = GetNode(nodeId);
    if (node)
    {
        node->title = title;
        m_isDirty = true;
    }
}

void PlaceholderGraphDocument::SetNodeFilepath(int nodeId, const std::string& filepath)
{
    PlaceholderNode* node = GetNode(nodeId);
    if (node)
    {
        node->filepath = filepath;
        m_isDirty = true;
    }
}

void PlaceholderGraphDocument::SetNodePosition(int nodeId, float x, float y)
{
    PlaceholderNode* node = GetNode(nodeId);
    if (node)
    {
        node->posX = x;
        node->posY = y;
        m_isDirty = true;
    }
}

void PlaceholderGraphDocument::SetNodeEnabled(int nodeId, bool enabled)
{
    PlaceholderNode* node = GetNode(nodeId);
    if (node)
    {
        node->enabled = enabled;
        m_isDirty = true;
    }
}

int PlaceholderGraphDocument::GetNextNodeId() const
{
    return m_nextNodeId;
}

void PlaceholderGraphDocument::Clear()
{
    m_nodes.clear();
    m_connections.clear();
    m_nextNodeId = 1;
    m_isDirty = false;
    std::cout << "[PlaceholderGraphDocument] Cleared" << std::endl;
}

bool PlaceholderGraphDocument::SerializeToJson(const std::string& filepath)
{
    // Stub: Full JSON serialization when nlohmann/json integration available
    return Save(filepath);
}

bool PlaceholderGraphDocument::DeserializeFromJson(const std::string& filepath)
{
    // Stub: Full JSON deserialization when nlohmann/json integration available
    return Load(filepath);
}

} // namespace Olympe
