#include "ComponentNodeRenderer.h"
#include "EntityPrefabGraphDocument.h"
#include "./../../third_party/imgui/imgui.h"
#include "../../system/system_utils.h"

namespace Olympe
{
    ComponentNodeRenderer::ComponentNodeRenderer() : m_showLabels(true), m_showProperties(true), m_nodeScale(1.0f) 
    { 
        m_style.normalColor = Vector(0.3f, 0.3f, 0.3f);
        m_style.selectedColor = Vector(0.0f, 0.7f, 1.0f);
        m_style.hoverColor = Vector(0.5f, 0.5f, 0.5f);
        m_style.disabledColor = Vector(0.2f, 0.2f, 0.2f);
        m_style.textColor = Vector(1.0f, 1.0f, 1.0f);
        m_style.borderWidth = 2.0f;
        m_style.cornerRadius = 5.0f;
    }
    ComponentNodeRenderer::~ComponentNodeRenderer() { }

    void ComponentNodeRenderer::Initialize() { }
    void ComponentNodeRenderer::Shutdown() { }

    void ComponentNodeRenderer::SetCanvasTransform(const Vector& offset, float zoom)
    {
        m_canvasOffset = offset;
        m_canvasZoom = (zoom > 0.1f) ? zoom : 0.1f;
    }

    Vector ComponentNodeRenderer::GetCanvasOffset() const { return m_canvasOffset; }
    float ComponentNodeRenderer::GetCanvasZoom() const { return m_canvasZoom; }

    void ComponentNodeRenderer::SetCanvasScreenPos(const ImVec2& screenPos) { m_canvasScreenPos = screenPos; }
    ImVec2 ComponentNodeRenderer::GetCanvasScreenPos() const { return m_canvasScreenPos; }

    Vector ComponentNodeRenderer::CanvasToScreen(const Vector& canvasPos) const
    {
        Vector screenPos = canvasPos;
        screenPos.x = screenPos.x * m_canvasZoom + m_canvasOffset.x + m_canvasScreenPos.x;
        screenPos.y = screenPos.y * m_canvasZoom + m_canvasOffset.y + m_canvasScreenPos.y;
        return screenPos;
    }

    void ComponentNodeRenderer::RenderNode(const ComponentNode& node)
    {
        RenderNodeBox(node);
        if (m_showLabels)
        {
            RenderNodeLabel(node);
        }
        RenderNodePorts(node);
    }

    void ComponentNodeRenderer::RenderNodes(const EntityPrefabGraphDocument* document)
    {
        if (document == nullptr) { return; }

        // Get the ImGui child window's screen position (top-left corner)
        ImVec2 canvasScreenPos = ImGui::GetCursorScreenPos();

        const std::vector<ComponentNode>& nodes = document->GetAllNodes();
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            RenderNode(nodes[i]);
        }
    }

    void ComponentNodeRenderer::RenderConnections(const EntityPrefabGraphDocument* document)
    {
        if (document == nullptr) { return; }
        const std::vector<std::pair<NodeId, NodeId>>& connections = document->GetConnections();
        for (size_t i = 0; i < connections.size(); ++i)
        {
            const ComponentNode* sourceNode = document->GetNode(connections[i].first);
            const ComponentNode* targetNode = document->GetNode(connections[i].second);
            if (sourceNode != nullptr && targetNode != nullptr)
            {
                Vector from = sourceNode->position;
                from.x += sourceNode->size.x * 0.5f;
                Vector to = targetNode->position;
                to.x -= targetNode->size.x * 0.5f;
                RenderConnectionLine(from, to);
            }
        }
    }

    void ComponentNodeRenderer::SetNodeStyle(const ComponentNodeStyle& style) { m_style = style; }
    const ComponentNodeStyle& ComponentNodeRenderer::GetNodeStyle() const { return m_style; }

    void ComponentNodeRenderer::SetNormalColor(const Vector& color) { m_style.normalColor = color; }
    void ComponentNodeRenderer::SetSelectedColor(const Vector& color) { m_style.selectedColor = color; }
    void ComponentNodeRenderer::SetHoverColor(const Vector& color) { m_style.hoverColor = color; }
    void ComponentNodeRenderer::SetDisabledColor(const Vector& color) { m_style.disabledColor = color; }
    void ComponentNodeRenderer::SetTextColor(const Vector& color) { m_style.textColor = color; }
    void ComponentNodeRenderer::SetBorderWidth(float width) { m_style.borderWidth = width; }
    void ComponentNodeRenderer::SetCornerRadius(float radius) { m_style.cornerRadius = radius; }

    bool ComponentNodeRenderer::IsPointInNode(const Vector& point, const ComponentNode& node) const
    { 
        Vector min = node.position;
        min.x -= node.size.x * 0.5f;
        min.y -= node.size.y * 0.5f;
        Vector max = node.position;
        max.x += node.size.x * 0.5f;
        max.y += node.size.y * 0.5f;
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
    }

    bool ComponentNodeRenderer::GetNodeBounds(const ComponentNode& node, Vector& outMin, Vector& outMax) const
    { 
        outMin = node.position;
        outMin.x -= node.size.x * 0.5f;
        outMin.y -= node.size.y * 0.5f;
        outMax = node.position;
        outMax.x += node.size.x * 0.5f;
        outMax.y += node.size.y * 0.5f;
        return true;
    }

    void ComponentNodeRenderer::SetShowLabels(bool show) { m_showLabels = show; }
    bool ComponentNodeRenderer::GetShowLabels() const { return m_showLabels; }

    void ComponentNodeRenderer::SetShowProperties(bool show) { m_showProperties = show; }
    bool ComponentNodeRenderer::GetShowProperties() const { return m_showProperties; }

    void ComponentNodeRenderer::SetNodeScale(float scale) { m_nodeScale = (scale > 0.1f) ? scale : 0.1f; }
    float ComponentNodeRenderer::GetNodeScale() const { return m_nodeScale; }

    void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Transform node position from canvas space to screen space
        Vector screenCenter = CanvasToScreen(node.position);

        // Apply scaled node size
        float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
        float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

        Vector min = screenCenter;
        min.x -= scaledWidth;
        min.y -= scaledHeight;

        Vector max = screenCenter;
        max.x += scaledWidth;
        max.y += scaledHeight;

        Vector color = GetNodeColor(node);
        ImU32 bgColor = ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, 1.0f));
        ImU32 borderColor = ImGui::GetColorU32(ImVec4(
            (color.x * 1.3f > 1.0f) ? 1.0f : color.x * 1.3f,
            (color.y * 1.3f > 1.0f) ? 1.0f : color.y * 1.3f,
            (color.z * 1.3f > 1.0f) ? 1.0f : color.z * 1.3f,
            1.0f
        ));

        // Draw selection glow if node is selected
        if (node.selected)
        {
            ImU32 glowColor = ImGui::GetColorU32(ImVec4(0.0f, 0.8f, 1.0f, 0.3f));
            float glowSize = 4.0f * m_nodeScale * m_canvasZoom;
            drawList->AddRectFilled(
                ImVec2(min.x - glowSize, min.y - glowSize),
                ImVec2(max.x + glowSize, max.y + glowSize),
                glowColor,
                m_style.cornerRadius + glowSize
            );
        }

        drawList->AddRectFilled(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            bgColor,
            m_style.cornerRadius
        );

        float borderWidth = node.selected ? m_style.borderWidth * 2.0f : m_style.borderWidth;
        drawList->AddRect(
            ImVec2(min.x, min.y),
            ImVec2(max.x, max.y),
            borderColor,
            m_style.cornerRadius,
            ImDrawFlags_RoundCornersAll,
            borderWidth
        );

        ImU32 titleBgColor = ImGui::GetColorU32(ImVec4(
            (color.x * 0.8f > 1.0f) ? 1.0f : color.x * 0.8f,
            (color.y * 0.8f > 1.0f) ? 1.0f : color.y * 0.8f,
            (color.z * 0.8f > 1.0f) ? 1.0f : color.z * 0.8f,
            1.0f
        ));
        float titleHeight = 25.0f * m_nodeScale * m_canvasZoom;
        drawList->AddRectFilled(
            ImVec2(min.x, min.y),
            ImVec2(max.x, min.y + titleHeight),
            titleBgColor,
            m_style.cornerRadius
        );
    }

    void ComponentNodeRenderer::RenderNodeLabel(const ComponentNode& node)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Transform node position from canvas space to screen space
        Vector screenCenter = CanvasToScreen(node.position);

        // Calculate node bounds in screen space
        float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
        float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

        Vector min = screenCenter;
        min.x -= scaledWidth;
        min.y -= scaledHeight;

        Vector textColor = m_style.textColor;
        ImU32 textColorU32 = ImGui::GetColorU32(ImVec4(textColor.x, textColor.y, textColor.z, 1.0f));

        ImVec2 textPos(min.x + 8.0f * m_nodeScale * m_canvasZoom, min.y + 4.0f * m_nodeScale * m_canvasZoom);

        const char* label = node.componentType.c_str();
        drawList->AddText(textPos, textColorU32, label);

        if (m_showProperties && node.properties.size() > 0)
        {
            ImVec2 propPos(min.x + 8.0f * m_nodeScale * m_canvasZoom, min.y + 28.0f * m_nodeScale * m_canvasZoom);
            size_t propCount = 0;
            for (auto it = node.properties.begin(); it != node.properties.end() && propCount < 3; ++it, ++propCount)
            {
                std::string propText = "  " + it->first + ": " + it->second;
                drawList->AddText(propPos, textColorU32, propText.c_str());
                propPos.y += 16.0f * m_nodeScale * m_canvasZoom;
            }
            if (node.properties.size() > 3)
            {
                ImVec2 morePos(min.x + 8.0f * m_nodeScale * m_canvasZoom, propPos.y);
                drawList->AddText(morePos, textColorU32, "  ...");
            }
        }
    }

    void ComponentNodeRenderer::RenderConnectionLine(const Vector& from, const Vector& to)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Transform canvas coordinates to screen coordinates
        Vector screenFrom = CanvasToScreen(from);
        Vector screenTo = CanvasToScreen(to);

        ImVec2 p1(screenFrom.x, screenFrom.y);
        ImVec2 p2(screenTo.x, screenTo.y);

        // Calculate bezier control point offset based on horizontal distance
        // This creates smooth curves that scale with the connection distance
        float dx = p2.x - p1.x;
        float horizontalDistance = (dx > 0.0f) ? dx : -dx;

        // Use a proportional offset: 40% of horizontal distance, minimum 50 pixels
        float controlOffset = (horizontalDistance * 0.4f > 50.0f) ? horizontalDistance * 0.4f : 50.0f;

        ImU32 lineColor = ImGui::GetColorU32(ImVec4(0.7f, 0.7f, 0.7f, 0.8f));

        // Draw bezier curve with symmetric control points
        drawList->AddBezierCubic(
            p1,
            ImVec2(p1.x + controlOffset, p1.y),
            ImVec2(p2.x - controlOffset, p2.y),
            p2,
            lineColor,
            2.0f,
            20
        );

        // Draw connection endpoints
        drawList->AddCircleFilled(p1, 4.0f, lineColor);
        drawList->AddCircleFilled(p2, 4.0f, lineColor);
    }

    Vector ComponentNodeRenderer::GetNodeColor(const ComponentNode& node) const
    { 
        return node.GetCurrentColor();
    }

    void ComponentNodeRenderer::RenderNodePorts(const ComponentNode& node)
    {
        const std::vector<NodePort>& ports = node.GetPorts();
        for (const auto& port : ports)
        {
            RenderPort(node, port);
        }
    }

    void ComponentNodeRenderer::RenderPort(const ComponentNode& node, const NodePort& port)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        if (drawList == nullptr) { return; }

        // Calculate port position on node edge
        Vector screenCenter = CanvasToScreen(node.position);
        float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
        float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

        Vector portPos = node.position;

        std::vector<NodePort> inputPorts;
        std::vector<NodePort> outputPorts;
        for (const auto& p : node.GetPorts())
        {
            if (p.isOutput)
                outputPorts.push_back(p);
            else
                inputPorts.push_back(p);
        }

        uint32_t portCountInType = port.isOutput ? outputPorts.size() : inputPorts.size();
        uint32_t portIndexInType = 0;
        if (port.isOutput)
        {
            for (size_t i = 0; i < outputPorts.size(); ++i)
            {
                if (outputPorts[i].portId == port.portId)
                {
                    portIndexInType = i;
                    break;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < inputPorts.size(); ++i)
            {
                if (inputPorts[i].portId == port.portId)
                {
                    portIndexInType = i;
                    break;
                }
            }
        }

        // Distribute ports vertically on the edge
        if (portCountInType > 0)
        {
            float spacing = (2.0f * scaledHeight) / (portCountInType + 1);
            float yOffset = -scaledHeight + spacing * (portIndexInType + 1);

            if (port.isOutput)
            {
                portPos.x += scaledWidth / m_canvasZoom;
            }
            else
            {
                portPos.x -= scaledWidth / m_canvasZoom;
            }
            portPos.y += yOffset / m_canvasZoom;
        }

        Vector screenPort = CanvasToScreen(portPos);
        float portRadius = port.radius * m_canvasZoom;

        ImU32 portColor = ImGui::GetColorU32(ImVec4(0.8f, 0.8f, 0.0f, 1.0f));
        ImU32 portBorderColor = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.0f, 1.0f));

        drawList->AddCircleFilled(ImVec2(screenPort.x, screenPort.y), portRadius, portColor);
        drawList->AddCircle(ImVec2(screenPort.x, screenPort.y), portRadius, portBorderColor, 0, 1.5f);
    }

    bool ComponentNodeRenderer::IsPointInPort(const Vector& point, const ComponentNode& node, PortId& outPortId) const
    {
        const std::vector<NodePort>& ports = node.GetPorts();

        for (const auto& port : ports)
        {
            // Calculate port position on node edge
            Vector screenCenter = CanvasToScreen(node.position);
            float scaledWidth = node.size.x * 0.5f * m_nodeScale * m_canvasZoom;
            float scaledHeight = node.size.y * 0.5f * m_nodeScale * m_canvasZoom;

            std::vector<NodePort> inputPorts;
            std::vector<NodePort> outputPorts;
            for (const auto& p : node.GetPorts())
            {
                if (p.isOutput)
                    outputPorts.push_back(p);
                else
                    inputPorts.push_back(p);
            }

            uint32_t portCountInType = port.isOutput ? outputPorts.size() : inputPorts.size();
            uint32_t portIndexInType = 0;
            if (port.isOutput)
            {
                for (size_t i = 0; i < outputPorts.size(); ++i)
                {
                    if (outputPorts[i].portId == port.portId)
                    {
                        portIndexInType = i;
                        break;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < inputPorts.size(); ++i)
                {
                    if (inputPorts[i].portId == port.portId)
                    {
                        portIndexInType = i;
                        break;
                    }
                }
            }

            Vector portPos = node.position;
            if (portCountInType > 0)
            {
                float spacing = (2.0f * scaledHeight) / (portCountInType + 1);
                float yOffset = -scaledHeight + spacing * (portIndexInType + 1);

                if (port.isOutput)
                {
                    portPos.x += scaledWidth / m_canvasZoom;
                }
                else
                {
                    portPos.x -= scaledWidth / m_canvasZoom;
                }
                portPos.y += yOffset / m_canvasZoom;
            }

            float dx = point.x - portPos.x;
            float dy = point.y - portPos.y;
            float distance = sqrtf(dx * dx + dy * dy);
            float portRadius = port.radius;

            if (distance <= portRadius)
            {
                outPortId = port.portId;
                return true;
            }
        }

        outPortId = InvalidPortId;
        return false;
    }

    void ComponentNodeRenderer::UpdatePortPositions(ComponentNode& node) const
    {
        // This can be used to update port positions if needed
        // Ports positions are calculated on-the-fly during rendering
    }

} // namespace Olympe
