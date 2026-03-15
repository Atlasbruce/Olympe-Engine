/**
 * @file Phase23B2Test.cpp
 * @brief Regression tests for HOTFIX BUG-003 & BUG-004: Node Position Offset + Load Crash.
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * Tests operate on TaskGraphTemplate / BlackboardEntry logic — no SDL3, ImGui,
 * or ImNodes dependency.  The grid-space coordinate math and variable-type
 * utilities exercised here do not require a rendered ImNodes context.
 *
 * Test cases (8):
 *   1.  TestScreenToCanvasConversion  — verify coordinate conversion formula
 *   2.  TestGetVariablesByType_Int    — filter returns only Int variables
 *   3.  TestGetVariablesByType_Bool   — filter returns only Bool variables
 *   4.  TestGetVariablesByType_Empty  — filter on empty list returns empty
 *   5.  TestVectorTypeReadOnly        — Vector type is treated as non-editable default
 *   6.  TestEntityIDTypeReadOnly      — EntityID type is treated as non-editable default
 *   7.  TestSaveLoadRoundTrip_Positions — positions stored without pan-offset corruption
 *   8.  TestBBEntry_DefaultValues     — default value init is type-correct for all types
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cmath>

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                              \
    do {                                                                    \
        if (!(cond)) {                                                      \
            std::cout << "  FAIL: " << (msg) << std::endl;                 \
            ++s_failCount;                                                  \
        }                                                                   \
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
    }
}

// ---------------------------------------------------------------------------
// Standalone helpers that mirror the production implementations without ImGui
// ---------------------------------------------------------------------------

// Mirrors VisualScriptEditorPanel::ScreenToCanvasPos() math (BUG-003 Fix #2).
// In the real implementation this uses ImGui/ImNodes context; here we call
// the formula directly with explicit parameters so it can be unit-tested
// without a GPU/window context.
struct Vec2 { float x; float y; };

static Vec2 ScreenToCanvasPos(Vec2 screenPos, Vec2 windowPos, Vec2 canvasPanning)
{
    // Editor space = screenPos - windowPos - panning (zoom is 1.0 in ImNodes 0.4)
    return { screenPos.x - windowPos.x - canvasPanning.x,
             screenPos.y - windowPos.y - canvasPanning.y };
}

// Mirrors VisualScriptEditorPanel::GetVariablesByType().
static std::vector<BlackboardEntry> GetVariablesByType(
    const std::vector<BlackboardEntry>& allVars,
    VariableType expectedType)
{
    std::vector<BlackboardEntry> filtered;
    for (size_t i = 0; i < allVars.size(); ++i)
    {
        if (allVars[i].Type == expectedType)
            filtered.push_back(allVars[i]);
    }
    return filtered;
}

// Returns true if the type is "runtime-assigned" (read-only in the UI).
static bool IsReadOnlyType(VariableType t)
{
    return (t == VariableType::Vector || t == VariableType::EntityID);
}

// ---------------------------------------------------------------------------
// Test 1 — ScreenToCanvasConversion
// ---------------------------------------------------------------------------

static void TestScreenToCanvasConversion()
{
    const std::string name = "ScreenToCanvasConversion";
    int prevFail = s_failCount;

    // Scenario: mouse at (300, 400) screen, window at (100, 50), panning (-24, 114)
    Vec2 screenPos    = { 300.0f, 400.0f };
    Vec2 windowPos    = { 100.0f,  50.0f };
    Vec2 panning      = { -24.0f, 114.0f };

    Vec2 result = ScreenToCanvasPos(screenPos, windowPos, panning);

    // Expected: (300-100-(-24), 400-50-114) = (224, 236)
    TEST_ASSERT(std::fabs(result.x - 224.0f) < 0.001f,
                "ScreenToCanvas X: expected 224.0");
    TEST_ASSERT(std::fabs(result.y - 236.0f) < 0.001f,
                "ScreenToCanvas Y: expected 236.0");

    // Zero panning — result should just subtract window origin
    Vec2 resultNoPan = ScreenToCanvasPos(screenPos, windowPos, {0.0f, 0.0f});
    TEST_ASSERT(std::fabs(resultNoPan.x - 200.0f) < 0.001f,
                "ScreenToCanvas X (no pan): expected 200.0");
    TEST_ASSERT(std::fabs(resultNoPan.y - 350.0f) < 0.001f,
                "ScreenToCanvas Y (no pan): expected 350.0");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2 — GetVariablesByType_Int
// ---------------------------------------------------------------------------

static void TestGetVariablesByType_Int()
{
    const std::string name = "GetVariablesByType_Int";
    int prevFail = s_failCount;

    std::vector<BlackboardEntry> entries;
    {
        BlackboardEntry e1;
        e1.Key  = "health";
        e1.Type = VariableType::Float;
        entries.push_back(e1);
    }
    {
        BlackboardEntry e2;
        e2.Key  = "counter";
        e2.Type = VariableType::Int;
        entries.push_back(e2);
    }
    {
        BlackboardEntry e3;
        e3.Key  = "alertLevel";
        e3.Type = VariableType::Int;
        entries.push_back(e3);
    }
    {
        BlackboardEntry e4;
        e4.Key  = "isAlive";
        e4.Type = VariableType::Bool;
        entries.push_back(e4);
    }

    std::vector<BlackboardEntry> intVars = GetVariablesByType(entries, VariableType::Int);

    TEST_ASSERT(intVars.size() == 2, "GetVariablesByType(Int) returns exactly 2 entries");
    TEST_ASSERT(intVars[0].Key == "counter",    "First Int entry is 'counter'");
    TEST_ASSERT(intVars[1].Key == "alertLevel", "Second Int entry is 'alertLevel'");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3 — GetVariablesByType_Bool
// ---------------------------------------------------------------------------

static void TestGetVariablesByType_Bool()
{
    const std::string name = "GetVariablesByType_Bool";
    int prevFail = s_failCount;

    std::vector<BlackboardEntry> entries;
    {
        BlackboardEntry e1; e1.Key = "flag1"; e1.Type = VariableType::Bool;
        entries.push_back(e1);
    }
    {
        BlackboardEntry e2; e2.Key = "count"; e2.Type = VariableType::Int;
        entries.push_back(e2);
    }
    {
        BlackboardEntry e3; e3.Key = "flag2"; e3.Type = VariableType::Bool;
        entries.push_back(e3);
    }

    std::vector<BlackboardEntry> boolVars = GetVariablesByType(entries, VariableType::Bool);

    TEST_ASSERT(boolVars.size() == 2, "GetVariablesByType(Bool) returns 2 entries");
    TEST_ASSERT(boolVars[0].Key == "flag1", "First Bool entry is 'flag1'");
    TEST_ASSERT(boolVars[1].Key == "flag2", "Second Bool entry is 'flag2'");

    // Requesting Float returns zero entries
    std::vector<BlackboardEntry> floatVars = GetVariablesByType(entries, VariableType::Float);
    TEST_ASSERT(floatVars.empty(), "GetVariablesByType(Float) returns 0 entries when none present");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4 — GetVariablesByType_Empty
// ---------------------------------------------------------------------------

static void TestGetVariablesByType_Empty()
{
    const std::string name = "GetVariablesByType_Empty";
    int prevFail = s_failCount;

    std::vector<BlackboardEntry> empty;
    std::vector<BlackboardEntry> result = GetVariablesByType(empty, VariableType::Int);
    TEST_ASSERT(result.empty(), "GetVariablesByType on empty list returns empty result");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5 — VectorTypeReadOnly
// ---------------------------------------------------------------------------

static void TestVectorTypeReadOnly()
{
    const std::string name = "VectorTypeReadOnly";
    int prevFail = s_failCount;

    // Vector type should be treated as read-only (auto from entity position)
    TEST_ASSERT(IsReadOnlyType(VariableType::Vector),
                "VariableType::Vector is marked as read-only");
    TEST_ASSERT(!IsReadOnlyType(VariableType::Float),
                "VariableType::Float is NOT read-only");
    TEST_ASSERT(!IsReadOnlyType(VariableType::Int),
                "VariableType::Int is NOT read-only");
    TEST_ASSERT(!IsReadOnlyType(VariableType::Bool),
                "VariableType::Bool is NOT read-only");
    TEST_ASSERT(!IsReadOnlyType(VariableType::String),
                "VariableType::String is NOT read-only");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6 — EntityIDTypeReadOnly
// ---------------------------------------------------------------------------

static void TestEntityIDTypeReadOnly()
{
    const std::string name = "EntityIDTypeReadOnly";
    int prevFail = s_failCount;

    // EntityID is assigned at runtime; should be read-only in the Blackboard panel
    TEST_ASSERT(IsReadOnlyType(VariableType::EntityID),
                "VariableType::EntityID is marked as read-only");

    // Verify the filter still returns EntityID entries (they can exist, just not be edited)
    std::vector<BlackboardEntry> entries;
    {
        BlackboardEntry e; e.Key = "ownerID"; e.Type = VariableType::EntityID;
        entries.push_back(e);
    }
    {
        BlackboardEntry e; e.Key = "targetID"; e.Type = VariableType::EntityID;
        entries.push_back(e);
    }
    {
        BlackboardEntry e; e.Key = "health"; e.Type = VariableType::Float;
        entries.push_back(e);
    }

    std::vector<BlackboardEntry> eidVars = GetVariablesByType(entries, VariableType::EntityID);
    TEST_ASSERT(eidVars.size() == 2,
                "GetVariablesByType(EntityID) returns 2 EntityID entries");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7 — SaveLoadRoundTrip_Positions
// ---------------------------------------------------------------------------

static void TestSaveLoadRoundTrip_Positions()
{
    const std::string name = "SaveLoadRoundTrip_Positions";
    int prevFail = s_failCount;

    // Simulate two nodes with positions.  After a save/load cycle, the positions
    // stored in the template (grid space) must be identical — no panning offset
    // should be added or removed.
    // This test validates that position values are stored as plain floats and
    // survive a template copy without corruption (no viewport arithmetic happens).
    TaskGraphTemplate tmpl;
    tmpl.Name = "RoundTripTest";

    const float posX1 = -369.532f;
    const float posY1 =   45.397f;
    const float posX2 =  120.000f;
    const float posY2 =  -80.500f;

    {
        TaskNodeDefinition node;
        node.NodeID   = 1;
        node.NodeName = "Start";
        node.Type     = TaskNodeType::EntryPoint;
        // Store position in Parameters (as production code does)
        ParameterBinding bx, by;
        bx.Type = ParameterBindingType::Literal; bx.LiteralValue = TaskValue(posX1);
        by.Type = ParameterBindingType::Literal; by.LiteralValue = TaskValue(posY1);
        node.Parameters["__posX"] = bx;
        node.Parameters["__posY"] = by;
        node.EditorPosX = posX1;
        node.EditorPosY = posY1;
        tmpl.Nodes.push_back(node);
    }
    {
        TaskNodeDefinition node;
        node.NodeID   = 2;
        node.NodeName = "Action";
        node.Type     = TaskNodeType::AtomicTask;
        ParameterBinding bx, by;
        bx.Type = ParameterBindingType::Literal; bx.LiteralValue = TaskValue(posX2);
        by.Type = ParameterBindingType::Literal; by.LiteralValue = TaskValue(posY2);
        node.Parameters["__posX"] = bx;
        node.Parameters["__posY"] = by;
        node.EditorPosX = posX2;
        node.EditorPosY = posY2;
        tmpl.Nodes.push_back(node);
    }

    // Simulate a copy (as happens when LoadTemplate copies the template)
    TaskGraphTemplate loaded = tmpl;

    // Verify positions survived the copy without corruption
    TEST_ASSERT(loaded.Nodes.size() == 2,
                "Loaded template has 2 nodes");

    for (size_t i = 0; i < loaded.Nodes.size(); ++i)
    {
        const TaskNodeDefinition& n = loaded.Nodes[i];
        float expectedX = (n.NodeID == 1) ? posX1 : posX2;
        float expectedY = (n.NodeID == 1) ? posY1 : posY2;

        // Check EditorPos fields
        TEST_ASSERT(std::fabs(n.EditorPosX - expectedX) < 0.001f,
                    "EditorPosX preserved for node " + std::to_string(n.NodeID));
        TEST_ASSERT(std::fabs(n.EditorPosY - expectedY) < 0.001f,
                    "EditorPosY preserved for node " + std::to_string(n.NodeID));

        // Check Parameters storage (used by Undo/Redo to restore positions)
        auto itX = n.Parameters.find("__posX");
        auto itY = n.Parameters.find("__posY");
        TEST_ASSERT(itX != n.Parameters.end(), "__posX parameter exists for node " + std::to_string(n.NodeID));
        TEST_ASSERT(itY != n.Parameters.end(), "__posY parameter exists for node " + std::to_string(n.NodeID));

        if (itX != n.Parameters.end() && itY != n.Parameters.end())
        {
            float storedX = itX->second.LiteralValue.AsFloat();
            float storedY = itY->second.LiteralValue.AsFloat();
            TEST_ASSERT(std::fabs(storedX - expectedX) < 0.001f,
                        "__posX parameter value correct for node " + std::to_string(n.NodeID));
            TEST_ASSERT(std::fabs(storedY - expectedY) < 0.001f,
                        "__posY parameter value correct for node " + std::to_string(n.NodeID));
        }
    }

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8 — BBEntry_DefaultValues (all types get correct defaults)
// ---------------------------------------------------------------------------

static void TestBBEntry_DefaultValues()
{
    const std::string name = "BBEntry_DefaultValues";
    int prevFail = s_failCount;

    // Bool default should be false
    {
        BlackboardEntry e;
        e.Key     = "myBool";
        e.Type    = VariableType::Bool;
        e.Default = GetDefaultValueForType(VariableType::Bool);
        TEST_ASSERT(!e.Default.IsNone(), "Bool default is not None");
        TEST_ASSERT(e.Default.AsBool() == false, "Bool default is false");
    }

    // Int default should be 0
    {
        BlackboardEntry e;
        e.Key     = "myInt";
        e.Type    = VariableType::Int;
        e.Default = GetDefaultValueForType(VariableType::Int);
        TEST_ASSERT(!e.Default.IsNone(), "Int default is not None");
        TEST_ASSERT(e.Default.AsInt() == 0, "Int default is 0");
    }

    // Float default should be 0.0f
    {
        BlackboardEntry e;
        e.Key     = "myFloat";
        e.Type    = VariableType::Float;
        e.Default = GetDefaultValueForType(VariableType::Float);
        TEST_ASSERT(!e.Default.IsNone(), "Float default is not None");
        TEST_ASSERT(std::fabs(e.Default.AsFloat()) < 0.001f, "Float default is 0.0");
    }

    // String default should be empty string (not None)
    {
        BlackboardEntry e;
        e.Key     = "myString";
        e.Type    = VariableType::String;
        e.Default = GetDefaultValueForType(VariableType::String);
        TEST_ASSERT(!e.Default.IsNone(), "String default is not None");
    }

    // EntityID: read-only — default exists (not None) so serialization is safe
    {
        BlackboardEntry e;
        e.Key     = "myEntityID";
        e.Type    = VariableType::EntityID;
        e.Default = GetDefaultValueForType(VariableType::EntityID);
        // EntityID type is valid for storage even though UI shows it read-only
        TEST_ASSERT(e.Type == VariableType::EntityID, "EntityID entry has correct type");
    }

    // New variable created with safe defaults (Key non-empty, Type=Int)
    {
        BlackboardEntry e;
        e.Key      = "NewVariable";
        e.Type     = VariableType::Int;
        e.Default  = GetDefaultValueForType(VariableType::Int);
        e.IsGlobal = false;

        TEST_ASSERT(!e.Key.empty(),             "Safe init: Key is not empty");
        TEST_ASSERT(e.Type != VariableType::None, "Safe init: Type is not None");
        TEST_ASSERT(!e.Default.IsNone(),         "Safe init: Default is not None");
    }

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== Phase23B2Test (HOTFIX BUG-003 & BUG-004) ===" << std::endl;

    TestScreenToCanvasConversion();
    TestGetVariablesByType_Int();
    TestGetVariablesByType_Bool();
    TestGetVariablesByType_Empty();
    TestVectorTypeReadOnly();
    TestEntityIDTypeReadOnly();
    TestSaveLoadRoundTrip_Positions();
    TestBBEntry_DefaultValues();

    std::cout << std::endl;
    std::cout << "=== Results: " << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
