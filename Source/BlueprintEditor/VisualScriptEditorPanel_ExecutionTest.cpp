/**
 * @file VisualScriptEditorPanel_ExecutionTest.cpp
 * @brief Execution testing implementation for VisualScriptEditorPanel.
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * This file contains the implementation of execution testing and simulation
 * functionality for the VisualScriptEditorPanel. It is included/linked as
 * part of the main editor panel compilation unit.
 *
 * Phase 24.3 — Execution simulation integration.
 * C++14 compliant.
 */

#include "VisualScriptEditorPanel.h"
#include "../system/system_utils.h"

namespace Olympe {

std::vector<ValidationError> VisualScriptEditorPanel::RunExecutionTest()
{
    SYSTEM_LOG << "[VSEditor] Running execution test on graph '" << m_template.Name << "'\n";

    // Sync current canvas state to template before testing
    SyncTemplateFromCanvas();

    // Run the execution test
    std::vector<ValidationError> errors = m_executionTestPanel.RunExecutionTest(m_template);

    // Log results
    int errorCount = 0, warningCount = 0;
    for (const auto& error : errors)
    {
        if (error.severity == ErrorSeverity::Error || error.severity == ErrorSeverity::Critical)
            ++errorCount;
        else if (error.severity == ErrorSeverity::Warning)
            ++warningCount;
    }

    SYSTEM_LOG << "[VSEditor] Execution test complete: "
               << errorCount << " errors, " << warningCount << " warnings\n";

    return errors;
}

} // namespace Olympe
