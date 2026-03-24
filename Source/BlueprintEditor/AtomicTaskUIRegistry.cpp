/**
 * @file AtomicTaskUIRegistry.cpp
 * @brief Implementation of AtomicTaskUIRegistry with built-in task specs.
 * @author Olympe Engine
 * @date 2026-03-14
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "AtomicTaskUIRegistry.h"

#include <algorithm>

namespace Olympe {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

AtomicTaskUIRegistry& AtomicTaskUIRegistry::Get()
{
    static AtomicTaskUIRegistry instance;
    return instance;
}

AtomicTaskUIRegistry::AtomicTaskUIRegistry()
{
    InitializeBuiltInTasks();
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

void AtomicTaskUIRegistry::Register(const TaskSpec& spec)
{
    if (!spec.id.empty())
        m_specs[spec.id] = spec;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const TaskSpec* AtomicTaskUIRegistry::GetTaskSpec(const std::string& id) const
{
    auto it = m_specs.find(id);
    return (it != m_specs.end()) ? &it->second : nullptr;
}

std::vector<std::string> AtomicTaskUIRegistry::GetAllTaskIds() const
{
    std::vector<std::string> ids;
    ids.reserve(m_specs.size());
    for (const auto& kv : m_specs)
        ids.push_back(kv.first);
    return ids;
}

std::vector<TaskSpec> AtomicTaskUIRegistry::GetTasksByCategory(const std::string& category) const
{
    std::vector<TaskSpec> result;
    for (const auto& kv : m_specs)
    {
        if (kv.second.category == category)
            result.push_back(kv.second);
    }
    std::sort(result.begin(), result.end(),
              [](const TaskSpec& a, const TaskSpec& b) {
                  return a.displayName < b.displayName;
              });
    return result;
}

std::vector<std::string> AtomicTaskUIRegistry::GetAllCategories() const
{
    std::vector<std::string> cats;
    for (const auto& kv : m_specs)
    {
        const std::string& cat = kv.second.category;
        bool found = false;
        for (size_t i = 0; i < cats.size(); ++i)
        {
            if (cats[i] == cat) { found = true; break; }
        }
        if (!found)
            cats.push_back(cat);
    }
    std::sort(cats.begin(), cats.end());
    return cats;
}

std::vector<TaskSpec> AtomicTaskUIRegistry::GetSortedForUI() const
{
    // Sort by category then by displayName
    std::vector<TaskSpec> result;
    result.reserve(m_specs.size());
    for (const auto& kv : m_specs)
        result.push_back(kv.second);

    std::sort(result.begin(), result.end(),
              [](const TaskSpec& a, const TaskSpec& b) {
                  if (a.category != b.category)
                      return a.category < b.category;
                  return a.displayName < b.displayName;
              });
    return result;
}

// ---------------------------------------------------------------------------
// Built-in task specs
// ---------------------------------------------------------------------------

void AtomicTaskUIRegistry::InitializeBuiltInTasks()
{
    // ---- Movement ----
    {
        TaskSpec spec{"move_to_goal", "Move To Goal", "Movement",
                      "Move the agent toward its current goal position."};
        spec.parameters = {
            {"targetKey", "String", "TargetPosition", "Variable key for the target position"},
            {"speed", "Float", "3.5", "Movement speed"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"rotate_to_face", "Rotate To Face", "Movement",
                      "Rotate the agent to face a target entity or position."};
        spec.parameters = {
            {"targetKey", "String", "TargetActor", "Variable key for the target"},
            {"rotationSpeed", "Float", "180.0", "Rotation speed in degrees per second"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"patrol_path", "Patrol Path", "Movement",
                      "Walk along a predefined patrol waypoint path."};
        spec.parameters = {
            {"pathName", "String", "", "Name of the patrol path to follow"},
            {"speed", "Float", "2.0", "Patrol movement speed"}
        };
        Register(spec);
    }

    // ---- Combat ----
    {
        TaskSpec spec{"attack_if_close", "Attack If Close", "Combat",
                      "Perform a melee or ranged attack if the target is within range."};
        spec.parameters = {
            {"targetKey", "String", "TargetActor", "Variable key for the target"},
            {"range", "Float", "5.0", "Attack range"},
            {"damage", "Float", "10.0", "Damage value"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"perform_dodge", "Perform Dodge", "Combat",
                      "Execute an evasive dodge maneuver."};
        spec.parameters = {
            {"dodgeDistance", "Float", "2.0", "Distance to dodge"},
            {"dodgeSpeed", "Float", "8.0", "Speed of dodge movement"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"take_cover", "Take Cover", "Combat",
                      "Move the agent to the nearest available cover position."};
        spec.parameters = {
            {"searchRadius", "Float", "20.0", "Search radius for cover positions"},
            {"moveSpeed", "Float", "5.0", "Speed to move to cover"}
        };
        Register(spec);
    }

    // ---- Animation ----
    {
        TaskSpec spec{"play_animation", "Play Animation", "Animation",
                      "Start playing a named animation clip."};
        spec.parameters = {
            {"animationName", "String", "", "Name of the animation to play"},
            {"speed", "Float", "1.0", "Animation playback speed"},
            {"loop", "Bool", "false", "Whether to loop the animation"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"stop_animation", "Stop Animation", "Animation",
                      "Stop the currently playing animation."};
        spec.parameters = {
            {"fadeOutTime", "Float", "0.5", "Time to fade out the animation"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"blend_animation", "Blend Animation", "Animation",
                      "Blend between two animation clips by a weight parameter."};
        spec.parameters = {
            {"animationNameA", "String", "", "First animation name"},
            {"animationNameB", "String", "", "Second animation name"},
            {"blendWeight", "Float", "0.5", "Blend weight (0.0 = A, 1.0 = B)"}
        };
        Register(spec);
    }

    // ---- Audio ----
    {
        TaskSpec spec{"play_sound", "Play Sound", "Audio",
                      "Play a sound effect or music track by name."};
        spec.parameters = {
            {"soundName", "String", "", "Name of the sound to play"},
            {"volume", "Float", "1.0", "Volume (0.0 to 1.0)"},
            {"loop", "Bool", "false", "Whether to loop the sound"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"stop_sound", "Stop Sound", "Audio",
                      "Stop a currently playing sound."};
        spec.parameters = {
            {"soundName", "String", "", "Name of the sound to stop"},
            {"fadeOutTime", "Float", "0.5", "Time to fade out"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"set_volume", "Set Volume", "Audio",
                      "Set the volume for a sound channel."};
        spec.parameters = {
            {"channelName", "String", "", "Name of the audio channel"},
            {"volume", "Float", "1.0", "Volume level (0.0 to 1.0)"}
        };
        Register(spec);
    }

    // ---- Misc ----
    {
        TaskSpec spec{"log_message", "Log Message", "Misc",
                      "Write a diagnostic message to the system log."};
        spec.parameters = {
            {"message", "String", "Debug message here", "The message to log"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"set_state", "Set State", "Misc",
                      "Set a named state variable on the agent."};
        spec.parameters = {
            {"stateName", "String", "", "Name of the state variable"},
            {"stateValue", "String", "", "Value to set"}
        };
        Register(spec);
    }

    {
        TaskSpec spec{"clear_target", "Clear Target", "Misc",
                      "Clear the agent's current target reference."};
        spec.parameters = {};  // No parameters
        Register(spec);
    }
}

} // namespace Olympe
