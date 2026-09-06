#pragma once

#include "AnimationTypes.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace OlympeAnimation
{
    class AnimationGraph
    {
    public:
        struct DirectionClip
        {
            std::string direction;
            std::string clip;
        };

        struct State
        {
            std::string name;
            std::string defaultClip;
            std::vector<DirectionClip> directionClips;
            std::vector<std::string> events;
            float blendTime = 0.1f;
            bool loop = true;
            int priority = 0;
        };

        struct Transition
        {
            std::string fromState;
            std::string toState;
            std::string condition;
            float duration = 0.1f;
            int priority = 0;
            bool interruptible = true;
        };

        AnimationGraph() = default;
        ~AnimationGraph() = default;

        bool LoadFromFile(const std::string& filePath);
        bool ParseJSON(const std::string& jsonContent);

        void SetCurrentState(const std::string& stateName);
        const std::string& GetCurrentState() const { return m_currentState; }

        void SetDirection(const std::string& direction) { m_direction = direction; }
        const std::string& GetDirection() const { return m_direction; }

        void SetParameter(const std::string& name, bool value);
        void SetParameter(const std::string& name, float value);
        void SetParameter(const std::string& name, int value);
        void SetParameter(const std::string& name, const std::string& value);

        bool GetParameterBool(const std::string& name, bool defaultValue = false) const;
        float GetParameterFloat(const std::string& name, float defaultValue = 0.0f) const;
        int GetParameterInt(const std::string& name, int defaultValue = 0) const;
        std::string GetParameterString(const std::string& name, const std::string& defaultValue = "") const;

        bool Update(float deltaTime);
        const std::string& GetGraphName() const { return m_graphName; }
        const std::string& GetDefaultState() const { return m_defaultState; }
        bool IsValid() const { return m_isValid; }
        std::string GetCurrentAnimationName() const;
        std::string ResolveClip(const std::string& stateName, const std::string& direction) const;
        const State* GetState(const std::string& stateName) const;

    private:
        std::string m_graphName;
        std::string m_description;
        std::string m_defaultState = "Idle";
        std::string m_currentState = "Idle";
        std::string m_direction = "S";
        bool m_isValid = false;

        std::unordered_map<std::string, ParameterValue> m_parameters;
        std::unordered_map<std::string, State> m_states;
        std::vector<Transition> m_transitions;

        bool EvaluateTransition(const Transition& transition) const;
    };

} // namespace OlympeAnimation
