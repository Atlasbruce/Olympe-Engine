/*
 * Olympe Blueprint Editor - Entity Blueprint
 * 
 * Defines the JSON structure and C++ representation for entity blueprints.
 * An entity blueprint contains component data that can be serialized to/from JSON.
 */

#pragma once
#include <string>
#include <vector>
#include "../../Source/third_party/nlohmann/json.hpp"

namespace Olympe {
namespace Blueprint {

// Forward declarations
using json = nlohmann::json;

// Component data holder - stores arbitrary component properties as JSON
struct ComponentData
{
    std::string type;           // Component type name (e.g., "Position", "BoundingBox", "VisualSprite")
    json properties;            // Component properties as JSON object
    
    ComponentData() = default;
    ComponentData(const std::string& t) : type(t) {}
    ComponentData(const std::string& t, const json& props) : type(t), properties(props) {}
};

// Entity Blueprint - complete definition of an entity in JSON format
struct EntityBlueprint
{
    std::string name;                       // Entity name/identifier
    std::string description;                // Optional description
    std::vector<ComponentData> components;  // List of components with their properties
    
    EntityBlueprint() = default;
    EntityBlueprint(const std::string& n) : name(n) {}
    
    // Add a component with its properties
    void AddComponent(const std::string& type, const json& properties);
    
    // Get component data by type (returns nullptr if not found)
    ComponentData* GetComponent(const std::string& type);
    const ComponentData* GetComponent(const std::string& type) const;
    
    // Check if entity has a specific component
    bool HasComponent(const std::string& type) const;
    
    // Remove a component by type
    bool RemoveComponent(const std::string& type);
    
    // Serialization
    json ToJson() const;
    static EntityBlueprint FromJson(const json& j);
    
    // File I/O
    bool SaveToFile(const std::string& filepath) const;
    static EntityBlueprint LoadFromFile(const std::string& filepath);
};

// Helper functions for creating common component data

// Position component (x, y)
ComponentData CreatePositionComponent(float x, float y);

// BoundingBox component (x, y, width, height)
ComponentData CreateBoundingBoxComponent(float x, float y, float width, float height);

// VisualSprite component (sprite path, source rect)
ComponentData CreateVisualSpriteComponent(
    const std::string& spritePath,
    float srcX, float srcY, float srcWidth, float srcHeight,
    float hotSpotX = 0.0f, float hotSpotY = 0.0f
);

// Movement component (direction, velocity)
ComponentData CreateMovementComponent(float dirX, float dirY, float velX, float velY);

// PhysicsBody component (mass, speed)
ComponentData CreatePhysicsBodyComponent(float mass, float speed);

// Health component (current, max)
ComponentData CreateHealthComponent(int current, int max);

// AIBehavior component (behavior type)
ComponentData CreateAIBehaviorComponent(const std::string& behaviorType);

} // namespace Blueprint
} // namespace Olympe
