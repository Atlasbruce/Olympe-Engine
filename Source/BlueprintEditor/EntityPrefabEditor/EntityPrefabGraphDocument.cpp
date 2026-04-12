#include "EntityPrefabGraphDocument.h"
#include "PrefabLoader.h"
#include "../../system/system_utils.h"
#include <fstream>

namespace Olympe
{
    EntityPrefabGraphDocument::EntityPrefabGraphDocument() : m_canvasZoom(1.0f), m_nextNodeId(1) { }
    EntityPrefabGraphDocument::~EntityPrefabGraphDocument() { }

    NodeId EntityPrefabGraphDocument::CreateComponentNode(const std::string& componentType)
    { return CreateComponentNode(componentType, ""); }

    NodeId EntityPrefabGraphDocument::CreateComponentNode(const std::string& componentType, const std::string& componentName)
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

    void EntityPrefabGraphDocument::RemoveNode(NodeId nodeId)
    { 
        for (size_t i = 0; i < m_nodes.size(); ++i)
        { if (m_nodes[i].nodeId == nodeId) { m_nodes.erase(m_nodes.begin() + i); m_isDirty = true; break; } }
    }

    bool EntityPrefabGraphDocument::HasNode(NodeId nodeId) const
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return true; } } return false; }

    ComponentNode* EntityPrefabGraphDocument::GetNode(NodeId nodeId)
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return &m_nodes[i]; } } return nullptr; }

    const ComponentNode* EntityPrefabGraphDocument::GetNode(NodeId nodeId) const
    { for (size_t i = 0; i < m_nodes.size(); ++i) { if (m_nodes[i].nodeId == nodeId) { return &m_nodes[i]; } } return nullptr; }

    const std::vector<ComponentNode>& EntityPrefabGraphDocument::GetAllNodes() const { return m_nodes; }

    void EntityPrefabGraphDocument::SelectNode(NodeId nodeId)
    { 
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        { if (*it == nodeId) { return; } }
        m_selectedNodes.push_back(nodeId);
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr) { node->selected = true; }
    }

    void EntityPrefabGraphDocument::DeselectNode(NodeId nodeId)
    { 
        for (auto it = m_selectedNodes.begin(); it != m_selectedNodes.end(); ++it)
        { if (*it == nodeId) { m_selectedNodes.erase(it); break; } }
        ComponentNode* node = GetNode(nodeId);
        if (node != nullptr) { node->selected = false; }
    }

    void EntityPrefabGraphDocument::DeselectAll()
    { 
        for (size_t i = 0; i < m_selectedNodes.size(); ++i)
        { ComponentNode* node = GetNode(m_selectedNodes[i]); if (node != nullptr) { node->selected = false; } }
        m_selectedNodes.clear();
    }

    NodeId EntityPrefabGraphDocument::GetSelectedNode() const
    { if (m_selectedNodes.size() > 0) { return m_selectedNodes[0]; } return InvalidNodeId; }

    const std::vector<NodeId>& EntityPrefabGraphDocument::GetSelectedNodes() const { return m_selectedNodes; }

    void EntityPrefabGraphDocument::AutoLayout() { }

    void EntityPrefabGraphDocument::ArrangeNodesInGrid(int gridWidth, float spacing)
    { 
        if (gridWidth < 1) gridWidth = 3;
        if (spacing < 50.0f) spacing = 200.0f;

        for (size_t i = 0; i < m_nodes.size(); ++i)
        {
            int row = i / gridWidth;
            int col = i % gridWidth;
            m_nodes[i].position.x = col * spacing;
            m_nodes[i].position.y = row * spacing;
        }
    }
    void EntityPrefabGraphDocument::CenterViewport() { }

    bool EntityPrefabGraphDocument::ConnectNodes(NodeId sourceId, NodeId targetId)
    { 
        // Validate the connection first
        if (!ValidateConnection(sourceId, targetId))
        {
            return false;
        }
        m_connections.push_back(std::make_pair(sourceId, targetId));
        m_isDirty = true;
        return true;
    }

    bool EntityPrefabGraphDocument::DisconnectNodes(NodeId sourceId, NodeId targetId)
    { 
        for (auto it = m_connections.begin(); it != m_connections.end(); ++it)
        { if (it->first == sourceId && it->second == targetId) { m_connections.erase(it); m_isDirty = true; return true; } }
        return false;
    }

    const std::vector<std::pair<NodeId, NodeId>>& EntityPrefabGraphDocument::GetConnections() const { return m_connections; }

    bool EntityPrefabGraphDocument::ValidateConnection(NodeId sourceId, NodeId targetId) const
    {
        // Prevent self-connections
        if (sourceId == targetId)
        {
            return false;
        }

        // Check if both nodes exist
        if (!HasNode(sourceId) || !HasNode(targetId))
        {
            return false;
        }

        // Prevent duplicate connections
        if (HasConnection(sourceId, targetId))
        {
            return false;
        }

        return true;
    }

    bool EntityPrefabGraphDocument::HasConnection(NodeId sourceId, NodeId targetId) const
    {
        for (const auto& connection : m_connections)
        {
            if (connection.first == sourceId && connection.second == targetId)
            {
                return true;
            }
        }
        return false;
    }

    json EntityPrefabGraphDocument::ToJson() const { return json::object(); }
    EntityPrefabGraphDocument EntityPrefabGraphDocument::FromJson(const json& data) { (void)data; return EntityPrefabGraphDocument(); }

    bool EntityPrefabGraphDocument::LoadFromFile(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Starting load from: " << filePath << "\n";

            // Load JSON from file
            json data = PrefabLoader::LoadJsonFromFile(filePath);
            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] JSON loaded successfully\n";

            // Verify structure
            if (!data.contains("data"))
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] ERROR: Missing 'data' section\n";
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
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Parsing nodes...\n";

                for (const auto& nodeJson : data["data"]["nodes"])
                {
                    try
                    {
                        // Extract node data
                        std::string componentType = nodeJson.value("componentType", "");
                        std::string componentName = nodeJson.value("componentName", "");

                        if (componentType.empty())
                        {
                            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] WARNING: Node missing componentType\n";
                            continue;
                        }

                        // Create node
                        NodeId id = CreateComponentNode(componentType, componentName);
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
                            // This handles cases where JSON properties are empty but schema provides defaults
                            auto schemaIt = m_parameterSchemas.find(componentType);
                            if (schemaIt != m_parameterSchemas.end())
                            {
                                for (const auto& paramPair : schemaIt->second)
                                {
                                    // Only add if not already loaded from JSON
                                    if (node->properties.find(paramPair.first) == node->properties.end())
                                    {
                                        node->properties[paramPair.first] = paramPair.second;
                                    }
                                }
                            }

                            nodesLoaded++;
                            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Loaded node: " << componentName 
                                      << " (type=" << componentType << ", id=" << id << ")\n";
                        }
                    }
                    catch (const std::exception& e)
                    {
                        SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] ERROR parsing node: " << e.what() << "\n";
                    }
                }
            }
            else
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] WARNING: 'nodes' array not found or invalid\n";
            }

            // Parse connections from JSON
            if (data["data"].contains("connections") && data["data"]["connections"].is_array())
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Parsing connections...\n";

                for (const auto& connJson : data["data"]["connections"])
                {
                    try
                    {
                        NodeId sourceId = connJson.value("sourceNodeId", InvalidNodeId);
                        NodeId targetId = connJson.value("targetNodeId", InvalidNodeId);

                        if (sourceId != InvalidNodeId && targetId != InvalidNodeId)
                        {
                            if (ConnectNodes(sourceId, targetId))
                            {
                                connectionsLoaded++;
                                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Loaded connection: " 
                                          << sourceId << " -> " << targetId << "\n";
                            }
                        }
                    }
                    catch (const std::exception& e)
                    {
                        SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] ERROR parsing connection: " << e.what() << "\n";
                    }
                }
            }
            else
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] WARNING: 'connections' array not found or invalid\n";
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

                    SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Canvas state restored: zoom=" 
                              << zoom << ", offset=(" << offsetX << ", " << offsetY << ")\n";
                }
                catch (const std::exception& e)
                {
                    SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] ERROR restoring canvas state: " << e.what() << "\n";
                }
            }

            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] SUCCESS: Loaded " << nodesLoaded 
                      << " nodes and " << connectionsLoaded << " connections\n";

            // Fallback : si AIBehavior_data existe mais sans behaviorTreePath, lire depuis la racine
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
                            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Fallback: Set behaviorTreePath from root behaviorTreeRef\n";
                        }
                        break;
                    }
                }
            }

            // Auto-layout nodes if they're all at Y=0 (not properly positioned)
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
                SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] Auto-arranging nodes in grid layout\n";
                ArrangeNodesInGrid(3, 200.0f);  // 3 columns, 200 pixel spacing
            }

            m_isDirty = false;
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocument::LoadFromFile] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    bool EntityPrefabGraphDocument::SaveToFile(const std::string& filePath)
    {
        try
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocument::SaveToFile] Starting save to: " << filePath << "\n";

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
                nodeJson["enabled"] = node.enabled;
                nodeJson["selected"] = node.selected;

                // Serialize position
                json posJson = json::object();
                posJson["x"] = node.position.x;
                posJson["y"] = node.position.y;
                posJson["z"] = node.position.z;
                nodeJson["position"] = posJson;

                // Serialize size
                json sizeJson = json::object();
                sizeJson["x"] = node.size.x;
                sizeJson["y"] = node.size.y;
                sizeJson["z"] = node.size.z;
                nodeJson["size"] = sizeJson;

                // Serialize properties
                json propsJson = json::object();
                for (auto it = node.properties.begin(); it != node.properties.end(); ++it)
                {
                    propsJson[it->first] = it->second;
                }
                nodeJson["properties"] = propsJson;

                nodesArray.push_back(nodeJson);
            }
            dataObj["nodes"] = nodesArray;

            // Serialize connections
            json connectionsArray = json::array();
            for (size_t i = 0; i < m_connections.size(); ++i)
            {
                json connJson = json::object();
                connJson["sourceNodeId"] = (int)m_connections[i].first;
                connJson["targetNodeId"] = (int)m_connections[i].second;
                connectionsArray.push_back(connJson);
            }
            dataObj["connections"] = connectionsArray;

            // Serialize canvas state
            json canvasStateJson = json::object();
            canvasStateJson["zoom"] = m_canvasZoom;
            canvasStateJson["offsetX"] = m_canvasOffset.x;
            canvasStateJson["offsetY"] = m_canvasOffset.y;
            dataObj["canvasState"] = canvasStateJson;

            // Expose behaviorTreePath au niveau racine pour faciliter la lecture runtime
            for (const auto& node : m_nodes)
            {
                if (node.componentType == "AIBehavior_data")
                {
                    auto it = node.properties.find("behaviorTreePath");
                    if (it != node.properties.end() && !it->second.empty())
                        dataObj["behaviorTreeRef"] = it->second;
                    break;
                }
            }

            data["data"] = dataObj;

            // Save JSON to file
            PrefabLoader::SaveJsonToFile(filePath, data);

            SYSTEM_LOG << "[EntityPrefabGraphDocument::SaveToFile] SUCCESS: Saved " << m_nodes.size() 
                      << " nodes and " << m_connections.size() << " connections\n";

            m_isDirty = false;
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocument::SaveToFile] EXCEPTION: " << e.what() << "\n";
            return false;
        }
    }

    void EntityPrefabGraphDocument::SetDocumentName(const std::string& name) { m_documentName = name; }
    std::string EntityPrefabGraphDocument::GetDocumentName() const { return m_documentName; }

    Vector EntityPrefabGraphDocument::GetCanvasOffset() const { return m_canvasOffset; }
    void EntityPrefabGraphDocument::SetCanvasOffset(const Vector& offset) { m_canvasOffset = offset; }

    float EntityPrefabGraphDocument::GetCanvasZoom() const { return m_canvasZoom; }
    void EntityPrefabGraphDocument::SetCanvasZoom(float zoom) { m_canvasZoom = zoom; }

    void EntityPrefabGraphDocument::Clear() { m_nodes.clear(); m_selectedNodes.clear(); m_connections.clear(); m_nextNodeId = 1; m_isDirty = false; }
    size_t EntityPrefabGraphDocument::GetNodeCount() const { return m_nodes.size(); }

    bool EntityPrefabGraphDocument::IsDirty() const { return m_isDirty; }
    void EntityPrefabGraphDocument::SetDirty(bool dirty) { m_isDirty = dirty; }

    void EntityPrefabGraphDocument::LoadParameterSchemas(const std::string& schemasFilePath)
    {
        using nlohmann::json;

        SYSTEM_LOG << "[EntityPrefabGraphDocument] Loading parameter schemas from: " << schemasFilePath << "\n";

        try
        {
            std::ifstream file(schemasFilePath);
            if (!file.is_open())
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument] WARNING: Could not open schemas file\n";
                return;
            }

            json jsonData;
            file >> jsonData;
            file.close();

            if (!jsonData.contains("schemas") || !jsonData["schemas"].is_array())
            {
                SYSTEM_LOG << "[EntityPrefabGraphDocument] WARNING: Schemas file missing 'schemas' array\n";
                return;
            }

            const json& schemasArray = jsonData["schemas"];
            for (const auto& schemaJson : schemasArray)
            {
                if (!schemaJson.contains("componentType"))
                    continue;

                std::string componentType = schemaJson["componentType"].get<std::string>();
                std::map<std::string, std::string> params;

                if (schemaJson.contains("parameters") && schemaJson["parameters"].is_array())
                {
                    for (const auto& paramJson : schemaJson["parameters"])
                    {
                        if (!paramJson.contains("name") || !paramJson.contains("defaultValue"))
                            continue;

                        std::string paramName = paramJson["name"].get<std::string>();

                        // Convert default value to string
                        std::string defaultValue;
                        if (paramJson["defaultValue"].is_string())
                        {
                            defaultValue = paramJson["defaultValue"].get<std::string>();
                        }
                        else if (paramJson["defaultValue"].is_boolean())
                        {
                            defaultValue = paramJson["defaultValue"].get<bool>() ? "true" : "false";
                        }
                        else if (paramJson["defaultValue"].is_number_integer())
                        {
                            defaultValue = std::to_string(paramJson["defaultValue"].get<int>());
                        }
                        else if (paramJson["defaultValue"].is_number_float())
                        {
                            defaultValue = std::to_string(paramJson["defaultValue"].get<float>());
                        }
                        else if (paramJson["defaultValue"].is_array())
                        {
                            // For arrays, store as JSON string
                            defaultValue = paramJson["defaultValue"].dump();
                        }

                        params[paramName] = defaultValue;
                    }
                }

                m_parameterSchemas[componentType] = params;
                SYSTEM_LOG << "[EntityPrefabGraphDocument] Loaded " << params.size() << " parameters for " << componentType << "\n";
            }

            SYSTEM_LOG << "[EntityPrefabGraphDocument] Successfully loaded " << m_parameterSchemas.size() << " component schemas\n";
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "[EntityPrefabGraphDocument] ERROR loading schemas: " << e.what() << "\n";
        }
    }

    void EntityPrefabGraphDocument::InitializeNodeProperties(ComponentNode& node)
    {
        // Look up component schema and populate node properties with defaults
        auto it = m_parameterSchemas.find(node.componentType);
        if (it != m_parameterSchemas.end())
        {
            for (const auto& paramPair : it->second)
            {
                node.SetProperty(paramPair.first, paramPair.second);
            }
        }
    }

    std::vector<LayoutNode> EntityPrefabGraphDocument::CalculateLayout() { return std::vector<LayoutNode>(); }

} // namespace Olympe
