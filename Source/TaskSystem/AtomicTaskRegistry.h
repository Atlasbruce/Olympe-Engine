/**
 * @file AtomicTaskRegistry.h
 * @brief Singleton registry for atomic task factories.
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * AtomicTaskRegistry stores factory functions keyed by a string task ID.
 * Call Register() to associate a factory with an ID, and Create() to
 * instantiate a task by ID.
 *
 * The REGISTER_ATOMIC_TASK(ClassName, Id) macro registers a factory at
 * static initialization time so that every translation unit that includes
 * the concrete task's .cpp file automatically populates the registry.
 *
 * C++14 compliant - no C++17/20 features.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

#include "IAtomicTask.h"

namespace Olympe {

/**
 * @class AtomicTaskRegistry
 * @brief Singleton registry mapping task IDs to factory functions.
 *
 * @details
 * Usage:
 * @code
 *   // Register (done automatically via REGISTER_ATOMIC_TASK macro):
 *   AtomicTaskRegistry::Get().Register("Task_LogMessage",
 *       []() { return std::unique_ptr<IAtomicTask>(new Task_LogMessage()); });
 *
 *   // Create:
 *   auto task = AtomicTaskRegistry::Get().Create("Task_LogMessage");
 *   if (task) { task->Execute(params); }
 * @endcode
 */
class AtomicTaskRegistry {
public:

    /// Factory function type: returns a heap-allocated IAtomicTask.
    using FactoryFn = std::function<std::unique_ptr<IAtomicTask>()>;

    /**
     * @brief Returns the singleton instance.
     */
    static AtomicTaskRegistry& Get();

    /**
     * @brief Registers a factory function under the given task ID.
     *
     * If a factory is already registered for @p id, it is replaced.
     *
     * @param id      Unique string identifier for the task type.
     * @param factory Callable that produces a new IAtomicTask instance.
     */
    void Register(const std::string& id, FactoryFn factory);

    /**
     * @brief Creates a new instance of the task identified by @p id.
     *
     * @param id Task type identifier (as registered via Register()).
     * @return A unique_ptr to the new task, or nullptr if @p id is unknown.
     */
    std::unique_ptr<IAtomicTask> Create(const std::string& id) const;

    /**
     * @brief Returns true if a factory is registered for @p id.
     * @param id Task type identifier.
     */
    bool IsRegistered(const std::string& id) const;

    /**
     * @brief Normalizes a task ID to its short (prefix-free) form.
     *
     * If @p id starts with the legacy prefix "Task_", the prefix is stripped
     * and the remainder is returned.  Otherwise @p id is returned unchanged.
     *
     * This helper is used internally by Create() and is exposed publicly so
     * that loaders or migration utilities can canonicalize IDs without
     * instantiating a task.
     *
     * An ID that consists only of the prefix with no following characters
     * (i.e. exactly "Task_") is returned unchanged because an empty short ID
     * would be meaningless.
     *
     * Examples:
     * @code
     *   NormalizeTaskID("Task_MoveToLocation") == "MoveToLocation"
     *   NormalizeTaskID("MoveToLocation")      == "MoveToLocation"
     *   NormalizeTaskID("Task_")               == "Task_"   // no suffix
     * @endcode
     *
     * @param id Task type identifier (with or without "Task_" prefix).
     * @return Short-form identifier with no "Task_" prefix.
     */
    static std::string NormalizeTaskID(const std::string& id);

private:

    AtomicTaskRegistry() = default;

    std::unordered_map<std::string, FactoryFn> m_factories;
};

} // namespace Olympe

// ---------------------------------------------------------------------------
// REGISTER_ATOMIC_TASK macro
// ---------------------------------------------------------------------------

/**
 * @def REGISTER_ATOMIC_TASK(ClassName, Id)
 * @brief Registers a factory for ClassName under Id at static init time.
 *
 * Place this macro in the .cpp file of your concrete task class (outside any
 * namespace or function):
 *
 * @code
 *   REGISTER_ATOMIC_TASK(Task_LogMessage, "Task_LogMessage")
 * @endcode
 *
 * The macro creates a file-scope object whose constructor calls
 * AtomicTaskRegistry::Get().Register(), ensuring registration happens
 * before main() is entered.
 */
#define REGISTER_ATOMIC_TASK(ClassName, Id)                                     \
    namespace {                                                                  \
        struct ClassName##_Registrar {                                           \
            ClassName##_Registrar() {                                            \
                ::Olympe::AtomicTaskRegistry::Get().Register(                   \
                    Id,                                                          \
                    []() -> std::unique_ptr<::Olympe::IAtomicTask> {            \
                        return std::unique_ptr<::Olympe::IAtomicTask>(          \
                            new ClassName());                                    \
                    });                                                          \
            }                                                                    \
        };                                                                       \
        static ClassName##_Registrar s_##ClassName##_registrar;                 \
    }
