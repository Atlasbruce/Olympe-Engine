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

// Forward declarations
namespace nlohmann {
	template <typename T> class basic_json;
	using json = basic_json<std::map>;
}

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

// Forward declarations
struct PrefabBlueprint;

// Singleton registry for parameter schemas
class ParameterSchemaRegistry
{
public:
	// Get singleton instance
	static ParameterSchemaRegistry& GetInstance()
	{
		static ParameterSchemaRegistry instance;
		instance.EnsureInitialized();  // Auto-initialize on first access
		return instance;
	}
	
	// Delete copy/move constructors
	ParameterSchemaRegistry(const ParameterSchemaRegistry&) = delete;
	ParameterSchemaRegistry& operator=(const ParameterSchemaRegistry&) = delete;
	ParameterSchemaRegistry(ParameterSchemaRegistry&&) = delete;
	ParameterSchemaRegistry& operator=(ParameterSchemaRegistry&&) = delete;
	
	// Initialize built-in schemas for standard components (public for manual reinit if needed)
	void InitializeBuiltInSchemas();
	
	// Load schema from JSON file (new unified method)
	bool LoadFromJSON(const std::string& filepath);
	
	// Legacy method for backward compatibility
	bool LoadSchemaFromFile(const std::string& filepath);
	
	// Get count of registered schemas
	size_t GetSchemaCount() const;
	
	// Find a parameter schema entry by parameter name (checks all components)
	const ParameterSchemaEntry* FindParameterSchema(const std::string& parameterName) const;
	
	// Get complete schema for a specific component type
	const ComponentSchema* GetComponentSchema(const std::string& componentType) const;
	
	// Validate a parameter against its schema
	bool ValidateParameter(const std::string& parameterName, const ComponentParameter& param) const;
	
	// Register a new parameter schema entry
	void RegisterParameterSchema(const ParameterSchemaEntry& entry);
	
	// Automatic schema discovery methods
	void DiscoverComponentSchema(const ComponentDefinition& componentDef);
	void DiscoverSchemasFromPrefab(const PrefabBlueprint& prefab);
	
private:
	// Private constructor for singleton
	ParameterSchemaRegistry() = default;
	~ParameterSchemaRegistry() = default;
	
	// Auto-initialization
	bool isInitialized_ = false;
	void EnsureInitialized();
	
	// Auto-register discovered parameter
	void AutoRegisterParameter(
		const std::string& componentType,
		const std::string& paramName,
		ComponentParameter::Type paramType,
		const ComponentParameter& defaultValue
	);
	
	// Helper methods for JSON loading
	ComponentParameter::Type StringToParameterType(const std::string& typeStr) const;
	ComponentParameter ParseDefaultValue(const nlohmann::json& valueJson, ComponentParameter::Type type) const;
	
	// Storage
	std::map<std::string, ComponentSchema> componentSchemas_;  // Component type -> schema
	std::map<std::string, std::string> parameterToComponent_;  // Parameter name -> component type (for fast lookup)
	std::map<std::string, std::string> aliasToParameter_;      // Alias -> canonical parameter name
};
