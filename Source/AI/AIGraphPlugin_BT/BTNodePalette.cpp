/**
 * @file BTNodePalette.cpp
 * @brief Implementation of BTNodePalette
 * @author Olympe Engine
 * @date 2026-02-18
 */

#include "BTNodePalette.h"
#include "BTNodeRegistry.h"
#include "../../third_party/imgui/imgui.h"
#include <cstring>

namespace Olympe {
namespace AI {

BTNodePalette::BTNodePalette()
    : m_isDragging(false)
{
    std::memset(m_searchFilter, 0, sizeof(m_searchFilter));
}

void BTNodePalette::Render(bool* isOpen) {
    if (!ImGui::Begin("BT Node Palette", isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }
    
    // Search filter
    ImGui::InputText("Search", m_searchFilter, sizeof(m_searchFilter));
    ImGui::Separator();
    
    // Render by category
    RenderCategory("Composites", BTNodeCategory::Composite);
    RenderCategory("Decorators", BTNodeCategory::Decorator);
    RenderCategory("Conditions", BTNodeCategory::Condition);
    RenderCategory("Actions", BTNodeCategory::Action);
    
    ImGui::End();
}

void BTNodePalette::RenderCategory(const std::string& categoryName, BTNodeCategory category) {
    if (ImGui::CollapsingHeader(categoryName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& registry = BTNodeRegistry::Get();
        auto nodeTypes = registry.GetNodeTypesByCategory(category);
        
        for (auto typeIt = nodeTypes.begin(); typeIt != nodeTypes.end(); ++typeIt) {
            const std::string& typeName = *typeIt;
            
            // Apply search filter
            if (m_searchFilter[0] != '\0') {
                const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(typeName);
                if (typeInfo != nullptr) {
                    // Check if search term is in display name
                    std::string displayNameLower = typeInfo->displayName;
                    std::string searchLower = m_searchFilter;
                    
                    // Simple case-insensitive search (ASCII only)
                    for (size_t i = 0; i < displayNameLower.length(); ++i) {
                        if (displayNameLower[i] >= 'A' && displayNameLower[i] <= 'Z') {
                            displayNameLower[i] = displayNameLower[i] + ('a' - 'A');
                        }
                    }
                    for (size_t i = 0; i < searchLower.length(); ++i) {
                        if (searchLower[i] >= 'A' && searchLower[i] <= 'Z') {
                            searchLower[i] = searchLower[i] + ('a' - 'A');
                        }
                    }
                    
                    if (displayNameLower.find(searchLower) == std::string::npos) {
                        continue; // Skip this node
                    }
                }
            }
            
            RenderNodeButton(typeName);
        }
    }
}

void BTNodePalette::RenderNodeButton(const std::string& typeName) {
    auto& registry = BTNodeRegistry::Get();
    const BTNodeTypeInfo* typeInfo = registry.GetNodeTypeInfo(typeName);
    
    if (typeInfo == nullptr) {
        return;
    }
    
    ImGui::PushID(typeName.c_str());
    
    // Convert color from RGBA to ImVec4
    uint32_t color = typeInfo->color;
    float r = static_cast<float>((color >> 0) & 0xFF) / 255.0f;
    float g = static_cast<float>((color >> 8) & 0xFF) / 255.0f;
    float b = static_cast<float>((color >> 16) & 0xFF) / 255.0f;
    float a = static_cast<float>((color >> 24) & 0xFF) / 255.0f;
    
    ImVec4 buttonColor(r, g, b, a);
    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
    
    // Create button label with icon and display name
    std::string label = typeInfo->icon + " " + typeInfo->displayName;
    
    if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 0.0f))) {
        // Start drag operation
        m_draggedNodeType = typeName;
        m_isDragging = true;
    }
    
    // Tooltip on hover
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", typeInfo->description.c_str());
        
        // Show parameters if any
        if (!typeInfo->parameterNames.empty()) {
            ImGui::Separator();
            ImGui::Text("Parameters:");
            for (auto paramIt = typeInfo->parameterNames.begin(); 
                 paramIt != typeInfo->parameterNames.end(); 
                 ++paramIt) {
                ImGui::BulletText("%s", paramIt->c_str());
            }
        }
        
        // Show child constraints
        if (typeInfo->minChildren >= 0 || typeInfo->maxChildren >= 0) {
            ImGui::Separator();
            if (typeInfo->minChildren >= 0 && typeInfo->maxChildren >= 0) {
                if (typeInfo->minChildren == typeInfo->maxChildren) {
                    ImGui::Text("Children: exactly %d", typeInfo->minChildren);
                } else {
                    ImGui::Text("Children: %d to %d", typeInfo->minChildren, typeInfo->maxChildren);
                }
            } else if (typeInfo->minChildren >= 0) {
                ImGui::Text("Children: at least %d", typeInfo->minChildren);
            } else {
                ImGui::Text("Children: at most %d", typeInfo->maxChildren);
            }
        }
        
        ImGui::EndTooltip();
    }
    
    ImGui::PopStyleColor();
    ImGui::PopID();
}

} // namespace AI
} // namespace Olympe
