#include "ComponentNodeRenderer.h"
#include <algorithm>
#include <cmath>

namespace OlympeEngine {

// ============================================================================
// Constructor & Destructor
// ============================================================================

ComponentNodeRenderer::ComponentNodeRenderer() {
    // Initialize default styles for common component types
}

ComponentNodeRenderer::~ComponentNodeRenderer() {
}

// ============================================================================
// Rendering
// ============================================================================

void ComponentNodeRenderer::RenderNode(
    const ComponentNodeData& nodeData,
    const glm::vec2& canvasOffset,
    float zoomLevel
) {
    // Placeholder for actual rendering implementation
    // This will use ImGui draw API to render nodes
}

void ComponentNodeRenderer::RenderNodeLabel(
    const std::string& label,
    const glm::vec2& position,
    const glm::vec4& color
) {
    // Placeholder for text rendering
}

void ComponentNodeRenderer::RenderNodePins(
    const ComponentNodeData& nodeData,
    const glm::vec2& nodePosition,
    bool hovered
) {
    // Placeholder for pin rendering
}

void ComponentNodeRenderer::RenderNodeImGui(
    const ComponentNodeData& nodeData,
    bool selected,
    bool hovered
) {
    // Placeholder for ImGui-based node rendering
}

// ============================================================================
// Style Management
// ============================================================================

void ComponentNodeRenderer::SetNodeStyle(
    const std::string& componentType,
    const ComponentNodeStyle& style
) {
    m_nodeStyles[componentType] = style;
}

ComponentNodeStyle ComponentNodeRenderer::GetNodeStyle(const std::string& componentType) const {
    auto it = m_nodeStyles.find(componentType);
    if (it != m_nodeStyles.end()) {
        return it->second;
    }

    // Return default style
    ComponentNodeStyle defaultStyle;
    defaultStyle.normalColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    return defaultStyle;
}

void ComponentNodeRenderer::ResetNodeStyleToDefaults() {
    m_nodeStyles.clear();
}

// ============================================================================
// Color & Visual State
// ============================================================================

glm::vec4 ComponentNodeRenderer::GetColorForState(ComponentNodeState state) const {
    switch (state) {
        case ComponentNodeState::Normal:
            return m_colors.normal;
        case ComponentNodeState::Hover:
            return m_colors.hover;
        case ComponentNodeState::Selected:
            return m_colors.selected;
        case ComponentNodeState::Error:
            return m_colors.error;
        case ComponentNodeState::Warning:
            return m_colors.warning;
        case ComponentNodeState::Disabled:
            return m_colors.disabled;
        default:
            return m_colors.normal;
    }
}

glm::vec4 ComponentNodeRenderer::GetColorForComponentType(const std::string& componentType) const {
    // Map component types to colors
    // This is a simple example - could be expanded

    if (componentType == "Transform") {
        return glm::vec4(0.2f, 0.8f, 0.2f, 1.0f);  // Green
    } else if (componentType == "Physics") {
        return glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // Red
    } else if (componentType == "Mesh") {
        return glm::vec4(0.2f, 0.2f, 0.8f, 1.0f);  // Blue
    } else if (componentType == "Entity") {
        return glm::vec4(0.8f, 0.8f, 0.2f, 1.0f);  // Yellow
    }

    // Default color
    return glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
}

// ============================================================================
// Bounds & Geometry
// ============================================================================

glm::vec4 ComponentNodeRenderer::GetNodeBounds(const glm::vec2& position, const glm::vec2& size) const {
    return glm::vec4(position.x, position.y, position.x + size.x, position.y + size.y);
}

glm::vec2 ComponentNodeRenderer::GetInputPinPosition(
    const glm::vec2& nodePosition,
    int pinIndex,
    int totalInputPins
) const {
    if (totalInputPins == 0) return nodePosition;

    float spacing = 60.0f / static_cast<float>(totalInputPins + 1);
    float yOffset = spacing * (pinIndex + 1);

    return glm::vec2(nodePosition.x, nodePosition.y + yOffset);
}

glm::vec2 ComponentNodeRenderer::GetOutputPinPosition(
    const glm::vec2& nodePosition,
    const glm::vec2& nodeSize,
    int pinIndex,
    int totalOutputPins
) const {
    if (totalOutputPins == 0) return nodePosition + nodeSize;

    float spacing = 60.0f / static_cast<float>(totalOutputPins + 1);
    float yOffset = spacing * (pinIndex + 1);

    return glm::vec2(nodePosition.x + nodeSize.x, nodePosition.y + yOffset);
}

// ============================================================================
// Icon & Asset Management
// ============================================================================

void ComponentNodeRenderer::SetComponentIcon(
    const std::string& componentType,
    const std::string& iconPath
) {
    m_componentIcons[componentType] = iconPath;
}

std::string ComponentNodeRenderer::GetComponentIcon(const std::string& componentType) const {
    auto it = m_componentIcons.find(componentType);
    if (it != m_componentIcons.end()) {
        return it->second;
    }
    return "";
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void ComponentNodeRenderer::DrawRoundedRectangle(
    const glm::vec2& position,
    const glm::vec2& size,
    float radius,
    const glm::vec4& color,
    float borderWidth
) {
    // Placeholder for rounded rectangle drawing
    // Will use ImGui drawing API
}

void ComponentNodeRenderer::DrawText(
    const std::string& text,
    const glm::vec2& position,
    const glm::vec4& color,
    float fontSize
) {
    // Placeholder for text drawing
    // Will use ImGui text rendering
}

}  // namespace OlympeEngine
