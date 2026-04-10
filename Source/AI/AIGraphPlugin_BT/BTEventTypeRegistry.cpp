#include "BTEventTypeRegistry.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// Static member definitions
std::vector<BTEventTypeRegistry::EventTypeEntry> BTEventTypeRegistry::s_eventTypes;
std::map<std::string, size_t> BTEventTypeRegistry::s_eventTypeMap;
bool BTEventTypeRegistry::s_initialized = false;

bool BTEventTypeRegistry::Initialize(const std::string& filepath)
{
	if (s_initialized)
	{
		return true;
	}

	// Try to load from JSON
	if (LoadFromJSON(filepath))
	{
		s_initialized = true;
		std::cout << "[BTEventTypeRegistry] Successfully loaded " << s_eventTypes.size() 
		          << " event types from JSON" << std::endl;
		return true;
	}

	// Fallback to hardcoded defaults
	std::cout << "[BTEventTypeRegistry] Failed to load from JSON, using hardcoded defaults" << std::endl;
	InitializeDefaults();
	s_initialized = true;
	return false;
}

bool BTEventTypeRegistry::LoadFromJSON(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open())
	{
		std::cerr << "[BTEventTypeRegistry] Could not open file: " << filepath << std::endl;
		return false;
	}

	try
	{
		nlohmann::json jsonData;
		file >> jsonData;

		if (!jsonData.contains("eventTypes") || !jsonData["eventTypes"].is_array())
		{
			std::cerr << "[BTEventTypeRegistry] Invalid JSON: missing 'eventTypes' array" << std::endl;
			return false;
		}

		s_eventTypes.clear();
		s_eventTypeMap.clear();

		const auto& eventTypesArray = jsonData["eventTypes"];
		for (const auto& entry : eventTypesArray)
		{
			if (!entry.contains("id") || !entry.contains("displayName"))
			{
				std::cerr << "[BTEventTypeRegistry] Skipping invalid entry (missing required fields)" << std::endl;
				continue;
			}

			EventTypeEntry eventType;
			eventType.id = entry["id"].get<std::string>();
			eventType.displayName = entry["displayName"].get<std::string>();
			eventType.domain = entry.value("domain", "System");
			eventType.description = entry.value("description", "");
			eventType.category = entry.value("category", "Other");

			s_eventTypeMap[eventType.id] = s_eventTypes.size();
			s_eventTypes.push_back(eventType);

			std::cout << "[BTEventTypeRegistry] Loaded: " << eventType.displayName 
			          << " (" << eventType.category << ")" << std::endl;
		}

		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[BTEventTypeRegistry] JSON parsing error: " << e.what() << std::endl;
		return false;
	}
}

void BTEventTypeRegistry::InitializeDefaults()
{
	s_eventTypes.clear();
	s_eventTypeMap.clear();

	// Core defaults from system_consts.h
	std::vector<EventTypeEntry> defaults = {
		{"Olympe_EventType_Object_Activate", "Object Activate", "Gameplay", "Trigger object activation", "Objects"},
		{"Olympe_EventType_Object_Deactivate", "Object Deactivate", "Gameplay", "Trigger object deactivation", "Objects"},
		{"Olympe_EventType_Object_Create", "Object Create", "Gameplay", "Object created", "Objects"},
		{"Olympe_EventType_Object_Destroy", "Object Destroy", "Gameplay", "Object destroyed", "Objects"},
		{"Olympe_EventType_Object_CollideEvent", "Object Collide", "Collision", "Object collision event", "Collision"},
		{"Olympe_EventType_Level_Load", "Level Load", "System", "Level loaded", "Level"},
		{"Olympe_EventType_Level_Unload", "Level Unload", "System", "Level unloaded", "Level"},
		{"Olympe_EventType_Game_Pause", "Game Pause", "System", "Game paused", "Game"},
		{"Olympe_EventType_Game_Resume", "Game Resume", "System", "Game resumed", "Game"},
		{"Olympe_EventType_AI_Explosion", "AI Explosion", "Gameplay", "Explosion detected", "AI Stimulus"},
		{"Olympe_EventType_AI_Noise", "AI Noise", "Gameplay", "Noise detected", "AI Stimulus"},
		{"Olympe_EventType_AI_DamageDealt", "AI Damage", "Gameplay", "Damage dealt", "AI Stimulus"},
		{"Olympe_EventType_Keyboard_KeyDown", "Key Down", "Input", "Keyboard key pressed", "Input"},
		{"Olympe_EventType_Mouse_ButtonDown", "Mouse Button Down", "Input", "Mouse button pressed", "Input"},
		{"Olympe_EventType_Camera_Shake", "Camera Shake", "Camera", "Start camera shake", "Camera"},
		{"Olympe_EventType_Menu_Enter", "Menu Enter", "UI", "Menu entered", "Menu"},
	};

	for (const auto& entry : defaults)
	{
		s_eventTypeMap[entry.id] = s_eventTypes.size();
		s_eventTypes.push_back(entry);
	}

	std::cout << "[BTEventTypeRegistry] Initialized " << s_eventTypes.size() << " default event types" << std::endl;
}

const std::vector<BTEventTypeRegistry::EventTypeEntry>& BTEventTypeRegistry::GetAllEventTypes()
{
	if (!s_initialized)
	{
		Initialize("./Gamedata/BehaviorTree/EventTypes.json");
	}
	return s_eventTypes;
}

const BTEventTypeRegistry::EventTypeEntry* BTEventTypeRegistry::GetEventTypeEntry(const std::string& id)
{
	if (!s_initialized)
	{
		Initialize("./Gamedata/BehaviorTree/EventTypes.json");
	}

	auto it = s_eventTypeMap.find(id);
	if (it != s_eventTypeMap.end() && it->second < s_eventTypes.size())
	{
		return &s_eventTypes[it->second];
	}
	return nullptr;
}

std::string BTEventTypeRegistry::GetDisplayName(const std::string& id)
{
	const auto* entry = GetEventTypeEntry(id);
	return entry ? entry->displayName : id;
}

std::string BTEventTypeRegistry::GetDomain(const std::string& id)
{
	const auto* entry = GetEventTypeEntry(id);
	return entry ? entry->domain : "System";
}

std::string BTEventTypeRegistry::GetCategory(const std::string& id)
{
	const auto* entry = GetEventTypeEntry(id);
	return entry ? entry->category : "Other";
}

std::vector<BTEventTypeRegistry::EventTypeEntry> BTEventTypeRegistry::GetEventTypesByCategory(const std::string& category)
{
	std::vector<EventTypeEntry> result;
	for (const auto& entry : GetAllEventTypes())
	{
		if (entry.category == category)
		{
			result.push_back(entry);
		}
	}
	return result;
}

std::vector<std::string> BTEventTypeRegistry::GetAllCategories()
{
	std::vector<std::string> categories;
	for (const auto& entry : GetAllEventTypes())
	{
		if (std::find(categories.begin(), categories.end(), entry.category) == categories.end())
		{
			categories.push_back(entry.category);
		}
	}
	std::sort(categories.begin(), categories.end());
	return categories;
}

bool BTEventTypeRegistry::Reload(const std::string& filepath)
{
	s_initialized = false;
	return Initialize(filepath);
}

bool BTEventTypeRegistry::IsValid(const std::string& id)
{
	if (!s_initialized)
	{
		Initialize("./Gamedata/BehaviorTree/EventTypes.json");
	}
	return s_eventTypeMap.find(id) != s_eventTypeMap.end();
}
