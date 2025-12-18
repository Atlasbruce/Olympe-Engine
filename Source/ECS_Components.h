#pragma once

#include "Config.h"

#include <string>
#include <iostream>
#include <SDL2/SDL.h>

// =============================================
// Existing Components
// =============================================

// --- additional components may be declared below ---

// =============================================
// Grid (rendered behind sprites)
// =============================================

// Projection mode used by GridSystem.
// Ortho: regular rectangular grid.
// Iso:   diamond isometric grid.
// Hex:   pointy-top axial-to-screen projection.
enum class GridProjection : uint8_t {
	Ortho = 0,
	Iso   = 1,
	Hex   = 2
};

// Singleton component controlling grid rendering.
// The grid is intended to be rendered behind sprites.
struct GridSettings_data {
	bool enabled = true;

	GridProjection projection = GridProjection::Ortho;

	// Size settings (meaning depends on projection)
	// Ortho: cell_w/cell_h in world units.
	// Iso:   tile_w/tile_h in world units.
	// Hex:   hex_size = cell_w (radius), cell_h unused.
	float cell_w = 32.0f;
	float cell_h = 32.0f;

	// Render parameters
	SDL_Color color = SDL_Color{ 60, 60, 60, 255 };
	int line_thickness = 1; // currently used as 1px lines; kept for future support

	// Limits (in cells) around the camera center to render
	int half_extent_x = 50;
	int half_extent_y = 50;
};
