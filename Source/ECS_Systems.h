#pragma once

#include "Config.h"
#include "ECS.h"

// Forward declarations for render subsystems
class RenderingSystem;

// =============================================
// Existing Systems
// =============================================

// --- additional systems may be declared below ---

// =============================================
// Grid System
// =============================================

// Renders a debug/editor grid behind sprites, per-viewport using the active ECS cameras.
class GridSystem : public System {
public:
	GridSystem() = default;
	~GridSystem() override = default;

	// Called by World rendering pipeline. Must be executed BEFORE RenderingSystem.
	void Render(World* world, SDL_Renderer* renderer);
};
