#pragma once

#include "./../../vector.h"
#include "ComponentNodeData.h"

namespace Olympe
{
    // Forward declarations
    class EntityPrefabGraphDocument;

    class ComponentNodeRenderer
    {
    public:
        ComponentNodeRenderer();
        ~ComponentNodeRenderer();

        void Initialize();
        void Shutdown();

        // Rendering
        void RenderNode(const ComponentNode& node);
        void RenderNodes(const EntityPrefabGraphDocument* document);
        void RenderConnections(const EntityPrefabGraphDocument* document);

        // Node styling
        void SetNodeStyle(const ComponentNodeStyle& style);
        const ComponentNodeStyle& GetNodeStyle() const;

        // Individual style components
        void SetNormalColor(const Vector& color);
        void SetSelectedColor(const Vector& color);
        void SetHoverColor(const Vector& color);
        void SetDisabledColor(const Vector& color);
        void SetTextColor(const Vector& color);
        void SetBorderWidth(float width);
        void SetCornerRadius(float radius);

        // Hit testing
        bool IsPointInNode(const Vector& point, const ComponentNode& node) const;
        bool GetNodeBounds(const ComponentNode& node, Vector& outMin, Vector& outMax) const;

        // Display options
        void SetShowLabels(bool show);
        bool GetShowLabels() const;

        void SetShowProperties(bool show);
        bool GetShowProperties() const;

        void SetNodeScale(float scale);
        float GetNodeScale() const;

    private:
        ComponentNodeStyle m_style;
        bool m_showLabels = true;
        bool m_showProperties = true;
        float m_nodeScale = 1.0f;

        void RenderNodeBox(const ComponentNode& node);
        void RenderNodeLabel(const ComponentNode& node);
        void RenderConnectionLine(const Vector& from, const Vector& to);
        Vector GetNodeColor(const ComponentNode& node) const;
    };
}
