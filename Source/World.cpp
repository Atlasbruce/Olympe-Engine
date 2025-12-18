#include "World.h"

#include "ECS_Systems.h"
#include "ECS_Components.h"

void World::RegisterDefaultSystems() {
	// Grid must render behind sprites, so register it before RenderingSystem.
	// (Assumes downstream pipeline calls systems in registration order.)
	this->ecs.RegisterSystem<GridSystem>();
	this->ecs.RegisterSystem<RenderingSystem>();

	// ... other systems

	// Ensure a GridSettings singleton exists
	Entity grid = this->ecs.GetSingleton<GridSettings_data>();
	if (!grid.IsValid()) {
		Entity e = this->ecs.CreateEntity();
		this->ecs.AddComponent<GridSettings_data>(e, GridSettings_data{});
	}
}
