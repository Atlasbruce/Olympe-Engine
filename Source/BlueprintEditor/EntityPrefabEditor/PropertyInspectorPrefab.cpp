#include "PropertyInspectorPrefab.h"
#include <algorithm>

namespace OlympeEngine {

// ============================================================================
// Constructor & Destructor
// ============================================================================

PropertyInspectorPrefab::PropertyInspectorPrefab()
    : m_hasSelection(false),
      m_isExpanded(true),
      m_readOnly(true) {
}

PropertyInspectorPrefab::~PropertyInspectorPrefab() {
}

// ============================================================================
// Rendering
// ============================================================================

void PropertyInspectorPrefab::Render(float width, float height) {
    if (!m_hasSelection) {
        // Display "No selection" message
        return;
    }

    // Render component header
    RenderComponentName();
    RenderComponentType();

    // Render properties
    RenderComponentProperties();

    // Render buttons (in Phase 3+)
}

void PropertyInspectorPrefab::RenderComponentName() {
    // Placeholder for component name rendering
}

void PropertyInspectorPrefab::RenderComponentType() {
    // Placeholder for component type rendering
}

void PropertyInspectorPrefab::RenderComponentProperties() {
    // Placeholder for properties rendering
}

void PropertyInspectorPrefab::RenderEntityCenterProperties() {
    // Placeholder for entity center properties
}

// ============================================================================
// Selection Management
// ============================================================================

void PropertyInspectorPrefab::SetSelectedNode(const ComponentNodeData& nodeData) {
    m_hasSelection = true;
    m_selectedComponentType = nodeData.GetComponentType();
    m_selectedComponentName = nodeData.GetComponentName();
    m_properties = nodeData.GetAllProperties();
    m_originalProperties = m_properties;
}

void PropertyInspectorPrefab::SetSelectedNode(
    const std::string& componentType,
    const std::string& componentName
) {
    m_hasSelection = true;
    m_selectedComponentType = componentType;
    m_selectedComponentName = componentName;
    m_properties.clear();
}

void PropertyInspectorPrefab::ClearSelection() {
    m_hasSelection = false;
    m_selectedComponentType.clear();
    m_selectedComponentName.clear();
    m_properties.clear();
    m_originalProperties.clear();
}

// ============================================================================
// Property Management
// ============================================================================

void PropertyInspectorPrefab::SetProperty(const std::string& propertyName, const std::string& value) {
    m_properties[propertyName] = value;
}

std::string PropertyInspectorPrefab::GetProperty(const std::string& propertyName) const {
    auto it = m_properties.find(propertyName);
    if (it != m_properties.end()) {
        return it->second;
    }
    return "";
}

// ============================================================================
// Callbacks
// ============================================================================

void PropertyInspectorPrefab::SetOnPropertyChangedCallback(OnPropertyChangedCallback callback) {
    m_onPropertyChanged = callback;
}

// ============================================================================
// Component Description Display
// ============================================================================

void PropertyInspectorPrefab::DisplayComponentDescription(const std::string& componentType) {
    // Placeholder for displaying component description
}

void PropertyInspectorPrefab::DisplayComponentDocumentation() {
    // Placeholder for displaying component documentation
}

// ============================================================================
// Property Type-Specific Rendering
// ============================================================================

void PropertyInspectorPrefab::RenderProperty(
    const std::string& propertyName,
    const std::string& propertyType,
    const std::string& currentValue
) {
    if (propertyType == "int") {
        RenderPropertyInt(propertyName, currentValue);
    } else if (propertyType == "float") {
        RenderPropertyFloat(propertyName, currentValue);
    } else if (propertyType == "bool") {
        RenderPropertyBool(propertyName, currentValue);
    } else if (propertyType == "string") {
        RenderPropertyString(propertyName, currentValue);
    } else if (propertyType == "Vector3") {
        RenderPropertyVector3(propertyName, currentValue);
    }
}

void PropertyInspectorPrefab::RenderPropertyInt(
    const std::string& label,
    const std::string& currentValue
) {
    // Placeholder for int property rendering
}

void PropertyInspectorPrefab::RenderPropertyFloat(
    const std::string& label,
    const std::string& currentValue
) {
    // Placeholder for float property rendering
}

void PropertyInspectorPrefab::RenderPropertyBool(
    const std::string& label,
    const std::string& currentValue
) {
    // Placeholder for bool property rendering
}

void PropertyInspectorPrefab::RenderPropertyString(
    const std::string& label,
    const std::string& currentValue
) {
    // Placeholder for string property rendering
}

void PropertyInspectorPrefab::RenderPropertyVector3(
    const std::string& label,
    const std::string& currentValue
) {
    // Placeholder for Vector3 property rendering
}

void PropertyInspectorPrefab::RenderPropertyEnum(
    const std::string& label,
    const std::string& currentValue,
    const std::vector<std::string>& enumValues
) {
    // Placeholder for enum property rendering
}

// ============================================================================
// Reset & Apply
// ============================================================================

void PropertyInspectorPrefab::ApplyChanges() {
    if (m_onPropertyChanged) {
        // Trigger callbacks for each changed property
        for (const auto& [key, value] : m_properties) {
            auto it = m_originalProperties.find(key);
            if (it == m_originalProperties.end() || it->second != value) {
                std::string oldValue = (it != m_originalProperties.end()) ? it->second : "";
                m_onPropertyChanged(key, oldValue, value);
            }
        }
    }
    m_originalProperties = m_properties;
}

void PropertyInspectorPrefab::DiscardChanges() {
    m_properties = m_originalProperties;
}

void PropertyInspectorPrefab::ResetToDefaults() {
    // Placeholder for resetting to defaults
    // Will query ComponentLibrary for default values
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void PropertyInspectorPrefab::RenderPropertyLabel(
    const std::string& label,
    const std::string& description
) {
    // Placeholder for label rendering
}

void PropertyInspectorPrefab::RenderPropertyInput(
    const std::string& propertyName,
    const std::string& propertyType,
    std::string& value
) {
    if (propertyType == "int") {
        InputIntProperty(propertyName.c_str(), value);
    } else if (propertyType == "float") {
        InputFloatProperty(propertyName.c_str(), value);
    } else if (propertyType == "bool") {
        InputBoolProperty(propertyName.c_str(), value);
    } else if (propertyType == "string") {
        InputTextProperty(propertyName.c_str(), value);
    } else if (propertyType == "Vector3") {
        InputVector3Property(propertyName.c_str(), value);
    }
}

bool PropertyInspectorPrefab::ValidatePropertyValue(
    const std::string& propertyType,
    const std::string& value
) const {
    if (propertyType == "int") {
        try {
            std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    } else if (propertyType == "float") {
        try {
            std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    } else if (propertyType == "bool") {
        return value == "true" || value == "false" || value == "0" || value == "1";
    }

    return true;
}

std::string PropertyInspectorPrefab::FormatPropertyValue(
    const std::string& propertyType,
    const std::string& value
) const {
    // Placeholder for value formatting
    return value;
}

// ============================================================================
// ImGui Input Wrappers
// ============================================================================

bool PropertyInspectorPrefab::InputIntProperty(const char* label, std::string& value) {
    // Placeholder for int input
    return false;
}

bool PropertyInspectorPrefab::InputFloatProperty(const char* label, std::string& value) {
    // Placeholder for float input
    return false;
}

bool PropertyInspectorPrefab::InputTextProperty(const char* label, std::string& value) {
    // Placeholder for text input
    return false;
}

bool PropertyInspectorPrefab::InputBoolProperty(const char* label, std::string& value) {
    // Placeholder for bool input
    return false;
}

bool PropertyInspectorPrefab::InputVector3Property(const char* label, std::string& value) {
    // Placeholder for Vector3 input
    return false;
}

}  // namespace OlympeEngine
