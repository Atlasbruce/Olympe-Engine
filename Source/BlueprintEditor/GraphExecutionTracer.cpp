/**
 * @file GraphExecutionTracer.cpp
 * @brief Implementation of graph execution tracing.
 * @author Olympe Engine
 * @date 2026-03-24
 */

#include "GraphExecutionTracer.h"
#include "../system/system_utils.h"
#include <sstream>
#include <iomanip>

namespace Olympe {

GraphExecutionTracer::GraphExecutionTracer()
    : m_stepCount(0), m_executionComplete(false), m_executionSuccess(false)
{
}

GraphExecutionTracer::~GraphExecutionTracer()
{
}

void GraphExecutionTracer::Reset()
{
    m_events.clear();
    m_errorNodes.clear();
    m_stepCount = 0;
    m_executionComplete = false;
    m_executionSuccess = false;
}

void GraphExecutionTracer::RecordNodeEntered(int32_t nodeId, const std::string& nodeName, const std::string& nodeType)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::NodeEntered;
    event.nodeId = nodeId;
    event.nodeName = nodeName;
    event.nodeType = nodeType;
    event.stepNumber = m_stepCount++;
    event.message = "Entering node";
    AddEvent(event);
}

void GraphExecutionTracer::RecordNodeExited(int32_t nodeId, int32_t nextNodeId)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::NodeExited;
    event.nodeId = nodeId;
    event.stepNumber = m_stepCount++;
    if (nextNodeId >= 0)
    {
        event.message = "Exiting node, next: " + std::to_string(nextNodeId);
    }
    else
    {
        event.message = "Exiting node";
    }
    AddEvent(event);
}

void GraphExecutionTracer::RecordConditionEvaluated(int32_t nodeId, const std::string& expression,
                                                    bool result, const std::string& message)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::ConditionEvaluated;
    event.nodeId = nodeId;
    event.conditionExpression = expression;
    event.conditionResult = result;
    event.stepNumber = m_stepCount++;
    event.message = "Condition '" + expression + "' = " + (result ? "TRUE" : "FALSE");
    if (!message.empty())
    {
        event.message += " (" + message + ")";
    }
    AddEvent(event);
}

void GraphExecutionTracer::RecordBranchTaken(int32_t nodeId, const std::string& branchName, int32_t nextNodeId)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::BranchTaken;
    event.nodeId = nodeId;
    event.stepNumber = m_stepCount++;
    event.message = "Branch taken: " + branchName + " -> Node " + std::to_string(nextNodeId);
    AddEvent(event);
}

void GraphExecutionTracer::RecordError(int32_t nodeId, const std::string& nodeName,
                                       const std::string& errorMessage, const std::string& severity)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::ErrorOccurred;
    event.nodeId = nodeId;
    event.nodeName = nodeName;
    event.stepNumber = m_stepCount++;
    event.message = "[" + severity + "] " + errorMessage;
    AddEvent(event);

    if (nodeId >= 0 && std::find(m_errorNodes.begin(), m_errorNodes.end(), nodeId) == m_errorNodes.end())
    {
        m_errorNodes.push_back(nodeId);
    }
}

void GraphExecutionTracer::RecordExecutionBlocked(int32_t nodeId, const std::string& reason)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::ExecutionBlocked;
    event.nodeId = nodeId;
    event.stepNumber = m_stepCount++;
    event.message = "Execution blocked: " + reason;
    AddEvent(event);

    if (nodeId >= 0 && std::find(m_errorNodes.begin(), m_errorNodes.end(), nodeId) == m_errorNodes.end())
    {
        m_errorNodes.push_back(nodeId);
    }
}

void GraphExecutionTracer::RecordDataPinResolved(int32_t nodeId, const std::string& pinName, const std::string& value)
{
    ExecutionEvent event;
    event.type = ExecutionEventType::DataPinResolved;
    event.nodeId = nodeId;
    event.stepNumber = m_stepCount++;
    event.message = "Data pin '" + pinName + "' = " + value;
    AddEvent(event);
}

void GraphExecutionTracer::RecordExecutionCompleted(bool success, const std::string& message)
{
    m_executionComplete = true;
    m_executionSuccess = success;

    ExecutionEvent event;
    event.type = ExecutionEventType::ExecutionCompleted;
    event.nodeId = -1;
    event.stepNumber = m_stepCount++;
    event.message = message;
    AddEvent(event);
}

std::string GraphExecutionTracer::GetTraceLog() const
{
    std::ostringstream oss;

    oss << "=== EXECUTION TRACE ===\n";
    oss << "Total Steps: " << m_stepCount << "\n";
    oss << "Errors: " << m_errorNodes.size() << "\n";
    oss << "Status: " << (m_executionComplete ? (m_executionSuccess ? "SUCCESS" : "FAILED") : "INCOMPLETE") << "\n";
    oss << "\n--- Event Log ---\n";

    for (size_t i = 0; i < m_events.size(); ++i)
    {
        const ExecutionEvent& event = m_events[i];
        oss << std::setw(3) << (i + 1) << ". ";

        switch (event.type)
        {
            case ExecutionEventType::NodeEntered:
                oss << "[ENTER]   ";
                break;
            case ExecutionEventType::NodeExited:
                oss << "[EXIT]    ";
                break;
            case ExecutionEventType::ConditionEvaluated:
                oss << "[COND]    ";
                break;
            case ExecutionEventType::BranchTaken:
                oss << "[BRANCH]  ";
                break;
            case ExecutionEventType::ErrorOccurred:
                oss << "[ERROR]   ";
                break;
            case ExecutionEventType::ExecutionBlocked:
                oss << "[BLOCKED] ";
                break;
            case ExecutionEventType::DataPinResolved:
                oss << "[DATA]    ";
                break;
            case ExecutionEventType::ExecutionCompleted:
                oss << "[DONE]    ";
                break;
            default:
                oss << "[???]     ";
                break;
        }

        if (event.nodeId >= 0)
        {
            oss << "Node " << std::setw(3) << event.nodeId;
            if (!event.nodeName.empty())
                oss << " (" << event.nodeName << ")";
            oss << ": ";
        }

        oss << event.message << "\n";
    }

    return oss.str();
}

std::string GraphExecutionTracer::GetExecutionSummary() const
{
    std::ostringstream oss;

    oss << "Execution Summary:\n";
    oss << "  Steps executed: " << m_stepCount << "\n";
    oss << "  Status: " << (m_executionComplete ? (m_executionSuccess ? "✓ SUCCESS" : "✗ FAILED") : "⊘ INCOMPLETE") << "\n";
    oss << "  Errors found: " << m_errorNodes.size() << "\n";

    if (!m_errorNodes.empty())
    {
        oss << "  Error nodes: ";
        for (size_t i = 0; i < m_errorNodes.size(); ++i)
        {
            if (i > 0) oss << ", ";
            oss << m_errorNodes[i];
        }
        oss << "\n";
    }

    // Count event types
    int enteredCount = 0, conditionCount = 0, errorCount = 0;
    for (const auto& event : m_events)
    {
        if (event.type == ExecutionEventType::NodeEntered) ++enteredCount;
        else if (event.type == ExecutionEventType::ConditionEvaluated) ++conditionCount;
        else if (event.type == ExecutionEventType::ErrorOccurred) ++errorCount;
    }

    oss << "  Nodes visited: " << enteredCount << "\n";
    oss << "  Conditions evaluated: " << conditionCount << "\n";
    oss << "  Errors logged: " << errorCount << "\n";

    return oss.str();
}

void GraphExecutionTracer::AddEvent(const ExecutionEvent& event)
{
    m_events.push_back(event);
}

} // namespace Olympe
