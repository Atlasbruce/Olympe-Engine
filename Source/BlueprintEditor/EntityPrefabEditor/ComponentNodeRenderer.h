#pragma once

#include "./../../vector.h"
#include "ComponentNodeData.h"
#include "./../../third_party/imgui/imgui.h"

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

        // Canvas transformation context
        void SetCanvasTransform(const Vector& offset, float zoom);
        Vector GetCanvasOffset() const;
        float GetCanvasZoom() const;
        void SetCanvasScreenPos(const ImVec2& screenPos);
        ImVec2 GetCanvasScreenPos() const;

        // Rendering
        void RenderNode(const ComponentNode& node);
        void RenderNodes(const EntityPrefabGraphDocument* document);
        void RenderConnections(const EntityPrefabGraphDocument* document, int hoveredConnectionIndex = -1);

        // Port rendering
        void RenderNodePorts(const ComponentNode& node);
        bool IsPointInPort(const Vector& point, const ComponentNode& node, PortId& outPortId) const;

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

        // Connection hit detection
        float GetDistanceToConnection(
            const Vector& testPoint, 
            const Vector& connectionStart, 
            const Vector& connectionEnd,
            Vector* outClosestPoint = nullptr
        ) const;

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
        Vector m_canvasOffset;
        float m_canvasZoom = 1.0f;
        ImVec2 m_canvasScreenPos = ImVec2(0, 0);

        void RenderNodeBox(const ComponentNode& node);
        void RenderNodeLabel(const ComponentNode& node);
        void RenderConnectionLine(const Vector& from, const Vector& to, bool isHovered = false);
        void RenderPort(const ComponentNode& node, const NodePort& port);
        Vector GetNodeColor(const ComponentNode& node) const;
        Vector CanvasToScreen(const Vector& canvasPos) const;
        void UpdatePortPositions(ComponentNode& node) const;
    };
}
