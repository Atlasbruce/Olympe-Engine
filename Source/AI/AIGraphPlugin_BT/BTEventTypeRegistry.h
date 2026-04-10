#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include "../../third_party/nlohmann/json.hpp"

// Forward declarations
enum class EventType;
enum class EventDomain;

/**
 * @brief Registry for BT-accessible event types loaded from JSON configuration
 * Mirrors BTNodeRegistry pattern for consistency
 */
class BTEventTypeRegistry
{
public:
	/**
	 * @brief Event type entry from JSON configuration
	 */
	struct EventTypeEntry
	{
		std::string id;               // e.g., "Olympe_EventType_AI_Explosion"
		std::string displayName;      // e.g., "AI Explosion"
		std::string domain;           // e.g., "Gameplay"
		std::string description;      // e.g., "Explosion detected at position"
		std::string category;         // e.g., "AI Stimulus"
	};

	/**
	 * @brief Initialize registry from JSON file with fallback
	 * @param filepath Path to EventTypes.json (e.g., "./Gamedata/BehaviorTree/EventTypes.json")
	 * @return true if loaded successfully (JSON or fallback)
	 */
	static bool Initialize(const std::string& filepath);

	/**
	 * @brief Get all registered event types
	 */
	static const std::vector<EventTypeEntry>& GetAllEventTypes();

	/**
	 * @brief Get event type entry by id
	 * @param id Event type identifier (e.g., "Olympe_EventType_AI_Explosion")
	 * @return pointer to entry, nullptr if not found
	 */
	static const EventTypeEntry* GetEventTypeEntry(const std::string& id);

	/**
	 * @brief Get display name for event type id
	 */
	static std::string GetDisplayName(const std::string& id);

	/**
	 * @brief Get domain for event type id
	 */
	static std::string GetDomain(const std::string& id);

	/**
	 * @brief Get category for event type id
	 */
	static std::string GetCategory(const std::string& id);

	/**
	 * @brief Get all event types for a specific category
	 */
	static std::vector<EventTypeEntry> GetEventTypesByCategory(const std::string& category);

	/**
	 * @brief Get all unique categories
	 */
	static std::vector<std::string> GetAllCategories();

	/**
	 * @brief Reload registry from file (for live editing)
	 */
	static bool Reload(const std::string& filepath);

	/**
	 * @brief Check if event type is registered
	 */
	static bool IsValid(const std::string& id);

private:
	/**
	 * @brief Load event types from JSON file
	 */
	static bool LoadFromJSON(const std::string& filepath);

	/**
	 * @brief Initialize default hardcoded event types (fallback)
	 */
	static void InitializeDefaults();

	static std::vector<EventTypeEntry> s_eventTypes;
	static std::map<std::string, size_t> s_eventTypeMap;  // id -> index
	static bool s_initialized;
};
