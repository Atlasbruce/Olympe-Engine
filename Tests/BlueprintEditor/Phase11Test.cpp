/**
 * @file Phase11Test.cpp
 * @brief Tests for Phase 11 — Visual Script Editor Save Fixes.
 * @author Olympe Engine
 * @date 2026-03-11
 *
 * @details
 * Validates the four critical save-flow fixes:
 *
 *  1.  SaveExtension_JsonFile_PreservesJsonExtension
 *  2.  SaveExtension_AtsFile_PreservesAtsExtension
 *  3.  SaveExtension_EmptyPath_DefaultsToAts
 *  4.  SaveExtension_NoExtension_DefaultsToAts
 *  5.  BuildSavePath_DirectoryFilenameExtension_CorrectPath
 *  6.  BuildSavePath_RoundTrip_JsonExtensionPreserved
 *  7.  GraphTypeDetection_JsonWithGraphTypeField_ReturnsGraphType
 *  8.  GraphTypeDetection_JsonWithTypeField_ReturnsType
 *  9.  GraphTypeDetection_JsonWithBothFields_PrefersGraphType
 * 10.  GraphTypeDetection_NoTypeFields_ReturnsUnknown
 * 11.  ExtractSaveFilename_WithExtension_StripsIt
 * 12.  ExtractSaveFilename_EmptyPath_ReturnsDefault
 *
 * No SDL3, ImGui, or ImNodes dependency.
 * C++14 compliant.
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>

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

// ---------------------------------------------------------------------------
// Mirror of the extension-derivation logic added to RenderSaveAsDialog().
// When the Save As dialog opens, it derives the file extension from the
// currently loaded path so that .json files are saved as .json (not .ats).
//
// NOTE: Intentionally mirrors the inline logic in
// VisualScriptEditorPanel.cpp::RenderSaveAsDialog(), following the headless-
// test pattern used throughout Phase10Test.cpp.
// ---------------------------------------------------------------------------

static std::string DeriveExtensionFromPath(const std::string& currentPath)
{
    if (currentPath.empty())
        return ".ats";

    size_t dotPos = currentPath.rfind('.');
    if (dotPos == std::string::npos)
        return ".ats";

    return currentPath.substr(dotPos);
}

// ---------------------------------------------------------------------------
// Mirror of the full-path construction used in RenderSaveAsDialog().
// ---------------------------------------------------------------------------

static std::string BuildSaveAsFullPath(const std::string& directory,
                                       const std::string& filename,
                                       const std::string& ext)
{
    return directory + "/" + filename + ext;
}

// ---------------------------------------------------------------------------
// Mirror of the graphType-first detection added to DetectAssetType() in
// blueprinteditor.cpp (priority 1 check for "graphType" field).
// ---------------------------------------------------------------------------

// Minimal JSON-like stub: just a map of key -> string value for top-level fields
using JsonStub = std::unordered_map<std::string, std::string>;

static std::string DetectTypeFromJsonStub(const JsonStub& j)
{
    // Priority 1: graphType field (VS v4 / ATS format) — NEW FIX
    auto gtIt = j.find("graphType");
    if (gtIt != j.end() && !gtIt->second.empty())
        return gtIt->second;

    // Priority 2: type field (v1 + v2)
    auto tIt = j.find("type");
    if (tIt != j.end() && !tIt->second.empty())
        return tIt->second;

    return "Unknown";
}

// ---------------------------------------------------------------------------
// Mirror of ExtractSaveAsFilename used in RenderSaveAsDialog().
// ---------------------------------------------------------------------------

static std::string ExtractSaveFilename(const std::string& currentPath)
{
    if (currentPath.empty())
        return "untitled_graph";

    size_t lastSlash = currentPath.find_last_of("/\\");
    std::string fname = (lastSlash != std::string::npos)
                        ? currentPath.substr(lastSlash + 1)
                        : currentPath;
    size_t dotPos = fname.rfind('.');
    if (dotPos != std::string::npos)
        fname = fname.substr(0, dotPos);
    return fname;
}

// ---------------------------------------------------------------------------
// Test 1: .json file → extension preserved as .json
// ---------------------------------------------------------------------------

static void Test1_SaveExtension_JsonFile_PreservesJsonExtension()
{
    int prevFail = s_failCount;

    TEST_ASSERT(DeriveExtensionFromPath("Blueprints/AI/guard.json") == ".json",
                ".json extension should be preserved");
    TEST_ASSERT(DeriveExtensionFromPath("guardV2_ai - VS-ATS.json") == ".json",
                ".json extension with spaces should be preserved");
    TEST_ASSERT(DeriveExtensionFromPath("C:\\Blueprints\\AI\\guard.json") == ".json",
                "Windows path .json extension should be preserved");

    ReportTest("Test1_SaveExtension_JsonFile_PreservesJsonExtension",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 2: .ats file → extension preserved as .ats
// ---------------------------------------------------------------------------

static void Test2_SaveExtension_AtsFile_PreservesAtsExtension()
{
    int prevFail = s_failCount;

    TEST_ASSERT(DeriveExtensionFromPath("Blueprints/AI/guard.ats") == ".ats",
                ".ats extension should be preserved");
    TEST_ASSERT(DeriveExtensionFromPath("Gamedata/TaskGraph/Examples/patrol.ats") == ".ats",
                "nested .ats path should preserve extension");

    ReportTest("Test2_SaveExtension_AtsFile_PreservesAtsExtension",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 3: empty path → defaults to .ats
// ---------------------------------------------------------------------------

static void Test3_SaveExtension_EmptyPath_DefaultsToAts()
{
    int prevFail = s_failCount;

    TEST_ASSERT(DeriveExtensionFromPath("") == ".ats",
                "Empty path should default to .ats");

    ReportTest("Test3_SaveExtension_EmptyPath_DefaultsToAts",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 4: path with no extension → defaults to .ats
// ---------------------------------------------------------------------------

static void Test4_SaveExtension_NoExtension_DefaultsToAts()
{
    int prevFail = s_failCount;

    TEST_ASSERT(DeriveExtensionFromPath("Blueprints/AI/guard") == ".ats",
                "Path without extension should default to .ats");
    TEST_ASSERT(DeriveExtensionFromPath("simple") == ".ats",
                "Filename without extension should default to .ats");

    ReportTest("Test4_SaveExtension_NoExtension_DefaultsToAts",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 5: BuildSaveAsFullPath correctly joins components
// ---------------------------------------------------------------------------

static void Test5_BuildSavePath_DirectoryFilenameExtension_CorrectPath()
{
    int prevFail = s_failCount;

    std::string result = BuildSaveAsFullPath("Blueprints/AI", "guard", ".json");
    TEST_ASSERT(result == "Blueprints/AI/guard.json",
                "Should produce 'Blueprints/AI/guard.json'");

    result = BuildSaveAsFullPath("Gamedata/TaskGraph/Examples", "patrol_v2", ".ats");
    TEST_ASSERT(result == "Gamedata/TaskGraph/Examples/patrol_v2.ats",
                "Should produce nested .ats path");

    ReportTest("Test5_BuildSavePath_DirectoryFilenameExtension_CorrectPath",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 6: End-to-end: open a .json file, compute full save-as path
// ---------------------------------------------------------------------------

static void Test6_BuildSavePath_RoundTrip_JsonExtensionPreserved()
{
    int prevFail = s_failCount;

    // Simulate the scenario: user opens guardV2_ai - VS-ATS.json and clicks
    // Save As.  The dialog should NOT rename it to .ats.
    const std::string currentPath = "Blueprints/AI/guardV2_ai - VS-ATS.json";
    std::string ext       = DeriveExtensionFromPath(currentPath);
    std::string filename  = ExtractSaveFilename(currentPath);
    std::string directory = "Blueprints/AI";
    std::string fullPath  = BuildSaveAsFullPath(directory, filename, ext);

    TEST_ASSERT(ext == ".json",        "Extension should be .json");
    TEST_ASSERT(filename == "guardV2_ai - VS-ATS", "Filename without extension");
    TEST_ASSERT(fullPath == "Blueprints/AI/guardV2_ai - VS-ATS.json",
                "Full path should preserve .json extension");

    ReportTest("Test6_BuildSavePath_RoundTrip_JsonExtensionPreserved",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 7: graphType field → returned as type (priority 1)
// ---------------------------------------------------------------------------

static void Test7_GraphTypeDetection_JsonWithGraphTypeField_ReturnsGraphType()
{
    int prevFail = s_failCount;

    JsonStub j;
    j["graphType"]     = "VisualScript";
    j["schema_version"] = "4";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "VisualScript",
                "graphType field should be returned directly");

    j.clear();
    j["graphType"] = "BehaviorTree";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "BehaviorTree",
                "graphType BehaviorTree should be returned");

    ReportTest("Test7_GraphTypeDetection_JsonWithGraphTypeField_ReturnsGraphType",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 8: type field (no graphType) → returned (priority 2)
// ---------------------------------------------------------------------------

static void Test8_GraphTypeDetection_JsonWithTypeField_ReturnsType()
{
    int prevFail = s_failCount;

    JsonStub j;
    j["type"] = "VisualScript";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "VisualScript",
                "type field should be returned when graphType absent");

    j.clear();
    j["type"] = "BehaviorTree";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "BehaviorTree",
                "type BehaviorTree without graphType");

    ReportTest("Test8_GraphTypeDetection_JsonWithTypeField_ReturnsType",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 9: both graphType and type → graphType wins (priority 1)
// ---------------------------------------------------------------------------

static void Test9_GraphTypeDetection_JsonWithBothFields_PrefersGraphType()
{
    int prevFail = s_failCount;

    JsonStub j;
    j["graphType"] = "VisualScript";
    j["type"]      = "VisualScript";   // same value — should still work
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "VisualScript",
                "graphType should win when both present");

    // Simulate a hypothetical conflict: graphType wins
    j["graphType"] = "VisualScript";
    j["type"]      = "BehaviorTree";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "VisualScript",
                "graphType should win over type on conflict");

    ReportTest("Test9_GraphTypeDetection_JsonWithBothFields_PrefersGraphType",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 10: no type fields → Unknown
// ---------------------------------------------------------------------------

static void Test10_GraphTypeDetection_NoTypeFields_ReturnsUnknown()
{
    int prevFail = s_failCount;

    JsonStub j;
    j["schema_version"] = "4";
    j["name"]           = "MyGraph";
    TEST_ASSERT(DetectTypeFromJsonStub(j) == "Unknown",
                "No type fields should return Unknown from this stub");

    TEST_ASSERT(DetectTypeFromJsonStub({}) == "Unknown",
                "Empty stub returns Unknown");

    ReportTest("Test10_GraphTypeDetection_NoTypeFields_ReturnsUnknown",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 11: ExtractSaveFilename strips extension correctly
// ---------------------------------------------------------------------------

static void Test11_ExtractSaveFilename_WithExtension_StripsIt()
{
    int prevFail = s_failCount;

    TEST_ASSERT(ExtractSaveFilename("Blueprints/AI/patrol_v2.ats") == "patrol_v2",
                "Unix path: extension stripped");
    TEST_ASSERT(ExtractSaveFilename("Blueprints/AI/guardV2_ai - VS-ATS.json") == "guardV2_ai - VS-ATS",
                "Path with spaces: extension stripped");
    TEST_ASSERT(ExtractSaveFilename("C:\\Blueprints\\guard.ats") == "guard",
                "Windows path: extension stripped");
    TEST_ASSERT(ExtractSaveFilename("Blueprints/AI/simple") == "simple",
                "No extension: filename returned as-is");

    ReportTest("Test11_ExtractSaveFilename_WithExtension_StripsIt",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Test 12: ExtractSaveFilename returns default for empty path
// ---------------------------------------------------------------------------

static void Test12_ExtractSaveFilename_EmptyPath_ReturnsDefault()
{
    int prevFail = s_failCount;

    TEST_ASSERT(ExtractSaveFilename("") == "untitled_graph",
                "Empty path should return 'untitled_graph'");

    ReportTest("Test12_ExtractSaveFilename_EmptyPath_ReturnsDefault",
               s_failCount == prevFail);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    std::cout << "=============================\n";
    std::cout << "   Phase 11 Unit Tests       \n";
    std::cout << "   VS Editor Save Fixes      \n";
    std::cout << "=============================\n";

    Test1_SaveExtension_JsonFile_PreservesJsonExtension();
    Test2_SaveExtension_AtsFile_PreservesAtsExtension();
    Test3_SaveExtension_EmptyPath_DefaultsToAts();
    Test4_SaveExtension_NoExtension_DefaultsToAts();
    Test5_BuildSavePath_DirectoryFilenameExtension_CorrectPath();
    Test6_BuildSavePath_RoundTrip_JsonExtensionPreserved();
    Test7_GraphTypeDetection_JsonWithGraphTypeField_ReturnsGraphType();
    Test8_GraphTypeDetection_JsonWithTypeField_ReturnsType();
    Test9_GraphTypeDetection_JsonWithBothFields_PrefersGraphType();
    Test10_GraphTypeDetection_NoTypeFields_ReturnsUnknown();
    Test11_ExtractSaveFilename_WithExtension_StripsIt();
    Test12_ExtractSaveFilename_EmptyPath_ReturnsDefault();

    std::cout << "=============================\n";
    std::cout << "Results: " << s_passCount << " passed, "
              << s_failCount << " failed.\n";

    return s_failCount > 0 ? 1 : 0;
}
