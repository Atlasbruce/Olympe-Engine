#include "../../../Source/BlueprintEditor/EntityPrefabEditor/EntityPrefabGraphDocument.h"
#include "../../../Source/BlueprintEditor/EntityPrefabEditor/PrefabLoader.h"
#include <iostream>
#include <cassert>

using namespace Olympe;

void TestPropertyLoading()
{
    // Test 1: Verify that Simple_NPC.ats loads properties correctly
    std::cout << "=== Test 1: Simple_NPC.ats Property Loading ===" << std::endl;

    EntityPrefabGraphDocument doc;
    bool loaded = doc.LoadFromFile("OlympeBlueprintEditor/Simple_NPC.ats");

    if (!loaded)
    {
        std::cout << "ERROR: Failed to load Simple_NPC.ats" << std::endl;
        return;
    }

    std::cout << "SUCCESS: File loaded" << std::endl;

    // Check that all nodes have properties
    const auto& nodes = doc.GetAllNodes();
    std::cout << "Total nodes: " << nodes.size() << std::endl;

    int nodeCounter = 0;
    for (const auto& node : nodes)
    {
        nodeCounter++;
        std::cout << "\nNode " << nodeCounter << " (" << node.componentType << "):" << std::endl;
        std::cout << "  Properties count: " << node.properties.size() << std::endl;

        if (node.properties.size() == 0)
        {
            std::cout << "  WARNING: Node has no properties!" << std::endl;
        }
        else
        {
            for (const auto& prop : node.properties)
            {
                std::cout << "    - " << prop.first << " = " << prop.second << std::endl;
            }
        }
    }

    // Expected: All nodes should have properties populated from schema
    // Before fix: All nodes had 0 properties
    // After fix: Each node should have properties from ComponentsParameters.json

    std::cout << "\n=== Test Complete ===" << std::endl;
}

int main()
{
    TestPropertyLoading();
    return 0;
}
