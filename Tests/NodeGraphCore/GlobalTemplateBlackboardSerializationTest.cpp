/**
 * @file GlobalTemplateBlackboardSerializationTest.cpp
 * @brief Test suite for GlobalTemplateBlackboard SaveToFile / LoadFromFile
 * @author Test Framework
 * @date 2026-03-26
 */

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

#include "../../Source/NodeGraphCore/GlobalTemplateBlackboard.h"
#include "../../Source/json_helper.h"

using json = nlohmann::json;

namespace Olympe {

static int s_passCount = 0;
static int s_failCount = 0;

void ReportTest(const std::string& name, bool passed)
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

// ============================================================================
// Test 1: Add variable and save to file
// ============================================================================

void TestA_AddAndSave()
{
    std::cout << "\nTest A: Add variable and save to file..." << std::endl;

    const std::string testPath = "./test_global_blackboard.json";

    // Clear any existing test file
    std::remove(testPath.c_str());

    // Create a fresh instance by using a new registry
    GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
    gtb.Clear();

    // Add a test variable
    bool added = gtb.AddVariable(
        "testVar",
        VariableType::Float,
        TaskValue(42.5f),
        "Test variable for serialization",
        true  // isPersistent
    );

    bool testPassed = true;
    if (!added)
    {
        std::cout << "  ERROR: Failed to add variable\n";
        testPassed = false;
    }

    // Save to file
    bool saved = gtb.SaveToFile(testPath);
    if (!saved)
    {
        std::cout << "  ERROR: Failed to save to file\n";
        testPassed = false;
    }

    // Verify file was created
    std::ifstream ifs(testPath);
    if (!ifs.is_open())
    {
        std::cout << "  ERROR: File was not created\n";
        testPassed = false;
    }
    else
    {
        ifs.close();
    }

    // Verify JSON structure
    try
    {
        std::ifstream jsonFile(testPath);
        json root;
        jsonFile >> root;
        jsonFile.close();

        if (!root.contains("variables") || !root["variables"].is_array())
        {
            std::cout << "  ERROR: JSON missing variables array\n";
            testPassed = false;
        }
        else
        {
            const json& varsArray = root["variables"];
            if (varsArray.size() != 1)
            {
                std::cout << "  ERROR: Expected 1 variable, got " << varsArray.size() << "\n";
                testPassed = false;
            }
            else
            {
                const json& var = varsArray[0];
                if (var["key"].get<std::string>() != "testVar")
                {
                    std::cout << "  ERROR: Variable key mismatch\n";
                    testPassed = false;
                }
                if (var["type"].get<std::string>() != "Float")
                {
                    std::cout << "  ERROR: Variable type mismatch\n";
                    testPassed = false;
                }
                if (std::abs(var["value"].get<float>() - 42.5f) > 0.001f)
                {
                    std::cout << "  ERROR: Variable value mismatch\n";
                    testPassed = false;
                }
            }
        }

        // Verify metadata fields
        if (!root.contains("schema_version"))
        {
            std::cout << "  WARNING: Missing schema_version\n";
        }
        if (!root.contains("lastModified"))
        {
            std::cout << "  WARNING: Missing lastModified\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "  ERROR: JSON parsing failed: " << e.what() << "\n";
        testPassed = false;
    }

    ReportTest("TestA_AddAndSave", testPassed);

    // Cleanup
    std::remove(testPath.c_str());
}

// ============================================================================
// Test 2: Load and verify round-trip
// ============================================================================

void TestB_LoadAndVerify()
{
    std::cout << "\nTest B: Load from file and verify round-trip..." << std::endl;

    const std::string testPath = "./test_round_trip.json";
    std::remove(testPath.c_str());

    bool testPassed = true;

    // Create original data
    {
        GlobalTemplateBlackboard& gtb1 = GlobalTemplateBlackboard::Get();
        gtb1.Clear();

        gtb1.AddVariable("var1", VariableType::Int, TaskValue(100), "First var", false);
        gtb1.AddVariable("var2", VariableType::String, TaskValue(std::string("hello")), "Second var", true);

        if (!gtb1.SaveToFile(testPath))
        {
            std::cout << "  ERROR: Failed to save\n";
            testPassed = false;
        }
    }

    // Load data
    {
        GlobalTemplateBlackboard& gtb2 = GlobalTemplateBlackboard::Get();
        gtb2.Clear();

        if (!gtb2.LoadFromFile(testPath))
        {
            std::cout << "  ERROR: Failed to load\n";
            testPassed = false;
        }

        if (gtb2.GetVariableCount() != 2)
        {
            std::cout << "  ERROR: Expected 2 variables, got " << gtb2.GetVariableCount() << "\n";
            testPassed = false;
        }

        const auto* var1 = gtb2.GetVariable("var1");
        if (!var1 || var1->Type != VariableType::Int || var1->DefaultValue.AsInt() != 100)
        {
            std::cout << "  ERROR: var1 mismatch\n";
            testPassed = false;
        }

        const auto* var2 = gtb2.GetVariable("var2");
        if (!var2 || var2->Type != VariableType::String || var2->DefaultValue.AsString() != "hello")
        {
            std::cout << "  ERROR: var2 mismatch\n";
            testPassed = false;
        }
    }

    ReportTest("TestB_LoadAndVerify", testPassed);

    std::remove(testPath.c_str());
}

// ============================================================================
// Test 3: Preserve metadata when updating
// ============================================================================

void TestC_PreserveMetadata()
{
    std::cout << "\nTest C: Preserve metadata when updating..." << std::endl;

    const std::string testPath = "./test_metadata.json";
    std::remove(testPath.c_str());

    bool testPassed = true;

    // Create initial file with custom metadata
    {
        json root;
        root["schema_version"] = 1;
        root["name"] = "Custom Registry";
        root["description"] = "Custom description";
        root["variables"] = json::array();

        std::ofstream ofs(testPath);
        ofs << root.dump(2);
        ofs.close();
    }

    // Add a variable (which should preserve metadata)
    {
        GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
        gtb.Clear();
        gtb.LoadFromFile(testPath);
        gtb.AddVariable("newVar", VariableType::Bool, TaskValue(true), "New variable", false);
        gtb.SaveToFile(testPath);
    }

    // Verify metadata was preserved
    {
        std::ifstream ifs(testPath);
        json root;
        ifs >> root;
        ifs.close();

        if (root.value("name", "") != "Custom Registry")
        {
            std::cout << "  ERROR: Name was not preserved (got: " << root.value("name", "") << ")\n";
            testPassed = false;
        }
        if (root.value("description", "") != "Custom description")
        {
            std::cout << "  ERROR: Description was not preserved\n";
            testPassed = false;
        }
    }

    ReportTest("TestC_PreserveMetadata", testPassed);

    std::remove(testPath.c_str());
}

// ============================================================================
// Test 4: Handle missing file gracefully
// ============================================================================

void TestD_MissingFile()
{
    std::cout << "\nTest D: Handle missing file (new registry)..." << std::endl;

    const std::string testPath = "./test_new_registry.json";
    std::remove(testPath.c_str());

    bool testPassed = true;

    // SaveToFile should create default metadata for new file
    {
        GlobalTemplateBlackboard& gtb = GlobalTemplateBlackboard::Get();
        gtb.Clear();
        gtb.AddVariable("first", VariableType::Int, TaskValue(1), "First var", false);

        if (!gtb.SaveToFile(testPath))
        {
            std::cout << "  ERROR: Failed to save new file\n";
            testPassed = false;
        }

        // Verify defaults were set
        std::ifstream ifs(testPath);
        json root;
        ifs >> root;
        ifs.close();

        if (root.value("schema_version", 0) != 1)
        {
            std::cout << "  ERROR: Default schema_version not set\n";
            testPassed = false;
        }
        if (root.value("name", "") != "Global Blackboard Register")
        {
            std::cout << "  ERROR: Default name not set\n";
            testPassed = false;
        }
    }

    ReportTest("TestD_MissingFile", testPassed);

    std::remove(testPath.c_str());
}

} // namespace Olympe

// ============================================================================
// main
// ============================================================================

int main()
{
    std::cout << "=== GlobalTemplateBlackboardSerializationTest ===" << std::endl;

    Olympe::TestA_AddAndSave();
    Olympe::TestB_LoadAndVerify();
    Olympe::TestC_PreserveMetadata();
    Olympe::TestD_MissingFile();

    std::cout << std::endl;
    std::cout << "Results: " << Olympe::s_passCount << " passed, "
              << Olympe::s_failCount << " failed" << std::endl;

    return (Olympe::s_failCount == 0) ? 0 : 1;
}
