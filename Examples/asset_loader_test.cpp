/*
 * Olympe Engine - Asset Loader Test
 * 
 * This test validates the unified asset loading API
 */

#include "../Source/AssetLoader.h"
#include <iostream>
#include <cassert>

using namespace Olympe::Assets;

void TestEntityBlueprint()
{
    std::cout << "[Test EntityBlueprint]" << std::endl;
    
    // Create a test entity blueprint
    json blueprint;
    blueprint["schema_version"] = 1;
    blueprint["type"] = "EntityBlueprint";
    blueprint["name"] = "TestEntity";
    blueprint["description"] = "A test entity";
    blueprint["components"] = json::array();
    
    json component;
    component["type"] = "Position";
    component["properties"]["x"] = 100.0f;
    component["properties"]["y"] = 200.0f;
    blueprint["components"].push_back(component);
    
    // Save it
    std::string filepath = "test_entity.json";
    bool saved = EntityBlueprintLoader::SaveEntityBlueprint(filepath, blueprint);
    assert(saved && "Failed to save entity blueprint");
    std::cout << "  ✓ Entity blueprint saved" << std::endl;
    
    // Load it back
    json loaded;
    bool loadSuccess = EntityBlueprintLoader::LoadEntityBlueprint(filepath, loaded);
    assert(loadSuccess && "Failed to load entity blueprint");
    std::cout << "  ✓ Entity blueprint loaded" << std::endl;
    
    // Verify data
    assert(loaded["name"].get<std::string>() == "TestEntity");
    assert(loaded["components"].is_array());
    assert(loaded["components"].size() == 1);
    std::cout << "  ✓ Entity blueprint data verified" << std::endl;
    
    std::cout << std::endl;
}

void TestAssetInfo()
{
    std::cout << "[Test AssetInfo]" << std::endl;
    
    // Create asset info
    AssetInfo info(AssetType::BehaviorTree, "TestBT");
    info.description = "A test behavior tree";
    info.schema_version = 1;
    
    // Convert to JSON
    json j = info.ToJson();
    assert(j["type"].get<std::string>() == "BehaviorTree");
    assert(j["name"].get<std::string>() == "TestBT");
    std::cout << "  ✓ AssetInfo serialized" << std::endl;
    
    // Convert back
    AssetInfo loaded = AssetInfo::FromJson(j);
    assert(loaded.type == AssetType::BehaviorTree);
    assert(loaded.name == "TestBT");
    assert(loaded.description == "A test behavior tree");
    std::cout << "  ✓ AssetInfo deserialized" << std::endl;
    
    std::cout << std::endl;
}

void TestAssetTypeConversion()
{
    std::cout << "[Test AssetType Conversion]" << std::endl;
    
    // Test all asset types
    assert(AssetTypeToString(AssetType::EntityBlueprint) == "EntityBlueprint");
    assert(AssetTypeToString(AssetType::Prefab) == "Prefab");
    assert(AssetTypeToString(AssetType::BehaviorTree) == "BehaviorTree");
    assert(AssetTypeToString(AssetType::HFSM) == "HFSM");
    assert(AssetTypeToString(AssetType::Template) == "Template");
    std::cout << "  ✓ AssetType to string conversion" << std::endl;
    
    assert(StringToAssetType("EntityBlueprint") == AssetType::EntityBlueprint);
    assert(StringToAssetType("Prefab") == AssetType::Prefab);
    assert(StringToAssetType("BehaviorTree") == AssetType::BehaviorTree);
    assert(StringToAssetType("HFSM") == AssetType::HFSM);
    assert(StringToAssetType("Template") == AssetType::Template);
    assert(StringToAssetType("InvalidType") == AssetType::Unknown);
    std::cout << "  ✓ String to AssetType conversion" << std::endl;
    
    std::cout << std::endl;
}

void TestBehaviorTree()
{
    std::cout << "[Test BehaviorTree]" << std::endl;
    
    // Create a test behavior tree
    json bt;
    bt["schema_version"] = 1;
    bt["type"] = "BehaviorTree";
    bt["name"] = "TestBT";
    bt["description"] = "A test behavior tree";
    bt["nodes"] = json::array();
    
    json node;
    node["id"] = 1;
    node["type"] = "Selector";
    node["children"] = json::array();
    node["children"].push_back(2);
    node["children"].push_back(3);
    bt["nodes"].push_back(node);
    
    // Save it
    std::string filepath = "test_bt.json";
    bool saved = BehaviorTreeLoader::SaveBehaviorTree(filepath, bt);
    assert(saved && "Failed to save behavior tree");
    std::cout << "  ✓ Behavior tree saved" << std::endl;
    
    // Load it back
    json loaded;
    bool loadSuccess = BehaviorTreeLoader::LoadBehaviorTree(filepath, loaded);
    assert(loadSuccess && "Failed to load behavior tree");
    std::cout << "  ✓ Behavior tree loaded" << std::endl;
    
    // Verify data
    assert(loaded["name"].get<std::string>() == "TestBT");
    assert(loaded["nodes"].is_array());
    std::cout << "  ✓ Behavior tree data verified" << std::endl;
    
    std::cout << std::endl;
}

void TestHFSM()
{
    std::cout << "[Test HFSM]" << std::endl;
    
    // Create a test HFSM
    json hfsm;
    hfsm["schema_version"] = 1;
    hfsm["type"] = "HFSM";
    hfsm["name"] = "TestHFSM";
    hfsm["description"] = "A test HFSM";
    hfsm["states"] = json::array();
    
    json state;
    state["name"] = "Idle";
    state["transitions"] = json::array();
    hfsm["states"].push_back(state);
    
    // Save it
    std::string filepath = "test_hfsm.json";
    bool saved = HFSMLoader::SaveHFSM(filepath, hfsm);
    assert(saved && "Failed to save HFSM");
    std::cout << "  ✓ HFSM saved" << std::endl;
    
    // Load it back
    json loaded;
    bool loadSuccess = HFSMLoader::LoadHFSM(filepath, loaded);
    assert(loadSuccess && "Failed to load HFSM");
    std::cout << "  ✓ HFSM loaded" << std::endl;
    
    // Verify data
    assert(loaded["name"].get<std::string>() == "TestHFSM");
    assert(loaded["states"].is_array());
    std::cout << "  ✓ HFSM data verified" << std::endl;
    
    std::cout << std::endl;
}

void TestAssetValidation()
{
    std::cout << "[Test Asset Validation]" << std::endl;
    
    // Valid asset
    json validAsset;
    validAsset["type"] = "EntityBlueprint";
    validAsset["name"] = "ValidAsset";
    assert(AssetLoader::ValidateAsset(validAsset));
    std::cout << "  ✓ Valid asset passes validation" << std::endl;
    
    // Invalid asset - missing type
    json invalidAsset1;
    invalidAsset1["name"] = "InvalidAsset";
    assert(!AssetLoader::ValidateAsset(invalidAsset1));
    std::cout << "  ✓ Asset without 'type' fails validation" << std::endl;
    
    // Invalid asset - missing name
    json invalidAsset2;
    invalidAsset2["type"] = "EntityBlueprint";
    assert(!AssetLoader::ValidateAsset(invalidAsset2));
    std::cout << "  ✓ Asset without 'name' fails validation" << std::endl;
    
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "=== Olympe Engine - Asset Loader Test ===" << std::endl;
    std::cout << std::endl;
    
    try
    {
        TestAssetTypeConversion();
        TestAssetInfo();
        TestEntityBlueprint();
        TestBehaviorTree();
        TestHFSM();
        TestAssetValidation();
        
        std::cout << "=== All Tests Passed! ===" << std::endl;
        std::cout << "Asset Loader API is working correctly." << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
