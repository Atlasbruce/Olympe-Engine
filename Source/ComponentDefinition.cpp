/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ComponentDefinition implementation: Dynamic component parameter storage and JSON parsing
for entity components. Supports type-safe parameter access and conversions.
*/

#include "ComponentDefinition.h"
#include "ParameterSchema.h"
#include "third_party/nlohmann/json.hpp"
#include "system/system_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>

using nlohmann::json;

// Helper function for clamping color values to valid range
static inline uint8_t ClampColorValue(int value)
{
	return static_cast<uint8_t>(value < 0 ? 0 : (value > 255 ? 255 : value));
}

// Helper function to convert ParameterType enum to string for logging
std::string ParameterTypeToString(ComponentParameter::Type type)
{
	switch (type) {
		case ComponentParameter::Type::Bool:    return "Bool";
		case ComponentParameter::Type::Int:     return "Int";
		case ComponentParameter::Type::Float:   return "Float";
		case ComponentParameter::Type::String:  return "String";
		case ComponentParameter::Type::Vector2: return "Vector2";
		case ComponentParameter::Type::Vector3: return "Vector3";
		case ComponentParameter::Type::Color:   return "Color";
		case ComponentParameter::Type::Array:   return "Array";
		case ComponentParameter::Type::EntityRef: return "EntityRef";
		default:                                return "Unknown";
	}
}

// ============================================================================
// ComponentParameter Factory Methods
// ============================================================================

ComponentParameter ComponentParameter::FromBool(bool value)
{
	ComponentParameter param;
	param.type = Type::Bool;
	param.boolValue = value;
	return param;
}

ComponentParameter ComponentParameter::FromInt(int value)
{
	ComponentParameter param;
	param.type = Type::Int;
	param.intValue = value;
	return param;
}

ComponentParameter ComponentParameter::FromFloat(float value)
{
	ComponentParameter param;
	param.type = Type::Float;
	param.floatValue = value;
	return param;
}

ComponentParameter ComponentParameter::FromString(const std::string& value)
{
	ComponentParameter param;
	param.type = Type::String;
	param.stringValue = value;
	return param;
}

ComponentParameter ComponentParameter::FromVector2(float x, float y)
{
	ComponentParameter param;
	param.type = Type::Vector2;
	param.vectorValue = Vector(x, y, 0.0f);
	return param;
}

ComponentParameter ComponentParameter::FromVector3(float x, float y, float z)
{
	ComponentParameter param;
	param.type = Type::Vector3;
	param.vectorValue = Vector(x, y, z);
	return param;
}

ComponentParameter ComponentParameter::FromColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	ComponentParameter param;
	param.type = Type::Color;
	param.colorValue = { r, g, b, a };
	return param;
}

ComponentParameter ComponentParameter::FromEntityRef(EntityID entityId)
{
	ComponentParameter param;
	param.type = Type::EntityRef;
	param.entityRefValue = entityId;
	return param;
}

ComponentParameter ComponentParameter::FromArray(const nlohmann::json& arrayData)
{
	ComponentParameter param;
	param.type = Type::Array;
	// Store array in shared_ptr for efficient copying
	param.arrayValue = std::make_shared<nlohmann::json>(arrayData);
	return param;
}

// ============================================================================
// ComponentParameter Type Conversion Methods
// ============================================================================

bool ComponentParameter::AsBool() const
{
	switch (type)
	{
	case Type::Bool:
		return boolValue;
	case Type::Int:
		return intValue != 0;
	case Type::Float:
		return floatValue != 0.0f;
	case Type::String:
	{
		std::string lower = stringValue;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return lower == "true" || lower == "1" || lower == "yes";
	}
	case Type::EntityRef:
		return entityRefValue != INVALID_ENTITY_ID;
	default:
		return false;
	}
}

int ComponentParameter::AsInt() const
{
	switch (type)
	{
	case Type::Int:
		return intValue;
	case Type::Bool:
		return boolValue ? 1 : 0;
	case Type::Float:
		return static_cast<int>(floatValue);
	case Type::String:
		try {
			return std::stoi(stringValue);
		}
		catch (...) {
			return 0;
		}
	case Type::EntityRef:
		return static_cast<int>(entityRefValue);
	default:
		return 0;
	}
}

float ComponentParameter::AsFloat() const
{
	switch (type)
	{
	case Type::Float:
		return floatValue;
	case Type::Int:
		return static_cast<float>(intValue);
	case Type::Bool:
		return boolValue ? 1.0f : 0.0f;
	case Type::String:
		try {
			return std::stof(stringValue);
		}
		catch (...) {
			return 0.0f;
		}
	default:
		return 0.0f;
	}
}

std::string ComponentParameter::AsString() const
{
	switch (type)
	{
	case Type::String:
		return stringValue;
	case Type::Int:
		return std::to_string(intValue);
	case Type::Float:
		return std::to_string(floatValue);
	case Type::Bool:
		return boolValue ? "true" : "false";
	case Type::Vector2:
	case Type::Vector3:
	{
		std::ostringstream oss;
		oss << "(" << vectorValue.x << ", " << vectorValue.y;
		if (type == Type::Vector3)
			oss << ", " << vectorValue.z;
		oss << ")";
		return oss.str();
	}
	case Type::Color:
	{
		std::ostringstream oss;
		oss << "rgba(" << (int)colorValue.r << ", " << (int)colorValue.g 
			<< ", " << (int)colorValue.b << ", " << (int)colorValue.a << ")";
		return oss.str();
	}
	case Type::Array:
	{
		// Convert array to JSON string representation
		if (arrayValue)
			return arrayValue->dump();
		else
			return "[]";
	}
	case Type::EntityRef:
		return std::to_string(entityRefValue);
	default:
		return "";
	}
}

Vector ComponentParameter::AsVector() const
{
	switch (type)
	{
	case Type::Vector2:
	case Type::Vector3:
		return vectorValue;
	case Type::String:
	{
		// Parse string like "(x, y)" or "(x, y, z)"
		std::string s = stringValue;
		s.erase(std::remove(s.begin(), s.end(), '('), s.end());
		s.erase(std::remove(s.begin(), s.end(), ')'), s.end());
		
		std::istringstream iss(s);
		std::string token;
		Vector result;
		int idx = 0;
		
		while (std::getline(iss, token, ',') && idx < 3)
		{
			// Trim whitespace
			token.erase(0, token.find_first_not_of(" \t\n\r"));
			token.erase(token.find_last_not_of(" \t\n\r") + 1);
			
			try {
				float val = std::stof(token);
				if (idx == 0) result.x = val;
				else if (idx == 1) result.y = val;
				else if (idx == 2) result.z = val;
				idx++;
			}
			catch (...) {
				break;
			}
		}
		return result;
	}
	default:
		return Vector(0.0f, 0.0f, 0.0f);
	}
}

SDL_Color ComponentParameter::AsColor() const
{
	switch (type)
	{
	case Type::Color:
		return colorValue;
	case Type::String:
	{
		// Parse hex color like "#RRGGBB" or "#RRGGBBAA"
		std::string s = stringValue;
		if (!s.empty() && s[0] == '#' && (s.length() == 7 || s.length() == 9))
		{
			try {
				unsigned long val = std::stoul(s.substr(1), nullptr, 16);
				if (s.length() == 7) {
					return SDL_Color{
						static_cast<uint8_t>((val >> 16) & 0xFF),
						static_cast<uint8_t>((val >> 8) & 0xFF),
						static_cast<uint8_t>(val & 0xFF),
						255
					};
				}
				else {
					return SDL_Color{
						static_cast<uint8_t>((val >> 24) & 0xFF),
						static_cast<uint8_t>((val >> 16) & 0xFF),
						static_cast<uint8_t>((val >> 8) & 0xFF),
						static_cast<uint8_t>(val & 0xFF)
					};
				}
			}
			catch (...) {
				SYSTEM_LOG << "Failed to parse color from hex string: " << s << std::endl;
			}
		}
		// Parse rgba like "rgba(r, g, b, a)" or "rgb(r, g, b)"
		else if (s.find("rgb") == 0)
		{
			std::string nums = s.substr(s.find('(') + 1);
			nums = nums.substr(0, nums.find(')'));
			
			std::istringstream iss(nums);
			std::string token;
			int values[4] = { 0, 0, 0, 255 };
			int idx = 0;
			
			while (std::getline(iss, token, ',') && idx < 4)
			{
				// Trim whitespace
				token.erase(0, token.find_first_not_of(" \t\n\r"));
				token.erase(token.find_last_not_of(" \t\n\r") + 1);
				
				try {
					values[idx] = ClampColorValue(std::stoi(token));
					idx++;
				}
				catch (...) {
					break;
				}
			}
			
			return SDL_Color{
				static_cast<uint8_t>(values[0]),
				static_cast<uint8_t>(values[1]),
				static_cast<uint8_t>(values[2]),
				static_cast<uint8_t>(values[3])
			};
		}
		return SDL_Color{ 255, 255, 255, 255 };
	}
	default:
		return SDL_Color{ 255, 255, 255, 255 };
	}
}

EntityID ComponentParameter::AsEntityRef() const
{
	switch (type)
	{
	case Type::EntityRef:
		return entityRefValue;
	case Type::Int:
		return static_cast<EntityID>(intValue);
	case Type::String:
		try {
			return static_cast<EntityID>(std::stoull(stringValue));
		}
		catch (...) {
			return INVALID_ENTITY_ID;
		}
	default:
		return INVALID_ENTITY_ID;
	}
}

const nlohmann::json& ComponentParameter::AsArray() const
{
	// Return empty array if not an array type or arrayValue is null
	static const nlohmann::json emptyArray = nlohmann::json::array();
	
	if (type == Type::Array && arrayValue)
	{
		return *arrayValue;
	}
	
	return emptyArray;
}

// ============================================================================
// ComponentDefinition JSON Parsing
// ============================================================================

// Parse a single parameter with schema-aware type detection
ComponentParameter ParseParameterWithSchema(
	const std::string& componentType,
	const std::string& paramName,
	const nlohmann::json& jsonValue)
{
	ComponentParameter param;
	
	// Step 1: Try to get schema for this component
	const ComponentSchema* schema = ParameterSchemaRegistry::GetInstance().GetComponentSchema(componentType);
	
	// Step 2: If schema exists, look up the parameter type
	if (schema != nullptr)
	{
		auto paramIt = schema->parameters.find(paramName);
		if (paramIt != schema->parameters.end())
		{
			const ParameterSchemaEntry& schemaEntry = paramIt->second;
			
			// Force conversion based on schema type
			switch (schemaEntry.expectedType) {
				case ComponentParameter::Type::Bool:
					if (jsonValue.is_boolean()) {
						param = ComponentParameter::FromBool(jsonValue.get<bool>());
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Bool for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromBool(false);
					}
					break;
					
				case ComponentParameter::Type::Int:
					if (jsonValue.is_number()) {
						// Convert any number to int
						param = ComponentParameter::FromInt(static_cast<int>(jsonValue.get<double>()));
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Int for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromInt(0);
					}
					break;
					
				case ComponentParameter::Type::Float:
					if (jsonValue.is_number()) {
						// CRITICAL FIX: Always convert to float
						param = ComponentParameter::FromFloat(static_cast<float>(jsonValue.get<double>()));
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Float for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromFloat(0.0f);
					}
					break;
					
				case ComponentParameter::Type::String:
					if (jsonValue.is_string()) {
						param = ComponentParameter::FromString(jsonValue.get<std::string>());
					} else {
						param = ComponentParameter::FromString("");
					}
					break;
					
				case ComponentParameter::Type::Vector2:
					if (jsonValue.is_object() && jsonValue.contains("x") && jsonValue.contains("y")) {
						float x = static_cast<float>(jsonValue["x"].get<double>());
						float y = static_cast<float>(jsonValue["y"].get<double>());
						param = ComponentParameter::FromVector2(x, y);
					} else if (jsonValue.is_array() && jsonValue.size() == 2) {
						// Support array format: [x, y]
						float x = jsonValue[0].get<float>();
						float y = jsonValue[1].get<float>();
						param = ComponentParameter::FromVector2(x, y);
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Vector2 for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromVector2(0.0f, 0.0f);
					}
					break;
					
				case ComponentParameter::Type::Vector3:
					if (jsonValue.is_object() && jsonValue.contains("x") && jsonValue.contains("y") && jsonValue.contains("z")) {
						float x = static_cast<float>(jsonValue["x"].get<double>());
						float y = static_cast<float>(jsonValue["y"].get<double>());
						float z = static_cast<float>(jsonValue["z"].get<double>());
						param = ComponentParameter::FromVector3(x, y, z);
					} else if (jsonValue.is_array() && jsonValue.size() == 3) {
						// Support array format: [x, y, z]
						float x = jsonValue[0].get<float>();
						float y = jsonValue[1].get<float>();
						float z = jsonValue[2].get<float>();
						param = ComponentParameter::FromVector3(x, y, z);
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Vector3 for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromVector3(0.0f, 0.0f, 0.0f);
					}
					break;
					
				case ComponentParameter::Type::Color:
					if (jsonValue.is_string()) {
						std::string colorStr = jsonValue.get<std::string>();
						// Parse color using existing AsColor logic
						param = ComponentParameter::FromString(colorStr);
						param.type = ComponentParameter::Type::Color;
						param.colorValue = param.AsColor();
					} else if (jsonValue.is_object() && jsonValue.contains("r") && jsonValue.contains("g") && jsonValue.contains("b")) {
						// Support object format: {"r": 255, "g": 128, "b": 64, "a": 255}
						uint8_t r = jsonValue["r"].is_number() ? ClampColorValue(jsonValue["r"].get<int>()) : 255;
						uint8_t g = jsonValue["g"].is_number() ? ClampColorValue(jsonValue["g"].get<int>()) : 255;
						uint8_t b = jsonValue["b"].is_number() ? ClampColorValue(jsonValue["b"].get<int>()) : 255;
						uint8_t a = jsonValue.contains("a") && jsonValue["a"].is_number() ? 
									ClampColorValue(jsonValue["a"].get<int>()) : 255;
						param = ComponentParameter::FromColor(r, g, b, a);
					} else if (jsonValue.is_array() && jsonValue.size() >= 3 && jsonValue.size() <= 4) {
						// Support array format: [r, g, b] or [r, g, b, a]
						uint8_t r = ClampColorValue(jsonValue[0].get<int>());
						uint8_t g = ClampColorValue(jsonValue[1].get<int>());
						uint8_t b = ClampColorValue(jsonValue[2].get<int>());
						uint8_t a = jsonValue.size() == 4 ? ClampColorValue(jsonValue[3].get<int>()) : 255;
						param = ComponentParameter::FromColor(r, g, b, a);
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Color for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromString("#FFFFFF");
						param.type = ComponentParameter::Type::Color;
						param.colorValue = param.AsColor();
					}
					break;
					
				case ComponentParameter::Type::EntityRef:
					if (jsonValue.is_string()) {
						try {
							EntityID entityId = std::stoull(jsonValue.get<std::string>());
							param = ComponentParameter::FromEntityRef(entityId);
						}
						catch (...) {
							SYSTEM_LOG << "  [WARN] Failed to parse EntityRef from string for '" << paramName << "'" << std::endl;
							param = ComponentParameter::FromEntityRef(INVALID_ENTITY_ID);
						}
					} else if (jsonValue.is_number()) {
						// Use double conversion to avoid MSVC linkage issues with int64_t types
						param = ComponentParameter::FromEntityRef(static_cast<EntityID>(jsonValue.get<double>()));
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects EntityRef for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromEntityRef(INVALID_ENTITY_ID);
					}
					break;
					
				case ComponentParameter::Type::Array:
					// Array type: store as-is (do NOT coerce into Vector unless schema expects Vector2/Vector3)
					if (jsonValue.is_array()) {
						param = ComponentParameter::FromArray(jsonValue);
					} else {
						SYSTEM_LOG << "  [WARN] Schema expects Array for '" << paramName 
								   << "', got " << jsonValue.type_name() << std::endl;
						param = ComponentParameter::FromArray(nlohmann::json::array());
					}
					break;
					
				default:
					SYSTEM_LOG << "  [ERROR] Unknown schema type for '" << paramName << "'" << std::endl;
					break;
			}
			
			#ifdef DEBUG_PARAMETER_PARSING
			SYSTEM_LOG << "  [SCHEMA-AWARE] " << paramName << " -> " 
					   << ParameterTypeToString(schemaEntry.expectedType) 
					   << " (from schema)" << std::endl;
			#endif
			
			return param;
		}
	}
	
	// FALLBACK: No schema found, use improved heuristics
	#ifdef DEBUG_PARAMETER_PARSING
	SYSTEM_LOG << "  [WARN] No schema for '" << componentType << "." << paramName << "', using heuristics" << std::endl;
	#endif
	
	if (jsonValue.is_boolean())
	{
		param = ComponentParameter::FromBool(jsonValue.get<bool>());
	}
	else if (jsonValue.is_number())
	{
		// IMPROVED HEURISTIC: Prefer Float over Int for all numbers
		// This matches most common use cases (speed, mass, positions, etc.)
		double numValue = jsonValue.get<double>();
		param = ComponentParameter::FromFloat(static_cast<float>(numValue));
		
		#ifdef DEBUG_PARAMETER_PARSING
		SYSTEM_LOG << "    -> Inferred as Float (" << numValue << ")" << std::endl;
		#endif
	}
	else if (jsonValue.is_string())
	{
		std::string strValue = jsonValue.get<std::string>();
		
		// Check if it's a color (hex or rgba format)
		if ((!strValue.empty() && strValue[0] == '#' && (strValue.length() == 7 || strValue.length() == 9)) ||
			strValue.find("rgb") == 0)
		{
			param = ComponentParameter::FromString(strValue);
			param.type = ComponentParameter::Type::Color;
			param.colorValue = param.AsColor();
		}
		// Check if key hints at entity reference
		else if (paramName.find("entity") != std::string::npos || 
				 paramName.find("Entity") != std::string::npos ||
				 paramName.find("ref") != std::string::npos ||
				 paramName.find("Ref") != std::string::npos)
		{
			try {
				EntityID entityId = std::stoull(strValue);
				param = ComponentParameter::FromEntityRef(entityId);
			}
			catch (...) {
				param = ComponentParameter::FromString(strValue);
			}
		}
		else
		{
			param = ComponentParameter::FromString(strValue);
		}
	}
	else if (jsonValue.is_object())
	{
		// Try Vector2/Vector3
		if (jsonValue.contains("x") && jsonValue.contains("y") && jsonValue.contains("z"))
		{
			float x = static_cast<float>(jsonValue["x"].get<double>());
			float y = static_cast<float>(jsonValue["y"].get<double>());
			float z = static_cast<float>(jsonValue["z"].get<double>());
			param = ComponentParameter::FromVector3(x, y, z);
		}
		else if (jsonValue.contains("x") && jsonValue.contains("y"))
		{
			float x = static_cast<float>(jsonValue["x"].get<double>());
			float y = static_cast<float>(jsonValue["y"].get<double>());
			param = ComponentParameter::FromVector2(x, y);
		}
		// Check for color object
		else if (jsonValue.contains("r") && jsonValue.contains("g") && jsonValue.contains("b"))
		{
			uint8_t r = jsonValue["r"].is_number() ? ClampColorValue(jsonValue["r"].get<int>()) : 255;
			uint8_t g = jsonValue["g"].is_number() ? ClampColorValue(jsonValue["g"].get<int>()) : 255;
			uint8_t b = jsonValue["b"].is_number() ? ClampColorValue(jsonValue["b"].get<int>()) : 255;
			uint8_t a = jsonValue.contains("a") && jsonValue["a"].is_number() ? 
						ClampColorValue(jsonValue["a"].get<int>()) : 255;
			
			param = ComponentParameter::FromColor(r, g, b, a);
		}
		else
		{
			// Store as JSON string
			param = ComponentParameter::FromString(jsonValue.dump());
		}
	}
	else if (jsonValue.is_array())
	{
		// Handle arrays based on size to avoid ambiguity
		// IMPORTANT: Only coerce to Vector2/Vector3/Color if schema is not available
		// Otherwise, preserve array structure for proper Array type parameters
		if (jsonValue.size() == 2 && jsonValue[0].is_number())
		{
			// Could be Vector2 - but check if paramName suggests array (e.g., "waypoints", "path")
			if (paramName.find("waypoint") != std::string::npos ||
			    paramName.find("path") != std::string::npos ||
			    paramName.find("points") != std::string::npos ||
			    paramName.find("Path") != std::string::npos)
			{
				// Preserve as array
				param = ComponentParameter::FromArray(jsonValue);
			}
			else
			{
				// Treat as Vector2
				float x = jsonValue[0].get<float>();
				float y = jsonValue[1].get<float>();
				param = ComponentParameter::FromVector2(x, y);
			}
		}
		else if (jsonValue.size() == 3 && jsonValue[0].is_number())
		{
			// Ambiguous: could be Vector3 or RGB color
			// Use heuristic: if values are in typical color range (0-255), treat as color
			double val0 = jsonValue[0].get<double>();
			double val1 = jsonValue[1].get<double>();
			double val2 = jsonValue[2].get<double>();
			
			// If all values are in [0, 255] range and are integers, likely a color
			bool looksLikeColor = (val0 >= 0 && val0 <= 255 && val0 == (int)val0) &&
			                      (val1 >= 0 && val1 <= 255 && val1 == (int)val1) &&
			                      (val2 >= 0 && val2 <= 255 && val2 == (int)val2);
			
			if (looksLikeColor)
			{
				param = ComponentParameter::FromColor(
					ClampColorValue((int)val0),
					ClampColorValue((int)val1),
					ClampColorValue((int)val2),
					255
				);
			}
			else
			{
				// Treat as Vector3
				float x = jsonValue[0].get<float>();
				float y = jsonValue[1].get<float>();
				float z = jsonValue[2].get<float>();
				param = ComponentParameter::FromVector3(x, y, z);
			}
		}
		else if (jsonValue.size() == 4 && jsonValue[0].is_number())
		{
			// 4 elements: likely RGBA color
			uint8_t r = ClampColorValue(jsonValue[0].get<int>());
			uint8_t g = ClampColorValue(jsonValue[1].get<int>());
			uint8_t b = ClampColorValue(jsonValue[2].get<int>());
			uint8_t a = ClampColorValue(jsonValue[3].get<int>());
			param = ComponentParameter::FromColor(r, g, b, a);
		}
		else
		{
			// Non-standard size or non-numeric: preserve as Array type
			param = ComponentParameter::FromArray(jsonValue);
		}
	}
	else
	{
		// Unknown type, store as string
		param = ComponentParameter::FromString(jsonValue.dump());
	}
	
	return param;
}

ComponentDefinition ComponentDefinition::FromJSON(const nlohmann::json& jsonObj)
{
	ComponentDefinition def;
	
	try
	{
		// Extract component type
		if (jsonObj.contains("type") && jsonObj["type"].is_string())
		{
			def.componentType = jsonObj["type"].get<std::string>();
		}
		else
		{
			SYSTEM_LOG << "Warning: Component definition missing 'type' field" << std::endl;
			return def;
		}
		
		// Check if there's a "properties" object - if so, parse its contents instead of top-level fields
		const json& fieldsToIterate = (jsonObj.contains("properties") && jsonObj["properties"].is_object()) 
									   ? jsonObj["properties"] 
									   : jsonObj;
		
		// Parse all fields as parameters using schema-aware parsing
		for (auto it = fieldsToIterate.begin(); it != fieldsToIterate.end(); ++it)
		{
			const std::string& key = it.key();
			
			// Skip the type field (normally at top level, but safeguard against it in properties)
			if (key == "type")
				continue;
			
			const auto& value = it.value();
			
			// Use schema-aware parsing
			ComponentParameter param = ParseParameterWithSchema(def.componentType, key, value);
			
			def.parameters[key] = param;
		}
	}
	catch (const std::exception& e)
	{
		SYSTEM_LOG << "Error parsing component JSON: " << e.what() << std::endl;
	}
	
	return def;
}

// ============================================================================
// ComponentDefinition Parameter Access
// ============================================================================

bool ComponentDefinition::HasParameter(const std::string& name) const
{
	return parameters.find(name) != parameters.end();
}

const ComponentParameter* ComponentDefinition::GetParameter(const std::string& name) const
{
	auto it = parameters.find(name);
	if (it != parameters.end())
		return &(it->second);
	return nullptr;
}
