/*
Olympe Engine V2 2025
Animation System - Animation Graph Implementation
*/

#include "AnimationGraph.h"
#include "../json_helper.h"
#include "../system/system_utils.h"
#include <algorithm>

using json = nlohmann::json;

namespace OlympeAnimation
{
    // ========================================================================
    // AnimationGraph Implementation
    // ========================================================================

    bool AnimationGraph::LoadFromFile(const std::string& filePath)
    {
        json j;
        if (!JsonHelper::LoadJsonFromFile(filePath, j))
        {
            SYSTEM_LOG << "AnimationGraph: Failed to load file: " << filePath << "\n";
            return false;
        }

        return ParseJSON(j.dump());
    }

    bool AnimationGraph::ParseJSON(const std::string& jsonContent)
    {
        try
        {
            json j = json::parse(jsonContent);

            // Parse basic info
            m_graphName = JsonHelper::GetString(j, "graphName", "unknown");
            m_description = JsonHelper::GetString(j, "description", "");
            m_animationBankPath = JsonHelper::GetString(j, "animationBankPath", "");
            m_defaultState = JsonHelper::GetString(j, "defaultState", "Idle");
            m_currentState = m_defaultState;

            // Parse parameters
            if (j.contains("parameters") && j["parameters"].is_array())
            {
                for (const auto& paramJson : j["parameters"])
                {
                    std::string name = JsonHelper::GetString(paramJson, "name", "");
                    std::string typeStr = JsonHelper::GetString(paramJson, "type", "float");

                    ParameterValue value;
                    if (typeStr == "bool")
                    {
                        value.type = ParameterType::Bool;
                        value.boolValue = JsonHelper::GetBool(paramJson, "defaultValue", false);
                    }
                    else if (typeStr == "float")
                    {
                        value.type = ParameterType::Float;
                        value.floatValue = JsonHelper::GetFloat(paramJson, "defaultValue", 0.0f);
                    }
                    else if (typeStr == "int")
                    {
                        value.type = ParameterType::Int;
                        value.intValue = JsonHelper::GetInt(paramJson, "defaultValue", 0);
                    }
                    else if (typeStr == "string")
                    {
                        value.type = ParameterType::String;
                        value.stringValue = JsonHelper::GetString(paramJson, "defaultValue", "");
                    }

                    m_parameters[name] = value;
                }
            }

            // Parse states
            if (j.contains("states") && j["states"].is_array())
            {
                for (const auto& stateJson : j["states"])
                {
                    AnimationState state;
                    state.name = JsonHelper::GetString(stateJson, "name", "");
                    state.animationName = JsonHelper::GetString(stateJson, "animationName", "");
                    state.priority = JsonHelper::GetInt(stateJson, "priority", 0);

                    std::string blendModeStr = JsonHelper::GetString(stateJson, "blendMode", "override");
                    if (blendModeStr == "additive")
                        state.blendMode = BlendMode::Additive;
                    else if (blendModeStr == "blend")
                        state.blendMode = BlendMode::Blend;
                    else
                        state.blendMode = BlendMode::Override;

                    m_states[state.name] = state;
                }
            }

            // Parse transitions
            if (j.contains("transitions") && j["transitions"].is_array())
            {
                for (const auto& transJson : j["transitions"])
                {
                    Transition trans;
                    trans.fromState = JsonHelper::GetString(transJson, "from", "");
                    trans.toState = JsonHelper::GetString(transJson, "to", "");
                    trans.transitionTime = JsonHelper::GetFloat(transJson, "transitionTime", 0.1f);

                    // Parse conditions
                    if (transJson.contains("conditions") && transJson["conditions"].is_array())
                    {
                        for (const auto& condJson : transJson["conditions"])
                        {
                            Condition cond;
                            cond.parameter = JsonHelper::GetString(condJson, "parameter", "");
                            
                            // Parse operator
                            std::string opStr = JsonHelper::GetString(condJson, "operator", "==");
                            if (opStr == "==") cond.op = ComparisonOperator::Equal;
                            else if (opStr == "!=") cond.op = ComparisonOperator::NotEqual;
                            else if (opStr == ">") cond.op = ComparisonOperator::Greater;
                            else if (opStr == ">=") cond.op = ComparisonOperator::GreaterOrEqual;
                            else if (opStr == "<") cond.op = ComparisonOperator::Less;
                            else if (opStr == "<=") cond.op = ComparisonOperator::LessOrEqual;

                            // Parse value based on type
                            if (condJson.contains("value"))
                            {
                                if (condJson["value"].is_boolean())
                                {
                                    cond.value = ParameterValue(condJson["value"].get<bool>());
                                }
                                else if (condJson["value"].is_number_float())
                                {
                                    cond.value = ParameterValue(condJson["value"].get<float>());
                                }
                                else if (condJson["value"].is_number_integer())
                                {
                                    cond.value = ParameterValue(condJson["value"].get<int>());
                                }
                                else if (condJson["value"].is_string())
                                {
                                    cond.value = ParameterValue(condJson["value"].get<std::string>());
                                }
                            }

                            trans.conditions.push_back(cond);
                        }
                    }

                    m_transitions.push_back(trans);
                }
            }

            m_isValid = true;
            SYSTEM_LOG << "AnimationGraph: Successfully loaded '" << m_graphName 
                      << "' with " << m_states.size() << " states and " 
                      << m_transitions.size() << " transitions\n";
            return true;
        }
        catch (const std::exception& e)
        {
            SYSTEM_LOG << "AnimationGraph: Error parsing JSON: " << e.what() << "\n";
            return false;
        }
    }

    void AnimationGraph::SetParameter(const std::string& name, bool value)
    {
        m_parameters[name] = ParameterValue(value);
    }

    void AnimationGraph::SetParameter(const std::string& name, float value)
    {
        m_parameters[name] = ParameterValue(value);
    }

    void AnimationGraph::SetParameter(const std::string& name, int value)
    {
        m_parameters[name] = ParameterValue(value);
    }

    void AnimationGraph::SetParameter(const std::string& name, const std::string& value)
    {
        m_parameters[name] = ParameterValue(value);
    }

    bool AnimationGraph::GetParameterBool(const std::string& name, bool defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == ParameterType::Bool)
            return it->second.boolValue;
        return defaultValue;
    }

    float AnimationGraph::GetParameterFloat(const std::string& name, float defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == ParameterType::Float)
            return it->second.floatValue;
        return defaultValue;
    }

    int AnimationGraph::GetParameterInt(const std::string& name, int defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == ParameterType::Int)
            return it->second.intValue;
        return defaultValue;
    }

    std::string AnimationGraph::GetParameterString(const std::string& name, const std::string& defaultValue) const
    {
        auto it = m_parameters.find(name);
        if (it != m_parameters.end() && it->second.type == ParameterType::String)
            return it->second.stringValue;
        return defaultValue;
    }

    void AnimationGraph::SetCurrentState(const std::string& stateName)
    {
        if (m_states.find(stateName) != m_states.end())
        {
            m_currentState = stateName;
        }
    }

    std::string AnimationGraph::GetCurrentAnimationName() const
    {
        auto it = m_states.find(m_currentState);
        if (it != m_states.end())
            return it->second.animationName;
        return "";
    }

    bool AnimationGraph::Update(float deltaTime)
    {
        const Transition* validTransition = FindValidTransition();
        if (validTransition)
        {
            m_currentState = validTransition->toState;
            return true;
        }
        return false;
    }

    bool AnimationGraph::EvaluateCondition(const Condition& condition) const
    {
        auto it = m_parameters.find(condition.parameter);
        if (it == m_parameters.end())
            return false;

        const ParameterValue& paramValue = it->second;
        const ParameterValue& condValue = condition.value;

        // Type must match
        if (paramValue.type != condValue.type)
            return false;

        switch (paramValue.type)
        {
        case ParameterType::Bool:
            if (condition.op == ComparisonOperator::Equal)
                return paramValue.boolValue == condValue.boolValue;
            else if (condition.op == ComparisonOperator::NotEqual)
                return paramValue.boolValue != condValue.boolValue;
            break;

        case ParameterType::Float:
            switch (condition.op)
            {
            case ComparisonOperator::Equal: return paramValue.floatValue == condValue.floatValue;
            case ComparisonOperator::NotEqual: return paramValue.floatValue != condValue.floatValue;
            case ComparisonOperator::Greater: return paramValue.floatValue > condValue.floatValue;
            case ComparisonOperator::GreaterOrEqual: return paramValue.floatValue >= condValue.floatValue;
            case ComparisonOperator::Less: return paramValue.floatValue < condValue.floatValue;
            case ComparisonOperator::LessOrEqual: return paramValue.floatValue <= condValue.floatValue;
            }
            break;

        case ParameterType::Int:
            switch (condition.op)
            {
            case ComparisonOperator::Equal: return paramValue.intValue == condValue.intValue;
            case ComparisonOperator::NotEqual: return paramValue.intValue != condValue.intValue;
            case ComparisonOperator::Greater: return paramValue.intValue > condValue.intValue;
            case ComparisonOperator::GreaterOrEqual: return paramValue.intValue >= condValue.intValue;
            case ComparisonOperator::Less: return paramValue.intValue < condValue.intValue;
            case ComparisonOperator::LessOrEqual: return paramValue.intValue <= condValue.intValue;
            }
            break;

        case ParameterType::String:
            if (condition.op == ComparisonOperator::Equal)
                return paramValue.stringValue == condValue.stringValue;
            else if (condition.op == ComparisonOperator::NotEqual)
                return paramValue.stringValue != condValue.stringValue;
            break;
        }

        return false;
    }

    bool AnimationGraph::EvaluateTransition(const Transition& transition) const
    {
        // All conditions must be true
        for (const auto& condition : transition.conditions)
        {
            if (!EvaluateCondition(condition))
                return false;
        }
        return true;
    }

    const Transition* AnimationGraph::FindValidTransition() const
    {
        // Check transitions sorted by priority (higher priority first)
        // First check "ANY" transitions, then specific transitions
        const Transition* anyTransition = nullptr;

        for (const auto& trans : m_transitions)
        {
            if (trans.fromState == "ANY")
            {
                if (EvaluateTransition(trans))
                {
                    if (!anyTransition)
                        anyTransition = &trans;
                }
            }
            else if (trans.fromState == m_currentState)
            {
                if (EvaluateTransition(trans))
                    return &trans; // Specific transitions take priority
            }
        }

        return anyTransition; // Return ANY transition if no specific transition found
    }

} // namespace OlympeAnimation
