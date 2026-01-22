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

// Auto-initialization
void ParameterSchemaRegistry::EnsureInitialized()
{
	if (!isInitialized_)
	{
		InitializeBuiltInSchemas();
		isInitialized_ = true;
		SYSTEM_LOG << "ParameterSchemaRegistry auto-initialized" << std::endl;
	}
}

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
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"useGravity", "PhysicsBody_data", "useGravity",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(false)
	));
	
	// AIBlackboard_data component schemas
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
	
	// Note: patrolPoints and patrolRoute are different fields for AI navigation
	RegisterParameterSchema(ParameterSchemaEntry(
		"patrolPoints", "AIBlackboard_data", "patrolPoints",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"initialized", "AIBlackboard_data", "initialized",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(false)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"patrolRoute", "AIBlackboard_data", "patrolRoute",
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
	
	// Note: width, height, layer are also used in VisualEditor_data component
	RegisterParameterSchema(ParameterSchemaEntry(
		"width", "VisualSprite_data", "width",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(32)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"height", "VisualSprite_data", "height",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(32)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"layer", "VisualSprite_data", "layer",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(0)
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
	
	// "category" is an alias for "tag" field (backward compatibility)
	// If both "category" and "tag" are specified, the last one processed will take precedence
	RegisterParameterSchema(ParameterSchemaEntry(
		"category", "Identity_data", "tag",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Untagged")
	));
	
	// "tag" parameter maps to "tag" field (preferred parameter name)
	RegisterParameterSchema(ParameterSchemaEntry(
		"tag", "Identity_data", "tag",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Untagged")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"entityType", "Identity_data", "entityType",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Generic")
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
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"offsetX", "BoundingBox_data", "offsetX",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(0.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"offsetY", "BoundingBox_data", "offsetY",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(0.0f)
	));
	
	// Movement_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"speed", "Movement_data", "speed",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(100.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"acceleration", "Movement_data", "acceleration",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(500.0f)
	));
	
	// Health_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"maxHealth", "Health_data", "maxHealth",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(100)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"currentHealth", "Health_data", "currentHealth",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(100)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"invulnerable", "Health_data", "invulnerable",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(false)
	));
	
	// VisualEditor_data component schemas
	// Note: Uses spritePath (like VisualSprite_data) but different purpose (editor visualization)
	RegisterParameterSchema(ParameterSchemaEntry(
		"spritePath", "VisualEditor_data", "spritePath",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	// Note: width, height, layer are separate registrations for VisualEditor_data component
	RegisterParameterSchema(ParameterSchemaEntry(
		"width", "VisualEditor_data", "width",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(32)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"height", "VisualEditor_data", "height",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(32)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"layer", "VisualEditor_data", "layer",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(0)
	));
	
	// AIState_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"currentState", "AIState_data", "currentState",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("Idle")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"previousState", "AIState_data", "previousState",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	// AISenses_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"visionRange", "AISenses_data", "visionRange",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(150.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"hearingRange", "AISenses_data", "hearingRange",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(100.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"alertLevel", "AISenses_data", "alertLevel",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(0)
	));
	
	// BehaviorTreeRuntime_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"treePath", "BehaviorTreeRuntime_data", "treePath",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("")
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"active", "BehaviorTreeRuntime_data", "active",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(true)
	));
	
	// MoveIntent_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"targetX", "MoveIntent_data", "targetX",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(0.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"targetY", "MoveIntent_data", "targetY",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(0.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"hasTarget", "MoveIntent_data", "hasTarget",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(false)
	));
	
	// AttackIntent_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"damage", "AttackIntent_data", "damage",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(10)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"attackRange", "AttackIntent_data", "attackRange",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(50.0f)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"cooldown", "AttackIntent_data", "cooldown",
		ComponentParameter::Type::Float, false,
		ComponentParameter::FromFloat(1.0f)
	));
	
	// PlayerBinding_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"playerIndex", "PlayerBinding_data", "playerIndex",
		ComponentParameter::Type::Int, false,
		ComponentParameter::FromInt(0)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"controlScheme", "PlayerBinding_data", "controlScheme",
		ComponentParameter::Type::String, false,
		ComponentParameter::FromString("WASD")
	));
	
	// Controller_data component schemas
	RegisterParameterSchema(ParameterSchemaEntry(
		"enabled", "Controller_data", "enabled",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(true)
	));
	
	RegisterParameterSchema(ParameterSchemaEntry(
		"inputEnabled", "Controller_data", "inputEnabled",
		ComponentParameter::Type::Bool, false,
		ComponentParameter::FromBool(true)
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

// Automatic schema discovery methods
void ParameterSchemaRegistry::DiscoverComponentSchema(const ComponentDefinition& componentDef)
{
	// Check if component already has a complete schema
	auto it = componentSchemas_.find(componentDef.componentType);
	
	// For each parameter in the component definition
	for (const auto& paramPair : componentDef.parameters)
	{
		const std::string& paramName = paramPair.first;
		const ComponentParameter& paramValue = paramPair.second;
		
		// Skip if already registered
		if (it != componentSchemas_.end() && 
			it->second.parameters.find(paramName) != it->second.parameters.end())
		{
			continue;
		}
		
		// Auto-discover and register
		AutoRegisterParameter(
			componentDef.componentType,
			paramName,
			paramValue.type,
			paramValue
		);
		
		SYSTEM_LOG << "[SchemaDiscovery] Auto-registered: " 
				   << componentDef.componentType << "." << paramName 
				   << " (type: " << static_cast<int>(paramValue.type) << ")" 
				   << std::endl;
	}
}

void ParameterSchemaRegistry::DiscoverSchemasFromPrefab(const PrefabBlueprint& prefab)
{
	for (const auto& componentDef : prefab.components)
	{
		DiscoverComponentSchema(componentDef);
	}
}

void ParameterSchemaRegistry::AutoRegisterParameter(
	const std::string& componentType,
	const std::string& paramName,
	ComponentParameter::Type paramType,
	const ComponentParameter& defaultValue)
{
	ParameterSchemaEntry entry(
		paramName,
		componentType,
		paramName,  // Field name defaults to parameter name (can differ in manual registrations)
		paramType,
		false,      // Not required by default
		defaultValue
	);
	
	RegisterParameterSchema(entry);
}
