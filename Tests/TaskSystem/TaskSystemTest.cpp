/**
 * @file TaskSystemTest.cpp
 * @brief Unit tests for TaskSystem (Phase 1.4 skeleton).
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Tests cover:
 *   a) TaskSystem can be instantiated without crashing.
 *   b) Process() completes without crashing when m_entities is empty.
 *   c) Process() skips entities bound to INVALID_ASSET_ID (no template crash).
 *   d) TaskRunnerComponent default state is correct.
 *
 * Run from the repository root.
 * No SDL3 or World dependency - tests exercise the standalone skeleton only.
 */

#include "TaskSystem/TaskSystem.h"
#include "ECS/Components/TaskRunnerComponent.h"
#include "Core/AssetManager.h"

#include <iostream>
#include <cassert>
#include <string>

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(condition, message)                             \
    do {                                                            \
        if (!(condition)) {                                         \
            std::cout << "  FAIL: " << (message) << std::endl;     \
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
// Test a: TaskSystem instantiation
// ---------------------------------------------------------------------------

static void TestA_InstantiateTaskSystem()
{
    std::cout << "Test A: TaskSystem instantiation..." << std::endl;

    bool passed = true;

    // TaskSystem must be instantiatable without crashing.
    Olympe::TaskSystem system;

    // m_entities should be empty after construction.
    TEST_ASSERT(system.m_entities.empty(), "m_entities should be empty after construction");
    if (!system.m_entities.empty()) { passed = false; }

    ReportTest("TestA_InstantiateTaskSystem", passed);
}

// ---------------------------------------------------------------------------
// Test b: Process() with empty entity set
// ---------------------------------------------------------------------------

static void TestB_ProcessEmptyEntities()
{
    std::cout << "Test B: Process() with empty m_entities..." << std::endl;

    Olympe::TaskSystem system;

    // Process() must not crash when there are no entities.
    system.Process();

    ReportTest("TestB_ProcessEmptyEntities", true);
}

// ---------------------------------------------------------------------------
// Test c: Process() with an entity bound to INVALID_ASSET_ID
// ---------------------------------------------------------------------------

static void TestC_ProcessInvalidAssetID()
{
    std::cout << "Test C: Process() skips entity with INVALID_ASSET_ID..." << std::endl;

    Olympe::TaskSystem system;

    // Add a dummy entity to m_entities.
    const EntityID dummyEntity = 42u;
    system.m_entities.insert(dummyEntity);

    // Process() should call AssetManager::GetTaskGraph(INVALID_ASSET_ID) which
    // returns nullptr; the entity should be skipped without crashing.
    system.Process();

    ReportTest("TestC_ProcessInvalidAssetID", true);
}

// ---------------------------------------------------------------------------
// Test d: TaskRunnerComponent default state
// ---------------------------------------------------------------------------

static void TestD_TaskRunnerComponentDefaults()
{
    std::cout << "Test D: TaskRunnerComponent default state..." << std::endl;

    bool passed = true;

    Olympe::TaskRunnerComponent runner;

    TEST_ASSERT(runner.GraphTemplateID == Olympe::INVALID_ASSET_ID,
                "GraphTemplateID should default to INVALID_ASSET_ID");
    if (runner.GraphTemplateID != Olympe::INVALID_ASSET_ID) { passed = false; }

    TEST_ASSERT(runner.CurrentNodeIndex == 0,
                "CurrentNodeIndex should default to 0");
    if (runner.CurrentNodeIndex != 0) { passed = false; }

    TEST_ASSERT(runner.StateTimer == 0.0f,
                "StateTimer should default to 0.0f");
    if (runner.StateTimer != 0.0f) { passed = false; }

    TEST_ASSERT(runner.LocalBlackboardData.empty(),
                "LocalBlackboardData should be empty by default");
    if (!runner.LocalBlackboardData.empty()) { passed = false; }

    TEST_ASSERT(runner.LastStatus == Olympe::TaskRunnerComponent::TaskStatus::Success,
                "LastStatus should default to Success");
    if (runner.LastStatus != Olympe::TaskRunnerComponent::TaskStatus::Success)
    {
        passed = false;
    }

    ReportTest("TestD_TaskRunnerComponentDefaults", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== TaskSystemTest ===" << std::endl;

    TestA_InstantiateTaskSystem();
    TestB_ProcessEmptyEntities();
    TestC_ProcessInvalidAssetID();
    TestD_TaskRunnerComponentDefaults();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
