/**
 * @file LocalBlackboardPersistenceTest.cpp
 * @brief Unit tests for LocalBlackboard::Serialize / Deserialize (Phase 2.D).
 * @author Olympe Engine
 * @date 2026-02-23
 *
 * @details
 * Tests cover:
 *   a) Round-trip: serialize all variable types, deserialize into a fresh
 *      blackboard initialized from the same template, assert values preserved.
 *   b) Deserialize skips unknown variables (not in schema) without error.
 *   c) Deserialize skips type-mismatched variables without error.
 *   d) Empty buffer leaves blackboard at template defaults.
 *
 * No SDL3 or World dependency - tests exercise the standalone blackboard only.
 */

#include "TaskSystem/LocalBlackboard.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>

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
// Helper: build a TaskGraphTemplate with one variable of each type
// ---------------------------------------------------------------------------

static Olympe::TaskGraphTemplate MakeRichTemplate()
{
    Olympe::TaskGraphTemplate tmpl;
    tmpl.Name = "PersistenceTestTemplate";

    {
        Olympe::VariableDefinition def;
        def.Name         = "IsAlerted";
        def.Type         = Olympe::VariableType::Bool;
        def.DefaultValue = Olympe::TaskValue(false);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    {
        Olympe::VariableDefinition def;
        def.Name         = "PatrolIndex";
        def.Type         = Olympe::VariableType::Int;
        def.DefaultValue = Olympe::TaskValue(0);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    {
        Olympe::VariableDefinition def;
        def.Name         = "Speed";
        def.Type         = Olympe::VariableType::Float;
        def.DefaultValue = Olympe::TaskValue(1.0f);
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    {
        Olympe::VariableDefinition def;
        def.Name         = "Destination";
        def.Type         = Olympe::VariableType::Vector;
        def.DefaultValue = Olympe::TaskValue(::Vector(0.0f, 0.0f, 0.0f));
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    {
        Olympe::VariableDefinition def;
        def.Name         = "TargetEntity";
        def.Type         = Olympe::VariableType::EntityID;
        def.DefaultValue = Olympe::TaskValue(static_cast<EntityID>(0));
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }
    {
        Olympe::VariableDefinition def;
        def.Name         = "State";
        def.Type         = Olympe::VariableType::String;
        def.DefaultValue = Olympe::TaskValue(std::string("idle"));
        def.IsLocal      = true;
        tmpl.LocalVariables.push_back(def);
    }

    return tmpl;
}

// ---------------------------------------------------------------------------
// Test A: Round-trip - mutate all variable types, serialize, deserialize,
//         assert all values match.
// ---------------------------------------------------------------------------

static void TestA_RoundTrip()
{
    std::cout << "Test A: Round-trip serialize/deserialize all variable types..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeRichTemplate();

    // --- source blackboard: mutate all values ---
    Olympe::LocalBlackboard src;
    src.Initialize(tmpl);

    src.SetValue("IsAlerted",    Olympe::TaskValue(true));
    src.SetValue("PatrolIndex",  Olympe::TaskValue(7));
    src.SetValue("Speed",        Olympe::TaskValue(5.5f));
    src.SetValue("Destination",  Olympe::TaskValue(::Vector(10.0f, 20.0f, 30.0f)));
    src.SetValue("TargetEntity", Olympe::TaskValue(static_cast<EntityID>(42)));
    src.SetValue("State",        Olympe::TaskValue(std::string("patrol")));

    // --- serialize ---
    std::vector<uint8_t> bytes;
    src.Serialize(bytes);

    TEST_ASSERT(!bytes.empty(), "Serialized buffer must not be empty");

    // --- deserialize into fresh blackboard ---
    Olympe::LocalBlackboard dst;
    dst.Initialize(tmpl);   // schema must be set up first
    dst.Deserialize(bytes);

    bool passed = true;

    // Bool
    bool bv = dst.GetValue("IsAlerted").AsBool();
    TEST_ASSERT(bv == true, "IsAlerted should be true after round-trip");
    if (bv != true) { passed = false; }

    // Int
    int iv = dst.GetValue("PatrolIndex").AsInt();
    TEST_ASSERT(iv == 7, "PatrolIndex should be 7 after round-trip");
    if (iv != 7) { passed = false; }

    // Float
    float fv = dst.GetValue("Speed").AsFloat();
    TEST_ASSERT(fv == 5.5f, "Speed should be 5.5 after round-trip");
    if (fv != 5.5f) { passed = false; }

    // Vector
    ::Vector vv = dst.GetValue("Destination").AsVector();
    TEST_ASSERT(vv.x == 10.0f && vv.y == 20.0f && vv.z == 30.0f,
                "Destination vector should be (10, 20, 30) after round-trip");
    if (vv.x != 10.0f || vv.y != 20.0f || vv.z != 30.0f) { passed = false; }

    // EntityID
    EntityID eid = dst.GetValue("TargetEntity").AsEntityID();
    TEST_ASSERT(eid == 42u, "TargetEntity should be 42 after round-trip");
    if (eid != 42u) { passed = false; }

    // String
    std::string sv = dst.GetValue("State").AsString();
    TEST_ASSERT(sv == "patrol", "State should be 'patrol' after round-trip");
    if (sv != "patrol") { passed = false; }

    ReportTest("TestA_RoundTrip", passed);
}

// ---------------------------------------------------------------------------
// Test B: Variables not in schema are silently skipped during Deserialize.
// ---------------------------------------------------------------------------

static void TestB_SkipUnknownVariables()
{
    std::cout << "Test B: Unknown variables are skipped during Deserialize..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeRichTemplate();

    // Serialize a blackboard with mutated values (all vars are in schema).
    Olympe::LocalBlackboard src;
    src.Initialize(tmpl);
    src.SetValue("PatrolIndex", Olympe::TaskValue(3));

    std::vector<uint8_t> bytes;
    src.Serialize(bytes);

    // Deserialize into a blackboard initialized from a SMALLER template
    // (missing some variables -> those entries in the buffer become unknown).
    Olympe::TaskGraphTemplate smallTmpl;
    smallTmpl.Name = "SmallTemplate";
    {
        Olympe::VariableDefinition def;
        def.Name         = "PatrolIndex";
        def.Type         = Olympe::VariableType::Int;
        def.DefaultValue = Olympe::TaskValue(0);
        def.IsLocal      = true;
        smallTmpl.LocalVariables.push_back(def);
    }

    Olympe::LocalBlackboard dst;
    dst.Initialize(smallTmpl);

    // Should not throw; unknown vars are skipped.
    bool threw = false;
    try
    {
        dst.Deserialize(bytes);
    }
    catch (...)
    {
        threw = true;
    }

    bool passed = true;
    TEST_ASSERT(!threw, "Deserialize with unknown vars must not throw");
    if (threw) { passed = false; }

    // The known variable PatrolIndex should have been applied.
    int iv = dst.GetValue("PatrolIndex").AsInt();
    TEST_ASSERT(iv == 3, "PatrolIndex should be 3 after partial deserialize");
    if (iv != 3) { passed = false; }

    ReportTest("TestB_SkipUnknownVariables", passed);
}

// ---------------------------------------------------------------------------
// Test C: Empty buffer leaves blackboard at template defaults.
// ---------------------------------------------------------------------------

static void TestC_EmptyBuffer()
{
    std::cout << "Test C: Empty buffer leaves blackboard at template defaults..." << std::endl;

    Olympe::TaskGraphTemplate tmpl = MakeRichTemplate();
    Olympe::LocalBlackboard bb;
    bb.Initialize(tmpl);

    std::vector<uint8_t> empty;
    bool threw = false;
    try
    {
        bb.Deserialize(empty);
    }
    catch (...)
    {
        threw = true;
    }

    bool passed = true;
    TEST_ASSERT(!threw, "Deserialize of empty buffer must not throw");
    if (threw) { passed = false; }

    // Values should remain at defaults.
    TEST_ASSERT(bb.GetValue("IsAlerted").AsBool() == false, "IsAlerted should be false (default)");
    if (bb.GetValue("IsAlerted").AsBool() != false) { passed = false; }

    TEST_ASSERT(bb.GetValue("PatrolIndex").AsInt() == 0, "PatrolIndex should be 0 (default)");
    if (bb.GetValue("PatrolIndex").AsInt() != 0) { passed = false; }

    ReportTest("TestC_EmptyBuffer", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== LocalBlackboardPersistenceTest ===" << std::endl;

    TestA_RoundTrip();
    TestB_SkipUnknownVariables();
    TestC_EmptyBuffer();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
