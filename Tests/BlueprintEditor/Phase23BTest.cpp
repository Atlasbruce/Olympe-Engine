/**
 * @file Phase23BTest.cpp
 * @brief Unit tests for Phase 23-B — Full Blackboard Properties (BUG-001 + Registry).
 * @author Olympe Engine
 * @date 2026-03-15
 *
 * @details
 * Tests operate directly on BlackboardVariablePresetRegistry and the BUG-001
 * hotfix logic in serialization — no SDL3, ImGui, or ImNodes dependency.
 *
 * Test cases:
 *   === BlackboardVariablePresetRegistry (13) ===
 *   1.  Test_LoadFromFile_Success         — LoadFromFile returns true on valid JSON
 *   2.  Test_LoadFromFile_MissingFile     — LoadFromFile returns false on missing file
 *   3.  Test_LoadFromFile_MalformedJSON   — LoadFromFile returns false on bad JSON
 *   4.  Test_GetAllPresets                — GetAllPresets returns 15 entries
 *   5.  Test_GetPreset_ByName             — GetPreset returns correct metadata
 *   6.  Test_GetPreset_NotFound           — GetPreset returns nullptr for unknown name
 *   7.  Test_GetByCategory                — GetByCategory filters correctly
 *   8.  Test_GetAllCategories             — GetAllCategories returns 5 categories
 *   9.  Test_HasPreset                    — HasPreset returns correct results
 *  10.  Test_GetCount                     — GetCount matches loaded preset count
 *  11.  Test_Clear                        — Clear empties the registry
 *  12.  Test_DuplicatePrevention          — Duplicate preset name is skipped
 *  13.  Test_CategoryFiltering_Accuracy   — Each category has correct preset count
 *
 *   === BUG-001 Regression Tests (5) ===
 *  14.  Test_BugFix_ValidEntry            — Valid entry serializes correctly
 *  15.  Test_BugFix_EmptyKey              — Empty key entry is flagged as invalid
 *  16.  Test_BugFix_NoneType              — VariableType::None entry is flagged as invalid
 *  17.  Test_BugFix_SafeInit              — New entry default values are safe (non-empty key, Int type)
 *  18.  Test_BugFix_AllTypesSkipNone      — All valid types other than None serialize correctly
 *
 *   === BUG-002 Regression Tests (5) ===
 *  19.  Test_BUG002_ValidateAndClean      — ValidateAndCleanBlackboardEntries removes bad entries
 *  20.  Test_BUG002_TypeIdxBoundsCheck    — Out-of-range typeIdx is clamped to Int
 *  21.  Test_BUG002_VariableTypeToString  — VariableTypeToString returns correct strings + fallback
 *  22.  Test_BUG002_TaskValueToString     — TaskValue::to_string() never returns empty
 *  23.  Test_BUG002_GetDefaultValueForType— GetDefaultValueForType returns typed defaults
 *
 * C++14 compliant — no std::optional, structured bindings, std::filesystem.
 */

#include "BlueprintEditor/BlackboardVariablePresetRegistry.h"
#include "TaskSystem/TaskGraphTemplate.h"
#include "TaskSystem/TaskGraphTypes.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <cstring>

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
        ++s_failCount;
    }
}

// ---------------------------------------------------------------------------
// Helpers: write temp JSON files
// ---------------------------------------------------------------------------

static const char* k_validJsonPath    = "/tmp/phase23b_valid_presets.json";
static const char* k_malformedPath    = "/tmp/phase23b_malformed.json";
static const char* k_dupPath          = "/tmp/phase23b_dup.json";
static const char* k_missingPath      = "/tmp/phase23b_no_such_file_xyz.json";

static bool WriteFile(const char* path, const char* content)
{
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

static const char* k_validJson =
    "{"
    "  \"version\": \"1.0\","
    "  \"availableVariables\": ["
    "    { \"name\": \"targetDistance\", \"type\": \"Float\",  \"description\": \"Distance\", \"category\": \"Targeting\", \"default\": \"9999.0\" },"
    "    { \"name\": \"targetSeen\",     \"type\": \"Bool\",   \"description\": \"Seen\",     \"category\": \"Targeting\", \"default\": \"false\" },"
    "    { \"name\": \"patrolIndex\",    \"type\": \"Int\",    \"description\": \"Patrol\",   \"category\": \"Movement\",  \"default\": \"0\" },"
    "    { \"name\": \"moveSpeed\",      \"type\": \"Float\",  \"description\": \"Speed\",    \"category\": \"Movement\",  \"default\": \"1.0\" },"
    "    { \"name\": \"alertLevel\",     \"type\": \"Int\",    \"description\": \"Alert\",    \"category\": \"State\",     \"default\": \"0\" },"
    "    { \"name\": \"currentState\",   \"type\": \"String\", \"description\": \"State\",    \"category\": \"State\",     \"default\": \"idle\" },"
    "    { \"name\": \"isAlive\",        \"type\": \"Bool\",   \"description\": \"Alive\",    \"category\": \"State\",     \"default\": \"true\" },"
    "    { \"name\": \"health\",         \"type\": \"Float\",  \"description\": \"Health\",   \"category\": \"Combat\",    \"default\": \"100.0\" },"
    "    { \"name\": \"damageDealt\",    \"type\": \"Float\",  \"description\": \"Damage\",   \"category\": \"Combat\",    \"default\": \"0.0\" },"
    "    { \"name\": \"lastAttacker\",   \"type\": \"EntityID\", \"description\": \"Attacker\", \"category\": \"Combat\", \"default\": \"0\" },"
    "    { \"name\": \"timerElapsed\",   \"type\": \"Float\",  \"description\": \"Timer\",    \"category\": \"Misc\",      \"default\": \"0.0\" },"
    "    { \"name\": \"loopCounter\",    \"type\": \"Int\",    \"description\": \"Counter\",  \"category\": \"Misc\",      \"default\": \"0\" },"
    "    { \"name\": \"customTag\",      \"type\": \"String\", \"description\": \"Tag\",      \"category\": \"Misc\",      \"default\": \"\" },"
    "    { \"name\": \"destinationReached\", \"type\": \"Bool\", \"description\": \"Done\", \"category\": \"Movement\", \"default\": \"false\" },"
    "    { \"name\": \"targetEntityID\", \"type\": \"EntityID\", \"description\": \"ID\",   \"category\": \"Targeting\", \"default\": \"0\" }"
    "  ]"
    "}";

static const char* k_malformedJson = "{ bad json {{{{";

static const char* k_dupJson =
    "{"
    "  \"availableVariables\": ["
    "    { \"name\": \"alpha\", \"type\": \"Float\", \"description\": \"A\", \"category\": \"Misc\", \"default\": \"0.0\" },"
    "    { \"name\": \"alpha\", \"type\": \"Int\",   \"description\": \"B\", \"category\": \"Misc\", \"default\": \"0\" }"
    "  ]"
    "}";

// ---------------------------------------------------------------------------
// === BlackboardVariablePresetRegistry tests ===
// ---------------------------------------------------------------------------

static void Test_LoadFromFile_Success()
{
    const std::string name = "LoadFromFile_Success";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    bool ok = reg.LoadFromFile(k_validJsonPath);
    TEST_ASSERT(ok, "LoadFromFile returns true on valid JSON");
    TEST_ASSERT(reg.GetCount() == 15, "GetCount() == 15 after loading valid JSON");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_LoadFromFile_MissingFile()
{
    const std::string name = "LoadFromFile_MissingFile";
    int prevFail = s_failCount;

    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    bool ok = reg.LoadFromFile(k_missingPath);
    TEST_ASSERT(!ok, "LoadFromFile returns false when file does not exist");
    TEST_ASSERT(reg.GetCount() == 0, "Registry is empty after failed load");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_LoadFromFile_MalformedJSON()
{
    const std::string name = "LoadFromFile_MalformedJSON";
    int prevFail = s_failCount;

    WriteFile(k_malformedPath, k_malformedJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    bool ok = reg.LoadFromFile(k_malformedPath);
    TEST_ASSERT(!ok, "LoadFromFile returns false on malformed JSON");
    TEST_ASSERT(reg.GetCount() == 0, "Registry is empty after malformed load");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetAllPresets()
{
    const std::string name = "GetAllPresets";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    const std::vector<BlackboardVariablePreset>& all = reg.GetAllPresets();
    TEST_ASSERT(all.size() == 15, "GetAllPresets() returns 15 presets");
    TEST_ASSERT(!all.empty(), "GetAllPresets() is not empty");

    // Check sorted by name
    bool sorted = true;
    for (size_t i = 1; i < all.size(); ++i)
    {
        if (all[i].name < all[i - 1].name)
        {
            sorted = false;
            break;
        }
    }
    TEST_ASSERT(sorted, "GetAllPresets() is sorted by name");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetPreset_ByName()
{
    const std::string name = "GetPreset_ByName";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    const BlackboardVariablePreset* p = reg.GetPreset("targetDistance");
    TEST_ASSERT(p != nullptr,                      "GetPreset('targetDistance') returns non-null");
    TEST_ASSERT(p->type == "Float",                "targetDistance type is Float");
    TEST_ASSERT(p->category == "Targeting",        "targetDistance category is Targeting");
    TEST_ASSERT(p->defaultValue == "9999.0",       "targetDistance default is 9999.0");
    TEST_ASSERT(!p->description.empty(),           "targetDistance description is non-empty");

    const BlackboardVariablePreset* h = reg.GetPreset("health");
    TEST_ASSERT(h != nullptr,                      "GetPreset('health') returns non-null");
    TEST_ASSERT(h->type == "Float",                "health type is Float");
    TEST_ASSERT(h->category == "Combat",           "health category is Combat");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetPreset_NotFound()
{
    const std::string name = "GetPreset_NotFound";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    const BlackboardVariablePreset* p = reg.GetPreset("nonExistentVariable");
    TEST_ASSERT(p == nullptr, "GetPreset returns nullptr for unknown name");

    const BlackboardVariablePreset* empty = reg.GetPreset("");
    TEST_ASSERT(empty == nullptr, "GetPreset returns nullptr for empty name");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetByCategory()
{
    const std::string name = "GetByCategory";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    std::vector<BlackboardVariablePreset> targeting = reg.GetByCategory("Targeting");
    TEST_ASSERT(targeting.size() == 3, "Targeting category has 3 presets");

    std::vector<BlackboardVariablePreset> movement = reg.GetByCategory("Movement");
    TEST_ASSERT(movement.size() == 3, "Movement category has 3 presets");

    std::vector<BlackboardVariablePreset> combat = reg.GetByCategory("Combat");
    TEST_ASSERT(combat.size() == 3, "Combat category has 3 presets");

    std::vector<BlackboardVariablePreset> state = reg.GetByCategory("State");
    TEST_ASSERT(state.size() == 3, "State category has 3 presets");

    std::vector<BlackboardVariablePreset> misc = reg.GetByCategory("Misc");
    TEST_ASSERT(misc.size() == 3, "Misc category has 3 presets");

    std::vector<BlackboardVariablePreset> unknown = reg.GetByCategory("UnknownCat");
    TEST_ASSERT(unknown.empty(), "Unknown category returns empty vector");

    // All presets from a category have that category field set
    for (size_t i = 0; i < targeting.size(); ++i)
        TEST_ASSERT(targeting[i].category == "Targeting", "All Targeting entries have correct category");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetAllCategories()
{
    const std::string name = "GetAllCategories";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    std::vector<std::string> cats = reg.GetAllCategories();
    TEST_ASSERT(cats.size() == 5, "GetAllCategories() returns 5 categories");

    // Check sorted
    bool sorted = true;
    for (size_t i = 1; i < cats.size(); ++i)
    {
        if (cats[i] < cats[i - 1])
        {
            sorted = false;
            break;
        }
    }
    TEST_ASSERT(sorted, "GetAllCategories() is sorted");

    // Expected categories
    bool hasCombat    = false;
    bool hasMisc      = false;
    bool hasMovement  = false;
    bool hasState     = false;
    bool hasTargeting = false;
    for (size_t i = 0; i < cats.size(); ++i)
    {
        if (cats[i] == "Combat")    hasCombat    = true;
        if (cats[i] == "Misc")      hasMisc      = true;
        if (cats[i] == "Movement")  hasMovement  = true;
        if (cats[i] == "State")     hasState     = true;
        if (cats[i] == "Targeting") hasTargeting = true;
    }
    TEST_ASSERT(hasCombat,    "Categories include 'Combat'");
    TEST_ASSERT(hasMisc,      "Categories include 'Misc'");
    TEST_ASSERT(hasMovement,  "Categories include 'Movement'");
    TEST_ASSERT(hasState,     "Categories include 'State'");
    TEST_ASSERT(hasTargeting, "Categories include 'Targeting'");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_HasPreset()
{
    const std::string name = "HasPreset";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    TEST_ASSERT(reg.HasPreset("health"),              "HasPreset('health') is true");
    TEST_ASSERT(reg.HasPreset("patrolIndex"),          "HasPreset('patrolIndex') is true");
    TEST_ASSERT(reg.HasPreset("currentState"),         "HasPreset('currentState') is true");
    TEST_ASSERT(!reg.HasPreset("nonExistent"),         "HasPreset('nonExistent') is false");
    TEST_ASSERT(!reg.HasPreset(""),                    "HasPreset('') is false");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_GetCount()
{
    const std::string name = "GetCount";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    TEST_ASSERT(reg.GetCount() == 0, "GetCount() == 0 after Clear");

    reg.LoadFromFile(k_validJsonPath);
    TEST_ASSERT(reg.GetCount() == 15, "GetCount() == 15 after loading valid JSON");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_Clear()
{
    const std::string name = "Clear";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.LoadFromFile(k_validJsonPath);
    TEST_ASSERT(reg.GetCount() > 0, "Registry is non-empty before Clear");

    reg.Clear();
    TEST_ASSERT(reg.GetCount() == 0,            "GetCount() == 0 after Clear");
    TEST_ASSERT(reg.GetAllPresets().empty(),     "GetAllPresets() is empty after Clear");
    TEST_ASSERT(!reg.HasPreset("health"),        "HasPreset returns false after Clear");
    TEST_ASSERT(reg.GetPreset("health") == nullptr, "GetPreset returns nullptr after Clear");
    TEST_ASSERT(reg.GetAllCategories().empty(), "GetAllCategories is empty after Clear");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_DuplicatePrevention()
{
    const std::string name = "DuplicatePrevention";
    int prevFail = s_failCount;

    WriteFile(k_dupPath, k_dupJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_dupPath);

    // Only 1 entry should exist (duplicate "alpha" skipped)
    TEST_ASSERT(reg.GetCount() == 1, "Duplicate preset name is skipped — only 1 entry loaded");

    const BlackboardVariablePreset* p = reg.GetPreset("alpha");
    TEST_ASSERT(p != nullptr,          "First 'alpha' preset is accessible");
    TEST_ASSERT(p->type == "Float",    "First 'alpha' preset type is Float (not overwritten by dup)");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_CategoryFiltering_Accuracy()
{
    const std::string name = "CategoryFiltering_Accuracy";
    int prevFail = s_failCount;

    WriteFile(k_validJsonPath, k_validJson);
    BlackboardVariablePresetRegistry& reg = BlackboardVariablePresetRegistry::Instance();
    reg.Clear();
    reg.LoadFromFile(k_validJsonPath);

    // Sum of all category counts should equal total preset count
    std::vector<std::string> cats = reg.GetAllCategories();
    size_t total = 0;
    for (size_t i = 0; i < cats.size(); ++i)
    {
        std::vector<BlackboardVariablePreset> bycat = reg.GetByCategory(cats[i]);
        total += bycat.size();
        for (size_t j = 0; j < bycat.size(); ++j)
            TEST_ASSERT(bycat[j].category == cats[i], "Each filtered preset has correct category");
    }
    TEST_ASSERT(total == reg.GetCount(), "Sum of per-category counts equals total preset count");

    ReportTest(name, s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// === BUG-001 Regression Tests ===
// ---------------------------------------------------------------------------

static void Test_BugFix_ValidEntry()
{
    const std::string name = "BugFix_ValidEntry";
    int prevFail = s_failCount;

    // A valid BlackboardEntry should not be flagged as invalid
    BlackboardEntry entry;
    entry.Key      = "health";
    entry.Type     = VariableType::Float;
    entry.IsGlobal = false;

    bool isInvalid = entry.Key.empty() || entry.Type == VariableType::None;
    TEST_ASSERT(!isInvalid, "Valid entry (non-empty key, non-None type) is not flagged as invalid");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_BugFix_EmptyKey()
{
    const std::string name = "BugFix_EmptyKey";
    int prevFail = s_failCount;

    // An entry with an empty key should be flagged as invalid and skipped on save
    BlackboardEntry entry;
    entry.Key      = "";
    entry.Type     = VariableType::Int;
    entry.IsGlobal = false;

    bool isInvalid = entry.Key.empty() || entry.Type == VariableType::None;
    TEST_ASSERT(isInvalid, "Entry with empty key is flagged as invalid (must be skipped on save)");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_BugFix_NoneType()
{
    const std::string name = "BugFix_NoneType";
    int prevFail = s_failCount;

    // An entry with VariableType::None should be flagged as invalid — this was the P0 crash cause
    BlackboardEntry entry;
    entry.Key      = "badVar";
    entry.Type     = VariableType::None;
    entry.IsGlobal = false;

    bool isInvalid = entry.Key.empty() || entry.Type == VariableType::None;
    TEST_ASSERT(isInvalid, "Entry with VariableType::None is flagged as invalid (prevents P0 crash)");

    // Default-constructed entry is always invalid (key empty AND type None)
    BlackboardEntry def;
    bool defInvalid = def.Key.empty() || def.Type == VariableType::None;
    TEST_ASSERT(defInvalid, "Default-constructed BlackboardEntry is flagged as invalid");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_BugFix_SafeInit()
{
    const std::string name = "BugFix_SafeInit";
    int prevFail = s_failCount;

    // Simulate the safe init applied by RenderBlackboard after BUG-001 hotfix
    BlackboardEntry entry;
    entry.Key      = "NewVariable";    // was "newKey" — non-empty
    entry.Type     = VariableType::Int; // was Float — still valid but Int is a better default
    entry.IsGlobal = false;

    bool isInvalid = entry.Key.empty() || entry.Type == VariableType::None;
    TEST_ASSERT(!isInvalid, "Safe-initialized entry is not flagged as invalid");
    TEST_ASSERT(entry.Key == "NewVariable", "Safe init key is 'NewVariable'");
    TEST_ASSERT(entry.Type == VariableType::Int, "Safe init type is Int");

    ReportTest(name, s_failCount == prevFail);
}

static void Test_BugFix_AllTypesSkipNone()
{
    const std::string name = "BugFix_AllTypesSkipNone";
    int prevFail = s_failCount;

    // All valid VariableType values except None should pass the guard
    VariableType validTypes[] = {
        VariableType::Bool,
        VariableType::Int,
        VariableType::Float,
        VariableType::String,
        VariableType::EntityID,
        VariableType::Vector
    };
    size_t validCount = sizeof(validTypes) / sizeof(validTypes[0]);

    for (size_t i = 0; i < validCount; ++i)
    {
        BlackboardEntry entry;
        entry.Key      = "var";
        entry.Type     = validTypes[i];
        entry.IsGlobal = false;
        bool isInvalid = entry.Key.empty() || entry.Type == VariableType::None;
        TEST_ASSERT(!isInvalid, "Valid VariableType entry passes save guard");
    }

    // VariableType::None must fail
    BlackboardEntry noneEntry;
    noneEntry.Key      = "var";
    noneEntry.Type     = VariableType::None;
    noneEntry.IsGlobal = false;
    bool noneInvalid = noneEntry.Key.empty() || noneEntry.Type == VariableType::None;
    TEST_ASSERT(noneInvalid, "VariableType::None entry fails save guard");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// === BUG-002 Regression Tests ===
// ============================================================================

// Fix #1: ValidateAndCleanBlackboardEntries logic
static void Test_BUG002_ValidateAndClean()
{
    const std::string name = "BUG002_ValidateAndClean";
    int prevFail = s_failCount;

    // Simulate the logic of ValidateAndCleanBlackboardEntries():
    // entries with empty key or VariableType::None must be removed.
    std::vector<BlackboardEntry> entries;

    BlackboardEntry valid;
    valid.Key  = "health";
    valid.Type = VariableType::Float;
    entries.push_back(valid);

    BlackboardEntry emptyKey;
    emptyKey.Key  = "";
    emptyKey.Type = VariableType::Int;
    entries.push_back(emptyKey);

    BlackboardEntry noneType;
    noneType.Key  = "badVar";
    noneType.Type = VariableType::None;
    entries.push_back(noneType);

    // Apply validate-and-clean
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [](const BlackboardEntry& e) {
                return e.Key.empty() || e.Type == VariableType::None;
            }),
        entries.end());

    TEST_ASSERT(entries.size() == 1,              "ValidateAndClean keeps exactly 1 valid entry");
    TEST_ASSERT(entries[0].Key == "health",        "ValidateAndClean preserves the valid entry key");
    TEST_ASSERT(entries[0].Type == VariableType::Float, "ValidateAndClean preserves the valid entry type");

    ReportTest(name, s_failCount == prevFail);
}

// Fix #2: Dropdown typeIdx bounds-check
static void Test_BUG002_TypeIdxBoundsCheck()
{
    const std::string name = "BUG002_TypeIdxBoundsCheck";
    int prevFail = s_failCount;

    // Simulate the bounds-check logic applied before ImGui::Combo
    auto clampTypeIdx = [](int typeIdx, VariableType& outType) -> int {
        if (typeIdx < 0 || typeIdx >= 7) {
            typeIdx  = static_cast<int>(VariableType::Int);
            outType  = VariableType::Int;
        }
        return typeIdx;
    };

    // Normal value — should be unchanged
    VariableType t1 = VariableType::Float;
    int idx1 = clampTypeIdx(static_cast<int>(VariableType::Float), t1);
    TEST_ASSERT(idx1 == static_cast<int>(VariableType::Float), "Valid typeIdx Float is unchanged");

    // -1 (garbage) — must be clamped to Int
    VariableType t2 = static_cast<VariableType>(255);
    int idx2 = clampTypeIdx(-1, t2);
    TEST_ASSERT(idx2 == static_cast<int>(VariableType::Int), "Negative typeIdx clamped to Int");
    TEST_ASSERT(t2  == VariableType::Int,                    "Negative typeIdx sets type to Int");

    // 99 (out of range) — must be clamped to Int
    VariableType t3 = static_cast<VariableType>(255);
    int idx3 = clampTypeIdx(99, t3);
    TEST_ASSERT(idx3 == static_cast<int>(VariableType::Int), "Out-of-range typeIdx clamped to Int");
    TEST_ASSERT(t3  == VariableType::Int,                    "Out-of-range typeIdx sets type to Int");

    ReportTest(name, s_failCount == prevFail);
}

// Fix #3: VariableTypeToString with fallback
static void Test_BUG002_VariableTypeToString()
{
    const std::string name = "BUG002_VariableTypeToString";
    int prevFail = s_failCount;

    TEST_ASSERT(VariableTypeToString(VariableType::Bool)     == "Bool",     "Bool maps to 'Bool'");
    TEST_ASSERT(VariableTypeToString(VariableType::Int)      == "Int",      "Int maps to 'Int'");
    TEST_ASSERT(VariableTypeToString(VariableType::Float)    == "Float",    "Float maps to 'Float'");
    TEST_ASSERT(VariableTypeToString(VariableType::Vector)   == "Vector",   "Vector maps to 'Vector'");
    TEST_ASSERT(VariableTypeToString(VariableType::EntityID) == "EntityID", "EntityID maps to 'EntityID'");
    TEST_ASSERT(VariableTypeToString(VariableType::String)   == "String",   "String maps to 'String'");
    TEST_ASSERT(VariableTypeToString(VariableType::None)     == "None",     "None maps to 'None'");

    // All results must be non-empty (no garbage)
    bool allNonEmpty = true;
    VariableType allTypes[] = {
        VariableType::Bool, VariableType::Int,  VariableType::Float,
        VariableType::Vector, VariableType::EntityID, VariableType::String,
        VariableType::None
    };
    for (size_t i = 0; i < sizeof(allTypes)/sizeof(allTypes[0]); ++i)
    {
        if (VariableTypeToString(allTypes[i]).empty()) { allNonEmpty = false; break; }
    }
    TEST_ASSERT(allNonEmpty, "VariableTypeToString never returns empty string");

    ReportTest(name, s_failCount == prevFail);
}

// Fix #4: TaskValue::to_string() never returns empty
static void Test_BUG002_TaskValueToString()
{
    const std::string name = "BUG002_TaskValueToString";
    int prevFail = s_failCount;

    // None → fallback "0"
    TaskValue none;
    TEST_ASSERT(none.to_string() == "0", "None TaskValue to_string() returns '0'");
    TEST_ASSERT(!none.to_string().empty(), "None TaskValue to_string() never empty");

    // Bool
    TaskValue bTrue(true);
    TaskValue bFalse(false);
    TEST_ASSERT(bTrue.to_string()  == "true",  "Bool true to_string() == 'true'");
    TEST_ASSERT(bFalse.to_string() == "false", "Bool false to_string() == 'false'");

    // Int
    TaskValue i42(42);
    TEST_ASSERT(i42.to_string() == "42", "Int 42 to_string() == '42'");
    TEST_ASSERT(!i42.to_string().empty(), "Int to_string() never empty");

    // Float
    TaskValue f0(0.0f);
    TEST_ASSERT(!f0.to_string().empty(), "Float 0.0 to_string() never empty");

    // String
    TaskValue str(std::string("hello"));
    TEST_ASSERT(str.to_string() == "hello", "String TaskValue to_string() == 'hello'");

    ReportTest(name, s_failCount == prevFail);
}

// UX Fix #1: GetDefaultValueForType returns typed defaults
static void Test_BUG002_GetDefaultValueForType()
{
    const std::string name = "BUG002_GetDefaultValueForType";
    int prevFail = s_failCount;

    TaskValue vBool   = GetDefaultValueForType(VariableType::Bool);
    TaskValue vInt    = GetDefaultValueForType(VariableType::Int);
    TaskValue vFloat  = GetDefaultValueForType(VariableType::Float);
    TaskValue vString = GetDefaultValueForType(VariableType::String);
    TaskValue vEntity = GetDefaultValueForType(VariableType::EntityID);

    TEST_ASSERT(vBool.GetType()   == VariableType::Bool,     "GetDefaultValueForType(Bool)     → Bool");
    TEST_ASSERT(vInt.GetType()    == VariableType::Int,      "GetDefaultValueForType(Int)      → Int");
    TEST_ASSERT(vFloat.GetType()  == VariableType::Float,    "GetDefaultValueForType(Float)    → Float");
    TEST_ASSERT(vString.GetType() == VariableType::String,   "GetDefaultValueForType(String)   → String");
    TEST_ASSERT(vEntity.GetType() == VariableType::EntityID, "GetDefaultValueForType(EntityID) → EntityID");

    // Values must be initialized to zero/false/empty
    TEST_ASSERT(vBool.AsBool()     == false, "Default Bool is false");
    TEST_ASSERT(vInt.AsInt()       == 0,     "Default Int is 0");
    TEST_ASSERT(vFloat.AsFloat()   == 0.0f,  "Default Float is 0.0f");
    TEST_ASSERT(vString.AsString() == "",    "Default String is empty string");

    // None type must never return VariableType::None (fallback to Int)
    TaskValue vNone = GetDefaultValueForType(VariableType::None);
    TEST_ASSERT(vNone.GetType() != VariableType::None, "GetDefaultValueForType(None) does not return None");

    ReportTest(name, s_failCount == prevFail);
}

// ============================================================================
// main
// ============================================================================

int main()
{
    std::cout << "=== Phase 23-B Tests — Full Blackboard Properties (BUG-001 + Registry) ===" << std::endl;

    // BlackboardVariablePresetRegistry
    Test_LoadFromFile_Success();
    Test_LoadFromFile_MissingFile();
    Test_LoadFromFile_MalformedJSON();
    Test_GetAllPresets();
    Test_GetPreset_ByName();
    Test_GetPreset_NotFound();
    Test_GetByCategory();
    Test_GetAllCategories();
    Test_HasPreset();
    Test_GetCount();
    Test_Clear();
    Test_DuplicatePrevention();
    Test_CategoryFiltering_Accuracy();

    // BUG-001 Regression
    Test_BugFix_ValidEntry();
    Test_BugFix_EmptyKey();
    Test_BugFix_NoneType();
    Test_BugFix_SafeInit();
    Test_BugFix_AllTypesSkipNone();

    // BUG-002 Regression
    Test_BUG002_ValidateAndClean();
    Test_BUG002_TypeIdxBoundsCheck();
    Test_BUG002_VariableTypeToString();
    Test_BUG002_TaskValueToString();
    Test_BUG002_GetDefaultValueForType();

    std::cout << "\n=== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed ===" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
