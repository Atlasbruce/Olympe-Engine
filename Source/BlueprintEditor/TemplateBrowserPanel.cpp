/*
 * Olympe Blueprint Editor - Template Browser Panel Implementation
 */

#include "TemplateBrowserPanel.h"
#include "BlueprintEditor.h"
#include "TemplateManager.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>
#include <cstring>

namespace Olympe
{
    TemplateBrowserPanel::TemplateBrowserPanel()
        : m_ShowPanel(true)
        , m_ShowSaveAsTemplateModal(false)
        , m_SelectedCategoryIndex(0)
    {
        m_SearchBuffer[0] = '\0';
        m_TemplateNameBuffer[0] = '\0';
        m_TemplateDescriptionBuffer[0] = '\0';
        std::strcpy(m_TemplateCategoryBuffer, "General");
    }

    TemplateBrowserPanel::~TemplateBrowserPanel()
    {
    }

    void TemplateBrowserPanel::Initialize()
    {
        // Initialize panel state
        m_ShowPanel = true;
    }

    void TemplateBrowserPanel::Shutdown()
    {
        // Cleanup
    }

    void TemplateBrowserPanel::Render()
    {
        if (m_ShowPanel)
        {
            RenderTemplateBrowser();
        }

        RenderSaveAsTemplateModal();
    }

    void TemplateBrowserPanel::RenderTemplateBrowser()
    {
        ImGui::Begin("Template Browser", &m_ShowPanel);

        auto& templateManager = TemplateManager::Get();
        if (!templateManager.IsInitialized())
        {
            ImGui::Text("Template Manager not initialized");
            ImGui::End();
            return;
        }

        // Toolbar buttons
        if (ImGui::Button("Save Current as Template"))
        {
            m_ShowSaveAsTemplateModal = true;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            BlueprintEditor::Get().ReloadTemplates();
        }

        ImGui::Separator();

        // Search bar
        ImGui::InputText("Search", m_SearchBuffer, sizeof(m_SearchBuffer));

        // Category filter
        std::vector<std::string> categories = templateManager.GetAllCategories();
        
        // Build category list with "All" at the beginning
        std::vector<const char*> categoryNames;
        categoryNames.push_back("All");
        for (const auto& cat : categories)
        {
            categoryNames.push_back(cat.c_str());
        }

        if (ImGui::Combo("Category", &m_SelectedCategoryIndex, categoryNames.data(), 
                        static_cast<int>(categoryNames.size())))
        {
            // Category changed
        }

        ImGui::Separator();

        // Template list
        auto& templates = templateManager.GetAllTemplates();
        
        if (templates.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                             "No templates available. Create one using 'Save Current as Template'");
        }
        else
        {
            // Filter templates
            std::string searchStr = m_SearchBuffer;
            std::string selectedCategory = (m_SelectedCategoryIndex > 0 && m_SelectedCategoryIndex <= static_cast<int>(categories.size())) 
                ? categories[m_SelectedCategoryIndex - 1] : "";

            for (const auto& tpl : templates)
            {
                // Apply search filter
                if (!searchStr.empty() && tpl.name.find(searchStr) == std::string::npos)
                {
                    continue;
                }

                // Apply category filter
                if (!selectedCategory.empty() && tpl.category != selectedCategory)
                {
                    continue;
                }

                ImGui::PushID(tpl.id.c_str());

                // Selectable template item
                bool isSelected = (m_SelectedTemplateId == tpl.id);
                if (ImGui::Selectable(tpl.name.c_str(), isSelected))
                {
                    m_SelectedTemplateId = tpl.id;
                }

                // Tooltip with details
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", tpl.name.c_str());
                    ImGui::Text("Description: %s", tpl.description.c_str());
                    ImGui::Text("Category: %s", tpl.category.c_str());
                    ImGui::Text("Author: %s", tpl.author.c_str());
                    ImGui::Text("Version: %s", tpl.version.c_str());
                    ImGui::EndTooltip();
                }

                // Context menu
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("Apply to Current Blueprint"))
                    {
                        if (BlueprintEditor::Get().ApplyTemplate(tpl.id))
                        {
                            std::cout << "Applied template: " << tpl.name << std::endl;
                        }
                        else
                        {
                            std::cerr << "Failed to apply template: " << tpl.name << std::endl;
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete"))
                    {
                        if (BlueprintEditor::Get().DeleteTemplate(tpl.id))
                        {
                            std::cout << "Deleted template: " << tpl.name << std::endl;
                            if (m_SelectedTemplateId == tpl.id)
                            {
                                m_SelectedTemplateId.clear();
                            }
                        }
                        else
                        {
                            std::cerr << "Failed to delete template: " << tpl.name << std::endl;
                        }
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();

        // Action buttons
        ImGui::BeginDisabled(m_SelectedTemplateId.empty());
        if (ImGui::Button("Apply Selected Template", ImVec2(200, 0)))
        {
            if (!m_SelectedTemplateId.empty())
            {
                if (BlueprintEditor::Get().ApplyTemplate(m_SelectedTemplateId))
                {
                    std::cout << "Applied template: " << m_SelectedTemplateId << std::endl;
                }
                else
                {
                    std::cerr << "Failed to apply template" << std::endl;
                }
            }
        }
        ImGui::EndDisabled();

        ImGui::End();
    }

    void TemplateBrowserPanel::RenderSaveAsTemplateModal()
    {
        if (!m_ShowSaveAsTemplateModal)
        {
            return;
        }

        ImGui::OpenPopup("Save as Template");
        
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Save as Template", &m_ShowSaveAsTemplateModal, 
                                   ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Create a new template from the current blueprint");
            ImGui::Separator();

            ImGui::InputText("Template Name", m_TemplateNameBuffer, sizeof(m_TemplateNameBuffer));
            ImGui::InputTextMultiline("Description", m_TemplateDescriptionBuffer, 
                                     sizeof(m_TemplateDescriptionBuffer), ImVec2(400, 100));
            ImGui::InputText("Category", m_TemplateCategoryBuffer, sizeof(m_TemplateCategoryBuffer));

            ImGui::Separator();

            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                if (std::strlen(m_TemplateNameBuffer) > 0)
                {
                    if (BlueprintEditor::Get().SaveCurrentAsTemplate(
                        m_TemplateNameBuffer,
                        m_TemplateDescriptionBuffer,
                        m_TemplateCategoryBuffer))
                    {
                        std::cout << "Template saved successfully: " << m_TemplateNameBuffer << std::endl;
                        
                        // Clear buffers
                        m_TemplateNameBuffer[0] = '\0';
                        m_TemplateDescriptionBuffer[0] = '\0';
                        std::strcpy(m_TemplateCategoryBuffer, "General");
                        
                        m_ShowSaveAsTemplateModal = false;
                    }
                    else
                    {
                        std::cerr << "Failed to save template: " 
                                 << BlueprintEditor::Get().GetLastError() << std::endl;
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_ShowSaveAsTemplateModal = false;
            }

            ImGui::EndPopup();
        }
    }
}
