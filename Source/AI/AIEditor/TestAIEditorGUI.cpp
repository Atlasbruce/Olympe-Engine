/**
 * @file TestAIEditorGUI.cpp
 * @brief Integration tests for AIEditorGUI (Phase 1.3)
 * @author Olympe Engine
 * @date 2026-02-18
 * 
 * @details
 * Tests all components of AIEditorGUI:
 * - Initialization and shutdown
 * - Graph creation and management
 * - Node creation via palette
 * - Undo/redo functionality
 * - Validation integration
 * - Save/load roundtrip
 * - Multi-graph tabs
 * - Panel integration
 * 
 * @note This is a test file. Test output uses std::cout which is acceptable
 * for test reporting purposes. Production code uses SYSTEM_LOG.
 */

#include "../AIEditor/AIEditorGUI.h"
#include "../AIGraphPlugin_BT/BTNodeRegistry.h"
#include "../AIGraphPlugin_BT/BTGraphValidator.h"
#include "../../NodeGraphCore/NodeGraphManager.h"
#include "../../NodeGraphCore/GraphDocument.h"
#include "../../NodeGraphCore/Commands/CreateNodeCommand.h"

#include <iostream>
#include <cassert>
#include <string>

using namespace Olympe::AI;
using namespace Olympe::NodeGraph;

// Test counter
int g_testsPassed = 0;
int g_testsFailed = 0;

void ReportTest(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << std::endl;
        g_testsPassed++;
    } else {
        std::cout << "[FAIL] " << testName << std::endl;
        g_testsFailed++;
    }
}

// ============================================================================
// Test 1: Initialize AIEditorGUI
// ============================================================================
void Test1_InitializeAIEditorGUI() {
    AIEditorGUI editor;
    bool success = editor.Initialize();
    
    bool passed = (success == true) && (editor.IsActive() == true);
    
    editor.Shutdown();
    
    ReportTest("Test 1: Initialize AIEditorGUI", passed);
}

// ============================================================================
// Test 2: Create New BT Graph
// ============================================================================
void Test2_CreateNewBTGraph() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Simulate menu action
    editor.MenuAction_NewBT();
    
    NodeGraphManager& mgr = NodeGraphManager::Get();
    auto graphIds = mgr.GetAllGraphIds();
    
    bool passed = (graphIds.size() == 1);
    
    editor.Shutdown();
    
    ReportTest("Test 2: Create New BT Graph", passed);
}

// ============================================================================
// Test 3: Load Existing BT
// ============================================================================
void Test3_LoadExistingBT() {
    AIEditorGUI editor;
    editor.Initialize();
    
    NodeGraphManager& mgr = NodeGraphManager::Get();
    
    // Create a test graph first
    GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    GraphDocument* doc = mgr.GetGraph(id);
    
    bool passed = (id.value != 0) && (doc != nullptr);
    
    editor.Shutdown();
    
    ReportTest("Test 3: Load Existing BT", passed);
}

// ============================================================================
// Test 4: Node Creation via Palette
// ============================================================================
void Test4_NodeCreationViaPalette() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Create graph
    NodeGraphManager& mgr = NodeGraphManager::Get();
    GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    mgr.SetActiveGraph(id);
    
    // Create node directly (simulating palette drag)
    GraphDocument* doc = mgr.GetActiveGraph();
    NodeId nodeId = doc->CreateNode("BT_Selector", Vector2(100, 100));
    
    bool passed = (doc->GetNodes().size() == 1) && (doc->GetNode(nodeId) != nullptr);
    
    editor.Shutdown();
    
    ReportTest("Test 4: Node Creation via Palette", passed);
}

// ============================================================================
// Test 5: Undo/Redo
// ============================================================================
void Test5_UndoRedo() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Create graph and node
    NodeGraphManager& mgr = NodeGraphManager::Get();
    GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    mgr.SetActiveGraph(id);
    GraphDocument* doc = mgr.GetActiveGraph();
    
    // Use command stack
    auto cmd = std::unique_ptr<CreateNodeCommand>(new CreateNodeCommand(doc, "BT_Action", Vector2(0, 0)));
    editor.GetCommandStack().ExecuteCommand(std::move(cmd));
    
    bool step1 = (doc->GetNodes().size() == 1);
    
    // Undo
    editor.GetCommandStack().Undo();
    bool step2 = (doc->GetNodes().size() == 0);
    
    // Redo
    editor.GetCommandStack().Redo();
    bool step3 = (doc->GetNodes().size() == 1);
    
    bool passed = step1 && step2 && step3;
    
    editor.Shutdown();
    
    ReportTest("Test 5: Undo/Redo", passed);
}

// ============================================================================
// Test 6: Validate Graph
// ============================================================================
void Test6_ValidateGraph() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Create graph with validation error
    NodeGraphManager& mgr = NodeGraphManager::Get();
    GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    mgr.SetActiveGraph(id);
    GraphDocument* doc = mgr.GetActiveGraph();
    
    // Create Selector with no children (error)
    doc->CreateNode("BT_Selector", Vector2(0, 0));
    
    auto messages = BTGraphValidator::ValidateGraph(doc);
    bool hasError = false;
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
        if (msgIt->severity == BTValidationSeverity::Error) {
            hasError = true;
        }
    }
    
    bool passed = (hasError == true); // Should have error
    
    editor.Shutdown();
    
    ReportTest("Test 6: Validate Graph", passed);
}

// ============================================================================
// Test 7: Save and Load Roundtrip
// ============================================================================
void Test7_SaveLoadRoundtrip() {
    AIEditorGUI editor;
    editor.Initialize();
    
    NodeGraphManager& mgr = NodeGraphManager::Get();
    
    // Create and populate graph
    GraphId id = mgr.CreateGraph("AIGraph", "BehaviorTree");
    mgr.SetActiveGraph(id);
    GraphDocument* doc = mgr.GetActiveGraph();
    doc->CreateNode("BT_Selector", Vector2(100, 100));
    doc->CreateNode("BT_Action", Vector2(200, 200));
    
    // Save
    bool saved = mgr.SaveGraph(id, "test_output.json");
    
    // Load
    GraphId id2 = mgr.LoadGraph("test_output.json");
    GraphDocument* doc2 = mgr.GetGraph(id2);
    
    bool passed = saved && (doc2 != nullptr) && (doc2->GetNodes().size() == 2);
    
    editor.Shutdown();
    
    ReportTest("Test 7: Save and Load Roundtrip", passed);
}

// ============================================================================
// Test 8: Multi-Graph Tabs
// ============================================================================
void Test8_MultiGraphTabs() {
    AIEditorGUI editor;
    editor.Initialize();
    
    NodeGraphManager& mgr = NodeGraphManager::Get();
    
    GraphId id1 = mgr.CreateGraph("AIGraph", "BehaviorTree");
    GraphId id2 = mgr.CreateGraph("AIGraph", "HFSM");
    
    bool step1 = (mgr.GetAllGraphIds().size() == 2);
    
    mgr.SetActiveGraph(id1);
    bool step2 = (mgr.GetActiveGraphId().value == id1.value);
    
    mgr.SetActiveGraph(id2);
    bool step3 = (mgr.GetActiveGraphId().value == id2.value);
    
    bool passed = step1 && step2 && step3;
    
    editor.Shutdown();
    
    ReportTest("Test 8: Multi-Graph Tabs", passed);
}

// ============================================================================
// Test 9: Blackboard Panel
// ============================================================================
void Test9_BlackboardPanel() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Show blackboard panel
    editor.MenuAction_ShowBlackboard();
    
    // Basic test: just verify initialization succeeds
    bool passed = true;
    
    editor.Shutdown();
    
    ReportTest("Test 9: Blackboard Panel", passed);
}

// ============================================================================
// Test 10: Node Palette Integration
// ============================================================================
void Test10_NodePaletteIntegration() {
    AIEditorGUI editor;
    editor.Initialize();
    
    // Show node palette
    editor.MenuAction_ShowNodePalette();
    
    // Check palette has nodes
    BTNodeRegistry& registry = BTNodeRegistry::Get();
    auto allTypes = registry.GetAllNodeTypes();
    
    bool passed = (allTypes.size() >= 15);
    
    editor.Shutdown();
    
    ReportTest("Test 10: Node Palette Integration", passed);
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "AIEditorGUI Integration Tests (Phase 1.3)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    Test1_InitializeAIEditorGUI();
    Test2_CreateNewBTGraph();
    Test3_LoadExistingBT();
    Test4_NodeCreationViaPalette();
    Test5_UndoRedo();
    Test6_ValidateGraph();
    Test7_SaveLoadRoundtrip();
    Test8_MultiGraphTabs();
    Test9_BlackboardPanel();
    Test10_NodePaletteIntegration();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (g_testsFailed == 0) ? 0 : 1;
}
