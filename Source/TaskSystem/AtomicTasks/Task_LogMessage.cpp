/**
 * @file Task_LogMessage.cpp
 * @brief Example atomic task that logs a message parameter.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Task_LogMessage reads the "message" string parameter from the parameter map
 * and writes it to the system log.  Returns TaskStatus::Success unconditionally.
 *
 * Parameter map:
 *   "message" (String) - the text to log.  Defaults to "(no message)" if absent.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "../AtomicTaskRegistry.h"
#include "../../system/system_utils.h"

namespace Olympe {

/**
 * @class Task_LogMessage
 * @brief Atomic task that logs the value of the "message" parameter.
 */
class Task_LogMessage : public IAtomicTask {
public:

    TaskStatus Execute(const ParameterMap& params) override
    {
        std::string message = "(no message)";

        auto it = params.find("message");
        if (it != params.end() && it->second.GetType() == VariableType::String)
        {
            message = it->second.AsString();
        }

        SYSTEM_LOG << "[Task_LogMessage] " << message << "\n";

        return TaskStatus::Success;
    }

    void Abort() override
    {
        // Task_LogMessage is instantaneous (always returns Success on first
        // Execute() call), so there is no in-progress state to clean up.
    }
};

REGISTER_ATOMIC_TASK(Task_LogMessage, "Task_LogMessage")

} // namespace Olympe
