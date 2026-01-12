/*
 * Olympe Blueprint Editor - Validation Panel Implementation
 */

#include "ValidationPanel.h"
#include "NodeGraphManager.h"
#include "BlueprintEditor.h"
#include "../third_party/imgui/imgui.h"
#include <iostream>
#include "BlueprintValidator.h"

namespace Olympe
{
    ValidationPanel::ValidationPanel()
    {
    }

    ValidationPanel::~ValidationPanel()
    {
    }

    void ValidationPanel::Initialize()
    {
        std::cout << "[ValidationPanel] Initialized\n";
    }

    void ValidationPanel::Shutdown()
    {
        std::cout << "[ValidationPanel] Shutdown\n";
    }

    void ValidationPanel::Render()
    {
        ImGui::Begin("Validation");

        // Auto-validate checkbox
        ImGui::Checkbox("Auto-validate", &m_AutoValidate);
        ImGui::SameLine();
        
        // Manual validate button
        if (ImGui::Button("Validate Now"))
        {
            ValidateActiveGraph();
        }

        ImGui::Separator();

        // Show summary
        RenderErrorSummary();

        ImGui::Separator();

        // Show error list
        RenderErrorList();

        ImGui::End();

        // Auto-validation
        if (m_AutoValidate)
        {
            // Validate every few seconds
            float currentTime = (float) ImGui::GetTime();
            if (currentTime - m_LastValidationTime > 2.0f)
            {
                ValidateActiveGraph();
                m_LastValidationTime = currentTime;
            }
        }
    }

    void ValidationPanel::ValidateActiveGraph()
    {
        m_Errors.clear();

        NodeGraph* graph = NodeGraphManager::Get().GetActiveGraph();
        if (!graph)
        {
            return;
        }

        m_Errors = m_Validator.ValidateGraph(graph);
        
        if (!m_Errors.empty())
        {
            std::cout << "[ValidationPanel] Found " << m_Errors.size() << " validation errors\n";
        }
    }

    bool ValidationPanel::HasErrors() const
    {
        for (const ValidationError& error : m_Errors)
        {
            if (error.severity == ErrorSeverity::Error || error.severity == ErrorSeverity::Critical)
                return true;
        }
        return false;
    }

    bool ValidationPanel::HasCriticalErrors() const
    {
        for (const ValidationError& error : m_Errors)
        {
            if (error.severity == ErrorSeverity::Critical)
                return true;
        }
        return false;
    }

    void ValidationPanel::RenderErrorSummary()
    {
        int criticalCount = m_Validator.GetErrorCount(m_Errors, ErrorSeverity::Critical);
        int errorCount = m_Validator.GetErrorCount(m_Errors, ErrorSeverity::Error);
        int warningCount = m_Validator.GetErrorCount(m_Errors, ErrorSeverity::Warning);
        int infoCount = m_Validator.GetErrorCount(m_Errors, ErrorSeverity::Info);

        if (m_Errors.empty())
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ No validation errors");
            return;
        }

        ImGui::Text("Validation Summary:");
        
        if (criticalCount > 0)
        {
            ImGui::TextColored(BlueprintValidator::SeverityToColor(ErrorSeverity::Critical),
                "  Critical: %d", criticalCount);
        }
        
        if (errorCount > 0)
        {
            ImGui::TextColored(BlueprintValidator::SeverityToColor(ErrorSeverity::Error),
                "  Errors: %d", errorCount);
        }
        
        if (warningCount > 0)
        {
            ImGui::TextColored(BlueprintValidator::SeverityToColor(ErrorSeverity::Warning),
                "  Warnings: %d", warningCount);
        }
        
        if (infoCount > 0)
        {
            ImGui::TextColored(BlueprintValidator::SeverityToColor(ErrorSeverity::Info),
                "  Info: %d", infoCount);
        }
    }

    void ValidationPanel::RenderErrorList()
    {
        if (m_Errors.empty())
        {
            return;
        }

        ImGui::BeginChild("ErrorList", ImVec2(0, 0), true);

        for (size_t i = 0; i < m_Errors.size(); ++i)
        {
            const ValidationError& error = m_Errors[i];
            
            ImVec4 color = BlueprintValidator::SeverityToColor(error.severity);
            const char* severityStr = BlueprintValidator::SeverityToString(error.severity);
            
            // Severity badge
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("[%s]", severityStr);
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            
            // Error message (clickable)
            bool isSelected = (m_SelectedErrorIndex == (int)i);
            
            std::string label = error.message;
            if (!error.nodeName.empty())
            {
                label = error.nodeName + ": " + label;
            }
            
            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                m_SelectedErrorIndex = (int)i;
                OnErrorClicked(error);
            }
            
            // Tooltip with more details
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                if (error.nodeId >= 0)
                {
                    ImGui::Text("Node ID: %d", error.nodeId);
                }
                if (!error.category.empty())
                {
                    ImGui::Text("Category: %s", error.category.c_str());
                }
                ImGui::Text("Severity: %s", severityStr);
                ImGui::Separator();
                ImGui::TextWrapped("%s", error.message.c_str());
                ImGui::EndTooltip();
            }
        }

        ImGui::EndChild();
    }

    void ValidationPanel::OnErrorClicked(const ValidationError& error)
    {
        if (error.nodeId >= 0)
        {
            // TODO: Focus on the node in the graph editor
            // This would require adding a method to NodeGraphPanel to focus on a specific node
            std::cout << "[ValidationPanel] Clicked on error for node " << error.nodeId << std::endl;
        }
    }
}
