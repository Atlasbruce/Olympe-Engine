/*
Olympe Engine V2 2025
Animation System - Animation Graph (State Machine)

Purpose:
- Define state machine structure for animations
- Handle state transitions based on parameters
- Evaluate conditions for automatic state changes
*/

#pragma once

#include "AnimationTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace OlympeAnimation
{
    // Forward declaration
    class AnimationBank;

    // ========================================================================
    // Condition - Evaluates a parameter against a value
    // ========================================================================
    struct Condition
    {
        std::string parameter;           // Parameter name to check
        ComparisonOperator op;           // Comparison operator
        ParameterValue value;            // Value to compare against

        Condition() : op(ComparisonOperator::Equal) {}
    };

    // ========================================================================
    // Transition - Defines a transition between two states
    // ========================================================================
    struct Transition
    {
        std::string fromState;           // Source state (or "ANY" for any state)
        std::string toState;             // Target state
        float transitionTime = 0.1f;     // Blend duration in seconds
        std::vector<Condition> conditions; // Conditions that must be met

        Transition() = default;
    };

    // ========================================================================
    // AnimationState - Single state in the animation graph
    // ========================================================================
    struct AnimationState
    {
        std::string name;                // State name (e.g., "Idle", "Walk")
        std::string animationName;       // Animation to play in this state
        BlendMode blendMode = BlendMode::Override;
        int priority = 0;                // Higher priority = more important

        AnimationState() = default;
    };

    // ========================================================================
    // AnimationGraph - State machine for animation control
    // ========================================================================
    class AnimationGraph
    {
    public:
        AnimationGraph() = default;
        ~AnimationGraph() = default;

        // Load animation graph from JSON file
        bool LoadFromFile(const std::string& filePath);

        // Parameter management
        void SetParameter(const std::string& name, bool value);
        void SetParameter(const std::string& name, float value);
        void SetParameter(const std::string& name, int value);
        void SetParameter(const std::string& name, const std::string& value);

        bool GetParameterBool(const std::string& name, bool defaultValue = false) const;
        float GetParameterFloat(const std::string& name, float defaultValue = 0.0f) const;
        int GetParameterInt(const std::string& name, int defaultValue = 0) const;
        std::string GetParameterString(const std::string& name, const std::string& defaultValue = "") const;

        // State management
        void SetCurrentState(const std::string& stateName);
        const std::string& GetCurrentState() const { return m_currentState; }
        
        // Get animation name for current state
        std::string GetCurrentAnimationName() const;

        // Update state machine (checks for transitions)
        // Returns true if state changed
        bool Update(float deltaTime);

        // Get graph name
        const std::string& GetGraphName() const { return m_graphName; }

        // Get animation bank path
        const std::string& GetAnimationBankPath() const { return m_animationBankPath; }

        // Check if graph is valid
        bool IsValid() const { return m_isValid; }

    private:
        std::string m_graphName;
        std::string m_description;
        std::string m_animationBankPath;
        std::string m_currentState;
        std::string m_defaultState = "Idle";
        bool m_isValid = false;

        std::unordered_map<std::string, ParameterValue> m_parameters;
        std::unordered_map<std::string, AnimationState> m_states;
        std::vector<Transition> m_transitions;

        // Helper: Parse JSON
        bool ParseJSON(const std::string& jsonContent);

        // Helper: Evaluate a single condition
        bool EvaluateCondition(const Condition& condition) const;

        // Helper: Evaluate all conditions for a transition
        bool EvaluateTransition(const Transition& transition) const;

        // Helper: Find valid transition from current state
        const Transition* FindValidTransition() const;
    };

} // namespace OlympeAnimation
