/*
 * Olympe Blueprint Editor - UI Helpers
 * 
 * Helper functions for creating ImGui widgets with enum catalogs
 */

#pragma once

#include "EnumCatalogManager.h"
#include "NodeValidator.h"
#include <string>

// Forward declarations for ImGui types
struct ImVec4;

namespace Olympe
{
    namespace UIHelpers
    {
        /**
         * Create a combo box for selecting an Action type
         * @param label Label for the combo box
         * @param currentActionType Current action type (will be updated if changed)
         * @return true if the value was changed
         */
        bool ActionTypeCombo(const char* label, std::string& currentActionType);

        /**
         * Create a combo box for selecting a Condition type
         * @param label Label for the combo box
         * @param currentConditionType Current condition type (will be updated if changed)
         * @return true if the value was changed
         */
        bool ConditionTypeCombo(const char* label, std::string& currentConditionType);

        /**
         * Create a combo box for selecting a Decorator type
         * @param label Label for the combo box
         * @param currentDecoratorType Current decorator type (will be updated if changed)
         * @return true if the value was changed
         */
        bool DecoratorTypeCombo(const char* label, std::string& currentDecoratorType);

        /**
         * Display validation errors and warnings
         * @param result Validation result to display
         */
        void ShowValidationErrors(const ValidationResult& result);

        /**
         * Render parameters for a node type
         * @param typeInfo Type information from catalog
         * @param nodeJson Node JSON to edit
         * @return true if any parameter was changed
         */
        bool RenderNodeParameters(const EnumTypeInfo* typeInfo, nlohmann::json& nodeJson);

        /**
         * Show a notification message
         * @param message Message to display
         */
        void ShowNotification(const char* message);

        /**
         * Show an error modal dialog
         * @param message Error message to display
         */
        void ShowErrorModal(const char* message);

    } // namespace UIHelpers
} // namespace Olympe
