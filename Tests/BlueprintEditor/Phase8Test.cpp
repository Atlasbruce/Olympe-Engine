/**
 * @file Phase8Test.cpp
 * @brief Unit tests for Phase 8 ATS Visual Scripting — Subgraph System.
 * @author Olympe Engine
 * @date 2026-03-09
 *
 * @details
 * Tests cover:
 *   1.  SubgraphMigrator_NeedsMigration_LegacyBTFormat
 *   2.  SubgraphMigrator_NeedsMigration_NewFormat_ReturnsFalse
 *   3.  SubgraphMigrator_NeedsMigration_HFSMFormat
 *   4.  SubgraphMigrator_IsNewFormat_DetectsRootGraph
 *   5.  SubgraphMigrator_Migrate_BT_NodesMovedToRootGraph
 *   6.  SubgraphMigrator_Migrate_BT_SubgraphsDictCreated
 *   7.  SubgraphMigrator_Migrate_BT_SchemaVersionBumped
 *   8.  SubgraphMigrator_Migrate_HFSM_StatesMovedToRootGraph
 *   9.  SubgraphMigrator_Migrate_AlreadyNew_ReturnsUnchanged
 *  10.  SubgraphMigrator_MakeEmptySubgraph_BT_HasRequiredFields
 *  11.  SubgraphMigrator_MakeEmptySubgraph_HFSM_HasStates
 *  12.  SubgraphMigrator_ValidateSubgraphRefs_EmptySubgraphs_OK
 *  13.  SubgraphMigrator_ValidateSubgraphRefs_MissingUUID_Fails
 *  14.  SubgraphMigrator_ValidateSubgraphRefs_CircularDep_Fails
 *  15.  SubgraphMigrator_ValidateSubgraphRefs_ValidChain_OK
 *  16.  SubgraphMigrator_ValidateSubgraphRefs_NotNewFormat_Fails
 *  17.  SubgraphMigrator_Migrate_LinksPreserved
 *  18.  SubgraphMigrator_Migrate_HFSM_TransitionsPreserved
 *
 * No SDL3, ImGui, or Editor dependency.
 * C++14 compliant — no C++17/20 features.
 */

#include "BlueprintEditor/SubgraphMigrator.h"
#include "third_party/nlohmann/json.hpp"

#include <iostream>
#include <string>

using json = nlohmann::json;
using namespace Olympe;

// ---------------------------------------------------------------------------
// Test infrastructure (same pattern as Phase7Test.cpp)
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
// Helper: build a minimal legacy BehaviorTree blueprint JSON
// ---------------------------------------------------------------------------

static json MakeLegacyBT()
{
    json bt;
    bt["schema_version"] = 2;
    bt["blueprintType"]  = "BehaviorTree";
    bt["name"]           = "TestTree";
    bt["data"]["rootNodeId"] = 1;
    bt["data"]["nodes"] = json::array();
    json node;
    node["id"]   = 1;
    node["type"] = "Sequence";
    node["name"] = "Root";
    bt["data"]["nodes"].push_back(node);
    bt["data"]["links"] = json::array();
    return bt;
}

// ---------------------------------------------------------------------------
// Helper: build a minimal legacy HFSM blueprint JSON
// ---------------------------------------------------------------------------

static json MakeLegacyHFSM()
{
    json hfsm;
    hfsm["schema_version"] = 2;
    hfsm["blueprintType"]  = "HFSM";
    hfsm["name"]           = "TestFSM";
    hfsm["data"]["initialState"] = "Idle";
    hfsm["data"]["states"] = json::array();
    json state;
    state["id"]   = "Idle";
    state["name"] = "Idle";
    hfsm["data"]["states"].push_back(state);
    hfsm["data"]["transitions"] = json::array();
    return hfsm;
}

// ---------------------------------------------------------------------------
// Helper: build an already-migrated blueprint JSON (Phase 8 format)
// ---------------------------------------------------------------------------

static json MakeNewFormatBT()
{
    json bt = MakeLegacyBT();
    SubgraphMigrator m;
    return m.Migrate(bt);
}

// ===========================================================================
// Tests
// ===========================================================================

static bool Test_NeedsMigration_LegacyBT()
{
    SubgraphMigrator m;
    bool result = m.NeedsMigration(MakeLegacyBT());
    TEST_ASSERT(result, "NeedsMigration should return true for legacy BT");
    return result;
}

static bool Test_NeedsMigration_NewFormat_False()
{
    SubgraphMigrator m;
    json newFmt = MakeNewFormatBT();
    bool result = !m.NeedsMigration(newFmt);
    TEST_ASSERT(result, "NeedsMigration should return false for new format");
    return result;
}

static bool Test_NeedsMigration_HFSM()
{
    SubgraphMigrator m;
    bool result = m.NeedsMigration(MakeLegacyHFSM());
    TEST_ASSERT(result, "NeedsMigration should return true for legacy HFSM");
    return result;
}

static bool Test_IsNewFormat_DetectsRootGraph()
{
    SubgraphMigrator m;
    json newFmt = MakeNewFormatBT();
    bool result = m.IsNewFormat(newFmt);
    TEST_ASSERT(result, "IsNewFormat should return true after migration");
    return result;
}

static bool Test_Migrate_BT_NodesMovedToRootGraph()
{
    SubgraphMigrator m;
    json migrated = m.Migrate(MakeLegacyBT());

    bool hasRootGraph = migrated["data"].contains("rootGraph");
    bool nodesInRootGraph = hasRootGraph &&
                            migrated["data"]["rootGraph"].contains("nodes");
    bool nodesNotInData   = !migrated["data"].contains("nodes");

    TEST_ASSERT(hasRootGraph,       "data.rootGraph should exist after migration");
    TEST_ASSERT(nodesInRootGraph,   "data.rootGraph.nodes should exist");
    TEST_ASSERT(nodesNotInData,     "data.nodes should be removed after migration");

    return hasRootGraph && nodesInRootGraph && nodesNotInData;
}

static bool Test_Migrate_BT_SubgraphsDictCreated()
{
    SubgraphMigrator m;
    json migrated = m.Migrate(MakeLegacyBT());

    bool hasSubgraphs = migrated["data"].contains("subgraphs");
    bool subgraphsIsObj = hasSubgraphs && migrated["data"]["subgraphs"].is_object();

    TEST_ASSERT(hasSubgraphs,    "data.subgraphs should exist after migration");
    TEST_ASSERT(subgraphsIsObj,  "data.subgraphs should be a JSON object");

    return hasSubgraphs && subgraphsIsObj;
}

static bool Test_Migrate_BT_SchemaVersionBumped()
{
    SubgraphMigrator m;
    json migrated = m.Migrate(MakeLegacyBT());

    bool hasVersion = migrated.contains("schema_version");
    bool versionIs5 = hasVersion && migrated["schema_version"].get<int>() >= 5;

    TEST_ASSERT(versionIs5, "schema_version should be >= 5 after Phase 8 migration");

    return versionIs5;
}

static bool Test_Migrate_HFSM_StatesMovedToRootGraph()
{
    SubgraphMigrator m;
    json migrated = m.Migrate(MakeLegacyHFSM());

    bool hasRootGraph   = migrated["data"].contains("rootGraph");
    bool statesInRoot   = hasRootGraph && migrated["data"]["rootGraph"].contains("states");
    bool statesNotInData = !migrated["data"].contains("states");

    TEST_ASSERT(hasRootGraph,    "data.rootGraph should exist after HFSM migration");
    TEST_ASSERT(statesInRoot,    "data.rootGraph.states should exist");
    TEST_ASSERT(statesNotInData, "data.states should be removed after migration");

    return hasRootGraph && statesInRoot && statesNotInData;
}

static bool Test_Migrate_AlreadyNew_ReturnsUnchanged()
{
    SubgraphMigrator m;
    json newFmt   = MakeNewFormatBT();
    json migrated = m.Migrate(newFmt);

    bool stillNewFormat = m.IsNewFormat(migrated);
    TEST_ASSERT(stillNewFormat, "Re-migrating new format should return unchanged");
    return stillNewFormat;
}

static bool Test_MakeEmptySubgraph_BT_HasRequiredFields()
{
    json sg = SubgraphMigrator::MakeEmptySubgraph("MySubGraph", "sg_uuid_1", "BehaviorTree");

    bool hasName  = sg.contains("name") && sg["name"] == "MySubGraph";
    bool hasUUID  = sg.contains("uuid") && sg["uuid"] == "sg_uuid_1";
    bool hasNodes = sg.contains("nodes") && sg["nodes"].is_array();
    bool hasLinks = sg.contains("links") && sg["links"].is_array();

    TEST_ASSERT(hasName,  "Empty subgraph should have 'name'");
    TEST_ASSERT(hasUUID,  "Empty subgraph should have 'uuid'");
    TEST_ASSERT(hasNodes, "Empty BT subgraph should have 'nodes' array");
    TEST_ASSERT(hasLinks, "Empty BT subgraph should have 'links' array");

    return hasName && hasUUID && hasNodes && hasLinks;
}

static bool Test_MakeEmptySubgraph_HFSM_HasStates()
{
    json sg = SubgraphMigrator::MakeEmptySubgraph("FSMSub", "sg_uuid_2", "HFSM");

    bool hasStates = sg.contains("states") && sg["states"].is_array();
    bool hasTransitions = sg.contains("transitions") && sg["transitions"].is_array();

    TEST_ASSERT(hasStates,      "Empty HFSM subgraph should have 'states' array");
    TEST_ASSERT(hasTransitions, "Empty HFSM subgraph should have 'transitions' array");

    return hasStates && hasTransitions;
}

static bool Test_ValidateSubgraphRefs_EmptySubgraphs_OK()
{
    json bp = MakeNewFormatBT();
    // No subgraph references — should pass.
    std::string err;
    bool ok = SubgraphMigrator::ValidateSubgraphReferences(bp, err);
    TEST_ASSERT(ok, "Validation should pass with no subgraph references");
    return ok;
}

static bool Test_ValidateSubgraphRefs_MissingUUID_Fails()
{
    json bp = MakeNewFormatBT();

    // Add a SubGraph node referencing a non-existent UUID.
    json sgNode;
    sgNode["id"]           = 99;
    sgNode["type"]         = "SubGraph";
    sgNode["name"]         = "CallSub";
    sgNode["subgraphUUID"] = "non_existent_uuid";
    bp["data"]["rootGraph"]["nodes"].push_back(sgNode);

    std::string err;
    bool ok = SubgraphMigrator::ValidateSubgraphReferences(bp, err);
    TEST_ASSERT(!ok, "Validation should fail when UUID is not in subgraphs dict");
    return !ok;
}

static bool Test_ValidateSubgraphRefs_CircularDep_Fails()
{
    json bp = MakeNewFormatBT();

    // A → B → A  (circular)
    json sgA;
    sgA["nodes"] = json::array();
    json nodeInA;
    nodeInA["id"] = 1;
    nodeInA["type"] = "SubGraph";
    nodeInA["subgraphUUID"] = "sg_B";
    sgA["nodes"].push_back(nodeInA);
    bp["data"]["subgraphs"]["sg_A"] = sgA;

    json sgB;
    sgB["nodes"] = json::array();
    json nodeInB;
    nodeInB["id"] = 1;
    nodeInB["type"] = "SubGraph";
    nodeInB["subgraphUUID"] = "sg_A";
    sgB["nodes"].push_back(nodeInB);
    bp["data"]["subgraphs"]["sg_B"] = sgB;

    std::string err;
    bool ok = SubgraphMigrator::ValidateSubgraphReferences(bp, err);
    TEST_ASSERT(!ok, "Validation should fail for circular dependency A→B→A");
    return !ok;
}

static bool Test_ValidateSubgraphRefs_ValidChain_OK()
{
    json bp = MakeNewFormatBT();

    // A → B  (no cycle)
    json sgB;
    sgB["nodes"] = json::array();
    bp["data"]["subgraphs"]["sg_B"] = sgB;

    json sgA;
    sgA["nodes"] = json::array();
    json nodeInA;
    nodeInA["id"] = 1;
    nodeInA["type"] = "SubGraph";
    nodeInA["subgraphUUID"] = "sg_B";
    sgA["nodes"].push_back(nodeInA);
    bp["data"]["subgraphs"]["sg_A"] = sgA;

    std::string err;
    bool ok = SubgraphMigrator::ValidateSubgraphReferences(bp, err);
    TEST_ASSERT(ok, "Validation should pass for a valid A→B chain");
    return ok;
}

static bool Test_ValidateSubgraphRefs_NotNewFormat_Fails()
{
    // Blueprint NOT in new format.
    std::string err;
    bool ok = SubgraphMigrator::ValidateSubgraphReferences(MakeLegacyBT(), err);
    TEST_ASSERT(!ok, "Validation should fail for legacy format (no rootGraph)");
    return !ok;
}

static bool Test_Migrate_LinksPreserved()
{
    json bt = MakeLegacyBT();
    json link;
    link["from"] = 1;
    link["to"]   = 2;
    bt["data"]["links"].push_back(link);

    SubgraphMigrator m;
    json migrated = m.Migrate(bt);

    bool linksInRoot = migrated["data"]["rootGraph"].contains("links") &&
                       migrated["data"]["rootGraph"]["links"].is_array() &&
                       migrated["data"]["rootGraph"]["links"].size() == 1;

    TEST_ASSERT(linksInRoot, "Links should be preserved inside data.rootGraph after migration");
    return linksInRoot;
}

static bool Test_Migrate_HFSM_TransitionsPreserved()
{
    json hfsm = MakeLegacyHFSM();
    json trans;
    trans["from"]    = "Idle";
    trans["to"]      = "Alert";
    trans["trigger"] = "EnemySeen";
    hfsm["data"]["transitions"].push_back(trans);

    SubgraphMigrator m;
    json migrated = m.Migrate(hfsm);

    bool transInRoot = migrated["data"]["rootGraph"].contains("transitions") &&
                       migrated["data"]["rootGraph"]["transitions"].is_array() &&
                       migrated["data"]["rootGraph"]["transitions"].size() == 1;

    TEST_ASSERT(transInRoot,
                "Transitions should be preserved inside data.rootGraph after HFSM migration");
    return transInRoot;
}

// ===========================================================================
// main
// ===========================================================================

int main()
{
    std::cout << "===== Phase 8 SubgraphMigrator Tests =====" << std::endl;

    s_passCount = 0;
    s_failCount = 0;

    ReportTest("SubgraphMigrator_NeedsMigration_LegacyBTFormat",        Test_NeedsMigration_LegacyBT());
    ReportTest("SubgraphMigrator_NeedsMigration_NewFormat_ReturnsFalse", Test_NeedsMigration_NewFormat_False());
    ReportTest("SubgraphMigrator_NeedsMigration_HFSMFormat",             Test_NeedsMigration_HFSM());
    ReportTest("SubgraphMigrator_IsNewFormat_DetectsRootGraph",          Test_IsNewFormat_DetectsRootGraph());
    ReportTest("SubgraphMigrator_Migrate_BT_NodesMovedToRootGraph",      Test_Migrate_BT_NodesMovedToRootGraph());
    ReportTest("SubgraphMigrator_Migrate_BT_SubgraphsDictCreated",       Test_Migrate_BT_SubgraphsDictCreated());
    ReportTest("SubgraphMigrator_Migrate_BT_SchemaVersionBumped",        Test_Migrate_BT_SchemaVersionBumped());
    ReportTest("SubgraphMigrator_Migrate_HFSM_StatesMovedToRootGraph",   Test_Migrate_HFSM_StatesMovedToRootGraph());
    ReportTest("SubgraphMigrator_Migrate_AlreadyNew_ReturnsUnchanged",   Test_Migrate_AlreadyNew_ReturnsUnchanged());
    ReportTest("SubgraphMigrator_MakeEmptySubgraph_BT_HasRequiredFields",Test_MakeEmptySubgraph_BT_HasRequiredFields());
    ReportTest("SubgraphMigrator_MakeEmptySubgraph_HFSM_HasStates",      Test_MakeEmptySubgraph_HFSM_HasStates());
    ReportTest("SubgraphMigrator_ValidateSubgraphRefs_EmptySubgraphs_OK",Test_ValidateSubgraphRefs_EmptySubgraphs_OK());
    ReportTest("SubgraphMigrator_ValidateSubgraphRefs_MissingUUID_Fails",Test_ValidateSubgraphRefs_MissingUUID_Fails());
    ReportTest("SubgraphMigrator_ValidateSubgraphRefs_CircularDep_Fails",Test_ValidateSubgraphRefs_CircularDep_Fails());
    ReportTest("SubgraphMigrator_ValidateSubgraphRefs_ValidChain_OK",    Test_ValidateSubgraphRefs_ValidChain_OK());
    ReportTest("SubgraphMigrator_ValidateSubgraphRefs_NotNewFormat_Fails",Test_ValidateSubgraphRefs_NotNewFormat_Fails());
    ReportTest("SubgraphMigrator_Migrate_LinksPreserved",                Test_Migrate_LinksPreserved());
    ReportTest("SubgraphMigrator_Migrate_HFSM_TransitionsPreserved",     Test_Migrate_HFSM_TransitionsPreserved());

    std::cout << "\n===== Results: "
              << s_passCount << " passed, "
              << s_failCount << " failed =====" << std::endl;

    return (s_failCount == 0) ? 0 : 1;
}
