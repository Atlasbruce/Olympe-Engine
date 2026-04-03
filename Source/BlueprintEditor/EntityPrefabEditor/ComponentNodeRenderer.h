#pragma once

#include "ComponentNodeData.h"
#include "PrefabLoader.h"
#include <glm/glm.hpp>
#include <memory>

namespace OlympeEngine {

// ============================================================================
// ComponentNodeRenderer - Rendering logic for component nodes
// ============================================================================

class ComponentNodeRenderer {
public:
    ComponentNodeRenderer();
    ~ComponentNodeRenderer();

    // Rendering
    void RenderNode(
        const ComponentNodeData& nodeData,
        const glm::vec2& canvasOffset = glm::vec2(0.0f),
        float zoomLevel = 1.0f
    );

    void RenderNodeLabel(
        const std::string& label,
        const glm::vec2& position,
        const glm::vec4& color
    );

    void RenderNodePins(
        const ComponentNodeData& nodeData,
        const glm::vec2& nodePosition,
        bool hovered = false
    );

    // ImGui-specific rendering (used by PrefabCanvas)
    void RenderNodeImGui(
        const ComponentNodeData& nodeData,
        bool selected = false,
        bool hovered = false
    );

    // Style Management
    void SetNodeStyle(const std::string& componentType, const ComponentNodeStyle& style);
    ComponentNodeStyle GetNodeStyle(const std::string& componentType) const;
    void ResetNodeStyleToDefaults();

    // Color & Visual State
    glm::vec4 GetColorForState(ComponentNodeState state) const;
    glm::vec4 GetColorForComponentType(const std::string& componentType) const;

    // Bounds & Geometry
    glm::vec4 GetNodeBounds(const glm::vec2& position, const glm::vec2& size) const;
    glm::vec2 GetInputPinPosition(
        const glm::vec2& nodePosition,
        int pinIndex,
        int totalInputPins
    ) const;
    glm::vec2 GetOutputPinPosition(
        const glm::vec2& nodePosition,
        const glm::vec2& nodeSize,
        int pinIndex,
        int totalOutputPins
    ) const;

    // Icon & Asset Management
    void SetComponentIcon(const std::string& componentType, const std::string& iconPath);
    std::string GetComponentIcon(const std::string& componentType) const;

private:
    std::map<std::string, ComponentNodeStyle> m_nodeStyles;
    std::map<std::string, std::string> m_componentIcons;

    // Default colors
    struct DefaultColors {
        glm::vec4 normal = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
        glm::vec4 hover = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        glm::vec4 selected = glm::vec4(0.2f, 0.8f, 1.0f, 1.0f);
        glm::vec4 error = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
        glm::vec4 warning = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
        glm::vec4 disabled = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        glm::vec4 text = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    } m_colors;

    // Helper methods
    void DrawRoundedRectangle(
        const glm::vec2& position,
        const glm::vec2& size,
        float radius,
        const glm::vec4& color,
        float borderWidth
    );
    void DrawText(
        const std::string& text,
        const glm::vec2& position,
        const glm::vec4& color,
        float fontSize
    );
};

}  // namespace OlympeEngine
