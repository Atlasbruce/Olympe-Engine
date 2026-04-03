#include "ComponentNodeData.h"
#include <algorithm>

namespace OlympeEngine {

// ============================================================================
// Constructor
// ============================================================================

ComponentNodeData::ComponentNodeData() = default;

ComponentNodeData::ComponentNodeData(const std::string& componentType)
    : m_componentType(componentType) {
}

// ============================================================================
// Property Management
// ============================================================================

void ComponentNodeData::SetProperty(const std::string& key, const std::string& value) {
    m_properties[key] = value;
}

std::string ComponentNodeData::GetProperty(const std::string& key) const {
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }
    return "";
}

bool ComponentNodeData::HasProperty(const std::string& key) const {
    return m_properties.find(key) != m_properties.end();
}

// ============================================================================
// Pin Management
// ============================================================================

void ComponentNodeData::AddInputPin(const std::string& name, const std::string& type) {
    ComponentNodePin pin;
    pin.name = name;
    pin.type = type;
    m_inputPins.push_back(pin);
}

void ComponentNodeData::AddOutputPin(const std::string& name, const std::string& type) {
    ComponentNodePin pin;
    pin.name = name;
    pin.type = type;
    m_outputPins.push_back(pin);
}

// ============================================================================
// Rendering
// ============================================================================

glm::vec4 ComponentNodeData::GetCurrentColor() const {
    switch (m_state) {
        case ComponentNodeState::Normal:
            return m_style.normalColor;
        case ComponentNodeState::Hover:
            return m_style.hoverColor;
        case ComponentNodeState::Selected:
            return m_style.selectedColor;
        case ComponentNodeState::Error:
            return m_style.errorColor;
        case ComponentNodeState::Warning:
            return m_style.warningColor;
        case ComponentNodeState::Disabled:
            return m_style.disabledColor;
        default:
            return m_style.normalColor;
    }
}

}  // namespace OlympeEngine
