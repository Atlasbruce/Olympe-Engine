/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ComponentDefinition implementation: Dynamic component parameter storage and JSON parsing
for entity components. Supports type-safe parameter access and conversions.
*/

#include "ComponentDefinition.h"
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

// ============================================================================
// ComponentDefinition JSON Parsing
// ============================================================================

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
		const json* fieldsToIterate = &jsonObj;
		if (jsonObj.contains("properties") && jsonObj["properties"].is_object())
		{
			fieldsToIterate = &jsonObj["properties"];
		}
		
		// Parse all fields as parameters
		for (auto it = fieldsToIterate->begin(); it != fieldsToIterate->end(); ++it)
		{
			const std::string& key = it.key();
			
			// Skip the type field (shouldn't be in properties, but check anyway)
			if (key == "type")
				continue;
			
			const auto& value = it.value();
			ComponentParameter param;
			
			// Type detection heuristics
			if (value.is_boolean())
			{
				param = ComponentParameter::FromBool(value.get<bool>());
			}
			else if (value.is_number_integer())
			{
				param = ComponentParameter::FromInt(value.get<int>());
			}
			else if (value.is_number_float())
			{
				param = ComponentParameter::FromFloat(value.get<float>());
			}
			else if (value.is_string())
			{
				std::string strValue = value.get<std::string>();
				
				// Check if it's a color (hex or rgba format)
				if ((!strValue.empty() && strValue[0] == '#' && (strValue.length() == 7 || strValue.length() == 9)) ||
					strValue.find("rgb") == 0)
				{
					param = ComponentParameter::FromString(strValue);
					param.type = ComponentParameter::Type::Color;
					param.colorValue = param.AsColor();
				}
				// Check if key hints at entity reference
				else if (key.find("entity") != std::string::npos || 
						 key.find("Entity") != std::string::npos ||
						 key.find("ref") != std::string::npos ||
						 key.find("Ref") != std::string::npos)
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
			else if (value.is_object())
			{
				// Check for vector-like objects
				if (value.contains("x") && value.contains("y"))
				{
					float x = value["x"].is_number() ? value["x"].get<float>() : 0.0f;
					float y = value["y"].is_number() ? value["y"].get<float>() : 0.0f;
					
					if (value.contains("z"))
					{
						float z = value["z"].is_number() ? value["z"].get<float>() : 0.0f;
						param = ComponentParameter::FromVector3(x, y, z);
					}
					else
					{
						param = ComponentParameter::FromVector2(x, y);
					}
				}
				// Check for color object
				else if (value.contains("r") && value.contains("g") && value.contains("b"))
				{
					uint8_t r = value["r"].is_number() ? ClampColorValue(value["r"].get<int>()) : 255;
					uint8_t g = value["g"].is_number() ? ClampColorValue(value["g"].get<int>()) : 255;
					uint8_t b = value["b"].is_number() ? ClampColorValue(value["b"].get<int>()) : 255;
					uint8_t a = value.contains("a") && value["a"].is_number() ? 
								ClampColorValue(value["a"].get<int>()) : 255;
					
					param = ComponentParameter::FromColor(r, g, b, a);
				}
				else
				{
					// Store as JSON string
					param = ComponentParameter::FromString(value.dump());
				}
			}
			else if (value.is_array())
			{
				// Check if it's a 2D or 3D vector array
				if (value.size() >= 2 && value.size() <= 3 && value[0].is_number())
				{
					float x = value[0].get<float>();
					float y = value[1].get<float>();
					
					if (value.size() == 3)
					{
						float z = value[2].get<float>();
						param = ComponentParameter::FromVector3(x, y, z);
					}
					else
					{
						param = ComponentParameter::FromVector2(x, y);
					}
				}
				// Check if it's a color array [r, g, b] or [r, g, b, a]
				else if (value.size() >= 3 && value.size() <= 4 && value[0].is_number())
				{
					uint8_t r = ClampColorValue(value[0].get<int>());
					uint8_t g = ClampColorValue(value[1].get<int>());
					uint8_t b = ClampColorValue(value[2].get<int>());
					uint8_t a = value.size() == 4 ? ClampColorValue(value[3].get<int>()) : 255;
					
					param = ComponentParameter::FromColor(r, g, b, a);
				}
				else
				{
					// Store as JSON string
					param = ComponentParameter::FromString(value.dump());
				}
			}
			else
			{
				// Unknown type, store as string
				param = ComponentParameter::FromString(value.dump());
			}
			
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
