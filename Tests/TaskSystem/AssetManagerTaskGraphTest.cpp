/**
 * @file AssetManagerTaskGraphTest.cpp
 * @brief Unit tests for AssetManager TaskGraph caching (Phase 1.3)
 * @author Olympe Engine
 * @date 2026-02-22
 *
 * @details
 * Tests cover:
 *   a) LoadTaskGraph returns a valid non-zero AssetID for a known blueprint
 *   b) GetTaskGraph returns a non-null, valid TaskGraphTemplate
 *   c) Repeated loads of the same path return the same AssetID (cache hit)
 *   d) UnloadTaskGraph removes the template; GetTaskGraph returns nullptr after
 *   e) LoadTaskGraph with empty path returns INVALID_ASSET_ID
 *   f) LoadTaskGraph with a non-existent file returns INVALID_ASSET_ID
 *
 * Run from the repository root so that relative paths to Blueprints/AI/ resolve.
 */

#include "Core/AssetManager.h"
#include "TaskSystem/TaskGraphTemplate.h"

#include <iostream>
#include <string>
#include <vector>

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
// Helper: reset AssetManager state between tests
// ---------------------------------------------------------------------------

static void UnloadIfLoaded(Olympe::AssetID id)
{
    if (id != Olympe::INVALID_ASSET_ID)
    {
        Olympe::AssetManager::Get().UnloadTaskGraph(id);
    }
}

// ---------------------------------------------------------------------------
// Test a: LoadTaskGraph returns valid AssetID for guardV2_ai.json
// ---------------------------------------------------------------------------

static void TestA_LoadValidBlueprint()
{
    std::cout << "Test A: LoadTaskGraph valid blueprint..." << std::endl;

    std::vector<std::string> errors;
    Olympe::AssetID id =
        Olympe::AssetManager::Get().LoadTaskGraph(
            "Blueprints/AI/guardV2_ai.json", errors);

    bool passed = true;

    TEST_ASSERT(id != Olympe::INVALID_ASSET_ID,
                "LoadTaskGraph should return non-zero AssetID");
    if (id == Olympe::INVALID_ASSET_ID)
    {
        passed = false;
        for (size_t i = 0; i < errors.size(); ++i)
        {
            std::cout << "  Error[" << i << "]: " << errors[i] << std::endl;
        }
    }

    UnloadIfLoaded(id);
    ReportTest("TestA_LoadValidBlueprint", passed);
}

// ---------------------------------------------------------------------------
// Test b: GetTaskGraph returns a valid, non-null TaskGraphTemplate
// ---------------------------------------------------------------------------

static void TestB_GetTaskGraphValid()
{
    std::cout << "Test B: GetTaskGraph returns valid template..." << std::endl;

    std::vector<std::string> errors;
    Olympe::AssetID id =
        Olympe::AssetManager::Get().LoadTaskGraph(
            "Blueprints/AI/guardV2_ai.json", errors);

    bool passed = true;

    if (id == Olympe::INVALID_ASSET_ID)
    {
        std::cout << "  SKIP: LoadTaskGraph failed, cannot test GetTaskGraph" << std::endl;
        ReportTest("TestB_GetTaskGraphValid", false);
        return;
    }

    const Olympe::TaskGraphTemplate* tmpl =
        Olympe::AssetManager::Get().GetTaskGraph(id);

    TEST_ASSERT(tmpl != nullptr,
                "GetTaskGraph should return non-null pointer");
    if (tmpl == nullptr)
    {
        passed = false;
    }

    if (tmpl != nullptr)
    {
        // Validate structural integrity
        TEST_ASSERT(tmpl->Validate(), "Template should pass Validate()");
        if (!tmpl->Validate()) { passed = false; }

        TEST_ASSERT(!tmpl->Nodes.empty(), "Template should have at least one node");
        if (tmpl->Nodes.empty()) { passed = false; }

        TEST_ASSERT(tmpl->RootNodeID != -1, "RootNodeID should not be -1");
        if (tmpl->RootNodeID == -1) { passed = false; }

        const Olympe::TaskNodeDefinition* root = tmpl->GetNode(tmpl->RootNodeID);
        TEST_ASSERT(root != nullptr, "Root node must be reachable via GetNode()");
        if (root == nullptr) { passed = false; }
    }

    UnloadIfLoaded(id);
    ReportTest("TestB_GetTaskGraphValid", passed);
}

// ---------------------------------------------------------------------------
// Test c: Loading the same path twice returns the same AssetID (cache hit)
// ---------------------------------------------------------------------------

static void TestC_CacheHitSamePath()
{
    std::cout << "Test C: Cache hit for same path..." << std::endl;

    const std::string path = "Blueprints/AI/npc_wander_ai.json";
    std::vector<std::string> errors1;
    std::vector<std::string> errors2;

    Olympe::AssetID id1 = Olympe::AssetManager::Get().LoadTaskGraph(path, errors1);
    Olympe::AssetID id2 = Olympe::AssetManager::Get().LoadTaskGraph(path, errors2);

    bool passed = true;

    TEST_ASSERT(id1 != Olympe::INVALID_ASSET_ID,
                "First load should return valid AssetID");
    if (id1 == Olympe::INVALID_ASSET_ID) { passed = false; }

    TEST_ASSERT(id1 == id2,
                "Second load of same path must return the same AssetID");
    if (id1 != id2) { passed = false; }

    UnloadIfLoaded(id1);
    ReportTest("TestC_CacheHitSamePath", passed);
}

// ---------------------------------------------------------------------------
// Test d: UnloadTaskGraph removes the asset; GetTaskGraph returns nullptr after
// ---------------------------------------------------------------------------

static void TestD_UnloadRemovesAsset()
{
    std::cout << "Test D: UnloadTaskGraph removes asset from cache..." << std::endl;

    const std::string path = "Blueprints/AI/guardV2_ai.json";
    std::vector<std::string> errors;

    Olympe::AssetID id = Olympe::AssetManager::Get().LoadTaskGraph(path, errors);

    bool passed = true;

    if (id == Olympe::INVALID_ASSET_ID)
    {
        std::cout << "  SKIP: LoadTaskGraph failed, cannot test UnloadTaskGraph" << std::endl;
        ReportTest("TestD_UnloadRemovesAsset", false);
        return;
    }

    // Verify it's present before unload
    TEST_ASSERT(Olympe::AssetManager::Get().GetTaskGraph(id) != nullptr,
                "GetTaskGraph should return non-null before unload");

    Olympe::AssetManager::Get().UnloadTaskGraph(id);

    // After unload the pointer must be null
    TEST_ASSERT(Olympe::AssetManager::Get().GetTaskGraph(id) == nullptr,
                "GetTaskGraph should return nullptr after UnloadTaskGraph");
    if (Olympe::AssetManager::Get().GetTaskGraph(id) != nullptr) { passed = false; }

    ReportTest("TestD_UnloadRemovesAsset", passed);
}

// ---------------------------------------------------------------------------
// Test e: LoadTaskGraph with empty path returns INVALID_ASSET_ID
// ---------------------------------------------------------------------------

static void TestE_EmptyPathReturnsInvalidID()
{
    std::cout << "Test E: Empty path returns INVALID_ASSET_ID..." << std::endl;

    std::vector<std::string> errors;
    Olympe::AssetID id =
        Olympe::AssetManager::Get().LoadTaskGraph("", errors);

    bool passed = (id == Olympe::INVALID_ASSET_ID && !errors.empty());

    TEST_ASSERT(id == Olympe::INVALID_ASSET_ID,
                "LoadTaskGraph(\"\") should return INVALID_ASSET_ID");
    TEST_ASSERT(!errors.empty(),
                "outErrors should not be empty for empty path");

    ReportTest("TestE_EmptyPathReturnsInvalidID", passed);
}

// ---------------------------------------------------------------------------
// Test f: LoadTaskGraph with non-existent file returns INVALID_ASSET_ID
// ---------------------------------------------------------------------------

static void TestF_NonExistentFileReturnsInvalidID()
{
    std::cout << "Test F: Non-existent file returns INVALID_ASSET_ID..." << std::endl;

    std::vector<std::string> errors;
    Olympe::AssetID id =
        Olympe::AssetManager::Get().LoadTaskGraph(
            "Blueprints/AI/does_not_exist.json", errors);

    bool passed = (id == Olympe::INVALID_ASSET_ID);

    TEST_ASSERT(id == Olympe::INVALID_ASSET_ID,
                "LoadTaskGraph for missing file should return INVALID_ASSET_ID");
    TEST_ASSERT(!errors.empty(),
                "outErrors should not be empty for missing file");

    ReportTest("TestF_NonExistentFileReturnsInvalidID", passed);
}

// ---------------------------------------------------------------------------
// Test g: ComputeAssetID is deterministic and returns 0 for empty string
// ---------------------------------------------------------------------------

static void TestG_ComputeAssetID()
{
    std::cout << "Test G: ComputeAssetID deterministic..." << std::endl;

    bool passed = true;

    Olympe::AssetID id1 = Olympe::AssetManager::ComputeAssetID("Blueprints/AI/guardV2_ai.json");
    Olympe::AssetID id2 = Olympe::AssetManager::ComputeAssetID("Blueprints/AI/guardV2_ai.json");
    Olympe::AssetID id3 = Olympe::AssetManager::ComputeAssetID("Blueprints/AI/npc_wander_ai.json");
    Olympe::AssetID idEmpty = Olympe::AssetManager::ComputeAssetID("");

    TEST_ASSERT(id1 == id2, "Same path must produce same AssetID");
    if (id1 != id2) { passed = false; }

    TEST_ASSERT(id1 != id3, "Different paths should produce different AssetIDs");
    if (id1 == id3) { passed = false; }

    TEST_ASSERT(idEmpty == Olympe::INVALID_ASSET_ID,
                "Empty path must return INVALID_ASSET_ID");
    if (idEmpty != Olympe::INVALID_ASSET_ID) { passed = false; }

    TEST_ASSERT(id1 != Olympe::INVALID_ASSET_ID,
                "Non-empty path must not return INVALID_ASSET_ID");
    if (id1 == Olympe::INVALID_ASSET_ID) { passed = false; }

    ReportTest("TestG_ComputeAssetID", passed);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=== AssetManagerTaskGraphTest ===" << std::endl;

    TestA_LoadValidBlueprint();
    TestB_GetTaskGraphValid();
    TestC_CacheHitSamePath();
    TestD_UnloadRemovesAsset();
    TestE_EmptyPathReturnsInvalidID();
    TestF_NonExistentFileReturnsInvalidID();
    TestG_ComputeAssetID();

    std::cout << std::endl;
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
