#include "AnimationGraph.h"
#include "../json_helper.h"
#include "../system/system_utils.h"
#include <algorithm>

using json = nlohmann::json;

namespace OlympeAnimation
{
    namespace
    {
        static std::string LowerCopy(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        }

        static std::string NormalizeDirection(const std::string& direction)
        {
            const std::string v = LowerCopy(direction);
            if (v == "n") return "N";
            if (v == "ne") return "NE";
            if (v == "e") return "E";
            if (v == "se") return "SE";
            if (v == "s") return "S";
            if (v == "sw") return "SW";
            if (v == "w") return "W";
            if (v == "nw") return "NW";
            return "S";
        }
    }

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
            m_graphName = JsonHelper::GetString(j, "name", JsonHelper::GetString(j, "graphName", "unknown"));
            m_description = JsonHelper::GetString(j, "description", "");
            m_defaultState = JsonHelper::GetString(j, "defaultState", "Idle");
            m_currentState = m_defaultState;

            if (j.contains("states") && j["states"].is_array())
            {
                for (const auto& stateJson : j["states"])
                {
                    State state;
                    state.name = JsonHelper::GetString(stateJson, "name", "");
                    state.defaultClip = JsonHelper::GetString(stateJson, "defaultClip", "");
                    state.blendTime = JsonHelper::GetFloat(stateJson, "blendTime", 0.1f);
                    state.loop = JsonHelper::GetBool(stateJson, "loop", true);
                    state.priority = JsonHelper::GetInt(stateJson, "priority", 0);
                    if (stateJson.contains("directionClips") && stateJson["directionClips"].is_array())
                    {
                        for (const auto& clipJson : stateJson["directionClips"])
                        {
                            DirectionClip clip;
                            clip.direction = NormalizeDirection(JsonHelper::GetString(clipJson, "direction", "S"));
                            clip.clip = JsonHelper::GetString(clipJson, "clip", "");
                            state.directionClips.push_back(clip);
                        }
                    }
                    if (stateJson.contains("events") && stateJson["events"].is_array())
                    {
                        for (const auto& evt : stateJson["events"])
                        {
                            if (evt.is_string())
                                state.events.push_back(evt.get<std::string>());
                        }
                    }
                    m_states[state.name] = state;
                }
            }

            if (j.contains("transitions") && j["transitions"].is_array())
            {
                for (const auto& transJson : j["transitions"])
                {
                    Transition transition;
                    transition.fromState = JsonHelper::GetString(transJson, "from", "");
                    transition.toState = JsonHelper::GetString(transJson, "to", "");
                    transition.condition = JsonHelper::GetString(transJson, "condition", "");
                    transition.duration = JsonHelper::GetFloat(transJson, "duration", 0.1f);
                    transition.priority = JsonHelper::GetInt(transJson, "priority", 0);
                    transition.interruptible = JsonHelper::GetBool(transJson, "interruptible", true);
                    m_transitions.push_back(transition);
                }
            }

            m_isValid = !m_states.empty();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void AnimationGraph::SetCurrentState(const std::string& stateName) { if (m_states.find(stateName) != m_states.end()) m_currentState = stateName; }
    void AnimationGraph::SetParameter(const std::string& name, bool value) { m_parameters[name] = ParameterValue(value); }
    void AnimationGraph::SetParameter(const std::string& name, float value) { m_parameters[name] = ParameterValue(value); }
    void AnimationGraph::SetParameter(const std::string& name, int value) { m_parameters[name] = ParameterValue(value); }
    void AnimationGraph::SetParameter(const std::string& name, const std::string& value) { m_parameters[name] = ParameterValue(value); }
    bool AnimationGraph::GetParameterBool(const std::string& name, bool defaultValue) const { const auto it = m_parameters.find(name); return (it != m_parameters.end() && it->second.type == ParameterType::Bool) ? it->second.boolValue : defaultValue; }
    float AnimationGraph::GetParameterFloat(const std::string& name, float defaultValue) const { const auto it = m_parameters.find(name); return (it != m_parameters.end() && it->second.type == ParameterType::Float) ? it->second.floatValue : defaultValue; }
    int AnimationGraph::GetParameterInt(const std::string& name, int defaultValue) const { const auto it = m_parameters.find(name); return (it != m_parameters.end() && it->second.type == ParameterType::Int) ? it->second.intValue : defaultValue; }
    std::string AnimationGraph::GetParameterString(const std::string& name, const std::string& defaultValue) const { const auto it = m_parameters.find(name); return (it != m_parameters.end() && it->second.type == ParameterType::String) ? it->second.stringValue : defaultValue; }

    bool AnimationGraph::Update(float deltaTime)
    {
        (void)deltaTime;
        const Transition* best = nullptr;
        for (const auto& transition : m_transitions)
        {
            if (transition.fromState != m_currentState && transition.fromState != "ANY")
                continue;
            if (!EvaluateTransition(transition))
                continue;
            if (!best || transition.priority > best->priority)
                best = &transition;
        }
        if (best)
        {
            m_currentState = best->toState;
            return true;
        }
        return false;
    }

    std::string AnimationGraph::GetCurrentAnimationName() const
    {
        return ResolveClip(m_currentState, m_direction);
    }

    std::string AnimationGraph::ResolveClip(const std::string& stateName, const std::string& direction) const
    {
        const State* state = GetState(stateName);
        if (!state) return "";
        const std::string normalizedDirection = NormalizeDirection(direction);
        for (const auto& clip : state->directionClips)
        {
            if (clip.direction == normalizedDirection && !clip.clip.empty())
                return clip.clip;
        }
        return state->defaultClip;
    }

    const AnimationGraph::State* AnimationGraph::GetState(const std::string& stateName) const
    {
        const auto it = m_states.find(stateName);
        return it != m_states.end() ? &it->second : nullptr;
    }

    bool AnimationGraph::EvaluateTransition(const Transition& transition) const
    {
        if (transition.condition.empty())
            return true;
        return true;
    }

} // namespace OlympeAnimation
