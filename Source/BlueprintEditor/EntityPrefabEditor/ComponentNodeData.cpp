/*
 * Olympe Entity Prefab Editor - Component Node Data
 * Visual representation of a component in the graph
 */

#include "ComponentNodeData.h"

namespace Olympe
{
    ComponentNode::ComponentNode()
        : nodeId(InvalidNodeId), selected(false), hovered(false), enabled(true)
    {
        position = Vector(0.0f, 0.0f, 0.0f);
        size = Vector(150.0f, 80.0f, 0.0f);
    }

    ComponentNode::ComponentNode(const std::string& type)
        : nodeId(InvalidNodeId), componentType(type), selected(false), hovered(false), enabled(true)
    {
        position = Vector(0.0f, 0.0f, 0.0f);
        size = Vector(150.0f, 80.0f, 0.0f);
    }

    bool ComponentNode::operator==(const ComponentNode& other) const
    {
        return nodeId == other.nodeId && componentType == other.componentType;
    }

    bool ComponentNode::operator!=(const ComponentNode& other) const
    {
        return !(*this == other);
    }

    std::string ComponentNode::ToDisplayString() const
    {
        std::string result = componentType;
        if (!componentName.empty())
        {
            result += " (" + componentName + ")";
        }
        if (!enabled)
        {
            result += " [Disabled]";
        }
        return result;
    }

    nlohmann::json ComponentNode::ToJson() const
    {
        json j;
        j["nodeId"] = static_cast<int>(nodeId);
        j["componentType"] = componentType;
        j["componentName"] = componentName;
        j["position"]["x"] = position.x;
        j["position"]["y"] = position.y;
        j["size"]["x"] = size.x;
        j["size"]["y"] = size.y;
        j["selected"] = selected;
        j["enabled"] = enabled;

        j["properties"] = json::object();
        for (auto it = properties.begin(); it != properties.end(); ++it)
        {
            j["properties"][it->first] = it->second;
        }

        j["ports"] = json::array();
        for (const auto& port : ports)
        {
            j["ports"].push_back(port.ToJson());
        }

        return j;
    }

    ComponentNode ComponentNode::FromJson(const nlohmann::json& data)
    {
        ComponentNode node;

        if (data.contains("nodeId"))
        {
            node.nodeId = static_cast<PrefabNodeId>(data["nodeId"].get<uint32_t>());
        }

        node.componentType = data.value("componentType", "");
        node.componentName = data.value("componentName", "");
        node.selected = data.value("selected", false);
        node.enabled = data.value("enabled", true);

        if (data.contains("position"))
        {
            node.position.x = data["position"].value("x", 0.0f);
            node.position.y = data["position"].value("y", 0.0f);
        }

        if (data.contains("size"))
        {
            node.size.x = data["size"].value("x", 150.0f);
            node.size.y = data["size"].value("y", 80.0f);
        }

        if (data.contains("properties"))
        {
            const nlohmann::json& propsJson = data["properties"];
            for (auto it = propsJson.begin(); it != propsJson.end(); ++it)
            {
                node.properties[it.key()] = it.value().dump();
            }
        }

        if (data.contains("ports"))
        {
            node.ports.clear();
            for (const auto& portJson : data["ports"])
            {
                NodePort port = NodePort::FromJson(portJson, node.nodeId);
                node.ports.push_back(port);
            }
        }
        else
        {
            node.InitializePorts(1, 1);
        }

        return node;
    }

    void ComponentNode::SetProperty(const std::string& key, const std::string& value)
    {
        properties[key] = value;
    }

    std::string ComponentNode::GetProperty(const std::string& key) const
    {
        auto it = properties.find(key);
        if (it != properties.end())
        {
            return it->second;
        }
        return "";
    }

    bool ComponentNode::HasProperty(const std::string& key) const
    {
        return properties.find(key) != properties.end();
    }

    Vector ComponentNode::GetCurrentColor() const
    {
        if (!enabled)
        {
            return style.disabledColor;
        }
        if (selected)
        {
            return style.selectedColor;
        }
        if (hovered)
        {
            return style.hoverColor;
        }
        return style.normalColor;
    }

    void ComponentNode::InitializePorts(uint32_t numInputPorts, uint32_t numOutputPorts)
    {
        ports.clear();

        for (uint32_t i = 0; i < numInputPorts; ++i)
        {
            NodePort port(nodeId, i, false);
            ports.push_back(port);
        }

        for (uint32_t i = 0; i < numOutputPorts; ++i)
        {
            NodePort port(nodeId, numInputPorts + i, true);
            ports.push_back(port);
        }
    }

    const std::vector<NodePort>& ComponentNode::GetPorts() const
    {
        return ports;
    }

    std::vector<NodePort>& ComponentNode::GetPorts()
    {
        return ports;
    }

    NodePort* ComponentNode::FindPort(PortId portId)
    {
        for (auto& port : ports)
        {
            if (port.portId == portId)
            {
                return &port;
            }
        }
        return nullptr;
    }

    const NodePort* ComponentNode::FindPort(PortId portId) const
    {
        for (const auto& port : ports)
        {
            if (port.portId == portId)
            {
                return &port;
            }
        }
        return nullptr;
    }
}
