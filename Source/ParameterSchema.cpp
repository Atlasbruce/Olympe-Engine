/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ParameterSchema purpose: Implementation of the parameter schema registry.
*/

#include "ParameterSchema.h"
#include "system/system_utils.h"
#include <algorithm>

void ParameterSchemaRegistry::InitializeBuiltInSchemas()
{
	SYSTEM_LOG << "Initializing built-in parameter schemas..." << std::endl;
	
	// Position_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"position", "Position_data", "position",
		ComponentParameter::Type::Vector3, true,
		ComponentParameter::FromVector3(0.0f, 0.0f, 0.0f)
	));
	
	// Register aliases for position
	aliasToParameter_["pos"] = "position";
	aliasToParameter_["location"] = "position";
	
	// PhysicsBody_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"speed", "PhysicsBody_data", "speed",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(150.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"mass", "PhysicsBody_data", "mass",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(1.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"friction", "PhysicsBody_data", "friction",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(0.5f)
	));
	
	// AIBlackboard_data component schemas (note: AIBlackboard_data not in ECS_Components.h, using generic approach)
	RegisterParameterSchema(ParameterSchemaEntry(
		"alertRadius", "AIBlackboard_data", "alertRadius",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(100.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"health", "AIBlackboard_data", "health",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(100)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"patrolPoints", "AIBlackboard_data", "patrolPoints",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	// VisualSprite_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"spritePath", "VisualSprite_data", "spritePath",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"hotSpot", "VisualSprite_data", "hotSpot",
		ComponentParameter::Type::Vector2, false,
		ComponentParameter::FromVector2(0.0f, 0.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"color", "VisualSprite_data", "color",
		ComponentParameter::Type::Color, false,
		ComponentParameter::FromColor(255, 255, 255, 255)
	));
	
	// Identity_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"name", "Identity_data", "name",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Entity")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"type", "Identity_data", "type",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Generic")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"category", "Identity_data", "tag",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Untagged")
	));
	
	// BoundingBox_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"width", "BoundingBox_data", "width",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(25.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"height", "BoundingBox_data", "height",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(25.0f)
	));
	
	SYSTEM_LOG << "Built-in parameter schemas initialized: " 
	           << parameterToComponent_.size() << " parameters registered." << std::endl;
}

bool ParameterSchemaRegistry::LoadSchemaFromFile(const std::string& filepath)
{
	SYSTEM_LOG << "LoadSchemaFromFile not yet implemented: " << filepath << std::endl;
	return false;
}

const ParameterSchemaEntry* ParameterSchemaRegistry::FindParameterSchema(const std::string& parameterName) const
{
	// Check if this is an alias first
	std::string actualParamName = parameterName;
	auto aliasIt = aliasToParameter_.find(parameterName);
	if (aliasIt != aliasToParameter_.end())
	{
		actualParamName = aliasIt->second;
	}
	
	// Find which component this parameter belongs to
	auto it = parameterToComponent_.find(actualParamName);
	if (it == parameterToComponent_.end())
	{
		return nullptr;
	}
	
	const std::string& componentType = it->second;
	
	// Get the component schema
	auto schemaIt = componentSchemas_.find(componentType);
	if (schemaIt == componentSchemas_.end())
	{
		return nullptr;
	}
	
	// Find the parameter within the component schema
	const ComponentSchema& schema = schemaIt->second;
	auto paramIt = schema.parameters.find(actualParamName);
	if (paramIt == schema.parameters.end())
	{
		return nullptr;
	}
	
	return &(paramIt->second);
}

const ComponentSchema* ParameterSchemaRegistry::GetComponentSchema(const std::string& componentType) const
{
	auto it = componentSchemas_.find(componentType);
	if (it == componentSchemas_.end())
	{
		return nullptr;
	}
	return &(it->second);
}

bool ParameterSchemaRegistry::ValidateParameter(const std::string& parameterName, const ComponentParameter& param) const
{
	const ParameterSchemaEntry* entry = FindParameterSchema(parameterName);
	if (!entry)
	{
		SYSTEM_LOG << "Warning: No schema found for parameter '" << parameterName << "'" << std::endl;
		return false;
	}
	
	// Check type compatibility
	if (entry->expectedType != param.type && param.type != ComponentParameter::Type::Unknown)
	{
		// Allow some type conversions
		bool allowConversion = false;
		
		// Int -> Float conversion
		if (entry->expectedType == ComponentParameter::Type::Float && param.type == ComponentParameter::Type::Int)
		{
			allowConversion = true;
		}
		// Float -> Int conversion (with warning)
		else if (entry->expectedType == ComponentParameter::Type::Int && param.type == ComponentParameter::Type::Float)
		{
			allowConversion = true;
			SYSTEM_LOG << "Warning: Converting float to int for parameter '" << parameterName << "'" << std::endl;
		}
		// Vector2 -> Vector3 conversion
		else if (entry->expectedType == ComponentParameter::Type::Vector3 && param.type == ComponentParameter::Type::Vector2)
		{
			allowConversion = true;
		}
		
		if (!allowConversion)
		{
			SYSTEM_LOG << "Error: Type mismatch for parameter '" << parameterName 
			           << "'. Expected type " << static_cast<int>(entry->expectedType)
			           << " but got " << static_cast<int>(param.type) << std::endl;
			return false;
		}
	}
	
	return true;
}

void ParameterSchemaRegistry::RegisterParameterSchema(const ParameterSchemaEntry& entry)
{
	// Get or create component schema
	ComponentSchema& schema = componentSchemas_[entry.targetComponent];
	if (schema.componentType.empty())
	{
		schema.componentType = entry.targetComponent;
	}
	
	// Add parameter to schema
	schema.parameters[entry.parameterName] = entry;
	
	// Track required parameters
	if (entry.isRequired)
	{
		schema.requiredParams.insert(entry.parameterName);
	}
	
	// Add to quick lookup map
	parameterToComponent_[entry.parameterName] = entry.targetComponent;
}
