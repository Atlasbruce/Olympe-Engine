/*
 * Olympe Blueprint Editor - Template Browser Panel
 * 
 * GUI panel for browsing, searching, and managing blueprint templates
 */

#pragma once

#include <string>
#include <vector>

namespace Olympe
{
    /**
     * TemplateBrowserPanel - ImGui panel for template management
     * Provides interface for browsing, searching, applying, and deleting templates
     */
    class TemplateBrowserPanel
    {
    public:
        TemplateBrowserPanel();
        ~TemplateBrowserPanel();

        void Initialize();
        void Shutdown();
        void Render();

    private:
        void RenderTemplateBrowser();
        void RenderSaveAsTemplateModal();

        // UI state
        bool m_ShowPanel;
        bool m_ShowSaveAsTemplateModal;
        std::string m_SelectedTemplateId;
        char m_SearchBuffer[256];
        int m_SelectedCategoryIndex;
        
        // Save as template modal state
        char m_TemplateNameBuffer[256];
        char m_TemplateDescriptionBuffer[512];
        char m_TemplateCategoryBuffer[256];
    };
}
