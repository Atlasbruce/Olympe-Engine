/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ParameterResolver implementation: Resolves and merges prefab blueprint parameters 
with level instance parameters using priority-based merging.
*/

#include "ParameterResolver.h"
#include "ParameterSchema.h"
#include "system/system_utils.h"
#include <algorithm>

ParameterResolver::ParameterResolver()
{
	SYSTEM_LOG << "[ParameterResolver] Initialized" << std::endl;
}

ParameterResolver::~ParameterResolver()
{
}

std::vector<ResolvedComponentInstance> ParameterResolver::Resolve(
	const PrefabBlueprint& prefab,
	const LevelInstanceParameters& instanceParams)
{
	SYSTEM_LOG << "[ParameterResolver] Resolving prefab '" << prefab.prefabName 
	           << "' for instance '" << instanceParams.objectName << "'" << std::endl;
	
	std::vector<ResolvedComponentInstance> resolvedComponents;
	
	// Iterate through all components in the prefab
	for (const auto& componentDef : prefab.components)
	{
		SYSTEM_LOG << "[ParameterResolver]   Processing component: " 
		           << componentDef.componentType << std::endl;
		
		// Resolve each component with instance parameters
		ResolvedComponentInstance resolved = ResolveComponent(componentDef, instanceParams);
		
		// Validate the resolved component
		ValidateResolvedComponent(resolved);
		
		if (!resolved.isValid)
		{
			SYSTEM_LOG << "[ParameterResolver]   WARNING: Component resolution failed for " 
			           << resolved.componentType << std::endl;
			for (const auto& error : resolved.errors)
			{
				SYSTEM_LOG << "[ParameterResolver]     Error: " << error << std::endl;
			}
		}
		
		resolvedComponents.push_back(resolved);
	}
	
	SYSTEM_LOG << "[ParameterResolver] Resolution complete. Resolved " 
	           << resolvedComponents.size() << " components" << std::endl;
	
	return resolvedComponents;
}

ResolvedComponentInstance ParameterResolver::ResolveComponent(
	const ComponentDefinition& componentDef,
	const LevelInstanceParameters& instanceParams)
{
	ResolvedComponentInstance resolved(componentDef.componentType);
	
	// Step 1: Start with prefab defaults (LOWEST priority)
	resolved.parameters = componentDef.parameters;
	
	// Check if this component needs special position handling
	bool isPositionComponent = (componentDef.componentType == "Position_data" || 
	                           componentDef.componentType == "Position");
	
	// Extract level overrides for this component
	auto componentParams = ExtractComponentParameters(componentDef.componentType, instanceParams);
	
	// Fast path: No overrides and not a position component - use prefab defaults directly
	if (!isPositionComponent && componentParams.empty())
	{
		SYSTEM_LOG << "[ParameterResolver]     ✓ Fast path: 0 overrides (using prefab defaults)" << std::endl;
		return resolved;
	}
	
	SYSTEM_LOG << "[ParameterResolver]     Starting with " << resolved.parameters.size() 
	           << " prefab default parameters" << std::endl;
	
	// Step 2: Apply position override (HIGHEST priority)
	// Position is special - it's always from the level instance
	if (isPositionComponent)
	{
		SYSTEM_LOG << "[ParameterResolver]     Applying level position override: (" 
		           << instanceParams.position.x << ", " 
		           << instanceParams.position.y << ", " 
		           << instanceParams.position.z << ")" << std::endl;
		
		resolved.parameters["x"] = ComponentParameter::FromFloat(instanceParams.position.x);
		resolved.parameters["y"] = ComponentParameter::FromFloat(instanceParams.position.y);
		resolved.parameters["z"] = ComponentParameter::FromFloat(instanceParams.position.z);
	}
	
	// Step 3: Apply level custom property overrides (HIGH priority)
	if (!componentParams.empty())
	{
		SYSTEM_LOG << "[ParameterResolver]     Applying " << componentParams.size() 
		           << " level property overrides" << std::endl;
		
		for (const auto& it : componentParams)
		{
		    const std::string& propName = it.first;
		    const ComponentParameter& propValue = it.second;
		    ApplyPropertyOverride(resolved, propName, propValue);
		}
	}
	
	return resolved;
}

void ParameterResolver::ApplyPropertyOverride(
	ResolvedComponentInstance& component,
	const std::string& propertyName,
	const ComponentParameter& propertyValue)
{
	// Get schema to determine expected type
	auto& schemaRegistry = ParameterSchemaRegistry::GetInstance();
	const ParameterSchemaEntry* schema = schemaRegistry.FindParameterSchema(propertyName);
	
	if (schema != nullptr)
	{
		// Convert property value to expected type
		ComponentParameter converted = ConvertLevelProperty(
			propertyName, 
			propertyValue, 
			schema->expectedType
		);
		
		// Override the parameter
		component.parameters[propertyName] = converted;
		
		SYSTEM_LOG << "[ParameterResolver]       Override: " << propertyName 
		           << " (type: " << static_cast<int>(converted.type) << ")" << std::endl;
	}
	else
	{
		// No schema found - apply as-is
		component.parameters[propertyName] = propertyValue;
		
		SYSTEM_LOG << "[ParameterResolver]       Override (no schema): " << propertyName 
		           << " (type: " << static_cast<int>(propertyValue.type) << ")" << std::endl;
	}
}

std::map<std::string, ComponentParameter> ParameterResolver::ExtractComponentParameters(
	const std::string& componentType,
	const LevelInstanceParameters& instanceParams)
{
	std::map<std::string, ComponentParameter> componentParams;
	
	// Get component schema
	auto& schemaRegistry = ParameterSchemaRegistry::GetInstance();
	const ComponentSchema* schema = schemaRegistry.GetComponentSchema(componentType);
	
	if (schema == nullptr)
	{
		SYSTEM_LOG << "[ParameterResolver]       No schema found for component: " 
		           << componentType << std::endl;
		return componentParams;
	}
	
	// Extract parameters that belong to this component
	for (const auto& paramPair : schema->parameters)
	{
		const std::string& paramName = paramPair.first;
		// Check if level instance has this parameter
		auto it = instanceParams.properties.find(paramName);
		if (it != instanceParams.properties.end())
		{
			componentParams[paramName] = it->second;
		}
	}
	
	return componentParams;
}

ComponentParameter ParameterResolver::ConvertLevelProperty(
	const std::string& propertyName,
	const ComponentParameter& levelProperty,
	ComponentParameter::Type expectedType)
{
	// If types match, return as-is
	if (levelProperty.type == expectedType)
	{
		return levelProperty;
	}
	
	SYSTEM_LOG << "[ParameterResolver]       Type conversion: " << propertyName 
	           << " from type " << static_cast<int>(levelProperty.type) 
	           << " to " << static_cast<int>(expectedType) << std::endl;
	
	// Perform type conversion based on expected type
	switch (expectedType)
	{
		case ComponentParameter::Type::Bool:
			return ComponentParameter::FromBool(levelProperty.AsBool());
			
		case ComponentParameter::Type::Int:
			return ComponentParameter::FromInt(levelProperty.AsInt());
			
		case ComponentParameter::Type::Float:
			return ComponentParameter::FromFloat(levelProperty.AsFloat());
			
		case ComponentParameter::Type::String:
			return ComponentParameter::FromString(levelProperty.AsString());
			
		case ComponentParameter::Type::Vector2:
		case ComponentParameter::Type::Vector3:
		{
			Vector v = levelProperty.AsVector();
			return ComponentParameter::FromVector3(v.x, v.y, v.z);
		}
		
		case ComponentParameter::Type::Color:
			return ComponentParameter::FromColor(
				levelProperty.colorValue.r,
				levelProperty.colorValue.g,
				levelProperty.colorValue.b,
				levelProperty.colorValue.a
			);
			
		case ComponentParameter::Type::EntityRef:
			return ComponentParameter::FromEntityRef(levelProperty.AsEntityRef());
			
		default:
			SYSTEM_LOG << "[ParameterResolver]       WARNING: Unknown type conversion for " 
			           << propertyName << std::endl;
			return levelProperty;
	}
}

void ParameterResolver::ValidateResolvedComponent(ResolvedComponentInstance& component)
{
	// Get component schema for validation
	auto& schemaRegistry = ParameterSchemaRegistry::GetInstance();
	const ComponentSchema* schema = schemaRegistry.GetComponentSchema(component.componentType);
	
	// Fast path: Skip validation if no schema exists
	if (schema == nullptr)
	{
		// No schema available - skip validation silently
		return;
	}
	
	bool hasErrors = false;
	
	// Check for missing required parameters
	for (const auto& requiredParam : schema->requiredParams)
	{
		if (component.parameters.find(requiredParam) == component.parameters.end())
		{
			std::string error = "Missing required parameter: " + requiredParam;
			component.errors.push_back(error);
			component.isValid = false;
			hasErrors = true;
			
			SYSTEM_LOG << "[ParameterResolver]     Validation ERROR: " << error << std::endl;
		}
	}
	
	// Validate parameter types (only log actual mismatches)
	for (const auto& paramPair : component.parameters)
	{
		const std::string& paramName = paramPair.first;
		const ComponentParameter& paramValue = paramPair.second;
		
		auto schemaIt = schema->parameters.find(paramName);
		if (schemaIt != schema->parameters.end())
		{
			const ParameterSchemaEntry& schemaEntry = schemaIt->second;
			
			// Type validation
			if (paramValue.type != schemaEntry.expectedType && 
			    paramValue.type != ComponentParameter::Type::Unknown)
			{
				std::string error = "Type mismatch for parameter '" + paramName + 
				                    "': expected " + std::to_string(static_cast<int>(schemaEntry.expectedType)) +
				                    ", got " + std::to_string(static_cast<int>(paramValue.type));
				component.errors.push_back(error);
				hasErrors = true;
				
				SYSTEM_LOG << "[ParameterResolver]     Validation WARNING: " << error << std::endl;
			}
		}
	}
	
	// Only log success if there were no errors (reduces noise)
	if (!hasErrors && component.isValid)
	{
		// Silently succeed - no need to log
	}
	else if (!component.isValid)
	{
		SYSTEM_LOG << "[ParameterResolver]     Validation: Component " 
		           << component.componentType << " has " << component.errors.size() 
		           << " errors" << std::endl;
	}
}
