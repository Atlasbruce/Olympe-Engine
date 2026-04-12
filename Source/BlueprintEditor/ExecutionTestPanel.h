/**
 * @file ExecutionTestPanel.h
 * @brief Panel for executing and testing blueprint graphs with simulation.
 * @author Olympe Engine
 * @date 2026-03-24
 *
 * @details
 * ExecutionTestPanel provides the UI for running execution simulation on
 * blueprints, displaying trace logs, and checking for logic errors.
 * Integrates with GraphExecutionSimulator and GraphExecutionTracer.
 *
 * Phase 24.3 — Blueprint validation with execution simulation.
 * C++14 compliant.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "../TaskSystem/TaskGraphTemplate.h"
#include "GraphExecutionSimulator.h"
#include "GraphExecutionTracer.h"
#include "BlueprintValidator.h"

namespace Olympe {

/**
 * @class ExecutionTestPanel
 * @brief ImGui panel for blueprint execution testing and validation.
 */
class ExecutionTestPanel {
public:

    ExecutionTestPanel();
    ~ExecutionTestPanel();

    void Initialize();
    void Shutdown();

    /**
     * @brief Renders the test execution panel window.
     */
    void Render();

    /**
     * @brief Runs execution test on the specified template.
     * @param tmpl  The TaskGraphTemplate to test.
     * @return      Vector of validation errors found during test.
     */
    std::vector<ValidationError> RunExecutionTest(const TaskGraphTemplate& tmpl);

    /**
     * @brief Display a pre-computed trace (for BehaviorTree or native executors).
     * @param tracer  The execution trace to display
     * @details Used by BehaviorTreeRenderer to pass trace from BehaviorTreeExecutor
     */
    void DisplayTrace(const GraphExecutionTracer& tracer);

    /**
     * @brief Returns true if there are test results to display.
     */
    bool HasResults() const { return !m_lastTestErrors.empty() || m_lastTestRun; }

    /**
     * @brief Returns the last execution trace.
     */
    const GraphExecutionTracer& GetLastTrace() const { return m_lastTracer; }

    /** @brief Show / hide the panel. */
    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const  { return m_visible; }

    /**
     * @brief Returns the execution log as a formatted string.
     */
    std::string GetExecutionLog() const;

    /**
     * @brief Returns a summary of the last test run.
     */
    std::string GetTestSummary() const;

private:

    void RenderTestControls();
    void RenderTraceLog();
    void RenderErrors();
    void RenderSummary();

    void RenderExecutionEventRow(const ExecutionEvent& event, size_t rowIndex);

    bool m_visible = true;
    bool m_lastTestRun = false;

    GraphExecutionSimulator m_simulator;
    GraphExecutionTracer m_lastTracer;
    std::vector<ValidationError> m_lastTestErrors;
    SimulationOptions m_testOptions;

    int32_t m_selectedEventIndex = -1;
    bool m_showTraceLog = true;
    bool m_showErrors = true;
    bool m_autoScroll = true;
};

} // namespace Olympe
