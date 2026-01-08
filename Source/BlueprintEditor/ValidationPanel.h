/*
 * Olympe Blueprint Editor - Validation Panel
 * 
 * GUI panel for displaying validation errors and warnings
 */

#pragma once

#include <string>
#include <vector>
#include "BlueprintValidator.h"

namespace Olympe
{
    /**
     * ValidationPanel - ImGui panel for validation error display
     * Shows errors, warnings, and info messages from blueprint validation
     */
    class ValidationPanel
    {
    public:
        ValidationPanel();
        ~ValidationPanel();

        void Initialize();
        void Shutdown();
        void Render();
        
        // Trigger validation of active graph
        void ValidateActiveGraph();
        
        // Get current validation errors
        const std::vector<ValidationError>& GetErrors() const { return m_Errors; }
        
        // Check if there are any errors
        bool HasErrors() const;
        bool HasCriticalErrors() const;

    private:
        void RenderErrorList();
        void RenderErrorSummary();
        
        // Click handler to focus on node with error
        void OnErrorClicked(const ValidationError& error);

    private:
        BlueprintValidator m_Validator;
        std::vector<ValidationError> m_Errors;
        bool m_AutoValidate = true;
        float m_LastValidationTime = 0.0f;
        int m_SelectedErrorIndex = -1;
    };
}
