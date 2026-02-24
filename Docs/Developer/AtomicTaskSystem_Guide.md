# Atomic Task System – Developer Guide

## Overview

The Atomic Task System (ATS) provides a lightweight, data-driven framework for
composing entity behaviours from small, reusable `IAtomicTask` units.  Each
entity carries a `TaskRunnerComponent` that references a `TaskGraphTemplate`
describing the sequence of tasks to execute.

---

## Core Concepts

| Concept | Description |
|---------|-------------|
| `IAtomicTask` | Base interface for a single unit of work. |
| `TaskGraphTemplate` | Immutable asset describing nodes, BB variables, and parameters. |
| `LocalBlackboard` | Per-entity runtime key-value store initialised from the template. |
| `TaskRunnerComponent` | ECS component that tracks current node, active task, and last status. |
| `TaskSystem` | ECS system that drives task execution each frame. |
| `AtomicTaskRegistry` | Singleton factory that maps task ID strings to `IAtomicTask` factories. |

---

## Writing an Atomic Task

1. Inherit from `IAtomicTask` and override `ExecuteWithContext()` and `Abort()`.
2. Use `REGISTER_ATOMIC_TASK(ClassName, "ID")` at the bottom of your `.cpp` file.

```cpp
#include "IAtomicTask.h"
#include "AtomicTaskRegistry.h"

namespace Olympe {

class Task_MyAction : public IAtomicTask {
public:
    TaskStatus ExecuteWithContext(const AtomicTaskContext& ctx,
                                  const ParameterMap& params) override
    {
        // Read a parameter
        auto it = params.find("Duration");
        float dur = (it != params.end()) ? it->second.AsFloat() : 1.0f;

        if (ctx.StateTimer >= dur) return TaskStatus::Success;
        return TaskStatus::Running;
    }

    TaskStatus Execute(const ParameterMap& /*params*/) override
    {
        return TaskStatus::Failure; // requires context
    }

    void Abort() override {}
};

REGISTER_ATOMIC_TASK(Task_MyAction, "Task_MyAction")

} // namespace Olympe
```

---

## LocalBlackboard

The `LocalBlackboard` stores typed `TaskValue` variables.  Variables are declared
in the `TaskGraphTemplate` schema and initialised to their defaults when the
runner starts.

```cpp
// Read
::Vector pos = ctx.LocalBB->GetValue("Position").AsVector();

// Write
ctx.LocalBB->SetValue("Position", TaskValue(newPos));
```

Supported types: `Bool`, `Int`, `Float`, `Vector`, `EntityID`, `String`.

---

## Async Tasks

Long-running tasks should return `TaskStatus::Running` until work is complete.
For CPU-bound or IO-bound work, offload to a background thread and poll each
tick.  See [AtomicTaskSystem_Pathfinding.md](AtomicTaskSystem_Pathfinding.md)
for a full example using `PathfindingManager`.

Pattern:

```
First tick:  submit work → return Running
Later ticks: poll → if done, write result to BB → return Success
Abort():     cancel pending work
```

---

## Parameter Binding

Parameters are bound in the `TaskNodeDefinition`:

| Binding Type | Description |
|-------------|-------------|
| `Literal` | Value is embedded in the template at authoring time. |
| `LocalVariable` | Value is read from the LocalBlackboard at runtime. |

---

## Testing Atomic Tasks (headless)

Use `TaskSystem::ExecuteNode()` directly to drive tasks without a running World:

```cpp
Olympe::TaskGraphTemplate   tmpl   = BuildTemplate();
Olympe::TaskSystem          system;
Olympe::TaskRunnerComponent runner;

for (int tick = 0; tick < 200; ++tick)
{
    system.ExecuteNode(1u, runner, &tmpl, 0.016f);
    if (runner.CurrentNodeIndex == Olympe::NODE_INDEX_NONE) break;
}

assert(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success);
```

Include `Tests/TestStubs.cpp` in test executables to satisfy the `SystemLogSink`
link dependency.

---

## Build & Run Tests

```bash
mkdir build && cd build
cmake -DBUILD_TESTS=ON -DBUILD_RUNTIME_ENGINE=OFF \
      -DBUILD_BLUEPRINT_EDITOR_STANDALONE=OFF ..
cmake --build .
ctest --output-on-failure
```

---

## Available Built-in Tasks

| Task ID | Parameters | BB Reads | BB Writes | Notes |
|---------|-----------|----------|-----------|-------|
| `Task_LogMessage` | `Message` (String) | – | – | Logs a message each tick. |
| `Task_Wait` | `Duration` (Float, s) | – | – | Waits for the given duration. |
| `Task_MoveToLocation` | `Target` (Vector), `Speed` (Float) | `Position` | `Position` | Moves entity toward target. |
| `Task_SetVariable` | `VarName` (String), `Value` (any) | – | `VarName` | Writes a value to the BB. |
| `Task_Compare` | `VarName` (String), `Value` (any) | `VarName` | – | Succeeds if BB value matches. |
| `Task_RequestPathfinding` | `Target` (Vector), `AsyncDelay` (Float) | `Position` | `Path` | Async path request via PathfindingManager. |
