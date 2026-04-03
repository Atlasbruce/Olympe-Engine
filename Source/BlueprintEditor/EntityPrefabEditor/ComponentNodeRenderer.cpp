#include "ComponentNodeRenderer.h"
#include "../../Source/third_party/imgui/imgui.h"

namespace Olympe
{
    ComponentNodeRenderer::ComponentNodeRenderer() : m_showLabels(true), m_showProperties(true), m_nodeScale(1.0f) { }
    ComponentNodeRenderer::~ComponentNodeRenderer() { }

    void ComponentNodeRenderer::Initialize() { }
    void ComponentNodeRenderer::Shutdown() { }

    void ComponentNodeRenderer::RenderNode(const ComponentNode& node) { (void)node; }
    void ComponentNodeRenderer::RenderNodes(const EntityPrefabGraphDocument* document) { (void)document; }
    void ComponentNodeRenderer::RenderConnections(const EntityPrefabGraphDocument* document) { (void)document; }

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
    { Vector min = node.position; min.x -= node.size.x * 0.5f; min.y -= node.size.y * 0.5f; Vector max = node.position; max.x += node.size.x * 0.5f; max.y += node.size.y * 0.5f; return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y; }

    bool ComponentNodeRenderer::GetNodeBounds(const ComponentNode& node, Vector& outMin, Vector& outMax) const
    { outMin = node.position; outMin.x -= node.size.x * 0.5f; outMin.y -= node.size.y * 0.5f; outMax = node.position; outMax.x += node.size.x * 0.5f; outMax.y += node.size.y * 0.5f; return true; }

    void ComponentNodeRenderer::SetShowLabels(bool show) { m_showLabels = show; }
    bool ComponentNodeRenderer::GetShowLabels() const { return m_showLabels; }

    void ComponentNodeRenderer::SetShowProperties(bool show) { m_showProperties = show; }
    bool ComponentNodeRenderer::GetShowProperties() const { return m_showProperties; }

    void ComponentNodeRenderer::SetNodeScale(float scale) { m_nodeScale = scale; }
    float ComponentNodeRenderer::GetNodeScale() const { return m_nodeScale; }

    void ComponentNodeRenderer::RenderNodeBox(const ComponentNode& node) { (void)node; }
    void ComponentNodeRenderer::RenderNodeLabel(const ComponentNode& node) { (void)node; }
    void ComponentNodeRenderer::RenderConnectionLine(const Vector& from, const Vector& to) { (void)from; (void)to; }
    Vector ComponentNodeRenderer::GetNodeColor(const ComponentNode& node) const { return node.GetCurrentColor(); }

} // namespace Olympe
