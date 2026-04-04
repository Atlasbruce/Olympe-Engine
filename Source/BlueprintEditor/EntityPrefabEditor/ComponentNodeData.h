#pragma once

#include <string>
#include <map>
#include <vector>
#include "./../../vector.h"
#include "./../../third_party/nlohmann/json.hpp"

namespace Olympe
{
    using json = nlohmann::json;
    typedef uint32_t NodeId;
    typedef uint32_t PortId;
    const NodeId InvalidNodeId = 0;
    const PortId InvalidPortId = 0;

    struct NodePort
    {
        PortId portId = InvalidPortId;
        NodeId nodeId = InvalidNodeId;
        uint32_t portIndex = 0;
        Vector position;
        float radius = 4.0f;
        bool isOutput = false;

        NodePort() = default;
        NodePort(NodeId nodeId, uint32_t index, bool isOutput = false)
            : portId(index + 1), nodeId(nodeId), portIndex(index), isOutput(isOutput)
        {
        }

        json ToJson() const
        {
            json j;
            j["portId"] = static_cast<int>(portId);
            j["portIndex"] = static_cast<int>(portIndex);
            j["position"] = { {"x", position.x}, {"y", position.y}, {"z", position.z} };
            j["radius"] = radius;
            j["isOutput"] = isOutput;
            return j;
        }

        static NodePort FromJson(const json& data, NodeId nodeId)
        {
            NodePort port;
            port.nodeId = nodeId;
            port.portId = data.value("portId", InvalidPortId);
            port.portIndex = data.value("portIndex", 0);
            port.radius = data.value("radius", 4.0f);
            port.isOutput = data.value("isOutput", false);
            if (data.contains("position"))
            {
                port.position.x = data["position"].value("x", 0.0f);
                port.position.y = data["position"].value("y", 0.0f);
                port.position.z = data["position"].value("z", 0.0f);
            }
            return port;
        }
    };

    struct ComponentNodeStyle
    {
        Vector normalColor;
        Vector selectedColor;
        Vector hoverColor;
        Vector disabledColor;
        Vector textColor;
        float borderWidth;
        float cornerRadius;

        ComponentNodeStyle()
            : normalColor(0.2f, 0.3f, 0.5f),
              selectedColor(0.4f, 0.6f, 0.9f),
              hoverColor(0.3f, 0.5f, 0.8f),
              disabledColor(0.3f, 0.3f, 0.3f),
              textColor(1.0f, 1.0f, 1.0f),
              borderWidth(2.0f),
              cornerRadius(4.0f)
        {
        }
    };

    struct ComponentNode
    {
        NodeId nodeId = InvalidNodeId;
        std::string componentType;
        std::string componentName;
        Vector position;
        Vector size = Vector(150.0f, 80.0f, 0.0f);
        bool selected = false;
        bool hovered = false;
        bool enabled = true;
        std::map<std::string, std::string> properties;
        ComponentNodeStyle style;
        std::vector<NodePort> ports;

        ComponentNode();
        explicit ComponentNode(const std::string& type);

        bool operator==(const ComponentNode& other) const;
        bool operator!=(const ComponentNode& other) const;

        std::string ToDisplayString() const;

        json ToJson() const;
        static ComponentNode FromJson(const json& data);

        void SetProperty(const std::string& key, const std::string& value);
        std::string GetProperty(const std::string& key) const;
        bool HasProperty(const std::string& key) const;

        Vector GetCurrentColor() const;

        // Port management
        void InitializePorts(uint32_t numInputPorts = 1, uint32_t numOutputPorts = 1);
        const std::vector<NodePort>& GetPorts() const;
        std::vector<NodePort>& GetPorts();
        NodePort* FindPort(PortId portId);
        const NodePort* FindPort(PortId portId) const;
    };
}
