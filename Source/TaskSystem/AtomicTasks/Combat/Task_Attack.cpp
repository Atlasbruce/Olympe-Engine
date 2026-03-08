/**
 * @file Task_Attack.cpp
 * @brief Atomic task: initiate a combat attack action.
 * @author Olympe Engine
 * @date 2026-03-08
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "Task_Attack.h"
#include "../../AtomicTaskRegistry.h"
#include "../../LocalBlackboard.h"
#include "../../../system/system_utils.h"

namespace Olympe {

static const float DEFAULT_DAMAGE = 10.0f;
static const float DEFAULT_RANGE  = 50.0f;

Task_Attack::Task_Attack() {}

void Task_Attack::Abort()
{
    SYSTEM_LOG << "[Task_Attack] Abort()\n";
}

TaskStatus Task_Attack::Execute(const ParameterMap& /*params*/)
{
    return TaskStatus::Failure; // requires context; use ExecuteWithContext
}

TaskStatus Task_Attack::ExecuteWithContext(const AtomicTaskContext& ctx,
                                            const ParameterMap& params)
{
    if (!ctx.LocalBB)
    {
        SYSTEM_LOG << "[Task_Attack] No LocalBlackboard in context\n";
        return TaskStatus::Failure;
    }

    // --- Resolve Damage parameter (optional) ---
    float damage = DEFAULT_DAMAGE;
    {
        auto it = params.find("Damage");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
        {
            damage = it->second.AsFloat();
        }
    }

    // --- Resolve Range parameter (optional) ---
    float range = DEFAULT_RANGE;
    {
        auto it = params.find("Range");
        if (it != params.end() && it->second.GetType() == VariableType::Float)
        {
            range = it->second.AsFloat();
        }
    }

    // --- Read target from LocalBlackboard ---
    EntityID target = INVALID_ENTITY_ID;
    {
        const TaskValue targetVal = ctx.LocalBB->GetValueScoped("local:Target");
        if (targetVal.GetType() == VariableType::EntityID)
        {
            target = targetVal.AsEntityID();
        }
    }

    SYSTEM_LOG << "[Task_Attack] Entity " << ctx.Entity
               << " attacks target=" << target
               << " damage=" << damage
               << " range=" << range << "\n";

    return TaskStatus::Success;
}

REGISTER_ATOMIC_TASK(Task_Attack, "Task_Attack")

} // namespace Olympe
