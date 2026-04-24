#pragma once

#include "../Framework/IGraphDocument.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Olympe {

/**
 * @class PlaceholderGraphDocument
 * @brief Data model for Placeholder test graph type
 *
 * Simple test graph with 3 node types (Blue, Green, Magenta).
 * Each node has title and filepath properties.
 * Demonstrates framework integration before migrating existing types.
 *
 * C++14 compliant
 */

// Node type enumeration
enum class PlaceholderNodeType
{
    Blue = 0,
    Green = 1,
    Magenta = 2
};

// Single placeholder node
struct PlaceholderNode
{
    int nodeId;
    PlaceholderNodeType type;
    std::string title;
    std::string filepath;
    float posX;
    float posY;
    int width;
    int height;
    bool enabled;

    PlaceholderNode()
        : nodeId(0), type(PlaceholderNodeType::Blue), title(""), filepath(""),
          posX(0.0f), posY(0.0f), width(120), height(60), enabled(true)
    {
    }

    PlaceholderNode(int id, PlaceholderNodeType t, const std::string& n)
        : nodeId(id), type(t), title(n), filepath(""),
          posX(0.0f), posY(0.0f), width(120), height(60), enabled(true)
    {
    }
};

// Connection between two nodes
struct PlaceholderConnection
{
    int fromNodeId;
    int toNodeId;
    int fromPortIndex;
    int toPortIndex;

    PlaceholderConnection()
        : fromNodeId(-1), toNodeId(-1), fromPortIndex(0), toPortIndex(0)
    {
    }

    PlaceholderConnection(int from, int to, int fromPort = 0, int toPort = 0)
        : fromNodeId(from), toNodeId(to), fromPortIndex(fromPort), toPortIndex(toPort)
    {
    }
};

class PlaceholderGraphDocument : public IGraphDocument
{
public:
    PlaceholderGraphDocument();
    virtual ~PlaceholderGraphDocument();

    // IGraphDocument interface
    virtual bool Load(const std::string& filePath) override;
    virtual bool Save(const std::string& filePath) override;
    virtual bool IsDirty() const override { return m_isDirty; }
    virtual std::string GetName() const override;
    virtual DocumentType GetType() const override { return DocumentType::UNKNOWN; }
    virtual std::string GetFilePath() const override { return m_documentPath; }
    virtual void SetFilePath(const std::string& path) override { m_documentPath = path; }
    virtual IGraphRenderer* GetRenderer() override { return nullptr; }
    virtual const IGraphRenderer* GetRenderer() const override { return nullptr; }
    virtual void OnDocumentModified() override { m_isDirty = true; }

    // Placeholder-specific node operations
    int CreateNode(PlaceholderNodeType type, const std::string& title, float posX = 0.0f, float posY = 0.0f);
    bool DeleteNode(int nodeId);
    PlaceholderNode* GetNode(int nodeId);
    int GetNodeCount() const { return static_cast<int>(m_nodes.size()); }
    const std::vector<PlaceholderNode>& GetAllNodes() const { return m_nodes; }

    // Connection operations
    int CreateConnection(int fromNodeId, int toNodeId, int fromPort = 0, int toPort = 0);
    bool DeleteConnection(int fromNodeId, int toNodeId);
    const std::vector<PlaceholderConnection>& GetAllConnections() const { return m_connections; }

    // Node property updates
    void SetNodeTitle(int nodeId, const std::string& title);
    void SetNodeFilepath(int nodeId, const std::string& filepath);
    void SetNodePosition(int nodeId, float x, float y);
    void SetNodeEnabled(int nodeId, bool enabled);
    void SetDirty(bool dirty) { m_isDirty = dirty; }

    // Utility
    int GetNextNodeId() const;
    void Clear();

private:
    std::vector<PlaceholderNode> m_nodes;
    std::vector<PlaceholderConnection> m_connections;
    std::string m_documentPath;
    bool m_isDirty;
    int m_nextNodeId;

    // JSON serialization helpers
    bool SerializeToJson(const std::string& filepath);
    bool DeserializeFromJson(const std::string& filepath);
};

} // namespace Olympe
