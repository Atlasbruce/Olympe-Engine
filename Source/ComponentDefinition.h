/*
Olympe Engine V2 - 2025
Nicolas Chereau
nchereau@gmail.com

This file is part of Olympe Engine V2.

ComponentDefinition purpose: Provides dynamic component parameter storage and JSON parsing
for entity components. Supports type-safe parameter access and conversions.
*/

#pragma once

#include <string>
#include <map>
#include <memory>
#include <SDL3/SDL.h>
#include "vector.h"
#include "ECS_Entity.h"

// Forward declaration
namespace nlohmann {
	class json;
}

struct ComponentParameter
{
	enum class Type : uint8_t
	{
		Unknown = 0,
		Bool,
		Int,
		Float,
		String,
		Vector2,
		Vector3,
		Color,
		Array,
		EntityRef
	};

	Type type = Type::Unknown;

	// Value storage
	bool boolValue = false;
	int intValue = 0;
	float floatValue = 0.0f;
	std::string stringValue;
	Vector vectorValue;
	SDL_Color colorValue = { 255, 255, 255, 255 };
	EntityID entityRefValue = INVALID_ENTITY_ID;

	// Factory methods
	static ComponentParameter FromBool(bool value);
	static ComponentParameter FromInt(int value);
	static ComponentParameter FromFloat(float value);
	static ComponentParameter FromString(const std::string& value);
	static ComponentParameter FromVector2(float x, float y);
	static ComponentParameter FromVector3(float x, float y, float z);
	static ComponentParameter FromColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
	static ComponentParameter FromEntityRef(EntityID entityId);

	// Type conversion helpers with fallback logic
	bool AsBool() const;
	int AsInt() const;
	float AsFloat() const;
	std::string AsString() const;
	Vector AsVector() const;
	SDL_Color AsColor() const;
	EntityID AsEntityRef() const;
};

struct ComponentDefinition
{
	std::string componentType;
	std::map<std::string, ComponentParameter> parameters;

	// JSON parsing with type detection heuristics
	static ComponentDefinition FromJSON(const nlohmann::json& jsonObj);

	// Parameter access
	bool HasParameter(const std::string& name) const;
	const ComponentParameter* GetParameter(const std::string& name) const;
};
