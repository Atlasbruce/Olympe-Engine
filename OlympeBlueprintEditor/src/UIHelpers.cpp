/*
 * Olympe Blueprint Editor - UI Helpers Implementation
 * 
 * Note: This file provides the implementation but requires ImGui to be available.
 * If ImGui is not yet integrated, these functions will be stubs.
 */

#include "../include/UIHelpers.h"
#include "../../Source/json_helper.h"

// Check if ImGui is available
// If the stub version is being used, we'll provide minimal implementations
#ifdef IMGUI_VERSION
    #include "../third_party/imgui/imgui.h"
    #define IMGUI_AVAILABLE 1
#else
    // Stub implementations when ImGui is not available
    #define IMGUI_AVAILABLE 0
    
    // Minimal ImGui stubs for compilation
    namespace ImGui
    {
        struct ImVec4 { float x, y, z, w; };
        inline bool Combo(const char*, int*, const char* const*, int) { return false; }
        inline bool IsItemHovered() { return false; }
        inline void SetTooltip(const char*, ...) {}
        inline void PushStyleColor(int, ImVec4) {}
        inline void PopStyleColor(int = 1) {}
        inline void TextWrapped(const char*, ...) {}
        inline void BulletText(const char*, ...) {}
        inline void Text(const char*, ...) {}
        inline bool InputFloat(const char*, float*) { return false; }
        inline bool InputText(const char*, char*, size_t) { return false; }
        inline bool InputInt(const char*, int*) { return false; }
        inline bool Checkbox(const char*, bool*) { return false; }
        inline void SameLine() {}
        inline void TextColored(ImVec4, const char*, ...) {}
        inline void Separator() {}
        inline bool BeginPopupModal(const char*, bool* = nullptr, int = 0) { return false; }
        inline void EndPopup() {}
        inline void OpenPopup(const char*) {}
        inline bool Button(const char*) { return false; }
        inline void CloseCurrentPopup() {}
        
        enum ImGuiCol_ { ImGuiCol_Text };
    }
#endif

#include <cstring>
#include <iostream>

namespace Olympe
{
    namespace UIHelpers
    {
        bool ActionTypeCombo(const char* label, std::string& currentActionType)
        {
#if IMGUI_AVAILABLE
            auto& catalog = EnumCatalogManager::Instance();
            const auto& types = catalog.GetActionTypes();
            auto names = catalog.GetActionTypeNames();

            if (names.empty())
            {
                ImGui::Text("%s: No actions available", label);
                return false;
            }

            // Find current index
            int currentIdx = -1;
            for (size_t i = 0; i < types.size(); ++i)
            {
                if (types[i].id == currentActionType)
                {
                    currentIdx = static_cast<int>(i);
                    break;
                }
            }

            // If not found, default to first item
            if (currentIdx < 0)
            {
                currentIdx = 0;
            }

            if (ImGui::Combo(label, &currentIdx, names.data(), static_cast<int>(names.size())))
            {
                currentActionType = types[currentIdx].id;
                return true;
            }

            // Show tooltip
            if (ImGui::IsItemHovered() && currentIdx >= 0 && currentIdx < static_cast<int>(types.size()))
            {
                const auto& info = types[currentIdx];
                ImGui::SetTooltip("%s", info.tooltip.c_str());
            }

            return false;
#else
            // Stub implementation
            (void)label;
            (void)currentActionType;
            return false;
#endif
        }

        bool ConditionTypeCombo(const char* label, std::string& currentConditionType)
        {
#if IMGUI_AVAILABLE
            auto& catalog = EnumCatalogManager::Instance();
            const auto& types = catalog.GetConditionTypes();
            auto names = catalog.GetConditionTypeNames();

            if (names.empty())
            {
                ImGui::Text("%s: No conditions available", label);
                return false;
            }

            // Find current index
            int currentIdx = -1;
            for (size_t i = 0; i < types.size(); ++i)
            {
                if (types[i].id == currentConditionType)
                {
                    currentIdx = static_cast<int>(i);
                    break;
                }
            }

            // If not found, default to first item
            if (currentIdx < 0)
            {
                currentIdx = 0;
            }

            if (ImGui::Combo(label, &currentIdx, names.data(), static_cast<int>(names.size())))
            {
                currentConditionType = types[currentIdx].id;
                return true;
            }

            // Show tooltip
            if (ImGui::IsItemHovered() && currentIdx >= 0 && currentIdx < static_cast<int>(types.size()))
            {
                const auto& info = types[currentIdx];
                ImGui::SetTooltip("%s", info.tooltip.c_str());
            }

            return false;
#else
            // Stub implementation
            (void)label;
            (void)currentConditionType;
            return false;
#endif
        }

        bool DecoratorTypeCombo(const char* label, std::string& currentDecoratorType)
        {
#if IMGUI_AVAILABLE
            auto& catalog = EnumCatalogManager::Instance();
            const auto& types = catalog.GetDecoratorTypes();
            auto names = catalog.GetDecoratorTypeNames();

            if (names.empty())
            {
                ImGui::Text("%s: No decorators available", label);
                return false;
            }

            // Find current index
            int currentIdx = -1;
            for (size_t i = 0; i < types.size(); ++i)
            {
                if (types[i].id == currentDecoratorType)
                {
                    currentIdx = static_cast<int>(i);
                    break;
                }
            }

            // If not found, default to first item
            if (currentIdx < 0)
            {
                currentIdx = 0;
            }

            if (ImGui::Combo(label, &currentIdx, names.data(), static_cast<int>(names.size())))
            {
                currentDecoratorType = types[currentIdx].id;
                return true;
            }

            // Show tooltip
            if (ImGui::IsItemHovered() && currentIdx >= 0 && currentIdx < static_cast<int>(types.size()))
            {
                const auto& info = types[currentIdx];
                ImGui::SetTooltip("%s", info.tooltip.c_str());
            }

            return false;
#else
            // Stub implementation
            (void)label;
            (void)currentDecoratorType;
            return false;
#endif
        }

        void ShowValidationErrors(const ValidationResult& result)
        {
#if IMGUI_AVAILABLE
            if (!result.isValid)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                ImGui::TextWrapped("❌ Erreurs de validation :");
                for (size_t i = 0; i < result.errors.size(); ++i)
                {
                    ImGui::BulletText("%s", result.errors[i].c_str());
                }
                ImGui::PopStyleColor();
            }

            if (!result.warnings.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::TextWrapped("⚠️ Avertissements :");
                for (size_t i = 0; i < result.warnings.size(); ++i)
                {
                    ImGui::BulletText("%s", result.warnings[i].c_str());
                }
                ImGui::PopStyleColor();
            }
#else
            // Console output for non-GUI mode
            if (!result.isValid)
            {
                std::cerr << "Validation Errors:" << std::endl;
                for (const auto& error : result.errors)
                {
                    std::cerr << "  - " << error << std::endl;
                }
            }

            if (!result.warnings.empty())
            {
                std::cout << "Validation Warnings:" << std::endl;
                for (const auto& warning : result.warnings)
                {
                    std::cout << "  - " << warning << std::endl;
                }
            }
#endif
        }

        bool RenderNodeParameters(const EnumTypeInfo* typeInfo, nlohmann::json& nodeJson)
        {
#if IMGUI_AVAILABLE
            if (!typeInfo)
            {
                return false;
            }

            bool changed = false;

            ImGui::Text("Paramètres pour %s:", typeInfo->name.c_str());
            ImGui::Separator();

            // Ensure parameters object exists
            if (!nodeJson.contains("parameters"))
            {
                nodeJson["parameters"] = nlohmann::json::object();
            }

            auto& params = nodeJson["parameters"];

            for (size_t i = 0; i < typeInfo->parameters.size(); ++i)
            {
                const auto& paramDef = typeInfo->parameters[i];

                if (!paramDef.contains("name") || !paramDef.contains("type"))
                {
                    continue;
                }

                std::string paramName = paramDef["name"].get<std::string>();
                std::string paramType = paramDef["type"].get<std::string>();
                bool required = JsonHelper::GetBool(paramDef, "required", false);

                // Generate UI based on type
                if (paramType == "float")
                {
                    float defaultValue = JsonHelper::GetFloat(paramDef, "default", 0.0f);
                    float value = JsonHelper::GetFloat(params, paramName, defaultValue);
                    
                    if (ImGui::InputFloat(paramName.c_str(), &value))
                    {
                        params[paramName] = static_cast<double>(value);
                        changed = true;
                    }
                }
                else if (paramType == "int")
                {
                    int defaultValue = JsonHelper::GetInt(paramDef, "default", 0);
                    int value = JsonHelper::GetInt(params, paramName, defaultValue);
                    
                    if (ImGui::InputInt(paramName.c_str(), &value))
                    {
                        params[paramName] = value;
                        changed = true;
                    }
                }
                else if (paramType == "string")
                {
                    std::string defaultValue = JsonHelper::GetString(paramDef, "default", "");
                    std::string value = JsonHelper::GetString(params, paramName, defaultValue);
                    
                    char buffer[256];
                    std::strncpy(buffer, value.c_str(), 255);
                    buffer[255] = '\0';
                    
                    if (ImGui::InputText(paramName.c_str(), buffer, 256))
                    {
                        params[paramName] = std::string(buffer);
                        changed = true;
                    }
                }
                else if (paramType == "bool")
                {
                    bool defaultValue = JsonHelper::GetBool(paramDef, "default", false);
                    bool value = JsonHelper::GetBool(params, paramName, defaultValue);
                    
                    if (ImGui::Checkbox(paramName.c_str(), &value))
                    {
                        params[paramName] = value;
                        changed = true;
                    }
                }

                // Mark required fields
                if (required)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImGui::ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "*");
                }
            }

            return changed;
#else
            (void)typeInfo;
            (void)nodeJson;
            return false;
#endif
        }

        void ShowNotification(const char* message)
        {
#if IMGUI_AVAILABLE
            // In a real implementation, this would show a toast notification
            ImGui::Text("✓ %s", message);
#else
            std::cout << "Notification: " << message << std::endl;
#endif
        }

        void ShowErrorModal(const char* message)
        {
#if IMGUI_AVAILABLE
            ImGui::OpenPopup("Error");
            
            if (ImGui::BeginPopupModal("Error", nullptr))
            {
                ImGui::Text("%s", message);
                ImGui::Separator();
                
                if (ImGui::Button("OK"))
                {
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }
#else
            std::cerr << "ERROR: " << message << std::endl;
#endif
        }

    } // namespace UIHelpers
} // namespace Olympe
