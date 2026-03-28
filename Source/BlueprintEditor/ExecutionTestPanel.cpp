/**
 * @file ExecutionTestPanel.cpp
 * @brief Implementation of execution test panel.
 * @author Olympe Engine
 * @date 2026-03-24
 */

#include "ExecutionTestPanel.h"
#include "../third_party/imgui/imgui.h"
#include "../system/system_utils.h"
#include <sstream>
#include <iomanip>

namespace Olympe {

ExecutionTestPanel::ExecutionTestPanel()
    : m_visible(true), m_lastTestRun(false), m_selectedEventIndex(-1),
      m_showTraceLog(true), m_showErrors(true), m_autoScroll(true)
{
}

ExecutionTestPanel::~ExecutionTestPanel()
{
}

void ExecutionTestPanel::Initialize()
{
    m_testOptions.maxStepsPerFrame = 1000;
    m_testOptions.maxSubGraphDepth = 10;
    m_testOptions.validateConditions = true;
    m_testOptions.validateDataFlow = true;
    m_testOptions.validateBranchPaths = true;

    SYSTEM_LOG << "[ExecutionTestPanel] Initialized\n";
}

void ExecutionTestPanel::Shutdown()
{
    SYSTEM_LOG << "[ExecutionTestPanel] Shutdown\n";
}

void ExecutionTestPanel::Render()
{
    if (!m_visible)
        return;

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Execution Test", &m_visible, ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }

    // Test controls
    RenderTestControls();

    ImGui::Separator();

    // Tabs for different views
    if (ImGui::BeginTabBar("ExecutionTestTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Trace Log"))
        {
            RenderTraceLog();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Errors"))
        {
            RenderErrors();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Summary"))
        {
            RenderSummary();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

std::vector<ValidationError> ExecutionTestPanel::RunExecutionTest(const TaskGraphTemplate& tmpl)
{
    m_lastTracer.Reset();
    m_lastTestErrors.clear();

    // Run simulation
    m_lastTestErrors = m_simulator.SimulateExecution(tmpl, m_testOptions, m_lastTracer);

    m_lastTestRun = true;

    // Log results
    SYSTEM_LOG << "[ExecutionTest] Test completed: " << m_lastTestErrors.size() << " issues found\n";
    SYSTEM_LOG << m_lastTracer.GetExecutionSummary() << "\n";

    return m_lastTestErrors;
}

std::string ExecutionTestPanel::GetExecutionLog() const
{
    return m_lastTracer.GetTraceLog();
}

std::string ExecutionTestPanel::GetTestSummary() const
{
    return m_lastTracer.GetExecutionSummary();
}

void ExecutionTestPanel::RenderTestControls()
{
    ImGui::Text("Execution Test Controls");

    if (ImGui::Button("Test Execution", ImVec2(120, 0)))
    {
        // TODO: Get the active template and run test
        SYSTEM_LOG << "[ExecutionTestPanel] Test execution button clicked\n";
    }

    ImGui::SameLine();
    ImGui::Text(m_lastTestRun ? "✓ Last test run" : "● No test run yet");

    ImGui::Separator();
    ImGui::Text("Test Options");

    ImGui::Checkbox("Validate Conditions##opt1", &m_testOptions.validateConditions);
    ImGui::Checkbox("Validate Data Flow##opt2", &m_testOptions.validateDataFlow);
    ImGui::Checkbox("Validate Branch Paths##opt3", &m_testOptions.validateBranchPaths);

    ImGui::SliderInt("Max Steps", &m_testOptions.maxStepsPerFrame, 100, 10000);
    ImGui::SliderInt("Max Depth", &m_testOptions.maxSubGraphDepth, 1, 20);

    ImGui::Checkbox("Auto-scroll", &m_autoScroll);
}

void ExecutionTestPanel::RenderTraceLog()
{
    ImGui::Text("Execution Trace (%zu events)", m_lastTracer.GetEvents().size());

    ImGui::Separator();

    // Trace log table
    if (ImGui::BeginTable("TraceTable", 6, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Node ID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Node Name", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 80.0f);

        ImGui::TableHeadersRow();

        const auto& events = m_lastTracer.GetEvents();
        size_t eventCount = events.size();

        ImGuiListClipper clipper;
        clipper.Begin((int)eventCount);

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                RenderExecutionEventRow(events[i], i);
            }
        }
        clipper.End();

        ImGui::EndTable();
    }
}

void ExecutionTestPanel::RenderExecutionEventRow(const ExecutionEvent& event, size_t rowIndex)
{
    ImGui::TableNextRow();

    // Step number
    ImGui::TableSetColumnIndex(0);
    ImGui::Text("%d", event.stepNumber);

    // Event type
    ImGui::TableSetColumnIndex(1);
    const char* typeStr = "Unknown";
    ImVec4 typeColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

    switch (event.type)
    {
        case ExecutionEventType::NodeEntered:
            typeStr = "Enter";
            typeColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
            break;
        case ExecutionEventType::NodeExited:
            typeStr = "Exit";
            typeColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
            break;
        case ExecutionEventType::ConditionEvaluated:
            typeStr = "Cond";
            typeColor = ImVec4(1.0f, 0.8f, 0.3f, 1.0f);
            break;
        case ExecutionEventType::BranchTaken:
            typeStr = "Branch";
            typeColor = ImVec4(0.9f, 0.6f, 0.2f, 1.0f);
            break;
        case ExecutionEventType::ErrorOccurred:
            typeStr = "ERROR";
            typeColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            break;
        case ExecutionEventType::ExecutionBlocked:
            typeStr = "Blocked";
            typeColor = ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
            break;
        case ExecutionEventType::DataPinResolved:
            typeStr = "Data";
            typeColor = ImVec4(0.7f, 0.7f, 0.9f, 1.0f);
            break;
        case ExecutionEventType::ExecutionCompleted:
            typeStr = "Done";
            typeColor = ImVec4(0.4f, 0.9f, 0.4f, 1.0f);
            break;
        default:
            break;
    }

    ImGui::TextColored(typeColor, "%s", typeStr);

    // Node ID
    ImGui::TableSetColumnIndex(2);
    if (event.nodeId >= 0)
        ImGui::Text("%d", event.nodeId);
    else
        ImGui::Text("-");

    // Node name
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%s", event.nodeName.c_str());

    // Message
    ImGui::TableSetColumnIndex(4);
    ImGui::Text("%s", event.message.c_str());

    // Condition result (if applicable)
    ImGui::TableSetColumnIndex(5);
    if (event.type == ExecutionEventType::ConditionEvaluated)
    {
        if (event.conditionResult)
        {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "TRUE");
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "FALSE");
        }
    }
}

void ExecutionTestPanel::RenderErrors()
{
    ImGui::Text("Validation Errors (%zu found)", m_lastTestErrors.size());

    ImGui::Separator();

    if (m_lastTestErrors.empty())
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "✓ No errors found!");
    }
    else
    {
        if (ImGui::BeginTable("ErrorTable", 5, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
        {
            ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Node ID", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Node Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_lastTestErrors.size(); ++i)
            {
                const ValidationError& error = m_lastTestErrors[i];

                ImGui::TableNextRow();

                // Severity
                ImGui::TableSetColumnIndex(0);
                const char* sevStr = BlueprintValidator::SeverityToString(error.severity);
                ImVec4 sevColor = BlueprintValidator::SeverityToColor(error.severity);
                ImGui::TextColored(sevColor, "%s", sevStr);

                // Node ID
                ImGui::TableSetColumnIndex(1);
                if (error.nodeId >= 0)
                    ImGui::Text("%d", error.nodeId);
                else
                    ImGui::Text("Graph");

                // Node name
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", error.nodeName.c_str());

                // Category
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", error.category.c_str());

                // Message
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", error.message.c_str());
            }

            ImGui::EndTable();
        }
    }
}

void ExecutionTestPanel::RenderSummary()
{
    ImGui::TextWrapped(m_lastTracer.GetExecutionSummary().c_str());

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Full Trace Log", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped(m_lastTracer.GetTraceLog().c_str());
    }
}

} // namespace Olympe
