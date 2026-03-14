/**
 * @file ParameterEditorRegistry.cpp
 * @brief Implementation of ParameterEditorRegistry with built-in node descriptors.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "ParameterEditorRegistry.h"

namespace Olympe {

// ---------------------------------------------------------------------------
// Static data
// ---------------------------------------------------------------------------

const std::vector<ParameterDescriptor> ParameterEditorRegistry::s_empty;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

ParameterEditorRegistry& ParameterEditorRegistry::Get()
{
    static ParameterEditorRegistry instance;
    return instance;
}

ParameterEditorRegistry::ParameterEditorRegistry()
{
    InitializeBuiltInParameters();
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void ParameterEditorRegistry::RegisterNodeType(
    TaskNodeType nodeType,
    const std::vector<ParameterDescriptor>& descriptors)
{
    m_params[static_cast<uint8_t>(nodeType)] = descriptors;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const std::vector<ParameterDescriptor>& ParameterEditorRegistry::GetNodeParameters(
    TaskNodeType nodeType) const
{
    auto it = m_params.find(static_cast<uint8_t>(nodeType));
    return (it != m_params.end()) ? it->second : s_empty;
}

const ParameterDescriptor* ParameterEditorRegistry::GetParameterDescriptor(
    TaskNodeType nodeType,
    const std::string& paramName) const
{
    const auto& params = GetNodeParameters(nodeType);
    for (size_t i = 0; i < params.size(); ++i)
    {
        if (params[i].name == paramName)
            return &params[i];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Built-in parameter specs
// ---------------------------------------------------------------------------

void ParameterEditorRegistry::InitializeBuiltInParameters()
{
    // ---- AtomicTask ----
    {
        ParameterDescriptor d;
        d.name        = "taskType";
        d.bindingType = ParameterBindingType::AtomicTaskID;
        d.description = "Select the atomic task to execute.";
        d.required    = true;
        RegisterNodeType(TaskNodeType::AtomicTask, {d});
    }

    // ---- Branch / While ----
    {
        std::vector<ParameterDescriptor> params;

        ParameterDescriptor cond;
        cond.name        = "conditionType";
        cond.bindingType = ParameterBindingType::ConditionID;
        cond.description = "Select the condition type to evaluate.";
        cond.required    = true;
        params.push_back(cond);

        ParameterDescriptor key;
        key.name        = "Key";
        key.bindingType = ParameterBindingType::LocalVariable;
        key.description = "Blackboard key to use in the condition.";
        key.required    = false;
        params.push_back(key);

        ParameterDescriptor op;
        op.name        = "Operator";
        op.bindingType = ParameterBindingType::ComparisonOp;
        op.description = "Comparison operator (for CompareValue condition).";
        op.required    = false;
        params.push_back(op);

        ParameterDescriptor val;
        val.name             = "Value";
        val.bindingType      = ParameterBindingType::Literal;
        val.literalValueType = VariableType::Float;
        val.description      = "Literal value to compare against.";
        val.required         = false;
        params.push_back(val);

        RegisterNodeType(TaskNodeType::Branch, params);
        RegisterNodeType(TaskNodeType::While,  params);
    }

    // ---- Switch ----
    {
        ParameterDescriptor d;
        d.name        = "switchVariable";
        d.bindingType = ParameterBindingType::LocalVariable;
        d.description = "Blackboard key whose value determines which case branch runs.";
        d.required    = true;
        RegisterNodeType(TaskNodeType::Switch, {d});
    }

    // ---- GetBBValue ----
    {
        ParameterDescriptor d;
        d.name        = "bbKey";
        d.bindingType = ParameterBindingType::LocalVariable;
        d.description = "Blackboard key to read.";
        d.required    = true;
        RegisterNodeType(TaskNodeType::GetBBValue, {d});
    }

    // ---- SetBBValue ----
    {
        ParameterDescriptor d;
        d.name        = "bbKey";
        d.bindingType = ParameterBindingType::LocalVariable;
        d.description = "Blackboard key to write.";
        d.required    = true;
        RegisterNodeType(TaskNodeType::SetBBValue, {d});
    }

    // ---- MathOp ----
    {
        ParameterDescriptor d;
        d.name        = "operation";
        d.bindingType = ParameterBindingType::MathOperator;
        d.description = "Arithmetic operator to apply (+, -, *, /, %).";
        d.required    = true;
        RegisterNodeType(TaskNodeType::MathOp, {d});
    }

    // ---- ForEach ----
    {
        ParameterDescriptor d;
        d.name        = "bbKey";
        d.bindingType = ParameterBindingType::LocalVariable;
        d.description = "Blackboard key of the list to iterate.";
        d.required    = true;
        d.filterVarType = VariableType::List;
        RegisterNodeType(TaskNodeType::ForEach, {d});
    }

    // ---- SubGraph ----
    {
        ParameterDescriptor d;
        d.name        = "subGraphPath";
        d.bindingType = ParameterBindingType::SubGraphPath;
        d.description = "Path to the sub-graph .ats file.";
        d.required    = true;
        RegisterNodeType(TaskNodeType::SubGraph, {d});
    }

    // ---- Delay ----
    {
        ParameterDescriptor d;
        d.name             = "duration";
        d.bindingType      = ParameterBindingType::Literal;
        d.literalValueType = VariableType::Float;
        d.description      = "Delay duration in seconds (must be > 0).";
        d.required         = true;
        RegisterNodeType(TaskNodeType::Delay, {d});
    }
}

} // namespace Olympe
