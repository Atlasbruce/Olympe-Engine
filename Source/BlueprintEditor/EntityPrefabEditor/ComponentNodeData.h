#pragma once

#include <string>
#include <map>
#include "./../../vector.h"
#include "./../../third_party/nlohmann/json.hpp"

namespace Olympe
{
    using json = nlohmann::json;
    typedef uint32_t NodeId;
    const NodeId InvalidNodeId = 0;

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
    };
}
