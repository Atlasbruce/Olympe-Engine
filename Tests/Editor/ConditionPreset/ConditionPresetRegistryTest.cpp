/**
 * @file ConditionPresetRegistryTest.cpp
 * @brief Unit tests for ConditionPresetRegistry (Phase 24.0).
 * @author Olympe Engine
 * @date 2026-03-16
 *
 * @details
 * Tests (12):
 *   1.  CreatePreset          — preset is stored, ID is returned
 *   2.  GetPreset             — mutable and const lookup work correctly
 *   3.  UpdatePreset          — data is replaced without changing ID
 *   4.  DeletePreset          — preset is removed from registry
 *   5.  DuplicatePreset       — copy is created with new UUID and " (Copy)" name
 *   6.  GetAllPresetIDs       — returns IDs in insertion order
 *   7.  ValidatePresetID      — returns true for existing, false for unknown
 *   8.  LoadFromJSON          — parses a well-formed JSON file correctly
 *   9.  SaveToJSON            — writes JSON that can be loaded back
 *   10. HandleMissingPreset   — GetPreset returns nullptr for unknown ID
 *   11. HandleDuplicateID     — CreatePreset with existing ID is a no-op
 *   12. RoundTripPersistence  — Save then Load preserves all preset data
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "Editor/ConditionPreset/ConditionPresetRegistry.h"
#include "Editor/ConditionPreset/ConditionPreset.h"
#include "Editor/ConditionPreset/Operand.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>  // std::remove

using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure
// ---------------------------------------------------------------------------

static int s_passCount = 0;
static int s_failCount = 0;

#define TEST_ASSERT(cond, msg)                                          \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::cout << "  FAIL: " << (msg) << std::endl;             \
            ++s_failCount;                                              \
        }                                                               \
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

/// Helper: builds a simple ConditionPreset with given ID and name.
static ConditionPreset MakePreset(const std::string& id, const std::string& name)
{
    ConditionPreset p(id,
                      Operand::CreateVariable("mHealth"),
                      ComparisonOp::LessEqual,
                      Operand::CreateConst(2.0));
    p.name = name;
    return p;
}

// ---------------------------------------------------------------------------
// Test 1: CreatePreset
// ---------------------------------------------------------------------------

static void Test1_CreatePreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    ConditionPreset p = MakePreset("p1", "Condition #1");
    std::string id = reg.CreatePreset(p);

    TEST_ASSERT(id == "p1",              "CreatePreset: returned ID should be 'p1'");
    TEST_ASSERT(reg.GetPresetCount() == 1, "CreatePreset: count should be 1");
    TEST_ASSERT(reg.ValidatePresetID("p1"), "CreatePreset: ID 'p1' should be valid");

    ok = (id == "p1") && (reg.GetPresetCount() == 1);
    ReportTest("CreatePreset", ok);
}

// ---------------------------------------------------------------------------
// Test 2: GetPreset
// ---------------------------------------------------------------------------

static void Test2_GetPreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p2", "Condition #2"));

    const ConditionPresetRegistry& cRef = reg;

    ConditionPreset* p = reg.GetPreset("p2");
    const ConditionPreset* cp = cRef.GetPreset("p2");

    TEST_ASSERT(p  != nullptr,        "GetPreset (mutable): should not be nullptr");
    TEST_ASSERT(cp != nullptr,        "GetPreset (const):   should not be nullptr");
    TEST_ASSERT(p->name == "Condition #2", "GetPreset: name should match");

    ok = (p != nullptr) && (cp != nullptr) && (p->name == "Condition #2");
    ReportTest("GetPreset", ok);
}

// ---------------------------------------------------------------------------
// Test 3: UpdatePreset
// ---------------------------------------------------------------------------

static void Test3_UpdatePreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p3", "Old Name"));

    ConditionPreset updated = MakePreset("p3", "New Name");
    reg.UpdatePreset("p3", updated);

    const ConditionPreset* p = reg.GetPreset("p3");
    TEST_ASSERT(p != nullptr,          "UpdatePreset: preset should still exist");
    TEST_ASSERT(p->name == "New Name", "UpdatePreset: name should be updated");
    TEST_ASSERT(p->id == "p3",         "UpdatePreset: id should remain unchanged");

    ok = (p != nullptr) && (p->name == "New Name") && (p->id == "p3");
    ReportTest("UpdatePreset", ok);
}

// ---------------------------------------------------------------------------
// Test 4: DeletePreset
// ---------------------------------------------------------------------------

static void Test4_DeletePreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p4", "To Delete"));
    TEST_ASSERT(reg.GetPresetCount() == 1, "Before delete: count should be 1");

    reg.DeletePreset("p4");
    TEST_ASSERT(reg.GetPresetCount() == 0,  "After delete: count should be 0");
    TEST_ASSERT(reg.GetPreset("p4") == nullptr, "After delete: GetPreset should return nullptr");

    // Deleting a non-existent ID should be a no-op (no crash)
    reg.DeletePreset("nonexistent");
    TEST_ASSERT(reg.GetPresetCount() == 0, "After no-op delete: count should remain 0");

    ok = (reg.GetPresetCount() == 0) && (reg.GetPreset("p4") == nullptr);
    ReportTest("DeletePreset", ok);
}

// ---------------------------------------------------------------------------
// Test 5: DuplicatePreset
// ---------------------------------------------------------------------------

static void Test5_DuplicatePreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("p5", "Original"));

    std::string newID = reg.DuplicatePreset("p5");
    TEST_ASSERT(!newID.empty(),           "DuplicatePreset: new ID should not be empty");
    TEST_ASSERT(newID != "p5",            "DuplicatePreset: new ID should differ from source");
    TEST_ASSERT(reg.GetPresetCount() == 2, "DuplicatePreset: count should be 2");

    const ConditionPreset* copy = reg.GetPreset(newID);
    TEST_ASSERT(copy != nullptr,              "DuplicatePreset: copy should exist");
    TEST_ASSERT(copy->name == "Original (Copy)", "DuplicatePreset: name should have ' (Copy)' suffix");

    ok = !newID.empty() && (newID != "p5") && (reg.GetPresetCount() == 2)
      && (copy != nullptr) && (copy->name == "Original (Copy)");
    ReportTest("DuplicatePreset", ok);
}

// ---------------------------------------------------------------------------
// Test 6: GetAllPresetIDs
// ---------------------------------------------------------------------------

static void Test6_GetAllPresetIDs()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("a", "A"));
    reg.CreatePreset(MakePreset("b", "B"));
    reg.CreatePreset(MakePreset("c", "C"));

    std::vector<std::string> ids = reg.GetAllPresetIDs();
    TEST_ASSERT(ids.size() == 3, "GetAllPresetIDs: should return 3 IDs");
    TEST_ASSERT(ids[0] == "a",   "GetAllPresetIDs: first should be 'a'");
    TEST_ASSERT(ids[1] == "b",   "GetAllPresetIDs: second should be 'b'");
    TEST_ASSERT(ids[2] == "c",   "GetAllPresetIDs: third should be 'c'");

    ok = (ids.size() == 3) && (ids[0] == "a") && (ids[1] == "b") && (ids[2] == "c");
    ReportTest("GetAllPresetIDs", ok);
}

// ---------------------------------------------------------------------------
// Test 7: ValidatePresetID
// ---------------------------------------------------------------------------

static void Test7_ValidatePresetID()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("valid_id", "Valid"));

    TEST_ASSERT( reg.ValidatePresetID("valid_id"),    "ValidatePresetID: existing ID should return true");
    TEST_ASSERT(!reg.ValidatePresetID("bogus_id"),    "ValidatePresetID: unknown ID should return false");
    TEST_ASSERT(!reg.ValidatePresetID(""),            "ValidatePresetID: empty ID should return false");

    ok = reg.ValidatePresetID("valid_id") && !reg.ValidatePresetID("bogus_id");
    ReportTest("ValidatePresetID", ok);
}

// ---------------------------------------------------------------------------
// Test 8: LoadFromJSON
// ---------------------------------------------------------------------------

static void Test8_LoadFromJSON()
{
    bool ok = true;

    // Write a minimal JSON file
    const std::string path = "/tmp/test_condition_presets_load.json";
    {
        std::ofstream ofs(path.c_str());
        ofs << R"({
  "version": 1,
  "presets": [
    {
      "id": "preset_001",
      "name": "Condition #1",
      "left":  { "mode": "Variable", "value": "mHealth" },
      "operator": "<=",
      "right": { "mode": "Const", "value": 2.0 }
    },
    {
      "id": "preset_005",
      "name": "Condition #5",
      "left":  { "mode": "Variable", "value": "mSpeed" },
      "operator": "==",
      "right": { "mode": "Pin", "value": "Pin:1" }
    }
  ]
})";
    }

    ConditionPresetRegistry reg;
    bool loaded = reg.Load(path);

    TEST_ASSERT(loaded,                      "Load should succeed");
    TEST_ASSERT(reg.GetPresetCount() == 2,   "Load: should have 2 presets");
    TEST_ASSERT(reg.ValidatePresetID("preset_001"), "Load: preset_001 should exist");
    TEST_ASSERT(reg.ValidatePresetID("preset_005"), "Load: preset_005 should exist");

    const ConditionPreset* p1 = reg.GetPreset("preset_001");
    TEST_ASSERT(p1 != nullptr,                    "Load: preset_001 pointer should not be null");
    if (p1)
    {
        TEST_ASSERT(p1->name == "Condition #1",   "Load: preset_001 name should match");
        TEST_ASSERT(p1->left.IsVariable(),         "Load: preset_001 left should be Variable");
        TEST_ASSERT(p1->op == ComparisonOp::LessEqual, "Load: preset_001 op should be <=");
        TEST_ASSERT(p1->right.IsConst(),           "Load: preset_001 right should be Const");
    }

    std::remove(path.c_str());

    ok = loaded && (reg.GetPresetCount() == 2);
    ReportTest("LoadFromJSON", ok);
}

// ---------------------------------------------------------------------------
// Test 9: SaveToJSON
// ---------------------------------------------------------------------------

static void Test9_SaveToJSON()
{
    bool ok = true;

    const std::string path = "/tmp/test_condition_presets_save.json";

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("s1", "Save Test"));

    bool saved = reg.Save(path);
    TEST_ASSERT(saved, "Save should succeed");

    // Verify the file exists and contains expected content
    {
        std::ifstream ifs(path.c_str());
        TEST_ASSERT(ifs.is_open(), "Saved file should be openable");
        if (ifs.is_open())
        {
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            TEST_ASSERT(content.find("s1")           != std::string::npos, "Saved JSON should contain 's1'");
            TEST_ASSERT(content.find("Save Test")    != std::string::npos, "Saved JSON should contain 'Save Test'");
            TEST_ASSERT(content.find("\"version\"")  != std::string::npos, "Saved JSON should contain 'version'");
            TEST_ASSERT(content.find("\"presets\"")  != std::string::npos, "Saved JSON should contain 'presets'");
        }
    }

    std::remove(path.c_str());

    ok = saved;
    ReportTest("SaveToJSON", ok);
}

// ---------------------------------------------------------------------------
// Test 10: HandleMissingPreset
// ---------------------------------------------------------------------------

static void Test10_HandleMissingPreset()
{
    bool ok = true;

    ConditionPresetRegistry reg;

    ConditionPreset* p = reg.GetPreset("does_not_exist");
    TEST_ASSERT(p == nullptr, "GetPreset for missing ID should return nullptr");

    // DuplicatePreset on missing ID should return empty string
    std::string dupID = reg.DuplicatePreset("does_not_exist");
    TEST_ASSERT(dupID.empty(), "DuplicatePreset for missing ID should return empty string");

    ok = (p == nullptr) && dupID.empty();
    ReportTest("HandleMissingPreset", ok);
}

// ---------------------------------------------------------------------------
// Test 11: HandleDuplicateID
// ---------------------------------------------------------------------------

static void Test11_HandleDuplicateID()
{
    bool ok = true;

    ConditionPresetRegistry reg;
    reg.CreatePreset(MakePreset("dup_id", "Original"));

    // Creating another preset with the same ID should be a no-op
    ConditionPreset p2 = MakePreset("dup_id", "Should Not Replace");
    std::string id = reg.CreatePreset(p2);

    TEST_ASSERT(id == "dup_id",           "Duplicate CreatePreset: returned ID should be 'dup_id'");
    TEST_ASSERT(reg.GetPresetCount() == 1, "Duplicate CreatePreset: count should remain 1");

    const ConditionPreset* stored = reg.GetPreset("dup_id");
    TEST_ASSERT(stored != nullptr,              "Duplicate CreatePreset: stored preset should exist");
    TEST_ASSERT(stored->name == "Original",     "Duplicate CreatePreset: original name should be preserved");

    ok = (id == "dup_id") && (reg.GetPresetCount() == 1)
      && (stored != nullptr) && (stored->name == "Original");
    ReportTest("HandleDuplicateID", ok);
}

// ---------------------------------------------------------------------------
// Test 12: RoundTripPersistence
// ---------------------------------------------------------------------------

static void Test12_RoundTripPersistence()
{
    bool ok = true;

    const std::string path = "/tmp/test_condition_presets_roundtrip.json";

    // Build registry and save
    ConditionPresetRegistry reg;
    {
        ConditionPreset p1("rt1",
                           Operand::CreateVariable("mHealth"),
                           ComparisonOp::LessEqual,
                           Operand::CreateConst(2.0));
        p1.name = "Health Check";

        ConditionPreset p2("rt2",
                           Operand::CreateVariable("mSpeed"),
                           ComparisonOp::Equal,
                           Operand::CreatePin("Pin:1"));
        p2.name = "Speed Check";

        reg.CreatePreset(p1);
        reg.CreatePreset(p2);
    }
    bool saved = reg.Save(path);
    TEST_ASSERT(saved, "RoundTrip: Save should succeed");

    // Load into new registry
    ConditionPresetRegistry reg2;
    bool loaded = reg2.Load(path);
    TEST_ASSERT(loaded,                       "RoundTrip: Load should succeed");
    TEST_ASSERT(reg2.GetPresetCount() == 2,   "RoundTrip: should have 2 presets");

    const ConditionPreset* p1 = reg2.GetPreset("rt1");
    const ConditionPreset* p2 = reg2.GetPreset("rt2");

    TEST_ASSERT(p1 != nullptr,                  "RoundTrip: rt1 should exist");
    TEST_ASSERT(p2 != nullptr,                  "RoundTrip: rt2 should exist");

    if (p1)
    {
        TEST_ASSERT(p1->name == "Health Check", "RoundTrip: rt1 name should match");
        TEST_ASSERT(p1->left.IsVariable(),      "RoundTrip: rt1 left should be Variable");
        TEST_ASSERT(p1->right.IsConst(),        "RoundTrip: rt1 right should be Const");
    }
    if (p2)
    {
        TEST_ASSERT(p2->name == "Speed Check",  "RoundTrip: rt2 name should match");
        TEST_ASSERT(p2->right.IsPin(),          "RoundTrip: rt2 right should be Pin");
    }

    std::remove(path.c_str());

    ok = saved && loaded && (reg2.GetPresetCount() == 2)
      && (p1 != nullptr) && (p2 != nullptr);
    ReportTest("RoundTripPersistence", ok);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== ConditionPresetRegistryTest (Phase 24.0) ===" << std::endl;

    Test1_CreatePreset();
    Test2_GetPreset();
    Test3_UpdatePreset();
    Test4_DeletePreset();
    Test5_DuplicatePreset();
    Test6_GetAllPresetIDs();
    Test7_ValidatePresetID();
    Test8_LoadFromJSON();
    Test9_SaveToJSON();
    Test10_HandleMissingPreset();
    Test11_HandleDuplicateID();
    Test12_RoundTripPersistence();

    std::cout << std::endl
              << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed." << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
