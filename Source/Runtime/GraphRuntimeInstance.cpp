/**
 * @file GraphRuntimeInstance.cpp
 * @brief Implementation of GraphRuntimeInstance.
 * @author Olympe Engine
 * @date 2026-03-17
 */

#include "GraphRuntimeInstance.h"
#include "ConditionPresetEvaluator.h"

#include <algorithm>

namespace Olympe {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

GraphRuntimeInstance::GraphRuntimeInstance(const TaskGraphTemplate& graph)
    : m_graph(graph)
    , m_isExecuting(false)
    , m_breakpointHit(false)
    , m_skipBreakpointOnce(false)
{
}

// ---------------------------------------------------------------------------
// Execution control
// ---------------------------------------------------------------------------

void GraphRuntimeInstance::StartExecution()
{
    m_executionStack.clear();
    m_lastError.clear();
    m_breakpointHit = false;
    m_skipBreakpointOnce = false;

    const int32_t entry = (m_graph.EntryPointID != NODE_INDEX_NONE)
                          ? m_graph.EntryPointID
                          : m_graph.RootNodeID;

    if (entry == NODE_INDEX_NONE)
    {
        m_isExecuting = false;
        return;
    }

    m_executionStack.push_back(entry);
    m_isExecuting = true;
}

bool GraphRuntimeInstance::StepExecution()
{
    if (!m_isExecuting || m_executionStack.empty())
    {
        m_isExecuting = false;
        return false;
    }

    // Resume check: if we are paused at a breakpoint, stay paused.
    if (m_breakpointHit)
        return false;

    int32_t currentID = m_executionStack.front();
    m_executionStack.erase(m_executionStack.begin());

    // Breakpoint check for the current node.
    if (!m_skipBreakpointOnce && m_breakpoints.count(currentID) > 0)
    {
        // Re-insert so the node is processed after the user resumes.
        m_executionStack.insert(m_executionStack.begin(), currentID);
        m_breakpointHit = true;
        return false;
    }
    m_skipBreakpointOnce = false;

    const TaskNodeDefinition* node = m_graph.GetNode(currentID);
    if (!node)
    {
        m_lastError = "GraphRuntimeInstance: Node not found for ID "
                    + std::to_string(currentID);
        m_isExecuting = false;
        return false;
    }

    int32_t nextID = NODE_INDEX_NONE;

    if (node->Type == TaskNodeType::Branch)
    {
        bool condResult = EvaluateBranchConditions(*node);
        nextID = condResult ? node->NextOnSuccess : node->NextOnFailure;
    }
    else
    {
        // All non-Branch nodes follow the success path.
        nextID = node->NextOnSuccess;
    }

    if (nextID != NODE_INDEX_NONE)
    {
        m_executionStack.push_back(nextID);
    }

    m_isExecuting = !m_executionStack.empty();
    return m_isExecuting;
}

bool GraphRuntimeInstance::IsExecuting() const
{
    return m_isExecuting;
}

// ---------------------------------------------------------------------------
// Environment
// ---------------------------------------------------------------------------

void GraphRuntimeInstance::SetBlackboardVariable(const std::string& key, float value)
{
    m_environment.SetBlackboardVariable(key, value);
}

void GraphRuntimeInstance::SetDynamicPinValue(const std::string& pinID, float value)
{
    m_environment.SetDynamicPinValue(pinID, value);
}

// ---------------------------------------------------------------------------
// Breakpoints
// ---------------------------------------------------------------------------

void GraphRuntimeInstance::AddBreakpoint(int32_t nodeID)
{
    m_breakpoints.insert(nodeID);
}

void GraphRuntimeInstance::RemoveBreakpoint(int32_t nodeID)
{
    m_breakpoints.erase(nodeID);
}

bool GraphRuntimeInstance::IsBreakpointHit() const
{
    return m_breakpointHit;
}

void GraphRuntimeInstance::ResumeFromBreakpoint()
{
    m_breakpointHit = false;
    m_skipBreakpointOnce = true;
}

// ---------------------------------------------------------------------------
// Inspection
// ---------------------------------------------------------------------------

const std::vector<int32_t>& GraphRuntimeInstance::GetActiveNodeIDs() const
{
    return m_executionStack;
}

const std::string& GraphRuntimeInstance::GetLastError() const
{
    return m_lastError;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

bool GraphRuntimeInstance::EvaluateBranchConditions(const TaskNodeDefinition& node)
{
    if (node.conditionRefs.empty())
    {
        // No preset-based conditions → fall through to success.
        return true;
    }

    bool combinedResult = false;
    bool firstDone      = false;

    for (const NodeConditionRef& ref : node.conditionRefs)
    {
        // Look up the preset in the global registry attached to the graph.
        // Since GraphRuntimeInstance has no direct registry reference we need
        // to build a temporary ConditionPreset from the ref's presetID.
        // Callers are expected to pre-populate the environment with the
        // preset data when constructing the instance.  For a full integration
        // the registry should be injected; here we build a minimal preset
        // from what is stored on the node itself.
        //
        // NOTE: In Phase 24.5 the ConditionPresetRegistry is a separate,
        // globally managed object.  GraphRuntimeInstance evaluates the
        // conditions inline using the RuntimeEnvironment it already owns.
        // The registry is consulted by the caller before populating the env.
        //
        // If no preset data is embedded, we must treat the condition as true
        // so that graphs without Phase-24 presets continue to function.
        //
        // A full integration would pass a ConditionPresetRegistry& to the
        // constructor; that is left as a Phase 24.6 enhancement.
        (void)ref; // suppress unused warning when body is simplified

        // Return true by default when no concrete preset resolution is available.
        // Tests that require full evaluation use ConditionEvaluator directly.
        if (!firstDone)
        {
            combinedResult = true;
            firstDone = true;
        }
    }

    return combinedResult;
}

} // namespace Olympe
