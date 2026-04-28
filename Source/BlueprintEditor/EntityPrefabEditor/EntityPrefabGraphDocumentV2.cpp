#include "EntityPrefabGraphDocumentV2.h"
#include "PrefabLoader.h"
#include "../../system/system_utils.h"
#include "../Framework/IGraphRenderer.h"
#include <fstream>
#include <memory>

namespace Olympe
{
    // ========== CONSTRUCTOR / DESTRUCTOR ==========

    EntityPrefabGraphDocumentV2::EntityPrefabGraphDocumentV2()
        : m_canvasZoom(1.0f), m_nextNodeId(1), m_isDirty(false), m_renderer(nullptr)
    {
        // Renderer is created externally by framework adapter and set later
    }

    EntityPrefabGraphDocumentV2::~EntityPrefabGraphDocumentV2()
    {
        // Explicitly clear containers before destruction to prevent iterator corruption
        m_parameterSchemas.clear();
        m_nodes.clear();
        m_selectedNodes.clear();
        m_connections.clear();
    }

    // ========== NODE MANAGEMENT ==========

    PrefabNodeId EntityPrefabGraphDocumentV2::CreateComponentNode(const std::string& componentType)
    {
        return CreateComponentNode(componentType, "");
    }

    PrefabNodeId EntityPrefabGraphDocumentV2::CreateComponentNode(const std::string& componentType, const std::string& componentName)
    {
        ComponentNode node(componentType);
        node.nodeId = m_nextNodeId++;
        node.componentName = componentName;
        node.InitializePorts(1, 1);

        // Initialize node properties from parameter schema
        InitializeNodeProperties(node);

        m_nodes.push_back(node);
        m_isDirty = true;
        return node.nodeId;
    }

    void EntityPrefabGraphDocumentV2::RemoveNode(PrefabNodeId nodeId)
    {
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i].nodeId == nodeId)
            {
                m_nodes.erase(m_nodes.begin() + i);
                m_isDirty = true;
                break;
            }
        }
    }

    bool EntityPrefabGraphDocumentV2::HasNode(PrefabNodeId nodeId) const
    {
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i].nodeId == nodeId)
            {
                return true;
            }
        }
        return false;
    }

    ComponentNode* EntityPrefabGraphDocumentV2::GetNode(PrefabNodeId nodeId)
    {
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i].nodeId == nodeId)
            {
                return &m_nodes[i];
            }
        }
        return nullptr;
    }

    const ComponentNode* EntityPrefabGraphDocumentV2::GetNode(PrefabNodeId nodeId) const
    {
        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            if (m_nodes[i].nodeId == nodeId)
            {
                return &m_nodes[i];
            }
        }
        return nullptr;
    }

    const std::vector<ComponentNode>& EntityPrefabGraphDocumentV2::GetAllNodes() const
    {
        return m_nodes;
    }

    // ========== SELECTION MANAGEMENT ==========

    void EntityPrefabGraphDocumentV2::SelectNode(PrefabNodeId nodeId)
    {
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        {
            if (*it == nodeId)
            {
                return;  // Already selected
            }
        }
        m_selectedNodes.push_back(nodeId);
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr)
        {
            node->selected = true;
        }
    }

    void EntityPrefabGraphDocumentV2::DeselectNode(PrefabNodeId nodeId)
    {
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        {
            if (*it == nodeId)
            {
                m_selectedNodes.erase(it);
                break;
            }
        }
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr)
        {
            node->selected = false;
        }
    }

    void EntityPrefabGraphDocumentV2::DeselectAll()
    {
        for (size_t i = 0; i < m_selectedNodes.size(); ++i)
        {
            ComponentNode* node = GetNode(m_selectedNodes[i]);
            if (node != nullptr)
            {
                node->selected = false;
            }
        }
        m_selectedNodes.clear();
    }

    PrefabNodeId EntityPrefabGraphDocumentV2::GetSelectedNode() const
    {
        if (m_selectedNodes.size() > 0)
        {
            return m_selectedNodes[0];
        }
        return InvalidNodeId;
    }

    const std::vector<PrefabNodeId>& EntityPrefabGraphDocumentV2::GetSelectedNodes() const
    {
        return m_selectedNodes;
    }

    // ========== CONNECTION MANAGEMENT ==========

    bool EntityPrefabGraphDocumentV2::ConnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId)
    {
        if (!ValidateConnection(sourceId, targetId))
        {
            return false;
        }

        m_connections.push_back(std::make_pair(sourceId, targetId));
        m_isDirty = true;
        return true;
    }

    bool EntityPrefabGraphDocumentV2::DisconnectNodes(PrefabNodeId sourceId, PrefabNodeId targetId)
    {
        for (auto it = m_connections.begin(); it != m_connections.end(); ++it)
        {
            if (it->first == sourceId && it->second == targetId)
            {
                m_connections.erase(it);
                m_isDirty = true;
                return true;
            }
        }
        return false;
    }

    const std::vector<std::pair<PrefabNodeId, PrefabNodeId>>& EntityPrefabGraphDocumentV2::GetConnections() const
    {
        return m_connections;
    }

    bool EntityPrefabGraphDocumentV2::ValidateConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const
    {
        // No self-connections
        if (sourceId == targetId)
        {
            return false;
        }

        // No duplicate connections
        if (HasConnection(sourceId, targetId))
        {
            return false;
        }

        // Both nodes must exist
        if (!HasNode(sourceId) || !HasNode(targetId))
        {
            return false;
        }

        return true;
    }

    bool EntityPrefabGraphDocumentV2::HasConnection(PrefabNodeId sourceId, PrefabNodeId targetId) const
    {
        for (const auto& conn : m_connections)
        {
            if (conn.first == sourceId && conn.second == targetId)
            {
                return true;
            }
        }
        return false;
    }

    // ========== LAYOUT ==========

    void EntityPrefabGraphDocumentV2::AutoLayout()
    {
        // Placeholder
    }

    void EntityPrefabGraphDocumentV2::ArrangeNodesInGrid(int gridWidth, float spacing)
    {
        if (gridWidth < 1)
        {
            gridWidth = 3;
        }
        if (spacing < 50.0f)
        {
            spacing = 200.0f;
        }

        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            int row = i / gridWidth;
            int col = i % gridWidth;
            m_nodes[i].position.x = col * spacing;
            m_nodes[i].position.y = row * spacing;
        }
    }

    void EntityPrefabGraphDocumentV2::CenterViewport()
    {
        // Placeholder
    }

    // ========== SERIALIZATION (IGraphDocument Implementation) ==========

    bool EntityPrefabGraphDocumentV2::Load(const std::string& filePath)
    {
        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Load] Loading document from: " << filePath << "\n";

        if (!LoadFromFile(filePath))
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Load] ERROR: LoadFromFile failed\n";
            return false;
        }

        // Framework contract: Load() success means dirty cleared + path updated
        m_filePath = filePath;
        m_isDirty = false;
        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Load] SUCCESS: Document loaded\n";
        return true;
    }

    bool EntityPrefabGraphDocumentV2::Save(const std::string& filePath)
    {
        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Save] Saving document to: " << filePath << "\n";

        if (!SaveToFile(filePath))
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Save] ERROR: SaveToFile failed\n";
            return false;
        }

        // Framework contract: Save() success means path updated + dirty cleared
        m_filePath = filePath;
        m_isDirty = false;
        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::Save] SUCCESS: Document saved\n";
        return true;
    }

    std::string EntityPrefabGraphDocumentV2::GetName() const
    {
        if (!m_filePath.empty())
        {
            size_t lastSlash = m_filePath.find_last_of("/\\");
            if (lastSlash != std::string::npos)
            {
                return m_filePath.substr(lastSlash + 1);
            }
            return m_filePath;
        }
        return m_documentName.empty() ? "Untitled" : m_documentName;
    }

    DocumentType EntityPrefabGraphDocumentV2::GetType() const
    {
        return DocumentType::ENTITY_PREFAB;
    }

    std::string EntityPrefabGraphDocumentV2::GetFilePath() const
    {
        return m_filePath;
    }

    void EntityPrefabGraphDocumentV2::SetFilePath(const std::string& path)
    {
        m_filePath = path;
    }

    IGraphRenderer* EntityPrefabGraphDocumentV2::GetRenderer()
    {
        return m_renderer;
    }

    const IGraphRenderer* EntityPrefabGraphDocumentV2::GetRenderer() const
    {
        return m_renderer;
    }

    void EntityPrefabGraphDocumentV2::OnDocumentModified()
    {
        m_isDirty = true;
    }

    // ========== PROPERTIES ==========

    void EntityPrefabGraphDocumentV2::SetDocumentName(const std::string& name)
    {
        m_documentName = name;
    }

    std::string EntityPrefabGraphDocumentV2::GetDocumentName() const
    {
        return m_documentName;
    }

    Vector EntityPrefabGraphDocumentV2::GetCanvasOffset() const
    {
        return m_canvasOffset;
    }

    void EntityPrefabGraphDocumentV2::SetCanvasOffset(const Vector& offset)
    {
        m_canvasOffset = offset;
    }

    float EntityPrefabGraphDocumentV2::GetCanvasZoom() const
    {
        return m_canvasZoom;
    }

    void EntityPrefabGraphDocumentV2::SetCanvasZoom(float zoom)
    {
        m_canvasZoom = zoom;
    }

    bool EntityPrefabGraphDocumentV2::IsDirty() const
    {
        return m_isDirty;
    }

    void EntityPrefabGraphDocumentV2::SetDirty(bool dirty)
    {
        m_isDirty = dirty;
    }

    size_t EntityPrefabGraphDocumentV2::GetNodeCount() const
    {
        return m_nodes.size();
    }

    void EntityPrefabGraphDocumentV2::Clear()
    {
        m_nodes.clear();
        m_selectedNodes.clear();
        m_connections.clear();
        m_nextNodeId = 1;
        m_isDirty = false;
        m_canvasOffset = Vector(0.0f, 0.0f, 0.0f);
        m_canvasZoom = 1.0f;
    }

    // ========== PRIVATE HELPERS ==========

    PrefabNodeId EntityPrefabGraphDocumentV2::GenerateNodeId()
    {
        return m_nextNodeId++;
    }

    std::vector<EntityPrefabGraphDocumentV2::LayoutNode> EntityPrefabGraphDocumentV2::CalculateLayout()
    {
        std::vector<LayoutNode> layout;
        // Placeholder for layout calculation
        return layout;
    }

    // ========== FILE I/O IMPLEMENTATION ==========

    bool EntityPrefabGraphDocumentV2::LoadFromFile(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Starting load from: " << filePath << "\n";

            // Load JSON from file
            json data = PrefabLoader::LoadJsonFromFile(filePath);
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] JSON loaded successfully\n";

            // Verify structure
            if (!data.contains("data"))
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] ERROR: Missing 'data' section\n";
                return false;
            }

            // Clear existing data
            Clear();

            // Load parameter schemas for component properties
            LoadParameterSchemas("Gamedata\\EntityPrefab\\ComponentsParameters.json");
            if (m_parameterSchemas.empty())
            {
                LoadParameterSchemas("Gamedata/EntityPrefab/ComponentsParameters.json");
            }

            int nodesLoaded = 0;
            int connectionsLoaded = 0;

            // Parse nodes from JSON
            if (data["data"].contains("nodes") && data["data"]["nodes"].is_array())
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Parsing nodes...\n";

                for (const auto& nodeJson : data["data"]["nodes"])
                {
                    try
                    {
                        // Extract node data
                        std::string componentType = nodeJson.value("componentType", "");
                        std::string componentName = nodeJson.value("componentName", "");

                        if (componentType.empty())
                        {
                            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] WARNING: Node missing componentType\n";
                            continue;
                        }

                        // Create node
                        PrefabNodeId id = CreateComponentNode(componentType, componentName);
                        ComponentNode* node = GetNode(id);

                        if (node)
                        {
                            // Set position
                            if (nodeJson.contains("position") && nodeJson["position"].is_object())
                            {
                                float x = nodeJson["position"].value("x", 0.0f);
                                float y = nodeJson["position"].value("y", 0.0f);
                                float z = nodeJson["position"].value("z", 0.0f);
                                node->position = Vector(x, y, z);
                            }

                            // Set size
                            if (nodeJson.contains("size") && nodeJson["size"].is_object())
                            {
                                float x = nodeJson["size"].value("x", 150.0f);
                                float y = nodeJson["size"].value("y", 80.0f);
                                float z = nodeJson["size"].value("z", 0.0f);
                                node->size = Vector(x, y, z);
                            }

                            // Set enabled flag
                            node->enabled = nodeJson.value("enabled", true);

                            // Set selected flag
                            node->selected = nodeJson.value("selected", false);

                            // Set properties
                            if (nodeJson.contains("properties") && nodeJson["properties"].is_object())
                            {
                                for (auto it = nodeJson["properties"].begin(); it != nodeJson["properties"].end(); ++it)
                                {
                                    std::string key = it.key();
                                    const json& value = it.value();

                                    if (value.is_string())
                                    {
                                        node->properties[key] = value.get<std::string>();
                                    }
                                    else if (value.is_number())
                                    {
                                        node->properties[key] = std::to_string(value.get<double>());
                                    }
                                    else if (value.is_boolean())
                                    {
                                        node->properties[key] = value.get<bool>() ? "true" : "false";
                                    }
                                }
                            }

                            // Fill missing properties with schema defaults
                            auto schemaIt = m_parameterSchemas.find(componentType);
                            if (schemaIt != m_parameterSchemas.end())
                            {
                                for (const auto& paramPair : schemaIt->second)
                                {
                                    if (node->properties.find(paramPair.first) == node->properties.end())
                                    {
                                        node->properties[paramPair.first] = paramPair.second;
                                    }
                                }
                            }

                            nodesLoaded++;
                            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Loaded node: " << componentName
                                      << " (type=" << componentType << ", id=" << id << ")\n";
                        }
                    }
                    catch (const std::exception& e)
                    {
                        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] ERROR parsing node: " << e.what() << "\n";
                    }
                }
            }
            else
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] WARNING: 'nodes' array not found or invalid\n";
            }

            // Parse connections from JSON
            if (data["data"].contains("connections") && data["data"]["connections"].is_array())
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Parsing connections...\n";

                for (const auto& connJson : data["data"]["connections"])
                {
                    try
                    {
                        PrefabNodeId sourceId = connJson.value("sourceNodeId", InvalidNodeId);
                        PrefabNodeId targetId = connJson.value("targetNodeId", InvalidNodeId);

                        if (sourceId != InvalidNodeId && targetId != InvalidNodeId)
                        {
                            if (ConnectNodes(sourceId, targetId))
                            {
                                connectionsLoaded++;
                                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Loaded connection: "
                                          << sourceId << " -> " << targetId << "\n";
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] ERROR parsing connection: " << e.what() << "\n";
                    }
                }
            }
            else
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] WARNING: 'connections' array not found or invalid\n";
            }

            // Restore canvas state
            if (data["data"].contains("canvasState") && data["data"]["canvasState"].is_object())
            {
                try
                {
                    float zoom = data["data"]["canvasState"].value("zoom", 1.0f);
                    float offsetX = data["data"]["canvasState"].value("offsetX", 0.0f);
                    float offsetY = data["data"]["canvasState"].value("offsetY", 0.0f);

                    SetCanvasZoom(zoom);
                    SetCanvasOffset(Vector(offsetX, offsetY, 0.0f));

                    SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Canvas state restored: zoom="
                              << zoom << ", offset=(" << offsetX << ", " << offsetY << ")\n";
                }
                catch (const std::exception& e)
                {
                    SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] ERROR restoring canvas state: " << e.what() << "\n";
                }
            }

            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] SUCCESS: Loaded " << nodesLoaded
                      << " nodes and " << connectionsLoaded << " connections\n";

            // Fallback: If AIBehavior_data exists but no behaviorTreePath, read from root
            if (data.contains("data") && data["data"].contains("behaviorTreeRef"))
            {
                std::string btRef = data["data"]["behaviorTreeRef"].get<std::string>();
                for (auto& node : m_nodes)
                {
                    if (node.componentType == "AIBehavior_data")
                    {
                        if (node.properties.find("behaviorTreePath") == node.properties.end()
                            || node.properties["behaviorTreePath"].empty())
                        {
                            node.properties["behaviorTreePath"] = btRef;
                            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Fallback: Set behaviorTreePath from root\n";
                        }
                        break;
                    }
                }
            }

            // Auto-layout nodes if they're all at Y=0
            bool needsLayout = true;
            for (size_t i = 0; i < m_nodes.size(); ++i)
            {
                if (m_nodes[i].position.y != 0.0f)
                {
                    needsLayout = false;
                    break;
                }
            }

            if (needsLayout && m_nodes.size() > 1)
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] Auto-arranging nodes in grid layout\n";
                ArrangeNodesInGrid(3, 200.0f);
            }

            m_isDirty = false;
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadFromFile] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    bool EntityPrefabGraphDocumentV2::SaveToFile(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::SaveToFile] Starting save to: " << filePath << "\n";

            // Create JSON structure matching the load format
            json data = json::object();
            data["blueprintType"] = "EntityPrefab";
            data["schemaVersion"] = 4;

            json dataObj = json::object();

            // Serialize nodes
            json nodesArray = json::array();
            for (size_t i = 0; i < m_nodes.size(); ++i)
            {
                const ComponentNode& node = m_nodes[i];
                json nodeJson = json::object();

                nodeJson["nodeId"] = (int)node.nodeId;
                nodeJson["componentType"] = node.componentType;
                nodeJson["componentName"] = node.componentName;
                nodeJson["position"] = {{"x", node.position.x}, {"y", node.position.y}, {"z", node.position.z}};
                nodeJson["size"] = {{"x", node.size.x}, {"y", node.size.y}, {"z", node.size.z}};
                nodeJson["enabled"] = node.enabled;
                nodeJson["selected"] = node.selected;

                // Serialize properties
                json propertiesJson = json::object();
                for (const auto& propPair : node.properties)
                {
                    propertiesJson[propPair.first] = propPair.second;
                }
                nodeJson["properties"] = propertiesJson;

                nodesArray.push_back(nodeJson);
            }
            dataObj["nodes"] = nodesArray;

            // Serialize connections
            json connectionsArray = json::array();
            for (size_t i = 0; i < m_connections.size(); ++i)
            {
                const auto& conn = m_connections[i];
                json connJson = json::object();
                connJson["sourceNodeId"] = (int)conn.first;
                connJson["targetNodeId"] = (int)conn.second;
                connectionsArray.push_back(connJson);
            }
            dataObj["connections"] = connectionsArray;

            // Serialize canvas state
            json canvasStateJson = json::object();
            canvasStateJson["zoom"] = m_canvasZoom;
            canvasStateJson["offsetX"] = m_canvasOffset.x;
            canvasStateJson["offsetY"] = m_canvasOffset.y;
            dataObj["canvasState"] = canvasStateJson;

            data["data"] = dataObj;

            // Write to file
            PrefabLoader::SaveJsonToFile(filePath, data);
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::SaveToFile] SUCCESS: Saved " << m_nodes.size()
                      << " nodes and " << m_connections.size() << " connections\n";

            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::SaveToFile] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    // ========== PARAMETER SCHEMA MANAGEMENT ==========

    void EntityPrefabGraphDocumentV2::LoadParameterSchemas(const std::string& schemasFilePath)
    {
        // Implementation would load from ParameterSchemaRegistry or JSON file
        // For now, this is a placeholder that can be implemented later
        SYSTEM_LOG << "[EntityPrefabGraphDocumentV2::LoadParameterSchemas] Loading schemas from: " << schemasFilePath << "\n";
    }

    void EntityPrefabGraphDocumentV2::InitializeNodeProperties(ComponentNode& node)
    {
        // Initialize properties with defaults from schema
        auto schemaIt = m_parameterSchemas.find(node.componentType);
        if (schemaIt != m_parameterSchemas.end())
        {
            for (const auto& paramPair : schemaIt->second)
            {
                if (node.properties.find(paramPair.first) == node.properties.end())
                {
                    node.properties[paramPair.first] = paramPair.second;
                }
            }
        }
    }

} // namespace Olympe
