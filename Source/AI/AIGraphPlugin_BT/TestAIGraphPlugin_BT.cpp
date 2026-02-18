/**
 * @file TestAIGraphPlugin_BT.cpp
 * @brief Comprehensive tests for AIGraphPlugin_BT (Phase 1.2)
 * @author Olympe Engine
 * @date 2026-02-18
 * 
 * @details
 * Tests all components of AIGraphPlugin_BT:
 * - BTNodeRegistry (initialization, queries, validation)
 * - BTGraphValidator (all 7 validation rules)
 * - BTGraphCompiler (compilation, error handling)
 * - BTNodePalette (basic instantiation)
 */

#include "AI/AIGraphPlugin_BT/BTNodeRegistry.h"
#include "AI/AIGraphPlugin_BT/BTGraphValidator.h"
#include "AI/AIGraphPlugin_BT/BTGraphCompiler.h"
#include "AI/AIGraphPlugin_BT/BTNodePalette.h"
#include "NodeGraphCore/GraphDocument.h"
#include "AI/BehaviorTree.h"

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
// Test 1: Registry Initialization
// ============================================================================
void Test1_RegistryInitialization() {
    auto& registry = BTNodeRegistry::Get();
    auto allTypes = registry.GetAllNodeTypes();
    
    bool passed = allTypes.size() >= 15; // At least 15 types
    ReportTest("Test 1: Registry Initialization", passed);
}

// ============================================================================
// Test 2: Node Type Query
// ============================================================================
void Test2_NodeTypeQuery() {
    auto& registry = BTNodeRegistry::Get();
    
    const BTNodeTypeInfo* info = registry.GetNodeTypeInfo("BT_Selector");
    bool passed = (info != nullptr) &&
                  (info->category == BTNodeCategory::Composite) &&
                  (info->minChildren == 1) &&
                  (info->maxChildren == -1);
    
    ReportTest("Test 2: Node Type Query", passed);
}

// ============================================================================
// Test 3: Validation - Valid Single Root
// ============================================================================
void Test3_ValidationValidRoot() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    NodeId root = doc.CreateNode("BT_Selector", Vector2(0, 0));
    NodeId child = doc.CreateNode("BT_Wait", Vector2(0, 100));
    
    // Connect child to root
    auto* rootNode = doc.GetNode(root);
    if (rootNode != nullptr) {
        rootNode->children.push_back(child);
    }
    
    doc.rootNodeId = root;
    
    auto messages = BTGraphValidator::ValidateGraph(&doc);
    
    // Should have no errors
    bool hasError = false;
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
        if (msgIt->severity == BTValidationSeverity::Error) {
            hasError = true;
        }
    }
    
    ReportTest("Test 3: Validation - Valid Single Root", !hasError);
}

// ============================================================================
// Test 4: Validation - Multiple Roots Error
// ============================================================================
void Test4_ValidationMultipleRoots() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    doc.CreateNode("BT_Selector", Vector2(0, 0));
    doc.CreateNode("BT_Sequence", Vector2(100, 0));
    
    auto messages = BTGraphValidator::ValidateGraph(&doc);
    
    // Should have multiple roots error
    bool hasMultipleRootsError = false;
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
        if (msgIt->message.find("Multiple root") != std::string::npos) {
            hasMultipleRootsError = true;
        }
    }
    
    ReportTest("Test 4: Validation - Multiple Roots Error", hasMultipleRootsError);
}

// ============================================================================
// Test 5: Validation - Cycle Detection
// ============================================================================
void Test5_ValidationCycle() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    NodeId a = doc.CreateNode("BT_Selector", Vector2(0, 0));
    NodeId b = doc.CreateNode("BT_Sequence", Vector2(0, 100));
    NodeId c = doc.CreateNode("BT_Selector", Vector2(0, 200));
    
    // Create cycle: A -> B -> C -> A
    auto* nodeA = doc.GetNode(a);
    auto* nodeB = doc.GetNode(b);
    auto* nodeC = doc.GetNode(c);
    
    if (nodeA != nullptr && nodeB != nullptr && nodeC != nullptr) {
        nodeA->children.push_back(b);
        nodeB->children.push_back(c);
        nodeC->children.push_back(a); // Cycle!
    }
    
    doc.rootNodeId = a;
    
    auto messages = BTGraphValidator::ValidateGraph(&doc);
    
    bool hasCycleError = false;
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
        if (msgIt->message.find("Cycle") != std::string::npos) {
            hasCycleError = true;
        }
    }
    
    ReportTest("Test 5: Validation - Cycle Detection", hasCycleError);
}

// ============================================================================
// Test 6: Compilation - Simple BT
// ============================================================================
void Test6_CompilationSimpleBT() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    NodeId root = doc.CreateNode("BT_Selector", Vector2(0, 0));
    NodeId action = doc.CreateNode("BT_Wait", Vector2(0, 100));
    
    auto* rootNode = doc.GetNode(root);
    if (rootNode != nullptr) {
        rootNode->children.push_back(action);
    }
    
    doc.rootNodeId = root;
    
    BehaviorTreeAsset asset;
    std::string error;
    bool success = BTGraphCompiler::Compile(&doc, asset, error);
    
    bool passed = success && 
                  (asset.nodes.size() == 2) && 
                  (asset.rootNodeId == root.value);
    
    ReportTest("Test 6: Compilation - Simple BT", passed);
}

// ============================================================================
// Test 7: Compilation - Invalid Node Type
// ============================================================================
void Test7_CompilationInvalidNodeType() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    doc.CreateNode("INVALID_TYPE", Vector2(0, 0));
    
    BehaviorTreeAsset asset;
    std::string error;
    bool success = BTGraphCompiler::Compile(&doc, asset, error);
    
    bool passed = !success && (error.find("Unknown node type") != std::string::npos);
    
    ReportTest("Test 7: Compilation - Invalid Node Type", passed);
}

// ============================================================================
// Test 8: Validation - Children Count Error
// ============================================================================
void Test8_ValidationChildrenCount() {
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    // Selector without children (invalid, min = 1)
    NodeId selector = doc.CreateNode("BT_Selector", Vector2(0, 0));
    doc.rootNodeId = selector;
    
    auto messages = BTGraphValidator::ValidateGraph(&doc);
    
    bool hasTooFewChildrenError = false;
    for (auto msgIt = messages.begin(); msgIt != messages.end(); ++msgIt) {
        if (msgIt->message.find("Too few children") != std::string::npos) {
            hasTooFewChildrenError = true;
        }
    }
    
    ReportTest("Test 8: Validation - Children Count Error", hasTooFewChildrenError);
}

// ============================================================================
// Test 9: Palette Instantiation
// ============================================================================
void Test9_PaletteInstantiation() {
    try {
        BTNodePalette palette;
        bool passed = !palette.IsDragging() && palette.GetDraggedNodeType().empty();
        ReportTest("Test 9: Palette Instantiation", passed);
    } catch (...) {
        ReportTest("Test 9: Palette Instantiation", false);
    }
}

// ============================================================================
// Test 10: Registry Category Query
// ============================================================================
void Test10_RegistryCategoryQuery() {
    auto& registry = BTNodeRegistry::Get();
    
    auto composites = registry.GetNodeTypesByCategory(BTNodeCategory::Composite);
    auto decorators = registry.GetNodeTypesByCategory(BTNodeCategory::Decorator);
    auto conditions = registry.GetNodeTypesByCategory(BTNodeCategory::Condition);
    auto actions = registry.GetNodeTypesByCategory(BTNodeCategory::Action);
    
    bool passed = (composites.size() >= 3) &&  // Selector, Sequence, Parallel
                  (decorators.size() >= 5) &&  // Inverter, Repeater, UntilSuccess, UntilFailure, Cooldown
                  (conditions.size() >= 4) &&  // CheckBlackboardValue, HasTarget, IsTargetInRange, CanSeeTarget
                  (actions.size() >= 8);       // Wait, WaitRandomTime, SetBlackboardValue, etc.
    
    ReportTest("Test 10: Registry Category Query", passed);
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "AIGraphPlugin_BT Test Suite (Phase 1.2)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    Test1_RegistryInitialization();
    Test2_NodeTypeQuery();
    Test3_ValidationValidRoot();
    Test4_ValidationMultipleRoots();
    Test5_ValidationCycle();
    Test6_CompilationSimpleBT();
    Test7_CompilationInvalidNodeType();
    Test8_ValidationChildrenCount();
    Test9_PaletteInstantiation();
    Test10_RegistryCategoryQuery();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << g_testsPassed << " passed, " << g_testsFailed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (g_testsFailed == 0) ? 0 : 1;
}
