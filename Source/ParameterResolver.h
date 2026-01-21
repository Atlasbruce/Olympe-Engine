/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ParameterResolver purpose: Resolves and merges prefab blueprint parameters with 
level instance parameters to create final component configurations. Handles property 
overrides with priority-based merging and type conversion.
*/

#pragma once

#include <string>
#include <map>
#include <vector>
#include "vector.h"
#include "ComponentDefinition.h"

// Forward declarations
namespace Olympe {
namespace Blueprint {
	struct EntityBlueprint;
}
}

// PrefabBlueprint - Simplified structure for prefab data
// Note: This will be refactored later, but we maintain compatibility
struct PrefabBlueprint
{
	std::string prefabName;
	std::string entityType;
	std::vector<ComponentDefinition> components;
	
	PrefabBlueprint() = default;
	PrefabBlueprint(const std::string& name, const std::string& type)
		: prefabName(name), entityType(type) {}
};

// Object reference for linking level objects together
struct ObjectReference
{
	std::string propertyName;      // Name of the property that references another object
	std::string targetObjectId;    // ID of the target object in the level
	std::string targetObjectName;  // Name of the target object (for debugging)
	
	ObjectReference() = default;
	ObjectReference(const std::string& prop, const std::string& id, const std::string& name)
		: propertyName(prop), targetObjectId(id), targetObjectName(name) {}
};

// Level instance parameters - represents an object instance in a level
struct LevelInstanceParameters
{
	std::string objectName;        // Unique name/ID of this object instance
	std::string objectType;        // Prefab type (e.g., "player", "enemy")
	Vector position;               // Position in the level (x, y, z)
	std::map<std::string, ComponentParameter> properties; // Custom property overrides
	std::vector<ObjectReference> objectReferences;        // References to other objects
	
	LevelInstanceParameters() = default;
	LevelInstanceParameters(const std::string& name, const std::string& type)
		: objectName(name), objectType(type), position(0.0f, 0.0f, 0.0f) {}
};

// Resolved component instance - final component with merged parameters
struct ResolvedComponentInstance
{
	std::string componentType;     // Component type name
	std::map<std::string, ComponentParameter> parameters; // Final resolved parameters
	bool isValid;                  // Whether resolution was successful
	std::vector<std::string> errors; // Errors encountered during resolution
	
	ResolvedComponentInstance() : isValid(true) {}
	explicit ResolvedComponentInstance(const std::string& type)
		: componentType(type), isValid(true) {}
};

// ParameterResolver - Resolves prefab and level parameters into final component configurations
class ParameterResolver
{
public:
	ParameterResolver();
	~ParameterResolver();
	
	// Main resolution method
	// Merges prefab defaults with level instance parameters
	// Priority: Level position > Level custom properties > Prefab defaults
	std::vector<ResolvedComponentInstance> Resolve(
		const PrefabBlueprint& prefab,
		const LevelInstanceParameters& instanceParams
	);
	
private:
	// Resolve a single component with instance parameters
	ResolvedComponentInstance ResolveComponent(
		const ComponentDefinition& componentDef,
		const LevelInstanceParameters& instanceParams
	);
	
	// Apply a property override from level to component
	void ApplyPropertyOverride(
		ResolvedComponentInstance& component,
		const std::string& propertyName,
		const ComponentParameter& propertyValue
	);
	
	// Extract component-specific parameters from level properties
	std::map<std::string, ComponentParameter> ExtractComponentParameters(
		const std::string& componentType,
		const LevelInstanceParameters& instanceParams
	);
	
	// Convert level property to component parameter (with type conversion)
	ComponentParameter ConvertLevelProperty(
		const std::string& propertyName,
		const ComponentParameter& levelProperty,
		ComponentParameter::Type expectedType
	);
	
	// Validate a resolved component
	void ValidateResolvedComponent(ResolvedComponentInstance& component);
};
