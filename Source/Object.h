/*
Olympe Engine V2 2025
Nicolas Chereau
nchereau@gmail.com

Purpose:
- Class representing a generic object in a game engine, with enumerations for object types and states.

*/
#pragma once
#include <string>
#include "system/message.h"
#include <chrono>
#include <cstdint>

using namespace std;

enum class ObjectType
{
	None = 0,
	Singleton, // e.g. GameEngine, ResourceManager, EventManager, etc.
	Entity, // Player, NPC, Item, etc.
	Component, // Physics, AI, Render, Audio, etc.
	Level, // Game level or environment
	Sector, // Subdivision of a level
	GraphicMap, // Tilemap or similar
	CollisionMap, // Collision data
	Count
};

class Object
{
public:
	Object()
	{
		// generate a unique id based on current time in nanoseconds
		using namespace std::chrono;
		auto now = system_clock::now();
		uid = static_cast<uint64_t>(duration_cast<nanoseconds>(now.time_since_epoch()).count());
	}
	virtual ~Object() {};

public:

	std::string name = "unnamed_object";
	// virtual ObjectType GetObjectType() const { return ObjectType::None; }

	virtual void Process() {};
	virtual void Render() {};
	virtual void OnEvent(const Message& msg) {};

	inline uint64_t GetUID() const { return uid; }

protected:
	// Object UID
	uint64_t uid = 0;
};


