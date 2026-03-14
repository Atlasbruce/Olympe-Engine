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
    Register({"move_to_goal",    "Move To Goal",    "Movement",
              "Move the agent toward its current goal position."});
    Register({"rotate_to_face",  "Rotate To Face",  "Movement",
              "Rotate the agent to face a target entity or position."});
    Register({"patrol_path",     "Patrol Path",     "Movement",
              "Walk along a predefined patrol waypoint path."});

    // ---- Combat ----
    Register({"attack_if_close", "Attack If Close", "Combat",
              "Perform a melee or ranged attack if the target is within range."});
    Register({"perform_dodge",   "Perform Dodge",   "Combat",
              "Execute an evasive dodge maneuver."});
    Register({"take_cover",      "Take Cover",      "Combat",
              "Move the agent to the nearest available cover position."});

    // ---- Animation ----
    Register({"play_animation",  "Play Animation",  "Animation",
              "Start playing a named animation clip."});
    Register({"stop_animation",  "Stop Animation",  "Animation",
              "Stop the currently playing animation."});
    Register({"blend_animation", "Blend Animation", "Animation",
              "Blend between two animation clips by a weight parameter."});

    // ---- Audio ----
    Register({"play_sound",      "Play Sound",      "Audio",
              "Play a sound effect or music track by name."});
    Register({"stop_sound",      "Stop Sound",      "Audio",
              "Stop a currently playing sound."});
    Register({"set_volume",      "Set Volume",      "Audio",
              "Set the volume for a sound channel."});

    // ---- Misc ----
    Register({"log_message",     "Log Message",     "Misc",
              "Write a diagnostic message to the system log."});
    Register({"set_state",       "Set State",       "Misc",
              "Set a named state variable on the agent."});
    Register({"clear_target",    "Clear Target",    "Misc",
              "Clear the agent's current target reference."});
}

} // namespace Olympe
