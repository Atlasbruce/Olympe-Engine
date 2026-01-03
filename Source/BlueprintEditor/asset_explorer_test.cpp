/*
 * Asset Explorer Backend Test
 * 
 * Simple test to verify the backend asset management API
 * This is a manual test file - compile and run separately from main project
 */

#include "BlueprintEditor/blueprinteditor.h"
#include <iostream>
#include <cassert>

using namespace Olympe;

void TestAssetScanning()
{
    std::cout << "=== Test: Asset Scanning ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    // Set asset root path
    editor.SetAssetRootPath("Blueprints");
    
    // Get asset tree
    auto tree = editor.GetAssetTree();
    
    if (tree)
    {
        std::cout << "✓ Asset tree created successfully" << std::endl;
        std::cout << "  Root: " << tree->name << std::endl;
        std::cout << "  Children: " << tree->children.size() << std::endl;
    }
    else
    {
        std::cout << "✗ Asset tree is null" << std::endl;
        if (editor.HasError())
        {
            std::cout << "  Error: " << editor.GetLastError() << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void TestAssetQueries()
{
    std::cout << "=== Test: Asset Queries ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    // Get all assets
    auto allAssets = editor.GetAllAssets();
    std::cout << "Total assets: " << allAssets.size() << std::endl;
    
    // Get assets by type
    auto entityBlueprints = editor.GetAssetsByType("EntityBlueprint");
    std::cout << "EntityBlueprints: " << entityBlueprints.size() << std::endl;
    
    auto behaviorTrees = editor.GetAssetsByType("BehaviorTree");
    std::cout << "BehaviorTrees: " << behaviorTrees.size() << std::endl;
    
    std::cout << std::endl;
}

void TestAssetSearch()
{
    std::cout << "=== Test: Asset Search ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    // Search for "guard"
    auto results = editor.SearchAssets("guard");
    std::cout << "Search 'guard': " << results.size() << " results" << std::endl;
    
    for (const auto& asset : results)
    {
        std::cout << "  - " << asset.name << " [" << asset.type << "]" << std::endl;
    }
    
    std::cout << std::endl;
}

void TestAssetMetadata()
{
    std::cout << "=== Test: Asset Metadata ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    // Test entity blueprint
    auto metadata = editor.GetAssetMetadata("Blueprints/example_entity_simple.json");
    
    if (metadata.isValid)
    {
        std::cout << "✓ Entity Blueprint loaded successfully" << std::endl;
        std::cout << "  Name: " << metadata.name << std::endl;
        std::cout << "  Type: " << metadata.type << std::endl;
        std::cout << "  Description: " << metadata.description << std::endl;
        std::cout << "  Components: " << metadata.componentCount << std::endl;
        
        for (const auto& comp : metadata.components)
        {
            std::cout << "    - " << comp << std::endl;
        }
    }
    else
    {
        std::cout << "✗ Failed to load entity blueprint" << std::endl;
        std::cout << "  Error: " << metadata.errorMessage << std::endl;
    }
    
    std::cout << std::endl;
}

void TestErrorHandling()
{
    std::cout << "=== Test: Error Handling ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    // Test with non-existent file
    auto metadata = editor.GetAssetMetadata("Blueprints/nonexistent.json");
    
    if (!metadata.isValid)
    {
        std::cout << "✓ Error handling works correctly" << std::endl;
        std::cout << "  Expected error: " << metadata.errorMessage << std::endl;
    }
    else
    {
        std::cout << "✗ Should have failed for non-existent file" << std::endl;
    }
    
    std::cout << std::endl;
}

void TestMultiFolderNavigation()
{
    std::cout << "=== Test: Multi-Folder Navigation ===" << std::endl;
    
    auto& editor = BlueprintEditor::Get();
    
    auto tree = editor.GetAssetTree();
    
    if (!tree)
    {
        std::cout << "✗ No asset tree available" << std::endl;
        return;
    }
    
    std::cout << "Root directory: " << tree->name << std::endl;
    
    // Count directories and files
    int dirCount = 0;
    int fileCount = 0;
    
    std::function<void(const std::shared_ptr<AssetNode>&, int)> traverse = 
        [&](const std::shared_ptr<AssetNode>& node, int depth) 
    {
        std::string indent(depth * 2, ' ');
        
        if (node->isDirectory)
        {
            dirCount++;
            std::cout << indent << "[DIR] " << node->name << std::endl;
            
            for (const auto& child : node->children)
            {
                traverse(child, depth + 1);
            }
        }
        else
        {
            fileCount++;
            std::cout << indent << "[FILE] " << node->name << " [" << node->type << "]" << std::endl;
        }
    };
    
    for (const auto& child : tree->children)
    {
        traverse(child, 0);
    }
    
    std::cout << "\nSummary:" << std::endl;
    std::cout << "  Directories: " << dirCount << std::endl;
    std::cout << "  Files: " << fileCount << std::endl;
    
    std::cout << std::endl;
}

int main()
{
    std::cout << "=====================================" << std::endl;
    std::cout << "  Asset Explorer Backend Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;
    
    // Initialize the editor
    BlueprintEditor::Get().Initialize();
    
    // Run tests
    TestAssetScanning();
    TestAssetQueries();
    TestAssetSearch();
    TestAssetMetadata();
    TestErrorHandling();
    TestMultiFolderNavigation();
    
    std::cout << "=====================================" << std::endl;
    std::cout << "  Test Suite Complete" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    return 0;
}
