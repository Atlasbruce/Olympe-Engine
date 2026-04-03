#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

namespace OlympeEngine {

// ============================================================================
// ComponentNodeState - Visual state of a component node in the editor
// ============================================================================

enum class ComponentNodeState {
    Normal,
    Hover,
    Selected,
    Error,
    Warning,
    Disabled
};

struct ComponentNodeStyle {
    glm::vec4 normalColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    glm::vec4 hoverColor = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
    glm::vec4 selectedColor = glm::vec4(0.2f, 0.8f, 1.0f, 1.0f);
    glm::vec4 errorColor = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);
    glm::vec4 warningColor = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
    glm::vec4 disabledColor = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);

    float nodeWidth = 150.0f;
    float nodeHeight = 80.0f;
    float cornerRadius = 5.0f;
    float borderWidth = 2.0f;
};

// ============================================================================
// ComponentNodePin - Input/Output pin for connections
// ============================================================================

struct ComponentNodePin {
    std::string name;
    std::string type;  // input, output, bidirectional
    bool connected = false;
    glm::vec2 position;
};

// ============================================================================
// ComponentNodeData - Complete component node representation
// ============================================================================

class ComponentNodeData {
public:
    ComponentNodeData();
    explicit ComponentNodeData(const std::string& componentType);

    // Basic Properties
    std::string GetComponentType() const { return m_componentType; }
    std::string GetComponentName() const { return m_componentName; }
    void SetComponentName(const std::string& name) { m_componentName = name; }

    // Position & Bounds
    glm::vec2 GetPosition() const { return m_position; }
    void SetPosition(const glm::vec2& pos) { m_position = pos; }
    glm::vec2 GetSize() const { return m_size; }
    void SetSize(const glm::vec2& size) { m_size = size; }

    // State Management
    ComponentNodeState GetState() const { return m_state; }
    void SetState(ComponentNodeState state) { m_state = state; }
    bool IsSelected() const { return m_state == ComponentNodeState::Selected; }
    bool IsHovered() const { return m_state == ComponentNodeState::Hover; }
    bool HasError() const { return m_state == ComponentNodeState::Error; }

    // Properties
    void SetProperty(const std::string& key, const std::string& value);
    std::string GetProperty(const std::string& key) const;
    bool HasProperty(const std::string& key) const;
    const std::map<std::string, std::string>& GetAllProperties() const { return m_properties; }

    // Pins
    void AddInputPin(const std::string& name, const std::string& type);
    void AddOutputPin(const std::string& name, const std::string& type);
    const std::vector<ComponentNodePin>& GetInputPins() const { return m_inputPins; }
    const std::vector<ComponentNodePin>& GetOutputPins() const { return m_outputPins; }
    int GetInputPinCount() const { return m_inputPins.size(); }
    int GetOutputPinCount() const { return m_outputPins.size(); }

    // Enabled/Disabled
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    // Style
    ComponentNodeStyle GetStyle() const { return m_style; }
    void SetStyle(const ComponentNodeStyle& style) { m_style = style; }
    glm::vec4 GetCurrentColor() const;

    // Validation
    bool IsValid() const { return m_isValid; }
    void SetValid(bool valid) { m_isValid = valid; }
    std::string GetErrorMessage() const { return m_errorMessage; }
    void SetErrorMessage(const std::string& msg) { m_errorMessage = msg; }

private:
    std::string m_componentType;
    std::string m_componentName;
    glm::vec2 m_position = glm::vec2(0.0f);
    glm::vec2 m_size = glm::vec2(150.0f, 80.0f);
    ComponentNodeState m_state = ComponentNodeState::Normal;
    std::map<std::string, std::string> m_properties;
    std::vector<ComponentNodePin> m_inputPins;
    std::vector<ComponentNodePin> m_outputPins;
    bool m_enabled = true;
    bool m_isValid = true;
    std::string m_errorMessage;
    ComponentNodeStyle m_style;
};

}  // namespace OlympeEngine
