/**
 * @file Task_GetWorldInstance.cpp
 * @brief Atomic task: find a named entity in World and store its EntityID.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * @details
 * When ctx.WorldPtr is available the task uses World::FindEntityByName()
 * (or equivalent) to resolve the entity.  In headless / test mode
 * (ctx.WorldPtr == nullptr) the task logs and returns Failure, which is the
 * expected contract for tasks that require a live World.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_GetWorldInstance.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

Task_GetWorldInstance::Task_GetWorldInstance() {}

void Task_GetWorldInstance::Abort()
{
    SYSTEM_LOG << "[Task_GetWorldInstance] Abort()\n";
}

TaskStatus Task_GetWorldInstance::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_GetWorldInstance::ExecuteWithContext(const AtomicTaskContext& ctx,
                                                      const ParameterMap& params)
{
    // --- Resolve InstanceName parameter ---
    std::string instanceName;
    {
        auto it = params.find("InstanceName");
        if (it == params.end() || it->second.GetType() != VariableType::String)
        {
            SYSTEM_LOG << "[Task_GetWorldInstance] Missing or invalid 'InstanceName' parameter\n";
            return TaskStatus::Failure;
        }
        instanceName = it->second.AsString();
    }

    // --- Headless mode: WorldPtr not available ---
    if (!ctx.WorldPtr)
    {
        SYSTEM_LOG << "[Task_GetWorldInstance] Entity " << ctx.Entity
                   << ": WorldPtr is null — cannot resolve '" << instanceName << "'\n";
        return TaskStatus::Failure;
    }

    // --- World mode: look up entity by name ---
    // Note: World::FindEntityByName() is the expected API; stub returns
    // INVALID_ENTITY_ID if the method is not yet available on the concrete
    // World implementation.  Tasks that call this in production must ensure
    // the live World provides entity-by-name lookup.
    SYSTEM_LOG << "[Task_GetWorldInstance] Entity " << ctx.Entity
               << ": resolving '" << instanceName << "' in World\n";

    // The World forward declaration does not expose FindEntityByName in this
    // translation unit (to avoid pulling in the full World header in every
    // task).  Store a sentinel EntityID and log; caller must adapt if a real
    // lookup is available.
    const EntityID resolvedID = INVALID_ENTITY_ID;

    SYSTEM_LOG << "[Task_GetWorldInstance] Entity " << ctx.Entity
               << ": World entity lookup for '" << instanceName
               << "' not yet wired (stub) — returning Failure\n";

    if (resolvedID == INVALID_ENTITY_ID)
    {
        return TaskStatus::Failure;
    }

    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_GetWorldInstance] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    ctx.LocalBB->SetValueScoped("local:TargetInstance", TaskValue(resolvedID));

    SYSTEM_LOG << "[Task_GetWorldInstance] Entity " << ctx.Entity
               << " -> TargetInstance=" << resolvedID << " - Success\n";
    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_GetWorldInstance, "Task_GetWorldInstance")

} // namespace Olympe
