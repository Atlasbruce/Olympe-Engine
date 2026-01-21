/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ParameterSchema purpose: Provides a schema registry for mapping blueprint parameters
to ECS component fields with type validation and default values.
*/

#pragma once

#include <string>
#include <map>
#include <set>
#include <memory>
#include "ComponentDefinition.h"

// Parameter schema entry defining how a blueprint parameter maps to a component field
struct ParameterSchemaEntry
{
	std::string parameterName;           // Name of the parameter in the blueprint
	std::string targetComponent;         // Target component type (e.g., "Position_data")
	std::string targetField;             // Target field name in the component
	ComponentParameter::Type expectedType; // Expected parameter type
	bool isRequired = false;             // Is this parameter required?
	ComponentParameter defaultValue;     // Default value if not provided
	
	ParameterSchemaEntry() = default;
	ParameterSchemaEntry(const std::string& name, 
	                     const std::string& component, 
	                     const std::string& field,
	                     ComponentParameter::Type type,
	                     bool required = false,
	                     const ComponentParameter& defValue = ComponentParameter())
		: parameterName(name)
		, targetComponent(component)
		, targetField(field)
		, expectedType(type)
		, isRequired(required)
		, defaultValue(defValue)
	{}
};

// Component schema containing all parameters for a component type
struct ComponentSchema
{
	std::string componentType;                           // Component type name
	std::map<std::string, ParameterSchemaEntry> parameters; // Parameter name -> schema entry
	std::set<std::string> requiredParams;                // Set of required parameter names
	
	ComponentSchema() = default;
	explicit ComponentSchema(const std::string& type) : componentType(type) {}
};

// Singleton registry for parameter schemas
class ParameterSchemaRegistry
{
public:
	// Get singleton instance
	static ParameterSchemaRegistry& GetInstance()
	{
		static ParameterSchemaRegistry instance;
		return instance;
	}
	
	// Delete copy/move constructors
	ParameterSchemaRegistry(const ParameterSchemaRegistry&) = delete;
	ParameterSchemaRegistry& operator=(const ParameterSchemaRegistry&) = delete;
	ParameterSchemaRegistry(ParameterSchemaRegistry&&) = delete;
	ParameterSchemaRegistry& operator=(ParameterSchemaRegistry&&) = delete;
	
	// Initialize built-in schemas for standard components
	void InitializeBuiltInSchemas();
	
	// Load schema from JSON file
	bool LoadSchemaFromFile(const std::string& filepath);
	
	// Find a parameter schema entry by parameter name (checks all components)
	const ParameterSchemaEntry* FindParameterSchema(const std::string& parameterName) const;
	
	// Get complete schema for a specific component type
	const ComponentSchema* GetComponentSchema(const std::string& componentType) const;
	
	// Validate a parameter against its schema
	bool ValidateParameter(const std::string& parameterName, const ComponentParameter& param) const;
	
	// Register a new parameter schema entry
	void RegisterParameterSchema(const ParameterSchemaEntry& entry);
	
private:
	// Private constructor for singleton
	ParameterSchemaRegistry() = default;
	~ParameterSchemaRegistry() = default;
	
	// Storage
	std::map<std::string, ComponentSchema> componentSchemas_;  // Component type -> schema
	std::map<std::string, std::string> parameterToComponent_;  // Parameter name -> component type (for fast lookup)
	std::map<std::string, std::string> aliasToParameter_;      // Alias -> canonical parameter name
};
