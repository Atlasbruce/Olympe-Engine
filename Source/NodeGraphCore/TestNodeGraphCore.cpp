/**
 * @file TestNodeGraphCore.cpp
 * @brief Basic tests for NodeGraphCore module
 * @author Olympe Engine
 * @date 2026-02-18
 * 
 * This file contains basic tests to validate NodeGraphCore functionality.
 * These tests can be compiled separately for validation purposes.
 */

#include "NodeGraphCore/NodeGraphCore.h"
#include "NodeGraphCore/GraphDocument.h"
#include "NodeGraphCore/NodeGraphManager.h"
#include "NodeGraphCore/CommandSystem.h"
#include "NodeGraphCore/Commands/CreateNodeCommand.h"
#include "NodeGraphCore/Commands/DeleteNodeCommand.h"
#include "NodeGraphCore/Commands/ConnectPinsCommand.h"
#include "NodeGraphCore/Commands/MoveNodeCommand.h"
#include "NodeGraphCore/GraphMigrator.h"

#include <iostream>
#include <cassert>
#include <string>

using namespace Olympe::NodeGraph;

// Test 1: Create GraphDocument
void Test1_CreateGraphDocument()
{
    std::cout << "Test 1: Create GraphDocument... ";
    
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    
    assert(doc.GetNodes().empty());
    assert(doc.GetLinks().empty());
    
    std::cout << "PASSED" << std::endl;
}

// Test 2: CRUD Nodes
void Test2_CRUDNodes()
{
    std::cout << "Test 2: CRUD Nodes... ";
    
    GraphDocument doc;
    
    NodeId id1 = doc.CreateNode("BT_Selector", Vector2(100, 100));
    NodeId id2 = doc.CreateNode("BT_Action", Vector2(200, 200));
    
    assert(doc.GetNodes().size() == 2);
    assert(doc.GetNode(id1) != nullptr);
    
    bool deleted = doc.DeleteNode(id1);
    assert(deleted == true);
    assert(doc.GetNodes().size() == 1);
    assert(doc.GetNode(id1) == nullptr);
    
    std::cout << "PASSED" << std::endl;
}

// Test 3: Connect Pins
void Test3_ConnectPins()
{
    std::cout << "Test 3: Connect Pins... ";
    
    GraphDocument doc;
    
    NodeId id1 = doc.CreateNode("BT_Selector", Vector2(0, 0));
    NodeId id2 = doc.CreateNode("BT_Action", Vector2(0, 100));
    
    PinId pin1;
    pin1.value = id1.value;
    PinId pin2;
    pin2.value = id2.value;
    
    LinkId link = doc.ConnectPins(pin1, pin2);
    
    assert(doc.GetLinks().size() == 1);
    assert(doc.GetLinks()[0].fromPin.value == pin1.value);
    
    std::cout << "PASSED" << std::endl;
}

// Test 4: Serialization
void Test4_Serialization()
{
    std::cout << "Test 4: Serialization... ";
    
    GraphDocument doc;
    doc.type = "AIGraph";
    doc.graphKind = "BehaviorTree";
    doc.CreateNode("BT_Selector", Vector2(100, 100));
    
    json j = doc.ToJson();
    
    assert(JsonHelper::GetInt(j, "schemaVersion", 0) == 2);
    assert(JsonHelper::GetString(j, "type", "") == "AIGraph");
    assert(JsonHelper::GetString(j, "graphKind", "") == "BehaviorTree");
    assert(j["data"]["nodes"].is_array());
    assert(j["data"]["nodes"].size() == 1);
    
    std::cout << "PASSED" << std::endl;
}

// Test 5: Multi-graph
void Test5_MultiGraph()
{
    std::cout << "Test 5: Multi-graph... ";
    
    NodeGraphManager& mgr = NodeGraphManager::Get();
    
    GraphId g1 = mgr.CreateGraph("AIGraph", "BehaviorTree");
    GraphId g2 = mgr.CreateGraph("AIGraph", "HFSM");
    
    assert(mgr.GetAllGraphIds().size() >= 2);
    
    mgr.SetActiveGraph(g2);
    assert(mgr.GetActiveGraphId().value == g2.value);
    
    // Clean up
    mgr.CloseGraph(g1);
    mgr.CloseGraph(g2);
    
    std::cout << "PASSED" << std::endl;
}

// Test 6: Undo/Redo
void Test6_UndoRedo()
{
    std::cout << "Test 6: Undo/Redo... ";
    
    GraphDocument doc;
    CommandStack stack;
    
    auto cmd = std::unique_ptr<ICommand>(new CreateNodeCommand(&doc, "BT_Selector", Vector2(0, 0)));
    stack.ExecuteCommand(std::move(cmd));
    
    assert(doc.GetNodes().size() == 1);
    
    stack.Undo();
    assert(doc.GetNodes().empty());
    
    stack.Redo();
    assert(doc.GetNodes().size() == 1);
    
    std::cout << "PASSED" << std::endl;
}

// Test 7: HasCycles
void Test7_HasCycles()
{
    std::cout << "Test 7: HasCycles... ";
    
    GraphDocument doc;
    
    // Create simple tree without cycles
    NodeId idA = doc.CreateNode("A", Vector2(0, 0));
    NodeId idB = doc.CreateNode("B", Vector2(100, 0));
    
    NodeData* nodeA = doc.GetNode(idA);
    if (nodeA != nullptr)
    {
        nodeA->children.push_back(idB);
    }
    
    assert(doc.HasCycles() == false);
    
    // Create cycle: B -> A
    NodeData* nodeB = doc.GetNode(idB);
    if (nodeB != nullptr)
    {
        nodeB->children.push_back(idA);
    }
    
    assert(doc.HasCycles() == true);
    
    std::cout << "PASSED" << std::endl;
}

// Test 8: ValidateGraph
void Test8_ValidateGraph()
{
    std::cout << "Test 8: ValidateGraph... ";
    
    GraphDocument doc;
    std::string error;
    
    // Empty graph should fail
    bool valid = doc.ValidateGraph(error);
    assert(valid == false);
    
    // Add a node
    doc.CreateNode("BT_Selector", Vector2(0, 0));
    
    // Graph with selector without children should fail
    valid = doc.ValidateGraph(error);
    assert(valid == false);
    assert(error.find("0 children") != std::string::npos);
    
    std::cout << "PASSED" << std::endl;
}

int main()
{
    std::cout << "==================================" << std::endl;
    std::cout << "NodeGraphCore Basic Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try
    {
        Test1_CreateGraphDocument();
        Test2_CRUDNodes();
        Test3_ConnectPins();
        Test4_Serialization();
        Test5_MultiGraph();
        Test6_UndoRedo();
        Test7_HasCycles();
        Test8_ValidateGraph();
        
        std::cout << "==================================" << std::endl;
        std::cout << "All tests PASSED!" << std::endl;
        std::cout << "==================================" << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
