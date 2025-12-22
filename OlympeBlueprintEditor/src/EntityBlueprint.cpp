/*
 * Olympe Blueprint Editor - Entity Blueprint Implementation
 */

#include "../include/EntityBlueprint.h"
#include "../../Source/json_helper.h"
#include <algorithm>

namespace Olympe {
namespace Blueprint {

// EntityBlueprint methods

void EntityBlueprint::AddComponent(const std::string& type, const json& properties)
{
    // Check if component already exists, if so, update it
    for (auto& comp : components)
    {
        if (comp.type == type)
        {
            comp.properties = properties;
            return;
        }
    }
    
    // Add new component
    components.push_back(ComponentData(type, properties));
}

ComponentData* EntityBlueprint::GetComponent(const std::string& type)
{
    for (auto& comp : components)
    {
        if (comp.type == type)
            return &comp;
    }
    return nullptr;
}

const ComponentData* EntityBlueprint::GetComponent(const std::string& type) const
{
    for (const auto& comp : components)
    {
        if (comp.type == type)
            return &comp;
    }
    return nullptr;
}

bool EntityBlueprint::HasComponent(const std::string& type) const
{
    return GetComponent(type) != nullptr;
}

bool EntityBlueprint::RemoveComponent(const std::string& type)
{
    auto it = std::remove_if(components.begin(), components.end(),
        [&type](const ComponentData& comp) { return comp.type == type; });
    
    if (it != components.end())
    {
        components.erase(it, components.end());
        return true;
    }
    return false;
}

json EntityBlueprint::ToJson() const
{
    json j;
    j["schema_version"] = 1;
    j["type"] = "EntityBlueprint";
    j["name"] = name;
    j["description"] = description;
    
    j["components"] = json::array();
    for (const auto& comp : components)
    {
        json compJson;
        compJson["type"] = comp.type;
        compJson["properties"] = comp.properties;
        j["components"].push_back(compJson);
    }
    
    return j;
}

EntityBlueprint EntityBlueprint::FromJson(const json& j)
{
    EntityBlueprint blueprint;
    
    blueprint.name = JsonHelper::GetString(j, "name", "");
    blueprint.description = JsonHelper::GetString(j, "description", "");
    
    if (JsonHelper::IsArray(j, "components"))
    {
        JsonHelper::ForEachInArray(j, "components", [&blueprint](const json& compJson, size_t i)
        {
            ComponentData comp;
            comp.type = JsonHelper::GetString(compJson, "type", "");
            
            if (compJson.contains("properties"))
                comp.properties = compJson["properties"];
            
            blueprint.components.push_back(comp);
        });
    }
    
    return blueprint;
}

bool EntityBlueprint::SaveToFile(const std::string& filepath) const
{
    return JsonHelper::SaveJsonToFile(filepath, ToJson(), 4);
}

EntityBlueprint EntityBlueprint::LoadFromFile(const std::string& filepath)
{
    json j;
    if (!JsonHelper::LoadJsonFromFile(filepath, j))
    {
        return EntityBlueprint();
    }
    
    return FromJson(j);
}

// Helper functions for creating component data

ComponentData CreatePositionComponent(float x, float y)
{
    json props = json::object();
    json position = json::object();
    position["x"] = x;
    position["y"] = y;
    position["z"] = 0.0f;
    props["position"] = position;
    return ComponentData("Position", props);
}

ComponentData CreateBoundingBoxComponent(float x, float y, float width, float height)
{
    json props = json::object();
    json boundingBox = json::object();
    boundingBox["x"] = x;
    boundingBox["y"] = y;
    boundingBox["w"] = width;
    boundingBox["h"] = height;
    props["boundingBox"] = boundingBox;
    return ComponentData("BoundingBox", props);
}

ComponentData CreateVisualSpriteComponent(
    const std::string& spritePath,
    float srcX, float srcY, float srcWidth, float srcHeight,
    float hotSpotX, float hotSpotY)
{
    json props = json::object();
    props["spritePath"] = spritePath;
    
    json srcRect = json::object();
    srcRect["x"] = srcX;
    srcRect["y"] = srcY;
    srcRect["w"] = srcWidth;
    srcRect["h"] = srcHeight;
    props["srcRect"] = srcRect;
    
    json hotSpot = json::object();
    hotSpot["x"] = hotSpotX;
    hotSpot["y"] = hotSpotY;
    props["hotSpot"] = hotSpot;
    
    return ComponentData("VisualSprite", props);
}

ComponentData CreateMovementComponent(float dirX, float dirY, float velX, float velY)
{
    json props = json::object();
    
    json direction = json::object();
    direction["x"] = dirX;
    direction["y"] = dirY;
    props["direction"] = direction;
    
    json velocity = json::object();
    velocity["x"] = velX;
    velocity["y"] = velY;
    props["velocity"] = velocity;
    
    return ComponentData("Movement", props);
}

ComponentData CreatePhysicsBodyComponent(float mass, float speed)
{
    json props = json::object();
    props["mass"] = mass;
    props["speed"] = speed;
    return ComponentData("PhysicsBody", props);
}

ComponentData CreateHealthComponent(int current, int max)
{
    json props = json::object();
    props["currentHealth"] = current;
    props["maxHealth"] = max;
    return ComponentData("Health", props);
}

ComponentData CreateAIBehaviorComponent(const std::string& behaviorType)
{
    json props = json::object();
    props["behaviorType"] = behaviorType;
    return ComponentData("AIBehavior", props);
}

} // namespace Blueprint
} // namespace Olympe
