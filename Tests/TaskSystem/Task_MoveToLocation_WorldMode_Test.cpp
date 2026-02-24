/**
 * @file Task_MoveToLocation_WorldMode_Test.cpp
 * @brief Unit tests for Task_MoveToLocation in World (ECS) mode.
 * @author Olympe Engine
 * @date 2026-02-24
 *
 * @details
 * Verifies that Task_MoveToLocation correctly uses PositionComponent and
 * MovementComponent when ctx.ComponentFacade is provided:
 *
 *   a) Returns Running and sets Velocity while the entity is far from target.
 *   b) Returns Success and zeroes Velocity when the entity reaches the target.
 *   c) Falls back to Failure (no LocalBB, no ComponentFacade) when neither
 *      ECS nor LocalBlackboard context is available.
 *   d) Falls back to Failure when ComponentFacade has null Position pointer.
 *   e) Velocity direction is correct (points toward target).
 *
 * Tests drive ExecuteWithContext() directly; no real World or SDL is required.
 * The TaskWorldFacade struct provides component pointers as a lightweight
 * test double.
 *
 * C++14 compliant - no C++17/20 features.
 */

#include "TaskSystem/AtomicTasks/Movement/Task_MoveToLocation.h"
#include "TaskSystem/AtomicTaskContext.h"
#include "TaskSystem/TaskWorldFacade.h"
#include "TaskSystem/TaskGraphTypes.h"
#include "ECS/Components/PositionComponent.h"
#include "ECS/Components/MovementComponent.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                      \
    do {                                                            \
        if (!(cond)) {                                              \
            std::cout << "  FAIL: " << (msg) << std::endl;         \
            ++s_failCount;                                          \
        }                                                           \
    } while (false)

static void ReportTest(const std::string& name, bool passed)
{
    if (passed)
    {
        std::cout << "[PASS] " << name << std::endl;
        ++s_passCount;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++s_failCount;
    }
}

// ---------------------------------------------------------------------------
// Helper: build a minimal ParameterMap with Target (and optional Speed)
// ---------------------------------------------------------------------------

static Olympe::IAtomicTask::ParameterMap MakeParams(
    ::Vector target,
    float speed = 100.0f)
{
    Olympe::IAtomicTask::ParameterMap params;
    params["Target"] = Olympe::TaskValue(target);
    params["Speed"]  = Olympe::TaskValue(speed);
    return params;
}

// ---------------------------------------------------------------------------
// Test a: Running while far from target; Velocity set in correct direction
// ---------------------------------------------------------------------------

static void TestA_RunningWhileFar()
{
    std::cout << "WorldMode: Running while far from target..." << std::endl;

    bool passed = true;

    Olympe::PositionComponent posComp;
    posComp.Position = ::Vector(0.0f, 0.0f, 0.0f);

    Olympe::MovementComponent moveComp;
    moveComp.Velocity = ::Vector(0.0f, 0.0f, 0.0f);

    Olympe::TaskWorldFacade facade;
    facade.Position = &posComp;
    facade.Movement = &moveComp;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity          = 1u;
    ctx.WorldPtr        = nullptr; // no real World required
    ctx.ComponentFacade = &facade;
    ctx.LocalBB         = nullptr;
    ctx.DeltaTime       = 0.016f;

    Olympe::Task_MoveToLocation task;
    Olympe::IAtomicTask::ParameterMap params =
        MakeParams(::Vector(10.0f, 0.0f, 0.0f), 200.0f);

    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Running,
                "Status should be Running while far from target");
    if (status != Olympe::TaskStatus::Running) passed = false;

    TEST_ASSERT(moveComp.Velocity.x > 0.0f,
                "Velocity.x should be positive (moving toward +x target)");
    if (moveComp.Velocity.x <= 0.0f) passed = false;

    TEST_ASSERT(moveComp.Velocity.y == 0.0f,
                "Velocity.y should be 0 (no y component in direction)");

    ReportTest("TestA_RunningWhileFar", passed);
}

// ---------------------------------------------------------------------------
// Test b: Success and Velocity zeroed when entity reaches target
// ---------------------------------------------------------------------------

static void TestB_SuccessAtTarget()
{
    std::cout << "WorldMode: Success when entity reaches target..." << std::endl;

    bool passed = true;

    // Place entity within ARRIVAL_TOLERANCE of target.
    Olympe::PositionComponent posComp;
    posComp.Position = ::Vector(9.9f, 0.0f, 0.0f);

    Olympe::MovementComponent moveComp;
    moveComp.Velocity = ::Vector(1.0f, 0.0f, 0.0f); // non-zero initial velocity

    Olympe::TaskWorldFacade facade;
    facade.Position = &posComp;
    facade.Movement = &moveComp;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity          = 2u;
    ctx.WorldPtr        = nullptr;
    ctx.ComponentFacade = &facade;
    ctx.LocalBB         = nullptr;
    ctx.DeltaTime       = 0.016f;

    Olympe::Task_MoveToLocation task;
    // Target = (10, 0, 0); entity is at (9.9, 0, 0) -> distance 0.1 <= tolerance 0.5
    Olympe::IAtomicTask::ParameterMap params =
        MakeParams(::Vector(10.0f, 0.0f, 0.0f), 100.0f);

    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Success,
                "Status should be Success when entity is within tolerance");
    if (status != Olympe::TaskStatus::Success) passed = false;

    TEST_ASSERT(moveComp.Velocity.Norm() == 0.0f,
                "Velocity should be zeroed on Success");
    if (moveComp.Velocity.Norm() != 0.0f) passed = false;

    ReportTest("TestB_SuccessAtTarget", passed);
}

// ---------------------------------------------------------------------------
// Test c: Failure when neither ComponentFacade nor LocalBB is provided
// ---------------------------------------------------------------------------

static void TestC_FailureNeitherContext()
{
    std::cout << "WorldMode: Failure when no context provided..." << std::endl;

    bool passed = true;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity          = 3u;
    ctx.WorldPtr        = nullptr;
    ctx.ComponentFacade = nullptr; // no facade
    ctx.LocalBB         = nullptr; // no blackboard
    ctx.DeltaTime       = 0.016f;

    Olympe::Task_MoveToLocation task;
    Olympe::IAtomicTask::ParameterMap params =
        MakeParams(::Vector(5.0f, 0.0f, 0.0f));

    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Status should be Failure when no context is available");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestC_FailureNeitherContext", passed);
}

// ---------------------------------------------------------------------------
// Test d: Falls back to Failure when ComponentFacade has null Position pointer
// ---------------------------------------------------------------------------

static void TestD_FallbackOnNullPosition()
{
    std::cout << "WorldMode: Fallback when Position pointer is null..." << std::endl;

    bool passed = true;

    Olympe::MovementComponent moveComp;

    Olympe::TaskWorldFacade facade;
    facade.Position = nullptr; // null Position => task must fall back
    facade.Movement = &moveComp;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity          = 4u;
    ctx.WorldPtr        = nullptr;
    ctx.ComponentFacade = &facade;
    ctx.LocalBB         = nullptr; // no LocalBB either
    ctx.DeltaTime       = 0.016f;

    Olympe::Task_MoveToLocation task;
    Olympe::IAtomicTask::ParameterMap params =
        MakeParams(::Vector(5.0f, 0.0f, 0.0f));

    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Failure,
                "Status should be Failure when Position component is null");
    if (status != Olympe::TaskStatus::Failure) passed = false;

    ReportTest("TestD_FallbackOnNullPosition", passed);
}

// ---------------------------------------------------------------------------
// Test e: Velocity magnitude equals speed when moving
// ---------------------------------------------------------------------------

static void TestE_VelocityMagnitude()
{
    std::cout << "WorldMode: Velocity magnitude equals requested speed..." << std::endl;

    bool passed = true;

    Olympe::PositionComponent posComp;
    posComp.Position = ::Vector(0.0f, 0.0f, 0.0f);

    Olympe::MovementComponent moveComp;
    moveComp.Velocity = ::Vector(0.0f, 0.0f, 0.0f);

    Olympe::TaskWorldFacade facade;
    facade.Position = &posComp;
    facade.Movement = &moveComp;

    Olympe::AtomicTaskContext ctx;
    ctx.Entity          = 5u;
    ctx.WorldPtr        = nullptr;
    ctx.ComponentFacade = &facade;
    ctx.LocalBB         = nullptr;
    ctx.DeltaTime       = 0.016f;

    const float speed = 150.0f;
    Olympe::Task_MoveToLocation task;
    Olympe::IAtomicTask::ParameterMap params =
        MakeParams(::Vector(100.0f, 0.0f, 0.0f), speed);

    Olympe::TaskStatus status = task.ExecuteWithContext(ctx, params);

    TEST_ASSERT(status == Olympe::TaskStatus::Running,
                "Status should be Running");

    float velocityMag = moveComp.Velocity.Norm();
    float diff = velocityMag - speed;
    if (diff < 0.0f) diff = -diff;
    TEST_ASSERT(diff < 0.001f,
                "Velocity magnitude should equal the requested speed");
    if (diff >= 0.001f) passed = false;

    ReportTest("TestE_VelocityMagnitude", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Task_MoveToLocation_WorldMode_Test ===" << std::endl;

    TestA_RunningWhileFar();
    TestB_SuccessAtTarget();
    TestC_FailureNeitherContext();
    TestD_FallbackOnNullPosition();
    TestE_VelocityMagnitude();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
