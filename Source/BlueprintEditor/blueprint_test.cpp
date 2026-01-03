/*
 * Blueprint Test - Demonstrates entity blueprint creation and serialization
 * 
 * This test program shows how to:
 * 1. Create entity blueprints programmatically
 * 2. Save blueprints to JSON files
 * 3. Load blueprints from JSON files
 * 4. Access and modify component properties
 */

#include "EntityBlueprint.h"
#include <iostream>

using namespace Olympe::Blueprint;

void TestCreateAndSaveBlueprint()
{
    std::cout << "=== Test: Creating Entity Blueprint ===" << std::endl;
    
    // Create a new entity blueprint
    EntityBlueprint entity("TestEntity");
    entity.description = "A test entity created programmatically";
    
    // Add components using helper functions
    entity.AddComponent("Position", CreatePositionComponent(100.0f, 200.0f).properties);
    entity.AddComponent("BoundingBox", CreateBoundingBoxComponent(0.0f, 0.0f, 32.0f, 32.0f).properties);
    entity.AddComponent("VisualSprite", 
        CreateVisualSpriteComponent("Resources/test.png", 0, 0, 32, 32, 16, 16).properties);
    entity.AddComponent("Health", CreateHealthComponent(100, 100).properties);
    entity.AddComponent("PhysicsBody", CreatePhysicsBodyComponent(1.0f, 50.0f).properties);
    
    // Save to file
    std::string filepath = "Blueprints/test_entity_generated.json";
    if (entity.SaveToFile(filepath))
    {
        std::cout << "✓ Blueprint saved to: " << filepath << std::endl;
    }
    else
    {
        std::cout << "✗ Failed to save blueprint" << std::endl;
    }
    
    // Display JSON
    std::cout << "\nGenerated JSON:\n" << entity.ToJson().dump(2) << std::endl;
}

void TestLoadAndModifyBlueprint()
{
    std::cout << "\n=== Test: Loading and Modifying Blueprint ===" << std::endl;
    
    // Load an existing blueprint
    std::string filepath = "Blueprints/example_entity_simple.json";
    EntityBlueprint entity = EntityBlueprint::LoadFromFile(filepath);
    
    if (entity.name.empty())
    {
        std::cout << "✗ Failed to load blueprint from: " << filepath << std::endl;
        return;
    }
    
    std::cout << "✓ Loaded blueprint: " << entity.name << std::endl;
    std::cout << "  Description: " << entity.description << std::endl;
    std::cout << "  Components: " << entity.components.size() << std::endl;
    
    // Display component list
    std::cout << "\nComponents:" << std::endl;
    for (const auto& comp : entity.components)
    {
        std::cout << "  - " << comp.type << std::endl;
    }
    
    // Modify a component
    if (auto* posComp = entity.GetComponent("Position"))
    {
        std::cout << "\n✓ Found Position component" << std::endl;
        std::cout << "  Original position: " << posComp->properties.dump() << std::endl;
        
        // Modify position
        posComp->properties["position"]["x"] = 500.0f;
        posComp->properties["position"]["y"] = 600.0f;
        
        std::cout << "  Modified position: " << posComp->properties.dump() << std::endl;
    }
    
    // Add a new component
    entity.AddComponent("Movement", CreateMovementComponent(1.0f, 0.0f, 0.0f, 0.0f).properties);
    std::cout << "\n✓ Added Movement component" << std::endl;
    
    // Check if component exists
    std::cout << "\nComponent checks:" << std::endl;
    std::cout << "  Has Position: " << (entity.HasComponent("Position") ? "Yes" : "No") << std::endl;
    std::cout << "  Has Movement: " << (entity.HasComponent("Movement") ? "Yes" : "No") << std::endl;
    std::cout << "  Has Health: " << (entity.HasComponent("Health") ? "Yes" : "No") << std::endl;
    
    // Save modified blueprint
    std::string outpath = "Blueprints/test_entity_modified.json";
    if (entity.SaveToFile(outpath))
    {
        std::cout << "\n✓ Modified blueprint saved to: " << outpath << std::endl;
    }
}

void TestCompleteBlueprint()
{
    std::cout << "\n=== Test: Complete Blueprint with All Components ===" << std::endl;
    
    std::string filepath = "Blueprints/example_entity_complete.json";
    EntityBlueprint entity = EntityBlueprint::LoadFromFile(filepath);
    
    if (entity.name.empty())
    {
        std::cout << "✗ Failed to load blueprint from: " << filepath << std::endl;
        return;
    }
    
    std::cout << "✓ Loaded complete blueprint: " << entity.name << std::endl;
    std::cout << "  Component count: " << entity.components.size() << std::endl;
    
    // List all components and their properties
    std::cout << "\nDetailed component properties:" << std::endl;
    for (const auto& comp : entity.components)
    {
        std::cout << "\n  Component: " << comp.type << std::endl;
        std::cout << "    Properties: " << comp.properties.dump(4) << std::endl;
    }
}

int main(int argc, char** argv)
{
    std::cout << "Olympe Blueprint Editor - Entity Blueprint Test\n" << std::endl;
    
    try
    {
        TestCreateAndSaveBlueprint();
        TestLoadAndModifyBlueprint();
        TestCompleteBlueprint();
        
        std::cout << "\n=== All tests completed successfully ===" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
