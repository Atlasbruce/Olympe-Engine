/**
 * @file AtomicTaskRegistryTest.cpp
 * @brief Unit tests for AtomicTaskRegistry and Task_LogMessage (Phase 2.B).
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Tests cover:
 *   a) Manual Register() + IsRegistered() reports the task as registered.
 *   b) Create() returns a non-null unique_ptr for a registered ID.
 *   c) Create() returns nullptr for an unknown ID.
 *   d) Task_LogMessage is auto-registered via REGISTER_ATOMIC_TASK macro.
 *   e) Task_LogMessage::Execute() returns TaskStatus::Success.
 *   f) Task_LogMessage::Execute() succeeds with no parameters.
 *
 * Run from the repository root.
 * No SDL3 or World dependency.
 */

#include "TaskSystem/AtomicTaskRegistry.h"
#include "TaskSystem/IAtomicTask.h"

#include <iostream>
#include <cassert>
#include <string>
#include <memory>

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
// Minimal concrete task used only within these tests
// ---------------------------------------------------------------------------

namespace {

class Task_AlwaysSuccess : public Olympe::IAtomicTask {
public:
    bool wasCalled = false;

    Olympe::TaskStatus Execute(const ParameterMap& /*params*/) override
    {
        wasCalled = true;
        return Olympe::TaskStatus::Success;
    }

    void Abort() override
    {
        // No in-progress state to clean up for this test stub.
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test a: Register() + IsRegistered()
// ---------------------------------------------------------------------------

static void TestA_RegisterAndIsRegistered()
{
    std::cout << "Test A: Register() + IsRegistered()..." << std::endl;

    bool passed = true;

    const std::string id = "Test_AlwaysSuccess_A";

    TEST_ASSERT(!Olympe::AtomicTaskRegistry::Get().IsRegistered(id),
                "Task should not be registered before Register()");
    if (Olympe::AtomicTaskRegistry::Get().IsRegistered(id)) { passed = false; }

    Olympe::AtomicTaskRegistry::Get().Register(id,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_AlwaysSuccess());
        });

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered(id),
                "Task should be registered after Register()");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered(id)) { passed = false; }

    ReportTest("TestA_RegisterAndIsRegistered", passed);
}

// ---------------------------------------------------------------------------
// Test b: Create() returns non-null for registered ID
// ---------------------------------------------------------------------------

static void TestB_CreateReturnsTask()
{
    std::cout << "Test B: Create() returns non-null for registered ID..." << std::endl;

    bool passed = true;

    const std::string id = "Test_AlwaysSuccess_B";
    Olympe::AtomicTaskRegistry::Get().Register(id,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_AlwaysSuccess());
        });

    std::unique_ptr<Olympe::IAtomicTask> task =
        Olympe::AtomicTaskRegistry::Get().Create(id);

    TEST_ASSERT(task != nullptr, "Create() should return non-null for registered ID");
    if (!task) { passed = false; }

    ReportTest("TestB_CreateReturnsTask", passed);
}

// ---------------------------------------------------------------------------
// Test c: Create() returns nullptr for unknown ID
// ---------------------------------------------------------------------------

static void TestC_CreateUnknownReturnsNull()
{
    std::cout << "Test C: Create() returns nullptr for unknown ID..." << std::endl;

    bool passed = true;

    std::unique_ptr<Olympe::IAtomicTask> task =
        Olympe::AtomicTaskRegistry::Get().Create("Task_DoesNotExist_XYZ");

    TEST_ASSERT(task == nullptr, "Create() should return nullptr for unknown ID");
    if (task != nullptr) { passed = false; }

    ReportTest("TestC_CreateUnknownReturnsNull", passed);
}

// ---------------------------------------------------------------------------
// Test d: Task_LogMessage is auto-registered via the macro
// ---------------------------------------------------------------------------

static void TestD_LogMessageAutoRegistered()
{
    std::cout << "Test D: Task_LogMessage auto-registered via macro..." << std::endl;

    bool passed = true;

    TEST_ASSERT(Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_LogMessage"),
                "Task_LogMessage should be registered at static init");
    if (!Olympe::AtomicTaskRegistry::Get().IsRegistered("Task_LogMessage")) { passed = false; }

    ReportTest("TestD_LogMessageAutoRegistered", passed);
}

// ---------------------------------------------------------------------------
// Test e: Task_LogMessage::Execute() returns Success with message param
// ---------------------------------------------------------------------------

static void TestE_LogMessageExecuteSuccess()
{
    std::cout << "Test E: Task_LogMessage::Execute() returns Success..." << std::endl;

    bool passed = true;

    std::unique_ptr<Olympe::IAtomicTask> task =
        Olympe::AtomicTaskRegistry::Get().Create("Task_LogMessage");

    TEST_ASSERT(task != nullptr, "Task_LogMessage should be creatable");
    if (!task) { passed = false; ReportTest("TestE_LogMessageExecuteSuccess", false); return; }

    Olympe::IAtomicTask::ParameterMap params;
    params["message"] = Olympe::TaskValue(std::string("Hello from test"));

    Olympe::TaskStatus status = task->Execute(params);

    TEST_ASSERT(status == Olympe::TaskStatus::Success,
                "Task_LogMessage should return Success");
    if (status != Olympe::TaskStatus::Success) { passed = false; }

    ReportTest("TestE_LogMessageExecuteSuccess", passed);
}

// ---------------------------------------------------------------------------
// Test f: Task_LogMessage::Execute() succeeds with no parameters
// ---------------------------------------------------------------------------

static void TestF_LogMessageNoParams()
{
    std::cout << "Test F: Task_LogMessage::Execute() with no params..." << std::endl;

    bool passed = true;

    std::unique_ptr<Olympe::IAtomicTask> task =
        Olympe::AtomicTaskRegistry::Get().Create("Task_LogMessage");

    TEST_ASSERT(task != nullptr, "Task_LogMessage should be creatable");
    if (!task) { passed = false; ReportTest("TestF_LogMessageNoParams", false); return; }

    // Empty parameter map: task should use default message and return Success.
    Olympe::IAtomicTask::ParameterMap emptyParams;
    Olympe::TaskStatus status = task->Execute(emptyParams);

    TEST_ASSERT(status == Olympe::TaskStatus::Success,
                "Task_LogMessage should return Success even with no params");
    if (status != Olympe::TaskStatus::Success) { passed = false; }

    ReportTest("TestF_LogMessageNoParams", passed);
}

// ---------------------------------------------------------------------------
// Test g: GetAllTaskIDs() returns all registered IDs
// ---------------------------------------------------------------------------

static void TestG_GetAllTaskIDs()
{
    std::cout << "Test G: GetAllTaskIDs() returns all registered IDs..." << std::endl;

    bool passed = true;

    // Register two fresh IDs that are unique to this test
    const std::string idX = "Test_GetAll_X";
    const std::string idY = "Test_GetAll_Y";

    Olympe::AtomicTaskRegistry::Get().Register(idX,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_AlwaysSuccess());
        });
    Olympe::AtomicTaskRegistry::Get().Register(idY,
        []() -> std::unique_ptr<Olympe::IAtomicTask> {
            return std::unique_ptr<Olympe::IAtomicTask>(new Task_AlwaysSuccess());
        });

    std::vector<std::string> ids = Olympe::AtomicTaskRegistry::Get().GetAllTaskIDs();

    // ids must be non-empty
    TEST_ASSERT(!ids.empty(), "GetAllTaskIDs() should return non-empty vector");
    if (ids.empty()) { passed = false; }

    // Both freshly registered IDs must appear in the list
    bool foundX = false;
    bool foundY = false;
    for (const auto& id : ids)
    {
        if (id == idX) foundX = true;
        if (id == idY) foundY = true;
    }
    TEST_ASSERT(foundX, "GetAllTaskIDs() should contain the first registered ID");
    TEST_ASSERT(foundY, "GetAllTaskIDs() should contain the second registered ID");
    if (!foundX || !foundY) { passed = false; }

    // Task_LogMessage (auto-registered) must also be present
    bool foundLogMessage = false;
    for (const auto& id : ids)
    {
        if (id == "Task_LogMessage") { foundLogMessage = true; break; }
    }
    TEST_ASSERT(foundLogMessage, "GetAllTaskIDs() should include auto-registered Task_LogMessage");
    if (!foundLogMessage) { passed = false; }

    ReportTest("TestG_GetAllTaskIDs", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== AtomicTaskRegistryTest ===" << std::endl;

    TestA_RegisterAndIsRegistered();
    TestB_CreateReturnsTask();
    TestC_CreateUnknownReturnsNull();
    TestD_LogMessageAutoRegistered();
    TestE_LogMessageExecuteSuccess();
    TestF_LogMessageNoParams();
    TestG_GetAllTaskIDs();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
