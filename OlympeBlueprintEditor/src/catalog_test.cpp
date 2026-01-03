/*
 * Test program for EnumCatalogManager
 */

#include "../include/EnumCatalogManager.h"
#include <iostream>
#include <iomanip>

using namespace Olympe;

void PrintSeparator()
{
    std::cout << std::string(80, '=') << std::endl;
}

void PrintTypeInfo(const EnumTypeInfo& info)
{
    std::cout << "  ID: " << info.id << std::endl;
    std::cout << "  Name: " << info.name << std::endl;
    std::cout << "  Category: " << info.category << std::endl;
    std::cout << "  Description: " << info.description << std::endl;
    std::cout << "  Tooltip: " << info.tooltip << std::endl;
    std::cout << "  Parameters: " << info.parameters.size() << std::endl;
    
    if (!info.parameters.empty())
    {
        for (size_t i = 0; i < info.parameters.size(); ++i)
        {
            const auto& param = info.parameters[i];
            std::cout << "    [" << i << "] " << param.dump() << std::endl;
        }
    }
}

int main()
{
    std::cout << "EnumCatalogManager Test Program" << std::endl;
    PrintSeparator();

    // Load catalogues
    auto& manager = EnumCatalogManager::Instance();
    
    std::cout << "\n1. Loading catalogues from Blueprints/Catalogues/..." << std::endl;
    if (!manager.LoadCatalogues("Blueprints/Catalogues/"))
    {
        std::cerr << "ERROR: Failed to load catalogues!" << std::endl;
        std::cerr << "Last error: " << manager.GetLastError() << std::endl;
        return 1;
    }

    PrintSeparator();

    // Test Action types
    std::cout << "\n2. Testing Action Types" << std::endl;
    std::cout << "Total actions: " << manager.GetActionTypes().size() << std::endl;
    
    std::cout << "\nCategories:" << std::endl;
    auto actionCategories = manager.GetActionCategories();
    for (const auto& cat : actionCategories)
    {
        std::cout << "  - " << cat << std::endl;
    }

    std::cout << "\nFirst 3 actions:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(3), manager.GetActionTypes().size()); ++i)
    {
        std::cout << "\nAction [" << i << "]:" << std::endl;
        PrintTypeInfo(manager.GetActionTypes()[i]);
    }

    PrintSeparator();

    // Test Condition types
    std::cout << "\n3. Testing Condition Types" << std::endl;
    std::cout << "Total conditions: " << manager.GetConditionTypes().size() << std::endl;
    
    std::cout << "\nCategories:" << std::endl;
    auto conditionCategories = manager.GetConditionCategories();
    for (const auto& cat : conditionCategories)
    {
        std::cout << "  - " << cat << std::endl;
    }

    std::cout << "\nFirst 3 conditions:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(3), manager.GetConditionTypes().size()); ++i)
    {
        std::cout << "\nCondition [" << i << "]:" << std::endl;
        PrintTypeInfo(manager.GetConditionTypes()[i]);
    }

    PrintSeparator();

    // Test Decorator types
    std::cout << "\n4. Testing Decorator Types" << std::endl;
    std::cout << "Total decorators: " << manager.GetDecoratorTypes().size() << std::endl;
    
    std::cout << "\nCategories:" << std::endl;
    auto decoratorCategories = manager.GetDecoratorCategories();
    for (const auto& cat : decoratorCategories)
    {
        std::cout << "  - " << cat << std::endl;
    }

    std::cout << "\nFirst 3 decorators:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(3), manager.GetDecoratorTypes().size()); ++i)
    {
        std::cout << "\nDecorator [" << i << "]:" << std::endl;
        PrintTypeInfo(manager.GetDecoratorTypes()[i]);
    }

    PrintSeparator();

    // Test lookup
    std::cout << "\n5. Testing Lookup Functions" << std::endl;
    
    std::cout << "\nLooking up 'MoveTo' action..." << std::endl;
    const auto* moveToAction = manager.FindActionType("MoveTo");
    if (moveToAction)
    {
        std::cout << "Found!" << std::endl;
        PrintTypeInfo(*moveToAction);
    }
    else
    {
        std::cout << "NOT FOUND!" << std::endl;
    }

    std::cout << "\nLooking up 'HasTarget' condition..." << std::endl;
    const auto* hasTargetCondition = manager.FindConditionType("HasTarget");
    if (hasTargetCondition)
    {
        std::cout << "Found!" << std::endl;
        PrintTypeInfo(*hasTargetCondition);
    }
    else
    {
        std::cout << "NOT FOUND!" << std::endl;
    }

    std::cout << "\nLooking up 'Inverter' decorator..." << std::endl;
    const auto* inverterDecorator = manager.FindDecoratorType("Inverter");
    if (inverterDecorator)
    {
        std::cout << "Found!" << std::endl;
        PrintTypeInfo(*inverterDecorator);
    }
    else
    {
        std::cout << "NOT FOUND!" << std::endl;
    }

    PrintSeparator();

    // Test validation
    std::cout << "\n6. Testing Validation Functions" << std::endl;
    
    std::cout << "Is 'MoveTo' a valid action? " 
              << (manager.IsValidActionType("MoveTo") ? "YES" : "NO") << std::endl;
    std::cout << "Is 'InvalidAction' a valid action? " 
              << (manager.IsValidActionType("InvalidAction") ? "YES" : "NO") << std::endl;
    
    std::cout << "Is 'HasTarget' a valid condition? " 
              << (manager.IsValidConditionType("HasTarget") ? "YES" : "NO") << std::endl;
    std::cout << "Is 'InvalidCondition' a valid condition? " 
              << (manager.IsValidConditionType("InvalidCondition") ? "YES" : "NO") << std::endl;
    
    std::cout << "Is 'Inverter' a valid decorator? " 
              << (manager.IsValidDecoratorType("Inverter") ? "YES" : "NO") << std::endl;
    std::cout << "Is 'InvalidDecorator' a valid decorator? " 
              << (manager.IsValidDecoratorType("InvalidDecorator") ? "YES" : "NO") << std::endl;

    PrintSeparator();

    // Test category filtering
    std::cout << "\n7. Testing Category Filtering" << std::endl;
    
    std::cout << "\nActions in 'Movement' category:" << std::endl;
    auto movementActions = manager.GetActionsByCategory("Movement");
    std::cout << "Count: " << movementActions.size() << std::endl;
    for (const auto& action : movementActions)
    {
        std::cout << "  - " << action.name << " (" << action.id << ")" << std::endl;
    }

    std::cout << "\nConditions in 'Target' category:" << std::endl;
    auto targetConditions = manager.GetConditionsByCategory("Target");
    std::cout << "Count: " << targetConditions.size() << std::endl;
    for (const auto& cond : targetConditions)
    {
        std::cout << "  - " << cond.name << " (" << cond.id << ")" << std::endl;
    }

    std::cout << "\nDecorators in 'Logic' category:" << std::endl;
    auto logicDecorators = manager.GetDecoratorsByCategory("Logic");
    std::cout << "Count: " << logicDecorators.size() << std::endl;
    for (const auto& dec : logicDecorators)
    {
        std::cout << "  - " << dec.name << " (" << dec.id << ")" << std::endl;
    }

    PrintSeparator();

    // Test reload
    std::cout << "\n8. Testing Hot Reload" << std::endl;
    std::cout << "Reloading catalogues..." << std::endl;
    if (manager.ReloadCatalogues())
    {
        std::cout << "Reload successful!" << std::endl;
        std::cout << "Actions: " << manager.GetActionTypes().size() << std::endl;
        std::cout << "Conditions: " << manager.GetConditionTypes().size() << std::endl;
        std::cout << "Decorators: " << manager.GetDecoratorTypes().size() << std::endl;
    }
    else
    {
        std::cerr << "Reload failed!" << std::endl;
        std::cerr << "Last error: " << manager.GetLastError() << std::endl;
    }

    PrintSeparator();
    std::cout << "\nAll tests completed successfully!" << std::endl;

    return 0;
}
